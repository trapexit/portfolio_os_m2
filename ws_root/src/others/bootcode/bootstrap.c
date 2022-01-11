/* @(#) bootstrap.c 96/09/25 1.148 */

/********************************************************************************
*
*	bootstrap.c
*
*	This file contains the C based portions of the ROM reset handlers.
*
*	At this point the system memory map looks like:
*
*	0000	+-------------------+
*	    	|    exception	    |
*		|    table (8k)     |
*	2000	+-------------------+
*		| bootcode data/bss |
*		|  ...gap...        |
*		| boot stack        |
*	3000	+-------------------+
*
*	(Addresses are offsets from start of RAM)
*
********************************************************************************/

#include <hardware/PPC.h>
#include <hardware/PPCasm.h>
#include <hardware/bda.h>
#include <hardware/cde.h>
#include <hardware/debugger.h>
#include <kernel/sysinfo.h>
#include <kernel/types.h>
#include <bootcode/bootglobals.h>
#include <bootcode/boothw.h>
#include "bootcode.h"

bootGlobals	BootGlobals;
uint32		BootSavedBATs[16];	/* area to save bats in bootstrap.s */


/********************************************************************************
*
*	InitBootGlobals
*
*	This routine is responsible for initializing most (but not all) of
*	the BootGlobals data structure.
*
*	Note that this routine may be called in either the hard reset case
*	(from BootStrap()) OR the soft reset case (from CleanupForDipir()).
*	The value of the CDE_BBLOCK_EN register is used to identify these
*	cases so that only the appropriate fields are initialized.
*
*	Input:	None
*	Output:	None
*	Calls:	MemorySize
*
********************************************************************************/

