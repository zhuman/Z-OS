#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <p24Hxxxx.h>
#include "Z-OS.h"

List Threads = {0};

unsigned int** CurrentStackPointer;
unsigned int CurrentCritNesting = 0;
ThreadInternal* CurrentThread;

UInt64 SystemTime = 0;

UInt8 ThreadManagerInitialized = 0;

// Contains lists of currently running threads in each priority level.
ThreadPriorityList RunningThreads[8] = {{0},{0},{0},{0},{0},{0},{0},{0}};

#define RESTORE_CONTEXT()																							\
	asm volatile(	"MOV	_CurrentStackPointer, W0		\n"	/* Restore the stack pointer for the task. */		\
					"MOV	[W0], W15				\n"																\
					"POP	W0						\n"	/* Restore the critical nesting counter for the task. */	\
					"MOV	W0, _CurrentCritNesting	\n"																\
					"POP	PSVPAG					\n"																\
					"POP	CORCON					\n"																\
					"POP	TBLPAG					\n"																\
					"POP	RCOUNT					\n"	/* Restore the registers from the stack. */					\
					"POP	W14						\n"																\
					"POP.D	W12						\n"																\
					"POP.D	W10						\n"																\
					"POP.D	W8						\n"																\
					"POP.D	W6						\n"																\
					"POP.D	W4						\n"																\
					"POP.D	W2						\n"																\
					"POP.D	W0						\n"																\
					"POP	SR						  " );


#define SAVE_CONTEXT()																								\
	asm volatile(	"PUSH	SR						\n"	/* Save the SR used by the task.... */						\
					"PUSH	W0						\n"	/* ....then disable interrupts. */							\
					"MOV	#32, W0				\n"																	\
					"MOV	W0, SR					\n"																\
					"PUSH	W1						\n"	/* Save registers to the stack. */							\
					"PUSH.D	W2						\n"																\
					"PUSH.D	W4						\n"																\
					"PUSH.D	W6						\n"																\
					"PUSH.D W8						\n"																\
					"PUSH.D W10						\n"																\
					"PUSH.D	W12						\n"																\
					"PUSH	W14						\n"																\
					"PUSH	RCOUNT					\n"																\
					"PUSH	TBLPAG					\n"																\
					"PUSH	CORCON					\n"																\
					"PUSH	PSVPAG					\n"																\
					"MOV	_CurrentCritNesting, W0	\n"	/* Save the critical nesting counter for the task. */		\
					"PUSH	W0						\n"																\
					"MOV	_CurrentStackPointer, W0\n"	/* Save the new top of stack into the TCB. */				\
					"MOV	W15, [W0]				  ");

// Initializes a thread's stack to start in ThreadProc with the specified params.
// Pass a pointer to the beginning of the thread's allocated stack.
// Returns the new top for the stack. This should be stored in the thread's structure.
unsigned int* InitStack(unsigned int* stack, ThreadStartParams* params)
{
	const unsigned int InitialStack[] =
	{
		0x1111,	// W1
		0x2222, // W2
		0x3333, // W3
		0x4444, // W4
		0x5555, // W5
		0x6666, // W6
		0x7777, // W7
		0x8888, // W8
		0x9999, // W9
		0xaaaa, // W10
		0xbbbb, // W11
		0xcccc, // W12
		0xdddd, // W13
		0xeeee, // W14
		0xcdce, // RCOUNT
		0xabac // TBLPAG
	};
	int i = 0;
	
	// Save the low bytes of the program counter.
	*(stack++) = (unsigned int)ThreadProc;

	// Save the high byte of the program counter.  This will always be zero
	// here as it is passed in a 16bit pointer.  If the address is greater than
	// 16 bits then the pointer will point to a jump table.
	*(stack++) = 0;

	// Initial status register (SR) with interrupts enabled
	*(stack++) = 0;
	
	// Parameters are passed in W0.
	*(stack++) = (unsigned int) params;
	
	for(i = 0; i < (sizeof(InitialStack) / sizeof(unsigned int)); i++) // (sizeof(InitialStack) / sizeof(int)); i++)
	{
		*(stack++) = InitialStack[i];
	}
	
	*(stack++) = CORCON;
	*(stack++) = PSVPAG;
	
	// Finally the critical nesting depth.
	*(stack++) = 0x00;
	
	return stack;
}

