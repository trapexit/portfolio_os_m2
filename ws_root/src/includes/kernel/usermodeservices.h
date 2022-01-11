#ifndef __KERNEL_USERMODESERVICES_H
#define __KERNEL_USERMODESERVICES_H


/******************************************************************************
**
**  @(#) usermodeservices.h 96/05/23 1.13
**
******************************************************************************/


#ifdef EXTERNAL_RELEASE
#error "This file may not be used in externally released source code or link lib"
#endif

#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif


/*****************************************************************************/


Err CallAsItemServer(void *func, void *arg1, bool privileged);


/*****************************************************************************/


#endif /* __KERNEL_USERMODESERVICES_H */
