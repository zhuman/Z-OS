#include "Z-OS.h"

//typedef void (InterruptHook*)(void);

List IntProgramHooks = {0};
List IntCNHooks = {0};

extern Int16 CurrentCritNesting;

inline void InterruptEnter(void)
{
	CurrentCritNesting++;
}

inline void InterruptExit(void)
{
	CurrentCritNesting--;
}
