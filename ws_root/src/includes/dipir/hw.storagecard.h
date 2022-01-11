#ifndef __DIPIR_HW_STORAGECARD_H
#define __DIPIR_HW_STORAGECARD_H 1


/******************************************************************************
**
**  @(#) hw.storagecard.h 96/02/20 1.2
**
**  Definitions related to the HWResource_StorCard structure.
**
******************************************************************************/


#ifdef EXTERNAL_RELEASE
#error "This file may not be used in externally released source code or link lib"
#endif

typedef struct HWResource_StorCard
{
	uint32		sc_MemSize;
	uint16		sc_SectorSize;
	uint8		sc_MemType;
	uint8		sc_ChipRev;
	uint8		sc_ChipMfg;
} HWResource_StorCard;

#endif /* __DIPIR_HW_STORAGECARD_H */
