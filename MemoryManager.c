#include "Z-OS.h"
#include <stdlib.h>
#include <string.h>

Int16 TotalMemAllocs = 0;
Int64 TotalMemUsage = 0;

#define HeapSize 5000

__attribute__ ((__far__)) UInt16 Heap[HeapSize] = {0};

//#undef malloc
//#undef free

// Wraps 'malloc' in a thread-safe zeroing wrapper; but not the kind in which you can find candy
void* zmalloc2(size_t size)
{
	void* space;
	EnterCriticalSection();
	space = calloc(size,1);
	
	TotalMemAllocs++;
	TotalMemUsage = TotalMemUsage + (Int64)size;
	//if (space > MaxPointer) MaxPointer = space;
	//if (size > MaxSize) MaxSize = size;
	
	ExitCriticalSection();
	if (!space)
	{
		printf("Out of Memory\r\n");
	}
	return space;
}

// Wraps 'free' in a thread-safe wrapper
void zfree2(void* pointer)
{
	EnterCriticalSection();
	puts("Before free.\r\n");
	free(pointer);
	puts("After free.\r\n");
	TotalMemAllocs--;
	ExitCriticalSection();
}

//#define malloc zmalloc
//#define free zfree

// Initializes an alternate heap algorithm
void HeapInit(void)
{
	// Currently unneeded
}

// Allocates using an alternate heap algorithm
void* malloc(size_t size)
{
	UInt16* block = (UInt16*)Heap;
	UInt16* lastBlock = block;
	
	// Make the size even
	if (size & 1) size++;
	
	// Check to see if the memory is available
	if ((size + sizeof(UInt16) * 2) > ((HeapSize * sizeof(UInt16)) - TotalMemUsage))
	{
		return NULL;
	}
	
	EnterCriticalSection();
	for(;;)
	{
		// If we are at a free spot
		if (!(block[0]) || (!(block[1]) && ((block[0] - (UInt16)block) > ((UInt16)size + sizeof(UInt16) * 2))))
		{
			UInt16* oldBlock = (UInt16*)block[0];
			//UInt16 i;
			
			// If there is no more room for this block, return null
			if (((UInt16)block - (UInt16)Heap + sizeof(UInt16) * 2 + (UInt16)size) > (HeapSize * sizeof(UInt16)))
			{
				ExitCriticalSection();
				return NULL;
			}
			
			block[1] = (UInt16)lastBlock;
			
			if (!block[0] || (block[0] - (UInt16)block > (UInt16)size + sizeof(UInt16) * 2))
			{
				block[0] = (UInt16)block + sizeof(UInt16) * 2 + (UInt16)size;
				((UInt16*)(block[0]))[0] = (UInt16)oldBlock;
				((UInt16*)(block[0]))[1] = 0;
			}
			
			TotalMemAllocs++;
			TotalMemUsage += (UInt16)size;
			
			// Zero the block
			block += sizeof(UInt16) * 2;
			memset(block,0,size);
			ExitCriticalSection();
			return (void*)block;
		}
		
		lastBlock = block;
		block = (UInt16*)(block[0]);
	}
}

// Deallocates using an alternate heap algorithm
void free(void* pointer)
{
	UInt16* block = (UInt16*)((UInt16)pointer - sizeof(UInt16) * 2);
	
	EnterCriticalSection();
	
	TotalMemAllocs--;
	TotalMemUsage -= block[0] - (UInt16)block;
	
	// Look forward
	if (!((UInt16*)block[0])[1]) block[0] = ((UInt16*)block[0])[0];
	
	// Look backward
	if ((block[1] != (UInt16)block) && !((UInt16*)block[1])[1])
	{
		((UInt16*)block[1])[0] = block[0];
		if (block[0] && ((UInt16*)(block[0]))[1]) ((UInt16*)(block[0]))[1] = block[1];
		block[1] = 0;
	}
	else
	{
		// Clear the back pointer
		block[1] = 0;
	}

	ExitCriticalSection();
}
