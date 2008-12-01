/////////////////////////////////////////////////////
// Z-OS Header
// Including this file includes all public Z-OS APIs.
/////////////////////////////////////////////////////

// See the individual files and the documentation for more
// information on the API functions and their usage.

// First, some useful functions

__inline static int Max(int x, int y)
{
	if ( x > y) return x;
	return y;
}

__inline static int Min(int x, int y)
{
	if ( x < y) return x;
	return y;
}

__inline static int Abs(int x)
{
	if ( x > 0) return x;
	return -x;
}

__inline static int Sgn(int x)
{
	if ( x < 0) return -1;
	if (x == 0) return 0;
	return 1;
}

// The PIC24H header
#include "p24Hxxxx.h"

// The Standard C Libraries
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// Z-OS Config
#include "Config.h"

// Z-OS APIs
#include "Types.h"
#include "Errors.h"
#include "Init.h"
#include "MemoryManager.h"
#include "Lists.h"
#include "ObjectManager.h"
#include "ThreadManager.h"
#include "Sync.h"
#include "Timer.h"
#include "Queue.h"
#include "Delegate.h"
#include "IO.h"
#include "Devices\Partition.h"
#include "Devices\File.h"
#include "Devices\VirtualFile.h"
#include "CacheManager.h"
