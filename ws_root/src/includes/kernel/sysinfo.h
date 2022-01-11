#ifndef __KERNEL_SYSINFO_H
#define __KERNEL_SYSINFO_H


/******************************************************************************
**
**  @(#) sysinfo.h 96/08/26 1.44
**
******************************************************************************/


#ifdef EXTERNAL_RELEASE
#error "This file may not be used in externally released source code or link lib"
#endif

#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif


/****************************************************************************/
/* Return status From SuperQuerySysInfo()/SuperSetSysInfo() */

#define	SYSINFO_SUCCESS		0
#define	SYSINFO_BADTAG		0xd57b9002	/* BADTAG */
#define	SYSINFO_FAILURE		0xd57b9003	/* BADTAGVAL */
#define	SYSINFO_END		1		/* no more data */

/* A useful shortcut macro */
#define	QUERY_SYS_INFO(x, y)	SuperQuerySysInfo(x, (void *)&y, sizeof(y))

/****************************************************************************/
/*
 * By convention, the high 16 bits (bits 0-15) of a SYSINFO tag
 * specify the subsystem.
 * Bits 16-19 are 0x0 for a QuerySysInfo tag, or 0x1 for a SetSysInfo tag.
 * Bits 20-31 specify the tag (unique within subsystem).
 */
#define	MakeTag(subsys,set,tag)	(((subsys) << 16) | ((set)<<12) | (tag))

#define	SI_KERNEL	0x0001	/* Kernel subsystem */
#define	SI_GRAF		0x0002	/* Graphics subsystem */
#define	SI_CTLPORT	0x0003	/* Control port subsystem */
#define	SI_AUDIO	0x0004	/* Audio subsystem */
#define	SI_INTL		0x0009	/* International subsystem */
#define	SI_ROM		0x000A	/* ROM subsystem */

#define	SI_QUERY	0x0	/* Tag for QuerySysInfo() */
#define	SI_SET		0x1	/* Tag for SetSysInfo() */


/****************************************************************************/
/* Description of a memory range (address & length) */
typedef struct SysInfoRange {
	void *		sir_Addr;		/* Address */
	uint32		sir_Len;		/* Length in bytes */
} SysInfoRange;

/****************************************************************************/
#define	SYSINFO_TAG_SYSTEM	MakeTag(SI_KERNEL, SI_QUERY, 0x013)
/* Mfgr, clk speed, reloc vect */
typedef	struct SystemInfo {			/* info param type */
	uint8	si_Mfgr;			/* Manufacturer ID */
	uint8	si_SysType;			/* System Type */
	uint8	si_SysFlags;			/* System flags */
	uint8	si_Slot7Intr;			/* PCMCIA Slot 7 intr num */
	uint32	si_BusClkSpeed;			/* Bus Clock Speed in Hz */
	uint32	si_CPUClkSpeed;			/* CPU Clock Speed in Hz */
	uint32	si_TicksPerSec;			/* # decrementer ticks/sec */
} SystemInfo;

/* Values for SysType field */
#define SYSINFO_SYSTYPE_PLAYER	0x0		/* Production player */
#define SYSINFO_SYSTYPE_UPGRADE	0x1		/* Production Opera upgrade */
#define SYSINFO_SYSTYPE_TESTBED	0x2		/* M2 test bed (may be reused in future) */
#define SYSINFO_SYSTYPE_DEVCARD	0x3		/* Development card */

/* Values for mgfr field */
#define	SYSINFO_MFGR_TOSHIBA	0x01
#define	SYSINFO_MFGR_MEC	0x02
#define	SYSINFO_MFGR_SAMSUNG	0x03
#define	SYSINFO_MFGR_FUJITSU	0x04
#define	SYSINFO_MFGR_TI		0x05
#define	SYSINFO_MFGR_ROHM	0x06
#define	SYSINFO_MFGR_CHIPEX	0x07
#define	SYSINFO_MFGR_YAMAHA	0x08
#define	SYSINFO_MFGR_SANYO	0x09
#define	SYSINFO_MFGR_IBM	0x0a
#define	SYSINFO_MFGR_GOLDSTAR	0x0b
#define	SYSINFO_MFGR_NEC	0x0c
#define	SYSINFO_MFGR_MOTOROLA	0x0d
#define	SYSINFO_MFGR_ATT	0x0e
#define	SYSINFO_MFGR_VLSI	0x0f

