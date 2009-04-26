#include "Z-OS.h"
#include "Devices\File.h"
#include "Devices\Partition.h"
#include "Devices\FileSystems.h"
#include "Devices\VolumeManager.h"

List Devices = {0};
List FileSystems = {0};
List Partitions = {0};
List CurrentFiles = {0};

Int16 TypeDevice;
Int16 TypeFile;
Int16 TypeDirectory;
Int16 TypePartition;

// Device Objects

static void DeviceCreate(InternalObject* obj)
{
	obj->Flags |= ObjectFlagPermanent;
}

static void DeviceDestroy(InternalObject* obj)
{
	// This should never be called until we have more of a PnP system
}

static Int16 DeviceMount(UInt16 handle, UInt16 partition, char* hint, char* path)
{
	InternalObject* obj;
	Int16 ret;
	//char hintHolder[8] = {0};
	
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj && obj->Data)
	{
		PartInternal* part;
		UInt16 partHandle;
		InternalObject* partObj;
		Int16 i;
		PartitionTable* partTable;
		DeviceInternal* dev = (DeviceInternal*)(obj->Data);
		
		EnterCriticalSection();
		
		// Check if this partition has been mounted before now
		if (dev->IsMounted)
		{
			ExitCriticalSection();
			return ErrorInUse;
		}
		
		// Create a partition object for this partition
		if ((ret = CreateObject(TypePartition,&partHandle,path)))
		{
			ExitCriticalSection();
			return ret;
		}
		else
		{
			puts("Created partition object successfully.\r\n");
		}
		part = zmalloc(sizeof(PartInternal));
		if (!part)
		{
			ReleaseObject(partHandle);
			ExitCriticalSection();
			puts("Error allocating part object\r\n");
			return ErrorOutOfMemory;
		}
		part->IsLive = True;
		part->Device = dev;
		if ((ret = InternalObjectFromHandle(partHandle,&partObj)))
		{
			ReleaseObject(partHandle);
			zfree(part);
			ExitCriticalSection();
			puts("Error retrieving internal part object data\r\n");
			return ret;
		}
		
		AddListItem(&Partitions,part);
		partObj->Data = part;
		partObj->Flags |= ObjectFlagPermanent;	
		ReleaseObject(partHandle);
		
		partTable = zmalloc(sizeof(PartitionTable));
		if (!partTable)
		{
			ExitCriticalSection();
			puts("Error allocating partition table\r\n");
			return ErrorOutOfMemory;
		}
		
		// Ask the volume manager to read the partition table
		if (!LoadMBR(dev,partTable))
		{
			PartitionTableEntry* entry = EntryFromPartitionIndex(partTable,partition);
			puts("Device is partitioned!\r\n");
			
			// Create a partition object based on the partition table
			if (GetNumPartitions(partTable) > partition && entry)
			{
				part->FirstByte = (UInt64)(entry->FirstSector) * (UInt64)(dev->Info.SectorSize);
				part->ByteLength = (UInt64)(entry->NumSectors) * (UInt64)(dev->Info.SectorSize);
				zfree(partTable);
			}
			else
			{
				zfree(partTable);
				return ErrorInvalidArg;
			}
		}
		else
		{
			zfree(partTable);
			
			// Since the device is just one big partition, create a partition object to represent it all
			part->FirstByte = 0;
			ret = dev->Funcs.GetAvailableBytes(dev->DeviceId,&(part->ByteLength));
			if (ret)
			{
				ExitCriticalSection();
				return ret;
			}
		}
		
		if (hint && !strcmp("RAW",hint))
		{
			// If the user only wanted to mount the drive in raw mode, we are finished.
			ExitCriticalSection();
			return ErrorSuccess;
		}
		
		for (i = 0; i < FileSystems.Length; i++)
		{
			FileSystemInternal* fs = GetListItem(&FileSystems,i);
			
			// If a hint was set, look for that FS first
			if (fs && ((hint && !strcmp(fs->Info.Name,hint)) || !hint))
			{
				puts("Trying to detect...\r\n");
				
				// Try to detect the FS
				if (fs->Funcs.Detect(part))
				{
					printf("Detect successful: %s\r\n",fs->Info.Name);
					// Try to mount the FS
					ret = fs->Funcs.MountDevice(part);
					if (ret)
					{
						ExitCriticalSection();
						return ret;
					}
					// Yay, we mounted it
					dev->IsMounted = True;
					
					part->FileSystem = fs;
					part->IsLive = True;
					
					ExitCriticalSection();
					return ErrorSuccess;
				}
				else if (hint)
				{
					// If we were just checking the hinted FS, start over 
					// from the beginning of the list
					hint = 0;
					i = 0;
				}
			}
		}
		ExitCriticalSection();
		return ErrorUnknownFS;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

static Int16 DeviceUnmount(UInt16 handle,UInt16 partition)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		UInt16 i;
		PartInternal* part = NULL;
		
		EnterCriticalSection();
		DeviceInternal* dev = obj->Data;
		
		for (i = 0; i < Partitions.Length; i++)
		{
			part = GetListItem(&Partitions,i);
			if (part->Device == dev && part->Index == partition) break;
			else part = NULL;
		}
		if (!part) return ErrorUnmounted;
		
		if (dev->IsMounted)
		{
			// TODO: Call with the proper partition object
			ret = part->FileSystem->Funcs.UnmountDevice(part,False);
			if (ret == ErrorSuccess)
			{
				part->IsLive = False;
				part->FileSystem = Null;
			}
			// If it failed, just return to the user?
			ExitCriticalSection();
			return ret;
		}
		else
		{
			return ErrorUnmounted;
		}
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

static Bool DeviceIsMounted(UInt16 handle)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		EnterCriticalSection();
		if (((DeviceInternal*)(obj->Data))->IsMounted)
		{
			ret = True;
		}
		else
		{
			ret = False;
		}
		ExitCriticalSection();
		return ret;
	}
	else
	{
		return False;
	}
}

