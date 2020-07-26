#pragma once

#include "ECS_Core.h"
#include "ECS_BaseComponents.h"
#include "ECS_Archetype.h"
#include "ECS_BattleComponents.h"
#include "DrawDebugHelpers.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "ParallelFor.h"
DECLARE_CYCLE_STAT(TEXT("ECS: DebugDraw"), STAT_DebugDraw, STATGROUP_ECS);

struct DebugDrawSystem :public System {

	const float UpdateRate = 0.1;

	float elapsed = 0;

	void update(ECS_Registry &registry, float dt) override;


	void schedule(ECSSystemScheduler* sysScheduler) override;

};

DECLARE_CYCLE_STAT(TEXT("ECS: Movement Update"), STAT_Movement, STATGROUP_ECS);
struct MovementSystem :public System {

	
	void update(ECS_Registry &registry, float dt) override;


	void schedule(ECSSystemScheduler* sysScheduler) override;

};

DECLARE_CYCLE_STAT(TEXT("ECS: Copy Transform To ECS"), STAT_CopyTransformECS, STATGROUP_ECS);
DECLARE_CYCLE_STAT(TEXT("ECS: Unpack Actor Transform"), STAT_UnpackActorTransform, STATGROUP_ECS);
struct CopyTransformToECSSystem :public System {	

	void update(ECS_Registry &registry, float dt) override;


	void schedule(ECSSystemScheduler* sysScheduler) override;

};
DECLARE_CYCLE_STAT(TEXT("ECS: Copy Transform To Actor"), STAT_CopyTransformActor, STATGROUP_ECS);
DECLARE_CYCLE_STAT(TEXT("ECS: Pack actor transform"), STAT_PackActorTransform, STATGROUP_ECS);

struct CopyTransformToActorSystem :public System {

	void update(ECS_Registry &registry, float dt) override;


	void schedule(ECSSystemScheduler* sysScheduler) override;

	struct ActorTransformParm {
		TWeakObjectPtr<AActor> actor;
		FTransform transform;		
	};

	TArray<ActorTransformParm> transforms;
};

DECLARE_CYCLE_STAT(TEXT("ECS: Spanwer System"), STAT_ECSSpawn, STATGROUP_ECS);
struct ArchetypeSpawnerSystem :public System {

	TMap<TSubclassOf<AECS_Archetype>, AECS_Archetype*> Archetypes;

	EntityHandle SpawnFromArchetype(ECS_Registry & registry,TSubclassOf<AECS_Archetype> &ArchetypeClass);

	void update(ECS_Registry &registry, float dt) override;


	void schedule(ECSSystemScheduler* sysScheduler) override;

};

class StaticMeshDrawSystem :public System {
public:

	struct ISMData {
		UInstancedStaticMeshComponent* ISM;
		int rendered;
	};

	UInstancedStaticMeshComponent * ISM;
	TMap<UStaticMesh*, ISMData> MeshMap;
	

	int render = 0;
	float Elapsed{5.f};	

	ISMData * GetInstancedMeshForMesh(UStaticMesh * mesh);

	void initialize(AActor * _Owner, ECS_World * _World) override{
		
		OwnerActor = _Owner;	
		World = _World;
		ISM = nullptr;		
	}	

	void update(ECS_Registry &registry,float dt) override;


	void schedule(ECSSystemScheduler* sysScheduler) override;
};

DECLARE_CYCLE_STAT(TEXT("ECS: Raycast System"), STAT_ECSRaycast, STATGROUP_ECS);
struct RaycastSystem :public System {

	void update(ECS_Registry &registry, float dt) override;

	void CheckRaycasts(ECS_Registry& registry, float dt, UWorld* GameWorld);

	void CreateExplosion(ECS_Registry& registry, EntityID entity, FVector ExplosionPoint);

	void schedule(ECSSystemScheduler* sysScheduler) override;

	struct ExplosionStr {
		EntityID et;
		FVector explosionPoint;
	};

	struct ActorBpCall {
		TWeakObjectPtr<AActor> actor;
	};

	struct RaycastUnit {
		EntityID et;
		FRaycastResult* ray;
	};
	TArray<RaycastUnit> rayUnits;
	moodycamel::ConcurrentQueue<ExplosionStr> explosions;
	moodycamel::ConcurrentQueue<ActorBpCall> actorCalls;
};
DECLARE_CYCLE_STAT(TEXT("ECS: Lifetime System"), STAT_Lifetime, STATGROUP_ECS);
struct LifetimeSystem :public System {

	static constexpr int DeletionSync = 200000;

	void update(ECS_Registry &registry, float dt) override;
	void schedule(ECSSystemScheduler* sysScheduler) override;
	
};
