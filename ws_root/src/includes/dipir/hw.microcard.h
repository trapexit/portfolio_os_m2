#ifndef __DIPIR_HW_MICROCARD_H
#define __DIPIR_HW_MICROCARD_H 1


/******************************************************************************
**
**  @(#) hw.microcard.h 96/02/20 1.3
**
**  Definitions related to the HWResource_MICRO structure.
**
******************************************************************************/


#ifdef EXTERNAL_RELEASE
#error "This file may not be used in externally released source code or link lib"
#endif

typedef struct HWResource_MICRO
{
	uint32		micro_Addr;
	uint16		micro_NumSlots;
	uint16		micro_Reserved;
} HWResource_MICRO;

#endif /* __DIPIR_HW_MICROCARD_H */