static Int16 DeviceSetPowerState(UInt16 handle, PowerStateEnum state)
{
	return ErrorSuccess;
}

static Int16 DeviceSendCommand(UInt16 handle, Int16 cmd, Int8* buffer, UInt16 bufferLen)
{
	return ErrorSuccess;
}

static Int16 DeviceRead(UInt16 handle, UInt8* buffer, UInt16 bufLen)
{
	InternalObject* obj;
	Int16 ret;
	puts("DeviceRead called...\r\n");
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		EnterCriticalSection();
		ret = InternalReadDevice(((DeviceInternal*)(obj->Data)),0,buffer,bufLen,True);
		ExitCriticalSection();
		return ret;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

static Int16 DeviceGetAvailableBytes(UInt16 handle, UInt64* bytes)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		EnterCriticalSection();
		if (((DeviceInternal*)(obj->Data))->Info.CanSeek)
		{
			ret = ((DeviceInternal*)(obj->Data))->Funcs.GetAvailableBytes(((DeviceInternal*)(obj->Data))->DeviceId, bytes);
			*bytes -= ((DeviceInternal*)(obj->Data))->Position;
			ExitCriticalSection();
			return ErrorSuccess;
		}
		ExitCriticalSection();
		return ErrorUnseekable;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

static Int16 DeviceWrite(UInt16 handle, UInt8* buffer, UInt16 bufLen)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		EnterCriticalSection();
		ret = InternalWriteDevice(((DeviceInternal*)(obj->Data)),0,buffer,bufLen,True);
		ExitCriticalSection();
		return ret;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

static Int16 DeviceSeek(UInt16 handle, Int64 position, SeekRelationEnum relation)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		EnterCriticalSection();
		DeviceInternal* dev = (DeviceInternal*)(obj->Data);
		if (!(dev->Info.CanSeek))
		{
			ExitCriticalSection();
			return ErrorUnseekable;
		}
		switch (relation)
		{
		case Beginning:
			dev->Position = position;
			break;
		case CurrentPos:
			dev->Position += position;
			break;
		case End:
			{
				UInt64 bytes;
				if ((ret = dev->Funcs.GetAvailableBytes(dev->DeviceId,&bytes)))
				{
					ExitCriticalSection();
					return ret;
				}
				if (bytes <= position) dev->Position += bytes - position;
				else dev->Position -= position - bytes;
				break;
			}
		}
		
		ExitCriticalSection();
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

static Int16 DeviceFlush(UInt16 handle)
{
		InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		EnterCriticalSection();
		ret = ((DeviceInternal*)(obj->Data))->Funcs.Flush(((DeviceInternal*)(obj->Data))->DeviceId);
		ExitCriticalSection();
		return ret;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

static Int16 DeviceGetInterface(UInt16 code, void** interface)
{
	switch (code)
	{
	case CodeIGeneric:
	{
		static const IGeneric inter = 
		{
			DeviceCreate,
			DeviceDestroy
		};
		*interface = (void*)&inter;
		break;
	}
	/*case CodeINamespace:
	{
		static const INamespace inter = 
		{
			DeviceCreateObject
		};
		*interface = (void*)&inter;
		break;
	}*/
	case CodeIDevice:
	{
		static const IDevice inter = 
		{
			DeviceMount,
			DeviceUnmount,
			DeviceIsMounted,
			DeviceSetPowerState,
			DeviceSendCommand
		};
		*interface = (void*)&inter;
		break;
	}
	case CodeISimpleIO:
	{
		static const ISimpleIO inter = 
		{
			DeviceRead,
			DeviceGetAvailableBytes,
			DeviceWrite,
			DeviceSeek,
			DeviceFlush
		};
		*interface = (void*)&inter;
		break;
	}
	default:
		return ErrorInvalidInterface;
	}
	return ErrorSuccess;
}

Int16 NextDeviceId = 1;

Int16 RegisterDevice(char* name, DeviceInfo info, DeviceFuncs funcs, Int16* id)
{
	DeviceInternal* device = zmalloc(sizeof(DeviceInternal));
	InternalObject* obj;
	UInt16 handle;
	Int16 ret;
	
	if (!device) return ErrorOutOfMemory;
	
	device->Info = info;
	device->Funcs = funcs;
	
	// Add the device to the internal Devices list
	EnterCriticalSection();
	AddListItem(&Devices,device);
	ExitCriticalSection();
	
	// Create the device object and set it to point to 
	// the DeviceInternal structure
	ret = CreateObject(TypeDevice,&handle,name);
	InternalObjectFromHandle(handle,&obj);
	obj->Data = device;
	ReleaseObject(handle);
	
	// Assign a unique device ID
	*id = device->DeviceId = NextDeviceId++;
	
	return ErrorSuccess;
}

Int16 GetDeviceFromID(Int16 id, DeviceInternal** device)
{
	Int16 i;
	EnterCriticalSection();
	for (i = 0; i < Devices.Length; i++)
	{
		DeviceInternal* item = GetListItem(&Devices,i);
		if (item && item->DeviceId == id)
		{
			*device = item;
			ExitCriticalSection();
			return ErrorSuccess;
		}
	}
	ExitCriticalSection();
	return ErrorNotFound;
}

Int16 InternalReadDevice(DeviceInternal* device, UInt64 pos, UInt8* buffer, UInt16 bufLen, Bool advance)
{
	Int16 ret;
	if (!device) return ErrorNullArg;
	if (device->Info.CanRead)
	{
		if (device->Funcs.Read)
		{
			ret = device->Funcs.Read(device->DeviceId, advance ? device->Position : pos, buffer, bufLen);
			if (!ret && advance) device->Position += bufLen;
		}
		else
		{
			ret = ErrorUnknown;
		}
	}
	else if (device->Info.CanWrite)
	{
		ret = ErrorWriteOnly;
	}
	else if (device->Info.CanMount)
	{
		ret = ErrorMountOnly;
	}
	else
	{
		puts("InternalReadDevice returning ErrorUnreadable\r\n");
		ret = ErrorUnreadable;
	}
	return ret;
}

Int16 InternalWriteDevice(DeviceInternal* device, UInt64 pos, UInt8* buffer, UInt16 bufLen, Bool advance)
{
	Int16 ret;
	if (!device) return ErrorNullArg;
	if (device->Info.CanWrite)
	{
		if (device->Funcs.Read)
		{
			ret = device->Funcs.Write(device->DeviceId, advance ? (device->Position) : pos, buffer, bufLen);
			if (!ret && advance) device->Position += bufLen;
		}
		else
		{
			ret = ErrorUnknown;
		}
	}
	else if (device->Info.CanRead)
	{
		ret = ErrorReadOnly;
	}
	else if (device->Info.CanMount)
	{
		ret = ErrorMountOnly;
	}
	else
	{
		ret = ErrorUnreadable;
	}
	return ret;
}

Int16 InternalWritePart(PartInternal* part, UInt64 pos, UInt8* buffer, UInt16 bufLen)
{
	if (!part || !(part->Device)) return ErrorNullArg;
	return InternalWriteDevice(part->Device,pos + part->FirstByte,buffer,bufLen,False);
}

Int16 InternalReadPart(PartInternal* part, UInt64 pos, UInt8* buffer, UInt16 bufLen)
{
	if (!part || !(part->Device)) return ErrorNullArg;
	printf("Reading %u bytes at %llu\r\n",bufLen,pos);
	return InternalReadDevice(part->Device,pos + part->FirstByte,buffer,bufLen,False);
}

Int16 RegisterFileSystem(FileSystemInfo info, FileSystemFuncs funcs)
{
	FileSystemInternal* fs = zmalloc(sizeof(FileSystemInternal));
	if (!fs) return ErrorOutOfMemory;
	
	fs->Info = info;
	fs->Funcs = funcs;
	AddListItem(&FileSystems,fs);
	
	return ErrorSuccess;
}

void InitializeIO(void)
{
	TypeRegistration device = {0};
	
	// Register the device type
	device.Type = TypeDevice = GetUniqueTypeCode();
	device.GetInterface = DeviceGetInterface;
	RegisterTypeManager(device);
	
	InitFiles();
	InitPartitions();
}
