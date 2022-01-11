/* @(#) launchdipir.c 96/09/25 1.17 */

/********************************************************************************
*
*	launchdipir.c
*
*	This file contains the routines which find and launch dipir.
*
********************************************************************************/

#include <string.h>
#include <dspptouch/dspp_addresses.h>
#include <dspptouch/touch_hardware.h>
#include <hardware/PPC.h>
#include <hardware/PPCasm.h>
#include <hardware/cde.h>
#include <kernel/types.h>
#include <kernel/interrupts.h>
#include <bootcode/bootglobals.h>
#include <bootcode/boothw.h>
#include "bootcode.h"

extern	bootGlobals	BootGlobals;


/********************************************************************************
*
*       FindROMTag
*
*       This routine finds a specified ROM tag in a specified ROM tag table
*       and returns a pointer to it.  If the ROM tag is not found then zero
*       is returned.
*
*       Input:  Pointer to ROM tag table
*               Number of tags in table
*               Subsystype of desired tag
*               Type of desired tag
*       Output: Pointer to tag, or zero if not found
*       Calls:  None
*
********************************************************************************/

RomTag *FindROMTag(RomTag *pROMTag, uint32 NumTags, uint8 SubSysType, uint8 Type) {

        if (pROMTag != 0) while (NumTags > 0) {
                if (pROMTag->rt_SubSysType == SubSysType && pROMTag->rt_Type == Type)
                        return pROMTag;
                pROMTag++;
                NumTags--;
        }
        return 0;
}


/********************************************************************************
*
*	InitDipir
*
*	This routine takes care of initializing dipir related data areas.
*
*	Input:	None
*	Output:	None
*	Calls:	FindROMTag
*		MemoryMove
*		MemorySet
*
********************************************************************************/

static void InitDipir(void) {

	ExtVolumeLabel		*pVolumeLabel;

	RomTag			*pROMTagTable,
				*pDipirROMTag;

	uint32			NumROMTags,
				source,
				target,
				length;

	pVolumeLabel = BootGlobals.bg_ROMVolumeLabel;
	pROMTagTable = (RomTag *)((uint32)pVolumeLabel + sizeof(ExtVolumeLabel));
	NumROMTags = pVolumeLabel->dl_NumRomTags;

	pDipirROMTag = FindROMTag(pROMTagTable, NumROMTags, RT_SUBSYS_ROM, ROM_DIPIR);
	if (!pDipirROMTag) {
		DBUG("Error, can't find dipir ROM tag\r");
		while (1);
	}

	BootGlobals.bg_DipirControl = pDipirROMTag->rt_Flags;

	/* if hard reset, init data/bss */
	if (!(CDE_READ(CDE_BASE, CDE_VISA_DIS) & CDE_NOT_1ST_DIPIR)) {
		DBUG("Moving data section to RAM\r");
		source = (uint32)pROMTagTable +
			pDipirROMTag->rt_Offset * pVolumeLabel->dl_VolumeBlockSize +
			pDipirROMTag->rt_DataOffset;
		target = pDipirROMTag->rt_DataAddress;
		length = pDipirROMTag->rt_Size - pDipirROMTag->rt_DataOffset;
		DBUGPV("  Source:  ", source);
		DBUGPV("  Target:  ", target);
		DBUGPV("  Length:  ", length);
		MemoryMove(source, target, length);

		DBUG("Clearing RAM data/bss area\r");
		target = pDipirROMTag->rt_BSSAddress;
		length = pDipirROMTag->rt_BSSSize;
		DBUGPV("  Target:  ", target);
		DBUGPV("  Length:  ", length);
		MemorySet(0, target, length);
	}
}


/********************************************************************************
*
*	CleanupForDipir
*
*	This routine takes care of any cleanup necessary to allow dipir to run
*	safely.
*
*	Input:	None
*	Output:	None
*	Calls:	InitBootGlobals
*
********************************************************************************/

static void CleanupForDipir(void) {
	InitBootGlobals();
}


/********************************************************************************
*
*	LaunchDipir
*
*	This routine finds dipir in ROM, initializes it's RAM data structures,
*	and launches it.
*
*	Input:	None
*	Output:	None
*	Calls:	CleanupForDipir
*		InitDipir
*		DIPIRCODESTART
*
********************************************************************************/

int32 LaunchDipir(void) {

	DBUG("... Entering LaunchDipir\r");
	CleanupForDipir();

	InitDipir();
	DBUGPV("... Calling dipir at ", DIPIRCODESTART);
	return (*(DipirFunction *)DIPIRCODESTART)(QueryROMSysInfo);
}


/********************************************************************************
*
*	CleanupForNewOS
*
*	This routine cleans up the system in order to prepare for the launch
*	of a new OS by dipir.  Currently this consists of:
*
*	- Making sure the DSP is properly disabled.
*
*	- Setting up the boot video display, which also handles clearing
*	  various memory ranges to a know state.
*
*	- Setting up bg_DipirFlags and bg_NumIcons.
*
*	- Reinitializing the RAM vector table.
*
*	Input:	None
*	Output:	None
*	Calls:	SetupBootVideo
*
********************************************************************************/

void CleanupForNewOS(void) {

	/* Set the audio hardware to a clean state */
	WriteHardware( DSPX_CONTROL, 0 ); /* Halt DSP */
	WriteHardware( DSPX_RESET, (DSPX_F_RESET_DSPP | DSPX_F_RESET_INPUT | DSPX_F_RESET_OUTPUT) );
	WriteHardware( DSPX_INTERRUPT_DISABLE, 0xFFFFFFFF );
	WriteHardware( DSPX_CHANNEL_DISABLE, 0xFFFFFFFF ); /* DMA Off */

	SetupBootVideo();

	BootGlobals.bg_DipirFlags &= ~DF_VIDEO_INUSE;
	BootGlobals.bg_NumIcons = 0;

	SetupExceptionTable();
}

