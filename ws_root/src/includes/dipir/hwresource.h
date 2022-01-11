#ifndef __DIPIR_HWRESOURCE_H
#define __DIPIR_HWRESOURCE_H 1


/******************************************************************************
**
**  @(#) hwresource.h 96/02/20 1.19
**
**  Definitions related to the HWResource structure.
**
******************************************************************************/


#ifdef EXTERNAL_RELEASE
#error "This file may not be used in externally released source code or link lib"
#endif

/*
 * A Channel identifier.
 */
typedef uint16 Channel;

/*
 * Channels (possible values of a Channel variable).
 */
#define	CHANNEL_SYSMEM		1  /* System memory (RAM, ROM) */
#define	CHANNEL_ONBOARD		2  /* Misc. motherboard hardware */
#define	CHANNEL_PCMCIA		3  /* PCMCIA cards */
#define	CHANNEL_MICROCARD	4  /* Microslot cards */
#define	CHANNEL_LCCD		5  /* Internal Low-Cost CD drive */
#define	CHANNEL_BRIDGIT		6  /* Bridgit connection to Opera system */
#define	CHANNEL_CTLPORT		7  /* Control port hardware */
#define	CHANNEL_SERIAL		8  /* Internal serial port */
#define	CHANNEL_RTC		9  /* Real-time clock */
#define	CHANNEL_HOST		10 /* Debugger host */


/*
 * A Slot identifier.
 * Meaning of the slot ID is dependent on the Channel,
 * but within one Channel, no two devices can have the same Slot ID.
 */
typedef uint16 Slot;
#define	SLOT_BASE_MASK		0x000F /* For PCMCIA only */

/*
 * The central data structure that represents a hardware device.
 */
typedef struct HWResource
{
	uint32		hwr_Version;	/* Version of HWResource struct */
	char		hwr_Name[32];	/* Name of device */
	Channel		hwr_Channel;	/* Which channel is device on? */
	Slot		hwr_Slot;	/* Which slot in the channel? */
	HardwareID	hwr_InsertID;	/* Insertion ID */
	uint32		hwr_filler1;
	uint32		hwr_Perms;	/* Dipir permissions */
	uint32		hwr_ROMSize;	/* Size of device ROM */
	uint32		hwr_ROMUserStart; /* Non-usable stuff at start of ROM */
	uint32		hwr_filler2[5];	/* Future expansion */
	uint32		hwr_DeviceSpecific[12];
					/* Info specific to the DeviceType */
} HWResource;

/* Bits in hwr_Perms */
#define	HWR_XIP_OK		0x00000001 /* Execute-in-place permitted */
#define	HWR_UNSIGNED_OK		0x00000002 /* Unsigned excutables permitted */
#define	HWR_WRITE_PROTECT	0x00000004 /* Device is write-protected */

#define DEVNAME_SEPARATOR     ('\\')	/* hwr_Name separator char */

extern int MatchDeviceName(const char *name, const char *fullname, uint32 part);

#endif /* __DIPIR_HWRESOURCE_H */
