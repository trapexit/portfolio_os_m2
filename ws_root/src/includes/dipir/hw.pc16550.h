#ifndef __DIPIR_HW_PC16550_H
#define __DIPIR_HW_PC16550_H 1


/******************************************************************************
**
**  @(#) hw.pc16550.h 96/02/20 1.4
**
**  Definitions related to the HWResource_PC16550 structure.
**
******************************************************************************/


#ifdef EXTERNAL_RELEASE
#error "This file may not be used in externally released source code or link lib"
#endif

typedef struct HWResource_PC16550
{
	uint32		pcs_Addr;	/* Memory address */
	uint32		pcs_Clock;	/* Clock frequency */
	uint16		pcs_CycleTime;	/* Required cycle time */
	uint16		pcs_Flags;	/* Kludge bits */
} HWResource_PC16550;

/* Bits in pcs_Flags */
#define	PCS_NOVAMODEM	0x0001

#endif /* __DIPIR_HW_PC16550_H */
