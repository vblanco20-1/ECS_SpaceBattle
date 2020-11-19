#pragma once

#include "ECS_Core.h"
#include "ECS_BaseComponents.h"
#include "ECS_Archetype.h"
#include "ECS_BattleComponents.h"

#include "LinearMemory.h"

DECLARE_CYCLE_STAT(TEXT("ECS: Explosion System"), STAT_Explosion, STATGROUP_ECS);

struct ExplosionSystem :public System {

	const float UpdateRate = 0.1;

	float elapsed = 0;

	void update(ECS_Registry &registry, float dt) override;


	void schedule(ECSSystemScheduler* sysScheduler) override;

	flecs::query <FExplosion, FScale> q_explosions;
};

struct SpaceshipSystem :public System {
	

	void update(ECS_Registry &registry, float dt) override;

	void schedule(ECSSystemScheduler* sysScheduler) override;

	flecs::query<FSpaceship, FRotationComponent, FVelocity> q_ships;
};


DECLARE_CYCLE_STAT(TEXT("ECS: Boids Update"), STAT_Boids, STATGROUP_ECS);
DECLARE_CYCLE_STAT(TEXT("ECS: Gridmap Update"), STAT_GridmapUpdate, STATGROUP_ECS);
struct BoidSystem :public System {

	const float GRID_DIMENSION = 500.0;

	struct GridItem {
		EntityHandle ID;
		FVector Position;
		EFaction Faction;

	};
	struct ProjectileData {
		//read only data
		FProjectile proj;
		FPosition pos;
		FFaction faction;
		//velocity is a pointer so it can be modified
		FVelocity * vel;
	};
	struct SpaceshipData {

		//read only data
		FSpaceship ship;
		FPosition pos;
		FFaction  faction;

		//velocity is a pointer so it can be modified
		FVelocity * vel;
	};

	
	TMap<FIntVector, TArray<GridItem>> GridMap;

	void AddToGridmap(flecs::entity ent, FPosition&pos);
	void Foreach_EntitiesInRadius(float radius, FVector origin, TFunctionRef<void(GridItem&)> Body)
	{
		const float radSquared = radius * radius;
		const FVector RadVector(radius, radius, radius);
		const FIntVector GridLoc = FIntVector(origin / GRID_DIMENSION);
		const FIntVector MinGrid = FIntVector((origin - RadVector) / GRID_DIMENSION);
		const FIntVector MaxGrid = FIntVector((origin + RadVector) / GRID_DIMENSION);


		for (int x = MinGrid.X; x <= MaxGrid.X; x++) {
			for (int y = MinGrid.Y; y <= MaxGrid.Y; y++) {
				for (int z = MinGrid.Z; z <= MaxGrid.Z; z++) {
					const FIntVector SearchLoc(x, y, z);
					const auto SearchGrid = GridMap.Find(SearchLoc);
					if (SearchGrid)
					{
						for (auto e : *SearchGrid)
						{
							if (FVector::DistSquared(e.Position, origin) < radSquared)
							{
								Body(e);

							}
						}
					}
				}
			}
		}
	}

	void update(ECS_Registry &registry, float dt) override;

	void UpdateAllBoids(ECS_Registry& registry, float dt);

	void update_projectile(ProjectileData& data, float dt);

	

	void update_spaceship(SpaceshipData& data/*TypedLinearMemory<SpaceshipData> SpaceshipArray, int32 Index*/, float dt);
	void UpdateGridmap(ECS_Registry& registry);


	void schedule(ECSSystemScheduler* sysScheduler) override;

	flecs::query <FGridMap, FPosition> q_grid;
	flecs::query <FSpaceship, FPosition, FVelocity, FFaction> q_ships;
	flecs::query<FProjectile, FPosition, FVelocity, FFaction > q_projectiles;

	//temporal storage
	TArray<ProjectileData> ProjArray;
	TArray<SpaceshipData> SpaceshipArray;
};