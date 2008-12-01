#include <stdlib.h>
#include "Types.h"

#ifndef _OBJECT_MANAGER_HEADER_
#define _OBJECT_MANAGER_HEADER_

// This struct is the internal representation
// of all objects.
typedef struct
{
	UInt16 Type;
	UInt16 DataLength; // For use by type manager
	UInt16 Flags; // Reserved for system use
	UInt16 OpenHandles; // Reserved for system use
	UInt16 KernelReferences; // Reserved for system use or for type managers
	char* Name;
	void* Data; // For use by type manager
} InternalObject;

// This struct is used to store type registrations.
typedef struct
{
	UInt16 Type;
	Int16 (*GetInterface)(UInt16 code, void** interface);
} TypeRegistration;

// Implemented to allow for namespaces 
// within object manager's namespace.
typedef struct
{
	// Opens an existing object or creates a new one.
	Int16 (*CreateObject)(UInt16 handle, UInt16* newHandle, char* path, char** reparse);
} INamespace;

// This code can be used as the basis for type manager functions
/*
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidHandle;
	}
*/

// This interface can be implemented
// by any type manager to setup any data
// and destroy/unalloc it on destruction.
// It is also the only interface in which
// the values do not have to be set since
// they are checked by the system. However,
// not setting a function is bad practice
// and is completely incorrect when implementing
// other interfaces. If a valid implementation
// for a function does not exist for an object,
// point the function pointer to a dummy function.
typedef struct
{
	void (*Create)(InternalObject*);
	void (*Destroy)(InternalObject*);
} IGeneric;

typedef enum
{
	CodeIGeneric,
// Define other standard system interfaces
// with codes from 1 to 0xFF
	CodeINamespace,
	CodeIWaitable,
	CodeIThread,
	CodeITimer,
	CodeISemaphore,
	CodeIEvent,
	CodeIQueue,
	CodeIDelegate,
	CodeIDevice,
	CodeISimpleIO,
	CodeIFile,
	CodeIDirectory
} InterfaceCodeEnum;

// Non-standard interfaces should have codes
// of 0x100 or above.

// Define standard system types here:
typedef enum
{
	TypeThread,
	TypeTimer,
	TypeSemaphore,
	TypeEvent,
	TypeQueue,
	TypeDelegate,
} TypeCodeEnum;

// Non-standard types are generated starting at 0x100

// Object Flags
#define ObjectFlagPermanent	0x1 // The object will not be deleted when its last handle closes, allowing it to be reopened.
#define ObjectFlagAutoFree	0x2 // The object's Data field will be freed by the object manager automatically.
#define ObjectFlagNoShare	0x4 // Only one handle can be open at a time for this object.
#define ObjectFlagSymLink	0xFFFF

// Public API

// Returns an interface of the specified type code from an object handle.
Int16 GetInterface(UInt16 handle, UInt16 code, void** interface);
// Create an object with the specififed name and type.
Int16 CreateObject(UInt16 type, UInt16* handle, char* name);
// Opens the object with the specified name and returns a new handle.
Int16 OpenObject(char* name, UInt16* handle);
// Releases a handle to an object and destroys it if it is no longer in use.
Int16 ReleaseObject(UInt16 handle);
// Returns true or false as to whether the number is a valid handle.
Int16 IsHandleValid(UInt16 handle);

// Type Manager API

// Registers a new type manager with the system.
Int16 RegisterTypeManager(TypeRegistration typeManager);
// Returns a unique type code.
Int16 GetUniqueTypeCode(void);
// Returns the internal object structure from an object handle for use by type managers.
UInt16 InternalObjectFromHandle(UInt16 handle, InternalObject** obj);
// Retrieves an InternalObject struct from a given data field
Int16 InternalObjectFromData(void* data,InternalObject** obj);

// Symbolic Link API

// Creates a symbolic link with the given name that points to the real object given
Int16 CreateSymbolicLink(char* name, InternalObject* pointedObj);
// Changes the object with the given structure to point to a different object
Int16 ChangeSymbolicLink(char* name, InternalObject* newObj);
Int16 InternalObjectFromName(char* name, InternalObject** obj);

#endif
