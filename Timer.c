#include "ObjectManager.h"
#include "ThreadManager.h"
#include "Timer.h"
#include "Errors.h"
#include "MemoryManager.h"

List RunningTimers = {0};

static Int16 TimerInterfaceSetInterval(UInt16 handle, UInt32 time)
{
	InternalObject* obj;
	Int16 ret = 0;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		((TimerInternal*)(obj->Data))->Interval = time;
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

static UInt32 TimerInterfaceGetInterval(UInt16 handle)
{
	InternalObject* obj;
	Int16 ret = 0;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return 0;
	if (obj)
	{
		return ((TimerInternal*)(obj->Data))->Interval;
	}
	else
	{
		return 0;
	}
}

static Int16 TimerInterfaceStart(UInt16 handle)
{
	InternalObject* obj;
	Int16 ret = 0;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		EnterCriticalSection();
		AddListItem(&RunningTimers,obj->Data);
		((TimerInternal*)(obj->Data))->Running = True;
		ExitCriticalSection();
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

static Int16 TimerInterfaceStop(UInt16 handle)
{
	InternalObject* obj;
	Int16 ret = 0;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		int i;
		EnterCriticalSection();
		for (i = 0; i < RunningTimers.Length; i++)
		{
			if (GetListItem(&RunningTimers,i) == obj->Data) RemoveListItem

(&RunningTimers,i);
		}
		((TimerInternal*)(obj->Data))->Running = False;
		ExitCriticalSection();
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

static Int16 TimerInterfaceReset(UInt16 handle)
{
	InternalObject* obj;
	Int16 ret = 0;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		((TimerInternal*)(obj->Data))->Counter = 0;
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

static Int16 TimerInterfaceIsRunning(UInt16 handle)
{
	InternalObject* obj;
	Int16 ret = 0;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		return ((TimerInternal*)(obj->Data))->Running;
	}
	else
	{
		return False;
	}
}

static void TimerInterfaceCreate(InternalObject* obj)
{
	TimerInternal* timer = zmalloc(sizeof(TimerInternal));
	if (!timer) return;
	obj->Data = timer;
	timer->Interval = 1000000;
	timer->Counter = 0;
	timer->Running = False;
}

static void TimerInterfaceDestroy(InternalObject* obj)
{
	zfree(obj->Data);
}

static Int16 TimerInterfaceStartWait(UInt16 handle)
{
	InternalObject* obj;
	Int16 ret = 0;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		if (!((TimerInternal*)(obj->Data))->Running)
		{
			return ErrorNoWait;
		}
		AddListItem(&(((TimerInternal*)(obj->Data))->WaitingThreads),CurrentThread);
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

static Int16 TimerGetInterface(UInt16 code, void** interface)
{
	switch (code)
	{
	case CodeIGeneric:
	{
		const IGeneric inter = 
		{
			TimerInterfaceCreate,
			TimerInterfaceDestroy
		};
		*interface = (void*)&inter;
		break;
	}
	case CodeITimer:
	{
		const ITimer inter = 
		{
			TimerInterfaceSetInterval,
			TimerInterfaceGetInterval,
			TimerInterfaceStart,
			TimerInterfaceStop,
			TimerInterfaceReset,
			TimerInterfaceIsRunning
		};
		*interface = (void*)&inter;
		break;
	}
	case CodeIWaitable:
	{
		const IWaitable inter = 
		{
			TimerInterfaceStartWait
		};
		*interface = (void*)&inter;
		break;
	}
	default:
		return ErrorInvalidInterface;
	}
	return ErrorSuccess;
}

// Called from the kernel's main() function before threading even starts
void InitializeTimers(void)
{
	TypeRegistration type = {0};
	type.Type = TypeTimer;
	type.GetInterface = TimerGetInterface;
	RegisterTypeManager(type);
}

// Called from the Timer 1 interrupt, so critical sections aren't necessary
void TimerInterruptHandler(void)
{
	int i;
	for (i = 0; i < RunningTimers.Length; i++)
	{
		TimerInternal* timer = ((TimerInternal*)GetListItem(&RunningTimers,i));
		timer->Counter++;
		if (timer->Counter >= timer->Interval)
		{
			// Stop the timer
			RemoveListItem(&RunningTimers,i--);
			timer->Running = False;
			timer->Counter = 0;
			
			// Resume any waiting threads
			FinishWait(timer,(ThreadInternal*)0);
		}
	}
}
