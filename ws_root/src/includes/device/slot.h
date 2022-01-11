#ifndef __DEVICE_SLOT_H
#define __DEVICE_SLOT_H


/******************************************************************************
**
**  @(#) slot.h 96/02/20 1.4
**
**  Definitions related to the SLOT device commands.
**
******************************************************************************/


#ifndef __KERNEL_DRIVER_H
#include <kernel/driver.h>
#endif

/*****************************************************************************
 * Extended status for the slot device.
 */
typedef struct SlotDeviceStatus
{
	DeviceStatus	sds;			/* Standard device status */
	uint32		sds_IntrNum;		/* Interrupt number */
} SlotDeviceStatus;


/*****************************************************************************
 * Passed in ioi_Send for CMD_SETTIMING.
 * All pct_xxxTime values are in nanoseconds.
 * pct_xxxTime == 0 means "do not change".
 */
typedef struct SlotTiming
{
	uint32		st_Flags;		/* See below */
	uint32		st_DeviceWidth;		/* Device width (8, 16, 32) */
	uint32		st_CycleTime;		/* Cycle time */
	uint32		st_PageModeCycleTime;	/* Page-mode cycle time */
	uint32		st_ReadHoldTime;	/* Read hold time */
	uint32		st_ReadSetupTime;	/* Read setup time */
	uint32		st_WriteHoldTime;	/* Write hold time */
	uint32		st_WriteSetupTime;	/* Write setup time */
	uint32		st_IOReadSetupTime;	/* Read setup time for I/O */
} SlotTiming;

/* Special st_xxx value to indicate "no change" */
#define	ST_NOCHANGE	0xFFFFFFFF

/* Bits in st_Flags. */
#define	ST_PAGE_MODE	0x00000001		/* Enable page-mode accesses */
#define	ST_SYNCA_MODE	0x00000002		/* Enable "MODEA" accesses */
#define	ST_HIDEA_MODE	0x00000004		/* Enable "HIDEA" accesses */

#if 0
/* These bits are NOT IMPLEMENTED. DO NOT USE. */
#define	ST_ATTR_SPACE	0x00000100		/* Use Attribute space */
#define	ST_IO_SPACE	0x00000200		/* Use I/O space */
#define	ST_IOIS16	0x00000400		/* Card selects device width */
#endif

#endif /* __DEVICE_SLOT_H */
