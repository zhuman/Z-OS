#ifndef IO_FS_HEADER
#define IO_FS_HEADER

typedef struct
{
	char Name[8];
	Bool HasSecurity;
	Bool IsCaseSensitive;
} FileSystemInfo;

typedef struct
{
	Bool (*Detect)(PartInternal* device);
	Int16 (*MountDevice)(PartInternal* device);
	Int16 (*UnmountDevice)(PartInternal* device, Bool suprise);
	Bool (*FileExists)(PartInternal* device, char* path);
	// Gets a file's (or dir's) attributes
	Int16 (*GetFile)(FileInternal* file, char** reparse);
	// Opens a file for IO
	Int16 (*OpenFile)(FileInternal* file, FileMode mode);
	// Sets file attributes
	Int16 (*SetFile)(FileInternal* file);
	// Deletes a file
	Int16 (*DeleteFile)(FileInternal* file);
	// Renames a file
	Int16 (*RenameFile)(FileInternal* file, char* path);
	// The IO functions for opened files
	Int16 (*ReadFile)(FileInternal* file, UInt64 pos, UInt8* buffer, UInt16 bufLen);
	Int16 (*WriteFile)(FileInternal* file, UInt64 pos, UInt8* buffer, UInt16 bufLen);
} FileSystemFuncs;

typedef struct t_FileSystemInternal
{
	FileSystemInfo Info;
	FileSystemFuncs Funcs;
} FileSystemInternal;

Int16 RegisterFileSystem(FileSystemInfo info, FileSystemFuncs funcs);

#endif
