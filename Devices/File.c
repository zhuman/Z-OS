#include "..\Z-OS.h"
#include "Partition.h"
#include "File.h"
#include "FileSystems.h"

extern Int16 TypeFile;

void FileCreate(InternalObject* obj)
{
	// Currently we have nothing to do
}

void FileDestroy(InternalObject* obj)
{
	
}

Int16 FileRead(UInt16 handle, UInt8* buffer, UInt16 bufLen)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj && obj->Data)
	{
		EnterCriticalSection();
		if (!((FileInternal*)(obj->Data))->IsOpened)
		{
			ExitCriticalSection();
			return ErrorUnopened;
		}
		ret = ((FileInternal*)(obj->Data))->FileSystem->Funcs.ReadFile(((FileInternal*)(obj->Data)),((FileInternal*)(obj->Data))->Position, buffer, bufLen);
		if (!ret) ((FileInternal*)(obj->Data))->Position += bufLen;
		ExitCriticalSection();
		return ret;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

Int16 FileGetAvailableBytes(UInt16 handle, UInt64* bytes)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj && obj->Data)
	{
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

Int16 FileWrite(UInt16 handle, UInt8* buffer, UInt16 bufLen)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj && obj->Data)
	{
		EnterCriticalSection();
		if (!((FileInternal*)(obj->Data))->IsOpened)
		{
			ExitCriticalSection();
			return ErrorUnopened;
		}
		ret = ((FileInternal*)(obj->Data))->FileSystem->Funcs.WriteFile(((FileInternal*)(obj->Data)),((FileInternal*)(obj->Data))->Position, buffer, bufLen);
		if (!ret) ((FileInternal*)(obj->Data))->Position += bufLen;
		ExitCriticalSection();
		return ret;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

Int16 FileSeek(UInt16 handle, Int64 position, SeekRelationEnum relation)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj && obj->Data)
	{
		UInt64 origPos = ((FileInternal*)(obj->Data))->Position;
		switch (relation)
		{
			case Beginning:
				((FileInternal*)(obj->Data))->Position = position;
				break;
			case CurrentPos:
				((FileInternal*)(obj->Data))->Position += position;
				break;
			case End:
				((FileInternal*)(obj->Data))->Position = ((FileInternal*)(obj->Data))->FileLength - position;
				break;
		}
		if (((FileInternal*)(obj->Data))->Position < 0 || ((FileInternal*)(obj->Data))->Position > ((FileInternal*)(obj->Data))->FileLength)
		{
			((FileInternal*)(obj->Data))->Position = origPos;
			return ErrorInvalidSeek;
		}
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

Int16 FileFlush(UInt16 handle)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj && obj->Data)
	{
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

Int16 FileGetInterface(UInt16 code, void** interface)
{
	switch (code)
	{
	case CodeISimpleIO:
	{
		static const ISimpleIO inter = 
		{
			FileRead,
			FileGetAvailableBytes,
			FileWrite,
			FileSeek,
			FileFlush
		};
		*interface = (void*)&inter;
		break;
	}
	/*case CodeIFile:
	{
		static const IFile inter = 
		{
			FileOpen,
			FileDelete,
			FileRename,
			FileSetAttributes,
			FileGetAttributes
		};	
		*interface = (void*)&inter;
		break;
	}*/
	/*case CodeIDirectory:
	{
		static const IDirectory inter = 
		{
			FileEnumObjects,
			FileEnumDirectories,
			FileRename,
			FileDelete
		};
		*interface = (void*)&inter;
		break;
	}*/
	default:
		return ErrorInvalidInterface;
	}
	return ErrorSuccess;
}

void InitFiles(void)
{
	TypeRegistration file = {0};
	
	// Register the file type
	file.Type = TypeFile = GetUniqueTypeCode();
	file.GetInterface = FileGetInterface;
	RegisterTypeManager(file);
}
