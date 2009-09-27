#include "..\Z-OS.h"
#include "DMA.h"

// DMA memory is layed out as a heap
#define DMAHeapSize 0x200
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

// Sets the value DMA interrupt flag for the indexed DMA channel
static inline void SetDMAIF(UInt16 index, Bool value)
{
	switch (index)
	{
	case 0:
		IFS0bits.DMA0IF = !!value;
		break;
	case 1:
		IFS0bits.DMA1IF = !!value;
		break;
	case 2:
		IFS1bits.DMA2IF = !!value;
		break;
	case 3:
		IFS2bits.DMA3IF = !!value;
		break;
	case 4:
		IFS2bits.DMA4IF = !!value;
		break;
	case 5:
		IFS3bits.DMA5IF = !!value;
		break;
	case 6:
		IFS4bits.DMA6IF = !!value;
		break;
	case 7:
		IFS4bits.DMA7IF = !!value;
		break;
	}
}

// Returns the value of a DMA interrupt flag for the indexed DMA channel
static inline Bool GetDMAIF(UInt16 index)
{
	switch (index)
	{
	case 0:
		return IFS0bits.DMA0IF;
		break;
	case 1:
		return IFS0bits.DMA1IF;
		break;
	case 2:
		return IFS1bits.DMA2IF;
		break;
	case 3:
		return IFS2bits.DMA3IF;
		break;
	case 4:
		return IFS2bits.DMA4IF;
		break;
	case 5:
		return IFS3bits.DMA5IF;
		break;
	case 6:
		return IFS4bits.DMA6IF;
		break;
	case 7:
		return IFS4bits.DMA7IF;
		break;
	}
}

// Sets the value DMA interrupt flag for the indexed DMA channel
static inline void SetDMAIE(UInt16 index, Bool value)
{
	switch (index)
	{
	case 0:
		IEC0bits.DMA0IE = !!value;
		break;
	case 1:
		IEC0bits.DMA1IE = !!value;
		break;
	case 2:
		IEC1bits.DMA2IE = !!value;
		break;
	case 3:
		IEC2bits.DMA3IE = !!value;
		break;
	case 4:
		IEC2bits.DMA4IE = !!value;
		break;
	case 5:
		IEC3bits.DMA5IE = !!value;
		break;
	case 6:
		IEC4bits.DMA6IE = !!value;
		break;
	case 7:
		IEC4bits.DMA7IE = !!value;
		break;
	}
}

// Returns the value of a DMA interrupt flag for the indexed DMA channel
static inline Bool GetDMAIE(UInt16 index)
{
	switch (index)
	{
	case 0:
		return IEC0bits.DMA0IE;
		break;
	case 1:
		return IEC0bits.DMA1IE;
		break;
	case 2:
		return IEC1bits.DMA2IE;
		break;
	case 3:
		return IEC2bits.DMA3IE;
		break;
	case 4:
		return IEC2bits.DMA4IE;
		break;
	case 5:
		return IEC3bits.DMA5IE;
		break;
	case 6:
		return IEC4bits.DMA6IE;
		break;
	case 7:
		return IEC4bits.DMA7IE;
		break;
	default:
		return 0;
	}
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
			dma->InUse = true;
			return;
		}
	}
}