// This function is our basic priority-based task-switching algorithm.
// It simply loops through the RunningThreads array in reverse, switching
// to the first highest-priority thread available.
static void SwitchContext(void)
{
	UInt16 i;
Beginning:
	EnterCriticalSection();
	CurrentThread->State = Queued;
	for (i = 7; i >= 0; i--)
	{
		if (RunningThreads[i].ThreadList.Length > 0)
		{
			InternalObject* obj;
			
			// Go back to the beginning of the list if necessary
			++(RunningThreads[i].CurrentItem);
			if ((RunningThreads[i].CurrentItem) >= RunningThreads[i].ThreadList.Length) RunningThreads[i].CurrentItem = 0;
			CurrentStackPointer = (unsigned int**)&(((ThreadInternal*)GetListItem(&(RunningThreads[i].ThreadList),RunningThreads[i].CurrentItem))->StackPointer);
			CurrentThread = ((ThreadInternal*)GetListItem(&(RunningThreads[i].ThreadList),RunningThreads[i].CurrentItem));
			CurrentThread->State = Running;
			
			// Change the "CurrentThread" object to point to the actual current thread.
			InternalObjectFromData(CurrentThread,&obj);
			ChangeSymbolicLink("CurrentThread",obj);
			
			ExitCriticalSection();
			return;
		}
	}
	// Oh no! No threads are running!
	// If we get here, bad things happen. In fact, let's go back to the
	// beginning until a better solution can be found.
	ExitCriticalSection();
	goto Beginning;
}

void StartTimer1(void)
{
	T1CONbits.TON = 0; // Disable Timer
	T1CONbits.TCS = 0; // Select internal instruction cycle clock
	T1CONbits.TGATE = 0; // Disable Gated Timer mode
	T1CONbits.TCKPS = 0b10; // Select 1:1 Prescaler
	TMR1 = 0x00; // Clear timer register
	PR1 = 10000; // Load the period value
	IPC0bits.T1IP = PreemptInterruptPriority; // Set Timer 1 Interrupt Priority Level
	IFS0bits.T1IF = 0; // Clear Timer 1 Interrupt Flag
	IEC0bits.T1IE = 1; // Enable Timer1 interrupt
	T1CONbits.TON = 1; // Start Timer
}

// This thread allows the system to continue running, 
// even when no user threads are in existence. We can
// actually take advantage of this by putting the chip
// into "idle mode," which simply waits for an interrupt
// to occur.
void IdleThread(void)
{
	for(;;)
	{
		puts("Idle\r\n");
		__asm__ volatile ("PWRSAV #1");
	}
}

// This function starts the system's threading engine.
// It should not return unless an error occurs in the
// object manager. At least one thread must exist in
// the system in order for anything to actually happen.
Int16 InitializeThreading(void)
{
	Int16 ret = 0;
	UInt16 initThreadHandle = 0;
	IThread* threadInterface;
	InternalObject* idleIntObj;
	
	printf("Registering thread type...\r\n");
	{
		TypeRegistration threadType = {0};
		threadType.Type = TypeThread;
		threadType.GetInterface = ThreadGetInterface;
		if ((ret = RegisterTypeManager(threadType))) return ret;
	}
	
	// Create the system idle looping thread
	if ((ret = CreateObject(TypeThread,&initThreadHandle,"IdleThread"))) return ret;
	if ((ret = GetInterface(initThreadHandle,CodeIThread,(void*)&threadInterface))) return ret;
	threadInterface->Start(initThreadHandle,(ThreadFunction)IdleThread,0);
	InternalObjectFromHandle(initThreadHandle, &idleIntObj);
	
	// Create the "CurrentThread" symbolic link
	if ((ret = CreateSymbolicLink("CurrentThread", idleIntObj))) return ret;
	
	// Release the idle thread
	ReleaseObject(initThreadHandle);
	
	// Create the StartupThreadProc thread, implemented by the user
	if ((ret = CreateObject(TypeThread,&initThreadHandle,"StartupThread"))) return ret;
	if ((ret = GetInterface(initThreadHandle,CodeIThread,(void*)&threadInterface))) return ret;
	threadInterface->Start(initThreadHandle,(ThreadFunction)StartupThreadProc,0);
	ReleaseObject(initThreadHandle);
	
	Phase1Init();
	
	printf("Starting init thread...\r\n");
	
	CurrentStackPointer = (unsigned int**)&(((ThreadInternal*)GetListItem(&Threads,0))->StackPointer);
	
	// Start the timer that controls the system clock and preemption
	StartTimer1();
	// We are ready to start preempting
	ThreadManagerInitialized = True;
	
	// "Restore" the context of the first thread
	SwitchContext();
	RESTORE_CONTEXT();
	
	// Pretend to return, but we are really returning to the first thread.
	asm volatile ("return");
	
	// We'll never get here:
	return 1;
}