void InitBootGlobals(void) {

#define HARD_RESET		(CDE_READ(CDE_BASE, CDE_BBLOCK_EN) & CDE_BLOCK_CLEAR)

	uint32	temp;

	BootGlobals.bg_BootGlobalsVersion = BG_INITIAL_VERSION;

/********************************************************
*
*	Hardware related information
*
********************************************************/

	if (_mfsr14() == WHOAMI_SINGLECPU) {
		BootGlobals.bg_NumCPUs = 1;
		_mtsr14(WHOAMI_MASTERCPU);
	} else BootGlobals.bg_NumCPUs = 2;

	BootGlobals.bg_SystemRAM.mr_Start = DRAMSTART;
	BootGlobals.bg_SystemRAM.mr_Size = MemorySize();
	BootGlobals.bg_SystemROM.mr_Start = SYSROMSTART;
	BootGlobals.bg_SystemROM.mr_Size = SystemROMSize;
	if (HARD_RESET) {
		InitBootAllocs();
	}

/********************************************************
*
*	System configuration (from SYSCONF in CDE)
*
********************************************************/

	temp = ~CDE_READ(CDE_BASE,CDE_SYSTEM_CONF);
	BootGlobals.bg_AudioConfig = (temp & SC_AUDIO_CONFIG_BITS) >> SC_AUDIO_CONFIG_SHIFT;
	BootGlobals.bg_SystemType = (temp & SC_SYSTEM_TYPE_BITS) >> SC_SYSTEM_TYPE_SHIFT;
	BootGlobals.bg_DefaultLanguage = (temp & SC_DEFAULT_LANGUAGE_BITS) >> SC_DEFAULT_LANGUAGE_SHIFT;
	switch (BootGlobals.bg_DefaultLanguage) {
		case SC_DEFAULT_LANGUAGE_US_ENG:
			BootGlobals.bg_DefaultLanguage = SYSINFO_INTLLANG_USENGLISH;
			break;
		case SC_DEFAULT_LANGUAGE_JAP:
			BootGlobals.bg_DefaultLanguage = SYSINFO_INTLLANG_JAPANESE;
			break;
		case SC_DEFAULT_LANGUAGE_UK_ENG:
			BootGlobals.bg_DefaultLanguage = SYSINFO_INTLLANG_UKENGLISH;
			break;
		default:
			BootGlobals.bg_DefaultLanguage = 0xFF;
			break;
	}
	BootGlobals.bg_VideoEncoder = (temp & SC_VIDEO_ENCODER_BITS) >> SC_VIDEO_ENCODER_SHIFT;
	BootGlobals.bg_VideoMode = (temp & SC_VIDEO_MODE_BITS) >> SC_VIDEO_MODE_SHIFT;
	if (BootGlobals.bg_VideoEncoder == BG_VIDEO_ENCODER_BT9103) {
		if (BootGlobals.bg_VideoMode == SC_VIDEO_MODE_3DO_PAL)
			BootGlobals.bg_VideoMode = BG_VIDEO_MODE_PAL;
		else BootGlobals.bg_VideoMode = BG_VIDEO_MODE_NTSC;
	} else if (BootGlobals.bg_VideoEncoder == BG_VIDEO_ENCODER_VP536) {
		if (BootGlobals.bg_VideoMode == SC_VIDEO_MODE_VP536_PAL)
			BootGlobals.bg_VideoMode = BG_VIDEO_MODE_PAL;
		else BootGlobals.bg_VideoMode = BG_VIDEO_MODE_NTSC;
	} else {
		if (BootGlobals.bg_VideoMode == SC_VIDEO_MODE_BT9103_PAL)
			BootGlobals.bg_VideoMode = BG_VIDEO_MODE_PAL;
		else BootGlobals.bg_VideoMode = BG_VIDEO_MODE_NTSC;
	}

/********************************************************
*
*	Device and volume related information
*
********************************************************/

	if (HARD_RESET) {
		BootGlobals.bg_RomAppDevice = 0;
		BootGlobals.bg_DevicePerms = 0;
		BootGlobals.bg_ROMVolumeLabel = (ExtVolumeLabel *)ROMVOLUMELABEL;
	}

/********************************************************
*
*	Boot video related information
*
********************************************************/

	if (BootGlobals.bg_VideoMode == BG_VIDEO_MODE_PAL) {
		BootGlobals.bg_ScrnLines = BV_PAL_HEIGHT;
		BootGlobals.bg_LinePixels = BV_PAL_WIDTH;
		BootGlobals.bg_BlankLines = BV_PAL_BLANK;
		if (BootGlobals.bg_VideoEncoder == BG_VIDEO_ENCODER_BT9103) {
			BootGlobals.bg_HStart = BV_HSTART_BT9103_PAL;
		} else if (BootGlobals.bg_VideoEncoder == BG_VIDEO_ENCODER_VP536) {
			BootGlobals.bg_HStart = BV_HSTART_VP536_PAL;
		} else {
			BootGlobals.bg_HStart = BV_HSTART_3DO_PAL;
		}
	} else {
		BootGlobals.bg_ScrnLines = BV_NTSC_HEIGHT;
		BootGlobals.bg_LinePixels = BV_NTSC_WIDTH;
		BootGlobals.bg_BlankLines = BV_NTSC_BLANK;
		if (BootGlobals.bg_VideoEncoder == BG_VIDEO_ENCODER_BT9103) {
			BootGlobals.bg_HStart = BV_HSTART_BT9103_NTSC;
		} else if (BootGlobals.bg_VideoEncoder == BG_VIDEO_ENCODER_VP536) {
			BootGlobals.bg_HStart = BV_HSTART_VP536_NTSC;
		} else {
			BootGlobals.bg_HStart = BV_HSTART_3DO_NTSC;
		}
	};

	BootGlobals.bg_PixelBytes = BV_PIXEL_BYTES;
	BootGlobals.bg_LineBytes = BootGlobals.bg_LinePixels * BootGlobals.bg_PixelBytes;

	BootGlobals.bg_DisplaySize = RNDUP_TOPAGE(BootGlobals.bg_ScrnLines * BootGlobals.bg_LineBytes);

	if (HARD_RESET) {
		BootGlobals.bg_DisplayPointer = BootAllocate(BootGlobals.bg_DisplaySize, BA_GRAPHICS);
	}

/********************************************************
*
*	Dipir related information
*
********************************************************/

	if (HARD_RESET) {
		(void) BootAllocate(DIPIRTEMPSIZE, BA_DIPIR_PRIVATE);
		BootGlobals.bg_DipirFlags = 0;
		BootGlobals.bg_DipirControl = 0;
	}

/********************************************************
*
*	OS related information
*
********************************************************/

	if (HARD_RESET) {
		BootGlobals.bg_KernelAddr = 0;
		BootGlobals.bg_KernelVersion = 0;
		BootGlobals.bg_UseDiagPort = 0;
	}

/********************************************************
*
*	Exported routines
*
********************************************************/

	BootGlobals.bg_PutC = PutC;
	BootGlobals.bg_MayGetChar = MayGetChar;
	BootGlobals.bg_PerformSoftReset = PerformSoftReset;
	BootGlobals.bg_CleanupForNewOS = CleanupForNewOS;
	BootGlobals.bg_AddBootAlloc = AddBootAlloc;
	BootGlobals.bg_DeleteBootAlloc = DeleteBootAlloc;
	BootGlobals.bg_VerifyBootAlloc = VerifyBootAlloc;

}


