#include "..\Z-OS.h"
#include "..\..\SDCard\Public.h"
#include "..\..\NewFAT\Public.h"
//#include "..\..\FileDevice\Public.h"

// This must be here in order to boot Z-OS
int main(void){return BootOS();}

// Called before the thread scheduler has been initialized.
// You can customize which subsystems are used by simply 
// removing calls to their initialization routines.
void Phase0Init(void)
{
	// Register the various object types
	InitializeTimers();
	InitializeSync();
	InitializeQueues();
	InitializeIO();
	
	InitSerialPorts();
	// Initialize other device drivers here
	InitSD();
	InitFAT();
	//InitVirtualFileDevice();
}

// Called after the thread manager has been initialized, but 
// before actual threading has started.
void Phase1Init(void)
{
	// Create the system worker threads and register the Delegate type.
	//InitializeDelegates();
	//InitShell();
}

void TestSDMount(void)
{
	UInt16 serHandle;
	UInt16 sdHandle;
	UInt16 fsHandle;
	Int16 ret;
	IDevice* sdDev;
	ISimpleIO* serIo;
	ISimpleIO* fsIo;
	IFile* fsFile;
	UInt8* buffer = zmalloc(512);
	
	if (!buffer)
	{
		puts("Buffer could not be allocated.\r\n");
		return;
	}
	
	if ((ret = OpenObject("SerialPort1", &serHandle))) return;
	if ((ret = GetInterface(serHandle, CodeISimpleIO, (void**)&serIo))) return;
	
	if ((ret = OpenObject("SD",&sdHandle)))
	{
		printf("Error on OpenObject(\"SD\"): 0x%x\r\n",ret);
		return;
	}
	else puts("OpenObject(\"SD\") succeeded.\r\n");
	if ((ret = GetInterface(sdHandle,CodeIDevice,(void**)&sdDev)))
	{
		printf("Error on GetInterface(\"SD\"): 0x%x\r\n",ret);
		return;
	}
	else puts("GetInterface(\"SD\") succeeded.\r\n");
	if ((ret = sdDev->Mount(sdHandle, 0, "FAT", "SDPart1")))
	{
		printf("Error on Mount(\"SD\"): 0x%x\r\n",ret);
		return;
	}
	else puts("Mount(\"SD\") succeeded.\r\n");
	ReleaseObject(sdHandle);
	
	if ((ret = OpenObject("SDPart1\\TEST.TXT",&fsHandle)))
	{
		printf("Error at OpenObject(file): 0x%x\r\n",ret);
		return;
	}
	if ((ret = GetInterface(fsHandle,CodeIFile,(void**)&fsFile)))
	{
		printf("Error on GetInterface(file): 0x%x\r\n",ret);
		return;
	}
	if ((ret = fsFile->Open(fsHandle,Read)))
	{
		printf("Error on Open(file): 0x%x\r\n",ret);
		return;
	}
	if ((ret = GetInterface(fsHandle,CodeISimpleIO,(void**)&fsIo)))
	{
		printf("Error on GetInterface(file): 0x%x\r\n",ret);
		return;
	}
	
	ReleaseObject(fsHandle);
	ReleaseObject(serHandle);
}

void TestSDSimpleIO(void)
{
	UInt16 handle;
	UInt16 serHandle;
	Int16 ret;
	ISimpleIO* io;
	ISimpleIO* serIo;
	UInt8* buffer = zmalloc(512);
	Int32 i;
	UInt64 size;
	
	if ((ret = OpenObject("SerialPort1", &serHandle))) return;
	if ((ret = GetInterface(serHandle, CodeISimpleIO, (void**)&serIo))) return;
	
	ret = OpenObject("SD", &handle);
	printf("OpenObject called on SD: %d\r\n", ret);
	
	ret = GetInterface(handle,CodeISimpleIO, (void**)&io);
	printf("GetInterface called on SD: %d\r\n", ret);
	
	ret = io->GetAvailableBytes(handle, &size);
	if (ret)
		printf("GetAvailableBytes failed on SD: %d\r\n",ret);
	else
		printf("GetAvailableBytes succeeded on SD: %llX\r\n",size);
	
	for (i = 0; i < 5; i++)
	{
		puts("Reading from SD...\r\n");
		ret = io->Read(handle, buffer, 512);
		printf("Finished reading from SD: %d\r\n",ret);
		//serIo->Write(serHandle, buffer, 512);
		puts("Finished sending through serial port...\r\n");
		//for (;;);
	}
	
	ReleaseObject(serHandle);
	ReleaseObject(handle);
}

extern size_t TotalMemUsage;

extern void CloseVirtualFile(void);

void StartupThreadProc(void)
{
	
	puts("---------------------------\r\nStarting tests...\r\n---------------------------\r\n");
	
	//TestSDSimpleIO();
	TestSDMount();
	
	//CloseVirtualFile();
	
	puts("---------------------------\r\nRetiring for the night...\r\n---------------------------\r\n");
	printf("Memory usage was: 0x%x\r\n", TotalMemUsage);
	
	for(;;);
}
