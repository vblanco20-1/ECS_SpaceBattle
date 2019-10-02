#include "ECS_BaseSystems.h"
#include "ECS_BattleComponents.h"

DECLARE_CYCLE_STAT(TEXT("ECS: Instance Mesh Prepare"), STAT_InstancedMeshPrepare, STATGROUP_ECS);
DECLARE_CYCLE_STAT(TEXT("ECS: Instance Mesh Draw"), STAT_InstancedMeshDraw, STATGROUP_ECS);
DECLARE_CYCLE_STAT(TEXT("ECS: Instance Mesh Clean"), STAT_InstancedMeshClean, STATGROUP_ECS);
void StaticMeshDrawSystem::update(ECS_Registry &registry, float dt)
{
	//reset instances once per second
	Elapsed -= dt;
	if (Elapsed < 0)
	{
		Elapsed = 1;
		for (auto i : MeshMap)
		{
			i.Value.ISM->ClearInstances();
		}			
	}	



	{
		SCOPE_CYCLE_COUNTER(STAT_InstancedMeshPrepare);
		//copy transforms into the ISM RenderTransform
		registry.view<FInstancedStaticMesh, FPosition>().each([&, dt](auto entity, FInstancedStaticMesh & ism, FPosition & pos) {
			ism.RenderTransform.SetLocation(pos.pos);
		});
		registry.view<FInstancedStaticMesh, FRotationComponent>().each([&, dt](auto entity, FInstancedStaticMesh & ism, FRotationComponent & rot) {
			ism.RenderTransform.SetRotation(rot.rot);
		});
		registry.view<FInstancedStaticMesh, FScale>().each([&, dt](auto entity, FInstancedStaticMesh & ism, FScale & sc) {
			ism.RenderTransform.SetScale3D(sc.scale);
		});

	}
	for (auto &i : MeshMap)
	{
		i.Value.rendered = 0;
	}

	{
		SCOPE_CYCLE_COUNTER(STAT_InstancedMeshDraw);
	
		auto view = registry.view<FInstancedStaticMesh>();

		for (auto entity : view) {
			
			FInstancedStaticMesh &mesh = view.get(entity);

			auto MeshData = GetInstancedMeshForMesh(mesh.mesh);
			auto RenderMesh = MeshData->ISM;

			const FTransform &RenderTransform = mesh.RenderTransform;// mesh.RelativeTransform.GetRelativeTransform(mesh.RenderTransform);

			if (RenderMesh->GetInstanceCount() < MeshData->rendered)
			{
				RenderMesh->AddInstanceWorldSpace(RenderTransform);
			}
			else
			{
				RenderMesh->UpdateInstanceTransform(MeshData->rendered, RenderTransform, true, false);
			}
			MeshData->rendered++;
		}

		
	}
	
	for (auto &i : MeshMap)
	{	
		SCOPE_CYCLE_COUNTER(STAT_InstancedMeshClean)

		FTransform nulltransform;
		nulltransform.SetScale3D(FVector(0.0, 0.0, 0.0));

		//if we have more instances than renderables, set the instances to null transform so they dont draw.
		//they will get cleanup once a second.
		if (IsValid(i.Value.ISM))
		{
			while (i.Value.rendered < i.Value.ISM->GetInstanceCount())
			{

				i.Value.rendered++;
				i.Value.ISM->UpdateInstanceTransform(i.Value.rendered, nulltransform, true, false);

			}
			i.Value.ISM->MarkRenderStateDirty();
		}
		
	}

}