/* Bits in si_SysFlags */
#define	SYSINFO_SYSF_HOT_PCMCIA	0x01		/* PCMCIA supports hot-insert */
#define	SYSINFO_SYSF_SERIAL_OK	0x02		/* Terminal present on diag serial port */
#define	SYSINFO_SYSF_DUAL_CPU	0x04		/* System has two CPUs */

/****************************************************************************/
#define	SYSINFO_TAG_BDA		MakeTag(SI_KERNEL, SI_QUERY, 0x014)
/* BDA id, base addresses */
typedef	struct BDAInfo {			/* info param type */
	unsigned long	bda_ID;			/* BDA ID */
	void		*bda_PBBase;		/* PowerBus controller base */
	void		*bda_MCBase;		/* Memory controller base */
	void		*bda_VDUBase;		/* VDU base */
	void		*bda_TEBase;		/* TE base */
	void		*bda_DSPBase;		/* DSP base */
	void		*bda_CPBase;		/* ControlPort base */
	void		*bda_MPEGBase;		/* MPEG base */
} BDAInfo;


/****************************************************************************/
#define	SYSINFO_TAG_CDE		MakeTag(SI_KERNEL, SI_QUERY, 0x015)
/* CDE id, base address(es) */
typedef	struct CDEInfo {			/* info param type */
	unsigned long	cde_ID;			/* CDE ID */
	void		*cde_Base;		/* CDE base */
} CDEInfo;


/****************************************************************************/
#define	SYSINFO_TAG_CACHE	MakeTag(SI_KERNEL, SI_QUERY, 0x016)
/* Prim/Sec I/D Cache size/type*/
typedef	struct SysCacheInfo {			/* info param type */
	unsigned long	sci_Flags;		/* I/D joint, enable, lock stat*/
	unsigned long	sci_PICacheSize;	/* Primary Instr cache size */
	unsigned long	sci_PDCacheSize;	/* Primary Data cache size */
	unsigned long	sci_SICacheSize;	/* Secndry Instr cache size */
	unsigned long	sci_SDCacheSize;	/* Secndry Data cache size */
	unsigned short	sci_PICacheLineSize;	/* Primary Inst cache line size*/
	unsigned short	sci_PDCacheLineSize;	/* Primary Data cache line size*/
	unsigned short	sci_SICacheLineSize;	/* Secndry Inst cache line size*/
	unsigned short	sci_SDCacheLineSize;	/* Secndry Data cache line size*/
	unsigned char	sci_PICacheSetSize;	/* Primary Instr cache set size*/
	unsigned char	sci_PDCacheSetSize;	/* Primary Data cache set size */
	unsigned char	sci_SICacheSetSize;	/* Secndry Instr cache set size*/
	unsigned char	sci_SDCacheSetSize;	/* Secndry Data cache set size */
} SysCacheInfo;

/* Values for Flags field */
#define	SYSINFO_CACHE_PUNIFIED	0x00000001	/* Joint Primry Inst/Data Cache*/
#define	SYSINFO_CACHE_SUNIFIED	0x00000002	/* Joint Scndry Inst/Data Cache*/

/****************************************************************************/
#define	SYSINFO_TAG_BATT	MakeTag(SI_KERNEL, SI_QUERY, 0x030)
/* battery backed info */
typedef	struct SysBatt
{
	unsigned long	sb_NumBytes;	/* # bytes in battmem */
	void           *sb_Addr;	/* address of HW      */
} SysBatt;

/****************************************************************************/
#define	SYSINFO_TAG_HWRESOURCE	MakeTag(SI_KERNEL, SI_QUERY, 0x020)
/* Get HWResources */

