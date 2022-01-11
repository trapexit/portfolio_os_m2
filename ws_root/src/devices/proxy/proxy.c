/* @(#) proxy.c 96/07/17 1.18 */

/* The driver for the Proxy device, which allows user-mode code to create
   devices and serve as drivers for many I/O operations. */

#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/kernel.h>
#include <kernel/mem.h>
#include <kernel/nodes.h>
#include <kernel/debug.h>
#include <kernel/list.h>
#include <kernel/listmacros.h>
#include <kernel/device.h>
#include <kernel/driver.h>
#include <kernel/msgport.h>
#include <kernel/kernelnodes.h>
#include <kernel/io.h>
#include <kernel/operror.h>
#include <kernel/super.h>
#include <kernel/tags.h>
#include <kernel/interrupts.h>
#include <file/filesystem.h>
#include <file/filefunctions.h>
#include <kernel/ddfnode.h>
#include <kernel/ddfuncs.h>
#include <device/proxy.h>
#include <loader/loadererr.h>
#include <loader/loader3do.h>
#include <stdio.h>
#include <string.h>


#undef DEBUG

#ifdef DEBUG
#define DBUG(x) printf x
#else
#define DBUG(x) /* x */
#endif

#ifdef BUILD_STRINGS
#define ERR(x) PrintfSysErr(x)
#else
#define ERR(x)
#endif

/*
   Paranoia levels are as follows:

   0 - utterly permissive and promiscuous.  Highly insecure.  Will compile
       in development mode, will not compile for actual use in a runtime
       environment.  For testing only.  Capable of mortally wounding
       Mr. Pockets.  YOU HAVE BEEN WARNED.
   1 - Twiddles the DeviceStatus structure returned by the user-mode driver.
       Clears DS_USAGE_TRUEROM, to warn the loader that the contents
       of the proxied device is subject to possible tampering and that XIP
       of signed code should not be undertaken.
   2 - Like mode 1.  Also clear DS_USAGE_UNSIGNED_OK, thus forbidding the
       execution of any unsigned programs (XIP or otherwise).
   3 - Like mode 2.  Also clears the XIP_OK bit in the MemMappableDeviceInfo
       structure, thus completely disabling XIP.
   4 - Like mode 2.  Also completely disables all commands having to do with
       memory mapping.
   5 - Disables almost everything, most of the code isn't even compiled in.
       The main() stub will return a "not supported" error code.

   If the NO_PROXY preprocessor variable is defined at compilation time, the
   maximum paranoia level will be used, and the proxy device will be
   disabled.

   A paranoia level of 3 is probably appropriate for most 3DO environments.
*/

#ifdef NO_PROXY
# define PROXY_PARANOIA 5
#else
# define PROXY_PARANOIA 3
#endif

#if PROXY_PARANOIA == 0
# ifndef BUILD_DEBUGGER
#  error Cannot compile non-paranoid proxy device for actual runtime use!
#  undef PROXY_PARANOIA
#  define PROXY_PARANOIA 3
# endif
#endif

#if PROXY_PARANOIA < 5

typedef struct SoftFirqNode {
  FirqNode   sfn;
  Task      *sfn_Task;
  uint32     sfn_Signal;
} SoftFirqNode;

static int32
ProxyDevCreateIO(IOReq *ior)
{
  Item msgItem;
  const TagArg IOMsg[] =
    {
      CREATEMSG_TAG_MSG_IS_SMALL, 0,
      TAG_END, 0
    };
  msgItem = SuperCreateItem(MKNODEID(KERNELNODE,MESSAGENODE), IOMsg);
  if (msgItem < 0) {
    DBUG(("Error creating IOReq carrier message\n"));
    ERR(msgItem);
  } else {
    DBUG(("Created message item 0x%X for IOReq\n", msgItem));
  }
  ior->io_Extension[0] = msgItem;
  return msgItem;
}

