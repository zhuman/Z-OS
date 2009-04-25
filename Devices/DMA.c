#include "..\Z-OS.h"
#include "DMA.h"

// DMA memory is layed out as a heap
#define DMAHeapSize 0x2000
UInt8 __attribute__ ((space(dma))) DMAHeap[DMAHeapSize] = {0};

List DMAObjects = {0};

// Functions for allocating/deallocating from the DMA heap

static void* AllocDMA(size_t size)
{
	return HeapAlloc((UInt16*)DMAHeap,DMAHeapSize,size);
}

static void FreeDMA(void* pointer)
{
	HeapFree((UInt16*)DMAHeap,DMAHeapSize,pointer);
}

static void DMACreate(InternalObject* obj)
{
	UInt16 i;
	
	// Find a DMA channel that isn't being used	and assign this object to it
	for (i = 0; i < DMAObjects.Length; i++)
	{
		DMAInternal* dma = GetListItem(&DMAObjects,i);
		if (!dma->InUse)
		{
			obj->Data = dma;
			return;
		}
	}
}

static void DMADestroy(InternalObject* obj)
{
	if (!obj) return;
	if (!obj->Data) return;
	obj->Data = null;
}

static Int16 DMASetFlags(UInt16 handle, DMAFlags flags)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		DMAInternal* dma = obj->Data;
		
		dma->ConBits->NULLW = !!(flags & DMANullDataWrite);
		dma->ReqBits->FORCE = !!(flags & DMAForce);
		
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

static Int16 DMAGetFlags(UInt16 handle, DMAFlags* flags)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		DMAInternal* dma = obj->Data;
		
		*flags = 	(dma->ConBits->NULLW ? DMANullDataWrite : 0) | 
					(dma->ReqBits->FORCE ? DMAForce : 0);
		
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

static Int16 DMASetSize(UInt16 handle, DMASize size)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		DMAInternal* dma = obj->Data;
		
		dma->ConBits->SIZE = size ? 0 : 1;
		
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

static Int16 DMAGetSize(UInt16 handle, DMASize* size)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		DMAInternal* dma = obj->Data;
		
		*size = dma->ConBits->SIZE ? DMAByte : DMAWord;
		
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

static Int16 DMASetDevice(UInt16 handle, UInt16* device)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		DMAInternal* dma = obj->Data;
		
		dma->PeriphAddr = device;
		
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

static Int16 DMAGetDevice(UInt16 handle, UInt16** device)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		DMAInternal* dma = obj->Data;
		
		*device = dma->PeriphAddr;
		
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

static Int16 DMASetMode(UInt16 handle, Bool pingPong, Bool continuous)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		DMAInternal* dma = obj->Data;
		
		dma->ConBits->MODE = (pingPong ? 0b01 : 0) | (continuous ? 0b01 : 0);
		
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

static Int16 DMAGetMode(UInt16 handle, Bool* pingPong, Bool* continuous)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		DMAInternal* dma = obj->Data;
		
		*pingPong = !!(dma->ConBits->MODE & 0b10);
		*continuous = !!(dma->ConBits->MODE & 0b01);
		
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

static Int16 DMACancelTransfer(UInt16 handle)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		DMAInternal* dma = obj->Data;
		
		// Disable the channel
		dma->ConBits->CHEN = 0;
		
		// Allow threads that are waiting on this channel to finish the wait
		FinishWait(dma,null);
		
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

static Int16 DMAStartWait(UInt16 handle)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

static Int16 DMATransfer(DMAInternal* dma, UInt16 direction, UInt8* buffer, UInt16 bufLen)
{
	// Disable the channel
	dma->ConBits->CHEN = 0;
	
	// Make sure a buffer exists in DMA memory
	if (dma->CurrentBuffer)
	{
		if (bufLen != *(dma->BufLen))
		{
			FreeDMA(dma->CurrentBuffer);
			dma->CurrentBuffer = AllocDMA(bufLen);
			if (!dma->CurrentBuffer) return ErrorOutOfMemory;
		}
	}
	else
	{
		// Allocate a buffer in DMA memory
		dma->CurrentBuffer = AllocDMA(bufLen);
		if (!dma->CurrentBuffer) return ErrorOutOfMemory;
	}
	
	// Copy over the data to the DMA buffer
	memcpy(dma->CurrentBuffer,buffer,bufLen);
	*(dma->StartAddr) = dma->CurrentBuffer;
	*(dma->BufLen) = bufLen;
	
	// Transfer direction
	dma->ConBits->DIR = direction;
	
	// DMA channel enable
	dma->ConBits->CHEN = 1;
	
	return ErrorSuccess;
}

