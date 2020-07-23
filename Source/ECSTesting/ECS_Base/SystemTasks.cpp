#include "SystemTasks.h"

DECLARE_CYCLE_STAT(TEXT("TaskSys: SyncPoint"), STAT_TS_SyncPoint, STATGROUP_ECS);
DECLARE_CYCLE_STAT(TEXT("TaskSys: GameTask"), STAT_TS_GameTask, STATGROUP_ECS);
DECLARE_CYCLE_STAT(TEXT("TaskSys: AsyncTask"), STAT_TS_AsyncTask, STATGROUP_ECS);


DECLARE_CYCLE_STAT(TEXT("TaskSys: TaskEnd1"), STAT_TS_End1, STATGROUP_ECS);

DECLARE_CYCLE_STAT(TEXT("TaskSys: TaskEnd2"), STAT_TS_End2, STATGROUP_ECS);
DECLARE_CYCLE_STAT(TEXT("TaskSys: WaitTime"), STAT_TS_Wait, STATGROUP_ECS);
DECLARE_CYCLE_STAT(TEXT("TaskSys: SyncLoop"), STAT_TS_SyncLoop, STATGROUP_ECS);

SystemTask* nextTask(SystemTask* task) {
	if (task->next) { return task->next; }
	else {	
		if (task->ownerGraph->next) {
			return (task->ownerGraph->next->firstTask);
		}		
	}
	return nullptr;
}

void ECSSystemScheduler::AddTaskgraph(SystemTaskGraph* newGraph)
{
	this->systasks.Add(newGraph);
}

void ECSSystemScheduler::Run(bool runParallel, ECS_Registry& reg)
{
	registry = &reg;
	systasks.Sort([](const SystemTaskGraph& tskA, const  SystemTaskGraph& tskB) {
		return tskA.sortKey < tskB.sortKey;
		});
	if (!runParallel) {
		for (auto s : systasks) {
			s->ExecuteSync(reg);
		}
	}
	else {

		endEvent = FPlatformProcess::GetSynchEventFromPool(false);
		//connect the tasks into a linked list
		for (int i = 0; i < systasks.Num() - 1; i++) {
			systasks[i]->next = systasks[i + 1];
		}

		//find how many tasks in total
		tasksUntilSync = 0;
		totalTasks = 0;
		SystemTask* itTask = systasks[0]->firstTask;
		while (itTask) {
			totalTasks++;
			itTask = nextTask(itTask);
		}


		//run the initial sync-d tasks
		itTask = systasks[0]->firstTask;
		while (itTask) {
			if (itTask->type != ESysTaskType::SyncPoint) {
				break;
			}
			else {
				SCOPE_CYCLE_COUNTER(STAT_TS_SyncPoint);
				//UE_LOG(LogFlying, Warning, TEXT("Executing sync task: %s"), *itTask->ownerGraph->name);

				itTask->function(reg);
				itTask = nextTask(itTask);
				totalTasks--;
			}
		}
		//bool bcontinue = true;
		while (totalTasks > 0) {

			//find how many tasks until first sync, and gather initial tasks
			//itTask = systasks[0]->firstTask;
			tasksUntilSync = 0;

			//run the initial sync-d tasks		
			while (itTask) {
				if (itTask->type != ESysTaskType::SyncPoint) {
					break;
				}
				else {
					SCOPE_CYCLE_COUNTER(STAT_TS_SyncPoint);

					//UE_LOG(LogFlying, Warning, TEXT("Executing sync task: %s"), *itTask->ownerGraph->name);


					itTask->function(reg);
					itTask = nextTask(itTask);
					totalTasks--;
				}
			}

			if (totalTasks == 0) return;

			SystemTask* checktsk = itTask;
			TArray<SystemTask*> initialTasks;

			if (checktsk->ownerGraph->firstTask != checktsk) {
				initialTasks.Add(checktsk);
			}


			while (checktsk) {
				if (checktsk->type == ESysTaskType::SyncPoint) {
					syncTask = checktsk;
					break;
				}
				if (checktsk->ownerGraph->firstTask == checktsk) {
					initialTasks.Add(checktsk);
				}
				tasksUntilSync++;
				checktsk = nextTask(checktsk);
			}

			//launch initial tasks
			for (auto t : initialTasks) {
				ExecuteTask(t);
			}

			while (tasksUntilSync.Load(EMemoryOrder::Relaxed) > 0) {

				SCOPE_CYCLE_COUNTER(STAT_TS_SyncLoop);

				SystemTask* gametask;
				while (gameTasks.Dequeue(gametask))
				{
					endmutex.Lock();

					bool bCanExecute = CanExecute(gametask);
					//endmutex.Unlock();

					if (bCanExecute) {
						//endmutex.Lock();
						AddPending(gametask,nullptr);
						endmutex.Unlock();


						//UE_LOG(LogFlying, Warning, TEXT("Executing game task: %s"), *gametask->ownerGraph->name);


						SCOPE_CYCLE_COUNTER(STAT_TS_GameTask);
						gametask->function(reg);

						AsyncFinished(gametask);
					}
					else {
						endmutex.Unlock();
						//back to the queue
						gameTasks.Enqueue(gametask);

						
					}
					
				}
				TArray<SystemTask*, TInlineAllocator<5>> executableTasks;

				SystemTask* tsk = nullptr;
				endmutex.Lock();

			
				if (waitingTasks.Num() != 0) {

					for (int i = 0; i < waitingTasks.Num(); i++) {
						if (CanExecute(waitingTasks[i])) {
							tsk = waitingTasks[i];
							waitingTasks.RemoveAt(i);
							executableTasks.Add(tsk);
							i--;
							//break;
						}
					}		
				}
				endmutex.Unlock();

				for (auto t : executableTasks) {
					ExecuteTask(t);
				}
				//if (tsk) {
				//
				//	if (!ExecuteTask(tsk)) {
				//		//special case, the task we just attempted to execute has 
				//	}
				//}
				//else 
				{
					//we ran out of stuff, wait a little bit
					//endmutex.Lock();

					endEvent->Wait();
					//pendingTasks[0]->future.Wait();

					//UE_LOG(LogFlying, Warning, TEXT("Post Wait Task"));


					//endmutex.Unlock();
				}
			}

			{
				SCOPE_CYCLE_COUNTER(STAT_TS_SyncPoint);
				syncTask->function(reg);
				itTask = nextTask(syncTask);
				totalTasks--;
			}
		}
	}
}

