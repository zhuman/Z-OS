#ifndef _MEMORY_MANAGER_HEADER_
#define _MEMORY_MANAGER_HEADER_

#define zmalloc(x) malloc(x)
#define zfree(x) free(x)

void HeapInit(void* heap, size_t size);
void* HeapAlloc(UInt16* heap, UInt16 heapSize, size_t allocSize);
void HeapFree(UInt16* heap, UInt16 heapSize, void* pointer);

void* mallocSafe(size_t size);
void freeSafe(void* ptr);
Bool safeCheck(void* ptr, size_t size);

#endif
