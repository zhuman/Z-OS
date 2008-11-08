#ifndef IO_PART_HEADER
#define IO_PART_HEADER

typedef struct
{
	DeviceInternal* Device;
	struct t_FileSystemInternal* FileSystem;
	UInt64 FirstByte;
	UInt64 ByteLength;
	Bool IsLive;
	UInt64 Position;
} PartInternal;

Int16 InternalReadPart(PartInternal* part, UInt64 pos, UInt8* buffer, UInt16 bufLen);
Int16 InternalWritePart(PartInternal* part, UInt64 pos, UInt8* buffer, UInt16 bufLen);

void InitPartitions(void);

#endif