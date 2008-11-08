#ifndef _DMA_DEVICE_HEADER_
#define _DMA_DEVICE_HEADER_

typedef enum
{
	Byte,
	Word
} DMASize;

typedef enum
{
	Force			= 0b00000001,
	NullDataWrite	= 0b00000010,
} DMAFlags;

typedef struct
{
	Int16 (*SetSize)(UInt16 handle, DMASize size);
	Int16 (*GetSize)(UInt16 handle, DMASize* size);
	Int16 (*SetDevice)(UInt16 handle, UInt16 device);
	Int16 (*GetDevice)(UInt16 handle, UInt16* device);
	Int16 (*SetFlags)(UInt16 handle, DMAFlags flags);
	Int16 (*GetFlags)(UInt16 handle, DMAFlags* flags);
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
	UInt8 Channel;
	
	// DMA configuration
	DMAxCON* ConBits;
	DMAxREQ* ReqBits;
	UInt16* PeriphAddr;
	void** StartAddr;
	UInt16* BufLen;
	
	Bool IsTransmitting;
} DMAInternal;

void InitDMA(void);

#endif
