#include "Z-OS.h"

// This is the object manager's internal
// list of objects.
List CurrentObjects = {0};

// This is the internal list of registered
// type managers.
List TypeManagers = {0};

// This list provides the outside world
// (not really, but close) with handle lookup.
List OpenHandles = {0};

// This method (for use only within the object manager) returns an
// interface for the object with the specified index (not handle).
// This is used to call the IGeneric.Destroy function since the handle
// has already been destroyed by that point.
Int16 GetInterfaceInternal(UInt16 index, UInt16 code, void** interface)
{
	UInt16 i = 0;
	InternalObject* obj;
	TypeRegistration* typeManager;
	Int16 ret = ErrorSuccess;
	
	EnterCriticalSection();
	obj = GetListItem(&CurrentObjects,index);
	ExitCriticalSection();
	
	if (!obj) return ErrorInvalidHandle;
	
	EnterCriticalSection();
	for (i = 0; i < TypeManagers.Length; i++)
	{
		if ((typeManager = ((TypeRegistration*)GetListItem(&TypeManagers,i)))->Type == obj->Type)
		{
			ret = typeManager->GetInterface(code,interface);
			ExitCriticalSection();
			return ret;
		}
	}
	
	ExitCriticalSection();
	return ErrorInvalidType;
}

// Returns an interface of the specified code for the
// object with the specified handle.
Int16 GetInterface(UInt16 handle, UInt16 code, void** interface)
{
	Int16 ret = 0;
	EnterCriticalSection();
	ret = GetInterfaceInternal(((UInt16)GetListItem(&OpenHandles,handle-1)),code,interface);
	ExitCriticalSection();
	return ret;
}

// Registers a type manager with the specified functions.
Int16 RegisterTypeManager(TypeRegistration typeManager)
{
	TypeRegistration* reg = zmalloc(sizeof(TypeRegistration));
	if (!reg) return ErrorOutOfMemory;
	*reg = typeManager;
	AddListItem(&TypeManagers,reg);
	return ErrorSuccess;
}

// Creates an object of the specified type,
// opens a handle for it and returns the handle.
Int16 CreateObject(UInt16 type, UInt16* handle, char* name)
{
	InternalObject* obj = zmalloc(sizeof(InternalObject));
	UInt16 index = 0;
	UInt16 i;
	Bool hasReplaced = False;
	IGeneric* gen;
	Int16 ret = 0;
	
	if (!obj) return ErrorOutOfMemory;
	
	// Assign the object name if wanted
	if (name)
	{
		if (OpenObject(name, handle) == ErrorSuccess)
		{
			ReleaseObject(*handle);
			zfree(obj);
			return ErrorNameInUse;
		}
		obj->Name = zmalloc(strlen(name)+1);
		if (!(obj->Name)) return ErrorOutOfMemory;
		strcpy(obj->Name, name);
	}
	
	EnterCriticalSection();
	
	for (i = 0; i < CurrentObjects.Length; i++)
	{
		if (!GetListItem(&CurrentObjects,i))
		{
			ReplaceListItem(&CurrentObjects,i,obj);
			index = i;
			hasReplaced = True;
			break;
		}		
	}
	if (!hasReplaced)
	{
		index = CurrentObjects.Length;
		AddListItem(&CurrentObjects,obj);
	}
	
	AddListItem(&OpenHandles,(void*)index);
	obj->Type = type;
	
	if (!(ret = GetInterface(OpenHandles.Length, CodeIGeneric, (void**)&gen)) && (gen->Create)) gen->Create(obj);
	
	*handle = OpenHandles.Length;
	
	obj->OpenHandles++;
	
	ExitCriticalSection();
	
	return ErrorSuccess;
}

// Returns if an object has any open handles.
Int16 IsObjectOpen(UInt16 index)
{
	UInt16 i = 0;
	EnterCriticalSection();
	for (i = 0; i < OpenHandles.Length; i++)
	{
		if ((Int16)GetListItem(&OpenHandles,i) == index) { ExitCriticalSection(); return TRUE; }
	}
	ExitCriticalSection();
	return FALSE;
}

