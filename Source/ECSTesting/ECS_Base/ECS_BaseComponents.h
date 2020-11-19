#pragma once
#include "CoreMinimal.h"
#include "ECSTesting.h"
#include "ECS_Core.h"
#include "ECS_ComponentWrapper.h"
#include "ECS_BaseComponents.generated.h"



USTRUCT(BlueprintType)
struct FPosition {

	GENERATED_BODY()

		FPosition() : pos(FVector::ZeroVector) {}
	FPosition(FVector _pos) :pos(_pos) {  };

	void Add(FVector offset) { pos += offset; }

	UPROPERTY(EditAnywhere, Category = ECS)
		FVector pos;
};

USTRUCT(BlueprintType)
struct FScale {
	GENERATED_BODY()

		FScale() : scale(FVector(1.0, 1.0, 1.0)) {}
	FScale(FVector _scale) :scale(_scale) {};

	UPROPERTY(EditAnywhere, Category = ECS)
		FVector scale;
};
class UStaticMesh;
class UMaterial;

USTRUCT(BlueprintType)
struct FMovement {

	GENERATED_BODY()

		UPROPERTY(EditAnywhere, Category = ECS)
		float GravityStrenght;
};

USTRUCT(BlueprintType)
struct FInstancedStaticMesh {
	GENERATED_BODY()

		FInstancedStaticMesh(UStaticMesh* _mesh = nullptr, UMaterial* _material = nullptr) : mesh(_mesh), material(_material) {}

	UPROPERTY(EditAnywhere, Category = ECS)
		UStaticMesh* mesh;
	UPROPERTY(EditAnywhere, Category = ECS)
		UMaterial * material;
	UPROPERTY(EditAnywhere, Category = ECS)
		FTransform RelativeTransform;

	FTransform RenderTransform;


	int instanceIndex{ -1 };
};

USTRUCT(BlueprintType)
struct FVelocity {

	GENERATED_BODY()

		FVelocity() : vel(FVector::ZeroVector) {}

	FVelocity(FVector _vel) : vel(_vel) { };

	void Add(FVector _vel) { vel += _vel; }

	UPROPERTY(EditAnywhere, Category = ECS)
		FVector vel;
};

USTRUCT(BlueprintType)
struct FLastPosition {

	GENERATED_BODY()

	FLastPosition() : pos(FVector::ZeroVector) {}

	FLastPosition(FVector _pos) : pos(_pos) { };

	UPROPERTY(EditAnywhere, Category = ECS)
	FVector pos;
};

USTRUCT(BlueprintType)
struct FMovementRaycast {

	GENERATED_BODY()

	
		FMovementRaycast() {};
	FMovementRaycast(TEnumAsByte<ECollisionChannel> _trace) : RayChannel(_trace) { };

	UPROPERTY(EditAnywhere, Category = ECS)
		TEnumAsByte<ECollisionChannel> RayChannel;
};

struct FRaycastResult {

	FRaycastResult() = default;
	FRaycastResult(FTraceHandle _handle) :handle(_handle) { };

	FTraceHandle handle;
};

struct FGridMap {
	FIntVector GridLocation;
};

USTRUCT(BlueprintType)
struct FRotationComponent {
	GENERATED_BODY()

		FRotationComponent() { rot = FQuat(); }
		FRotationComponent(FQuat _rot) : rot(_rot) {}
	FRotationComponent(FRotator _rot) { rot = _rot.Quaternion(); }

	UPROPERTY(EditAnywhere, Category = ECS)
		FQuat rot;
};

USTRUCT(BlueprintType)
struct FRandomArcSpawn {
	GENERATED_BODY()

		UPROPERTY(EditAnywhere, Category = ECS)
		float MinAngle;
	UPROPERTY(EditAnywhere, Category = ECS)
		float MaxAngle;

	UPROPERTY(EditAnywhere, Category = ECS)
		float MinVelocity;
	UPROPERTY(EditAnywhere, Category = ECS)
		float MaxVelocity;
};


USTRUCT(BlueprintType)
struct FDebugSphere {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = ECS)
	float radius;

	UPROPERTY(EditAnywhere, Category = ECS)
	FColor color;
};


UENUM(BlueprintType)
enum class ETransformSyncType : uint8 {
	ECS_To_Actor,
	Actor_To_ECS,
	BothWays,
	Disabled

};
USTRUCT(BlueprintType)
struct FActorReference {
	GENERATED_BODY()
	FActorReference(AActor * Owner = nullptr) { ptr = Owner; }

	TWeakObjectPtr<AActor> ptr;

};
USTRUCT(BlueprintType)
struct FLifetime {
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "ECS")
	float LifeLeft=0;
};

//struct FDestroy
//{
//
//};

USTRUCT(BlueprintType)
struct FCopyTransformToECS {
	GENERATED_BODY()
		FCopyTransformToECS() : bWorldSpace(true){};
	
