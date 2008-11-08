#ifndef _MEMORY_MANAGER_HEADER_
#define _MEMORY_MANAGER_HEADER_

#define malloc(x) zmalloc(x)
#define free(x) zfree(x)

void* zmalloc(size_t size);
void zfree(void* pointer);

#endif
