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

	void update(ECS_Registry &registry, float dt) override
	{
		assert(OwnerActor);
		elapsed -= dt;
		if (elapsed > 0)
		{
			return;
		}

		elapsed = UpdateRate;

		registry.view<FDebugSphere, FPosition>().each([&,dt](auto entity, FDebugSphere & ds, FPosition & pos) {

			SCOPE_CYCLE_COUNTER(STAT_DebugDraw);
			DrawDebugSphere(OwnerActor->GetWorld(),pos.pos,ds.radius,12,ds.color,true,UpdateRate);
		});
	}
};

DECLARE_CYCLE_STAT(TEXT("ECS: Movement Update"), STAT_Movement, STATGROUP_ECS);
struct MovementSystem :public System {

	
	void update(ECS_Registry &registry, float dt) override
	{		
		SCOPE_CYCLE_COUNTER(STAT_Movement);

		//movement raycast gets a "last position" component
		registry.view<FMovementRaycast,FPosition>().each([&, dt](auto entity,FMovementRaycast & ray, FPosition & pos) {
			registry.accommodate<FLastPosition>(entity, pos.pos);
		});

		//add gravity and basic movement from velocity
		registry.view<FMovement, FPosition, FVelocity>().each([&, dt](auto entity, FMovement & m, FPosition & pos, FVelocity & vel) {

			//gravity
			const FVector gravity = FVector(0.f, 0.f, -980) * m.GravityStrenght;
			vel.Add(gravity*dt);

			pos.Add( vel.vel * dt);			
		});
	}
};

DECLARE_CYCLE_STAT(TEXT("ECS: Copy Transform To ECS"), STAT_CopyTransformECS, STATGROUP_ECS);
DECLARE_CYCLE_STAT(TEXT("ECS: Unpack Actor Transform"), STAT_UnpackActorTransform, STATGROUP_ECS);
struct CopyTransformToECSSystem :public System {	

	void update(ECS_Registry &registry, float dt) override
	{
		assert(OwnerActor);		

		//copy transforms from actor into FActorTransform
		auto ActorTransformView = registry.persistent<FCopyTransformToECS, FActorReference>();
		for (auto e : ActorTransformView)
		{
			SCOPE_CYCLE_COUNTER(STAT_CopyTransformECS);
			FActorReference & actor = ActorTransformView.get<FActorReference>(e);
			if (actor.ptr.IsValid())
			{
				const FTransform & ActorTransform = actor.ptr->GetActorTransform();
				registry.accommodate<FActorTransform>(e, ActorTransform);
			}
		}		
		{

			SCOPE_CYCLE_COUNTER(STAT_UnpackActorTransform);
			//unpack from ActorTransform into the separate transform components, only if the entity does have that component
			registry.view<FActorTransform, FPosition>().each([&, dt](auto entity, FActorTransform & transform, FPosition & pos) {

				pos.pos = transform.transform.GetLocation();
			});
			registry.view<FActorTransform, FRotationComponent>().each([&, dt](auto entity, FActorTransform & transform, FRotationComponent & rot) {

				rot.rot = transform.transform.GetRotation();
			});
			registry.view<FActorTransform, FScale>().each([&, dt](auto entity, FActorTransform & transform, FScale & sc) {

				sc.scale = transform.transform.GetScale3D();
			});
		}
	}
};
DECLARE_CYCLE_STAT(TEXT("ECS: Copy Transform To Actor"), STAT_CopyTransformActor, STATGROUP_ECS);
DECLARE_CYCLE_STAT(TEXT("ECS: Pack actor transform"), STAT_PackActorTransform, STATGROUP_ECS);

struct CopyTransformToActorSystem :public System {

	void update(ECS_Registry &registry, float dt) override
	{
		
		assert(OwnerActor);

		{
			SCOPE_CYCLE_COUNTER(STAT_PackActorTransform);
			//fill ActorTransform from separate components		
			registry.view<FActorTransform, FPosition>().each([&, dt](auto entity, FActorTransform & transform, FPosition & pos) {
				transform.transform.SetLocation(pos.pos);
			});
			registry.view<FActorTransform, FRotationComponent>().each([&, dt](auto entity, FActorTransform & transform, FRotationComponent & rot) {
				transform.transform.SetRotation(rot.rot);
			});
			registry.view<FActorTransform, FScale>().each([&, dt](auto entity, FActorTransform & transform, FScale & sc) {

				transform.transform.SetScale3D(sc.scale);
			});
		}
		
		//copy transforms from actor into FActorTransform	
		auto TransformView = registry.persistent<FCopyTransformToActor, FActorReference, FActorTransform>();
		for (auto e : TransformView)
		{
			SCOPE_CYCLE_COUNTER(STAT_CopyTransformActor);
			const FTransform&transform = TransformView.get<FActorTransform>(e).transform;
			FActorReference&actor = TransformView.get<FActorReference>(e);

			if (actor.ptr.IsValid())
			{
				actor.ptr->SetActorTransform(transform);
			}
		}		
	}
};

DECLARE_CYCLE_STAT(TEXT("ECS: Spanwer System"), STAT_ECSSpawn, STATGROUP_ECS);
struct ArchetypeSpawnerSystem :public System {

	TMap<TSubclassOf<AECS_Archetype>, AECS_Archetype*> Archetypes;

	EntityHandle SpawnFromArchetype(ECS_Registry & registry,TSubclassOf<AECS_Archetype> &ArchetypeClass)
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

	void update(ECS_Registry &registry, float dt) override;
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

	ISMData * GetInstancedMeshForMesh(UStaticMesh * mesh)
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
			auto &d = MeshMap.Add(mesh, NewData);
			
			return &d;
		}				

	}

	void initialize(AActor * _Owner, ECS_World * _World) override{
		
		OwnerActor = _Owner;	
		World = _World;
		ISM = nullptr;		
	}	

	void update(ECS_Registry &registry,float dt) override;
};

DECLARE_CYCLE_STAT(TEXT("ECS: Raycast System"), STAT_ECSRaycast, STATGROUP_ECS);
struct RaycastSystem :public System {

	void update(ECS_Registry &registry, float dt) override;
};
DECLARE_CYCLE_STAT(TEXT("ECS: Lifetime System"), STAT_Lifetime, STATGROUP_ECS);
struct LifetimeSystem :public System {

	void update(ECS_Registry &registry, float dt) override
	{
		assert(OwnerActor);
		
		SCOPE_CYCLE_COUNTER(STAT_Lifetime);

		//tick the lifetime timers
		auto LifetimeView = registry.view<FLifetime>();
		for (auto e : LifetimeView)
		{
			auto &Deleter = LifetimeView.get(e);

			Deleter.LifeLeft -= dt;
			if (Deleter.LifeLeft < 0)
			{
				//add a Destroy component so it gets deleted
				registry.accommodate<FDestroy>(e);				
			}
		}

		//logic can be done here for custom deleters, nothing right now


		//delete everything with a FDestroy component
		auto DeleteView = registry.view<FDestroy>();
		for (auto e : DeleteView)
		{			
			registry.destroy(e);			
		}		
	}
};
