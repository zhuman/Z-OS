#include "Z-OS.h"

// Semaphore object implementation

void SemaphoreInterfaceCreate(InternalObject* obj)
{
	EnterCriticalSection();
	obj->Data = zmalloc(sizeof(SemaphoreInternal));
	if (obj->Data)
	{
		((SemaphoreInternal*)(obj->Data))->MaxCaptures = 1;
	}
	ExitCriticalSection();
}

void SemaphoreInterfaceDestroy(InternalObject* obj)
{
	zfree(obj->Data);
}

UInt16 SemaphoreInterfaceGetMax(UInt16 handle)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		EnterCriticalSection();
		ret = ((SemaphoreInternal*)(obj->Data))->MaxCaptures;
		ExitCriticalSection();
		return ret;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

Int16 SemaphoreInterfaceSetMax(UInt16 handle, UInt16 max)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		EnterCriticalSection();
		((SemaphoreInternal*)(obj->Data))->MaxCaptures = max;
		ExitCriticalSection();
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

Int16 SemaphoreInterfaceQueueCapture(UInt16 handle)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		EnterCriticalSection();
		if (((SemaphoreInternal*)(obj->Data))->CapturedThreads.Length < ((SemaphoreInternal*)(obj->Data))->MaxCaptures)
		{
			AddListItem(&(((SemaphoreInternal*)(obj->Data))->CapturedThreads),CurrentThread);
		}
		else
		{
			AddListItem(&(((SemaphoreInternal*)(obj->Data))->QueuedThreads),CurrentThread);
		}
		ExitCriticalSection();

		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

Int16 SemaphoreInterfaceIsCaptured(UInt16 handle)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		int i;
		EnterCriticalSection();
		for (i = 0; i < ((SemaphoreInternal*)(obj->Data))->CapturedThreads.Length; i++)
		{
			if (GetListItem(&(((SemaphoreInternal*)(obj->Data))->CapturedThreads),i) == CurrentThread)
			{
				ExitCriticalSection();
				return True;
			}
		}
		ExitCriticalSection();
		return False;
	}
	else
	{
		return False;
	}
}

Int16 SemaphoreInterfaceReleaseCapture(UInt16 handle)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		int i;
		int wasCaptured = False;
		EnterCriticalSection();
		for (i = 0; i < ((SemaphoreInternal*)(obj->Data))->CapturedThreads.Length; i++)
		{
			if (GetListItem(&(((SemaphoreInternal*)(obj->Data))->CapturedThreads),i) == CurrentThread)
			{
				RemoveListItem(&(((SemaphoreInternal*)(obj->Data))->CapturedThreads),i);
				wasCaptured = True;
				break;
			}
		}
		// Were we actually captured to begin with?
		if (!wasCaptured)
		{
			ExitCriticalSection();
			return ErrorSuccess;
		}
		if (((SemaphoreInternal*)(obj->Data))->CapturedThreads.Length < ((SemaphoreInternal*)(obj->Data))->MaxCaptures && ((SemaphoreInternal*)(obj->Data))->QueuedThreads.Length > 0)
		{
			ThreadInternal* waitingThr = GetListItem(&(((SemaphoreInternal*)(obj->Data))->QueuedThreads),0);
			AddListItem(&(((SemaphoreInternal*)(obj->Data))->CapturedThreads),waitingThr);
			FinishWait(obj->Data,waitingThr);
			RemoveListItem(&(((SemaphoreInternal*)(obj->Data))->QueuedThreads),0);
		}
		YieldThread();
		ExitCriticalSection();
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

Int16 SemaphoreInterfaceStartWait(UInt16 handle)
{
	InternalObject* obj;
	Int16 ret;
	if (SemaphoreInterfaceIsCaptured(handle)) return ErrorNoWait;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

Int16 SemaphoreGetInterface(UInt16 code, void** interface)
{
	switch (code)
	{
	case CodeIGeneric:
	{
		const IGeneric inter = 
		{
			SemaphoreInterfaceCreate,
			SemaphoreInterfaceDestroy
		};
		*interface = (void*)&inter;
		break;
	}
	case CodeISemaphore:
	{
		const ISemaphore inter = 
		{
			SemaphoreInterfaceGetMax,
			SemaphoreInterfaceSetMax,
			SemaphoreInterfaceQueueCapture,
			SemaphoreInterfaceIsCaptured,
			SemaphoreInterfaceReleaseCapture
		};
		*interface = (void*)&inter;
		break;
	}
	case CodeIWaitable:
	{
		const IWaitable inter = 
		{
			SemaphoreInterfaceStartWait
		};
		*interface = (void*)&inter;
		break;
	}
	default:
		return ErrorInvalidInterface;
	}
	return ErrorSuccess;
}

// Event object implementation

void EventInterfaceCreate(InternalObject* obj)
{
	obj->Data = zmalloc(sizeof(EventInternal));
}

void EventInterfaceDestroy(InternalObject* obj)
{
	zfree(obj->Data);
}

Int16 EventInterfaceSet(UInt16 handle)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		EnterCriticalSection();
		((EventInternal*)(obj->Data))->State = True;
		FinishWait(obj->Data,0);
		ExitCriticalSection();
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

Int16 EventInterfacePulse(UInt16 handle)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		EnterCriticalSection();
		if (!((EventInternal*)(obj->Data))->State)
		{
			((EventInternal*)(obj->Data))->State = False;
			FinishWait(obj->Data,0);
		}
		ExitCriticalSection();
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

Int16 EventInterfaceReset(UInt16 handle)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		EnterCriticalSection();
		((EventInternal*)(obj->Data))->State = False;
		ExitCriticalSection();
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

Int16 EventInterfaceStartWait(UInt16 handle)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		EnterCriticalSection();
		if (((EventInternal*)(obj->Data))->State)
		{
			ExitCriticalSection();
			return ErrorNoWait;
		}
		ExitCriticalSection();
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

Int16 EventInterfaceIsSet(UInt16 handle)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		EnterCriticalSection();
		if (((EventInternal*)(obj->Data))->State)
		{
			ExitCriticalSection();
			return True;
		}
		else
		{
			ExitCriticalSection();
			return False;
		}
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

Int16 EventGetInterface(UInt16 code, void** interface)
{
	switch (code)
	{
	case CodeIGeneric:
	{
		const IGeneric inter = 
		{
			EventInterfaceCreate,
			EventInterfaceDestroy
		};
		*interface = (void*)&inter;
		break;
	}
	case CodeIEvent:
	{
		const IEvent inter = 
		{
			EventInterfaceSet,
			EventInterfacePulse,
			EventInterfaceReset,
			EventInterfaceIsSet
		};
		*interface = (void*)&inter;
		break;
	}
	case CodeIWaitable:
	{
		const IWaitable inter = 
		{
			EventInterfaceStartWait
		};
		*interface = (void*)&inter;
		break;
	}
	default:
		return ErrorInvalidInterface;
	}
	return ErrorSuccess;
}

// Called before threading starts to register the type objects
void InitializeSync(void)
{
	TypeRegistration semaphore;
	TypeRegistration event;
	semaphore.Type = TypeSemaphore;
	event.Type = TypeEvent;
	semaphore.GetInterface = SemaphoreGetInterface;
	event.GetInterface = EventGetInterface;
	RegisterTypeManager(semaphore);
	RegisterTypeManager(event);
}
