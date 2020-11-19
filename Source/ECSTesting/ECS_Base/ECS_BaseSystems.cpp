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
	SystemTaskBuilder builder("StaticDraws", 1500, sysScheduler);

	float dt = 1.0 / 60.0;


	init_query(q_transform, sysScheduler->registry);

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

			pack_transforms();
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
					q_transform.each([&](auto et, FInstancedStaticMesh& mesh) {


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
					});
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

void StaticMeshDrawSystem::pack_transforms()
{
	q_transform.iter([&](flecs::iter it, FInstancedStaticMesh* t) {

		if (it.has_column<FPosition>())
		{
			auto pos = it.table_column<const FPosition>();
			for (auto i : it)
			{
				t[i].RenderTransform.SetLocation(pos[i].pos);
			}
		}

		if (it.has_column<FRotationComponent>())
		{
			auto pos = it.table_column<const FRotationComponent>();
			for (auto i : it)
			{
				t[i].RenderTransform.SetRotation(pos[i].rot);
			}
		}
		if (it.has_column<FScale>())
		{
			auto pos = it.table_column<const FScale>();
			for (auto i : it)
			{
				t[i].RenderTransform.SetScale3D(pos[i].scale);
			}
		}
		});
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
//PRAGMA_DISABLE_OPTIMIZATION
void ArchetypeSpawnerSystem::schedule(ECSSystemScheduler* sysScheduler)
{
	SystemTaskBuilder builder("Spawner", 1000000, sysScheduler,0.1);
	float dt = 1.0 / 60.0;
	

	init_query(q_arcspawners, sysScheduler->registry);
	init_query(q_spawners, sysScheduler->registry);

	TaskDependencies deps;

	deps.AddWrite<FArchetypeSpawner>();
	builder.AddDependency("EndBarrier");
	builder.AddTask(TaskDependencies{}, 
	//builder.AddSyncTask(
		[=](ECS_Registry& reg) {

		SCOPE_CYCLE_COUNTER(STAT_ECSSpawn);

		//exclusively update timing
		q_spawners.each([dt](auto e, FArchetypeSpawner& spawner) {
			spawner.TimeUntilSpawn -= dt;
		});
	});
	

	builder.AddSyncTask(
		[=](ECS_Registry& reg) {
			//return;
			SCOPE_CYCLE_COUNTER(STAT_ECSSpawn);
		//spawn from arc and actortransform
			reg.defer_begin();
			
			q_arcspawners.each([&](auto e, FArchetypeSpawner& spawner, FRandomArcSpawn& arc, FActorTransform& ActorTransform) {
				
				if (spawner.TimeUntilSpawn < 0)
				{
					if (spawner.ArchetypeClass)
					{
						EntityHandle h = SpawnFromArchetype(reg, spawner.ArchetypeClass);
						flecs::entity spawned{ reg,h.handle };
						spawned.set<FPosition>({ ActorTransform.transform.GetLocation() });

						if (e.has<FFaction>())
						{
							spawned.set<FFaction>(*e.get<FFaction>());
						}

						FVelocity vel;
						const float VelMagnitude = World->rng.FRandRange(arc.MinVelocity, arc.MaxVelocity);
						const float Arc = FMath::DegreesToRadians(World->rng.FRandRange(arc.MinAngle, arc.MaxAngle));


						FVector ArcVel = World->rng.VRandCone(FVector(1.0, 0.0, 0.0), Arc) * VelMagnitude;

						FVector BulletVelocity = ActorTransform.transform.GetRotation().RotateVector(ArcVel);
						spawned.set<FVelocity>(BulletVelocity);
					}

					if (spawner.bLoopSpawn)
					{
						spawner.TimeUntilSpawn = spawner.SpawnRate;
					}
					else
					{
						e.remove<FArchetypeSpawner>();
					}
				}
			});
		
			//Spawn with basic position
			q_spawners.each([&](auto e, FArchetypeSpawner& spawner) {
				if (!e.has<FPosition>()) return;
				const FVector& SpawnPosition = e.get<FPosition>()->pos;
				if (spawner.TimeUntilSpawn < 0)
				{
					if (IsValid(spawner.ArchetypeClass))
					{
						EntityHandle h = SpawnFromArchetype(reg, spawner.ArchetypeClass);
						flecs::entity spawned{ reg,h.handle };
						spawned.set<FPosition>(FPosition{ SpawnPosition });
					}

					if (spawner.bLoopSpawn)
					{
						spawner.TimeUntilSpawn = spawner.SpawnRate;
					}
					else
					{
						e.remove<FArchetypeSpawner>();
					}
				}
			});
		
			reg.defer_end();
		}
	);

	sysScheduler->AddTaskgraph(builder.FinishGraph());
}
//PRAGMA_ENABLE_OPTIMIZATION
void RaycastSystem::update(ECS_Registry &registry, float dt)
{
	
}

void RaycastSystem::CheckRaycasts(ECS_Registry& registry, float dt, UWorld* GameWorld)
{
	rayUnits.Reset();

	//check all the raycast results from the async raycast	
	for (auto& ray : rayRequests)
	{
		if (GameWorld->IsTraceHandleValid(ray.handle, false))
		{
			rayUnits.Add({ ray.et, &ray.handle });
		}
	}
	
	ParallelFor(rayUnits.Num(), [&](auto i) {

		auto &ray = *rayUnits[i].ray;
		auto entity = rayUnits[i].et;

		FTraceDatum tdata;
		GameWorld->QueryTraceData(ray, tdata);
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

				flecs::entity et{ registry,entity };
				//if the entity was a projectile, create explosion and destroy it
				if(et.has<FProjectile>())
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
	flecs::entity e{ registry,entity };

	auto explosionclass = e.get<const FProjectile>()->ExplosionArchetypeClass;
	if (explosionclass)
	{
		//create new entity to spawn explosion
		auto h = ecs_new(registry.c_ptr(), 0);
		flecs::entity spawner{ registry,h };
		
		spawner.set<FPosition>({ ExplosionPoint });
		spawner.set<FLifetime>({ 0.1 });

		FArchetypeSpawner spawn;
		spawn.bLoopSpawn = false;
		spawn.ArchetypeClass = explosionclass;
		spawn.SpawnRate = 1;
		spawn.TimeUntilSpawn = 0.0;
		spawn.Canary = 69;
		spawner.set<FArchetypeSpawner>({ spawn });
	}
}

DECLARE_CYCLE_STAT(TEXT("ECS: Raycast BP"), STAT_RaycastBP, STATGROUP_ECS);
DECLARE_CYCLE_STAT(TEXT("ECS: Raycast Explosions"), STAT_RaycastExplosions, STATGROUP_ECS);
DECLARE_CYCLE_STAT(TEXT("ECS: Raycast Enqueue"), STAT_RaycastResults, STATGROUP_ECS);
void  RaycastSystem::schedule(ECSSystemScheduler* sysScheduler)
{
	SystemTaskBuilder builder("RayCheck", 999, sysScheduler);

	init_query(q_rays, sysScheduler->registry);
	init_query(q_raycreate, sysScheduler->registry);

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
	//deps2.AddWrite< FRaycastResult >();
	deps2.AddRead<FMovementRaycast>();
	deps2.AddRead<FPosition>();
	deps2.AddRead<FLastPosition>();
	//deps2.AddWrite<ECS_Registry>();

	SystemTaskBuilder builder_ray("Raycast", 999, sysScheduler,2.5);

	builder_ray.AddTask(deps2, [=](ECS_Registry& reg) {
		SCOPE_CYCLE_COUNTER(STAT_RaycastResults);

		//add raycast result in bulk to whatever doesnt have one yet		
		rayRequests.Reset();
		q_raycreate.each([&, dt](auto entity, const FMovementRaycast& ray, const FPosition& pos, const FLastPosition& lastPos/*, FRaycastResult& handle*/) {

			if (pos.pos != lastPos.pos)
			{
				RaycastRequest newRay;
				newRay.Start = lastPos.pos;
				newRay.End = pos.pos;
				newRay.RayChannel = ray.RayChannel;
				newRay.et = entity.id();
				rayRequests.Add(newRay);
			}
			});
		});
	builder_ray.AddDependency("Movement");
	builder_ray.AddDependency("RayCheck");

	SystemTaskBuilder builder_rayg("RaycastGame", 1200, sysScheduler, 5);
	builder_rayg.AddGameTask({},[=](ECS_Registry& reg) {
		SCOPE_CYCLE_COUNTER(STAT_RaycastResults);
		UWorld* GameWorld = OwnerActor->GetWorld();
		for (auto& r : rayRequests)
		{
			r.handle = GameWorld->AsyncLineTraceByChannel(EAsyncTraceType::Single, r.Start, r.End, r.RayChannel);
		}
	});

	builder_rayg.AddDependency("Raycast");
	
	sysScheduler->AddTaskgraph(builder_rayg.FinishGraph());
	sysScheduler->AddTaskgraph(builder_ray.FinishGraph());
		
	SystemTaskBuilder builder2("RayExplosions", LifetimeSystem::DeletionSync-1, sysScheduler);
	builder2.AddSyncTask(
		[=](ECS_Registry& reg) {

			SCOPE_CYCLE_COUNTER(STAT_RaycastExplosions);

			DeletionContext* del = DeletionContext::GetFromRegistry(reg);
			reg.defer_begin();
			bulk_dequeue(explosions, [&reg,&del,this](const ExplosionStr& ex) {
				flecs::entity e{ reg,ex.et };
				if (e.has<FProjectile>())
				{
					CreateExplosion(reg, ex.et, ex.explosionPoint);
				}
				del->AddToQueue(ex.et);
			});		
			reg.defer_end();
		}
	);
	builder2.AddDependency("EndBarrier");

	SystemTaskBuilder builder3("raycast system: broadcast BP", 0, sysScheduler);
	builder3.AddDependency("Raycast");
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
	init_query(q_lifetime, sysScheduler->registry);


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
			
			q_lifetime.each([&](auto e, FLifetime& Deleter) {

				Deleter.LifeLeft -= 1.0 / 60.0;

				if (Deleter.LifeLeft < 0)
				{
					del->AddToQueue(e.id());
				}
			});
		});

	sysScheduler->AddTaskgraph(builder.FinishGraph());

	//SystemTaskBuilder builder2("lifetime system- Delete", LifetimeSystem::DeletionSync, sysScheduler);
	SystemTaskBuilder builder2("lifetime system- Delete", 0, sysScheduler, 0.1);
	builder2.AddDependency("lifetime system");
	builder2.AddSyncTask(
		[=](ECS_Registry& reg) {			

			SCOPE_CYCLE_COUNTER(STAT_LifeDelete);
			DeletionContext* del = DeletionContext::GetFromRegistry(reg);

			bulk_dequeue(del->entitiesToDelete, [&](EntityID id) {
				flecs::entity et{ reg,id };
				if (et.is_alive())
				{
					et.destruct();
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
		q_transform.iter([&](flecs::iter it, FActorTransform* t) {

			if (it.has_column<FPosition>())
			{
				auto pos = it.table_column<const FPosition>();
				for (auto i : it)
				{
					t[i].transform.SetLocation(pos[i].pos);
				}
			}

			if (it.has_column<FRotationComponent>())
			{
				auto pos = it.table_column<const FRotationComponent>();
				for (auto i : it)
				{
					t[i].transform.SetRotation(pos[i].rot);
				}
			}
			if (it.has_column<FScale>())
			{
				auto pos = it.table_column<const FScale>();
				for (auto i : it)
				{
					t[i].transform.SetScale3D(pos[i].scale);
				}
			}
			if (it.has_column<FCopyTransformToActor>())
			{
				if (it.has_column<FActorReference>())
				{
					auto actors = it.table_column<FActorReference>();
					for (auto i : it)
					{
						if (actors[i].ptr.IsValid())
						{
							transforms.Add({ actors[i].ptr,t[i].transform });
						}
					}
				}
			}
		});
	}

}

void  CopyTransformToActorSystem::schedule(ECSSystemScheduler* sysScheduler)
{
	SystemTaskBuilder builder("CopyBack", 10000, sysScheduler);

	
	init_query(q_transform, sysScheduler->registry);

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
	builder2.AddDependency("CopyBack");
	builder.AddDependency("Movement");
	sysScheduler->AddTaskgraph(builder.FinishGraph());
	sysScheduler->AddTaskgraph(builder2.FinishGraph());
}

void CopyTransformToECSSystem::update(ECS_Registry& registry, float dt)
{
}


void CopyTransformToECSSystem::schedule(ECSSystemScheduler* sysScheduler)
{
	SystemTaskBuilder builder("CopyTransform", 100, sysScheduler);
	init_query(q_copyactor_tf, sysScheduler->registry);
	//init_query(q_copyactor, sysScheduler->registry);


	init_query(q_transforms, sysScheduler->registry);

	TaskDependencies deps1;
	deps1.AddWrite < FActorTransform>();
	deps1.AddRead<FCopyTransformToECS>();
	deps1.AddRead<FActorReference>();

	TaskDependencies depsfirst;

	depsfirst.AddWrite<ECS_Registry>();

	builder.AddGameTask(depsfirst,
		
		[=](ECS_Registry& reg) {
			SCOPE_CYCLE_COUNTER(STAT_CopyTransformECS);

			//all movers get a transform component if they dont have one yet
			auto raypos_filter = flecs::filter(reg).include<FCopyTransformToECS>().include<FActorReference>().exclude<FActorTransform>();

			reg.add<FActorTransform>(raypos_filter);
		}
	);

	builder.AddGameTask(deps1,
		[=](ECS_Registry& reg) {
			SCOPE_CYCLE_COUNTER(STAT_CopyTransformECS);

			q_copyactor_tf.each([&](auto e, const FCopyTransformToECS& p, const FActorReference& v, FActorTransform& tf) {
				if (v.ptr.IsValid())
				{
					const FTransform& ActorTransform = v.ptr->GetActorTransform();
					tf.transform = ActorTransform;
				}
			});
		}
	);


	TaskDependencies deps;
	deps.AddWrite<FPosition>();
	deps.AddWrite<FVelocity>();
	deps.AddWrite<FScale>();
	deps.AddRead<FActorTransform>();
		
	builder.AddTask(
			deps,
			[=](ECS_Registry& reg) {

				SCOPE_CYCLE_COUNTER(STAT_UnpackActorTransform);
				
				q_transforms.each([](auto entity, const FActorTransform& transform, FPosition* pos, FRotationComponent* rot, FScale* sc) {
					if (pos)
					{
						pos->pos = transform.transform.GetLocation();
					}
					if (rot)
					{
						rot->rot = transform.transform.GetRotation();
					}
					if (sc)
					{
						sc->scale = transform.transform.GetScale3D();
					}
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
	SystemTaskBuilder builder("Movement", 300, sysScheduler);

	float dt = 1.0 / 60.0;

	init_query(q_positions, sysScheduler->registry);
	init_query(q_moves, sysScheduler->registry);

	TaskDependencies deps;
	deps.AddWrite<FPosition>();
	deps.AddWrite<FVelocity>();
	deps.AddWrite<FLastPosition>();
	deps.AddRead<FMovement>();
	
	builder.AddDependency("Boids");

	builder.AddSyncTask(
		[=](ECS_Registry& reg) {

			//movement raycast gets a "last position" component
			auto raypos_filter = flecs::filter(reg).include<FMovementRaycast>().include<FPosition>().exclude<FLastPosition>();

			reg.add<FLastPosition>(raypos_filter);
		}
	);
	builder.AddTask(
		deps,
		[=](ECS_Registry& reg) {
			q_positions.each([&, dt](auto entity, FLastPosition& lastpos,const FPosition& pos) {
				lastpos.pos = pos.pos;
			});

			q_moves.each([&, dt](auto entity, const FMovement& m, FPosition& pos, FVelocity& vel) {

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
	
	query.each([&, dt](auto entity,const FDebugSphere& ds, const FPosition& pos) {

		SCOPE_CYCLE_COUNTER(STAT_DebugDraw);
		DrawDebugSphere(OwnerActor->GetWorld(), pos.pos, ds.radius, 12, ds.color, true, UpdateRate);
		});
}

void  DebugDrawSystem::schedule(ECSSystemScheduler* sysScheduler)
{
	SystemTaskBuilder builder("DebugDraw", 10000, sysScheduler);

	TaskDependencies deps;
	deps.AddRead<FPosition>();
	
	deps.AddRead<FDebugSphere>();

	query.init(*sysScheduler->registry);

	builder.AddGameTask(deps,
		[=](ECS_Registry& reg) {
			this->update(reg, 1.0 / 60.0);
		}
	);

	sysScheduler->AddTaskgraph(builder.FinishGraph());
}