void ArchetypeSpawnerSystem::update(ECS_Registry &registry, float dt)
{
	assert(OwnerActor);

	SCOPE_CYCLE_COUNTER(STAT_ECSSpawn);

	//exclusively update timing
	auto SpawnerView = registry.view<FArchetypeSpawner>();
	for (auto e : SpawnerView)
	{
		SpawnerView.get(e).TimeUntilSpawn -= dt;
	}

	//spawn from arc and actortransform
	auto SpawnerArcView = registry.view<FArchetypeSpawner, FRandomArcSpawn, FActorTransform>();
	for (auto e : SpawnerArcView)
	{
		const FTransform &ActorTransform = SpawnerArcView.get<FActorTransform>(e).transform;
		FArchetypeSpawner& spawner = SpawnerArcView.get<FArchetypeSpawner>(e);
		const FRandomArcSpawn& arc = SpawnerArcView.get<FRandomArcSpawn>(e);

		if (spawner.TimeUntilSpawn < 0)
		{
			if (spawner.ArchetypeClass)
			{
				EntityHandle h = SpawnFromArchetype(registry, spawner.ArchetypeClass);
				registry.accommodate<FPosition>(h.handle, ActorTransform.GetLocation());

				if (registry.has<FFaction>(e))
				{
					registry.accommodate<FFaction>(h.handle, registry.get<FFaction>(e));
				}

				FVelocity vel;
				const float VelMagnitude = World->rng.FRandRange(arc.MinVelocity, arc.MaxVelocity);
				const float Arc = FMath::DegreesToRadians(World->rng.FRandRange(arc.MinAngle, arc.MaxAngle));


				FVector ArcVel = World->rng.VRandCone(FVector(1.0, 0.0, 0.0), Arc) *VelMagnitude;

				FVector BulletVelocity = ActorTransform.GetRotation().RotateVector(ArcVel);
				registry.accommodate<FVelocity>(h.handle, BulletVelocity);
			}

			if (spawner.bLoopSpawn)
			{
				spawner.TimeUntilSpawn = spawner.SpawnRate;
			}
			else
			{
				registry.remove<FArchetypeSpawner>(e);
			}
		}
	}

	//Spawn with basic position
	auto SpawnerPositionView = registry.view<FArchetypeSpawner, FPosition>();
	for (auto e : SpawnerPositionView)
	{
		const FVector &SpawnPosition = SpawnerPositionView.get<FPosition>(e).pos;
		FArchetypeSpawner& spawner = SpawnerPositionView.get<FArchetypeSpawner>(e);

		if (spawner.TimeUntilSpawn < 0)
		{
			if (spawner.ArchetypeClass)
			{
				EntityHandle h = SpawnFromArchetype(registry, spawner.ArchetypeClass);
				registry.accommodate<FPosition>(h.handle, SpawnPosition);
			}

			if (spawner.bLoopSpawn)
			{
				spawner.TimeUntilSpawn = spawner.SpawnRate;
			}
			else
			{
				registry.remove<FArchetypeSpawner>(e);
			}
		}
	}		
}

void RaycastSystem::update(ECS_Registry &registry, float dt)
{
	assert(OwnerActor);
	UWorld * GameWorld = OwnerActor->GetWorld();
	SCOPE_CYCLE_COUNTER(STAT_ECSRaycast);

	//check all the raycast results from the async raycast
	registry.view<FRaycastResult>().each([&, dt](auto entity, FRaycastResult & ray) {
		

		if (GameWorld->IsTraceHandleValid(ray.handle, false))
		{			
			FTraceDatum tdata;
			GameWorld->QueryTraceData(ray.handle, tdata);
			if (tdata.OutHits.IsValidIndex(0))
			{
				//it actually hit
				if (tdata.OutHits[0].bBlockingHit)
				{
					//if its an actor, try to damage it. 
					AActor* act = tdata.OutHits[0].GetActor();
					if (act)
					{
						auto hcmp = act->FindComponentByClass<UECS_HealthComponentWrapper>();
						if (hcmp)
						{
							hcmp->OnDamaged.Broadcast(99.0);
						}
					}
					
					//if the entity was a projectile, create explosion and destroy it
					if (registry.has<FProjectile>(entity))
					{
						
						auto explosionclass = registry.get<FProjectile>(entity).ExplosionArchetypeClass;
						if (explosionclass)
						{
							//create new entity to spawn explosion
							auto h = registry.create();
							registry.assign<FPosition>(h);
							registry.assign<FLifetime>(h);
							registry.assign<FArchetypeSpawner>(h);
							registry.get<FPosition>(h).pos = tdata.OutHits[0].ImpactPoint;
							registry.get<FLifetime>(h).LifeLeft = 0.1;
							FArchetypeSpawner &spawn = registry.get<FArchetypeSpawner>(h);
							spawn.bLoopSpawn = false;
							spawn.ArchetypeClass = explosionclass;
							spawn.SpawnRate = 1;
							spawn.TimeUntilSpawn = 0.0;
						}

						registry.accommodate<FDestroy>(entity);
					}					
				}
			}
		}
	});

	//movement raycast needs a "last position" component
	registry.view<FMovementRaycast, FPosition, FLastPosition>().each([&, dt](auto entity, FMovementRaycast & ray, FPosition & pos, FLastPosition & lastPos) {
		
		if (pos.pos != lastPos.pos)
		{			
			FTraceHandle hit = GameWorld->AsyncLineTraceByChannel(EAsyncTraceType::Single, lastPos.pos, pos.pos, ray.RayChannel);
			
			registry.accommodate<FRaycastResult>(entity, hit);			
		}
	});	
}
