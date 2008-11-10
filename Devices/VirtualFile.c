#include "..\Z-OS.h"
#include "stdio.h"

char* RealFile = "C:\\Documents and Settings\\Owner\\Desktop\\DiskDump.txt";
FILE* fileHandle;

Int16 FileDeviceRead(Int16 id, UInt64 pos, UInt8* buffer, UInt16 bufferLen)
{
	Int16 ret;
	ret = fseek(fileHandle,pos,SEEK_SET);
	if (ret) return ErrorUnknown;
	ret = fread(buffer,1,bufferLen,fileHandle);
	if (ret != bufferLen) return ErrorUnknown;
	return ErrorSuccess;
}

Int16 FileDeviceWrite(Int16 id, UInt64 pos, UInt8* buffer, UInt16 bufferLen)
{
	Int16 ret;
	ret = fseek(fileHandle,pos,SEEK_SET);
	if (ret) return ErrorUnknown;
	ret = fwrite(buffer,1,bufferLen,fileHandle);
	if (ret != bufferLen) return ErrorUnknown;
	return ErrorSuccess;
}

Int16 FileDeviceCommand(Int16 id, Int16 cmd, UInt8* buffer, UInt16 bufferLen)
{
	return ErrorUnimplemented;
}

Int16 FileDeviceGetAvailableBytes(Int16 id, UInt64* bytes)
{
	Int16 ret;
	UInt64 len;
	ret = fseek(fileHandle,0,SEEK_END);
	if (ret) return ErrorUnknown;
	len = ftell(fileHandle);
	*bytes = len;
	return ErrorSuccess;
}

Int16 FileDeviceFlush(Int16 id)
{
	Int16 ret;
	ret = fflush(fileHandle);
	if (ret) return ErrorUnknown;
	return ErrorSuccess;
}

void CloseVirtualFile(void)
{
	fclose(fileHandle);
}

void InitFileDevice(void)
{
	DeviceInfo info = {0};
	DeviceFuncs funcs = {0};
	Int16 id;
	
	fileHandle = fopen(RealFile,"rb");
	
	// Fill in the info structure
	info.CanRead = True;
	info.CanWrite = True;
	info.CanSeek = True;
	info.CanMount = True;
	info.UseCacheManager = False;
	info.IsSectored = True;
	info.SectorSize = 512;
	
	// Fill in the device interface
	funcs.Read = FileDeviceRead;
	funcs.Write = FileDeviceWrite;
	funcs.Command = FileDeviceCommand;
	funcs.GetAvailableBytes = FileDeviceGetAvailableBytes;
	funcs.Flush = FileDeviceFlush;
	
	RegisterDevice("VirtualFile",info,funcs,&id);
}