static int32
ProxyDevDeleteIO(IOReq *ior)
{
  Err err = 0;
  Item msgItem;
  DBUG(("Deleting proxy IOReq\n"));
  msgItem = ior->io_Extension[0];
  ior->io_Extension[0] = 0;
  if (msgItem > 0) {
    DBUG((" It has a message 0x%X, deleting it\n", msgItem));
    err = SuperDeleteItem(msgItem);
  }
  DBUG(("IOReq deletion completed\n"));
  return err;
}

static int32
ProxyChangeIOROwnerID(IOReq *ior, Item newOwner)
{
  Err err = 0;
  Item msgItem;
  DBUG(("Changing owner of proxy IOReq\n"));
  msgItem = ior->io_Extension[0];
  if (msgItem > 0) {
    err = SuperSetItemOwner(msgItem, newOwner);
  }
  return err;
}

static int32
ProxyChangeDevOwnerID(Device *px, Item newOwner)
{
  Err err = 0;
  ProxiedDeviceData *pxd;
  Item task;
  DBUG(("Changing owner of proxy device '%s'\n", px->dev.n_Name));
  pxd = (ProxiedDeviceData *) px->dev_DriverData;
  if (pxd) {
    task = pxd->pd_ServerTaskItem ;
    if (task) {
      DBUG((" Transferring task '%s' to new owner '%s'\n",
	     TASK(task)->t.n_Name, TASK(newOwner)->t.n_Name));
      err = SuperSetItemOwner(task, newOwner);
    }
  }
  return err;
}

static int32
SendOpenClose(enum proxy_function func, Device *px, Task *t)
{
  Err err;
  Item portItem, msgItem = 0;
  Message *msg;
  Item devItem;
  ProxiedDeviceData *pxd;
#ifdef BUILD_STRINGS
  char *reason;
#endif

  DBUG(("Open/close hook for '%s' by '%s' func %d\n",
	px->dev.n_Name, t->t.n_Name, func));
  devItem = px->dev.n_Item;
  pxd = (ProxiedDeviceData *) px->dev_DriverData;
  if (px->dev_DDFNode == px->dev_Driver->drv_DDF) {
    DBUG(("Opening proxy device itself, done.\n"));
    return devItem;
  }
  if (t->t.n_Item == pxd->pd_ServerTaskItem) {
    DBUG(("%s by the server task, done.\n",
	   (func == PROXY_FUNC_OPEN_DEVICE) ? "Open" : "Close"));
    return devItem;
  }
  DBUG(("Creating short-term port\n"));
  err = portItem = SuperCreateMsgPort(NULL, 0, 0);
  if (portItem < 0) {
    DBUG(("No port\n"));
#ifdef BUILD_STRINGS
    reason = "CreateMsgPort";
#endif
    goto bail;
  }
  DBUG(("Creating short-term message\n"));
  err = msgItem = SuperCreateSmallMsg(NULL, 0, portItem);
  if (msgItem < 0) {
    DBUG(("No message\n"));
#ifdef BUILD_STRINGS
    reason = "CreateSmallMsg";
#endif
    goto bail;
  }
  err = SuperSendSmallMsg(pxd->pd_MsgPortItem, msgItem, func,
			  t->t.n_Item);
  if (err < 0) {
    DBUG(("Send failed\n"));
#ifdef BUILD_STRINGS
    reason = "SendSmallMsg";
#endif
    goto bail;
  }
  err = SuperWaitPort(portItem, msgItem);
  if (err < 0) {
    DBUG(("WaitPort failed\n"));
  }
  msg = MESSAGE(msgItem);
  if (msg) {
    DBUG(("Got a result of 0x%X\n",msg->msg_Result));
    err = msg->msg_Result;
    if (err == 0) {
      err = devItem;
    }
  } else {
    DBUG(("Whoops, the message went away!\n"));
    err = kPRErrSoftErr;
  }

#ifdef BUILD_STRINGS
  reason = "Reply";
#endif

 bail:
#ifdef BUILD_STRINGS
  if (err < 0) {
    printf("%s: ", reason);
    ERR(err);
  }
#endif
  if (msgItem > 0) {
    DBUG(("Deleting message 0x%X\n", msgItem));
    SuperDeleteItem(msgItem);
  }
  if (portItem > 0) {
    DBUG(("Deleting port 0x%X\n", portItem));
    SuperDeleteItem(portItem);
  }
  return err;
}