// Returns an interface for a thread object.
Int16 ThreadGetInterface(UInt16 code, void** interface)
{
	switch (code)
	{
	case CodeIGeneric:
		{
			const IGeneric inter =
			{
				ThreadInterfaceCreate, // Create()
				ThreadInterfaceDestroy // Destroy()
			};
			*interface = (void*)&inter;
			break;
		}
	case CodeIThread:
		{
			const IThread inter =
			{
				ThreadInterfaceGetID, // GetID()
				ThreadInterfaceSetPriority, // SetPriority()
				ThreadInterfaceGetPriority, // GetPriority()
				ThreadInterfaceStart, // Start()
				ThreadInterfaceGetState, // GetState()
				ThreadInterfaceSuspend, // Suspend()
				ThreadInterfaceResume, // Resume()
				ThreadInterfaceStop // Stop()
			};
			*interface = (void*)&inter;
			break;
		}
	case CodeIWaitable:
		{
			const IWaitable inter = 
			{
				ThreadInterfaceStartWait
			};
			*interface = (void*)&inter;
		}
	default:
		return ErrorInvalidInterface;
	}
	return ErrorSuccess;
}

// This contains the next unused thread ID
UInt16 ThreadIDCounter = 1;

// Initializes a thread object to point to a new internal thread
// structure. It also initializes the thread's initial properties.
void ThreadInterfaceCreate(InternalObject* obj)
{
	Int16 ret = 0;
	ThreadInternal* internalObj = zmalloc(sizeof(ThreadInternal));
	
	EnterCriticalSection();
	
	if (!internalObj) { ret = ErrorOutOfMemory; goto ReturnFunc; }
	
	internalObj->ThreadID = ThreadIDCounter++;
	
	// If this is the idle thread, set its priority to 0
	if (internalObj->ThreadID == 1)
	{
		internalObj->BasePriority = 0;
	}
	else
	{
		internalObj->BasePriority = 1;
	}
	internalObj->Stack = zmalloc(DefaultStackSize);
	if (!(internalObj->Stack)) { zfree(internalObj); ret = ErrorOutOfMemory; goto ReturnFunc; }
	internalObj->StackSize = DefaultStackSize;
	// Leave the stack uninitialized until the user calls Start() on it.
	internalObj->StackPointer = internalObj->Stack;
	
	obj->Flags |= ObjectFlagPermanent;
	
	// Add the thread object to the threads list point the object to it.
	AddListItem(&Threads,internalObj);
	obj->Data = internalObj;
	
	// Great success!
	ret = ErrorSuccess;
ReturnFunc:
	ExitCriticalSection();
	//return ret;
}

void ThreadInterfaceDestroy(InternalObject* obj)
{
	EnterCriticalSection();
	
	ExitCriticalSection();
}

