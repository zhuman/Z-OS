#include "..\..\Z-OS.h"

// Sample File System Driver
//
// Implements a virtual file system that does not 
// actually access the drive it is on. Files can 
// be added to the VirtualFiles array.

char VirtualFiles[] = 
{
	"MYFILE.TXT\0"
	"SAMPLE.TXT\0"
	"COOLFILE.FIL\0"
	"TASKS.TSK\0"
	"\0"
};

char VirtualDirs[] = 
{
	"DIR1\0"
	"MYDIR\0"
	"COOLDIR\0"
	"\0"
};

Bool SampleDetect(UInt16 deviceHandle)
{
	// Normally one would actually request an ISimpleIO 
	// interface and read some partition table to 
	// determine whether the drive is supported.
	return True;
}

Int16 SampleMountDevice(UInt16 deviceHandle)
{
	// Initialize data structures, prepare caches, etc.
	return ErrorSuccess;
}

Bool SampleFileExists(Int16 deviceId, char* name)
{
	//Int16 i;
	
	return ErrorSuccess;
}

/*void SampleFSInit(void)
{
	FileSystemInfo info;
	FileSystemFuncs funcs;
	
	strcpy(info.Name,"ZFS");
	info.HasSecurity = False;
	info.NameRestrictions = EightDotThree;
	info.IsCaseSensitive = False;
	
	funcs.Detect = SampleDetect;
	funcs.MountDevice = SampleMountDevice;
	funcs.FileExists = SampleFileExists;
	
	RegisterFileSystem(info,funcs);
}*/
