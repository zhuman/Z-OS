#include "..\Z-OS.h"
/*
int open(const char* name, int access, int mode)
{
	// Open the file with the Object Manager
	UInt16 handle;
	Int16 ret;
	IFile* file;
	if ((ret = OpenObject((char*)name,&handle))) return 0;
	if ((ret = GetInterface(handle,CodeIFile,(void**)&file)))
	{
		if ((ret = file->Open(handle, (access == 0) ? Read : Write))) return 0;
	}
	return *(int*)(&handle); // Convert it to an int without actual conversion
}

int close(int handle)
{
	ReleaseObject(*(UInt16*)(&handle));
	return 0;
}

int write(int handle, void* buffer, unsigned int len)
{
	UInt16 realHandle = *(UInt16*)(&handle);
	Int16 ret;
	ISimpleIO* inter;
	if ((ret = GetInterface(realHandle,CodeISimpleIO,(void**)&inter))) return len;
	if ((ret = inter->Write(realHandle,buffer,len))) return len;
	return 0;
}

int read(int handle, void *buffer, unsigned int len)
{
	return 0;
}
*/
