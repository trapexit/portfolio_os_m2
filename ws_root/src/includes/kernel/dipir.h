#ifndef __KERNEL_DIPIR_H
#define __KERNEL_DIPIR_H


/******************************************************************************
**
**  @(#) dipir.h 96/02/20 1.4
**
******************************************************************************/


#ifdef EXTERNAL_RELEASE
#error "This file may not be used in externally released source code or link lib"
#endif

#ifndef __KERNEL_NODES_H
#include <kernel/nodes.h>
#endif

/* Signal allocated by the Operator to trigger rescanning for devices */
/* invoked by TriggerDeviceRescan() in the Kernel. */
/* Also used by File folio to tell the Operator that it has started. */
#define RESCAN_SIGNAL 0x00000800

/*
   Signal used by File folio to tell the Operator that the initial
   scan for DDFs and mountable filesystems has been completed.
*/
#define WAKEE_WAKE_SIGNAL 0x00001000

/* Used by Kernel and Operator to perform "duck" and "recover" processing. */
typedef struct CallBackNode
{
    MinNode cb_n;
    void (*cb_code)(uint32);
    uint32 cb_param;
}
CallBackNode;

/* Used by Operator, Filesystem, and high-level devices which support */
/* filesystems to handle online/offline events. */
typedef struct RescanNode
{
    MinNode rs_n;
    struct Device* rs_dev;
    uint8 rs_unit;
    uint8 rs_status;
}
RescanNode;

/* RescanNode.rs_status */
#define RS_UNIT_OFFLINE 0	/* The unit in rs_unit is gone. */
#define RS_UNIT_ONLINE 1	/* The unit in rs_unit is new. */
#define RS_DEVICE_OFFLINE 2	/* All units are gone. */

/*****************************************************************************/


#endif	/* __KERNEL_DIPIR_H */