static int32
ProxyOpen(Device *px, Task *t)
{
  return SendOpenClose(PROXY_FUNC_OPEN_DEVICE, px, t);
}

static int32
ProxyClose(Device *px, Task *t)
{
  return SendOpenClose(PROXY_FUNC_CLOSE_DEVICE, px, t);
}

static Err GetNodeString(DDFNode *ddf, char *name, char **val, int32 required)
{
  DDFTokenSeq seq;
  DDFToken token;
  Err err;
  *val = NULL;
  err = ScanForDDFToken(ddf->ddf_Provides, name, &seq);
  if (err < 0) {
    if (required) {
      return err;
    } else {
      return 0;
    }
  }
  err = GetDDFToken(&seq, &token);
  if (err < 0) {
    return err;
  }
  if (token.tok_Type != TOK_STRING) return -4;
  *val = token.tok_Value.v_String;
  return 0;
}

static const TagArg filesystemSearch[] =
{
    FILESEARCH_TAG_SEARCH_FILESYSTEMS,  (TagData) DONT_SEARCH_UNBLESSED,
    TAG_END
};

static Err ProxyDeviceCreate(Device *prox)
{
  Item module;
  Item task;
  Module *mptr;
  Err err;
  int32 cmdLen;
  ProxiedDeviceData *pxd;
  char *serverPath, *serverArg;
  char pathname[256], cmdline[256], devstr[11];

  if (!prox->dev_DDFNode) {
    return -1; /* FIXME */
  }

  DBUG(("Fetching SERVERPATH from DDF\n"));
  err = GetNodeString((DDFNode *) prox->dev_DDFNode,
		      "SERVERPATH", &serverPath, TRUE);
  if (err < 0) {
    return err;
  }
  DBUG(("Found %s\n", serverPath));

  DBUG(("Fetching SERVERARG from DDF\n"));
  err = GetNodeString((DDFNode *) prox->dev_DDFNode,
		      "SERVERARG", &serverArg, FALSE);
  if (err < 0) {
    return err;
  }
  DBUG(("Found %s\n", serverArg));

  DBUG(("Searching for server program\n"));
  err = SuperFindFileAndIdentify(pathname, sizeof(pathname), serverPath,
			       filesystemSearch);
  if (err < 0) {
    DBUG(("Can't find server program\n"));
    ERR(err);
    return err;
  }
  DBUG(("Found %s\n", pathname));

  sprintf(devstr, "0x%X", prox->dev.n_Item);

  cmdLen = strlen(pathname) + strlen(devstr) + 3;

  if (serverArg) {
    cmdLen += strlen(serverArg) + 1;
  }

  if (cmdLen > sizeof cmdline) {
    DBUG(("Command too long\n"));
    return NOMEM;
  }

  if (serverArg) {
    sprintf(cmdline, "%s %s %s", pathname, devstr, serverArg);
  } else {
    sprintf(cmdline, "%s %s", pathname, devstr);
  }

  DBUG(("Opening module\n"));

  module = OpenModule(pathname, OPENMODULE_FOR_TASK, NULL);
/* want signing?           (mustBeSigned ? modTags : NULL)); */

  if (module < 0) {
    DBUG(("Could not open module\n"));
    ERR(module);
    return module;
  }

  mptr = (Module *)LookupItem(module);

  DBUG(("Creating task\n"));

  task = CreateTaskVA(module, mptr->n.n_Name,
		      CREATETASK_TAG_CMDSTR, cmdline,
		      CREATETASK_TAG_DEFAULTMSGPORT, 0,
		      TAG_END);

  DBUG(("Task creation returned 0x%X\n", task));

  if (task < 0) {
    DBUG(("Could not create task!\n"));
    ERR(task);
    CloseModule(module);
    return task;
  }

  pxd = (ProxiedDeviceData *) prox->dev_DriverData;
  PrepList(&pxd->pd_RequestsQueued);
  PrepList(&pxd->pd_RequestsInProgress);
  pxd->pd_ServerTaskItem = task;
  pxd->pd_MsgPortItem = TASK(task)->t_DefaultMsgPort;
  DBUG(("Proxied device has been created\n"));
  return prox->dev.n_Item;
}

