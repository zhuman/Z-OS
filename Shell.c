#include "Z-OS.h"
#include "Shell.h"
#include <uart.h>

extern List Threads;
#define ASM_INST(x) {__asm__ volatile (x);}

Int8 CompareCmd(UInt8* buffer, char* cmd)
{
	Int16 i = 0;
	while (buffer[i] == cmd[i])
	{
		i++;
		if (cmd[i] == 0 || cmd[i] == '\r' || cmd[i] == '\n') return True;
	}
	return False;
}

void ShellThreadProc(void* arg)
{
	UInt8 buffer[17] = {0};
	UInt8* pBuf = (UInt8*)buffer;
	for(;;)
	{
		int val;
		if (!DataRdyUART1()) continue;
		val = getcUART1(); //(5, (unsigned int*)buffer, 65535);
		if ((*(pBuf++) = (UInt8)val) != '\n')
		{
			if (pBuf >= (UInt8*)&(buffer[16]))
			{
				puts("Invalid command.\r\n");
				pBuf = (UInt8*)buffer;
			}
		}
		else
		{
			pBuf = (UInt8*)buffer;
			if (CompareCmd(pBuf,"info") || CompareCmd(pBuf, "ver"))
			{
				puts("Z-OS v0.1a\r\nA real-time multitasking operating system for the PIC 24H.\r\nCoded by Zachary Northrup\r\n");
			}
			else if (CompareCmd(pBuf,"listThreads"))
			{
				UInt16 i;
				puts("Running Threads:\r\n\r\n");
				EnterCriticalSection();
				for (i = 0; i < Threads.Length; i++)
				{
					ThreadInternal* thr = GetListItem(&Threads,i);
					InternalObject* obj;
					if (InternalObjectFromData(thr,&obj)) continue;
					if (obj->Name) printf("%s:\r\n",obj->Name);
					else puts("Unnamed Thread:\r\n");
					printf(" ID: %d\r\n Stack Bottom: 0x%x\r\n Stack Top: 0x%x\r\n Start Address: 0x%x\r\n State: ",
					 thr->ThreadID,
					  (int)thr->Stack,
					   (int)thr->StackPointer,
					    (int)thr->StartParams.StartFunc);
					switch (thr->State)
					{
						case Running: puts("Running"); break;
						case Queued: puts("Queued"); break;
						case Waiting: puts("Waiting"); break;
						case Suspended: puts("Suspended"); break;
						case Stopped: puts("Stopped"); break;
					}
					puts("\r\n\r\n");
				}
				ExitCriticalSection();
			}
			else if (CompareCmd(pBuf, "prgm"))
			{
				EnterCriticalSection();
				ASM_INST("mov #0x0000,W0");
				ASM_INST("mov W0,TBLPAG");
				ASM_INST("mov #0x0c00,W0");
				ASM_INST("mov #0xFFFF, W1");
				ASM_INST("tblwtl W1,[W0]");
				ASM_INST("tblwth W1,[W0]");
				ASM_INST("reset");
			}
			else if (CompareCmd(pBuf, "reset"))
			{
				ASM_INST("reset");
			}
			else if (CompareCmd(pBuf, "freeze"))
			{
				EnterCriticalSection();
			}
			else if (CompareCmd(pBuf, "unfreeze"))
			{
				ExitCriticalSection();
			}
			else
			{
				puts("Invalid command.\r\n");
			}
			memset(buffer,0,16);
		}
	}
}
