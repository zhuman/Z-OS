#include "p24Hxxxx.h"
#include <uart.h>
#include "Z-OS.h"

int __attribute__ ((__weak__)) main(void)
{
	return BootOS();
}

UInt16 __attribute__ ((persistent)) BootNum;

int BootOS(void)
{
	Int16 ret;
	AD1PCFGH = AD1PCFGL = AD2PCFGL = 0xFFFF;  // A/D pins ALL DIGITAL
	
	// Initialize INT1 as the "program" button
	INTCON2bits.INT1EP = 1;  // interrupt on falling edge
	IPC5bits.INT1IP = 7;     // highest priority interrupt
	IFS1bits.INT1IF = 0;
	IEC1bits.INT1IE = 1;     // interrupt enabled
		
	OpenUART1(0x8000, 0x400, 20);
	//OpenUART2(0x8000, 0x400, 20);
	puts("Welcome to Z-OS 1.2\r\nBooting...\r\n");
	printf("System boots: %d\r\n",BootNum++);
	
	//for(;;);
	
	Phase0Init();
	
	// Here we go...
	ret = InitializeThreading();
	
	// We will only get here if the thread manager was unable to initialize properly.
	printf("Fatal error initializing threading: 0x%x\r\nThe system cannot continue.\r\n",ret);
	for (;;);
	
	return 0;
}

// Called before the thread scheduler has been initialized.
void __attribute__((__weak__)) Phase0Init(void)
{
	// Register the various object types
	InitializeTimers();
	InitializeSync();
	InitializeQueues();
	InitializeIO();
	
	InitSerialPorts();
	//InitVirtualFileDevice();
	// Initialize other device drivers here
}

// Called after the thread manager has been initialized, but 
// before actual threading has started.
void __attribute__((__weak__)) Phase1Init(void)
{
	// Create the system worker threads and register the Delegate type.
	InitializeDelegates();
}

#define ASM_INST(x) {__asm__ volatile (x);}
void __attribute__((interrupt, no_auto_psv)) _INT1Interrupt(void)
{
	ASM_INST("mov #0x0000,W0");
	ASM_INST("mov W0,TBLPAG");
	ASM_INST("mov #0x0c00,W0");
	ASM_INST("mov #0xFFFF, W1");
	ASM_INST("tblwtl W1,[W0]");
	ASM_INST("tblwth W1,[W0]");	
	ASM_INST("reset");
}


