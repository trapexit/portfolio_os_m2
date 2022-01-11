/* @(#) lowlevelio.c 96/08/28 1.25 */

/********************************************************************************
*
*	lowlevelio.c
*
*	This file contains the low level, hardware dependent I/O support
*	routines used by the bootcode to setup and support I/O services
*	for itself and for the rest of the system.
*
********************************************************************************/

#include <hardware/cde.h>
#include <hardware/debugger.h>
#include <bootcode/bootglobals.h>
#include <bootcode/boothw.h>
#include "bootcode.h"

extern bootGlobals BootGlobals;


/********************************************************************************
*
*	MayGetChar
*
*	This routine attempts to read a character from the appropriate input
*	source.  If BUILD_DEBUGGER is defined then input via this routine is
*	disabled and MayGetChar returns -1.  If no terminal is connected to
*	the diagnostic serial port, MayGetChar returns -1.  Otherwise input is
*	taken from the port using the following method:
*
*		- If serial port read done bit isn't set then no character is
*		  available so set character to -1
*		- Otherwise read character from the serial port read register
*		  AND clear the serial port read done bit in preparation for
*		  receiving the next character (this really should be automatic
*		  whenever a character is read - sigh)
*		- Return character
*
*	Input:  None
*	Output: Character that was read, or -1 if no character available
*	Calls:	None
*
********************************************************************************/

int MayGetChar(void) {

#ifndef	BUILD_DEBUGGER

	if (BootGlobals.bg_UseDiagPort) {

		int	character;

		if (!(CDE_READ(CDE_BASE,CDE_INT_STS) & CDE_SDBG_RD_DONE)) character = -1;
		else {
			character = CDE_READ(CDE_BASE, CDE_SDBG_RD) & 0xff;
			CDE_WRITE(CDE_BASE, CDE_INT_STC, CDE_SDBG_RD_DONE);
		}
		return(character);
	} else

#endif	/* ifndef BUILD_DEBUGGER */

	return -1;
}


/********************************************************************************
*
*	PutC
*
*	This routine sends a character to the appropriate output destination.
*	If BUILD_DEBUGGER is defined then output is sent to the Mac based
*	debugger using the following method:
*
*	- Disable interrupts, saving the initial setting so we can restore
*	  it later
*	- If the character is not NULL then add it to the storage buffer
*	- If the character is NULL or the storage buffer is full then send
*	  the buffer contents as follows:
*		Wait for the debugger to acknowledge any previous transfer
*		Copy the storage buffer to the transmit buffer
*		Flush the transmit buffer data from the CPU's data cache
*		Tell the debugger how many bytes are being sent
*		Reset the storage buffer by setting its bytecount to zero
*	- Reenable interrupts with the previous interrupt setting
*
*	Note that instead of checking the current buffer contents against
*	the buffer length, we check against the buffer length minus four.
*	This ensures that if we take an exception while waiting for the
*	debugger acknowledgement and that exception causes output to occur,
*	we won't overflow the buffer.
*
*	If a terminal is connected to the diagnostic serial port, output
*	is sent to the diagnostic serial port using the following method:
*
*	- If the character is \n change it to \r so that TTerm+
*	  will treat it properly
*	- Poll the serial port write done bit until set to make
*	  sure the previous character has been sent
*	- Clear that same done bit in preparation for sending the
*	  next character (this really should be automatic whenever
*	  a new character is written out - sigh)
*	- Write the next character to the serial port write register
*
*	Input:	Character to write
*	Output:	None
*	Calls:	DisableInterrupts
*		RestoreInterrupts
*
********************************************************************************/

