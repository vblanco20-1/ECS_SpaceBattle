// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ECS_Core.h"
#include "Battle_ECSWorld.generated.h"


UCLASS()
class ECSTESTING_API A_ECSWorldActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	A_ECSWorldActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	TUniquePtr<ECS_World> ECSWorld;	
};
