#ifndef __HARDWARE_MICROCARD_H
#define __HARDWARE_MICROCARD_H


/******************************************************************************
**
**  @(#) microcard.h 96/02/20 1.9
**
**  Microslot protocol stuff.
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif


/*****************************************************************************/


/*
 * Generic Microcard commands.
 */
#define	MC_OUTSYNCH		0x000	/* Sync the card */
#define	MC_OUTBYTE		0x200	/* Send a byte of data */
#define	MC_OUTRESET		0x304	/* Reset the card */
#define	MC_OUTSELECT		0x340	/* Select a slot */
#define	MC_OUTIDENTIFY		0x380	/* Requests card to identify itself */
#define	MC_OUTDOWNLOAD		0x390	/* Download data from card ROM */
#define	MC_OUTATTENREG		0x3A0	/* Requests card to send ATTEN byte */

/*
 * Storage card commands.
 */
#define	SC_READCONFIG		0x201	/* Read from config ROM */

/*
 * Layout of the card ID byte (response to OUTIDENTIFY).
 */
#define	MC_CARDDOWNLOADABLE(id)	((id) & 0x80)
#define	MC_CARDTYPE(id)		(((id) >> 3) & 0xF)
#define	MC_CARDVERSION(id)	((id) & 0x7)

/* Values in the MC_CARDTYPE field */
#define	MC_STORAGECARD		0x0
#define	MC_DIAGCARD		0x1

#define	MC_DOWNLOADABLE		0x80

/*
 * Layout of the attention byte (response to OUTATTENREG).
 */
#define	MC_NEWCARD		0x01
#define	MC_PROTOCOL_ERR		0x02

/*
 * Header in the Microcard ROM.
 */
typedef struct MicrocardHeader
{
	uint32		mh_Magic;		/* Constant to identify hdr. */
	uint32		mh_Config;		/* Configuration bits */
	uint32		mh_RomSize;		/* Size of the ROM */
	uint32		mh_RomUserOffset;	/* Offset of filesystem data */
} MicrocardHeader;

/* Value in the mh_Magic word. */
#define	MICROCARD_MAGIC		0x4D61726B	/* Identifies MicrocardHeader */

#define	SC_ROMOFFSET		0x00800000	/* Addr of icon ROM */

/*
 * Contents of StorageCard configuration ROM.
 */
typedef struct StorageCardROM
{
	uint8		scr_ChipRev;	/* Venturi revision number */
	uint8		scr_Mem;	/* Memory size & type */
	uint8		scr_Sector;	/* Sector size & flags */
	uint8		scr_Config;	/* Configuration bits */
	uint8		scr_CardID;	/* Response to OutIdentify */
} StorageCardROM;

/* Bits in scr_ChipRev */
#define	SCR_CHIP_REV		0xF0	/* Revision number */
#define	SCR_CHIP_MFG		0x0F	/* Manufacturer */

/* Bits in scr_Mem */
#define SCR_MEM_SIZE		0xF0	/* Memory size */
#define	SCR_MEM_2CHIPS		0x08	/* Two memory chips? */
#define	SCR_MEM_TYPE		0x07	/* Memory type */

/* Bits in scr_Sector */
#define	SCR_SECTOR_SIZE		0x38	/* Sector size */
#define	SCR_WRITEPROT		0x04	/* Write-protect bit */

/* Bits in scr_Config */
#define	SCR_CONFIG_SLOTADDR	0x0E	/* Slot geographic address */


/*****************************************************************************/


#endif /* __HARDWARE_MICROCARD_H */
