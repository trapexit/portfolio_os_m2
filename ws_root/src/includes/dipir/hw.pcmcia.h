#ifndef __DIPIR_HW_PCMCIA_H
#define __DIPIR_HW_PCMCIA_H 1


/******************************************************************************
**
**  @(#) hw.pcmcia.h 96/02/20 1.4
**
**  Definitions related to the HWResource_PCMCIA structure.
**
******************************************************************************/


#ifdef EXTERNAL_RELEASE
#error "This file may not be used in externally released source code or link lib"
#endif

typedef struct HWResource_PCMCIA
{
	uint32		pcmcia_Addr;
	uint16		pcmcia_NumSlots;
	uint16		pcmcia_Reserved;
} HWResource_PCMCIA;

#endif /* __DIPIR_HW_PCMCIA_H */
