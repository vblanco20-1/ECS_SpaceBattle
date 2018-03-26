// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Info.h"
#include "ECS_Core.h"
#include "ECS_Archetype.generated.h"




class ECS_World;

//base class for pure entity archetypes. This class cant be used directly, but can be spawned from an ECS spawner type system
UCLASS(Blueprintable)
class ECSTESTING_API AECS_Archetype : public AInfo
{
	GENERATED_BODY()
	
public:	
	
	AECS_Archetype();
	

	virtual EntityHandle CreateNewEntityFromThis(ECS_World* _ECS);
	
};
