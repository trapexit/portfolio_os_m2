/* @(#) romsysinfo.c 96/09/20 1.54 */

/********************************************************************************
*
*	romsysinfo.c
*
*	This file contains the ROM based portion of the sysinfo mechanism.
*
********************************************************************************/

#include <hardware/bda.h>
#include <hardware/cde.h>
#include <kernel/sysinfo.h>
#include <kernel/interrupts.h>
#include <bootcode/bootglobals.h>
#include <bootcode/boothw.h>
#include "bootcode.h"

extern	bootGlobals	BootGlobals;
extern	void		PutC(char c);


/********************************************************************************
*
*	GetBDAGPIO
*
*	This routine returns the value of the specified BDA GPIO bit.
*
*	Input:  bit = BDA GPIO bit number (0-3)
*		direction = GPIO direction (0 = input, 1 = output)
*	Output: Current value of specified bit (0 or 1)
*	Calls:	None
*
********************************************************************************/

bool GetBDAGPIO(uint32 bit, bool direction) {

	uint32 template;

	template = BDA_READ(BDAMCTL_MREF);
	if (direction) BDA_WRITE(BDAMCTL_MREF, BDAMREF_GPIO_OUT(bit, template));
	else BDA_WRITE(BDAMCTL_MREF, BDAMREF_GPIO_IN(bit, template));

	template = BDA_READ(BDAMCTL_MREF);
	if (BDAMREF_GPIO_GET(bit, template)) return 1;
	else return 0;
}


/********************************************************************************
*
*	SetBDAGPIO
*
*	This routine sets the specified BDA GPIO bit to be an output with the
*	desired value.
*
*	Input:  bit = BDA GPIO bit number (0-3)
*		value = Value to set output to (0 or 1)
*	Output: None
*	Calls:	None
*
********************************************************************************/

void SetBDAGPIO(uint32 bit, uint32 value) {

	uint32 template;

	template = BDA_READ(BDAMCTL_MREF);
	BDAMREF_GPIO_OUT(bit, template);
	if (value) BDAMREF_GPIO_SET(bit, template);
	else BDAMREF_GPIO_CLR(bit, template);
	BDA_WRITE(BDAMCTL_MREF, template);
}


/********************************************************************************
*
*	SetROMSysInfo
*
*	This routine sets a specified system parameter to a specified setting.
*
*	Input:  tag = Identifier for system parameter
*		info = Pointer to setting information
*		size = Data size of setting information
*	Output: Success, failure, or badflag
*	Calls:	SetBDAGPIO
*
********************************************************************************/