/********************************************************************************
*
*	SetupExceptionTable
*
*	This routine creates an exception table in RAM.
*
*	Input:	None
*	Output:	None
*	Calls:	MemoryMove
*		FlushDCache
*		InvalidateICache
*
********************************************************************************/

void SetupExceptionTable(void) {

	uint32	pattern,
		source,
		target,
		length;

#ifdef	BUILD_DEBUGGER
	pattern = 0x11;
#else	/* ifdef BUILD_DEBUGGER */
	pattern = 0xEE;
#endif	/* ifdef BUILD_DEBUGGER */
	target = VECTORSTART;
	length = VECTORSIZE;
	MemorySet(pattern, target, length);

	source = (uint32)&ExceptionPreamble;
	target = VECTORSTART - ((uint32)&ExceptionStart - (uint32)&ExceptionPreamble);
	length = (uint32)&ExceptionEnd - (uint32)&ExceptionPreamble;
	for (target += 0x200; target < VECTORSTART + VECTORSIZE - 0x100; target += 0x100) {
		MemoryMove(source, target, length);
		FlushDCache(target, length);
	}
	InvalidateICache();
}


/********************************************************************************
*
*	PrintBootInfo
*
*	This routine prints out various boot related information.
*
*	Input:	None
*	Output:	None
*	Calls:	PrintString
*		PRINTVALUE
*
********************************************************************************/