// Returns a handle to an object with the specified name.
Int16 OpenObject(char* name, UInt16* handle)
{
	UInt16 i = 0;
	UInt16 index;
	Int16 firstAvailInd;
	EnterCriticalSection();
	for (i = 0; i < CurrentObjects.Length; i++)
	{
		InternalObject* obj = (InternalObject*)GetListItem(&CurrentObjects,i);
		if (obj->Name && !strcmp(name,obj->Name))
		{
			// Follow any symbolic links
			while (obj->Flags == ObjectFlagSymLink) obj = obj->Data;
			
			if ((obj->Flags & ObjectFlagNoShare) && IsObjectOpen(i))
			{
				*handle = 0;
				ExitCriticalSection();
				return ErrorInUse;
			}
			index = GetIndexOf(&CurrentObjects,obj);
			if ((firstAvailInd = GetIndexOf(&OpenHandles,(void*)0xFFFF)) == -1)
			{
				AddListItem(&OpenHandles,(void*)index);
				*handle = OpenHandles.Length;
			}
			else
			{
				ReplaceListItem(&OpenHandles,firstAvailInd,(void*)index);
				*handle = firstAvailInd + 1;
			}
			obj->OpenHandles++;
			ExitCriticalSection();
			return ErrorSuccess;
		}
		else if (strstr(name,obj->Name) == name && name[strlen(obj->Name)] == '\\')
		{
			INamespace* namespace;
			Int16 ret;
			if (GetInterfaceInternal(i,CodeINamespace,(void**)(&namespace)) == ErrorSuccess)
			{
				char truncName[0x50] = {0};
				char* reparseName;
				UInt16 namespaceHandle;
				
				strcpy(truncName,obj->Name);
				if ((ret = OpenObject(obj->Name,&namespaceHandle)) != ErrorSuccess)
				{
					ExitCriticalSection();
					return ret;
				}
				ret = namespace->CreateObject(handle,truncName,&reparseName);
				// If a reparse point was hit, reparseName contains the new location
				if (ret == ErrorReparse)
				{
					// And begin again...
					ret = OpenObject(reparseName,handle);
					zfree(reparseName);
				}
				ExitCriticalSection();
				return ret;
			}
		}
	}
	
	ExitCriticalSection();
	return ErrorNotFound;
}

Int16 DuplicateHandle(UInt16 handle, UInt16* newHandle)
{
	UInt16 index;
	EnterCriticalSection();
	index = (UInt16)GetListItem(&OpenHandles, handle - 1);
	AddListItem(&OpenHandles,(void*)index);
	*newHandle = OpenHandles.Length;
	ExitCriticalSection();
	return ErrorSuccess;
}

// Releases an object and destroys it if needed.
Int16 ReleaseObject(UInt16 handle)
{
	UInt16 index;
	InternalObject* obj = NULL;
	IGeneric* gen;
	
	EnterCriticalSection();
	
	index = (UInt16)GetListItem(&OpenHandles,handle-1);
	if (handle == OpenHandles.Length)
	{
		RemoveListItem(&OpenHandles,handle-1);
	}
	else
	{
		ReplaceListItem(&OpenHandles,handle-1,(void*)0xFFFF);
	}
	obj = GetListItem(&CurrentObjects,index);
	obj->OpenHandles--;
	
	if (obj->OpenHandles || obj->KernelReferences)
	{
		ExitCriticalSection();
		return ErrorSuccess;
	}
	// TODO: Remove this:
	ExitCriticalSection();
	return ErrorSuccess;
	
	// If we have gotten here, the object is no longer
	// in use (aka, no open handles exist for it).
	if (!obj)
	{
		ExitCriticalSection();
		return ErrorInvalidHandle;
	}
	if (!(obj->Flags | ObjectFlagPermanent)) // Objects can be made permanent
	{
		if (!GetInterfaceInternal(index,CodeIGeneric,(void**)&gen))
		{
			gen->Destroy(obj);
		}
		if ((obj->Flags | ObjectFlagAutoFree) && obj->Data) zfree(obj->Data); // Objects can be set to be freed by us
		if (obj->Name) zfree(obj->Name);
		if (index == CurrentObjects.Length - 1)
		{
			zfree(RemoveListItem(&CurrentObjects,index));
		}
		else
		{
			zfree(ReplaceListItem(&CurrentObjects,index,(void*)0));
		}
	}
	ExitCriticalSection();
	return ErrorSuccess;
}

