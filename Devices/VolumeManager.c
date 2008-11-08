#include "..\Z-OS.h"
#include "VolumeManager.h"

PartitionTableEntry* EntryFromPartitionIndex(PartitionTable* table, Int8 i)
{
	switch (i)
	{
		case 0:
		return &(table->Partition0);
		case 1:
		return &(table->Partition1);
		case 2:
		return &(table->Partition2);
		default:
		return &(table->Partition3);
	}
}
	
Int16 DebugPartitionTable(PartitionTable* table)
{
	int i;
	int num = GetNumPartitions(table);
	printf("Partition table has %d partitions:\r\n",num);
	for (i = 0; i < num; i++)
	{
		PartitionTableEntry* entry = EntryFromPartitionIndex(table,i);
		printf("Partition %d: sector %lu, %lu sectors\r\n",i,entry->FirstSector,entry->NumSectors);
	}
	puts("\r\n");
	return ErrorSuccess;
}

Int16 GetNumPartitions(PartitionTable* table)
{
	if (!table) return 0;
	if (!table->Partition0.BootDes == 0x80) return 0;
	if (!table->Partition1.BootDes == 0x80) return 1;
	if (!table->Partition2.BootDes == 0x80) return 2;
	if (!table->Partition3.BootDes == 0x80) return 3;
	return 4;
}

Int16 LoadMBR(DeviceInternal* dev, PartitionTable* table)
{
    PartitionTable Partition;
    Int16 ret;
	
	// assign it the partition table structure
	if ((ret = InternalReadDevice(dev, 0, (UInt8*)&Partition, sizeof(PartitionTable), False))) return ret;
	
	// Ensure its good
	if((Partition.Signature0 != 0x55) || (Partition.Signature1 != 0xAA))
	{
	    return ErrorInvalidPartition;
	}
	else
	{    
		return ErrorSuccess;
	} 
}

Int16 GuessPartition(PartitionTableEntry table, char* hint)
{
	switch (table.FSDesc)
    {
	    // FAT12
		case 0x01:
		
		// FAT16
		case 0x04:
		case 0x06:
		case 0x0E:
		
		// FAT32
		case 0x0B:
		case 0x0C:
			
			strcpy(hint,"FAT");
			return ErrorSuccess;
			break;
		default:
			return ErrorSuccess;
    }
}
