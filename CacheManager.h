#ifndef _CACHE_MANAGER_HEADER_
#define _CACHE_MANAGER_HEADER_

/*typedef enum
{
	Read,
	Write
} BufferMode;

typedef enum
{
	Empty = 0,
	PartiallyFull = 1,
	Full = 2,
	LockedForDMA = Full | 4
} BufferPieceStatus;

typedef struct
{
	List Pieces;
	BufferMode Mode;
	UInt16 MinPieceSize;
	UInt16 MaxPieceSize;
	UInt16 AverageAddSize;
	UInt16 TotalSize;
	UInt16 IOPosition;
	List ThreadPositions;
} Buffer;

typedef struct
{
	UInt16 Size;
	BufferPieceStatus Status;
	UInt8* Bytes;
} BufferPiece;

typedef struct
{
	ThreadInternal* Thread;
	UInt16 Position;
} ThreadPosition;*/

#endif
