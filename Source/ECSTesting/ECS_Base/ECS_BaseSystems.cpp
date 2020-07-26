#include "ECS_BaseSystems.h"
#include "ECS_BattleComponents.h"

#include "SystemTasks.h"

DECLARE_CYCLE_STAT(TEXT("ECS: Instance Mesh Prepare"), STAT_InstancedMeshPrepare, STATGROUP_ECS);
DECLARE_CYCLE_STAT(TEXT("ECS: Instance Mesh Draw"), STAT_InstancedMeshDraw, STATGROUP_ECS);
DECLARE_CYCLE_STAT(TEXT("ECS: Instance Mesh Clean"), STAT_InstancedMeshClean, STATGROUP_ECS);

StaticMeshDrawSystem::ISMData* StaticMeshDrawSystem::GetInstancedMeshForMesh(UStaticMesh* mesh)
{
	auto find = MeshMap.Find(mesh);
	if (find)
	{
		return find;
	}
	else
	{
		UInstancedStaticMeshComponent* NewComp = NewObject<UInstancedStaticMeshComponent>(OwnerActor, UInstancedStaticMeshComponent::StaticClass());
		if (!NewComp)
		{
			return NULL;
		}

		NewComp->RegisterComponent();
		NewComp->SetStaticMesh(mesh);
		NewComp->SetWorldLocation(FVector(0.0, 0.0, 0.0));
		NewComp->SetCastShadow(false);
		NewComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		NewComp->SetWorldScale3D(FVector(1.0, 1.0, 1.0));
		NewComp->SetCanEverAffectNavigation(false);

		ISMData NewData;
		NewData.ISM = NewComp;
		NewData.rendered = 0;
		auto& d = MeshMap.Add(mesh, NewData);

		return &d;
	}
}

void StaticMeshDrawSystem::update(ECS_Registry &registry, float dt)
{
}

