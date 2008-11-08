#ifndef _TIMER_HEADER_
#define _TIMER_HEADER_

#include "Lists.h"

extern List RunningTimers;

typedef struct
{
	UInt32 Interval;
	UInt32 Counter;
	UInt8 Running;
	List WaitingThreads;
} TimerInternal;

// The timer interface wraps all of the functionality of a timer except the wait function.
typedef struct
{
	Int16 (*SetInterval)(UInt16 handle, UInt32 time);
	UInt32 (*GetInterval)(UInt16 handle);
	Int16 (*Start)(UInt16 handle);
	Int16 (*Stop)(UInt16 handle);
	Int16 (*Reset)(UInt16 handle);
	Int16 (*IsRunning)(UInt16 handle);
} ITimer;

void InitializeTimers(void);
void TimerInterruptHandler(void);

#endif
