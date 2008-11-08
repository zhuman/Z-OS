#include "Z-OS.h"
#include <stdlib.h>
#include <string.h>

Int16 TotalMemAllocs = 0;
Int64 TotalMemUsage = 0;

#define HeapSize 6000

__attribute__ ((__far__)) UInt16 Heap[HeapSize] = {0};

#undef malloc
#undef free

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

#define malloc zmalloc
#define free zfree

// Initializes an alternate heap algorithm
void HeapInit(void)
{
	// Currently unneeded
}

// Allocates using an alternate heap algorithm
void* zmalloc(size_t size)
{
	UInt16* block = (UInt16*)Heap;
	UInt16* lastBlock = block;
	
	// Make the size even
	if (size & 1) size++;
	
	EnterCriticalSection();
	for(;;)
	{
		// If we are at a free spot
		if (!(block[0]) || (!(block[1]) && ((block[0] - (UInt16)block) > ((UInt16)size + 4))))
		{
			UInt16* oldBlock = (UInt16*)block[0];
			UInt16 i;
			
			// If there is no more room for this block, return null
			if (((UInt16)block - (UInt16)Heap + 4 + (UInt16)size) > HeapSize << 1)
			{
				ExitCriticalSection();
				return null;
			}
			
			block[1] = (UInt16)lastBlock;
			
			if (!block[0] || (block[0] - (UInt16)block > (UInt16)size + 8))
			{
				block[0] = (UInt16)block + 4 + (UInt16)size;
				((UInt16*)(block[0]))[0] = (UInt16)oldBlock;
				((UInt16*)(block[0]))[1] = 0;
			}
			
			TotalMemAllocs++;
			TotalMemUsage += (UInt16)size;
			
			// Zero the block
			for (i = 0; i < (UInt16)(size >> 1); i++) ((UInt16*)(block + 4))[i] = 0;
			ExitCriticalSection();
			return (void*)((UInt16)block + 4);
		}
		
		lastBlock = block;
		block = (UInt16*)(block[0]);
		if ((UInt16)block & 1)
		{
			for(;;);
		}
	}
	
}

// Deallocates using an alternate heap algorithm
void zfree(void* pointer)
{
	UInt16* block = (UInt16*)((UInt16)pointer - 4);
	
	EnterCriticalSection();
	
	TotalMemAllocs--;
	TotalMemUsage -= block[0] - (UInt16)block;
	
	// Look forward
	if (!((UInt16*)block[0])[1]) block[0] = ((UInt16*)block[0])[0];
	// Look backward
	if (block[1] != (UInt16)block && !((UInt16*)block[1])[1]) ((UInt16*)block[1])[0] = block[0];
	
	// Clear the back pointer
	block[1] = 0;
	
	ExitCriticalSection();
}
