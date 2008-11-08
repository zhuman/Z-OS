#ifndef _TYPES_HEADER_
#define _TYPES_HEADER_

// Integer Types
typedef unsigned int UInt16;
typedef signed int Int16;
typedef unsigned char UInt8;
typedef signed char Int8;
typedef unsigned long int UInt32;
typedef signed long int Int32;
typedef unsigned long long int UInt64;
typedef signed long long int Int64;

// Boolean Types
#define False	0
#define True	(!False)
#define false	False
#define true	True
#define FALSE	False
#define TRUE	True
typedef unsigned int Bool;

// Null
#define null	((void*)0)
#ifdef NULL
	#undef NULL
#endif
#define NULL	null
#define Null	null

#endif
