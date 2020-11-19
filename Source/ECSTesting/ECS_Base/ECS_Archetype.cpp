// Fill out your copyright notice in the Description page of Project Settings.

#include "ECS_Archetype.h"
#include "ECS_ComponentWrapper.h"


// Sets default values
AECS_Archetype::AECS_Archetype()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}


EntityHandle AECS_Archetype::CreateNewEntityFromThis(ECS_World * _ECS)
{
	EntityHandle h = _ECS->NewEntity();

	for (auto c : GetComponents())
	{
		auto wr = Cast<IComponentWrapper>(c);
		if (wr)
		{
			wr->AddToEntity(_ECS, h);
		}
	}

	return h;
}

