#include "Z-OS.h"
#include <stdlib.h>
#include <string.h>

Int16 TotalMemAllocs = 0;
Int64 TotalMemUsage = 0;

#define Word UInt16
#define WordSize sizeof(Word)

#define HeapSize 5000

__attribute__ ((__far__)) Word Heap[HeapSize] = {0};
size_t FreeWords = HeapSize;

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
	memset(Heap,0,HeapSize * sizeof(UInt16));
}

// Allocates using an alternate heap algorithm
void* malloc(size_t size)
{
	Word *block = Heap;
	size_t wsize;
	Word backref = -1;
	Word forwardref = 0;
	
	// block represents a list node
	// block[0] is an offset to the previous node
	// block[1] is an offset to the next node
	
	// Check for zero size
	if(!size)
		return NULL;
	
	// Align size to a word boundary
	if(size & (WordSize - 1))
		size = (size | (WordSize - 1)) + 1;
	
	wsize = size / WordSize + 2;
	
	EnterCriticalSection();
	
	// Quick free space check
	if(wsize > FreeWords)
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
			// If this is the last block, the forward reference will be null, and the block's size is just the remaining heap
			// length. Otherwise, the size of this block is the value of the forward reference.
			size_t bsize = block[1] ? block[1] : HeapSize - (block - Heap);
			
			// Is free region large enough?
			if(bsize >= wsize)
			{
				// Allocate the block by setting the back and forward references to their new values
				block[0] = backref;
				
				// Since a valid block must be at least 3 words long, if the size of the free fragment left after this allocation
				// would be less than 3, just allocate the remainder of the block to avoid unallocatable free fragments.
				if(bsize - wsize >= 3)
				{
					Word *freefragment = block + wsize;
					// Insert node for the remainder of the free space
					freefragment[0] = 0;
					if(block[1])
						// Not the last block, inserted free fragment must have a forward reference and the proceeding block must
						// be updated to point back to the new free fragment as well.
						freefragment[1] = *(block + bsize) = bsize - wsize;
					else
						freefragment[1] = 0;
					
					block[1] = wsize;
					
					// Update free word count
					FreeWords -= wsize;
				}
				// Else, the forward reference stays the same
				else
					FreeWords -= bsize;
				
				// Zero the actual memory (depending on this in an allocator is bad practice, however)
				memset(&block[2], 0, (wsize - 2) * WordSize);
				
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

// Deallocates using an alternate heap algorithm
void free(void* pointer)
{
	Word *block = (Word *) pointer;  // This cast is not required in C, but C++ is retarded.
	block -= 2;
	
	EnterCriticalSection();
	
	// Update free word count
	FreeWords += block[1] ? block[1] : HeapSize - (block - Heap);
	
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
		
	ExitCriticalSection();
}
