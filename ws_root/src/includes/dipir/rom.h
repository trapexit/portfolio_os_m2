#ifndef __DIPIR_ROM_H
#define __DIPIR_ROM_H


/******************************************************************************
**
**  @(#) rom.h 96/08/23 1.32
**
**  ROM tags.  Tells us where to find the pieces.
**
******************************************************************************/



/*****************************************************************************
 * ROM tag structure definition
 */

typedef struct RomTag
{
    uint8	rt_SubSysType;		/* Subsystem */
    uint8	rt_Type;		/* Type (within subsystem) */
    uint8	rt_Version;		/* Version number */
    uint8	rt_Revision;		/* Revision number */

    uint8	rt_Flags;		/* */
    uint8	rt_TypeSpecific;	/* Defined by each type */
    uint8	rt_Reserved1;
    uint8	rt_Reserved2;

    uint32	rt_Offset;		/* Block offset from base of table */
    uint32	rt_Size;		/* Byte size of this component */
    uint32	rt_Reserved3[4];	/* Defined by each type */
} RomTag;

/*
 * A RomTag table is an array of consecutive RomTag structures.
 * When used with an old DiscLabel structure, the array is terminated
 * with a NULL SubSysType and Type (a longword of 0x00000000).
 * When used with a new ExtVolumeLabel structure, the volume label
 * tells how many RomTags are in the RomTag table.
 *
 * rt_Offsets are in units of block size, relative to the first block
 * of the RomTag table (table must start on a block boundary).
 */


/*****************************************************************************
 * ROM tag defines for RSA-able or RSA related components on a device.
 */

#define RSANODE			0x0F		/* rt_SubSysType for such components */

/* rt_Type definitions for rt_SubSysType RSANODE (Opera) */
#define RSA_MUST_RSA		0x01		/* rt_Reserved2 = end-of-block siglen */
#define RSA_BLOCKS_ALWAYS	0x02
#define RSA_BLOCKS_SOMETIMES	0x03
#define RSA_BLOCKS_RANDOM	0x04
#define RSA_SIGNATURE_BLOCK	0x05		/* Block of digest signatures */
#define RSA_BOOT		0x06		/* Old CD dipir tag */
#define RSA_OS			0x07		/* CD's copy of kernel, operator, fs */
#  define  rt_OSFlags		rt_Reserved3[1]	/* Use a reserved field for OS flags */
#  define  OS_ROMAPP		0x00000001	/* OS can run RomApps */
#  define  OS_ANYTITLE		0x00000002	/* OS can run apps on other media */
#  define  OS_NORMAL		0x00000004	/* OS can run normal apps */
#  define  rt_OSReservedMem	rt_Reserved3[2]	/* Use a reserved field for reserved memory */
#define RSA_CDINFO		0x08		/* Optional mastering information */
						/* rt_Offset       = Zeros */
						/* rt_Size         = Zeros */
						/* rt_Reserved3[0] = Copy of VolumeUniqueId */
						/* rt_Reserved3[1] = Random number in case unique ID isn't */
						/* rt_Reserved3[2] = Date & time */
						/* rt_Reserved3[3] = Reserved */
#define RSA_NEWBOOT		0x09		/* Old CD dipir tag, double key scheme */
#define RSA_NEWNEWBOOT		0x0A		/* Old CD dipir tag, cheezo-encrypted */
#define RSA_NEWNEWGNUBOOT	0x0B		/* Old CD dipir tag, doubly encrypted */
#define RSA_BILLSTUFF		0x0C		/* Stuff for Bill Duvall */
#define RSA_NEWKNEWNEWGNUBOOT	0x0D		/* Current CD dipir, quadruple secure */
#define RSA_MISCCODE		0x10		/* Current misc_code tag */
#define RSA_APP			0x11		/* Start of app area on a CD */
#define RSA_DRIVER		0x12		/* Dipir device driver */
#  define  rt_ComponentID	rt_Reserved3[0]	/* Use a reserved field for component ID */
#define RSA_DEVDIPIR		0x13		/* Dipir code for device */
#define RSA_APPBANNER		0x14		/* App banner screen image */
#define RSA_DEPOTCONFIG		0x15		/* Depot configuration file */
#define RSA_DEVICE_INFO		0x16		/* Device ID & related info */
#define	RSA_DEV_PERMS		0x17		/* List of devices which we can use */
#define	RSA_BOOT_OVERLAY	0x18		/* Overlay module for RSA_NEW*BOOT */

/* rt_Type definitions for rt_SubSysType RSANODE (M2) */
#define RSA_M2_OS		0x87		/* CD's copy of M2 OS */
						/* Uses same field extensions as RSA_OS tag */
#define	RSA_M2_MISCCODE		0x90		/* M2 version of misc code */
#define RSA_M2_DRIVER		0x92		/* M2 dipir device driver */
						/* Uses rt_ComponentID from RSA_DRIVER tag */
#  define  DDDID_LCCD		1		/* Component is LCCD driver */
#  define  DDDID_MICROCARD_MEM	2		/* Component is microcard memory driver */
#  define  DDDID_PCMCIA_MEM	3		/* Component is 3DO card memory driver */
#  define  DDDID_HOST		4		/* Component is debugger host FS driver */
#  define  DDDID_HOSTCD		5		/* Component is host CD-ROM emulator driver */
#  define  DDDID_PCHOST		6		/* Component is PC debugger host FS driver */

