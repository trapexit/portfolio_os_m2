#ifndef __DIPIR_HW_HOST_H
#define __DIPIR_HW_HOST_H 1


/******************************************************************************
**
**  @(#) hw.host.h 96/02/29 1.2
**
**  Definitions related to the HWResource_Host structure.
**
******************************************************************************/


#ifdef EXTERNAL_RELEASE
#error "This file may not be used in externally released source code or link lib"
#endif

typedef struct HWResource_Host
{
	void   *host_ReferenceToken;
	uint32  host_BlockSize;
	uint32  host_NumBlocks;
} HWResource_Host;


#endif /* __DIPIR_HW_HOST_H */