Int16 ThreadInterfaceStartWait(UInt16 handle)
{
	InternalObject* obj;
	InternalObjectFromHandle(handle,&obj);
	if (obj && obj->Data)
	{
		ThreadInternal* thr = obj->Data;
		EnterCriticalSection();
		if (thr->State == Stopped)
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

UInt16 ThreadInterfaceGetID(UInt16 handle)
{
	InternalObject* obj;
	InternalObjectFromHandle(handle,&obj);
	if (obj && obj->Data)
	{
		UInt16 ret = 0;
		EnterCriticalSection();
		ret = ((ThreadInternal*)(obj->Data))->ThreadID;
		ExitCriticalSection();
		return ret;
	}
	else
	{
		return 0;
	}
}

void ThreadInterfaceSetPriority(UInt16 handle, UInt8 priority)
{
	InternalObject* obj;
	InternalObjectFromHandle(handle,&obj);
	if (obj)
	{
		// Limit it to the 7 user thread priorities.
		if (priority == 0) priority = 1;
		else if (priority >= 8) priority = 7;
		
		EnterCriticalSection();
		
		DequeueThread((ThreadInternal*)(obj->Data));
		((ThreadInternal*)(obj->Data))->BasePriority = priority;
		QueueThread((ThreadInternal*)(obj->Data));
		
		ExitCriticalSection();
	}
}

UInt8 ThreadInterfaceGetPriority(UInt16 handle)
{
	InternalObject* obj;
	InternalObjectFromHandle(handle,&obj);
	if (obj)
	{
		return ((ThreadInternal*)(obj->Data))->BasePriority;
	}
	else
	{
		return 0;
	}
}

UInt16 ThreadInterfaceStart(UInt16 handle, ThreadFunction startFunc, void* arg)
{
	InternalObject* obj;
	InternalObjectFromHandle(handle,&obj);
	if (obj)
	{
		ThreadInternal* thread = (ThreadInternal*)(obj->Data);
		UInt16 ret = 0;
		EnterCriticalSection();
		
		// Store the start address and the arg and then initialize the stack.
		thread->StartParams.StartFunc = startFunc;
		thread->StartParams.Parameter = arg;
		thread->StackPointer = (int*)InitStack((unsigned int*)(thread->Stack),&(thread->StartParams));
		thread->StackTopPointer = thread->Stack + thread->StackSize - TrapStackSize;
		
		// Setup the thread for running if it isn't suspended.
		if (!(thread->State & Suspended))
		{
			QueueThread(thread);
		}
		
		ExitCriticalSection();
		
		return ret;
	}
	else
	{
		return 0;
	}
}

UInt16 ThreadInterfaceGetState(UInt16 handle)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		return ((ThreadInternal*)(obj->Data))->State;
	}
	else
	{
		return 0;
	}
}

Int16 ThreadInterfaceSuspend(UInt16 handle)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		EnterCriticalSection();
		((ThreadInternal*)(obj->Data))->State |= Suspended;
		DequeueThread((ThreadInternal*)(obj->Data));
		ExitCriticalSection();
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

