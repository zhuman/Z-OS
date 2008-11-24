#ifndef _IO_MANAGER_HEADER_
#define _IO_MANAGER_HEADER_

typedef enum
{
	Beginning,
	CurrentPos,
	End
} SeekRelationEnum;

typedef enum
{
	On,
	Off
} PowerStateEnum;

typedef struct
{
	Int16 (*Read)(UInt16 handle, UInt8* buffer, UInt16 bufLen);
	Int16 (*GetAvailableBytes)(UInt16 handle, UInt64* bytes);
	Int16 (*Write)(UInt16 handle, UInt8* buffer, UInt16 bufLen);
	Int16 (*Seek)(UInt16 handle, Int64 position, SeekRelationEnum relation);
	Int16 (*Flush)(UInt16 handle);
} ISimpleIO;

typedef struct
{
	Int16 (*Mount)(UInt16 handle, UInt16 partition, char* hint, char* path);
	Int16 (*Unmount)(UInt16 handle, UInt16 partition);
	Bool (*IsMounted)(UInt16 handle);
	Int16 (*SetPowerState)(UInt16 handle, PowerStateEnum state);
	Int16 (*SendCommand)(UInt16 handle, Int16 cmd, Int8* buffer, UInt16 bufferLen);
} IDevice;

typedef struct
{
	Bool CanRead;
	Bool CanWrite;
	Bool CanSeek;
	Bool CanMount;
	Bool UseCacheManager;
	Bool IsSectored;
	UInt16 SectorSize;
} DeviceInfo;

typedef struct
{
	Int16 (*Read)(Int16 id, UInt64 pos, UInt8* buffer, UInt16 bufferLen);
	Int16 (*Write)(Int16 id, UInt64 pos, UInt8* buffer, UInt16 bufferLen);
	Int16 (*Command)(Int16 id, Int16 cmd, UInt8* buffer, UInt16 bufferLen);
	Int16 (*GetAvailableBytes)(Int16 id, UInt64* bytes);
	Int16 (*Flush)(Int16 id);
} DeviceFuncs;

typedef struct
{
	Int16 DeviceId; // Unique device ID
	UInt16 Partition;
	DeviceInfo Info; // Information about the device
	DeviceFuncs Funcs; // Functions implemented by the driver
	Bool IsMounted;
	struct t_FileSystemInternal* MountedFS; // The file system used to mount it
	UInt64 Position; // The current seek position
} DeviceInternal;

// IO Manager API
void InitializeIO(void);
Int16 RegisterDevice(char* name, DeviceInfo info, DeviceFuncs funcs, Int16* id);

// IO Manager Internal API for device drivers
Int16 InternalReadDevice(DeviceInternal* device, UInt64 pos, UInt8* buffer, UInt16 bufLen, Bool advance);
Int16 InternalWriteDevice(DeviceInternal* device, UInt64 pos, UInt8* buffer, UInt16 bufLen, Bool advance);
Int16 GetDeviceFromID(Int16 id, DeviceInternal** device);

// Device Drivers included in the OS
void InitSerialPorts(void);

#endif
