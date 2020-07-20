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
		
		ECSWorld->CreateAndRegisterSystem<CopyTransformToECSSystem>("CopyTransform");
		ECSWorld->CreateAndRegisterSystem<BoidSystem>("Boids");

		ECSWorld->CreateAndRegisterSystem<MovementSystem>("Movement");
		ECSWorld->CreateAndRegisterSystem<ExplosionSystem>("Explosion");
		ECSWorld->CreateAndRegisterSystem<SpaceshipSystem>("Spaceship");
		ECSWorld->CreateAndRegisterSystem<RaycastSystem>("Raycast");
		ECSWorld->CreateAndRegisterSystem<LifetimeSystem>("Lifetime");
		
		ECSWorld->CreateAndRegisterSystem<DebugDrawSystem>("DebugDraw");

		
		ECSWorld->CreateAndRegisterSystem<StaticMeshDrawSystem>("StaticDraws");

		ECSWorld->CreateAndRegisterSystem<CopyTransformToActorSystem>("CopyBack");

		ECSWorld->CreateAndRegisterSystem<ArchetypeSpawnerSystem>("Spawner");		

		ECSWorld->InitializeSystems(this);
		
		
	
}

// Called every frame
void A_ECSWorldActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SCOPE_CYCLE_COUNTER(STAT_TotalUpdate);

	ECSWorld->UpdateSystem("CopyTransform",DeltaTime);

	auto inst = Async(EAsyncExecution::TaskGraph, [&]() {
		ECSWorld->UpdateSystem("Boids", DeltaTime);
		ECSWorld->UpdateSystem("Movement", DeltaTime);
		ECSWorld->UpdateSystem("Explosion", DeltaTime);
		ECSWorld->UpdateSystem("Spaceship", DeltaTime);
		});

	ECSWorld->UpdateSystem("Raycast", DeltaTime);

	inst.Wait();

	ECSWorld->UpdateSystem("Lifetime", DeltaTime);
	
	ECSWorld->UpdateSystem("DebugDraw", DeltaTime);


	auto inst2 = Async(EAsyncExecution::TaskGraph, [&]() {
		ECSWorld->UpdateSystem("StaticDraws", DeltaTime); 
	});

	ECSWorld->UpdateSystem("CopyBack", DeltaTime);

	inst2.Wait();

	ECSWorld->UpdateSystem("Spawner", DeltaTime);

}