/****************************************************************************/
#define	SYSINFO_TAG_DEVICEPERMS	MakeTag(SI_KERNEL, SI_QUERY, 0x021)
/* Get device permission bits */

#define	SYSINFO_DEVICEPERM_AUDIN	0x00000001	/* Audio-input */
#define	SYSINFO_DEVICEPERM_SERIAL	0x00000002	/* Serial port */
#define	SYSINFO_DEVICEPERM_SERIAL_HI	0x00000004	/* Hi-speed serial */

/****************************************************************************/
#define	SYSINFO_TAG_WATCHDOG	MakeTag(SI_KERNEL, SI_SET, 0x011)
/* Enable Watchdog timer */
#define	SYSINFO_WDOGENABLE		1		/* info value */

/****************************************************************************/
#define	SYSINFO_TAG_DATADISC	MakeTag(SI_KERNEL, SI_SET, 0x012)

/****************************************************************************/
#define	SYSINFO_TAG_NOREBOOT	MakeTag(SI_KERNEL, SI_SET, 0x017)




/****************************************************************************/
/* FIXME: remove in favor of info from HWResource? */
#define	SYSINFO_TAG_GRAFDISP	MakeTag(SI_GRAF, SI_QUERY, 0x010)
/* Display mode */
typedef	unsigned long	DispModeInfo;		/* info param type */

/* Values for dispmode_info */
#define	SYSINFO_NTSC_SUPPORTED	0x00000001
#define	SYSINFO_PAL_SUPPORTED	0x00000002
#define	SYSINFO_INTLC_SUPPORTED	0x00000004
#define	SYSINFO_PROG_SUPPORTED	0x00000008

#define	SYSINFO_NTSC_DFLT	0x00000100
#define	SYSINFO_PAL_DFLT	0x00000200
#define	SYSINFO_INTLC_DFLT	0x00000400
#define	SYSINFO_PROG_DFLT	0x00000800

#define	SYSINFO_NTSC_CURDISP	0x00010000
#define	SYSINFO_PAL_CURDISP	0x00020000
#define	SYSINFO_INTLC_CURDISP	0x00040000
#define	SYSINFO_PROG_CURDISP	0x00080000

/****************************************************************************/
#define	SYSINFO_TAG_HSTART_NTSC	MakeTag(SI_GRAF, SI_QUERY, 0x011) /* NTSC HSTART bias */
#define	SYSINFO_TAG_HSTART_PAL	MakeTag(SI_GRAF, SI_QUERY, 0x012) /* PAL HSTART bias */
typedef	unsigned long	HSTARTInfo;		/* info param type */

/****************************************************************************/
#define	SYSINFO_TAG_GRAFBUSY	MakeTag(SI_GRAF, SI_SET, 0x002)
/* Say whether video is in use */

/****************************************************************************/
#define	SYSINFO_TAG_INTERLACED	MakeTag(SI_GRAF, SI_SET, 0x003)
#define	SYSINFO_TAG_PROGRESSIVE	MakeTag(SI_GRAF, SI_SET, 0x004)
/* Say whether video is in use */





/****************************************************************************/
#define	SYSINFO_TAG_CONTROLPORT	MakeTag(SI_CTLPORT, SI_QUERY, 0x013)
/* Control Port information */
typedef	struct	ContPortInfo {			/* info param type */
	unsigned char	cpi_CPFlags;		/* See below */
	unsigned char	cpi_RFU;		/* Reserved for future use */
	unsigned char	cpi_CPLClkSpeedFac;	/* LClk speed fac rel to Opera */
	unsigned char	cpi_CPSClkSpeedFac;	/* SClk speed fac rel to Opera */
} ContPortInfo;

#define SYSINFO_CONTROLPORT_FLAG_PRESENT          0x80
#define SYSINFO_CONTROLPORT_FLAG_OPERA_COMPATIBLE 0x40
#define SYSINFO_CONTROLPORT_FLAG_OLD_OPERA        0x20
#define SYSINFO_CONTROLPORT_FLAG_BROKEN_BDA       0x10