static Err ProxyDeviceDelete(Device *prox)
{
  ProxiedDeviceData *pxd;
  pxd = (ProxiedDeviceData *) prox->dev_DriverData;
  DBUG(("Delete hook called for proxy device\n"));
  if (pxd && pxd->pd_ServerTaskItem) {
    DBUG(("Killing server task\n"));
    SuperInternalDeleteItem(pxd->pd_ServerTaskItem);
  }
  return 0;
}

static void ProxyAbortIO(IOReq *ior)
{
/** FIXME this needs a lot more work! **/
  ior->io_Error = ABORTED;
  return;
}

static long SoftFirqHandler(FirqNode *firq)
{
  SoftFirqNode *softFirq = (SoftFirqNode *) firq;
  if (softFirq->sfn_Task && softFirq->sfn_Signal) {
    SuperinternalSignal(softFirq->sfn_Task, softFirq->sfn_Signal);
  }
  return 0;
}

static int32 CmdCreateSoftFirq(IOReq *ior)
{
  TagArg *tagArray, *tag;
  uint32 interrupt = 0;
  uint32 signal = 0;
  SoftFirqNode *softFirq;
  uint8 oldpriv;
  Item firqItem;
  Device *px;
  ProxiedDeviceData *pxd;
  TagArg firqArgs[] =
    {
      CREATEFIRQ_TAG_NUM,	0,
      CREATEFIRQ_TAG_CODE,      (TagData) SoftFirqHandler,
      TAG_END,		0,
    };
  px = ior->io_Dev;
  pxd = (ProxiedDeviceData *) px->dev_DriverData;
  if (ior->io.n_Owner != pxd->pd_ServerTaskItem) {
    ior->io_Error = kPRErrNotPrivileged;
    return 1;
  }
  tagArray = (TagArg *) ior->io_Info.ioi_Send.iob_Buffer;
  while ((tag = NextTagArg(&tagArray)) != NULL) {
    switch (tag->ta_Tag) {
    case CREATESOFTFIRQ_TAG_INTERRUPT:
      interrupt = (uint32) tag->ta_Arg;
      break;
    case CREATESOFTFIRQ_TAG_SIGNAL:
      signal = (uint32) tag->ta_Arg;
      break;
    default:
      ior->io_Error = kPRErrBadArg;
      DBUG(("Bad tag 0x%X\n", tag->ta_Tag));
      return 1;
    }
  }
  if (signal == 0 ||
      (signal & SIGF_RESERVED) != 0 ||
      (interrupt != INT_CDE_DEV5 &&
       interrupt != INT_CDE_DEV6 &&
       interrupt != INT_CDE_DEV7)) {
    ior->io_Error = kPRErrBadArg;
    DBUG(("Bad signal or interrupt 0x%X\n", tag->ta_Tag));
    return 1;
  }
  firqArgs[0].ta_Arg = (TagData) interrupt;
  oldpriv = PromotePriv(CURRENTTASK);
  firqItem = SuperCreateSizedItem(MKNODEID(KERNELNODE,FIRQNODE),
				  firqArgs,
				  sizeof (SoftFirqNode));
  DemotePriv(CURRENTTASK, oldpriv);
  if (firqItem < 0) {
    DBUG(("Couldn't create the FIRQ node\n"));
    ior->io_Error = firqItem;
    return 1;
  }
  softFirq = (SoftFirqNode *) LookupItem(firqItem);
  softFirq->sfn_Task = CURRENTTASK;
  softFirq->sfn_Signal = signal;
  ior->io_Info.ioi_CmdOptions = firqItem;
  return 1;
}

