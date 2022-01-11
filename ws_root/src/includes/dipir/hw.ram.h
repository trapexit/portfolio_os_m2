#ifndef __DIPIR_HW_RAM_H
#define __DIPIR_HW_RAM_H 1


/******************************************************************************
**
**  @(#) hw.ram.h 96/02/20 1.7
**
**  Definitions related to the HWResource_RAM structure.
**
******************************************************************************/


#ifdef EXTERNAL_RELEASE
#error "This file may not be used in externally released source code or link lib"
#endif

typedef struct HWResource_RAM
{
	uint32		ram_Addr;	/* Memory address */
	uint32		ram_Size;	/* Size in bytes */
	uint32		ram_Start;	/* Offset of user visible area */
	uint32		ram_PageSize;	/* Page size */
	uint32		ram_Flags;	/* Flags (see below) */
	uint32		ram_MemTypes;	/* Memory type bits (see mem.h) */
	uint8		ram_FSType;	/* Preferred filesystem type */
} HWResource_RAM;

/* Bits in ram_Flags */
#define	RAM_READONLY		0x01	/* Memory is read-only */
#define	RAM_NONVOLATILE		0x02	/* Memory is nonvolatile */
#define	RAM_BYTEACCESS		0x04	/* Memory must be accessed as bytes */

#endif /* __DIPIR_HW_RAM_H */
