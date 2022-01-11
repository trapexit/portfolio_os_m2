#ifndef __KERNEL_DDFUNCS_H
#define __KERNEL_DDFUNCS_H


/******************************************************************************
**
**  @(#) ddfuncs.h 96/04/29 1.10
**
******************************************************************************/


#ifdef EXTERNAL_RELEASE
#error "This file may not be used in externally released source code or link lib"
#endif

#ifndef __KERNEL_DDFTOKEN_H
#include <kernel/ddftoken.h>
#endif

void RebuildDDFEnables(void);
void ProcessDDFBuffer(uint8 *buf, int32 l, uint8 version, uint8 revision);
Err ScanForDDFToken(void *np, char const *name, DDFTokenSeq *rhstokens);


#endif /* __KERNEL_DDFUNCS_H */
