#pragma once


#include "entt/entt.hpp"

#include <vector>
#include "ECSTesting.h"
#include "LinearMemory.h"
DECLARE_CYCLE_STAT(TEXT("ECS: Total System Update"), STAT_TotalUpdate, STATGROUP_ECS);


using EntityID = std::uint64_t;
using ECS_Registry = entt::Registry<std::uint64_t>;
class ECS_World;
class AActor;

struct EntityHandle {
	EntityHandle(EntityID h = 0): handle(h) {};
	EntityID handle;
};

struct System {

	AActor * OwnerActor;
	ECS_World * World;

	virtual ~System() {}
	virtual void initialize(AActor * _Owner, ECS_World * _World) {
		OwnerActor = _Owner;
		World = _World;
	};
	virtual void update(ECS_Registry &registry, float dt) = 0;

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


	void UpdateSystems(float DeltaTime)
	{
		SCOPE_CYCLE_COUNTER(STAT_TotalUpdate);

		for (auto s : systems)
		{
			s->update(registry, DeltaTime);
		}
	}

	template<typename T>
	System* CreateAndRegisterSystem()
	{
		System * s = new T();
		if (s)
		{
			systems.push_back(s);
		}
		return s;
	}

	void RegisterSystem(System* newSystem)
	{
		systems.push_back(newSystem);
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

	ECS_Registry *GetRegistry() { return &registry; };
	FRandomStream rng;

	LinearMemory ScratchPad;

protected:

	AActor * Owner;

	std::vector<System*> systems;

	ECS_Registry registry;
	
};