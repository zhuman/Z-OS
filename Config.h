////////////////////////////
//   Z-OS Configuration   //
////////////////////////////

// Edit these configuration options to control how Z-OS functions.

// The number of system worker threads in the system to
// be used for asynchronous delegates.
#define SystemWorkerThreadCount	3

// The priority of the timer interrupt used to preempt threads. 
// This must not only be changed here, but must be changed 
// in the ThreadManager.c assembly section.
#define PreemptInterruptPriority	1

// The size of the stack for each thread. Stack overflows are 
// not usually detected, so it must be large enough to contain 
// the maximum amount of stack and small enough to fit once per 
// thread in the heap.
#define DefaultStackSize			0x300

// The size of the stack space left for trap handlers.
#define TrapStackSize				0x20

// Devices
#define UseSerial
#define UseSPI