	UPROPERTY(EditAnywhere, Category = ECS)
	bool bWorldSpace;	
};
USTRUCT(BlueprintType)
struct FActorTransform {
	GENERATED_BODY()
		FActorTransform() {};
		FActorTransform(FTransform trns) : transform(trns) {};

		UPROPERTY(EditAnywhere, Category = ECS)
		FTransform transform;
};
USTRUCT(BlueprintType)
struct FCopyTransformToActor {
	GENERATED_BODY()

		FCopyTransformToActor() : bWorldSpace(true), bSweep(false) {};

	UPROPERTY(EditAnywhere, Category = ECS)
	bool bWorldSpace;
	UPROPERTY(EditAnywhere, Category = ECS)
	bool bSweep;
};
class AECS_Archetype;
USTRUCT(BlueprintType)
struct FArchetypeSpawner {
	GENERATED_BODY()

		UPROPERTY(EditAnywhere, Category = ECS)
		TSubclassOf<AECS_Archetype> ArchetypeClass;

	UPROPERTY(EditAnywhere, Category = ECS)
		float TimeUntilSpawn;
	UPROPERTY(EditAnywhere, Category = ECS)
		float SpawnRate;
	UPROPERTY(EditAnywhere, Category = ECS)
		bool bLoopSpawn;
	UPROPERTY(EditAnywhere, Category = ECS)
	int Canary;
};


class A_ECSWorldActor;

// Syncs the actor with the ECS subsystem. You need this component if you want the others to do anything
UCLASS(ClassGroup = (ECS), meta = (BlueprintSpawnableComponent))
class ECSTESTING_API UECS_ComponentSystemLink : public UActorComponent
{
	GENERATED_BODY()

public:
	
	UECS_ComponentSystemLink();

public:
	virtual void BeginPlay();

	virtual void RegisterWithECS(ECS_World* _ECS, EntityHandle entity);
	
	UPROPERTY(EditAnywhere, Category = "ECS")
		ETransformSyncType TransformSync;

	UPROPERTY(EditAnywhere, Category = "ECS")
		FCopyTransformToActor CopyToActor;
	UPROPERTY(EditAnywhere, Category = "ECS")
		FCopyTransformToECS CopyToECS;

public:
	EntityHandle myEntity;
	A_ECSWorldActor * WorldActor;
};

UCLASS(ClassGroup = (ECS), meta = (BlueprintSpawnableComponent))
class ECSTESTING_API UECS_PositionComponentWrapper : public UActorComponent, public IComponentWrapper
{
	GENERATED_BODY()
public:
	UECS_PositionComponentWrapper() { PrimaryComponentTick.bCanEverTick = false; };

	virtual void AddToEntity(ECS_World * world, EntityHandle entity) {
		init_comp(world, entity, Value);
		//world->GetRegistry()->accommodate<FPosition>(entity.handle,Value);
	};

	UPROPERTY(EditAnywhere, Category = "ECS")
		FPosition Value;
};
UCLASS(ClassGroup = (ECS), meta = (BlueprintSpawnableComponent))
class ECSTESTING_API UECS_DeleterComponentWrapper : public UActorComponent, public IComponentWrapper
{
	GENERATED_BODY()
public:
	UECS_DeleterComponentWrapper() { PrimaryComponentTick.bCanEverTick = false; };

	virtual void AddToEntity(ECS_World * world, EntityHandle entity) {
		init_comp(world, entity, Value);//world->GetRegistry()->accommodate<FLifetime>(entity.handle, Value);
	};

	UPROPERTY(EditAnywhere, Category = "ECS")
		FLifetime Value;
};
UCLASS(ClassGroup = (ECS), meta = (BlueprintSpawnableComponent))
class ECSTESTING_API UECS_FRandomArcSpawnComponentWrapper : public UActorComponent, public IComponentWrapper
{
	GENERATED_BODY()
public:
	UECS_FRandomArcSpawnComponentWrapper() { PrimaryComponentTick.bCanEverTick = false; };

	virtual void AddToEntity(ECS_World * world, EntityHandle entity) {
		init_comp(world, entity, Value);//world->GetRegistry()->accommodate<FRandomArcSpawn>(entity.handle, Value);
	};

	UPROPERTY(EditAnywhere, Category = "ECS")
		FRandomArcSpawn Value;
};

UCLASS(ClassGroup = (ECS), meta = (BlueprintSpawnableComponent))
class ECSTESTING_API UECS_ScaleComponentWrapper : public UActorComponent, public IComponentWrapper
{
	GENERATED_BODY()
public:
	UECS_ScaleComponentWrapper() { PrimaryComponentTick.bCanEverTick = false; };

	virtual void AddToEntity(ECS_World * world, EntityHandle entity) {
		init_comp(world, entity, Value);//world->GetRegistry()->accommodate<FScale>(entity.handle, Value);
	};