/****************************************************************************/
#define	SYSINFO_TAG_DSPPCLK	MakeTag(SI_AUDIO, SI_QUERY, 0x010)
/* DSPP Clock rate in Hz */
typedef	unsigned long	DSPPClkInfo;		/* info param type */

/****************************************************************************/
#define	SYSINFO_TAG_AUDIN	MakeTag(SI_AUDIO, SI_SET, 0x012)
/* Select source of audio input */
typedef	unsigned long	AudInInfo;			/* info param type */
#define	SYSINFO_AUDIN_MOTHERBOARD	1		/* Audio-in from motherboard */
#define	SYSINFO_AUDIN_PCCARD_5		2		/* Audio-in from PCMCIA card in slot 5 */
#define	SYSINFO_AUDIN_PCCARD_6		3		/* Audio-in from PCMCIA card in slot 6 */
#define	SYSINFO_AUDIN_PCCARD_7		4		/* Audio-in from PCMCIA card in slot 7 */

/****************************************************************************/
#define SYSINFO_TAG_KERNELBASE MakeTag(SI_ROM, SI_SET, 0x013)
/* Pass kernel base into the CD ROM driver */

/****************************************************************************/
#define	SYSINFO_TAG_AUDIOMUTE	MakeTag(SI_AUDIO, SI_SET, 0x014)
#define	SYSINFO_AUDIOMUTE_QUIET		1		/* Audio muted */
#define	SYSINFO_AUDIOMUTE_NOTQUIET	0		/* Audio not muted */



/****************************************************************************/
#define	SYSINFO_TAG_INTLLANG	MakeTag(SI_INTL, SI_QUERY, 0x010)
/* Default international lang */
typedef	unsigned long	IntlLangInfo;		/* info param type */

/* Values for IntlLangInfo */
#define	SYSINFO_INTLLANG_USENGLISH	0		/* US/Eglish */
#define	SYSINFO_INTLLANG_GERMAN		1		/* Germany/German */
#define	SYSINFO_INTLLANG_JAPANESE	2		/* Japan/Japanese */
#define	SYSINFO_INTLLANG_SPANISH	3		/* Spain/Spanish */
#define	SYSINFO_INTLLANG_ITALIAN	4		/* Italy/Italian */
#define	SYSINFO_INTLLANG_CHINESE	5		/* China/Chinese */
#define	SYSINFO_INTLLANG_KOREAN		6		/* Korea/Korean */
#define	SYSINFO_INTLLANG_FRENCH		7		/* France/French */
#define	SYSINFO_INTLLANG_UKENGLISH	8		/* UK/English */
#define	SYSINFO_INTLLANG_AUSENGLISH	9		/* Australia/English */
#define	SYSINFO_INTLLANG_MEXSPANISH	10		/* Mexico/Spanish */
#define	SYSINFO_INTLLANG_CANENGLISH	11		/* Canada/English */





/****************************************************************************/
#define SYSINFO_TAG_PUTC	MakeTag(SI_ROM, SI_QUERY, 0x010)
/* Pointer to PutC routine */

/****************************************************************************/
#define SYSINFO_TAG_MAYGETCHAR	MakeTag(SI_ROM, SI_QUERY, 0x011)
/* Pointer to MayGetChar routine */

/****************************************************************************/
#define SYSINFO_TAG_DIPIRROUTINES MakeTag(SI_ROM, SI_QUERY, 0x012)
/* Pointer to table of dipir routines */

/****************************************************************************/
#define SYSINFO_TAG_SETROMSYSINFO MakeTag(SI_ROM, SI_QUERY, 0x013)
/* Pointer to SetROMSysInfo function */

/****************************************************************************/
#define SYSINFO_TAG_APPVOLUMELABEL MakeTag(SI_ROM, SI_QUERY, 0x014)
/* Volume label of media where App lives */

/****************************************************************************/
#define SYSINFO_TAG_KERNELADDRESS MakeTag(SI_ROM, SI_QUERY, 0x015)
/* Address of kernel in memory */

