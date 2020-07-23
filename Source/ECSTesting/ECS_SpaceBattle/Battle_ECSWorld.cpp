// Fill out your copyright notice in the Description page of Project Settings.

#include "Battle_ECSWorld.h"
#include "ECS_Core.h"
#include "ECS_BaseSystems.h"
#include "ECS_BattleSystems.h"
#include "SystemTasks.h"


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
namespace ECSCVars
{
	// Listen server smoothing
	static int32 EnableParallel = 0;
	FAutoConsoleVariableRef CVarParallelECS(
		TEXT("p.parallelECS"),
		EnableParallel,
		TEXT("Whether to enable mesh smoothing on listen servers for the local view of remote clients.\n")
		TEXT("0: Disable, 1: Enable"),
		ECVF_Default);
}
// Called every frame
void A_ECSWorldActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SCOPE_CYCLE_COUNTER(STAT_TotalUpdate);

	

	ECSSystemScheduler* sched = new ECSSystemScheduler();
	sched->registry = &ECSWorld->registry;

	for(auto sys : ECSWorld->systems){
		 sys->schedule(sched);		
	}

	sched->Run(ECSCVars::EnableParallel == 1,ECSWorld->registry);
}

