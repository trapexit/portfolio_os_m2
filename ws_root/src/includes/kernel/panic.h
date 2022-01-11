#ifndef __KERNEL_PANIC_H
#define __KERNEL_PANIC_H


/******************************************************************************
**
**  @(#) panic.h 96/02/20 1.6
**
******************************************************************************/


#ifdef EXTERNAL_RELEASE
#error "This file may not be used in externally released source code or link lib"
#endif

#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_OPERROR_H
#include <kernel/operror.h>
#endif

#ifndef __KERNEL_INTERNALF_H
#include <kernel/internalf.h>
#endif

#define PANIC(err)  OSPanic(0, MakePanicErr(err),__FILE__,__LINE__)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern void Panic(uint32 panicerr,char *file,uint32 line);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __KERNEL_PANIC_H */




