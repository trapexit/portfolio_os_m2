#ifndef __KERNEL_IO_H
#define __KERNEL_IO_H


/******************************************************************************
**
**  @(#) io.h 96/02/18 1.23
**
**  Kernel device IO definitions
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_NODES_H
#include <kernel/nodes.h>
#endif

#ifndef __KERNEL_MSGPORT_H
#include <kernel/msgport.h>
#endif

#ifndef __KERNEL_DEVICE_H
#include <kernel/device.h>
#endif

#ifndef __KERNEL_DEVICECMD_H
#include <kernel/devicecmd.h>
#endif


/*****************************************************************************/


typedef struct IOBuf
{
    void  *iob_Buffer;	/* ptr to user's buffer                 */
    int32  iob_Len;	/* len of this buffer, or transfer size */
} IOBuf;

/* Information supplied by the user when initiating an I/O operation */
typedef struct IOInfo
{
    uint32 ioi_Command;    /* command to be executed                   */
    uint32 ioi_Flags;      /* control flags                            */
    uint32 ioi_CmdOptions; /* device dependent options                 */
    int32  ioi_Offset;     /* offset into device for transfer to begin */
    IOBuf  ioi_Send;       /* information being written out            */
    IOBuf  ioi_Recv;       /* where to put incoming information        */
    void  *ioi_UserData;   /* user-private data                        */
} IOInfo;


/*****************************************************************************/

#ifndef EXTERNAL_RELEASE
/* get the address of an IOReq from the embedded io_Link address */
#define IOReq_Addr(a)	(IOReq *)((uint32)(a) - Offset(IOReq *, io_Link))
#endif

typedef struct IOReq
{
    ItemNode       io;
    struct Device *io_Dev;           /* Device this IOReq belongs to     */
    IOInfo         io_Info;          /* as supplied by user              */
    int32          io_Actual;        /* actual size of request completed */
    uint32         io_Flags;         /* state flags, see below           */
    int32          io_Error;         /* completion status of last use    */
    int32          io_Extension[2];  /* device-specific data             */

    union
    {
    Item           io_msgItem;       /* message sent on completion       */
    int32          io_signal;        /* or signal sent on completion     */
    } io_Completion;

#ifndef  EXTERNAL_RELEASE
    MinNode        io_Link;          /* for device-specific list */
    struct IOReq  *(*io_CallBack)(struct IOReq *iorP); /* call on completion */
    Item           io_SigItem;       /* designated receiver of the signal */
    void          *io_Transaction;   /* for iodebug                       */
#endif
} IOReq;

#define io_MsgItem io_Completion.io_msgItem
#define io_Signal  io_Completion.io_signal

/* io_Flags 0x0000ffff for generic stuff, and 0xffff0000 for device-specific */
#define IO_DONE		1       /* I/O is not in use                    */
#define IO_QUICK	2	/* also for IOInfo.ioi_Flags            */
#define IO_INTERNAL	4       /* for internal use                     */
#define IO_WAITING      8       /* owner currently waiting for this I/O */
#define IO_MESSAGEBASED 16      /* when I/O completes, send a message   */

enum ioreq_tags
{
    CREATEIOREQ_TAG_REPLYPORT = TAG_ITEM_LAST+1,    /* optional */
    CREATEIOREQ_TAG_DEVICE,			    /* required */
    CREATEIOREQ_TAG_SIGNAL			    /* signal to send when IO is done */
};

/* convenient way to access an IOReq structure starting from its item number */
#define IOREQ(ioreqItem) ((IOReq *)LookupItem(ioreqItem))


/*****************************************************************************/


/* debugging control flags for ControlIODebug() */
#define IODEBUGF_PREVENT_PREREAD   0x00000001
#define IODEBUGF_PREVENT_POSTWRITE 0x00000002


/*****************************************************************************/


#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */


Err   SendIO(Item ior, const IOInfo *ioiP);
Err   DoIO(Item ior, const IOInfo *ioiP);
Err   AbortIO(Item ior);
Err   WaitIO(Item ior);
int32 CheckIO(Item ior);
#ifndef EXTERNAL_RELEASE
void CompleteIO(IOReq *ior);
#endif

Item  CreateIOReq(const char *name, uint8 pri, Item dev, Item mp);/* mp can be 0 */
Err   ControlIODebug(uint32 controlFlags);


#ifdef  __cplusplus
}
#endif  /* __cplusplus */


#define DeleteIOReq(x)	DeleteItem(x)


/*****************************************************************************/


#ifdef __DCC__
#pragma no_side_effects CheckIO
#endif


/*****************************************************************************/


#endif	/* __KERNEL_IO_H */