uint32 SetROMSysInfo(uint32 tag, void *info, uint32 size) {

	TOUCH(info);
	TOUCH(size);

	switch (tag) {
		case SYSINFO_TAG_AUDIN:
			switch ((uint32)info) {
				case SYSINFO_AUDIN_MOTHERBOARD:
					CDE_WRITE(CDE_BASE, CDE_GPIO2, CDE_GPIO_DIRECTION);
					break;
				case SYSINFO_AUDIN_PCCARD_5:
					CDE_WRITE(CDE_BASE, CDE_GPIO2, CDE_GPIO_DIRECTION + CDE_GPIO_OUT_VALUE);
					break;
				case SYSINFO_AUDIN_PCCARD_6:
				case SYSINFO_AUDIN_PCCARD_7:
				default:
					return SYSINFO_FAILURE;
			}
			break;

		case SYSINFO_TAG_AUDIOMUTE:
			switch ((uint32)info) {
				case SYSINFO_AUDIOMUTE_QUIET:
					SetBDAGPIO(BDAMREF_GPIO_AUDIOMUTE, 0);
					break;
				case SYSINFO_AUDIOMUTE_NOTQUIET:
					SetBDAGPIO(BDAMREF_GPIO_AUDIOMUTE, 1);
					break;
				default:
					return SYSINFO_FAILURE;
			}
			break;

		case SYSINFO_TAG_PERSISTENTMEM:
			DeleteAllBootAllocs(BA_PERSISTENT, BA_PERSISTENT);
			BootGlobals.bg_PersistentMem = *((PersistentMem*)info);
			if (BootGlobals.bg_PersistentMem.pm_AppID != 0)
				AddBootAlloc(BootGlobals.bg_PersistentMem.pm_StartAddress, BootGlobals.bg_PersistentMem.pm_Size, BA_PERSISTENT);
			break;

		case SYSINFO_TAG_INTERLACED:
			if (((BDA_READ(BDAVDU_VLOC) & VDU_VLOC_VCOUNT_MASK) >> VDU_VLOC_VCOUNT_SHIFT) >= VID_THRESHOLD) {
				return SYSINFO_FAILURE;
			}
			if (BootGlobals.bg_VideoEncoder == BG_VIDEO_ENCODER_BT9103) {
				if (!GetBDAGPIO(BDAMREF_GPIO_INTERLACE, 1))
					SetBDAGPIO(BDAMREF_GPIO_INTERLACE, 1);
			} else if (BootGlobals.bg_VideoEncoder == BG_VIDEO_ENCODER_VP536) {
				if (GetBDAGPIO(BDAMREF_GPIO_INTERLACE, 1))
					SetBDAGPIO(BDAMREF_GPIO_INTERLACE, 0);
			} else {
				/* fixme - make sure DENC is in interlaced mode */
			}
			break;

		case SYSINFO_TAG_PROGRESSIVE:
			if (((BDA_READ(BDAVDU_VLOC) & VDU_VLOC_VCOUNT_MASK) >> VDU_VLOC_VCOUNT_SHIFT) >= VID_THRESHOLD) {
				return SYSINFO_FAILURE;
			}
			if (BootGlobals.bg_VideoEncoder == BG_VIDEO_ENCODER_BT9103) {
				if (GetBDAGPIO(BDAMREF_GPIO_INTERLACE, 1))
					SetBDAGPIO(BDAMREF_GPIO_INTERLACE, 0);
			} else if (BootGlobals.bg_VideoEncoder == BG_VIDEO_ENCODER_VP536) {
				if (!GetBDAGPIO(BDAMREF_GPIO_INTERLACE, 1))
					SetBDAGPIO(BDAMREF_GPIO_INTERLACE, 1);
			} else {
				/* fixme - make sure DENC is in progressive scan mode */
			}
			break;

		case SYSINFO_TAG_GRAFBUSY:
			if (info) BootGlobals.bg_DipirFlags |= DF_VIDEO_INUSE;
			else BootGlobals.bg_DipirFlags &= ~DF_VIDEO_INUSE;
			break;

		case SYSINFO_TAG_DATADISC:
			if (info)
				BootGlobals.bg_DipirFlags |= DF_EXPECT_DATADISC;
			else
				BootGlobals.bg_DipirFlags &= ~DF_EXPECT_DATADISC;
			break;

		case SYSINFO_TAG_NOREBOOT:
			if (info)
				BootGlobals.bg_DipirFlags |= DF_NO_REBOOT;
			else
				BootGlobals.bg_DipirFlags &= ~DF_NO_REBOOT;
			break;

		case SYSINFO_TAG_KERNELADDRESS:
			BootGlobals.bg_KernelAddr = info;
			break;

		case SYSINFO_TAG_DIPIRPRIVATEBUF: /* Ptr to private Dipir buf */
			{
			SysInfoRange *range = (SysInfoRange *)info;
			DeleteAllBootAllocs(BA_DIPIR_PRIVATE, BA_DIPIR_PRIVATE);
			AddBootAlloc((void*)range->sir_Addr, range->sir_Len, 
				BA_DIPIR_PRIVATE);
			}
			break;

		case SYSINFO_TAG_KERNELBASE:
			BootGlobals.bg_KernelBase = info;
			break;

		default:
			return SYSINFO_BADTAG;
	}
	return SYSINFO_SUCCESS;
}


/********************************************************************************
*
*	QueryROMSysInfo
*
*	This routine returns a specified system parameter.
*
*	Input:  tag = Identifier for system parameter
*		info = Pointer to area to place setting information
*		size = Data size of setting information
*	Output: Success, failure, or badflag
*	Calls:	None
*
********************************************************************************/

