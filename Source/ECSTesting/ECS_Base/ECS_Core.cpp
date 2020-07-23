#include "ECS_Core.h"

#include "SystemTasks.h"

void ECS_World::UpdateSystems(float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_TotalUpdate);

	for (auto s : systems)
	{
		s->update(registry, DeltaTime);
	}
}

System* ECS_World::GetSystem(FString name)
{
	return namedSystems[name];
}

void ECS_World::UpdateSystem(FString name, float Dt)
{
	GetSystem(name)->update(registry, Dt);
}

DeletionContext* DeletionContext::GetFromRegistry(ECS_Registry& registry)
{
	if (registry.view<DeletionContext>().size() == 0) {
		EntityID id = registry.create();
		registry.assign<DeletionContext>(id);
	}

	return registry.view<DeletionContext>().raw();
}

void DeletionContext::AddToQueue(EntityID entity)
{
	entitiesToDelete.enqueue(entity);
}
