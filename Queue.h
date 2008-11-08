#ifndef _QUEUE_HEADER_
#define _QUEUE_HEADER_

typedef struct
{
	List Items;
} QueueInternal;

typedef struct
{
	Int16 (*AddItem)(UInt16 handle, void* item);
	Int16 (*Peek)(UInt16 handle, void** item);
	Int16 (*GetItem)(UInt16 handle, void** item);
} IQueue;

void InitializeQueues(void);

#endif
