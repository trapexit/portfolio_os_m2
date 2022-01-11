#ifndef __DEVICE_PROXY_H
#define __DEVICE_PROXY_H


/******************************************************************************
**
**  @(#) proxy.h 96/02/21 1.4
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_NODES_H
#include <kernel/nodes.h>
#endif

#ifndef __KERNEL_DEVICE_H
#include <kernel/device.h>
#endif


/* Tag Args when creating a proxy device */
enum create_proxy_tags
{
    CREATEPROXY_TAG_MAXUNIT=TAG_ITEM_LAST+1,  /* The number of units         */
    CREATEPROXY_TAG_MSGPORT,                  /* port to forward IOReqs to   */
    CREATEPROXY_TAG_STORAGE                   /* void* to userdriver storage */
};

enum create_softfirq_tags
{
    CREATESOFTFIRQ_TAG_INTERRUPT=TAG_ITEM_LAST+1, /* the interrupt number    */
    CREATESOFTFIRQ_TAG_SIGNAL                     /* the signal bit          */
};

enum proxy_tags
{
    PROXY_TAG_IOREQ = TAG_ITEM_LAST + 1,      /* the IOReq being munged      */
    PROXY_TAG_SET_SRC_PTR,                    /* source ptr for copy         */
    PROXY_TAG_SET_COUNT,                      /* # of bytes to copy          */
    PROXY_TAG_SET_DEST_OFFSET,                /* offset into user recv bfr   */
    PROXY_TAG_MEMCPY,                         /* validate above and do copy  */
    PROXY_TAG_SET_IO_ACTUAL,                  /* set io_Actual               */
    PROXY_TAG_SET_IO_ERROR,                   /* set io_Error                */
    PROXY_TAG_SET_IOI_OFFSET,                 /* set io_Info.ioi_Offset      */
    PROXY_TAG_SET_IOI_CMDOPTIONS,             /* set io_Info.ioi_CmdOptions  */
    PROXY_TAG_COMPLETEIO,                     /* call SuperCompleteIO        */
    PROXY_TAG_ENABLE_INTERRUPT,               /* enable a soft firq          */
    PROXY_TAG_DISABLE_INTERRUPT               /* disable a soft firq         */
};

enum proxy_function {
  PROXY_FUNC_OPEN_DEVICE,
  PROXY_FUNC_SENDIO,
  PROXY_FUNC_ABORTIO,
  PROXY_FUNC_CLOSE_DEVICE,
  PROXY_FUNC_DELETE_DEVICE
};

#define MakePRErr(svr,class,err)    MakeErr(ER_DEVC,((Make6Bit('P')<<6)|Make6Bit('R')),svr,ER_E_SSTM,class,err)

#define kPRErrNotSupported          MakePRErr(ER_SEVERE, ER_C_STND, ER_NotSupported);
#define kPRErrAborted		    MakePRErr(ER_SEVERE, ER_C_STND, ER_Aborted)
#define kPRErrBadArg		    MakePRErr(ER_SEVERE, ER_C_STND, ER_BadIOArg)
#define kPRErrBadCommand	    MakePRErr(ER_SEVERE, ER_C_STND, ER_BadCommand)
#define kPRErrBadUnit		    MakePRErr(ER_SEVERE, ER_C_STND, ER_BadUnit)
#define kPRErrNotPrivileged	    MakePRErr(ER_SEVERE, ER_C_STND, ER_NotPrivileged);
#define kPRErrSoftErr		    MakePRErr(ER_SEVERE, ER_C_STND, ER_SoftErr)
#define kPRErrNoMemAvail	    MakePRErr(ER_SEVERE, ER_C_STND, ER_NoMem)
#define kPRErrIOInProgress	    MakePRErr(ER_SEVERE, ER_C_STND, ER_IONotDone)
#define kPRErrIOIncomplete	    MakePRErr(ER_SEVERE, ER_C_STND, ER_IOIncomplete)
#define kPRErrDeviceOffline	    MakePRErr(ER_SEVERE, ER_C_STND, ER_DeviceOffline)
#define kPRErrDeviceError	    MakePRErr(ER_SEVERE, ER_C_STND, ER_DeviceEr

typedef struct ProxiedDeviceData {
  List         pd_RequestsQueued;
  List         pd_RequestsInProgress;
  Item         pd_MsgPortItem;
  Item         pd_ServerTaskItem;
} ProxiedDeviceData;

#endif
