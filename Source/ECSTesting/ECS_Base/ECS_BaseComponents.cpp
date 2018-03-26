#include "ECS_BaseComponents.h"
#include "GameFramework/Actor.h"
#include "Battle_ECSWorld.h"
#include "EngineUtils.h"
#include "EngineMinimal.h"



UECS_ComponentSystemLink::UECS_ComponentSystemLink()
{
	PrimaryComponentTick.bCanEverTick = false;
	bAutoActivate = true;
	
}

void UECS_ComponentSystemLink::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GEngine->GetWorldFromContextObject(GetOwner(),EGetWorldErrorMode::Assert);

	// We do nothing if no is class provided, rather than giving ALL actors!
	WorldActor = nullptr;
	if (World)
	{
		for (TActorIterator<A_ECSWorldActor> It(World, A_ECSWorldActor::StaticClass()); It; ++It)
		{
			if (IsValid(*It))
			{
				WorldActor = *It;
				break;
			}
		}
	}

	if (WorldActor == nullptr)
	{
		WorldActor = World->SpawnActor<A_ECSWorldActor>(A_ECSWorldActor::StaticClass());
	}
	if (WorldActor)
	{
		myEntity = WorldActor->ECSWorld->NewEntity();
		RegisterWithECS(WorldActor->ECSWorld.Get(), myEntity);
	}
	

}

void UECS_ComponentSystemLink::RegisterWithECS(ECS_World* _ECS, EntityHandle entity)
{
	auto reg = _ECS->GetRegistry();
	auto ent = entity.handle;

	reg->accommodate<FActorReference>(ent, GetOwner());

	if (TransformSync == ETransformSyncType::Actor_To_ECS || TransformSync == ETransformSyncType::BothWays)
	{
		reg->accommodate<FCopyTransformToECS>(ent,CopyToECS);
	}
	if (TransformSync == ETransformSyncType::ECS_To_Actor	|| TransformSync == ETransformSyncType::BothWays)
	{
		reg->accommodate<FCopyTransformToActor>(ent, CopyToActor);
	}
	reg->accommodate<FGridMap>(ent);


	//request all other components to add their ECS component to this actor
	for (auto c : GetOwner()->GetComponents())
	{
		auto wr = Cast<IComponentWrapper>(c);
		if (wr)
		{
			wr->AddToEntity(_ECS, entity);
		}
	}

}