void ECSSystemScheduler::AsyncFinished(SystemTask* task)
{
	SCOPE_CYCLE_COUNTER(STAT_TS_End1);

	//UE_LOG(LogFlying, Warning, TEXT("Task Finished: %s"), *task->ownerGraph->name);


	endmutex.Lock();

	tasksUntilSync--;
	totalTasks--;
	
	RemovePending(task);

	endmutex.Unlock();

	endEvent->Trigger();

	//trigger execution of next task
	if (task->next) {
		ExecuteTask(task->next);
	}
	//attempt execution of a pending task
	//else
	{
		SystemTask* tsk = nullptr;
		endmutex.Lock();
		TArray<SystemTask*, TInlineAllocator<5>> executableTasks;
		if (waitingTasks.Num() != 0) {

			for (int i = 0; i < waitingTasks.Num(); i++) {
				if (CanExecute(waitingTasks[i])) {
					tsk = waitingTasks[i];
					waitingTasks.RemoveAt(i);
					executableTasks.Add(tsk);
					i--;
					//break;
				}
			}
		}
		endmutex.Unlock();

		for (auto t : executableTasks) {
			ExecuteTask(t);
		}

		//if (waitingTasks.Num() != 0) {
		//	for (int i = 0; i < waitingTasks.Num(); i++) {
		//		if (CanExecute(waitingTasks[i])) {
		//			tsk = waitingTasks[i];
		//			waitingTasks.RemoveAt(i);
		//			break;
		//		}
		//	}
		//}
		//endmutex.Unlock();
		//
		//if (tsk) {
		//	ExecuteTask(tsk);
		//}	

	}
	//if (task->next) {
	//	waitingTasks.Add(task->next);
	//}
	//else {
	//	if (task->ownerGraph->next) {
	//		waitingTasks.Add(task->ownerGraph->next->firstTask);
	//	}
	//}	

	
}

bool ECSSystemScheduler::ExecuteTask(SystemTask* task)
{
	SCOPE_CYCLE_COUNTER(STAT_TS_End2);
	endmutex.Lock();
	if (task->type == ESysTaskType::SyncPoint) {
		//syncpoints are special case
		//assert(false);
		//endmutex.Lock();
		//syncTask = task;		
	}
	else if (task->type == ESysTaskType::FreeTask || task->type == ESysTaskType::GameThread) {
		//check dependencies
		
		
		if (!CanExecute(task))
		{
			waitingTasks.Add(task);
			endmutex.Unlock();
			return false;
		}
		else{

			EAsyncExecution exec;
			if (task->type == ESysTaskType::FreeTask)
			{
				exec = EAsyncExecution::TaskGraph;

				//UE_LOG(LogFlying, Warning, TEXT("launching AsyncTask: %s"), *task->ownerGraph->name);


				TFuture<void> fut = Async(exec, [=]() {
					SCOPE_CYCLE_COUNTER(STAT_TS_AsyncTask);
					task->function(*registry);
					AsyncFinished(task);
					},
					//when it finishes					
					[=]() {
						
					}
					);

				AddPending(task,&fut);
				//TSharedPtr<LaunchedTask> newTask = MakeShared<LaunchedTask>();
				//
				//
				//newTask->task = task;
				//newTask->future = std::move(fut);
				//newTask->dependencies = task->deps;
				//
				////endmutex.Lock();
				//pendingTasks.Add(newTask);
			}
			else {				
				gameTasks.Enqueue(task);
			}

		
			//endmutex.Unlock();
		}
		//endmutex.Unlock();
	}
	
	endmutex.Unlock();
	return true;
}

void ECSSystemScheduler::AddPending(SystemTask* task, TFuture<void> *future)
{
	TSharedPtr<LaunchedTask> newTask = MakeShared<LaunchedTask>();


	newTask->task = task;
	if (future) {
		newTask->future = std::move(*future);
	}
	
	newTask->dependencies = task->deps;
	
	pendingTasks.Add(newTask);
}

void ECSSystemScheduler::RemovePending(SystemTask* task)
{
	//UE_LOG(LogFlying, Warning, TEXT("Task Removed: %s"), *task->ownerGraph->name);
	//remove from pending tasks
	for (int i = 0; i < pendingTasks.Num(); i++) {
		if (pendingTasks[i]->task == task) {
			pendingTasks.RemoveAt(i);
			break;
		}
	}
}

bool ECSSystemScheduler::CanExecute(SystemTask* task)
{
	for (auto& t : pendingTasks) {
		if (t->dependencies.ConflictsWith(task->deps)) {
			return false;
		}
	}
	return true;
}
