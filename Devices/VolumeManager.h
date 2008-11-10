#ifndef _IO_VOLUMEMAN_HEADER_
#define _IO_VOLUMEMAN_HEADER_

typedef struct
{
	__attribute__ ((packed)) UInt8	BootDes;		// Boot Descriptor, 0x80
	__attribute__ ((packed)) Int16	FirstSector16;	// First Partion Sector
	__attribute__ ((packed)) UInt8	FSDesc;			// File System Descriptor 
	__attribute__ ((packed)) Int16	LastPartSect;	// Last Partion Sector
	__attribute__ ((packed)) UInt32	FirstSector;	// First Sector Position
	__attribute__ ((packed)) UInt32	NumSectors;		// Number of Sectors in partion
} PartitionTableEntry;

typedef struct
{
	__attribute__ ((packed)) UInt8					ConsChkRtn[512 - 2 - 4 * sizeof(PartitionTableEntry)];
	__attribute__ ((packed)) PartitionTableEntry	Partition0;
	__attribute__ ((packed)) PartitionTableEntry	Partition1;
	__attribute__ ((packed)) PartitionTableEntry	Partition2;
	__attribute__ ((packed)) PartitionTableEntry	Partition3;
	__attribute__ ((packed)) UInt8					Signature0; // 0x55
	__attribute__ ((packed)) UInt8					Signature1; // 0xAA
} PartitionTable;

Int16 GetNumPartitions(PartitionTable* table);
Int16 LoadMBR(DeviceInternal* dev, PartitionTable* table);
Int16 GuessPartition(PartitionTableEntry table, char* hint);

#endif
