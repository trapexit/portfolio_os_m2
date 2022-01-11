#ifndef __CONTEXT_H
#define __CONTEXT_H


/******************************************************************************
**
**  @(#) context.h 96/05/20 1.7
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif


/*****************************************************************************/


Err  AllocTEContext(struct DeviceState *devState);
void FreeTEContext(struct DeviceState *devState);
void SaveTEContext(const struct DeviceState *devState);
void RestoreTEContext(const struct DeviceState *devState);


/*****************************************************************************/


#endif /* __CONTEXT_H */
