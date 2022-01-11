#ifndef __KERNEL_MEMMAPREC_H
#define __KERNEL_MEMMAPREC_H


/******************************************************************************
**
**  @(#) memmaprec.h 96/02/20 1.3
**
**  Utility functions for keeping track of memory mappings in a device.
**
******************************************************************************/


#ifdef EXTERNAL_RELEASE
#error "This file may not be used in externally released source code or link lib"
#endif


typedef struct MemMapRecord
{
	Node		mr_n;
	uint32		mr_Offset;
	uint32		mr_Size;
	uint32		mr_Flags;
} MemMapRecord;

#endif /* __KERNEL_MEMMAPREC_H */