static void SpringboardPushRequests(Device *px)
{
  ProxiedDeviceData *pxd;
  int32 interrupts;
  IOReq *ior;
  MsgPort *port;
  Message *msg;
  Err err;
  pxd = (ProxiedDeviceData *) px->dev_DriverData;
  port = MSGPORT(pxd->pd_MsgPortItem);
  while (1) {
    interrupts = Disable();
    ior = (IOReq *) RemHead(&pxd->pd_RequestsQueued);
    Enable(interrupts);
    if (!ior) {
      break;
    }
#if PROXY_PARANOIA >= 4
    switch (ior->io_Info.ioi_Command) {
    case CMD_GETMAPINFO:
    case CMD_MAPRANGE:
    case CMD_UNMAPRANGE:
      ior->io_Error = kPRErrNotSupported;
      SuperCompleteIO(ior);
      continue;
    default:
      break;
    }
#endif
    msg = MESSAGE((Item) ior->io_Extension[0]);
    if (!port || !msg) {
      err = kPRErrDeviceOffline;
    } else {
      DBUG(("Sending message 0x%X at 0x%X flags 0x%X, data 0x%08X 0x%08X\n",
	    ior->io_Extension[0], msg, msg->msg.n_Flags, PROXY_FUNC_SENDIO,
	    ior->io.n_Item));
      err = SuperInternalPutMsg (port, msg, (void *) PROXY_FUNC_SENDIO,
				 ior->io.n_Item);
    }
    if (err < 0) {
      DBUG(("Error when sending!\n"));
      ERR(err);
      ior->io_Error = err;
      SuperCompleteIO(ior);
    } else {
      interrupts = Disable();
      AddTail(&pxd->pd_RequestsInProgress, (Node *) ior);
      Enable(interrupts);
    }
  }
}


#if PROXY_PARANOIA > 0
void BeParanoid(IOReq *ior)
{
  DeviceStatus ds;
#if PROXY_PARANOIA == 3
  MemMappableDeviceInfo mmdi;
#endif
  int32 bytes;
  if (!ior->io_Info.ioi_Recv.iob_Buffer) {
    return;
  }
  switch (ior->io_Info.ioi_Command) {
  case CMD_STATUS:
    bytes = ior->io_Actual;
    if (bytes > sizeof ds) {
      bytes = sizeof ds;
    }
    memcpy(&ds, ior->io_Info.ioi_Recv.iob_Buffer, bytes);
#if PROXY_PARANOIA == 1
    ds.ds_DeviceUsageFlags &= ~DS_USAGE_TRUEROM;
#else
    ds.ds_DeviceUsageFlags &= ~(DS_USAGE_TRUEROM|DS_USAGE_UNSIGNED_OK);
#endif
    memcpy(ior->io_Info.ioi_Recv.iob_Buffer, &ds, bytes);
    break;
#if PROXY_PARANOIA == 3
  case CMD_GETMAPINFO:
    bytes = ior->io_Actual;
    if (bytes > sizeof mmdi) {
      bytes = sizeof mmdi;
    }
    memcpy(&mmdi, ior->io_Info.ioi_Recv.iob_Buffer, bytes);
    mmdi.mmdi_Permissions &= ~XIP_OK;
    memcpy(ior->io_Info.ioi_Recv.iob_Buffer, &mmdi, bytes);
    break;
#endif
  default:
    break;
  }
  return;
}
#endif

