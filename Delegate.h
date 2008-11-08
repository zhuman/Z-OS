#ifndef _DELEGATE_HEADER_
#define _DELEGATE_HEADER_

typedef struct
{
	List Handlers;
	UInt16 RemainingAsync;
} DelegateInternal;

typedef void (*DelegateHandler)(void* arg);

typedef struct
{
	// Execute the delegate on the current thread
	Int16 (*Invoke)(UInt16 handle, void* arg);
	// Execute the delegate on separate system worker threads
	Int16 (*InvokeAsync)(UInt16 handle, void* arg);
	// Add a handler proc to be executed whenever the delegate is executed.
	Int16 (*AddHandler)(UInt16 handle, DelegateHandler handler);
	// Removes a previously added handler from the internal list of handlers.
	Int16 (*RemoveHandler)(UInt16 handle, DelegateHandler handler);
} IDelegate;

typedef struct
{
	DelegateHandler Handler;
	void* Arg;
	DelegateInternal* DelegateObj;
} DelegateAsyncQueueItem;

void InitializeDelegates(void);

#endif