#define	RSA_M2_DEVDIPIR		0x93		/* M2 device dipir */
						/* Uses rt_ComponentID from RSA_DRIVER tag */
#  define  DIPIRID_CD		2		/* Dipir to validate CD-ROM media */
#  define  DIPIRID_ROMAPP	3		/* Dipir to validate non-bootable RomApp device */
#  define  DIPIRID_SYSROMAPP	4		/* Dipir to load RomApp OS from system ROM */
#  define  DIPIRID_HOST		5		/* Dipir to load OS from debugger host */
#  define  DIPIRID_CART		6		/* Dipir to load OS from bootable cartridge */
#  define  DIPIRID_BOOTROMAPP	7		/* Dipir to load RomApp OS from device */
#  define  DIPIRID_MICROCARD	8		/* Dipir to validate generic Microcard */
#  define  DIPIRID_VISA		9		/* Dipir to validate generic VISA card */
#  define  DIPIRID_PC16550	10		/* Dipir to validate proto PC16550 card */
#  define  DIPIRID_MEDIA_DEBUG	11		/* Dipir to do debug testing of RomApp media */
#  define  DIPIRID_PCDEVCARD	12		/* Dipir to validate PC developer card */

#define RSA_M2_APPBANNER	0x94		/* M2 application banner image */
#define RSA_M2_APP_KEYS		0x95		/* 65 followed by 129 byte APP key */
#define RSA_OPERA_CD_IMAGE	0x96		/* Opera CD image downloaded by Bridgit */
#define	RSA_M2_ICON		0x97		/* Device icon image */


/*****************************************************************************/
/* ROM tag defines for components in a system ROM */

#define	RT_SUBSYS_ROM		0x10		/* rt_SubSysType for such components */

/* rt_Type definitions for rt_SubSysType RT_SUBSYS_ROM */
#define	ROM_NULL		0x0		/* NULL ROM component type */

/* Boot code related ROM rt_Types */
#define	ROM_DIAGNOSTICS		0x10		/* In-ROM diagnostics code */
#define	ROM_DIAG_LOADER		0x11		/* Loader for downloadable diagnostics */
#define	ROM_VER_STRING		0x12		/* Version string for static screen */
						/* rt_Flags[0] = Show flag (1 = show version string) */
						/* rt_Reserved1 = VCoord for string */
						/* rt_Reserved2 = HCoord for string */
#define ROM_STATIC_SCREEN	0x19		/* Boot screen image */
						/* rt_Reserved3[0] (upper 16 bits) = Image width */
						/* rt_Reserved3[0] (lower 16 bits) = Image height */
						/* rt_Reserved3[1] (upper 16 bits) = Horizontal coord */
						/* rt_Reserved3[1] (lower 16 bits) = Vertical coord */
						/* rt_Reserved3[2] Key color to map to background (0 if none) */
						/* rt_Reserved3[3] Background color to use for static screen */
#define ROM_CHARACTER_DATA	0x1A		/* ASCII character data */

/* Dipir related ROM rt_Types */
#define	ROM_DIPIR		0x20		/* System ROM dipir code */
#  define  DC_NOKEY		0x00000001	/* rt_Flags[0] = Key check always passes */
#  define  DC_DEMOKEY		0x00000002	/* rt_Flags[1] = Check with demo key */
#  define  DC_IGNORE_FAIL	0x00000004	/* rt_Flags[2] = Ignore bad devices */
#  define  DC_ALT_PCMCIA_SPACE	0x00000008	/* rt_Flags[3] = Allow alternate PCMCIA spaces */
#  define  rt_DataOffset	rt_Reserved3[0]	/* rt_Reserved3[0] = Byte offset of data section in file */
#  define  rt_DataAddress	rt_Reserved3[1]	/* rt_Reserved3[1] = Target address of data section in RAM */
#  define  rt_BSSSize		rt_Reserved3[2]	/* rt_Reserved3[2] = Size of BSS in RAM */
#  define  rt_BSSAddress	rt_Reserved3[3]	/* rt_Reserved3[3] = Target address of BSS in RAM */
#define	ROM_DIPIR_DRIVERS	0x21		/* Various dipir device drivers */

/* OS related ROM rt_Types */
#define	ROM_KERNEL_ROM		0x30		/* Reduced kernel for ROM apps */
						/* Uses same field extensions as RSA_OS tag */
#define	ROM_KERNEL_CD		0x31		/* Full but misc-less kernel for titles */
#define	ROM_OPERATOR		0x32		/* Operator for ROM apps or titles */
#define	ROM_FS			0x33		/* Fs for ROM apps or titles */
#define ROM_KINIT_ROM		0x34		/* The kernel init module */
#define	ROM_OPINIT		0x35		/* The operator init module */
#define ROM_FSINIT		0x36		/* The filesystem init stuff */

/* Configuration related ROM rt_Types */
#define ROM_SYSINFO		0x40		/* SYSINFO (system information code) */
#define	ROM_FS_IMAGE		0x41		/* Mountable ROM file system */
#define	ROM_PLATFORM_ID		0x42		/* ID for the ROM release(see sysinfo.h) */
#define	ROM_ROM2_BASE		0x43		/* Base Address of the 2nd ROM Bank */


#endif	/* __DIPIR_ROM_H */

