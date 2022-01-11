#ifndef __BOOTCODE_BOOTGLOBALS_H
#define __BOOTCODE_BOOTGLOBALS_H


/******************************************************************************
*
*	@(#) bootglobals.h 96/08/26 1.33
*
*	This file contains the definitions of the BootGlobals structure and
*	its various substructures.  BootGlobals is used as a repository for
*	hardware and boot related information which is used by dipir and the
*	OS, as well as by the bootcode itself.
*
******************************************************************************/


#include <file/discdata.h>
#include <hardware/bda.h>
#include <hardware/m2vdl.h>
#include <kernel/mem.h>
#include <kernel/sysinfo.h>

/* Defines for decoding bg_BootGlobalsVersion */
#define BG_INITIAL_VERSION	0			/* First released version of BootGlobals */

/* Defines for decoding bg_AudioConfig */
#define BG_AUDIO_CONFIG_ASAHI	0			/* Asahi DAC */
#define BG_AUDIO_CONFIG_CS4216	2			/* CS4216 CODEC */

/* Defines for decoding bg_VideoEncoder */
#define BG_VIDEO_ENCODER_3DO	0			/* 3DO DENC */
#define BG_VIDEO_ENCODER_BT9103	1			/* BT9103 (or BT851) */
#define BG_VIDEO_ENCODER_VP536	2			/* VP536 */

/* Defines for decoding bg_VideoMode */
#define BG_VIDEO_MODE_PAL	0			/* PAL */
#define BG_VIDEO_MODE_NTSC	1			/* NTSC */

/* Defines for decoding bg_DipirFlags */
#define	DF_EXPECT_DATADISC	0x00000001		/* App expects a datadisc now */
#define	DF_VIDEO_INUSE		0x00000002		/* Video can't be used by dipir */
#define	DF_HARD_RESET		0x00000004		/* 1 = hardreset, 0 = softreset */
#define	DF_NO_REBOOT		0x00000008		/* Don't reboot on media insertion */


/********************************************************************************
*
*	Miscellaneous substructures
*
********************************************************************************/

typedef uint16 VideoDist;				/* Distance on the screen in pixels */

typedef struct VideoPos { 				/* Position on the screen */
	VideoDist	x,				/* Horizontal coordinate */
			y;				/* Vertical coordinate */
} VideoPos;

typedef struct VideoRect {				/* Rectangle on the screen */
	VideoPos	LL,				/* Lower left corner */
			UR;				/* Upper right corner */
} VideoRect;

typedef struct bootVDL {				/* Video display list */
	VDLHeader	bv_TopBlank;			/* Entry for upper blank region */
	ShortVDL	bv_ActiveRegion;		/* Entry for active region */
	VDLHeader	bv_BottomBlank;			/* Entry for lower blank region */
} bootVDL;


#define	NUM_BOOT_ALLOCS	32


/********************************************************************************
*
*	bootGlobals
*
*	This structure is intended for use only by low level code, and may
*	change from one hardware platform to another.  Therefore all OS
*	references to information in BootGlobals should be made via SysInfo
*	calls.
*
********************************************************************************/

typedef struct bootGlobals {

	uint32		bg_BootGlobalsVersion;		/* Version of this structure */

/********************************************************
*
*	Hardware related information
*
********************************************************/

	uint32		bg_NumCPUs;			/* Number of CPUs in the system */
	uint8		bg_PBusDevs[MAX_PBUS_SLOTS];	/* List of PowerBus devices by slot */
	uint32		bg_BusClock;			/* Frequency of bus clock in Hz */
	uint32		bg_CPUClock;			/* Frequency of CPU clock in Hz */
	uint32		bg_TicksPerSec;			/* CPU decrementer ticks per second */
	MemRange	bg_SystemRAM;			/* Amount of RAM in system */
	MemRange	bg_SystemROM;			/* Amount of ROM in system */

/********************************************************
*
*	System configuration (from SYSCONF in CDE)
*
********************************************************/

	uint8		bg_AudioConfig;			/* Audio configuration */
	uint8		bg_SystemType;			/* System type */
	uint8		bg_DefaultLanguage;		/* Default system language */
	uint8		bg_VideoEncoder;		/* Video encoder */
	uint8		bg_VideoMode;			/* Video mode */

/********************************************************
*
*	Device and volume related information
*
********************************************************/

	uint32		bg_RomAppDevice;		/* Insertion ID of RomApp media */
	uint32		bg_DevicePerms;			/* Device permission mask */
	ExtVolumeLabel	*bg_ROMVolumeLabel;		/* Pointer to ROM volume label */
	void		*bg_OSVolumeLabel;		/* Pointer to OS volume label */
	void		*bg_AppVolumeLabel;		/* Pointer to application volume label */

/********************************************************
*
*	Boot video related information
*
********************************************************/

	uint32		*bg_DisplayPointer;		/* Pointer to boot framebuffer */
	uint32		bg_DisplaySize;			/* Size of framebuffer in bytes */
	VDLHeader	bg_NullVDL;			/* Null video VDL */
	bootVDL		bg_BootVDL0;			/* Boot video display list for field 0 */
	bootVDL		bg_BootVDL1;			/* Boot video display list for field 1 */
	uint32		bg_ScrnLines;			/* Lines per screen */
	uint32		bg_LinePixels;			/* Pixels per line */
	uint32		bg_PixelBytes;			/* Bytes per pixel */
	uint32		bg_LineBytes;			/* Bytes per line */
	uint32		bg_BlankLines;			/* Number of lines in blank area */
	uint32		bg_HStart;			/* Horizontal delay to active area */

/********************************************************
*
*	Dipir related information
*
********************************************************/


	uint32		bg_DipirFlags;			/* Flags for comm between dipir & OS */
	void		*bg_DipirRtns;			/* DIPIR's public rtns table	*/
	uint32		bg_NumIcons;			/* Amount of IconRect used already */
	uint8		bg_DipirControl;		/* Dipir control flags, from dipir RomTag */

/********************************************************
*
*	OS related information
*
********************************************************/

	void		*bg_KernelAddr;			/* Pointer to kernel module (zero if none) */
	void		*bg_KernelBase;			/* Pointer to KernelBase structure */
	uint32		bg_KernelVersion;		/* Version of current kernel (zero if none) */

	PersistentMem	bg_PersistentMem;		/* Persistent memory area */

	uint8		bg_UseDiagPort;			/* If not 0, diag port terminal connected */

/********************************************************
*
*	Exported routines
*
********************************************************/

	void		(*bg_PutC)(char);		/* Diagnostic output routine */
	int		(*bg_MayGetChar)(void);		/* Diagnostic input routine */
	uint32		(*bg_PerformSoftReset)(void);	/* Universal soft reset routine */
	void		(*bg_CleanupForNewOS)(void);	/* Routine called by dipir to prep for new OS */
	void		(*bg_AddBootAlloc)(void *, uint32, uint32);
	void		(*bg_DeleteBootAlloc)(void *, uint32, uint32);
	uint32		(*bg_VerifyBootAlloc)(void *, uint32, uint32);

/********************************************************
*
*	Miscellaneous
*
********************************************************/

	List		bg_BootAllocs;
	List		bg_BootAllocPool;
	BootAlloc	bg_BootAllocArray[NUM_BOOT_ALLOCS];
} bootGlobals ;


#endif /* __BOOTCODE_BOOTGLOBALS_H */
