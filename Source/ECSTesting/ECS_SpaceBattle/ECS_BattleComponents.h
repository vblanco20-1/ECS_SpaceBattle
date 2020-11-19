#pragma once
#include "ECS_BaseComponents.h"


#include "ECS_BattleComponents.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDamagedDelegate,float, Damage);


UENUM(BlueprintType)
enum class EFaction :uint8 {
	Red,
	Blue,
	Neutral
};
USTRUCT(BlueprintType)
struct FFaction {
	GENERATED_BODY()

		FFaction(EFaction _fact) : faction(_fact) {};
		FFaction() : faction(EFaction::Neutral){};

	
	UPROPERTY(EditAnywhere, Category = ECS)
		EFaction faction;
};

USTRUCT(BlueprintType)
struct FSpaceship {
	GENERATED_BODY()

		UPROPERTY(EditAnywhere)
		float AvoidanceStrenght;
	UPROPERTY(EditAnywhere)
		float MaxVelocity;

	UPROPERTY(EditAnywhere)
	FVector TargetMoveLocation;
};
USTRUCT(BlueprintType)
struct FProjectile {
	GENERATED_BODY()

		UPROPERTY(EditAnywhere)
		float HeatSeekStrenght;
	UPROPERTY(EditAnywhere)
		float MaxVelocity;
	UPROPERTY(EditAnywhere, Category = ECS)
		TSubclassOf<AECS_Archetype> ExplosionArchetypeClass;
};

USTRUCT(BlueprintType)
struct FExplosion {
	GENERATED_BODY()

		float LiveTime;

		UPROPERTY(EditAnywhere)
		float Duration;
	UPROPERTY(EditAnywhere)
		float MaxScale;
};


USTRUCT(BlueprintType)
struct FHealth {
	GENERATED_BODY()

		
	FHealth(float _h = 0.f) : Health(_h) {};


	UPROPERTY(EditAnywhere)
		float Health;
};

UCLASS(ClassGroup = (ECS), meta = (BlueprintSpawnableComponent))
class ECSTESTING_API UECS_FactionComponentWrapper : public UActorComponent, public IComponentWrapper
{
	GENERATED_BODY()
public:
	UECS_FactionComponentWrapper() { PrimaryComponentTick.bCanEverTick = false; };

	virtual void AddToEntity(ECS_World * world, EntityHandle entity) {
		init_comp(world, entity, Value);
	};
	UPROPERTY(EditAnywhere, Category = ECS)
	FFaction Value;
};

UCLASS(ClassGroup = (ECS), meta = (BlueprintSpawnableComponent))
class ECSTESTING_API UECS_HealthComponentWrapper : public UActorComponent, public IComponentWrapper
{
	GENERATED_BODY()
public:
	UECS_HealthComponentWrapper() { PrimaryComponentTick.bCanEverTick = false; };

	virtual void AddToEntity(ECS_World * world, EntityHandle entity) {
		init_comp(world, entity, Value);
	};
	UPROPERTY(BlueprintAssignable, Category = "ECS")
		FDamagedDelegate OnDamaged;

	UPROPERTY(EditAnywhere, Category = ECS)
		FHealth Value;
};
UCLASS(ClassGroup = (ECS), meta = (BlueprintSpawnableComponent))
class ECSTESTING_API UECS_SpaceshipComponentWrapper : public UActorComponent, public IComponentWrapper
{
	GENERATED_BODY()
public:
	UECS_SpaceshipComponentWrapper() { PrimaryComponentTick.bCanEverTick = false; };

	virtual void AddToEntity(ECS_World * world, EntityHandle entity) {
		init_comp(world, entity, Value);
	};
	UPROPERTY(EditAnywhere, Category = ECS)
		FSpaceship Value;
};
UCLASS(ClassGroup = (ECS), meta = (BlueprintSpawnableComponent))
class ECSTESTING_API UECS_ProjectileComponentWrapper : public UActorComponent, public IComponentWrapper
{
	GENERATED_BODY()
public:
	UECS_ProjectileComponentWrapper() { PrimaryComponentTick.bCanEverTick = false; };

	virtual void AddToEntity(ECS_World * world, EntityHandle entity) {
		init_comp(world, entity, Value);
	};
	UPROPERTY(EditAnywhere, Category = ECS)
		FProjectile Value;
};

UCLASS(ClassGroup = (ECS), meta = (BlueprintSpawnableComponent))
class ECSTESTING_API UECS_ExplosionComponentWrapper: public UActorComponent, public IComponentWrapper
{
	GENERATED_BODY()
public:
	UECS_ExplosionComponentWrapper() { PrimaryComponentTick.bCanEverTick = false; };

	virtual void AddToEntity(ECS_World * world, EntityHandle entity) {
		init_comp(world, entity, Value);
	};
	UPROPERTY(EditAnywhere, Category = ECS)
		FExplosion Value;
};
