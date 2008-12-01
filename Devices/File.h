#ifndef IO_FILE_HEADER
#define IO_FILE_HEADER

#include "..\IO.h"

typedef enum
{
	Read,
	Write,
	Append	
} FileMode;

typedef enum
{
	ReadOnly,
	Hidden,
	Archive,
	System
} FileAttributes;

typedef struct
{
	Int16 (*Open)(UInt16 handle, FileMode mode);
	Int16 (*Delete)(UInt16 handle);
	Int16 (*Rename)(UInt16 handle, char* name);
	Int16 (*SetAttributes)(UInt16 handle, FileAttributes attribs, Bool on);
	FileAttributes (*GetAttributes)(UInt16 handle);
} IFile;

typedef struct
{
	Int16 (*EnumObjects)(UInt16 handle, List* list);
	Int16 (*EnumDirectories)(UInt16 handle, List* list);
	Int16 (*Rename)(UInt16 handle, char* name);
	Int16 (*Delete)(UInt16 handle);
} IDirectory;

typedef struct
{
	char* Name;
	PartInternal* Partition;
	struct t_FileSystemInternal* FileSystem;
	Bool IsDirectory;
	FileAttributes Attributes;
	UInt64 FileLength;
	
	// File Properties
	Bool IsOpened;
	FileMode Mode;
	UInt64 Position;
	Bool Locked;
	
	// Available for FS use
	void* Data1;
	void* Data2;
	void* Data3;
	void* Data4;
} FileInternal;

void InitFiles(void);

#endif
