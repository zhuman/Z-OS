#include "..\Z-OS.h"
#include "DMA.h"

// DMA memory is layed out as a heap
#define DMAHeapSize 0x2000
UInt8 __attribute__ ((space(dma))) DMAHeap[DMAHeapSize];

UInt16 TypeDMA;
List DMAObjects = {0};

UInt8 NextDMAChannel = 0;

void DMACreate(InternalObject* obj)
{
	DMAInternal* dma = zmalloc(sizeof(DMAInternal));
	if (!dma) return;
	switch (NextDMAChannel++)
	{
		case 0:
			dma->ConBits = (DMAxCON*)&DMA0CON;
			dma->ReqBits = (DMAxREQ*)&DMA0REQ;
			dma->PeriphAddr = (UInt16*)&DMA0PAD;
			dma->StartAddr = (void**)&DMA0STA;
			dma->BufLen = (UInt16*)&DMA0CNT;
		break;
		case 1:
			dma->ConBits = (DMAxCON*)&DMA1CON;
			dma->ReqBits = (DMAxREQ*)&DMA1REQ;
			dma->PeriphAddr = (UInt16*)&DMA1PAD;
			dma->StartAddr = (void**)&DMA1STA;
			dma->BufLen = (UInt16*)&DMA1CNT;
		break;
		case 2:
			dma->ConBits = (DMAxCON*)&DMA2CON;
			dma->ReqBits = (DMAxREQ*)&DMA2REQ;
			dma->PeriphAddr = (UInt16*)&DMA2PAD;
			dma->StartAddr = (void**)&DMA2STA;
			dma->BufLen = (UInt16*)&DMA2CNT;
		break;
		case 3:
			dma->ConBits = (DMAxCON*)&DMA3CON;
			dma->ReqBits = (DMAxREQ*)&DMA3REQ;
			dma->PeriphAddr = (UInt16*)&DMA3PAD;
			dma->StartAddr = (void**)&DMA3STA;
			dma->BufLen = (UInt16*)&DMA3CNT;
		break;
		case 4:
			dma->ConBits = (DMAxCON*)&DMA4CON;
			dma->ReqBits = (DMAxREQ*)&DMA4REQ;
			dma->PeriphAddr = (UInt16*)&DMA4PAD;
			dma->StartAddr = (void**)&DMA4STA;
			dma->BufLen = (UInt16*)&DMA4CNT;
		break;
		case 5:
			dma->ConBits = (DMAxCON*)&DMA5CON;
			dma->ReqBits = (DMAxREQ*)&DMA5REQ;
			dma->PeriphAddr = (UInt16*)&DMA5PAD;
			dma->StartAddr = (void**)&DMA5STA;
			dma->BufLen = (UInt16*)&DMA5CNT;
		break;
		case 6:
			dma->ConBits = (DMAxCON*)&DMA6CON;
			dma->ReqBits = (DMAxREQ*)&DMA6REQ;
			dma->PeriphAddr = (UInt16*)&DMA6PAD;
			dma->StartAddr = (void**)&DMA6STA;
			dma->BufLen = (UInt16*)&DMA6CNT;
		break;
		case 7:
			dma->ConBits = (DMAxCON*)&DMA7CON;
			dma->ReqBits = (DMAxREQ*)&DMA7REQ;
			dma->PeriphAddr = (UInt16*)&DMA7PAD;
			dma->StartAddr = (void**)&DMA7STA;
			dma->BufLen = (UInt16*)&DMA7CNT;
		break;
		default:
			return;
	}
	obj->Data = (void*)dma;
	AddListItem(&DMAObjects,dma);
	obj->Flags |= ObjectFlagPermanent;
}

// Theoretically, this should never be called
void DMADestroy(InternalObject* obj)
{
	if (!obj) return;
	if (!obj->Data) return;
	RemoveListItem(&DMAObjects,GetIndexOf(&DMAObjects,obj->Data));
	zfree(obj->Data);
}

Int16 DMAGetInterface(UInt16 code, void** interface)
{
	switch (code)
	{
	case CodeIGeneric:
	{
		static const IGeneric inter = 
		{
			DMACreate,
			DMADestroy
		};
		*interface = (void*)&inter;
		break;
	}
/*	case CodeIDMA:
	{
		static const IDMA inter = 
		{
			DMASetSize,
			DMAGetSize,
			DMASetDevice,
			DMAGetDevice,
			DMA
		};
		*interface = (void*)&inter;
		break;
	}*/
/*	case CodeISimpleIO:
	{
		static const ISimpleIO inter = 
		{
			DMARead,
			DMAGetAvailableBytes,
			DMAWrite,
			DMASeek,
			DMAFlush
		};
		*interface = (void*)&inter;
		break;
	}
	case CodeIWaitable:
	{
		static const IWaitable inter = 
		{
			DMAStartWait
		};
		*interface = (void*)&inter;
		break;
	}
*/	default:
		return ErrorInvalidInterface;
	}
	return ErrorSuccess;
}

void InitDMA(void)
{
	// Create the DMA type
	TypeRegistration dma = {0};
	dma.Type = TypeDMA = GetUniqueTypeCode();
	dma.GetInterface = DMAGetInterface;
	
	// Create the 8 DMA channel objects
	
}

// Functions for allocating/deallocating from the DMA heap

void* AllocDMA(size_t size)
{
	UInt16* block = (UInt16*)DMAHeap;
	UInt16* lastBlock = block;
	
	// Make the size even
	if (size & 1) size++;
	
	EnterCriticalSection();
	for(;;)
	{
		// If we are at a free spot
		if (!(block[0]) || (!(block[1]) && (block[0] - (UInt16)block > (UInt16)size + 4)))
		{
			UInt16* oldBlock = (UInt16*)block[0];
			UInt16 i;
			
			// If there is no more room for this block, return null
			if (((UInt16)block - (UInt16)DMAHeap + 4 + (UInt16)size) > DMAHeapSize)
			{
				ExitCriticalSection();
				return null;
			}
			
			block[1] = (UInt16)lastBlock;
			
			if (!block[0] || (block[0] - (UInt16)block > (UInt16)size + 8))
			{
				block[0] = (UInt16)block + 4 + (UInt16)size;
				((UInt16*)(block[0]))[0] = (UInt16)oldBlock;
				((UInt16*)(block[0]))[1] = 0;
			}
			
			//TotalMemAllocs++;
			//TotalMemUsage += (UInt16)size;
			
			// Zero the block
			for (i = 0; i < (UInt16)(size >> 1); i++) ((UInt16*)(block + 4))[i] = 0;
			ExitCriticalSection();
			return (void*)((UInt16)block + 4);
		}
		
		lastBlock = block;
		block = (UInt16*)(block[0]);
	}
	
}


void FreeDMA(void* pointer)
{
	UInt16* block = (UInt16*)((UInt16)pointer - 4);
	
	EnterCriticalSection();
	
	//TotalMemAllocs--;
	//TotalMemUsage -= block[0] - (UInt16)block;
	
	// Look forward
	if (!((UInt16*)block[0])[1]) block[0] = ((UInt16*)block[0])[0];
	// Look backward
	if (block[1] != (UInt16)block && !((UInt16*)block[1])[1]) ((UInt16*)block[1])[0] = block[0];
	
	// Clear the back pointer
	block[1] = 0;
	
	ExitCriticalSection();
}

static void RunDMAxInterrupt(UInt8 channel)
{
	
}