/****************************************************************************/
#define SYSINFO_TAG_OSVOLUMELABEL MakeTag(SI_ROM, SI_QUERY, 0x016)
/* Volume label of media where OS lives */

/****************************************************************************/
/* WARNING: do not use this tag! */
#define SYSINFO_TAG_BOOTGLOBALS	MakeTag(SI_ROM, SI_QUERY, 0x017)
/* Pointer to bootGlobals structure */

/****************************************************************************/
#define SYSINFO_TAG_PERFORMSOFTRESET MakeTag(SI_ROM, SI_QUERY, 0x01b)
/* Pointer to PerformSoftReset routine */

/****************************************************************************/
#define SYSINFO_TAG_DIPIRSHAREDBUF MakeTag(SI_ROM, SI_QUERY, 0x01c)
/* SysInfoRange for shared M2CD/Dipir buf */

/****************************************************************************/
#define SYSINFO_TAG_PERSISTENTMEM MakeTag(SI_ROM, SI_QUERY, 0x01d)
/* Get/Set PersistentMem structure */
typedef struct PersistentMem {
	uint32		pm_AppID;		/* Application ID */
	void *		pm_StartAddress;	/* Address of persistent mem */
	uint32		pm_Size;		/* Size of persistent mem */
} PersistentMem;

/****************************************************************************/
#define	SYSINFO_TAG_PBUSDEV	MakeTag(SI_ROM, SI_QUERY, 0x021)
/* Get address of a PowerBus device */
typedef struct PowerBusDevice {
	uint32		pb_DeviceID;		/* Device ID (input) */
	uint32		pb_Address;		/* Address (output) */
} PowerBusDevice;

/****************************************************************************/
#define	SYSINFO_TAG_ALL_PBUSDEVS MakeTag(SI_ROM, SI_QUERY, 0x022)
/* Get list of all PowerBus devices */

/****************************************************************************/
#define SYSINFO_TAG_ROMAPPDEVICE MakeTag(SI_ROM, SI_QUERY, 0x023)
/* Get HardwareID of RomApp media device */

/****************************************************************************/
#define SYSINFO_TAG_DIPIRPRIVATEBUF MakeTag(SI_ROM, SI_QUERY, 0x024)
/* SysInfoRange for private Dipir buf */

/****************************************************************************/
#define SYSINFO_TAG_DEBUGGERREGION MakeTag(SI_ROM, SI_QUERY, 0x025)
/* Debugger communication area */

/****************************************************************************/
#define SYSINFO_TAG_CDINIT MakeTag(SI_ROM, SI_QUERY, 0x026)
/* ROM copy of CD driver */

/****************************************************************************/
#define SYSINFO_TAG_ADDBOOTALLOC	MakeTag(SI_ROM, SI_QUERY, 0x028)
/* Pointer to AddBootAlloc routine */

/****************************************************************************/
#define SYSINFO_TAG_DELETEBOOTALLOC	MakeTag(SI_ROM, SI_QUERY, 0x029)
/* Pointer to DeleteBootAlloc routine */

/****************************************************************************/
#define SYSINFO_TAG_VERIFYBOOTALLOC	MakeTag(SI_ROM, SI_QUERY, 0x02a)
/* Pointer to VerifyBootAlloc routine */

/****************************************************************************/
#define SYSINFO_TAG_BOOTALLOCLIST	MakeTag(SI_ROM, SI_QUERY, 0x02b)
/* Pointer to BootAlloc list */

typedef struct BootAlloc
{
	struct BootAlloc *ba_Next;
	struct BootAlloc *ba_Prev;
	void *          ba_Start;
	uint32          ba_Size;
	uint32          ba_Flags;
} BootAlloc;

/* Bits in ba_Flags in BootAlloc */
#define	BA_DIPIR_PRIVATE	0x00000001
#define	BA_PERSISTENT		0x00000002
#define	BA_GRAPHICS		0x00000004
#define	BA_OSDATA		0x00000008
#define	BA_PREMULTITASK		0x00000010
#define	BA_DIPIR_SHARED		0x00000020

#endif /* __KERNEL_SYSINFO_H */