static int32
CmdDwim(IOReq *ior)
{
  IOReq *proxiedIOReq = NULL;
  Message *msg;
  Err err;
  char *srcPtr = NULL;
  int32 count = 0, destOffset = -1;
  TagArg *tagList, *tag;
  SoftFirqNode *softFirq;
  ProxiedDeviceData *pxd;
  ior->io_Flags |= IO_DONE; /* we always complete synchronously */
  pxd = (ProxiedDeviceData *) ior->io_Dev->dev_DriverData;
  if (ior->io.n_Owner != pxd->pd_ServerTaskItem) {
    ior->io_Error = kPRErrNotPrivileged;
    return 1;
  }
  tagList = (TagArg *) ior->io_Info.ioi_Send.iob_Buffer;
  while ((tag = NextTagArg(&tagList)) != NULL) {
    switch (tag->ta_Tag) {
    case PROXY_TAG_IOREQ:
      proxiedIOReq = (IOReq *) CheckItem((Item) tag->ta_Arg, KERNELNODE, IOREQNODE);
      if (!proxiedIOReq) {
	ior->io_Error = kPRErrBadArg;
	return 1;
      }
      if ((proxiedIOReq->io_Flags & IO_DONE) ||
	  proxiedIOReq->io_Dev != ior->io_Dev) {
	ior->io_Error = kPRErrNotPrivileged;
	return 1;
      }
      break;
    case PROXY_TAG_SET_SRC_PTR:
      srcPtr = (char *) tag->ta_Arg;
      break;
    case PROXY_TAG_SET_COUNT:
      count = (int32) tag->ta_Arg;
      break;
    case PROXY_TAG_SET_DEST_OFFSET:
      destOffset = (int32) tag->ta_Arg;
      break;
    case PROXY_TAG_MEMCPY:
      if (!proxiedIOReq ||
	  count < 0 || destOffset < 0 ||
	  !IsMemReadable(srcPtr,count) ||
	  count + destOffset > proxiedIOReq->io_Info.ioi_Recv.iob_Len) {
	ior->io_Error = kPRErrBadArg;
	return 1;
      }
      DBUG(("Copying %d bytes from 0x%X to 0x%X\n", count, srcPtr,
	    destOffset + (char *) proxiedIOReq->io_Info.ioi_Recv.iob_Buffer));
      memcpy(destOffset + (char *) proxiedIOReq->io_Info.ioi_Recv.iob_Buffer,
	     srcPtr, count);
      break;
    case PROXY_TAG_SET_IO_ACTUAL:
      if (!proxiedIOReq) {
	ior->io_Error = kPRErrBadArg;
	return 1;
      }
      proxiedIOReq->io_Actual = (uint32) tag->ta_Arg;
      break;
    case PROXY_TAG_SET_IO_ERROR:
      if (!proxiedIOReq) {
	ior->io_Error = kPRErrBadArg;
	return 1;
      }
      proxiedIOReq->io_Error = (uint32) tag->ta_Arg;
      DBUG(("Setting io_Error to 0x%X\n", tag->ta_Arg));
      break;
    case PROXY_TAG_SET_IOI_OFFSET:
      if (!proxiedIOReq) {
	ior->io_Error = kPRErrBadArg;
	return 1;
      }
      proxiedIOReq->io_Info.ioi_Offset = (uint32) tag->ta_Arg;
      break;
    case PROXY_TAG_SET_IOI_CMDOPTIONS:
      if (!proxiedIOReq) {
	ior->io_Error = kPRErrBadArg;
	return 1;
      }
      proxiedIOReq->io_Info.ioi_CmdOptions = (uint32) tag->ta_Arg;
      break;
    case PROXY_TAG_COMPLETEIO:
      if (!proxiedIOReq) {
	ior->io_Error = kPRErrBadArg;
	return 1;
      }
      msg = MESSAGE((Item) proxiedIOReq->io_Extension[0]);
      if (msg && !msg->msg_ReplyPort &&
	  msg->msg.n_Owner == CURRENTTASK->t.n_Item) {
	DBUG(("Transferring message ownership\n"));
	err = SuperSetItemOwner(msg->msg.n_Item, proxiedIOReq->io.n_Owner);
	if (err < 0) {
	  DBUG(("Could not change ownership\n"));
	  ERR(err);
	}
      }
      RemNode((Node *) proxiedIOReq);
#if PROXY_PARANOIA > 0
      BeParanoid(proxiedIOReq);
#endif
      SuperCompleteIO(proxiedIOReq);
      break;
    case PROXY_TAG_ENABLE_INTERRUPT:
    case PROXY_TAG_DISABLE_INTERRUPT:
      softFirq = (SoftFirqNode *) CheckItem((uint32) tag->ta_Arg,
					    KERNELNODE, FIRQNODE);
      if (!softFirq ||
	  softFirq->sfn.firq.n_Size != sizeof (SoftFirqNode) ||
	  (void *)softFirq->sfn.firq_Code != (void *)SoftFirqHandler ||
	  softFirq->sfn_Task != CURRENTTASK) {
	ior->io_Error = kPRErrBadArg;
	return 1;
      }
      if (tag->ta_Tag == PROXY_TAG_ENABLE_INTERRUPT) {
	EnableInterrupt(softFirq->sfn.firq_Num);
      } else {
	DisableInterrupt(softFirq->sfn.firq_Num);
      }
      break;
    default:
      ior->io_Error = kPRErrBadArg;
      return 1;
    }
  }
  return 1;
}