static void DMADestroy(InternalObject* obj)
{
	DMAInternal* dma;
	if (!obj) return;
	if (!obj->Data) return;
	dma = obj->Data;
	dma->InUse = false;
	obj->Data = NULL;
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
		SetDMAIE(dma->Channel, !(flags & DMANoInterrupt));
		
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
					(dma->ReqBits->FORCE ? DMAForce : 0) |
					(GetDMAIE(dma->Channel) ? 0 : DMANoInterrupt);
		
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

static Int16 DMASetDevice(UInt16 handle, UInt16* device, UInt8 requestIrq)
{
	InternalObject* obj;
	Int16 ret;
	
	if (!device) return ErrorNullArg;
	
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		DMAInternal* dma = obj->Data;
		
		*(dma->PeriphAddr) = (UInt16)device;
		dma->ReqBits->IRQSEL = 0b01111111 & requestIrq;
		
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

static Int16 DMAGetDevice(UInt16 handle, UInt16** device, UInt8* requestIrq)
{
	InternalObject* obj;
	Int16 ret;
	
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		DMAInternal* dma = obj->Data;
		
		*device = (UInt16*)*(dma->PeriphAddr);
		*requestIrq = (UInt8)(dma->ReqBits->IRQSEL);
		
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
		
		dma->ConBits->MODE = (pingPong ? 0b10 : 0) | (!continuous ? 0b01 : 0);
		
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
		FinishWait(dma, NULL);
		
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
		DMAInternal* dma = obj->Data;
		if (!(dma->ConBits->CHEN))
		{
			return ErrorNoWait;
		}
		return ErrorSuccess;
	}
	else
	{
		return ErrorInvalidHandle;
	}
}

static Int16 DMATransfer(UInt16 handle, DMAInternal* dma, UInt16 direction, UInt8* buffer, UInt16 bufLen)
{
	Bool bufferExisted = true;
	
	// Disable the channel
	dma->ConBits->CHEN = 0;
	
	// Make sure a buffer exists in DMA memory
	if (dma->CurrentBuffer)
	{
		if (bufLen != dma->CurrentBufferLen)
		{
			bufferExisted = false;
			FreeDMA(dma->CurrentBuffer);
			dma->CurrentBuffer = AllocDMA(bufLen);
			dma->CurrentBufferLen = bufLen;
			if (!dma->CurrentBuffer) return ErrorOutOfMemory;
		}
	}
	else
	{
		bufferExisted = false;
		
		// Allocate a buffer in DMA memory
		dma->CurrentBuffer = AllocDMA(bufLen);
		dma->CurrentBufferLen = bufLen;
		if (!dma->CurrentBuffer) return ErrorOutOfMemory;
	}
	
	// Transfer direction
	dma->ConBits->DIR = direction;
	
	// Set up the DMA buffer
	*(dma->StartAddrA) = (void*)((int)dma->CurrentBuffer - 4000);
	*(dma->BufLen) = bufLen; //(dma->ConBits->SIZE) ? bufLen : (bufLen >> 1);
	
	// If writing...
	if (direction)
	{
		// Copy over the data to the DMA buffer
		memcpy(dma->CurrentBuffer, buffer, bufLen);
		
		// DMA channel enable
		SetDMAIF(dma->Channel,0);
		dma->ConBits->CHEN = 1;
		
		return ErrorSuccess;
	}
	// If reading...
	else
	{
		// DMA channel enable
		SetDMAIF(dma->Channel, 0);
		dma->ConBits->CHEN = 1;
		
		WaitForObject(handle);
		
		// Copy over the data to the user buffer
		memcpy(buffer, dma->CurrentBuffer, bufLen);
		
		return ErrorSuccess;
	}
}

static Int16 DMARead(UInt16 handle, UInt8* buffer, UInt16 bufLen)
{
	InternalObject* obj;
	Int16 ret;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return ret;
	if (obj)
	{
		DMAInternal* dma = obj->Data;
		
		return DMATransfer(handle, dma, 0, buffer, bufLen);
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
		
		return DMATransfer(handle, dma, 1, buffer, bufLen);
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
	dma->StartAddrA = (void**)&DMA##x##STA;	\
	dma->StartAddrB = (void**)&DMA##x##STB;	\
	dma->BufLen = (UInt16*)&DMA##x##CNT;	\
} while (0)

void InitDMA(void)
{
	UInt16 i;
	
	// Create the DMA type
	TypeRegistration dma = {0};
	dma.Type = TypeDMA;
	dma.GetInterface = DMAGetInterface;
	RegisterTypeManager(dma);
	
	// Init the DMA-enabled memory heap
	HeapInit(DMAHeap, DMAHeapSize);
	
	// Create the 8 DMA channels
	for (i = 0; i < 8; i++)
	{
		DMAInternal* dma = zmalloc(sizeof(DMAInternal));
		if (!dma) return;
		
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

static inline void RunDMAxInterrupt(UInt8 channel)
{
	DMAInternal* dma = GetListItem(&DMAObjects,channel);
	FinishWait(dma, NULL);
	putchar('C');
	printf("DMA completed: %d\r\n", (UInt16)channel);
	
	// Disable the interrupt flag
	SetDMAIF(channel, 0);
}

#define DefineDMAInterrupt(x)												\
void __attribute__((interrupt, auto_psv)) _DMA##x##Interrupt(void)			\
{																			\
	RunDMAxInterrupt(x);													\
}

DefineDMAInterrupt(0)
DefineDMAInterrupt(1)
DefineDMAInterrupt(2)
DefineDMAInterrupt(3)
DefineDMAInterrupt(4)
DefineDMAInterrupt(5)
DefineDMAInterrupt(6)
DefineDMAInterrupt(7)