void PutC(char character) {

#ifdef	BUILD_DEBUGGER
#define	MAX_CHARS 64

static  char            StorageBuffer[MAX_CHARS];
volatile char          *TransmitBuffer = ((DebuggerInfo *)DEBUGGERREGION)->dbg_PrintfPtr;
static  uint8           bytecount;
        uint32          interrupts;
        uint32          count;

	interrupts = DisableInterrupts();
	if (character) StorageBuffer[bytecount++] = character;

	if ((bytecount >= (MAX_CHARS - 4)) || !character) {

		/* the role of this loop is to exit with the debugger's mailbox
		 * register being cleared. This double loop does this, while also
		 * making sure that interrupts aren't being disabled for long
		 * periods of time. */

		while (!DEBUGGER_ACK()) {
			RestoreInterrupts(interrupts);
			while (!DEBUGGER_ACK());
			interrupts = DisableInterrupts();
		}

		for (count = 0; count < bytecount; count++) TransmitBuffer[count] = StorageBuffer[count];
		FlushDCache((uint32)TransmitBuffer, MAX_CHARS);
		DEBUGGER_SENDCHARS(bytecount);
		bytecount = 0;
	}
	RestoreInterrupts(interrupts);

#else	/* ifdef BUILD_DEBUGGER */

	if (BootGlobals.bg_UseDiagPort) {
		if (character == '\n') character = '\r';
		while(!(CDE_READ(CDE_BASE,CDE_INT_STS) & CDE_SDBG_WRT_DONE));
		CDE_WRITE(CDE_BASE, CDE_INT_STC, CDE_SDBG_WRT_DONE);
		CDE_WRITE(CDE_BASE, CDE_SDBG_WRT, character);
	}

#endif	/* ifdef BUILD_DEBUGGER */

}


/********************************************************************************
*
*	InitIO
*
*	This routine performs all necessary initialization to enable low level
*	I/O.  If BUILD_DEBUGGER is defined then I/O is handled via the debugger,
*	for which no initialization is necessary.  Otherwise the diagnostic
*	serial port is initialized using the following method:
*
*		- Set serial port baud rate to 38,400
*		- Prime the output path by sending #
*		- Use PutC to send a carriage return
*
*	If BUILD_DEBUGGER is undefined then we check to see if any input
*	has been received via the serial port.  If yes, then a flag is set
*	indicating this, otherwise the flag is cleared.  The system only
*	sends output to the serial port if the flag is set.  This scheme
*	means that in a ROM based system, there will be no diagnostic I/O
*	unless a key press is detected during the very early stages of the
*	power-on sequence (before the display of the boot image).
*
*	Note that we set up the baud rate even if we don't detect anything
*	connected to the port so that the OS can later use the port if it
*	detects a connection.
*
*	Input:	None
*	Output:	None
*	Calls:	PutC
*
********************************************************************************/

void InitIO(void) {

#ifndef	BUILD_DEBUGGER

	uint32	BusClkSpeed;

#ifdef	BUILD_HARDCODED_BUSCLK
	BusClkSpeed = 25000000;
#else	/* ifdef BUILD_HARDCODED_BUSCLK */
	BusClkSpeed = BootGlobals.bg_BusClock;
#endif	/* ifdef BUILD_HARDCODED_BUSCLK */

	CDE_WRITE(CDE_BASE, CDE_SDBG_CNTL, ((BusClkSpeed/(4*9600))<<4) );
	CDE_WRITE(CDE_BASE, CDE_SDBG_WRT, '#');
	PutC('\r');

	if (CDE_SDBG_RD_DONE & CDE_READ(CDE_BASE, CDE_INT_STS)) BootGlobals.bg_UseDiagPort = 1;
	else BootGlobals.bg_UseDiagPort = 0;

#endif	/* ifndef BUILD_DEBUGGER */


}


/********************************************************************************
*
*	PrintString
*
*	This routine prints a string.
*
*	Input:	Pointer to string
*	Output:	None
*	Calls:	PutC
*
********************************************************************************/

void PrintString(const char *string) {

	char	character;

	while (character = *string++) PutC(character);

#ifdef BUILD_DEBUGGER
	PutC(0);		/* flush data out */
#endif	/* ifdef BUILD_DEBUGGER */

}


/********************************************************************************
*
*	PrintHexNum
*
*	This routine prints a 32 bit number in hex format.
*
*	Input:	32 bit number
*	Output:	None
*	Calls:	PrintString
*
********************************************************************************/

void PrintHexNum(ulong number) {

	int	digit,
		character;

	char	buffer[12];

	buffer[0] = '0';
	buffer[1] = 'x';

	for (digit = 0; digit < 8; digit++) {
		character = number >> (28 - digit * 4);
		character &= 15;
		if (character < 10) character += '0';
		else character = character + 'a' - 10;
		buffer[digit+2] = character;
	}
	buffer[digit+2] = 0;
	PrintString(buffer);
}


