#ifndef _SYNC_HEADER_
#define _SYNC_HEADER_

typedef struct
{
	UInt16 MaxCaptures;
	List CapturedThreads;
	List QueuedThreads;
} SemaphoreInternal;

typedef struct
{
	UInt8 State;
} EventInternal;

typedef struct
{
	UInt16 (*GetMax)(UInt16 handle);
	Int16 (*SetMax)(UInt16 handle, UInt16 max);
	Int16 (*QueueCapture)(UInt16 handle);
	Int16 (*IsCaptured)(UInt16 handle);
	Int16 (*ReleaseCapture)(UInt16 handle);
} ISemaphore;

typedef struct
{
	Int16 (*Set)(UInt16 handle);
	Int16 (*Pulse)(UInt16 handle);
	Int16 (*Reset)(UInt16 handle);
	Int16 (*IsSet)(UInt16 handle);
} IEvent;

void InitializeSync(void);

#endif