void  StaticMeshDrawSystem::schedule(ECSSystemScheduler* sysScheduler)
{
	SystemTaskBuilder builder(this->name, 1500, sysScheduler);

	float dt = 1.0 / 60.0;

	{

	TaskDependencies deps;
	deps.AddWrite<FInstancedStaticMesh>();
	deps.AddRead<FPosition>();
	deps.AddRead<FRotationComponent>();
	deps.AddRead<FScale>();

	builder.AddDependency("Movement");

	builder.AddTask(TaskDependencies{}, [=](ECS_Registry& reg) {
			SCOPE_CYCLE_COUNTER(STAT_InstancedMeshPrepare);
			//copy transforms into the ISM RenderTransform
			reg.view<FInstancedStaticMesh, FPosition>().each([&, dt](auto entity, FInstancedStaticMesh& ism, FPosition& pos) {
				ism.RenderTransform.SetLocation(pos.pos);
				});
			reg.view<FInstancedStaticMesh, FRotationComponent>().each([&, dt](auto entity, FInstancedStaticMesh& ism, FRotationComponent& rot) {
				ism.RenderTransform.SetRotation(rot.rot);
				});
			reg.view<FInstancedStaticMesh, FScale>().each([&, dt](auto entity, FInstancedStaticMesh& ism, FScale& sc) {
				ism.RenderTransform.SetScale3D(sc.scale);
				});	
	});

	}
	{
		TaskDependencies deps;		

		deps.AddRead<FInstancedStaticMesh>();

		//builder.AddGameTask(deps,
		builder.AddTask(deps,
			[=](ECS_Registry& reg) {
				this->Elapsed -= dt;
				if (this->Elapsed < 0)
				{
					this->Elapsed = 1;
					for (auto i : MeshMap)
					{
						i.Value.ISM->ClearInstances();
					}
				}

				for (auto &i : MeshMap)
				{
					i.Value.rendered = 0;
				}

				{
					SCOPE_CYCLE_COUNTER(STAT_InstancedMeshDraw);

					auto view = reg.view<FInstancedStaticMesh>();

					for (auto entity : view) {

						FInstancedStaticMesh& mesh = view.get(entity);

						auto MeshData = GetInstancedMeshForMesh(mesh.mesh);
						auto RenderMesh = MeshData->ISM;

						const FTransform& RenderTransform = mesh.RenderTransform;

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
				{
					SCOPE_CYCLE_COUNTER(STAT_InstancedMeshClean)
						for (auto& i : MeshMap)
						{
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
			}
		);
	}

	sysScheduler->AddTaskgraph(builder.FinishGraph());
}

EntityHandle ArchetypeSpawnerSystem::SpawnFromArchetype(ECS_Registry& registry, TSubclassOf<AECS_Archetype>& ArchetypeClass)
{
	//try to find the spawn archetype in the map, spawn a new one if not found, use it to initialize entity

	auto Found = Archetypes.Find(ArchetypeClass);
	AECS_Archetype* FoundArchetype = nullptr;
	if (!Found)
	{
		FoundArchetype = OwnerActor->GetWorld()->SpawnActor<AECS_Archetype>(ArchetypeClass);
		UE_LOG(LogFlying, Warning, TEXT("Spawned archetype: %s"), *GetNameSafe(FoundArchetype));
		if (!FoundArchetype)
		{
			UE_LOG(LogFlying, Warning, TEXT("Error when spawning archetype: %s"), *GetNameSafe(ArchetypeClass));
		}
		else
		{
			Archetypes.Emplace(ArchetypeClass, FoundArchetype);
		}
	}
	else
	{
		FoundArchetype = *Found;
	}
	if (FoundArchetype)
	{
		return FoundArchetype->CreateNewEntityFromThis(World);
	}
	else
	{
		UE_LOG(LogFlying, Warning, TEXT("Failed new Entity: %s"), *GetNameSafe(ArchetypeClass));
	}

	return EntityHandle();
}

void ArchetypeSpawnerSystem::update(ECS_Registry &registry, float dt)
{
	
}
PRAGMA_DISABLE_OPTIMIZATION
void ArchetypeSpawnerSystem::schedule(ECSSystemScheduler* sysScheduler)
{
	SystemTaskBuilder builder(this->name, 1000000, sysScheduler,0.1);
	float dt = 1.0 / 60.0;
	
	TaskDependencies deps;

	deps.AddWrite<FArchetypeSpawner>();
	builder.AddDependency("EndBarrier");
	builder.AddTask(TaskDependencies{}, 
	//builder.AddSyncTask(
		[=](ECS_Registry& reg) {

		SCOPE_CYCLE_COUNTER(STAT_ECSSpawn);

		//exclusively update timing
		auto SpawnerView = reg.view<FArchetypeSpawner>();
		for (auto e : SpawnerView)
		{
			SpawnerView.get(e).TimeUntilSpawn -= dt;
		}
	});
	

	builder.AddSyncTask(
		[=](ECS_Registry& reg) {
			//return;
			SCOPE_CYCLE_COUNTER(STAT_ECSSpawn);
		//spawn from arc and actortransform
		auto SpawnerArcView = reg.view<FArchetypeSpawner, FRandomArcSpawn, FActorTransform>();
		for (auto e : SpawnerArcView)
		{
			const FTransform& ActorTransform = SpawnerArcView.get<FActorTransform>(e).transform;
			FArchetypeSpawner& spawner = SpawnerArcView.get<FArchetypeSpawner>(e);
			const FRandomArcSpawn& arc = SpawnerArcView.get<FRandomArcSpawn>(e);

			if (spawner.TimeUntilSpawn < 0)
			{
				if (spawner.ArchetypeClass)
				{
					EntityHandle h = SpawnFromArchetype(reg, spawner.ArchetypeClass);
					reg.accommodate<FPosition>(h.handle, ActorTransform.GetLocation());

					if (reg.has<FFaction>(e))
					{
						reg.accommodate<FFaction>(h.handle, reg.get<FFaction>(e));
					}

					FVelocity vel;
					const float VelMagnitude = World->rng.FRandRange(arc.MinVelocity, arc.MaxVelocity);
					const float Arc = FMath::DegreesToRadians(World->rng.FRandRange(arc.MinAngle, arc.MaxAngle));


					FVector ArcVel = World->rng.VRandCone(FVector(1.0, 0.0, 0.0), Arc) * VelMagnitude;

					FVector BulletVelocity = ActorTransform.GetRotation().RotateVector(ArcVel);
					reg.accommodate<FVelocity>(h.handle, BulletVelocity);
				}

				if (spawner.bLoopSpawn)
				{
					spawner.TimeUntilSpawn = spawner.SpawnRate;
				}
				else
				{
					reg.remove<FArchetypeSpawner>(e);
				}
			}
		}

		//Spawn with basic position
		auto SpawnerPositionView = reg.view<FArchetypeSpawner>();
		for (auto e : SpawnerPositionView)
		{
			assert(reg.has<FArchetypeSpawner>(e));
			assert(reg.has<FPosition>(e));

			const FVector& SpawnPosition = reg.get<FPosition>(e).pos;
			FArchetypeSpawner& spawner = reg.get<FArchetypeSpawner>(e);

			
			if (spawner.TimeUntilSpawn < 0)
			{
				if (IsValid(spawner.ArchetypeClass))
				{
					EntityHandle h = SpawnFromArchetype(reg, spawner.ArchetypeClass);
					reg.accommodate<FPosition>(h.handle, SpawnPosition);
				}

				if (spawner.bLoopSpawn)
				{
					spawner.TimeUntilSpawn = spawner.SpawnRate;
				}
				else
				{
					reg.remove<FArchetypeSpawner>(e);
				}
			}
		}
		}
	);

	sysScheduler->AddTaskgraph(builder.FinishGraph());
}
PRAGMA_ENABLE_OPTIMIZATION
void RaycastSystem::update(ECS_Registry &registry, float dt)
{
	
}

void RaycastSystem::CheckRaycasts(ECS_Registry& registry, float dt, UWorld* GameWorld)
{
	rayUnits.Reset();
	//check all the raycast results from the async raycast
	registry.view<FRaycastResult>().each([&, dt](auto entity, FRaycastResult& ray) {

		if (GameWorld->IsTraceHandleValid(ray.handle, false))
		{
			rayUnits.Add({ entity, &ray });
		}

	});

	ParallelFor(rayUnits.Num(), [&](auto i) {

		auto ray = *rayUnits[i].ray;
		auto entity = rayUnits[i].et;

		FTraceDatum tdata;
		GameWorld->QueryTraceData(ray.handle, tdata);
		if (tdata.OutHits.IsValidIndex(0))
		{
			//it actually hit
			if (tdata.OutHits[0].bBlockingHit)
			{
				////if its an actor, try to damage it. 
				AActor* act = tdata.OutHits[0].GetActor();
				if (act)
				{
					actorCalls.enqueue({ act });
				}

				//if the entity was a projectile, create explosion and destroy it
				if (registry.has<FProjectile>(entity))
				{
					FVector ExplosionPoint = tdata.OutHits[0].ImpactPoint;

					explosions.enqueue({ entity,ExplosionPoint });
				}
			}
		}		
	});
}

void RaycastSystem::CreateExplosion(ECS_Registry& registry, EntityID entity, FVector ExplosionPoint)
{
	auto explosionclass = registry.get<FProjectile>(entity).ExplosionArchetypeClass;
	if (explosionclass)
	{
		//create new entity to spawn explosion
		auto h = registry.create();
		registry.assign<FPosition>(h);
		registry.assign<FLifetime>(h);
		registry.assign<FArchetypeSpawner>(h);
		registry.get<FPosition>(h).pos = ExplosionPoint;
		registry.get<FLifetime>(h).LifeLeft = 0.1;
		FArchetypeSpawner& spawn = registry.get<FArchetypeSpawner>(h);
		spawn.bLoopSpawn = false;
		spawn.ArchetypeClass = explosionclass;
		spawn.SpawnRate = 1;
		spawn.TimeUntilSpawn = 0.0;
		spawn.Canary = 69;
	}

	//registry.accommodate<FDestroy>(entity);
}

DECLARE_CYCLE_STAT(TEXT("ECS: Raycast BP"), STAT_RaycastBP, STATGROUP_ECS);
DECLARE_CYCLE_STAT(TEXT("ECS: Raycast Explosions"), STAT_RaycastExplosions, STATGROUP_ECS);
DECLARE_CYCLE_STAT(TEXT("ECS: Raycast Enqueue"), STAT_RaycastResults, STATGROUP_ECS);
void  RaycastSystem::schedule(ECSSystemScheduler* sysScheduler)
{
	SystemTaskBuilder builder("RayCheck", 999, sysScheduler);

	TaskDependencies deps1;
	float dt = 1.0 / 60.0;
	deps1.AddRead<FRaycastResult>();

	//builder.AddGameTask(deps,
	builder.AddTask(deps1, [=](ECS_Registry& reg) {

		SCOPE_CYCLE_COUNTER(STAT_ECSRaycast);
		UWorld* GameWorld = OwnerActor->GetWorld();
		this->CheckRaycasts(reg, dt,GameWorld);
	});

	sysScheduler->AddTaskgraph(builder.FinishGraph());
	TaskDependencies deps2;

	deps2.AddWrite< FRaycastResult >();
	deps2.AddRead<FMovementRaycast>();
	deps2.AddRead<FPosition>();
	deps2.AddRead<FLastPosition>();


	SystemTaskBuilder builder_ray(this->name, 999, sysScheduler,2.5);

	//builder_ray.SetPriority(2.5);
	builder_ray.AddGameTask(deps2,[=](ECS_Registry& reg) {
	//builder.AddSyncTask(/*deps2,*/ [=](ECS_Registry& reg) {
		SCOPE_CYCLE_COUNTER(STAT_RaycastResults);

		UWorld* GameWorld = OwnerActor->GetWorld();
		//movement raycast needs a "last position" component
		reg.view<FMovementRaycast, FPosition, FLastPosition>().each([&, dt](auto entity, FMovementRaycast& ray, FPosition& pos, FLastPosition& lastPos) {

			if (pos.pos != lastPos.pos)
			{
				FTraceHandle hit = GameWorld->AsyncLineTraceByChannel(EAsyncTraceType::Single, lastPos.pos, pos.pos, ray.RayChannel);

				reg.accommodate<FRaycastResult>(entity, hit);
			}
		});
	});

	builder_ray.AddDependency("RayCheck");
	builder_ray.AddDependency("Movement");

	sysScheduler->AddTaskgraph(builder_ray.FinishGraph());
		
	SystemTaskBuilder builder2("RayExplosions", LifetimeSystem::DeletionSync-1, sysScheduler);
	builder2.AddSyncTask(
		[=](ECS_Registry& reg) {

			SCOPE_CYCLE_COUNTER(STAT_RaycastExplosions);

			DeletionContext* del = DeletionContext::GetFromRegistry(reg);

			bulk_dequeue(explosions, [&reg,&del,this](const ExplosionStr& ex) {
				if (reg.has<FProjectile>(ex.et))
				{
					CreateExplosion(reg, ex.et, ex.explosionPoint);
				}
				del->AddToQueue(ex.et);
			});			
		}
	);
	builder2.AddDependency("EndBarrier");

	SystemTaskBuilder builder3("raycast system: broadcast BP", 0, sysScheduler);
	builder3.AddDependency(this->name);
	builder3.AddGameTask(TaskDependencies {},
		[=](ECS_Registry& reg) {

			SCOPE_CYCLE_COUNTER(STAT_RaycastBP);

			bulk_dequeue(actorCalls, [](const ActorBpCall& a) {
				AActor* act = a.actor.Get();
				if (act) {

					auto hcmp = act->FindComponentByClass<UECS_HealthComponentWrapper>();
					if (hcmp)
					{
						hcmp->OnDamaged.Broadcast(99.0);
					}
				}
			});				
			
			
		}, ESysTaskFlags::NoECS
	);
	builder3.AddDependency("EndBarrier");
	sysScheduler->AddTaskgraph(builder2.FinishGraph());

	sysScheduler->AddTaskgraph(builder3.FinishGraph());
}

void LifetimeSystem::update(ECS_Registry& registry, float dt)
{
	
}



DECLARE_CYCLE_STAT(TEXT("ECS: Lifetime count"), STAT_LifeCount, STATGROUP_ECS);
DECLARE_CYCLE_STAT(TEXT("ECS: Lifetime Delete"), STAT_LifeDelete, STATGROUP_ECS);

void  LifetimeSystem::schedule(ECSSystemScheduler* sysScheduler)
{
	SystemTaskBuilder builder_end("EndBarrier", 100000, sysScheduler);

	builder_end.AddDependency("Movement");
	builder_end.AddTask(
		TaskDependencies{},
		[=](ECS_Registry& reg) {			
		}
	);

	sysScheduler->AddTaskgraph(builder_end.FinishGraph());

	SystemTaskBuilder builder("lifetime system", 100000, sysScheduler);

	DeletionContext::GetFromRegistry(*sysScheduler->registry);

	TaskDependencies deps;
	deps.AddWrite<FLifetime>();

	builder.AddDependency("EndBarrier");
	builder.AddTask(
		deps,
		[=](ECS_Registry& reg) {
			SCOPE_CYCLE_COUNTER(STAT_LifeCount);
			DeletionContext* del = DeletionContext::GetFromRegistry(reg);

			//tick the lifetime timers
			auto LifetimeView = reg.view<FLifetime>();
			for (auto e : LifetimeView)
			{
				auto& Deleter = LifetimeView.get(e);

				Deleter.LifeLeft -= 1.0/60.0;

				if (Deleter.LifeLeft < 0)
				{
					del->AddToQueue(e);
				}
			}
		}
	);

	sysScheduler->AddTaskgraph(builder.FinishGraph());

	//SystemTaskBuilder builder2("lifetime system- Delete", LifetimeSystem::DeletionSync, sysScheduler);
	SystemTaskBuilder builder2("lifetime system- Delete", 0, sysScheduler, 0.1);
	builder2.AddDependency("lifetime system");
	builder2.AddSyncTask(
		[=](ECS_Registry& reg) {			

			SCOPE_CYCLE_COUNTER(STAT_LifeDelete);
			DeletionContext* del = DeletionContext::GetFromRegistry(reg);

			bulk_dequeue(del->entitiesToDelete, [&](EntityID id) {
				if (reg.valid(id))
				{
					reg.destroy(id);
				}
			});
		}
	);

	sysScheduler->AddTaskgraph(builder2.FinishGraph());
}

void CopyTransformToActorSystem::update(ECS_Registry& registry, float dt)
{
}

void CopyTransformToActorSystem::PackTransforms(ECS_Registry& registry)
{
	SCOPE_CYCLE_COUNTER(STAT_PackActorTransform);
	{

		//fill ActorTransform from separate components		
		registry.view<FActorTransform, FPosition>().each([&](auto entity, FActorTransform& transform, FPosition& pos) {
			transform.transform.SetLocation(pos.pos);
			});
		registry.view<FActorTransform, FRotationComponent>().each([&](auto entity, FActorTransform& transform, FRotationComponent& rot) {
			transform.transform.SetRotation(rot.rot);
			});
		registry.view<FActorTransform, FScale>().each([&](auto entity, FActorTransform& transform, FScale& sc) {

			transform.transform.SetScale3D(sc.scale);
			});
	}

	//copy transforms from actor into FActorTransform	
	auto TransformView = registry.view<FCopyTransformToActor, FActorReference, FActorTransform>();
	for (auto e : TransformView)
	{
		const FTransform& transform = TransformView.get<FActorTransform>(e).transform;
		FActorReference& actor = TransformView.get<FActorReference>(e);

		if (actor.ptr.IsValid())
		{
			//actor.ptr->SetActorTransform(transform);
			transforms.Add({ actor.ptr,transform });
		}
	}
}

void  CopyTransformToActorSystem::schedule(ECSSystemScheduler* sysScheduler)
{
	SystemTaskBuilder builder(this->name, 10000, sysScheduler);

	TaskDependencies deps1;
	deps1.AddWrite < FActorTransform>();
	deps1.AddRead<FScale>();
	deps1.AddRead<FPosition>();
	deps1.AddRead<FRotationComponent>();
	deps1.AddRead<FActorReference>();

	builder.AddTask(deps1,
	//builder.AddSyncTask(
		[=](ECS_Registry& reg) {
			PackTransforms(reg);
		}
	);

	builder.AddDependency("Movement");

	SystemTaskBuilder builder2("Movement Apply", 11000, sysScheduler);

	builder2.AddGameTask(deps1,
		//builder.AddSyncTask(
		[=](ECS_Registry& reg) {
			SCOPE_CYCLE_COUNTER(STAT_CopyTransformActor);
			for (auto& a : transforms) {
				if (a.actor.IsValid())
				{
					a.actor->SetActorTransform(a.transform);
				}
			}
			transforms.Reset();
		},ESysTaskFlags::NoECS
	);
	builder2.AddDependency(this->name);
	builder.AddDependency("Movement");
	sysScheduler->AddTaskgraph(builder.FinishGraph());
	sysScheduler->AddTaskgraph(builder2.FinishGraph());
}

void CopyTransformToECSSystem::update(ECS_Registry& registry, float dt)
{
}

void CopyTransformToECSSystem::schedule(ECSSystemScheduler* sysScheduler)
{
	SystemTaskBuilder builder(this->name, 100, sysScheduler);

	TaskDependencies deps1;
	deps1.AddWrite < FActorTransform>();
	deps1.AddRead<FCopyTransformToECS>();
	deps1.AddRead<FActorReference>();

	builder.AddGameTask(deps1,
	//builder.AddSyncTask(//deps1,
		[=](ECS_Registry& reg) {
			SCOPE_CYCLE_COUNTER(STAT_CopyTransformECS);
			//copy transforms from actor into FActorTransform
			auto ActorTransformView = reg.view<FCopyTransformToECS, FActorReference>();
			for (auto e : ActorTransformView)
			{
				if (!reg.has<FActorTransform>()) {

					
					FActorReference& actor = ActorTransformView.get<FActorReference>(e);
					if (actor.ptr.IsValid())
					{
						const FTransform& ActorTransform = actor.ptr->GetActorTransform();
						reg.accommodate<FActorTransform>(e, ActorTransform);
					}
				}
			}
		}
	);


	TaskDependencies deps;
	deps.AddWrite<FPosition>();
	deps.AddWrite < FVelocity>();
	deps.AddWrite < FScale>();
	deps.AddRead<FActorTransform>();
		
	builder.AddTask(
			deps,
			[=](ECS_Registry& reg) {

				SCOPE_CYCLE_COUNTER(STAT_UnpackActorTransform);
				//unpack from ActorTransform into the separate transform components, only if the entity does have that component
				reg.view<FActorTransform, FPosition>().each([&](auto entity, FActorTransform& transform, FPosition& pos) {

		pos.pos = transform.transform.GetLocation();
		});
				reg.view<FActorTransform, FRotationComponent>().each([&](auto entity, FActorTransform& transform, FRotationComponent& rot) {

		rot.rot = transform.transform.GetRotation();
		});
				reg.view<FActorTransform, FScale>().each([&](auto entity, FActorTransform& transform, FScale& sc) {

		sc.scale = transform.transform.GetScale3D();
		});
			}
	);

	sysScheduler->AddTaskgraph(builder.FinishGraph());
}

void MovementSystem::update(ECS_Registry& registry, float dt)
{
}

void  MovementSystem::schedule(ECSSystemScheduler* sysScheduler)
{
	SystemTaskBuilder builder(this->name, 300, sysScheduler);

	float dt = 1.0 / 60.0;

	TaskDependencies deps;
	deps.AddWrite<FPosition>();
	deps.AddWrite<FVelocity>();
	deps.AddWrite<FLastPosition>();
	deps.AddRead<FMovement>();
	deps.AddRead<FMovementRaycast>();

	builder.AddDependency("Boids");
	builder.AddTask(
		deps,
		[=](ECS_Registry& reg) {
			//movement raycast gets a "last position" component
			reg.view<FMovementRaycast, FPosition>().each([&, dt](auto entity, FMovementRaycast& ray, FPosition& pos) {
				reg.accommodate<FLastPosition>(entity, pos.pos);
			});

			//add gravity and basic movement from velocity
			reg.view<FMovement, FPosition, FVelocity>().each([&, dt](auto entity, FMovement& m, FPosition& pos, FVelocity& vel) {

				//gravity
				const FVector gravity = FVector(0.f, 0.f, -980) * m.GravityStrenght;
				vel.Add(gravity * dt);

				pos.Add(vel.vel * dt);
			});
		}
	);

	sysScheduler->AddTaskgraph(builder.FinishGraph());
}

void DebugDrawSystem::update(ECS_Registry& registry, float dt)
{
	assert(OwnerActor);
	elapsed -= dt;
	if (elapsed > 0)
	{
		return;
	}

	elapsed = UpdateRate;

	registry.view<FDebugSphere, FPosition>().each([&, dt](auto entity, FDebugSphere& ds, FPosition& pos) {

		SCOPE_CYCLE_COUNTER(STAT_DebugDraw);
		DrawDebugSphere(OwnerActor->GetWorld(), pos.pos, ds.radius, 12, ds.color, true, UpdateRate);
	});
}

void  DebugDrawSystem::schedule(ECSSystemScheduler* sysScheduler)
{
	SystemTaskBuilder builder(this->name, 10000, sysScheduler);

	TaskDependencies deps;
	deps.AddRead<FPosition>();
	
	deps.AddRead<FDebugSphere>();

	builder.AddGameTask(deps,
		[=](ECS_Registry& reg) {
			this->update(reg, 1.0 / 60.0);
		}
	);

	sysScheduler->AddTaskgraph(builder.FinishGraph());
}
