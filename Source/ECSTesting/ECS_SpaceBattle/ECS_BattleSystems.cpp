#include "ECS_BattleSystems.h"
#include "SystemTasks.h"
#include "DrawDebugHelpers.h"

void SpaceshipSystem::update(ECS_Registry& registry, float dt)
{
	assert(OwnerActor);
	SCOPE_CYCLE_COUNTER(STAT_Explosion);


	registry.view<FSpaceship, FRotationComponent, FVelocity>().each([&, dt](auto entity, FSpaceship& ship, FRotationComponent& rotation, FVelocity& vel) {

		rotation.rot = vel.vel.Rotation().Quaternion();
	});
}

void SpaceshipSystem::schedule(ECSSystemScheduler* sysScheduler)
{
	SystemTaskBuilder builder("Spaceship", 400, sysScheduler);

	TaskDependencies deps;

	deps.AddWrite< FRotationComponent >();
	deps.AddRead < FSpaceship > ();
	deps.AddRead < FVelocity > ();

	builder.AddDependency("Boids");
	builder.AddTask(deps,
		[=](ECS_Registry& reg) {
			this->update(reg, 1.0 / 60.0);
		}
	);

	sysScheduler->AddTaskgraph(builder.FinishGraph());
}

void BoidSystem::update(ECS_Registry& registry, float dt)
{	
	UpdateGridmap(registry);
	UpdateAllBoids(registry, dt);

	//Draw gridmap
	const bool bDebugGridMap = false;
	if (bDebugGridMap)
	{
		const float MaxUnits = 5.0;
		for (auto& g : GridMap)
		{
			const FVector BoxMin = FVector(g.Key) * GRID_DIMENSION;
			const FVector BoxMax = BoxMin + FVector(GRID_DIMENSION, GRID_DIMENSION, GRID_DIMENSION);
			const FVector BoxCenter = BoxMin + FVector(GRID_DIMENSION, GRID_DIMENSION, GRID_DIMENSION) / 2.0;
			const FColor BoxColor = FColor::MakeRedToGreenColorFromScalar(g.Value.Num() / MaxUnits);
			DrawDebugBox(OwnerActor->GetWorld(), BoxCenter, FVector(GRID_DIMENSION, GRID_DIMENSION, GRID_DIMENSION) / 2.0, BoxColor);
		}
	}
}

void BoidSystem::UpdateAllBoids(ECS_Registry& registry, float dt)
{
	{
		SCOPE_CYCLE_COUNTER(STAT_Boids);

		TypedLinearMemory<ProjectileData> ProjArray(World->ScratchPad);

		auto ProjectileView = registry.view<FProjectile, FPosition, FVelocity, FFaction>(entt::persistent_t{});
		//ProjArray.Reset();
		unsigned int NumProjectiles = 0;
		//copy projectile data into array so we can do a parallel update later
		for (auto e : ProjectileView)
		{
			ProjectileData Projectile;
			Projectile.faction = ProjectileView.get<FFaction>(e);
			Projectile.pos = ProjectileView.get<FPosition>(e);
			Projectile.proj = ProjectileView.get<FProjectile>(e);

			Projectile.vel = &ProjectileView.get<FVelocity>(e);
			ProjArray.push_back(Projectile);
			NumProjectiles++;
			//ProjArray.Add(Projectile);
		}


		ParallelFor(NumProjectiles, [&](int32 Index)
			{
				ProjectileData data = ProjArray[Index];

				//unpack projectile data
				const FVector ProjPosition = data.pos.pos;
				const EFaction ProjFaction = data.faction.faction;
				const float ProjSeekStrenght = data.proj.HeatSeekStrenght;
				const float ProjMaxVelocity = data.proj.MaxVelocity;
				FVector& ProjVelocity = data.vel->vel;


				const float ProjCheckRadius = 1000;
				Foreach_EntitiesInRadius(ProjCheckRadius, ProjPosition, [&](GridItem& item) {

					if (item.Faction != ProjFaction)
					{
						const FVector TestPosition = item.Position;

						const float DistSquared = FVector::DistSquared(TestPosition, ProjPosition);

						const float AvoidanceDistance = ProjCheckRadius * ProjCheckRadius;
						const float DistStrenght = FMath::Clamp(1.0 - (DistSquared / (AvoidanceDistance)), 0.1, 1.0) * dt;
						const FVector AvoidanceDirection = TestPosition - ProjPosition;

						ProjVelocity += (AvoidanceDirection.GetSafeNormal() * ProjSeekStrenght * DistStrenght);
					}
					});

				ProjVelocity = ProjVelocity.GetClampedToMaxSize(ProjMaxVelocity);
			});

	}
	//its not good to have both spaceship and projectile logic here, they should be on their own systems
	{
		SCOPE_CYCLE_COUNTER(STAT_Boids);


		//auto spaceshipEnd = SpaceshipView.size()

		//int nShips = registry.view<FSpaceship>().size();
		auto SpaceshipView = registry.view<FSpaceship, FPosition, FVelocity, FFaction>(entt::persistent_t{});

		TypedLinearMemory<SpaceshipData> SpaceshipArray(World->ScratchPad);
		unsigned int NumShips = 0;
		//SpaceshipArray.Reset();// Spaces//nShips);
		//copy spaceship data into array so we can do a paralle update later
		for (auto e : SpaceshipView)
		{
			SpaceshipData Ship;
			Ship.faction = SpaceshipView.get<FFaction>(e);
			Ship.pos = SpaceshipView.get<FPosition>(e);
			Ship.ship = SpaceshipView.get<FSpaceship>(e);

			Ship.vel = &SpaceshipView.get<FVelocity>(e);
			NumShips++;
			SpaceshipArray.push_back(Ship);
			//SpaceshipArray.Add(Ship);
		}

		ParallelFor(NumShips, [&](int32 Index)
			{
				SpaceshipData data = SpaceshipArray[Index];

				//unpack ship variables from the array
				const FVector ShipPosition = data.pos.pos;
				const EFaction ShipFaction = data.faction.faction;
				const float ShipAvoidanceStrenght = data.ship.AvoidanceStrenght;
				const float ShipMaxVelocity = data.ship.MaxVelocity;
				FVector& ShipVelocity = data.vel->vel;
				const FVector ShipTarget = data.ship.TargetMoveLocation;

				const float shipCheckRadius = 1000;
				Foreach_EntitiesInRadius(shipCheckRadius, ShipPosition, [&](GridItem& item) {

					if (item.Faction == ShipFaction)
					{
						const FVector TestPosition = item.Position;

						const float DistSquared = FVector::DistSquared(TestPosition, ShipPosition);

						const float AvoidanceDistance = shipCheckRadius * shipCheckRadius;
						const float DistStrenght = FMath::Clamp(1.0 - (DistSquared / (AvoidanceDistance)), 0.1, 1.0) * dt;
						const FVector AvoidanceDirection = ShipPosition - TestPosition;

						ShipVelocity += AvoidanceDirection.GetSafeNormal() * ShipAvoidanceStrenght * DistStrenght;
					}
					});

				FVector ToTarget = ShipTarget - ShipPosition;
				ToTarget.Normalize();

				ShipVelocity += (ToTarget * 500 * dt);
				ShipVelocity = ShipVelocity.GetClampedToMaxSize(ShipMaxVelocity);
			});


	}
}

