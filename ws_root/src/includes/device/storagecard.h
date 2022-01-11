#ifndef __DEVICE_STORAGECARD_H
#define __DEVICE_STORAGECARD_H


/******************************************************************************
**
**  @(#) storagecard.h 96/02/18 1.12
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif


/*****************************************************************************/


#define STCRD_DEVICE_NAME "storagecard"

typedef struct StorageCardStatus
{
    DeviceStatus sc_ds;
    uint8        sc_ChipRev;
    uint8        sc_ChipMfg;
    uint8        sc_MemType;
    uint8        sc_Reserved;
} StorageCardStatus;

/* sc_MemType */
#define SCMT_AT29C    0
#define SCMT_ROM      1
#define SCMT_SRAM     2
#define SCMT_AMDINTEL 3
/* Other values are reserved. */


/*****************************************************************************/


#endif /* __DEVICE_STORAGECARD_H */
