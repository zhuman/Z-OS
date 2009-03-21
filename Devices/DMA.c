#include "..\Z-OS.h"
#include "DMA.h"

// DMA memory is layed out as a heap
#define DMAHeapSize 0x2000
UInt8 __attribute__ ((space(dma))) DMAHeap[DMAHeapSize] = {0};

UInt16 TypeDMA;
List DMAObjects = {0};

UInt8 NextDMAChannel = 0;

static void DMACreate(InternalObject* obj)
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
static void DMADestroy(InternalObject* obj)
{
	if (!obj) return;
	if (!obj->Data) return;
	RemoveListItem(&DMAObjects,GetIndexOf(&DMAObjects,obj->Data));
	zfree(obj->Data);
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
/*	case CodeIDMA:
	{
		static const IDMA inter = 
		{
			DMASetSize,
			DMAGetSize,
			DMASetDevice,
			DMAGetDevice,
			DMASetFlags,
			DMAGetFlags
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
	}*/
	default:
		return ErrorInvalidInterface;
	}
	return ErrorSuccess;
}

void InitDMA(void)
{
	UInt16 handle;
	Int16 ret;
	UInt16 i;
	InternalObject* obj;
	
	// Create the DMA type
	TypeRegistration dma = {0};
	dma.Type = TypeDMA = GetUniqueTypeCode();
	dma.GetInterface = DMAGetInterface;
	
	// Init the DMA-enabled memory heap
	HeapInit(DMAHeap, DMAHeapSize);
	
	// Create the 8 DMA channel objects
	for (i = 0; i < 8; i++)
	{
		char name[5] = {0};
		sprintf(name, "DMA%d", i + 1);
		if ((ret = CreateObject(TypeDMA,&handle,name))) return;
		ReleaseObject(handle);
	}
	
	if ((ret = OpenObject("DMA1",&handle))) return;
	if ((ret = InternalObjectFromHandle(handle,&obj))) return;
	if ((ret = CreateSymbolicLink("DMA",obj))) return;
}

// Functions for allocating/deallocating from the DMA heap

static void* AllocDMA(size_t size)
{
	return HeapAlloc((UInt16*)DMAHeap,DMAHeapSize,size);
}

static void FreeDMA(void* pointer)
{
	HeapFree((UInt16*)DMAHeap,DMAHeapSize,pointer);
}

static void RunDMAxInterrupt(UInt8 channel)
{
	
}