static void PrintBootInfo(void) {

	uint32 caches = _mfhid0();

	PrintString("\n\nM2 alive...\n\n");

	PRINTVALUE("BusClk ", BootGlobals.bg_BusClock);
	PRINTVALUE("CPUClk ", BootGlobals.bg_CPUClock);
	PRINTVALUE("CPUs   ", BootGlobals.bg_NumCPUs);
	PRINTVALUE("ICache ", caches & HID_ICE);
	PRINTVALUE("DCache ", caches & HID_DCE);
	PRINTVALUE("RAMSz  ", BootGlobals.bg_SystemRAM.mr_Size);
	PRINTVALUE("ROMSz  ", BootGlobals.bg_SystemROM.mr_Size);
	PRINTVALUE("*Disp  ", (ulong)(BootGlobals.bg_DisplayPointer));
	PRINTVALUE("*VDL0  ", (ulong)&(BootGlobals.bg_BootVDL0));
	PRINTVALUE("*VDL1  ", (ulong)&(BootGlobals.bg_BootVDL1));
	PRINTVALUE("MCfg   ", (ulong)BDA_READ(BDAMCTL_MCONFIG));

	PRINTVALUE("SysCfg ", CDE_READ(CDE_BASE,CDE_SYSTEM_CONF));
	PrintString(" ");
	switch (BootGlobals.bg_SystemType) {
		case SYSINFO_SYSTYPE_PLAYER:
			PrintString("Multiplayer");
			break;
		case SYSINFO_SYSTYPE_UPGRADE:
			PrintString("Upgrade");
			break;
		case SYSINFO_SYSTYPE_DEVCARD:
			PrintString("DevCard");
	}
	PrintString("\n ");
	switch (BootGlobals.bg_DefaultLanguage) {
		case SYSINFO_INTLLANG_USENGLISH:
			PrintString("US");
			break;
		case SYSINFO_INTLLANG_GERMAN:
			PrintString("Ger");
			break;
		case SYSINFO_INTLLANG_JAPANESE:
			PrintString("Jap");
			break;
		case SYSINFO_INTLLANG_SPANISH:
			PrintString("Spain");
			break;
		case SYSINFO_INTLLANG_ITALIAN:
			PrintString("Italy");
			break;
		case SYSINFO_INTLLANG_CHINESE:
			PrintString("China");
			break;
		case SYSINFO_INTLLANG_KOREAN:
			PrintString("Kor");
			break;
		case SYSINFO_INTLLANG_FRENCH:
			PrintString("Fra");
			break;
		case SYSINFO_INTLLANG_UKENGLISH:
			PrintString("UK");
			break;
		case SYSINFO_INTLLANG_AUSENGLISH:
			PrintString("Aus");
			break;
		case SYSINFO_INTLLANG_MEXSPANISH:
			PrintString("Mex");
			break;
		case SYSINFO_INTLLANG_CANENGLISH:
			PrintString("Can");
	}
	PrintString("\n ");
	switch (BootGlobals.bg_AudioConfig) {
		case BG_AUDIO_CONFIG_ASAHI:
			PrintString("Asahi");
			break;
		case BG_AUDIO_CONFIG_CS4216:
			PrintString("CS4216");
	}
	PrintString("\n ");
	switch (BootGlobals.bg_VideoEncoder) {
		case BG_VIDEO_ENCODER_3DO:
			PrintString("DENC");
			break;
		case BG_VIDEO_ENCODER_BT9103:
			PrintString("BT9103");
			break;
		case BG_VIDEO_ENCODER_VP536:
			PrintString("VP536");
	}
	PrintString("\n ");
	switch (BootGlobals.bg_VideoMode) {
		case BG_VIDEO_MODE_PAL:
			PrintString("PAL");
			break;
		case BG_VIDEO_MODE_NTSC:
			PrintString("NTSC");
	}
	PrintString("\n\n");

}


/********************************************************************************
*
*	BootStrap
*
*	This routine is the C based portion of the hard reset handler.  At this
*	point in the boot process the caches are on and interrupts are still
*	disabled.
*
*	Setup of the diagnostic serial port requires knowledge of the bus
*	clock speed, which is derived from the video clocks.  Therefore
*	InitIO and PrintBootInfo are not called until after SetupBootVideo
*	(which calls GetClockSpeeds).
*
*	Input:	None
*	Output:	None
*	Calls:	SetupExceptionTable
*		InitBootGlobals
*		SetupBootVideo
*		InitIO
*		PrintBootInfo
*		TestDCache (only if TESTDCBI is defined)
*		PerformSoftReset
*		PrintString
*
********************************************************************************/

void Bootstrap(void) {

	SetupExceptionTable();	/* Set up a RAM exception table		*/
	InitBootGlobals();	/* Initialize boot globals		*/
	SetupBootVideo();	/* Set up and turn on video display	*/
	InitIO();		/* Initialize low level I/O services	*/
	PrintBootInfo();	/* Print out boot relate information	*/

#ifdef TESTDCBI
	PRINTVALUE("MSR=",_mfmsr());
	PRINTVALUE("HID0=",_mfhid0());
	TestDCache();
#endif

	for (;;) {
		PerformSoftReset();	/* Clear BBLOCK_EN and launch dipir */
		PrintString("No OS launched; retrying...\r");
	}
}

