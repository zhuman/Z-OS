#include "..\Z-OS.h"
#include "Partition.h"
#include "File.h"
#include "FileSystems.h"

extern Int16 TypeFile;

/*static void FileCreate(InternalObject* obj)
{
	// Currently we have nothing to do
}

static void FileDestroy(InternalObject* obj)
{
	
}*/

static Int16 FileRead(UInt16 handle, UInt8* buffer, UInt16 bufLen)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj && obj->Data)
	{
		FileInternal* file = obj->Data;
		if (!file->IsOpened)
		{
			return ErrorUnopened;
		}
		else if (file->Position + bufLen > file->FileLength)
		{
			return ErrorInvalidSeek;
		}
		EnterCriticalSection();
		ret = file->FileSystem->Funcs.ReadFile(file,file->Position, buffer, bufLen);
		if (!ret) file->Position += bufLen;
		ExitCriticalSection();
		return ret;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

static Int16 FileGetAvailableBytes(UInt16 handle, UInt64* bytes)
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

static Int16 FileWrite(UInt16 handle, UInt8* buffer, UInt16 bufLen)
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

static Int16 FileSeek(UInt16 handle, Int64 position, SeekRelationEnum relation)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj && obj->Data)
	{
		FileInternal* file = obj->Data;
		UInt64 origPos = file->Position;
		switch (relation)
		{
			case Beginning:
				file->Position = position;
				break;
			case CurrentPos:
				file->Position += position;
				break;
			case End:
				file->Position = file->FileLength - position;
				break;
		}
		if ((file->Position < 0) || (file->Position > file->FileLength))
		{
			file->Position = origPos;
			return ErrorInvalidSeek;
		}
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

static Int16 FileFlush(UInt16 handle)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj && obj->Data)
	{
		FileInternal* file = obj->Data;
		EnterCriticalSection();
		ret = file->FileSystem->Funcs.FlushFile(file);
		ExitCriticalSection();
		return ret;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

static Int16 FileOpen(UInt16 handle, FileMode mode)
{
	return ErrorUnimplemented;
}

static Int16 FileDelete(UInt16 handle)
{
	return ErrorUnimplemented;
}

static Int16 FileRename(UInt16 handle, char* name)
{
	return ErrorUnimplemented;
}

static Int16 FileSetAttributes(UInt16 handle, FileAttributes attribs, Bool on)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj && obj->Data)
	{
		FileInternal* file = obj->Data;
		if (on)
			file->Attributes |= attribs;
		else
			file->Attributes &= ~attribs;
		
		EnterCriticalSection();
		ret = file->FileSystem->Funcs.SetFile(file);
		ExitCriticalSection();
		return ret;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

static FileAttributes FileGetAttributes(UInt16 handle)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj && obj->Data)
	{
		FileInternal* file = obj->Data;
		return file->Attributes;
	}
	else
	{
		return 0;
	}
}

static Int16 FileGetInterface(UInt16 code, void** interface)
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
	case CodeIFile:
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
	}
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
