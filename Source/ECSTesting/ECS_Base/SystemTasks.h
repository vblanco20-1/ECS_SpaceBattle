#pragma once
#include <ECS_Core.h>


struct TypeHash {
	size_t name_hash{ 0 };
	size_t matcher_hash{ 0 };

	bool operator==(const TypeHash& other)const {
		return name_hash == other.name_hash;
	}
	template<typename T>
	static constexpr const char* name_detail() {
		return __FUNCSIG__;
	}

	static inline constexpr uint64_t hash_fnv1a(const char* key) {
		uint64_t hash = 0xcbf29ce484222325;
		uint64_t prime = 0x100000001b3;

		int i = 0;
		while (key[i]) {
			uint8_t value = key[i++];
			hash = hash ^ value;
			hash *= prime;
		}

		return hash;
	}

	template<typename T>
	static constexpr size_t hash() {

		static_assert(!std::is_reference_v<T>, "dont send references to hash");
		static_assert(!std::is_const_v<T>, "dont send const to hash");
		return hash_fnv1a(name_detail<T>());
	}
};


struct TaskDependencies {


	template<typename T>
	void AddRead() {
		readHashes.Add(TypeHash::hash<T>());
	}

	template<typename T>
	void AddWrite()
	{
		writeHashes.Add(TypeHash::hash<T>());
	}

	TArray<size_t, TInlineAllocator<5>> readHashes;
	TArray<size_t, TInlineAllocator<5>> writeHashes;

	bool ConflictsWith(const TaskDependencies& other) const{
		//very dumb algo version
		
		for (auto s : writeHashes) {
			for (auto r :other.readHashes) {
				if (s == r) {
					return true;
				}
			}
			for (auto r : other.writeHashes) {
				if (s == r) {
					return true;
				}
			}
		}

		for (auto s : readHashes) {
			for (auto r : other.writeHashes) {
				if (s == r) {
					return true;
				}
			}
		}
		return false;
	}

};
enum class ESysTaskType : uint8_t {
	GameThread,
	SyncPoint,
	FreeTask
};

enum class ESysTaskFlags : uint32_t {
	ExecuteAsync = 1 << 0,
	ExecuteGameThread = 1 << 1,
	HighPriority = 1 << 2
};

struct SystemTask {
	ESysTaskType type;
	ESysTaskFlags flags;
	TaskDependencies deps;
	SystemTask* next = nullptr;
	
	class SystemTaskChain* ownerGraph{nullptr};
	TFunction<void(ECS_Registry&)> function;
};

struct GraphTask {
	SystemTask* original;
	FString TaskName;
	int chainIndex = 0;
	float priorityWeight = 0;
	int predecessorCount = 1;
	TArray<GraphTask*, TInlineAllocator<2>> successors;

	void AddSuccesor(GraphTask* succesor) {
		succesor->predecessorCount++;
		successors.Add(succesor);
	}
	void BuildName();
};

class SystemTaskChain {
public:
	SystemTaskChain* next{nullptr};
	SystemTask* firstTask{ nullptr };
	SystemTask* lastTask{ nullptr };
	int sortKey;
	FString name;
	float priority = 1.f;
	TArray<FString, TInlineAllocator<2>> SystemDependencies;

	bool HasSyncPoint() {
		SystemTask* task = firstTask;
		while (task)
		{
			if (task->type == ESysTaskType::SyncPoint) {
				return true;
			}
			task = task->next;
		}
		return false;
	}

	void ExecuteSync(ECS_Registry& reg) {
		SystemTask* task = firstTask;
		while (task)
		{
			task->function(reg);
			task = task->next;
		}
	};

	void AddTask(SystemTask* task) {
		if (firstTask == nullptr) {
			firstTask = task;
			lastTask = task;
		}
		else {
			lastTask->next = task;
			lastTask = task;
		}
	}
};


class SystemTaskBuilder {
public:
	SystemTaskBuilder(FString name, int sortkey, class ECSSystemScheduler* _scheduler, float Priority = 1.f) {
	
		graph = new SystemTaskChain();
		graph->name = name;
		graph->sortKey = sortkey;
		graph->priority = Priority;
		scheduler = _scheduler;
	};
	
	template< typename C>
	void AddTask(const TaskDependencies& deps, C&& c , ESysTaskFlags flags = ESysTaskFlags::ExecuteAsync) {
		SystemTask* task = scheduler->NewTask();
		task->deps = deps;
		task->function = std::move(c);
		task->type = ESysTaskType::FreeTask;
		task->flags = flags;
		task->ownerGraph = graph;
		graph->AddTask(task);
		//graph->functions.Add(std::move(c)); 
	};

	template< typename C>
	void AddGameTask(const TaskDependencies& deps, C&& c) {
		SystemTask* task = scheduler->NewTask();
		task->deps = deps;
		task->function = std::move(c);
		task->type = ESysTaskType::GameThread;
		task->flags = ESysTaskFlags::ExecuteGameThread;
		task->ownerGraph = graph;
		graph->AddTask(task);
	};

	void AddDependency(FString dependency) {
		graph->SystemDependencies.Add(dependency);
	}


	template<typename C>
	void AddSyncTask( C&& c) {
		SystemTask* task = scheduler->NewTask();//new SystemTask();		
		task->function = std::move(c);
		task->type = ESysTaskType::SyncPoint;
		task->ownerGraph = graph;
		graph->AddTask(task);
	};

	SystemTaskChain* FinishGraph() { return graph; };
	
	SystemTaskChain* graph;
	ECSSystemScheduler* scheduler{nullptr};
};

class ECSSystemScheduler {

	struct LaunchedTask {
		GraphTask* task;
		//SystemTask* task;
		TaskDependencies dependencies;
		TFuture<void> future;
	};



public:
	
	TArray<SystemTaskChain*> systasks;
	ECS_Registry* registry;
	
	void AddTaskgraph(SystemTaskChain* newGraph);
	
	void Run(bool runParallel, ECS_Registry& reg);

#if 0
	void AsyncFinished(SystemTask* task);

	bool ExecuteTask(SystemTask* task);

	void AddPending(SystemTask* task, TFuture<void>* future);
	void RemovePending(SystemTask* task);

	bool CanExecute(SystemTask* task);
	SystemTask* syncTask;
	TArray<SystemTask*> waitingTasks;
	TArray<TSharedPtr<LaunchedTask>> pendingTasks;

	TQueue<SystemTask*> gameTasks;
#else


	void AsyncFinished(GraphTask* task);

	bool LaunchTask(GraphTask* task);

	void AddPending(GraphTask* task, TFuture<void>* future);
	void RemovePending(GraphTask* task);

	bool CanExecute(GraphTask* task);

	TArray<GraphTask*> waitingTasks;
	TArray<TSharedPtr<LaunchedTask>> pendingTasks;

	TQueue<GraphTask*> gameTasks;
	GraphTask* syncTask;
#endif
	FCriticalSection mutex;

	FCriticalSection endmutex;

	TAtomic<int> totalTasks;
	//TAtomic<int> tasksUntilSync;
	FEvent* endEvent;


	SystemTask* NewTask();
	GraphTask* NewGraphTask(SystemTask* originalTask);

	TArray<SystemTask*> AllocatedTasks;
	TArray<GraphTask*> AllocatedGraphTasks;
};

