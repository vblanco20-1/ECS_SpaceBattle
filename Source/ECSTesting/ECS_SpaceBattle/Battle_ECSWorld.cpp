// Fill out your copyright notice in the Description page of Project Settings.

#include "Battle_ECSWorld.h"
#include "ECS_Core.h"
#include "ECS_BaseSystems.h"
#include "ECS_BattleSystems.h"


// Sets default values
A_ECSWorldActor::A_ECSWorldActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void A_ECSWorldActor::BeginPlay()
{
	Super::BeginPlay();
	

		ECSWorld = MakeUnique<ECS_World>();
		
		ECSWorld->CreateAndRegisterSystem<CopyTransformToECSSystem>();
		ECSWorld->CreateAndRegisterSystem<BoidSystem>();

		ECSWorld->CreateAndRegisterSystem<MovementSystem>();
		ECSWorld->CreateAndRegisterSystem<ExplosionSystem>();
		ECSWorld->CreateAndRegisterSystem<SpaceshipSystem>();
		ECSWorld->CreateAndRegisterSystem<RaycastSystem>();
		ECSWorld->CreateAndRegisterSystem<LifetimeSystem>();
		ECSWorld->CreateAndRegisterSystem<StaticMeshDrawSystem>();
		ECSWorld->CreateAndRegisterSystem<DebugDrawSystem>();

		ECSWorld->CreateAndRegisterSystem<CopyTransformToActorSystem>();

		ECSWorld->CreateAndRegisterSystem<ArchetypeSpawnerSystem>();		

				ECSWorld->InitializeSystems(this);
		
		
	
}

// Called every frame
void A_ECSWorldActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ECSWorld->UpdateSystems(DeltaTime);

}