static int32 ProxyDispatch(IOReq *ior)
{
  Device *px;
  int32 interrupts;
  ProxiedDeviceData *pxd;
  px = ior->io_Dev;
  if (px->dev_DDFNode == px->dev_Driver->drv_DDF) {
/*
   This is the proxy device itself - it supports a minimal CMD_STATUS and
   nothing else.
*/
    DeviceStatus status;
    int32 len;
    switch (ior->io_Info.ioi_Command) {
    case CMD_STATUS:
      memset(&status, 0, sizeof status);
      status.ds_MaximumStatusSize = sizeof status;
      status.ds_DeviceBlockSize = 1;
      len = ior->io_Info.ioi_Recv.iob_Len;
      if (len > sizeof status) {
	len = sizeof status;
      }
      if (len > 0) {
	memcpy(ior->io_Info.ioi_Recv.iob_Buffer, &status, len);
      }
      ior->io_Actual = len;
      break;
    default:
      ior->io_Error = kPRErrBadCommand;
      break;
    }
    return 1;
  }
  switch (ior->io_Info.ioi_Command) {
  case PROXY_CMD_DWIM:
    return CmdDwim(ior);
  case PROXY_CMD_CREATE_SOFTFIRQ:
    return CmdCreateSoftFirq(ior);
  default:
    pxd = (ProxiedDeviceData *) px->dev_DriverData;
    ior->io_Flags &= ~IO_QUICK;
    interrupts = Disable();
    AddTail(&pxd->pd_RequestsQueued, (Node *) ior);
    Enable(interrupts);
    if (!(ior->io_Flags & IO_INTERNAL)) {
      SpringboardPushRequests(px);
    }
  }
  return 0;
}

static const TagArg drvrArgs[] =
{
	TAG_ITEM_PRI,	                 (TagData) 1,
	TAG_ITEM_NAME,	                 (TagData) "proxy",
	CREATEDRIVER_TAG_MODULE,         (TagData) 0, /* fill in */
	CREATEDRIVER_TAG_ABORTIO,	 (TagData) ProxyAbortIO,
	CREATEDRIVER_TAG_DISPATCH,       (TagData) ProxyDispatch,
	CREATEDRIVER_TAG_CREATEDEV,      (TagData) ProxyDeviceCreate,
	CREATEDRIVER_TAG_DELETEDEV,      (TagData) ProxyDeviceDelete,
	CREATEDRIVER_TAG_OPENDEV,        (TagData) ProxyOpen,
	CREATEDRIVER_TAG_CLOSEDEV,       (TagData) ProxyClose,
	CREATEDRIVER_TAG_CRIO,           (TagData) ProxyDevCreateIO,
	CREATEDRIVER_TAG_DLIO,           (TagData) ProxyDevDeleteIO,
	CREATEDRIVER_TAG_CHANGEOWNER,    (TagData) ProxyChangeIOROwnerID,
	CREATEDRIVER_TAG_DEVICEDATASIZE, (TagData) sizeof(ProxiedDeviceData),
	CREATEDRIVER_TAG_CHOWN_DEV,      (TagData) ProxyChangeDevOwnerID,
	TAG_END,		0,
};

#define numTags sizeof (drvrArgs) / sizeof (TagArg)

int main(void)
{
  TagArg myTags[numTags];

  memcpy(myTags, drvrArgs, sizeof myTags);

  myTags[2].ta_Arg = (TagData) FindCurrentModule();

  DBUG(("Creating Proxy driver\n"));

  return CreateItem(MKNODEID(KERNELNODE,DRIVERNODE),myTags);
}

#else

int main(int32 argc, char **argv)
{
  TOUCH(argc);
  TOUCH(argv);
  return kPRErrNotSupported;
}

#endif