Int16 ThreadInterfaceResume(UInt16 handle)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj && obj->Data)
	{
		ThreadInternal* thr = (ThreadInternal*)(obj->Data);
		EnterCriticalSection();
		thr->State &= ~Suspended;
		if (thr->State != Waiting)
		{
			QueueThread(thr);
		}
		ExitCriticalSection();
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

Int16 ThreadInterfaceStop(UInt16 handle)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj && obj->Data)
	{
		ThreadInternal* thr = (ThreadInternal*)(obj->Data);
		EnterCriticalSection();
		thr->State = Stopped;
		DequeueThread(thr);
		FinishWait(thr,null);
		if (CurrentThread == thr)
		{
			YieldThread();
		}
		ExitCriticalSection();
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

Int16 WaitForObject(UInt16 handle)
{
	IWaitable* interface;
	Int16 ret;
	if ((ret = GetInterface(handle,CodeIWaitable,(void**)&interface))) return ret;
	if (interface)
	{
		EnterCriticalSection();
		if ((ret = interface->StartWait(handle)))
		{
			ExitCriticalSection();
			return ret;
		}
		DequeueThread(CurrentThread);
		AddListItem(&(CurrentThread->WaitHandles),(void*)handle);
		CurrentThread->State = Waiting;
		
		// Yield to another thread until the wait has completed
		YieldThread();
		
		ExitCriticalSection();
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidInterface;
	}
}

Int16 WaitForObjects(UInt16 handles[], UInt16 handleCount, Bool all)
{
	IWaitable* interface;
	Int16 ret = ErrorNoWait;
	int i;
	EnterCriticalSection();
	CurrentThread->WaitMode = all ? WaitAll : WaitAny;
	for (i = 0; i < handleCount; i++)
	{
		if ((ret = GetInterface(handles[i],CodeIWaitable,(void**)&interface))) continue;
		if (interface)
		{
			if ((ret = interface->StartWait(handles[i])))
			{
				continue;
			}
			AddListItem(&(CurrentThread->WaitHandles),(void*)handles[i]);
		}
		interface = (IWaitable*)0;
	}
	if (CurrentThread->WaitHandles.Length > 0)
	{
		DequeueThread(CurrentThread);
		CurrentThread->State = Waiting;
		
		// Yield to another thread until the wait has completed
		YieldThread();
		
		ExitCriticalSection();
		return ErrorSuccess;
	}
	else
	{
		ExitCriticalSection();
		return ret;
	}
}

Int16 FinishWait(void* data, ThreadInternal* specThread)
{
	int i;
	EnterCriticalSection();
	for (i = 0; i < Threads.Length; i++)
	{
		int j;
		ThreadInternal* thread = GetListItem(&Threads,i);
		if (specThread && (specThread != thread)) continue;
		
		for (j = 0; j < thread->WaitHandles.Length; j++)
		{
			InternalObject* obj;
			if (InternalObjectFromHandle((UInt16)GetListItem(&(thread->WaitHandles),j),&obj) == ErrorSuccess)
			{
				if (obj && (obj->Data == data))
				{
					RemoveListItem(&thread->WaitHandles,j);
					if (((thread->WaitMode == WaitAll) && (thread->WaitHandles.Length == 0)) || (thread->WaitMode == WaitAny))
					{
						ClearList(&(thread->WaitHandles));
						if (thread->State & Suspended)
						{
							thread->State = Suspended;
						}
						else
						{
							QueueThread(thread);
						}
					}
					break; // Next thread
				}
			}
		}
	}
	ExitCriticalSection();
	return ErrorSuccess;
}

void QueueThread(ThreadInternal* thread)
{
	EnterCriticalSection();
	thread->State = Queued;
	AddListItem(&(RunningThreads[thread->BasePriority].ThreadList),thread);
	ExitCriticalSection();
}

void DequeueThread(ThreadInternal* thread)
{
	int i;
	EnterCriticalSection();
	for (i = 0; i < RunningThreads[thread->BasePriority].ThreadList.Length; i++)
	{
		if (GetListItem(&(RunningThreads[thread->BasePriority].ThreadList),i) == thread)
		{
			RemoveListItem(&(RunningThreads[thread->BasePriority].ThreadList),i);
			break;
		}
	}
	thread->State = (thread->State & Suspended) ? Suspended : Stopped;
	ExitCriticalSection();
}

volatile UInt16 NumRunningThreads = 0;

// This function is the actual starting address of a thread
// and serves to provide a hard-coded backup in case the user's
// thread routine returns. It also makes it easier on the user
// since they can simply return to stop their thread. It will
// call DequeueThread() on the current thread in order to stop being
// switched to and then enter an infinite loop.
void ThreadProc(ThreadStartParams* params)
{
	EnterCriticalSection();
	
	printf("Starting thread at address 0x%x\r\n",(UInt16)(params->StartFunc));
	
	// Increment the number of running threads to detect shutdowns
	NumRunningThreads++;
	
	ExitCriticalSection();
	
	// Call the user's function.
	params->StartFunc(params->Parameter);
	
	// The thread has ended.
	printf("Ending thread with start address 0x%x\r\n",(UInt16)(params->StartFunc));
	
	EnterCriticalSection();
	{
		InternalObject* obj;
		InternalObjectFromData(CurrentThread,&obj);
		
		CurrentThread->State = Stopped;
		obj->KernelReferences--;
		
		NumRunningThreads--;
		
		// Remove the thread from the thread queue
		DequeueThread(CurrentThread);
		
		// Join any threads waiting on this thread
		FinishWait(CurrentThread,null);
		
		// If no other threads are running, reset the system
		if (!NumRunningThreads)
		{
			puts("All threads on the system have exited. The system may have become unstable. Resetting...\r\n");
			__asm__ ("reset");
		}	
		
		// Force a context switch
		YieldThread();
	}
	ExitCriticalSection();
	
	for (;;);
}

// Enters a critical section by disabling interrupts.
void EnterCriticalSection(void)
{
	SR |= PreemptInterruptPriority << 5;
	CurrentCritNesting++;
 }	

// Returns from a critical section by re-enabling interrupts.
void ExitCriticalSection(void)
{
	if (CurrentCritNesting)
	{
		if (!(--CurrentCritNesting)) SR &= ~(PreemptInterruptPriority << 5);
	}
	else
	{
		printf("An extra ExitCriticalSection() has been detected.\r\n Did you forget to EnterCriticalSection()?\r\n");
	}
}

// Performs a thread switch to the next available thread.
void InternalYieldThread(void)
{
	SAVE_CONTEXT();
	SwitchContext();
//	if ((int*)SPLIM < *CurrentStackPointer) SPLIM = (UInt16)(CurrentThread->StackTopPointer);
	RESTORE_CONTEXT();
//	SPLIM = (UInt16)(CurrentThread->StackTopPointer);
}
volatile Int8 ThreadYielded = False;

void YieldThread(void)
{
	EnterCriticalSection();
	ThreadYielded = True;
	asm volatile ("CALL _InternalYieldThread \n NOP");
	ThreadYielded = True;
	ExitCriticalSection();
}

// Returns the system time in Timer1 ticks.
UInt64 GetSystemTime(void)
{
	UInt64 ret = 0;
	EnterCriticalSection();
	ret = SystemTime;
	ExitCriticalSection();
	return ret;
}

Int16 DidPreempt = False;
Int16 PreemptTimes = 0;

// Preempts the current thread, increments the system clock,
// increments any waiting timers, etc.
void __attribute__((interrupt,auto_psv)) _T1Interrupt(void)
{
	SystemTime++;
	
	// Stop the interrupt
	IFS0bits.T1IF = 0;
	
	// Increment any running timers and perform 
	// thread rescheduling if necessary
	TimerInterruptHandler();
	
	if (ThreadManagerInitialized)
	{
		//if (!ThreadYielded)
		{
			asm volatile ("CALL _InternalYieldThread \n NOP");
			DidPreempt = True;
			//PreemptTimes++;
		}
		/*else
		{
			DidPreempt = False;
			ThreadYielded = False;
		}*/
	}
}

// These error interrupts should immediately kill the
// current thread, or even reset the processor.
//#define ResetOnError

#define _WREG15 WREG15

void __attribute__((interrupt, auto_psv)) _AddressError(void)
{
	printf("Address Error\r\n Occurred in thread %d at 0x%X\r\n",CurrentThread->ThreadID,*(UInt16*)(_WREG15 - 24));
#ifdef ResetOnError
	__asm__ volatile ("reset");
#else
	CurrentThread->State = Stopped;
	SR &= ~(PreemptInterruptPriority << 5);
	DequeueThread(CurrentThread);
	INTCON1bits.ADDRERR = 0;
	IFS0bits.T1IF = 1;
#endif
}

void __attribute__((interrupt, auto_psv)) _StackError(void)
{
	printf("Stack Error\r\n Occurred in thread %d at 0x%X\r\n",CurrentThread->ThreadID,*(UInt16*)(_WREG15 - 24));
#ifdef ResetOnError
	__asm__ volatile ("reset");
#else
	CurrentThread->State = Stopped;
	SR &= ~(PreemptInterruptPriority << 5);
	DequeueThread(CurrentThread);
	INTCON1bits.STKERR = 0;
	IFS0bits.T1IF = 1;
#endif
}

void __attribute__((interrupt, auto_psv)) _MathError(void)
{
	if (INTCON1bits.DIV0ERR)
	{
		printf("Divide by 0\r\n Ocurred in thread %d at 0x%X\r\n",CurrentThread->ThreadID,*(UInt16*)(_WREG15 - 24));
	}
	else
	{
		printf("Math Error\r\n Occurred in thread %d at 0x%X\r\n",CurrentThread->ThreadID,*(UInt16*)(_WREG15 - 24));
	}
#ifdef ResetOnError
	__asm__ volatile ("reset");
#else
	CurrentThread->State = Stopped;
	SR &= ~(PreemptInterruptPriority << 5);
	DequeueThread(CurrentThread);
	INTCON1bits.DIV0ERR = 0;
	INTCON1bits.MATHERR = 0;
	IFS0bits.T1IF = 1;
#endif
}

/*void __attribute__((interrupt, auto_psv)) _DMACError(void)
{
	printf("DMA Controller Error\r\n Occurred in thread %d at 0x%x\r\n",CurrentThread->ThreadID,*(UInt16*)(_WREG15 - 24));
#ifdef ResetOnError
	__asm__ volatile ("reset");
#else
	CurrentThread->State = Stopped;
	EnterCriticalSection();
	DequeueThread(CurrentThread);
	INTCON1bits.DMACERR = 0;
	IFS0bits.T1IF = 1;
#endif
}*/
