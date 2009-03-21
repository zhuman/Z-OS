#ifndef _THREAD_MANAGER_HEADER_
#define _THREAD_MANAGER_HEADER_

#include "Types.h"
#include "Lists.h"
#include "ObjectManager.h"
#include "Timer.h"

typedef struct
{
	UInt16 CurrentItem;
	List ThreadList;
} ThreadPriorityList;

// Defines the various thread states.
typedef enum
{
	Stopped = 0,
	Waiting = 1,
	Queued = 2,
	Running = 3,
	// Flags
	Suspended = 4
} ThreadStateEnum;

typedef enum
{
	WaitAny,
	WaitAll
} ThreadWaitMode;

typedef struct
{
	void (*StartFunc)(void*);
	void* Parameter;
} ThreadStartParams;

typedef struct
{
	// The ID used by the thread manager to uniquely identify the thread.
	UInt16 ThreadID;
	// The stack is allocated when the thread object is created.
	int* Stack;
	// The stack size should be the same as DefaultStackSize.
	int StackSize;
	// Points to the currently open position on the stack. Used to save and restore context.
	int* StackPointer;
	// Points to the top of the stack with some room left for interrupt handling.
	int* StackTopPointer;
	// Contains the start address and start argument of the thread.
	ThreadStartParams StartParams;
	// The current state of the thread, used to setup and update context switching.
	ThreadStateEnum State;
	// The type of wait the thread is in
	ThreadWaitMode WaitMode;
	// The priority of the thread without boosts.
	UInt8 BasePriority;
	// A list of handles a thread is currently waiting on.
	List WaitHandles;
} ThreadInternal;

typedef void (*ThreadFunction)(void*);

// This interface represents a thread and its functions.
typedef struct
{
	// Returns the ID of the thread.
	UInt16 (*GetID)(UInt16 handle);
	// Sets the priority of the thread.
	void (*SetPriority)(UInt16 handle, UInt8 priority);
	// Returns the priority of the thread.
	UInt8 (*GetPriority)(UInt16 handle);
	// Starts a thread at the given function address with the specified arguments.
	UInt16 (*Start)(UInt16 handle, ThreadFunction startFunc, void* arg);
	// Returns the current state of the thread.
	UInt16 (*GetState)(UInt16 handle);
	// Stops a thread from being switched to.
	Int16 (*Suspend)(UInt16 handle);
	// Lets a thread continue being switched to.
	Int16 (*Resume)(UInt16 handle);
	// Stops the thread and resets it. You may use it again by calling the Start() function.
	Int16 (*Stop)(UInt16 handle);
} IThread;

// This interface can be implemented by many objects to provide a way to 
typedef struct
{
	Int16 (*StartWait)(UInt16 handle);
} IWaitable;

extern ThreadInternal* CurrentThread;

Int16 InitializeThreading(void);
UInt64 GetSystemTime(void);
void YieldThread(void);
void EnterCriticalSection(void);
void ExitCriticalSection(void);
void ThreadProc(ThreadStartParams* params);

void QueueThread(ThreadInternal*);
void DequeueThread(ThreadInternal*);
Int16 FinishWait(void* data, ThreadInternal* specThread);
Int16 WaitForObject(UInt16 handle);
Int16 WaitForObjects(UInt16 handles[], UInt16 handleCount, Bool all);

// This function must be implemented by the user and
// is the first user thread started in the system.
// This should be used to start other threads and perform
// other initialization functions.
void StartupThreadProc(void);

#endif