uint32 QueryROMSysInfo(uint32 tag, void *info, uint32 size)
{
	TOUCH(size);

	switch (tag) {
	case SYSINFO_TAG_SYSTEM:	/* Mfgr, clk speed, reloc vect */
		{
			SystemInfo *si = (SystemInfo *) info;

			si->si_Mfgr = SYSINFO_MFGR_MEC;
			si->si_SysType = BootGlobals.bg_SystemType;
			si->si_BusClkSpeed = BootGlobals.bg_BusClock;
			si->si_CPUClkSpeed = BootGlobals.bg_CPUClock;
			si->si_TicksPerSec = BootGlobals.bg_TicksPerSec;
			si->si_SysFlags = SYSINFO_SYSF_HOT_PCMCIA;
			si->si_Slot7Intr = INT_CDE_GPIO1INT;
			if (BootGlobals.bg_UseDiagPort) {
				si->si_SysFlags |= SYSINFO_SYSF_SERIAL_OK;
			}
			if (BootGlobals.bg_NumCPUs == 2) {
				si->si_SysFlags |= SYSINFO_SYSF_DUAL_CPU;
			}
		}
		break;

	case SYSINFO_TAG_BDA:		/* BDA id, base addresses */
		((BDAInfo *)info)->bda_ID = BDA_READ(BDAPCTL_DEVID);
#ifdef	BUILD_BDA2_VIDEO_HACK
		if (BootGlobals.bg_NumCPUs == 1) {
			((BDAInfo *)info)->bda_ID |= 0xFF000000;
		}
#endif	/* ifdef BUILD_BDA2_VIDEO_HACK */
		((BDAInfo *)info)->bda_PBBase = (void *)BDAPCTL_BASE;
		((BDAInfo *)info)->bda_MCBase = (void *)BDAMCTL_BASE;
		((BDAInfo *)info)->bda_VDUBase = (void *)BDAVDU_BASE;
		((BDAInfo *)info)->bda_TEBase = (void *)BDATE_BASE;
		((BDAInfo *)info)->bda_DSPBase = (void *)BDADSP_BASE;
		((BDAInfo *)info)->bda_CPBase = (void *)BDACP_BASE;
		((BDAInfo *)info)->bda_MPEGBase = (void *)BDAMPEG_BASE;
		break;

	case SYSINFO_TAG_CONTROLPORT:	/* Control Port information */
		((ContPortInfo *)info)->cpi_CPFlags =
			SYSINFO_CONTROLPORT_FLAG_PRESENT |
			SYSINFO_CONTROLPORT_FLAG_OPERA_COMPATIBLE;
		break;

	case SYSINFO_TAG_GRAFDISP:	/* Display mode */
		if (BootGlobals.bg_VideoMode == BG_VIDEO_MODE_PAL) {
			*(DispModeInfo *)info =
				SYSINFO_PAL_SUPPORTED |
				SYSINFO_PAL_DFLT |
				SYSINFO_PAL_CURDISP;
		} else {
			*(DispModeInfo *)info =
				SYSINFO_NTSC_SUPPORTED |
				SYSINFO_NTSC_DFLT |
				SYSINFO_NTSC_CURDISP;
		}
		*(DispModeInfo *)info |=
			SYSINFO_INTLC_SUPPORTED |
			SYSINFO_PROG_SUPPORTED |
			SYSINFO_INTLC_DFLT;
		if (BootGlobals.bg_VideoEncoder == BG_VIDEO_ENCODER_BT9103) {
			if (GetBDAGPIO(BDAMREF_GPIO_INTERLACE, 1)) *(DispModeInfo *)info |= SYSINFO_INTLC_CURDISP;
			else *(DispModeInfo *)info |= SYSINFO_PROG_CURDISP;
		} else if (BootGlobals.bg_VideoEncoder == BG_VIDEO_ENCODER_VP536) {
			if (GetBDAGPIO(BDAMREF_GPIO_INTERLACE, 1)) *(DispModeInfo *)info |= SYSINFO_PROG_CURDISP;
			else *(DispModeInfo *)info |= SYSINFO_INTLC_CURDISP;
		} else {
			/* fixme - set properly for DENC */
		}
		break;

	case SYSINFO_TAG_HSTART_NTSC:
		if (BootGlobals.bg_VideoEncoder == BG_VIDEO_ENCODER_BT9103) {
			*(HSTARTInfo *)info = BV_HSTART_BT9103_NTSC;
		} else if (BootGlobals.bg_VideoEncoder == BG_VIDEO_ENCODER_VP536) {
			*(HSTARTInfo *)info = BV_HSTART_VP536_NTSC;
		} else {
			*(HSTARTInfo *)info = BV_HSTART_3DO_NTSC;
		}
		break;


	case SYSINFO_TAG_HSTART_PAL:
		if (BootGlobals.bg_VideoEncoder == BG_VIDEO_ENCODER_BT9103) {
			*(HSTARTInfo *)info = BV_HSTART_BT9103_PAL;
		} else if (BootGlobals.bg_VideoEncoder == BG_VIDEO_ENCODER_VP536) {
			*(HSTARTInfo *)info = BV_HSTART_VP536_PAL;
		} else {
			*(HSTARTInfo *)info = BV_HSTART_3DO_PAL;
		}
		break;

	case SYSINFO_TAG_PBUSDEV:
		{
			uint32 slot;
			PowerBusDevice *pb = (PowerBusDevice *) info;

			for (slot = 0;  slot < MAX_PBUS_SLOTS; slot++)
			{
				if (pb->pb_DeviceID ==
					BootGlobals.bg_PBusDevs[slot])
				{
					pb->pb_Address = PBUSADDR(slot);
					return SYSINFO_SUCCESS;
				}
			}
			return SYSINFO_FAILURE;
		}

	case SYSINFO_TAG_ALL_PBUSDEVS:
		{
			uint32 slot;
			uint8 *result = (uint8 *) info;

			for (slot = 0;  slot < MAX_PBUS_SLOTS;  slot++)
				result[slot] = BootGlobals.bg_PBusDevs[slot];
		}
		break;

	case SYSINFO_TAG_CACHE:
		{
			SysCacheInfo *sci = (SysCacheInfo *) info;
			sci->sci_Flags = 0;
			sci->sci_PICacheSize = 4096;
			sci->sci_PDCacheSize = 4096;
			sci->sci_SICacheSize = 0;
			sci->sci_SDCacheSize = 0;
			sci->sci_PICacheLineSize = 32;
			sci->sci_PDCacheLineSize = 32;
			sci->sci_SICacheLineSize = 0;
			sci->sci_SDCacheLineSize = 0;
			sci->sci_PICacheSetSize = 2;
			sci->sci_PDCacheSetSize = 2;
			sci->sci_SICacheSetSize = 0;
			sci->sci_SDCacheSetSize = 0;
		}
		break;

	case SYSINFO_TAG_BATT:
		{
			SysBatt *sb = (SysBatt *) info;
			sb->sb_NumBytes = 15;
			sb->sb_Addr     = (void *)(BIO_DEV_7 + 0x01000000);
		}
		break;

	case SYSINFO_TAG_SETROMSYSINFO:	/* Pointer to SetROMSysInfo() */
		*((uint32 *)info) = (uint32)SetROMSysInfo;
		break;

	case SYSINFO_TAG_BOOTGLOBALS:	/* Pointer to BootGlobals struct */
		*((uint32 *)info) = (uint32)&BootGlobals;
		break;

	case SYSINFO_TAG_PERFORMSOFTRESET: /* Pointer to PerformSoftReset() */
		*((uint32 *)info) = (uint32)BootGlobals.bg_PerformSoftReset;
		break;

	case SYSINFO_TAG_PUTC:		/* Pointer to PutC() for the kernel */
		*((uint32 *)info) = (uint32)PutC;
		break;

	case SYSINFO_TAG_MAYGETCHAR:	/* Pointer to MayGetChar() */
		*((uint32 *)info) = (uint32)BootGlobals.bg_MayGetChar;
		break;

	case SYSINFO_TAG_ADDBOOTALLOC:	/* */
		*((uint32 *)info) = (uint32)BootGlobals.bg_AddBootAlloc;
		break;

	case SYSINFO_TAG_DELETEBOOTALLOC:	/* */
		*((uint32 *)info) = (uint32)BootGlobals.bg_DeleteBootAlloc;
		break;

	case SYSINFO_TAG_VERIFYBOOTALLOC:	/* */
		*((uint32 *)info) = (uint32)BootGlobals.bg_VerifyBootAlloc;
		break;

	case SYSINFO_TAG_BOOTALLOCLIST:
		*((List **)info) = &BootGlobals.bg_BootAllocs;
		break;

	case SYSINFO_TAG_DIPIRROUTINES:	/* Pointer to Dipir routine table */
		*((uint32 *)info) = (uint32)BootGlobals.bg_DipirRtns;
		break;

	case SYSINFO_TAG_KERNELADDRESS:
		*((uint32 *)info) = (uint32)BootGlobals.bg_KernelAddr;
		break;

	case SYSINFO_TAG_OSVOLUMELABEL:
		*((uint32 *)info) = (uint32)BootGlobals.bg_OSVolumeLabel;
		break;

	case SYSINFO_TAG_APPVOLUMELABEL:
		*((uint32 *)info) = (uint32)BootGlobals.bg_AppVolumeLabel;
		break;

	case SYSINFO_TAG_DEVICEPERMS:
		*((uint32 *)info) = BootGlobals.bg_DevicePerms;
		break;

	case SYSINFO_TAG_GRAFBUSY:
		*((uint32 *)info) = BootGlobals.bg_DipirFlags & DF_VIDEO_INUSE;
		break;

	case SYSINFO_TAG_PERSISTENTMEM:
		*((PersistentMem*)info) = BootGlobals.bg_PersistentMem;
		break;

	case SYSINFO_TAG_ROMAPPDEVICE:
		*((HardwareID *)info) = BootGlobals.bg_RomAppDevice;
		break;

#ifdef BUILD_DEBUGGER
	case SYSINFO_TAG_DEBUGGERREGION:
		*((uint32 *)info) = DEBUGGERREGION;
		break;
#endif

	case SYSINFO_TAG_INTLLANG:		/* Default international lang */
		*(IntlLangInfo *)info =  BootGlobals.bg_DefaultLanguage;
		break;

	default:
		return SYSINFO_BADTAG;
	}
	return SYSINFO_SUCCESS;
}
