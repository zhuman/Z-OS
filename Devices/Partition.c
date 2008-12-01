#include "..\Z-OS.h"
#include "Partition.h"
#include "File.h"
#include "FileSystems.h"

extern Int16 TypeFile;
extern Int16 TypePartition;

// Partition objects are only created when Mount is called on a device with a partition number.

Int16 PartCreateObject(UInt16 handle, UInt16* newHandle, char* path, char** reparse)
{
	InternalObject* obj;
	Int16 ret = ErrorSuccess;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj && (obj->Data))
	{
		PartInternal* part = (PartInternal*)(obj->Data);
		
		EnterCriticalSection();
		if (part->IsLive && part->Device->IsMounted)
		{
			InternalObject* fileObj;
			FileInternal* file = zmalloc(sizeof(FileInternal));
			if (!file)
			{
				ExitCriticalSection();
				return ErrorOutOfMemory;
			}
			
			// Set up a file structure
			file->Partition = part;
			file->Name = zmalloc(strlen(path) + 1);
			if (!file->Name)
			{
				zfree(file);
				ExitCriticalSection();
				return ErrorOutOfMemory;
			}
			strcpy(file->Name, path);
			
			// TODO: Ask the FS driver for file information
			if ((ret = part->FileSystem->Funcs.GetFile(file,reparse)))
			{
				ExitCriticalSection();
				zfree(file);
				return ret;
			}
			
			puts("Creating file object\r\n");
			
			// Create the actual file object
			if ((ret = CreateObject(TypeFile, newHandle, (char*)0)))
			{
				ExitCriticalSection();
				zfree(file);
				return ret;
			}
			if ((ret = InternalObjectFromHandle(*newHandle,&fileObj)))
			{
				ExitCriticalSection();
				zfree(file);
				return ret;
			}
			fileObj->Data = file;
		}
		else
		{
			ret = ErrorUnmounted;
		}
		ExitCriticalSection();
		return ret;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

Int16 PartRead(UInt16 handle, UInt8* buffer, UInt16 bufLen)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		EnterCriticalSection();
		if (((PartInternal*)(obj->Data))->IsLive && ((PartInternal*)(obj->Data))->Device)
		{
			ret = InternalReadDevice(((PartInternal*)(obj->Data))->Device,((PartInternal*)(obj->Data))->Position + ((PartInternal*)(obj->Data))->FirstByte, buffer,bufLen, False);
			if (!ret) ((PartInternal*)(obj->Data))->Position += bufLen;
			ExitCriticalSection();
			return ret;
		}
		ExitCriticalSection();
		return ErrorUnmounted;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

Int16 PartGetAvailableBytes(UInt16 handle, UInt64* bytes)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		EnterCriticalSection();
		if (((PartInternal*)(obj->Data))->IsLive)
		{
			*bytes = ((PartInternal*)(obj->Data))->ByteLength - ((PartInternal*)(obj->Data))->Position;
			ExitCriticalSection();
			return ret;
		}
		ExitCriticalSection();
		return ErrorUnmounted;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

Int16 PartWrite(UInt16 handle, UInt8* buffer, UInt16 bufLen)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		EnterCriticalSection();
		if (((PartInternal*)(obj->Data))->IsLive)
		{
			ret = InternalWriteDevice(((PartInternal*)(obj->Data))->Device,((PartInternal*)(obj->Data))->Position + ((PartInternal*)(obj->Data))->FirstByte, buffer,bufLen, False);
			if (!ret) ((PartInternal*)(obj->Data))->Position += bufLen;
			ExitCriticalSection();
			return ret;
		}
		ExitCriticalSection();
		return ErrorUnmounted;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

Int16 PartSeek(UInt16 handle, Int64 position, SeekRelationEnum relation)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		EnterCriticalSection();
		if (((PartInternal*)(obj->Data))->IsLive)
		{
			UInt64 oldPos = ((PartInternal*)(obj->Data))->Position;
			switch (relation)
			{
			case Beginning:
				((PartInternal*)(obj->Data))->Position = position;
			break;
			case CurrentPos:
				((PartInternal*)(obj->Data))->Position += position;
			break;
			case End:
				((PartInternal*)(obj->Data))->Position = ((PartInternal*)(obj->Data))->ByteLength - position;
			break;
			}
			if (((PartInternal*)(obj->Data))->Position < 0 || ((PartInternal*)(obj->Data))->Position >= ((PartInternal*)(obj->Data))->ByteLength)
			{
				((PartInternal*)(obj->Data))->Position = oldPos;
				ret = ErrorInvalidSeek;
			}
			ExitCriticalSection();
			return ret;
		}
		ExitCriticalSection();
		return ErrorUnmounted;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

Int16 PartFlush(UInt16 handle)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		EnterCriticalSection();
		if (((PartInternal*)(obj->Data))->IsLive)
		{
			ret = ((PartInternal*)(obj->Data))->Device->Funcs.Flush(((PartInternal*)(obj->Data))->Device->DeviceId);
			ExitCriticalSection();
			return ret;
		}
		ExitCriticalSection();
		return ErrorUnmounted;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

Int16 PartGetInterface(UInt16 code, void** interface)
{
	switch (code)
	{
	/*case CodeIGeneric:
	{
		static const IGeneric inter = 
		{
			PartCreate,
			PartDestroy
		};
		*interface = (void*)&inter;
		break;
	}*/
	case CodeINamespace:
	{
		static const INamespace inter = 
		{
			PartCreateObject
		};
		*interface = (void*)&inter;
		break;
	}
	case CodeISimpleIO:
	{
		static const ISimpleIO inter = 
		{
			PartRead,
			PartGetAvailableBytes,
			PartWrite,
			PartSeek,
			PartFlush
		};
		*interface = (void*)&inter;
		break;
	}
	default:
		return ErrorInvalidInterface;
	}
	return ErrorSuccess;
}

void InitPartitions(void)
{
	TypeRegistration part = {0};
	
	// Register the mount point/partition type
	part.Type = TypePartition = GetUniqueTypeCode();
	part.GetInterface = PartGetInterface;
	RegisterTypeManager(part);
}
