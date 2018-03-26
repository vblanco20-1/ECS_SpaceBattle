// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ECSTesting.h"
#include "ECS_Core.h"

#include "ECS_ComponentWrapper.generated.h"

UINTERFACE()
class UComponentWrapper : public UInterface {
	GENERATED_BODY()
};

class IComponentWrapper {

	GENERATED_BODY()
public:

	virtual void AddToEntity(ECS_World * world, EntityHandle entity) {};
};