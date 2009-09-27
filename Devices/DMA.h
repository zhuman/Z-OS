#ifndef _DMA_DEVICE_HEADER_
#define _DMA_DEVICE_HEADER_

typedef enum
{
	DMAByte,
	DMAWord
} DMASize;

typedef enum
{
	DMAForce			= 0b00000001,
	DMANullDataWrite	= 0b00000010,
	DMANoInterrupt		= 0b00000100
} DMAFlags;

typedef struct
{
	Int16 (*SetSize)(UInt16 handle, DMASize size);
	Int16 (*GetSize)(UInt16 handle, DMASize* size);
	Int16 (*SetDevice)(UInt16 handle, UInt16* device, UInt8 requestIrq);
	Int16 (*GetDevice)(UInt16 handle, UInt16** device, UInt8* requestIrq);
	Int16 (*SetFlags)(UInt16 handle, DMAFlags flags);
	Int16 (*GetFlags)(UInt16 handle, DMAFlags* flags);
	Int16 (*SetMode)(UInt16 handle, Bool pingPong, Bool continuous);
	Int16 (*GetMode)(UInt16 handle, Bool* pingPong, Bool* continuous);
	Int16 (*CancelTransfer)(UInt16 handle);
} IDMA;

typedef struct
{
	union
	{
		struct
		{
			__attribute__ ((packed)) unsigned MODE:2;
			__attribute__ ((packed)) unsigned :2;
			__attribute__ ((packed)) unsigned AMODE:2;
			__attribute__ ((packed)) unsigned :5;
			__attribute__ ((packed)) unsigned NULLW:1;
			__attribute__ ((packed)) unsigned HALF:1;
			__attribute__ ((packed)) unsigned DIR:1;
			__attribute__ ((packed)) unsigned SIZE:1;
			__attribute__ ((packed)) unsigned CHEN:1;
		};
		struct
		{
			__attribute__ ((packed)) unsigned MODE0:1;
			__attribute__ ((packed)) unsigned MODE1:1;
			__attribute__ ((packed)) unsigned :2;
			__attribute__ ((packed)) unsigned AMODE0:1;
			__attribute__ ((packed)) unsigned AMODE1:1;
		};
	};
} DMAxCON;

typedef struct
{
	union
	{
		struct
		{
			__attribute__ ((packed)) unsigned IRQSEL:7;
			__attribute__ ((packed)) unsigned :8;
			__attribute__ ((packed)) unsigned FORCE:1;
		};
		struct
		{
			__attribute__ ((packed)) unsigned IRQSEL0:1;
			__attribute__ ((packed)) unsigned IRQSEL1:1;
			__attribute__ ((packed)) unsigned IRQSEL2:1;
			__attribute__ ((packed)) unsigned IRQSEL3:1;
			__attribute__ ((packed)) unsigned IRQSEL4:1;
			__attribute__ ((packed)) unsigned IRQSEL5:1;
			__attribute__ ((packed)) unsigned IRQSEL6:1;
		};
	};
} DMAxREQ;

typedef struct
{
	Bool InUse;
	UInt8 Channel;
	
	// DMA Registers
	DMAxCON* ConBits;
	DMAxREQ* ReqBits;
	UInt16* PeriphAddr;
	void** StartAddrA;
	void** StartAddrB;
	UInt16* BufLen;
	
	UInt8* CurrentBuffer;
	UInt16 CurrentBufferLen;
	
} DMAInternal;

void InitDMA(void);

#endif
