#include "..\Z-OS.h"
#include <UART.h>

Int16 SerialPort1Id, SerialPort2Id;
Int16 DataTimeout = 1000;

Int16 SerialPortRead(Int16 id, UInt64 pos, UInt8* buffer, UInt16 bufLen)
{
	if (id == SerialPort1Id)
	{
	    int wait = 0;
	    Int16* temp_ptr = (Int16*) buffer;
	
	    while(bufLen)                         /* read till length is 0 */
	    {
	        while(!DataRdyUART1())
	        {
	            if(wait < DataTimeout)
	                wait++ ;                  /*wait for more data */
	            else
	                return ErrorTimeout;           /*Time out- Return words/bytes to be read */
	        }
	        wait = 0;
	        if(U1MODEbits.PDSEL == 3)         /* check if TX/RX is 8bits or 9bits */
	            *temp_ptr++ = U1RXREG;          /* data word from HW buffer to SW buffer */
			else
	            *buffer++ = U1RXREG & 0xFF; /* data byte from HW buffer to SW buffer */
	
	        bufLen--;
	    }
	    return ErrorSuccess;
	}
	else if (id == SerialPort2Id)
	{
	    int wait = 0;
	    Int16* temp_ptr = (Int16*) buffer;
	
	    while(bufLen)                         /* read till length is 0 */
	    {
	        while(!DataRdyUART2())
	        {
	            if(wait < DataTimeout)
	                wait++ ;                  /*wait for more data */
	            else
	                return ErrorTimeout;           /*Time out- Return words/bytes to be read */
	        }
	        wait = 0;
	        if(U2MODEbits.PDSEL == 3)         /* check if TX/RX is 8bits or 9bits */
	            *temp_ptr++ = U2RXREG;          /* data word from HW buffer to SW buffer */
		else
	            *buffer++ = U2RXREG & 0xFF; /* data byte from HW buffer to SW buffer */
	
	        bufLen--;
	    }
	    return ErrorSuccess;
	}
	return ErrorUnknown;
}

Int16 SerialPortWrite(Int16 id, UInt64 pos, UInt8* buffer, UInt16 bufLen)
{
	if (id == SerialPort1Id)
	{
		Int16* temp_ptr = (Int16*)buffer;
		
	    if(U1MODEbits.PDSEL == 3)        // check if TX is 8bits or 9bits
	    {
	        while(bufLen) 
	        {
	            while(U1STAbits.UTXBF); // wait if the buffer is full
	            U1TXREG = *temp_ptr++;    // transfer data word to TX reg
	            bufLen--;
	        }
	    }
	    else
	    {
	        while(bufLen)
	        {
	            while(U1STAbits.UTXBF);  // wait if the buffer is full
	            U1TXREG = *buffer++;   // transfer data byte to TX reg
	            bufLen--;
	        }
	    }
	    return ErrorSuccess;
	}
	else if (id == SerialPort2Id)
	{
		Int16* temp_ptr = (Int16*) buffer;
	
	    if(U2MODEbits.PDSEL == 3)        // check if TX is 8bits or 9bits
	    {
	        while(bufLen) 
	        {
	            while(U2STAbits.UTXBF); // wait if the buffer is full
	            U2TXREG = *temp_ptr++;    // transfer data word to TX reg
	            bufLen--;
	        }
	    }
	    else
	    {
	        while(bufLen)
	        {
	            while(U2STAbits.UTXBF);  // wait if the buffer is full
	            U2TXREG = *buffer++;   // transfer data byte to TX reg
	            bufLen--;
	        }
	    }
	    return ErrorSuccess;
	}
	return ErrorUnknown;
}

Int16 SerialPortCommand(Int16 id, Int16 cmd, UInt8* buffer, UInt16 bufferLen)
{
	// Currently no commands are supported
	return ErrorSuccess;
}

void InitSerialPorts(void)
{
	DeviceInfo info = {0};
	DeviceFuncs funcs = {0};
	
	info.CanMount = False;
	info.CanRead = True;
	info.CanSeek = False;
	info.CanWrite = True;
	info.UseCacheManager = False;
	
	funcs.Read = SerialPortRead;
	funcs.Write = SerialPortWrite;
	funcs.Command = SerialPortCommand;
	
	RegisterDevice("SerialPort1",info,funcs,&SerialPort1Id);
	RegisterDevice("SerialPort2",info,funcs,&SerialPort2Id);
}