// Returns the internal object data structure from a handle.
UInt16 InternalObjectFromHandle(UInt16 handle, InternalObject** obj)
{
	EnterCriticalSection();
	*obj = GetListItem(&CurrentObjects,((UInt16)GetListItem(&OpenHandles,handle-1)));
	ExitCriticalSection();
	if (!(*obj)) return ErrorUnknown;
	return ErrorSuccess;
}

// Returns true or false.
Int16 IsHandleValid(UInt16 handle)
{
	UInt16 length;
	EnterCriticalSection();
	length = OpenHandles.Length;
	ExitCriticalSection();
	if (!handle || handle > length) return False;
	return True;
}

void zstrcpy(char* dest, char* src)
{
	Int8 i = 0;
	while ((dest[i] = src[i])) i++;
}

Int16 CreateSymbolicLink(char* name, InternalObject* pointedObj)
{
	InternalObject* obj = zmalloc(sizeof(InternalObject));
	
	if (!obj) return ErrorOutOfMemory;
	
	// Assign the object name
	if (name)
	{
		obj->Name = zmalloc(strlen(name)+1);
		if (!(obj->Name))
		{
			zfree(obj);
			return ErrorOutOfMemory;
		}
		strcpy(obj->Name, name);
	}
	else
	{
		zfree(obj);
		return ErrorNullArg;
	}
	
	obj->Flags = ObjectFlagSymLink;
	obj->Data = pointedObj;
	
	EnterCriticalSection();
	
	if (pointedObj->Flags != ObjectFlagSymLink) pointedObj->KernelReferences++;
	AddListItem(&CurrentObjects,obj);
	
	ExitCriticalSection();
	
	return ErrorSuccess;
}

Int16 ChangeSymbolicLink(char* name, InternalObject* newObj)
{
	Int16 ret;
	InternalObject* symObj;
	if ((ret = InternalObjectFromName(name, &symObj))) return ret;
	
	EnterCriticalSection();
	
	if (((InternalObject*)(symObj->Data))->Flags != ObjectFlagSymLink) ((InternalObject*)(symObj->Data))->KernelReferences--;
	symObj->Data = newObj;
	if (newObj->Flags != ObjectFlagSymLink) newObj->KernelReferences++;
	
	ExitCriticalSection();
	
	return ErrorSuccess;
}

Int16 InternalObjectFromName(char* name, InternalObject** obj)
{
	int i;
	InternalObject* currObj;
	EnterCriticalSection();
	for (i = 0; i < CurrentObjects.Length; i++)
	{
		currObj = GetListItem(&CurrentObjects,i);
		if (currObj->Name && !strcmp(currObj->Name,name))
		{
			*obj = currObj;
			ExitCriticalSection();
			return ErrorSuccess;
		}
	}
	ExitCriticalSection();
	return ErrorInvalidObject;
}

Int16 InternalObjectFromData(void* data,InternalObject** obj)
{
	int i;
	InternalObject* currObj;
	EnterCriticalSection();
	for (i = 0; i < CurrentObjects.Length; i++)
	{
		currObj = GetListItem(&CurrentObjects,i);
		if (currObj->Data == data)
		{
			*obj = currObj;
			ExitCriticalSection();
			return ErrorSuccess;
		}
	}
	ExitCriticalSection();
	return ErrorInvalidObject;
}

Int16 NextAvailableTypeCode = 0x100;

Int16 GetUniqueTypeCode(void)
{
	Int16 nextAvailable;
	EnterCriticalSection();
	nextAvailable = NextAvailableTypeCode++;
	ExitCriticalSection();
	return nextAvailable;
}