void BoidSystem::UpdateGridmap(ECS_Registry& registry)
{//add everything to the gridmap
	GridMap.Empty(50);
	auto GridView = registry.view<FGridMap, FPosition>();

	{
		SCOPE_CYCLE_COUNTER(STAT_GridmapUpdate);
		for (auto e : GridView)
		{

			EntityHandle handle(e);

			AddToGridmap(handle, GridView.get<FPosition>(e));
		}
	}
}

void BoidSystem::schedule(ECSSystemScheduler* sysScheduler)
{
	SystemTaskBuilder builder("Boids",200,sysScheduler,3);


	TaskDependencies deps1;
	deps1.AddRead < FPosition >();
	deps1.AddWrite < FGridMap >();

	builder.AddTask(deps1,
		[=](ECS_Registry& reg) {
			UpdateGridmap(reg);
		
	});
	

	TaskDependencies deps2;
	deps2.AddWrite < FVelocity >();
	deps2.AddRead < FGridMap >();	
	deps2.AddRead < FSpaceship>();
	deps2.AddWrite <FPosition >();
	deps2.AddRead < FProjectile>();
	//builder.AddSyncTask(
	builder.AddTask(deps2,
		[=](ECS_Registry& reg) {
			UpdateAllBoids(reg, 1.0/60.0);
		}
	);

	builder.AddDependency("CopyTransform");

	sysScheduler->AddTaskgraph(builder.FinishGraph());
}

void ExplosionSystem::update(ECS_Registry& registry, float dt)
{
}

void ExplosionSystem::schedule(ECSSystemScheduler* sysScheduler)
{
	SystemTaskBuilder builder("Explosion", 200000, sysScheduler);

	float dt = 1.0 / 60.f;
	TaskDependencies deps1;

	//deps1.AddWrite < FDestroy > ();
	deps1.AddWrite < FExplosion > ();
	//builder.AddGameTask(deps1,
	builder.AddDependency("Movement");
	builder.AddSyncTask(//deps1,
		[=](ECS_Registry& reg) {
			SCOPE_CYCLE_COUNTER(STAT_Explosion);

			DeletionContext* del = DeletionContext::GetFromRegistry(reg);

			auto AllExplosionsView = reg.view<FExplosion>();
			for (auto e : AllExplosionsView) {
				FExplosion& ex = AllExplosionsView.get(e);

				ex.LiveTime += dt;
				if (ex.LiveTime > ex.Duration)
				{

					del->AddToQueue(e);
				}
			}
		}
	);

	TaskDependencies deps;

	deps.AddWrite < FScale > ();
	deps.AddRead < FExplosion > ();

	builder.AddTask(deps,
		[=](ECS_Registry& reg) {
			reg.view<FExplosion, FScale>().each([&, dt](auto entity, FExplosion& ex, FScale& scale) {

				scale.scale = FVector((ex.LiveTime / ex.Duration) * ex.MaxScale);
			});
		}
	);

	sysScheduler->AddTaskgraph(builder.FinishGraph());
}
