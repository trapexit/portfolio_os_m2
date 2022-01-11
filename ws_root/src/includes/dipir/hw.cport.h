#ifndef __DIPIR_HW_CPORT_H
#define __DIPIR_HW_CPORT_H 1


/******************************************************************************
**
**  @(#) hw.cport.h 96/07/22 1.1
**
**  Definitions related to the HWResource_ControlPort structure.
**
******************************************************************************/


#ifdef EXTERNAL_RELEASE
#error "This file may not be used in externally released source code or link lib"
#endif

typedef struct HWResource_ControlPort
{
	uint32		cport_Config1;		/* Config reg value */
	uint32		cport_Config1Fast;	/* Fast config reg value */
} HWResource_ControlPort;

#endif /* __DIPIR_HW_CPORT_H */