	UPROPERTY(EditAnywhere, Category = "ECS")
		FScale Value;
};

UCLASS(ClassGroup = (ECS), meta = (BlueprintSpawnableComponent))
class ECSTESTING_API UECS_MovementComponentWrapper : public UActorComponent, public IComponentWrapper
{
	GENERATED_BODY()
public:
	UECS_MovementComponentWrapper() { PrimaryComponentTick.bCanEverTick = false; };

	virtual void AddToEntity(ECS_World* world, EntityHandle entity) {
		//FMovement m;
		init_comp(world, entity, FMovement{});//world->GetRegistry()->accommodate<FMovement>(entity.handle);
	};
};

UCLASS(ClassGroup = (ECS), meta = (BlueprintSpawnableComponent))
class ECSTESTING_API UECS_DebugSphereComponentWrapper : public UActorComponent, public IComponentWrapper
{
	GENERATED_BODY()
public:
	UECS_DebugSphereComponentWrapper(){ PrimaryComponentTick.bCanEverTick = false; }

	virtual void AddToEntity(ECS_World * world, EntityHandle entity) {
		init_comp(world, entity, Value);//world->GetRegistry()->accommodate<FDebugSphere>(entity.handle, Value);
	};

	UPROPERTY(EditAnywhere, Category = "ECS")
		FDebugSphere Value;
};

UCLASS(ClassGroup = (ECS), meta = (BlueprintSpawnableComponent))
class ECSTESTING_API UECS_InstancedStaticMeshComponentWrapper : public UActorComponent, public IComponentWrapper
{
	GENERATED_BODY()
public:
	UECS_InstancedStaticMeshComponentWrapper() { PrimaryComponentTick.bCanEverTick = false; };

	virtual void AddToEntity(ECS_World * world, EntityHandle entity) {
		init_comp(world, entity, Value);//world->GetRegistry()->accommodate<FInstancedStaticMesh>(entity.handle, Value);
	};

	UPROPERTY(EditAnywhere, Category = "ECS")
		FInstancedStaticMesh Value;

};

UCLASS(ClassGroup = (ECS), meta = (BlueprintSpawnableComponent))
class ECSTESTING_API UECS_VelocityComponentWrapper : public UActorComponent, public IComponentWrapper
{
	GENERATED_BODY()
public:
	UECS_VelocityComponentWrapper() { PrimaryComponentTick.bCanEverTick = false; };

	virtual void AddToEntity(ECS_World * world, EntityHandle entity) {
		init_comp(world, entity, Value);//world->GetRegistry()->accommodate<FVelocity>(entity.handle, Value);
	};

	UPROPERTY(EditAnywhere, Category = "ECS")
		FVelocity Value;

};

UCLASS(ClassGroup = (ECS), meta = (BlueprintSpawnableComponent))
class ECSTESTING_API UECS_RotationComponentWrapper : public UActorComponent, public IComponentWrapper
{
	GENERATED_BODY()
public:
	UECS_RotationComponentWrapper() { PrimaryComponentTick.bCanEverTick = false; };

	virtual void AddToEntity(ECS_World * world, EntityHandle entity) {
		init_comp(world, entity, Value);//world->GetRegistry()->accommodate<FRotationComponent>(entity.handle, Value);
	};

	UPROPERTY(EditAnywhere, Category = "ECS")
		FRotationComponent Value;

};

UCLASS(ClassGroup = (ECS), meta = (BlueprintSpawnableComponent))
class ECSTESTING_API UECS_ArchetypeSpawnerComponentWrapper : public UActorComponent, public IComponentWrapper
{
	GENERATED_BODY()
public:
	UECS_ArchetypeSpawnerComponentWrapper() { PrimaryComponentTick.bCanEverTick = false; };

	virtual void AddToEntity(ECS_World * world, EntityHandle entity) {
		Value.Canary = 420;
		init_comp(world, entity, Value);//world->GetRegistry()->accommodate<FArchetypeSpawner>(entity.handle, Value);
	};

	UPROPERTY(EditAnywhere, Category = "ECS")
		FArchetypeSpawner Value;

};
UCLASS(ClassGroup = (ECS), meta = (BlueprintSpawnableComponent))
class ECSTESTING_API UECS_MovementRaycastComponentWrapper : public UActorComponent, public IComponentWrapper
{
	GENERATED_BODY()
public:
	UECS_MovementRaycastComponentWrapper() { PrimaryComponentTick.bCanEverTick = false; };

	virtual void AddToEntity(ECS_World * world, EntityHandle entity) {
		init_comp(world, entity, Value);//world->GetRegistry()->accommodate<FMovementRaycast>(entity.handle, Value);
	};
	
	UPROPERTY(EditAnywhere, Category = "ECS")
		FMovementRaycast Value;

};