#pragma once


//#include "entt/entt.hpp"

#include "entt/entity/registry.hpp"
#include "concurrentqueue.h"
#include <vector>
#include "ECSTesting.h"
#include "LinearMemory.h"
#include "Map.h"
#include "String.h"
DECLARE_CYCLE_STAT(TEXT("ECS: Total System Update"), STAT_TotalUpdate, STATGROUP_ECS);


using EntityID = uint32_t;
using ECS_Registry = entt::Registry<EntityID>;
class ECS_World;
class AActor;

struct EntityHandle {
	EntityHandle(EntityID h = 0): handle(h) {};
	EntityID handle;
};

class SystemTaskGraph;

struct System {

	AActor* OwnerActor;
	ECS_World* World;
	FString name;
	virtual ~System() {}
	virtual void initialize(AActor* _Owner, ECS_World* _World) {
		OwnerActor = _Owner;
		World = _World;
	};
	virtual void update(ECS_Registry& registry, float dt) = 0;

	virtual void schedule(class ECSSystemScheduler* sysScheduler)
	{
		
	};
};

struct DeletionContext {
	moodycamel::ConcurrentQueue<EntityID> entitiesToDelete;

	static DeletionContext* GetFromRegistry(ECS_Registry& registry);
	void AddToQueue(EntityID entity);
};

template<typename T, typename Traits, typename F>
void bulk_dequeue(moodycamel::ConcurrentQueue<T, Traits>& queue, F&& fun) {
	T block[Traits::BLOCK_SIZE];
	while (true)
	{
		int dequeued = queue.try_dequeue_bulk(block, Traits::BLOCK_SIZE);
		if (dequeued > 0)
		{
			for (int i = 0; i < dequeued; i++)
			{
				fun(block[i]);
			}
		}
		else
		{
			return;
		}
	}
};

class ECS_World {
public:
	~ECS_World() {
		for (auto s : systems)
		{
			delete s;
		}
		FreeLinearMemory(ScratchPad);
	}

	void InitializeSystems(AActor* _Owner)
	{
		//allocate 10 mb
		ScratchPad = AllocateLinearMemory(MEGABYTE * 10);

		Owner = _Owner;
		for (auto s : systems)
		{
			s->initialize(_Owner, this);
		}
	}


	void UpdateSystems(float DeltaTime);

	template<typename T>
	System* CreateAndRegisterSystem(FString name)
	{
		System * s = new T();
		
		if (s)
		{
			s->name = name;
			systems.push_back(s);
			namedSystems.Add(name, s);
		}
		return s;
	}

	void RegisterSystem(System* newSystem, FString name)
	{
		systems.push_back(newSystem);
		namedSystems[name] = newSystem;
	}
	EntityHandle NewEntity() {
		EntityHandle h;
		h.handle= registry.create();
		return h;
	}
	void DestroyEntity(EntityHandle ent)
	{
		registry.destroy(ent.handle);
	}

	System* GetSystem(FString name);
	void UpdateSystem(FString name, float Dt);

	ECS_Registry *GetRegistry() { return &registry; };
	FRandomStream rng;

	LinearMemory ScratchPad;

public:

	AActor * Owner;

	std::vector<System*> systems;
	TMap<FString, System*> namedSystems;
	ECS_Registry registry;
	
};