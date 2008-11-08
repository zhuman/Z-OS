#include "Z-OS.h"

/*Int16 BufferMemUsage = 0;

// Drivers should have first called EnterCriticalSection() before 
// calling this method if they wish to be thread-safe.
Int16 BufferInit(BufferMode mode, UInt16 initSize, Buffer* buf)
{
	BufferPiece* firstPiece;
	
	// Clear any remnants out of the pieces list
	while (buf->Pieces.Length > 0)
	{
		zfree(RemoveListItem(&(buf->Pieces),0));
	}
	buf->Mode = mode;
	buf->MinPieceSize = 0x20;
	buf->MaxPieceSize = 0x100;
	buf->AverageAddSize = initSize;
	buf->TotalSize = initSize;
	buf->IOPosition = 0;
	// Clear any remnants out of the thread positions list
	while (buf->ThreadPositions.Length > 0)
	{
		zfree(RemoveListItem(&(buf->ThreadPositions),0));
	}
	
	// Allocate the first piece
	firstPiece = zmalloc(sizeof(BufferPiece));
	if (!firstPiece) return ErrorOutOfMemory;
	firstPiece->Size = initSize;
	firstPiece->Bytes = zmalloc((size_t)initSize);
	if (!(firstPiece->Bytes)) return ErrorOutOfMemory;
	AddListItem(&(buf->Pieces),firstPiece);
	
	return ErrorSuccess;
}

// Copies data between a user's buffer and the buffer object 
// in the direction required by the buffer's mode (e.g. for 'read'
// buffers, data is copied buffer -> user and vice versa for 'write').
// This method should be called by drivers' SimpleIO.Read and 
// SimpleIO.Write functions.
Int16 BufferUserCopy(Buffer* buf, UInt8* userBuf, UInt16 userBufLen)
{
	Int16 i;
	ThreadPosition* thrPos;
	Int16 wasPositioned = False;
	
	EnterCriticalSection();
	
	// Find the thread's position if it has read/written before
	for (i = 0; i < buf->ThreadPositions.Length; i++)
	{
		thrPos = (ThreadPosition*)GetListItem(&(buf->ThreadPositions),i);
		if (thrPos->Thread == CurrentThread)
		{
			wasPositioned = True;
			break;
		}
	}
	
	// Create the thread's position tracking struct
	if (!wasPositioned)
	{
		thrPos = zmalloc(sizeof(ThreadPosition));
		if (!thrPos) return ErrorOutOfMemory;
		thrPos->Thread = CurrentThread;
		thrPos->Position = 0;
		
	}
	
	// Disable all peripheral interrupts
	__asm volatile ("disi #0x3FFF"); // Btw, let's assume that this is large enough
	
	if (buf->Mode == Read)
	{
		UInt16 pos = 0;
		for (i = 0; i < buf->Pieces.Length; i++)
		{
			BufferPiece* piece = (BufferPiece*)GetListItem(&(buf->Pieces),i);
			if ((thrPos->Position > pos) && (thrPos->Position - pos <= piece->Size))
			{
				UInt16 j;
				for (j = thrPos->Position - pos; (j < (thrPos->Position - pos + userBufLen)) && (j < piece->Size); j++)
				{
					userBuf[j] = piece->Bytes[pos];
				}
			}
			pos += piece->Size;
		}
		thrPos->Position += userBufLen;
		
		// Release any now-useless buffer pieces
		// Find the thread's position if it has read/written before
		{
			Int16 minPos;
			for (i = 0; i < buf->ThreadPositions.Length; i++)
			{
				thrPos = (ThreadPosition*)GetListItem(&(buf->ThreadPositions),i);
				if (thrPos->Position < minPos) minPos = thrPos->Position;
			}
		}
	}
	else // buf->Mode == Write
	{
		
	}
	
	// Re-enable all peripheral interrupts
	__asm volatile ("disi #0");
	ExitCriticalSection();
	
	return ErrorSuccess;
}
*/
