#include "Z-OS.h"
#include <stdlib.h>
#include <string.h>

Int16 TotalMemAllocs = 0;
Int64 TotalMemUsage = 0;

#define Word UInt16
#define WordSize sizeof(Word)

#define HeapSize 5000

__attribute__ ((__far__)) Word MainHeap[HeapSize] = {0};
size_t FreeWords = HeapSize;

void HeapInit(void* heap, size_t size)
{
	memset(MainHeap,0,size);
}

void* HeapAlloc(Word* heap, UInt16 heapSize, size_t allocSize)
{
	Word *block = heap;
	size_t wallocSize;
	Word backref = -1;
	Word forwardref = 0;
	
	// block represents a list node
	// block[0] is an offset to the previous node
	// block[1] is an offset to the next node
	
	// Check for zero allocSize
	if(!allocSize)
		return NULL;
	
	// Align allocSize to a word boundary
	if(allocSize & (WordSize - 1))
		allocSize = (allocSize | (WordSize - 1)) + 1;
	
	wallocSize = allocSize / WordSize + 2;
	
	EnterCriticalSection();
	
	// Quick free space check
	if(wallocSize > FreeWords)
	{
		ExitCriticalSection();
		return NULL;
	}
	
	// Iterate through the list
	do
	{
		// Move to the next block
		block += forwardref;
		
		// Back references are only needed to recombine blocks during freeing. So, free regions don't need
		// a back reference. Therefore, a block will be marked as free if its back reference is null.
		if(!block[0])
		{
			// If this is the last block, the forward reference will be null, and the block's allocSize is just the remaining heap
			// length. Otherwise, the allocSize of this block is the value of the forward reference.
			size_t ballocSize = block[1] ? block[1] : heapSize - (block - heap);
			
			// Is free region large enough?
			if(ballocSize >= wallocSize)
			{
				// Allocate the block by setting the back and forward references to their new values
				block[0] = backref;
				
				// Since a valid block must be at least 3 words long, if the allocSize of the free fragment left after this allocation
				// would be less than 3, just allocate the remainder of the block to avoid unallocatable free fragments.
				if(ballocSize - wallocSize >= 3)
				{
					Word *freefragment = block + wallocSize;
					// Insert node for the remainder of the free space
					freefragment[0] = 0;
					if(block[1])
						// Not the last block, inserted free fragment must have a forward reference and the proceeding block must
						// be updated to point back to the new free fragment as well.
						freefragment[1] = *(block + ballocSize) = ballocSize - wallocSize;
					else
						freefragment[1] = 0;
					
					block[1] = wallocSize;
					
					// Update free word count
					FreeWords -= wallocSize;
				}
				// Else, the forward reference stays the same
				else
					FreeWords -= ballocSize;
				
				// Zero the actual memory (depending on this in an allocator is bad practice, however)
				memset(&block[2], 0, (wallocSize - 2) * WordSize);
				
				TotalMemAllocs++;
				TotalMemUsage += wallocSize * WordSize;
				
				ExitCriticalSection();
				return &block[2];
			}
		}
		
		// Prepare to move to the next block in the next loop iteration
		backref = forwardref = block[1];
		
	} while(forwardref);
	
	// No contiguous regions of free space were large enough to contain the allocation
	ExitCriticalSection();
	return NULL;
}

void HeapFree(Word* heap, UInt16 heapSize, void* pointer)
{
	Word *block = (Word*) pointer;  // This cast is not required in C, but C++ is retarded.
	block -= 2;
	
	EnterCriticalSection();
	
	// Update free word count
	FreeWords += block[1] ? block[1] : heapSize - (block - heap);
	TotalMemUsage = (heapSize - FreeWords) * WordSize;
	
	// Recombine a proceeding free region
	if(block[1] && !((block + block[1])[0]))
	{
		Word *nextblock = block + block[1];
		// Test if the proceeding free region is the last block
		if(nextblock[1])
		{
			// Since we're recombining with the proceeding free block, the block after next, which was pointing to the block
			// to be recombined, needs to have its back reference updated (we're removing the recombined node)
			*(nextblock + nextblock[1]) += block[1];
			block[1] += nextblock[1];
		}
		else
			block[1] = 0;
	}
	
	// Recombine a preceeding free region (a value of -1 for the back reference indicates that the block is allocated and that it
	// is the first block)
	if(block[0] != -1 && !((block - block[0])[0]))
	{
		Word *prevblock = block - block[0];
		if(block[1])
		{
			// This time, the proceeding block needs to have its back reference updated (we're removing the node being freed)
			*(block + block[1]) += prevblock[1];
			prevblock[1] += block[1];
		}
		else
			prevblock[1] = 0;
	}
	
	// Free the block
	else
		block[0] = 0;
		
	TotalMemAllocs--;
		
	ExitCriticalSection();
}

// Reimplementation of standard C malloc and free

void* malloc(size_t size)
{
	return HeapAlloc(MainHeap, HeapSize * sizeof(UInt16), size);
}

void free(void* pointer)
{
	HeapFree(MainHeap, HeapSize * sizeof(UInt16), pointer);
}

// Some safe-buffer routines for the IO manager and other debug purposes

void* mallocSafe(size_t size)
{
	UInt16* ptr;
	
	if (size & 1) size++;
	
	ptr = malloc(size + 4);
	ptr[0] = (UInt16)ptr;
	ptr[(size >> 1) + 1] = ~(UInt16)ptr;
	
	return ptr + 1;
}

// Frees a buffer allocated using mallocSafe
void freeSafe(void* ptr)
{
	free((UInt16*)ptr - 1);
}

// Checks a buffer for possible corruption
//  true - check succeeded
//  false - check failed, corruption likely
Bool safeCheck(void* ptr, size_t size)
{
	UInt16* p = ptr;
	if ((*(p - 1) != (UInt16)(p - 1)) || (*(p + size) != ~(UInt16)(p - 1)))
	{
		return false;
	}
	return true;
}
