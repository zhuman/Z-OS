#include "Z-OS.h"

static void DelegateInterfaceCreate(InternalObject* obj)
{
	DelegateInternal* intObj = zmalloc(sizeof(DelegateInternal));
	obj->Data = intObj;
}

static void DelegateInterfaceDestroy(InternalObject* obj)
{
	zfree(obj->Data);
}

static Int16 DelegateInterfaceInvoke(UInt16 handle, void* arg)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		DelegateHandler* handlers;
		Int16 handlerCount;
		Int16 i;
		
		// First, copy over the list
		EnterCriticalSection();
		handlerCount = ((DelegateInternal*)(obj->Data))->Handlers.Length;
		handlers = zmalloc(sizeof(DelegateHandler) * handlerCount);
		if (!handlers)
		{
			ExitCriticalSection();
			return ErrorOutOfMemory;
		}
		for (i = 0; i < handlerCount; i++) handlers[i] = GetListItem(&(((DelegateInternal*)(obj->Data))->Handlers),i);
		ExitCriticalSection();
		
		// Execute each handler
		for (i = 0; i < handlerCount; i++) handlers[i](arg);
		zfree(handlers);
		
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

static Int16 DelegateInterfaceInvokeAsync(UInt16 handle, void* arg)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		DelegateHandler* handlers;
		Int16 handlerCount;
		Int16 i;
		IQueue* queue;
		UInt16 queueHandle;
		
		// First, copy over the list
		EnterCriticalSection();
		handlerCount = ((DelegateInternal*)(obj->Data))->Handlers.Length;
		handlers = zmalloc(sizeof(DelegateHandler) * handlerCount);
		if (!handlers)
		{
			ExitCriticalSection();
			return ErrorOutOfMemory;
		}
		for (i = 0; i < handlerCount; i++) handlers[i] = GetListItem(&(((DelegateInternal*)(obj->Data))->Handlers),i);
		((DelegateInternal*)(obj->Data))->RemainingAsync += handlerCount;
		ExitCriticalSection();
		
		// Open the queue object and grab its IQueue
		if ((ret = OpenObject("WorkerThreadQueue",&queueHandle))) return ret;
		if ((ret = GetInterface(queueHandle,CodeIQueue,(void**)&queue)))
		{
			ReleaseObject(queueHandle);
			return ret;
		}
		
		// Execute each handler
		for (i = 0; i < handlerCount; i++)
		{
			DelegateAsyncQueueItem* item = zmalloc(sizeof(DelegateAsyncQueueItem));
			if (!item) continue;
			item->Handler = handlers[i];
			item->Arg = arg;
			item->DelegateObj = (DelegateInternal*)(obj->Data);
			queue->AddItem(queueHandle,item);
		}
		
		ReleaseObject(queueHandle);
		zfree(handlers);
		
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

static Int16 DelegateInterfaceAddHandler(UInt16 handle, DelegateHandler handler)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		EnterCriticalSection();
		AddListItem(&(((DelegateInternal*)(obj->Data))->Handlers),handler);
		ExitCriticalSection();
		
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

static Int16 DelegateInterfaceRemoveHandler(UInt16 handle, DelegateHandler handler)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		Int16 i;
		EnterCriticalSection();
		i = GetIndexOf(&(((DelegateInternal*)(obj->Data))->Handlers),handler);
		if (i >= 0) RemoveListItem(&(((DelegateInternal*)(obj->Data))->Handlers),i);
		ExitCriticalSection();
		
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

static Int16 DelegateInterfaceStartWait(UInt16 handle)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		EnterCriticalSection();
		if (!(((DelegateInternal*)(obj->Data))->RemainingAsync)) return ErrorNoWait;
		ExitCriticalSection();
		
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

static Int16 DelegateGetInterface(Int16 code, void** interface)
{
	switch (code)
	{
	case CodeIGeneric:
	{
		static const IGeneric inter = 
		{
			DelegateInterfaceCreate,
			DelegateInterfaceDestroy
		};
		*interface = (void*)&inter;
		break;
	}
	case CodeIDelegate:
	{
		static const IDelegate inter = 
		{
			DelegateInterfaceInvoke,
			DelegateInterfaceInvokeAsync,
			DelegateInterfaceAddHandler,
			DelegateInterfaceRemoveHandler
		};
		*interface = (void*)&inter;
		break;
	}
	case CodeIWaitable:
	{
		static const IWaitable inter = 
		{
			DelegateInterfaceStartWait
		};
		*interface = (void*)&inter;
		break;
	}
	default:
		return ErrorInvalidInterface;
	}
	return ErrorSuccess;
}

static void WorkerThreadProc(void* arg)
{
	UInt16 mutexHandle;
	UInt16 queueHandle;
	ISemaphore* semaphore;
	IQueue* queue;
	DelegateAsyncQueueItem* item;
	
	if (OpenObject("WorkerThreadMutex", &mutexHandle) != ErrorSuccess) return;
	if (GetInterface(mutexHandle, CodeISemaphore, (void**)&semaphore) != ErrorSuccess)
	{
		ReleaseObject(mutexHandle);
		return;
	}
	if (OpenObject("WorkerThreadQueue", &queueHandle) != ErrorSuccess) return;
	if (GetInterface(queueHandle,CodeIQueue,(void**)&queue) != ErrorSuccess)
	{
		ReleaseObject(queueHandle);
		return;
	}
	
	for (;;)
	{
		// Acquire the mutex
		semaphore->QueueCapture(mutexHandle);
		WaitForObject(mutexHandle);
		
		// Get the next item in the queue
		WaitForObject(queueHandle);
		queue->GetItem(queueHandle,(void**)&item);
		
		// Release the mutex to allow other worker threads to process work items
		semaphore->ReleaseCapture(mutexHandle);
		
		// Execute the work item
		puts("Executing delegate handler async.\r\n");
		item->Handler(item->Arg);
		puts("Done executing handler async.\r\n");
		EnterCriticalSection();
		item->DelegateObj->RemainingAsync--;
		if (!(item->DelegateObj->RemainingAsync))
		{
			FinishWait(item->DelegateObj, NULL);
		}
		ExitCriticalSection();
		zfree(item);
	}
}

void InitializeDelegates(void)
{
	UInt16 queueHandle;
	UInt16 mutexHandle;
	IThread* thread;
	UInt16 threadHandle;
	Int16 i;
	
	// Register the Delegate type
	TypeRegistration type;
	type.Type = TypeDelegate;
	type.GetInterface = (void*)DelegateGetInterface;
	RegisterTypeManager(type);
	
	// Create the mutex and queue
	if (CreateObject(TypeSemaphore,&mutexHandle, "WorkerThreadMutex") != ErrorSuccess) return;
	ReleaseObject(mutexHandle);
	if (CreateObject(TypeQueue, &queueHandle, "WorkerThreadQueue") != ErrorSuccess) return;
	ReleaseObject(queueHandle);
	
	// Init the system worker threads
	for (i = 0; i < SystemWorkerThreadCount; i++)
	{
		char name[20] = {0};
		sprintf(name, "WorkerThread%d", i + 1);
		if (CreateObject(TypeThread, &threadHandle, name) != ErrorSuccess) continue;
		if (GetInterface(threadHandle,CodeIThread,(void**)&thread) != ErrorSuccess) continue;
		if (thread->Start(threadHandle,WorkerThreadProc,(void*)i) != ErrorSuccess) continue;
		ReleaseObject(threadHandle);
	}
}