static Int16 DMARead(UInt16 handle, UInt8* buffer, UInt16 bufLen)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		DMAInternal* dma = obj->Data;
		
		return DMATransfer(dma,0,buffer,bufLen);
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

static Int16 DMAGetAvailableBytes(UInt16 handle, UInt64* bytes)
{
	return ErrorUnimplemented;
}

static Int16 DMAWrite(UInt16 handle, UInt8* buffer, UInt16 bufLen)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		DMAInternal* dma = obj->Data;
		
		return DMATransfer(dma,1,buffer,bufLen);
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

static Int16 DMASeek(UInt16 handle, Int64 position, SeekRelationEnum relation)
{
	return ErrorUnimplemented;
}

static Int16 DMAFlush(UInt16 handle)
{
	return WaitForObject(handle);
}

static Int16 DMAGetInterface(UInt16 code, void** interface)
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
	case CodeIDMA:
	{
		static const IDMA inter = 
		{
			DMASetSize,
			DMAGetSize,
			DMASetDevice,
			DMAGetDevice,
			DMASetFlags,
			DMAGetFlags,
			DMASetMode,
			DMAGetMode,
			DMACancelTransfer
		};
		*interface = (void*)&inter;
		break;
	}
	case CodeISimpleIO:
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
	default:
		return ErrorInvalidInterface;
	}
	return ErrorSuccess;
}

// Assigns DMA registers to a DMAInternal* variable named 'dma'
#define AssignDmaChannel(x) do				\
{											\
	dma->ConBits = (DMAxCON*)&DMA##x##CON;	\
	dma->ReqBits = (DMAxREQ*)&DMA##x##REQ;	\
	dma->PeriphAddr = (UInt16*)&DMA##x##PAD;\
	dma->StartAddr = (void**)&DMA##x##STA;	\
	dma->BufLen = (UInt16*)&DMA##x##CNT;	\
} while (0)

void InitDMA(void)
{
	UInt16 i;
	
	// Create the DMA type
	TypeRegistration dma = {0};
	dma.Type = TypeDMA;
	dma.GetInterface = DMAGetInterface;
	
	// Init the DMA-enabled memory heap
	HeapInit(DMAHeap, DMAHeapSize);
	
	// Create the 8 DMA channels
	for (i = 0; i < 8; i++)
	{
		DMAInternal* dma = zmalloc(sizeof(DMAInternal));
		AddListItem(&DMAObjects,dma);
		
		dma->Channel = i;
		switch (i)
		{
			case 0:
				AssignDmaChannel(0);
			break;
			case 1:
				AssignDmaChannel(1);
			break;
			case 2:
				AssignDmaChannel(2);
			break;
			case 3:
				AssignDmaChannel(3);
			break;
			case 4:
				AssignDmaChannel(4);
			break;
			case 5:
				AssignDmaChannel(5);
			break;
			case 6:
				AssignDmaChannel(6);
			break;
			case 7:
				AssignDmaChannel(7);
			break;
		}
	}
}

// All of the DMA interrupts are defined to run a common function

static void RunDMAxInterrupt(UInt8 channel)
{
	FinishWait(GetListItem(&DMAObjects,channel),null);
}

#define DefineDMAInterrupt(x) \
void __attribute__((__interrupt__, no_auto_psv)) _DMA##x##Interrupt(void){RunDMAxInterrupt(x);}

DefineDMAInterrupt(0)
DefineDMAInterrupt(1)
DefineDMAInterrupt(2)
DefineDMAInterrupt(3)
DefineDMAInterrupt(4)
DefineDMAInterrupt(5)
DefineDMAInterrupt(6)
DefineDMAInterrupt(7)
