/* @(#) io.c 96/02/18 1.59 */

/* IMPORTANT: Three often forgotten rules when writing a device:
 *
 * - When a cmd function (say the typical CmdWrite() function) returns
 *   1, it means the command has completed synchronously. If you use the default
 *   command dispatcher supplied by the kernel _YOU MUST NOT CALL COMPLETEIO()_
 *   when your function returns 1. The kernel's dispatch function will do this
 *   for you.
 *
 * - When a cmd function returns 0, meaning the command completes
 *   asynchronously, your code _MUST CLEAR IO_QUICK_. And be careful when
 *   clearing the flag. If your driver can call CompleteIO() from an interrupt
 *   handler, then IO_QUICK must be cleared while interrupts are disabled.
 *   In addition, the bit must be cleared before there is any possibility that
 *   CompleteIO() be called on it by your interrupt handler or your daemon.
 *
 * Not following the above rules yields an unstable I/O environment. In
 * particular, not clearing the IO_QUICK bit when needed will cause WaitIO()
 * to sometimes hang. Calling CompleteIO() when you're not supposed to leads
 * to varying levels of confusion as it will result in an IOReq's callback
 * being called back twice.
 */

#include <kernel/types.h>
#include <kernel/list.h>
#include <kernel/listmacros.h>
#include <kernel/item.h>
#include <kernel/msgport.h>
#include <kernel/io.h>
#include <kernel/kernelnodes.h>
#include <kernel/task.h>
#include <kernel/kernel.h>
#include <kernel/device.h>
#include <kernel/driver.h>
#include <kernel/operror.h>
#include <kernel/semaphore.h>
#include <kernel/lumberjack.h>
#include <stdio.h>
#include <string.h>
#include <kernel/internalf.h>


#define DBUG(x)		/* printf x */
#define MBUG(x)		/* printf x */
#define DBUGCIO(x)	/* printf x */
#define DBUGWIO(x)	/* printf x */
#define DBUGWAIO(x)	/* printf x */

#ifdef MASTERDEBUG
#define DBUGSIO(x)	if (CheckDebug(KernelBase,24)) printf x
#else
#define DBUGSIO(x)
#endif

#define CHECKIO(ior) (ior->io_Flags & IO_DONE)


/*****************************************************************************/


Err SetIOReqOwner(IOReq *ior, Item newOwner)
{
    if (CHECKIO(ior) == 0)
    {
        /* can't do it if the I/O's in progress */
        return MAKEKERR(ER_SEVER,ER_C_STND,ER_IONotDone);
    }

    if (ior->io_Flags & IO_MESSAGEBASED)
    {
        /* can't have a msg there */
        return NOSUPPORT;
    }

    if (IsItemOpened(newOwner, ior->io_Dev->dev.n_Item) < 0)
    {
        /* new owner must also have the device opened */
        return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_ItemNotOpen);
    }

    /* cause a pointer revalidation the next time StartIO() is called */
    ior->io_Info.ioi_Recv.iob_Len = 0;

    if (ior->io_Dev->dev_Driver->drv_ChangeIOReqOwner)
        return (*(ior->io_Dev->dev_Driver->drv_ChangeIOReqOwner))(ior, newOwner);

    return 0;
}


/*****************************************************************************/


typedef struct IOReqInfo
{
    Item  iri_ReplyPort;
    int32 iri_Signal;
    bool  iri_GotSignal;
    bool  iri_GotReplyPort;
} IOReqInfo;


static int32 icior_c(IOReq *ior, IOReqInfo *iri, uint32 tag, uint32 arg)
{
    TOUCH(ior);

    switch (tag)
    {
        case CREATEIOREQ_TAG_REPLYPORT: iri->iri_ReplyPort = (Item)arg;
                                        iri->iri_GotReplyPort = TRUE;
                                        break;

        case CREATEIOREQ_TAG_DEVICE   : break;

        case CREATEIOREQ_TAG_SIGNAL   : iri->iri_Signal    = (int32)arg;
                                        iri->iri_GotSignal = TRUE;
                                        break;

        default                       : return BADTAG;
    }

    return 0;
}

/**
|||	AUTODOC -public -class Kernel -group IO -name CreateIOReq
|||	Creates an I/O request.
|||
|||	  Synopsis
|||
|||	    Item CreateIOReq(const char *name, uint8 pri, Item dev,
|||	                     Item msgPort);
|||
|||	  Description
|||
|||	    This function creates an I/O request item.
|||
|||	    When you create an I/O request, you must decide how the device will
|||	    notify you when an I/O operation completes. There are two choices:
|||
|||	    - Notification by signal
|||
|||	    - Notification by message
|||
|||	    With notification by signal, the device will send your task the
|||	    SIGF_IODONE signal whenever an I/O operation completes. This is a
|||	    low-overhead mechanism, which is also low on information. When you
|||	    get the signal, all you know is that an I/O operation has completed.
|||	    You do not know which operation has completed. This has to be
|||	    determined by looking at the state of all outstanding I/O requests.
|||
|||	    Notification by message involves slightly more overhead, but
|||	    provides much more information. When you create the I/O request, you
|||	    indicate a message port. Whenever an I/O operation completes, the
|||	    device will send a message to that message port. The message will
|||	    contain the following information:
|||
|||	    msg_Result
|||	        Contains the io_Error value from the I/O request. This
|||	        indicates the state of the I/O operation, whether it worked or
|||	        failed.
|||
|||	    msg_Val1
|||	        Contains the item number of the I/O request that completed.
|||
|||	    msg_Val2
|||	        Contains the value of the ioi_UserData field taken from
|||	        the IOInfo structure used when initiating the I/O operation.
|||
|||	    You can also create I/O requests which send you different signals
|||	    when they complete. You must use CreateItem() and build up the tag
|||	    list yourself to do this however. See the documentation entry for
|||	    the IOReq(@) item for more information on this.
|||
|||	  Arguments
|||
|||	    name
|||	        The name of the I/O request, or NULL if unnamed.
|||
|||	    pri
|||	        The priority of the I/O request. For some devices, this value
|||	        determines the order in which I/O requests are processed.
|||	        When in doubt about what value to use, use 0.
|||
|||	    dev
|||	        The item number of the device to which to send the I/O request.
|||	        This device must have been opened by the current task or
|||	        thread.
|||
|||	    msgPort
|||	        If you want to receive a message when an I/O request is
|||	        finished, this argument must be the item number of the message
|||	        port to receive the message. To receive a signal instead,
|||	        pass 0 for this argument.
|||
|||	  Return Value
|||
|||	    Returns the item number of the new I/O request, or a negative error
|||	    code for failure. Possible error codes currently include:
|||
|||	    BADITEM
|||	        The msgPort argument was not zero but did not specify a valid
|||	        message port, or the device item didn't refer to a device.
|||
|||	    ER_Kr_ItemNotOpen
|||	        The device specified by the dev argument is not open.
|||
|||	    NOMEM
|||	        There was not enough memory to complete the operation.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libc.a V20.
|||
|||	  Associated Files
|||
|||	    <kernel/io.h>, libc.a
|||
|||	  See Also
|||
|||	    DeleteIOReq()
|||
**/

/**
|||	AUTODOC -public -class items -name IOReq
|||	An item used to communicate between a task and an I/O device.
|||
|||	  Description
|||
|||	    An IOReq item is used to carry information from a client task to a
|||	    device in order to have the device perform some operation. Once the
|||	    device is stores return information in the IOReq, and returns it to
|||	    the client task.
|||
|||	  Folio
|||
|||	    Kernel
|||
|||	  Item Type
|||
|||	    IOREQNODE
|||
|||	  Create
|||
|||	    CreateIOReq(), CreateItem()
|||
|||	  Delete
|||
|||	    DeleteIOReq(), DeleteItem()
|||
|||	  Query
|||
|||	    FindItem()
|||
|||	  Modify
|||
|||	    SetItemOwner(), SetItemPri()
|||
|||	  Use
|||
|||	    SendIO(), DoIO(), AbortIO(), WaitIO(), CheckIO()
|||
|||	  Tags
|||
|||	    CREATEIOREQ_TAG_REPLYPORT (Item) - Create
|||	        The item number of a message port. This is where the device
|||	        will send a message whenever an I/O operation involving
|||	        this IOReq completes. If you do not specify this tag, the
|||	        device will send your task the SIGF_IODONE signal instead.
|||	        This tag is mutually exclusive with the CREATEIOREQ_TAG_SIGNAL
|||	        tag.
|||
|||	    CREATEIOREQ_TAG_SIGNAL (int32) - Create
|||	        Specifies the signal mask to send to your task whenever an
|||	        I/O operation involving this IOReq completes. If this
|||	        tag is not supplied, the standard SIGF_IODONE signal is
|||	        sent instead. This tag is mutually exclusive with the
|||	        CREATEIOREQ_TAG_REPLYPORT tag.
|||
|||	    CREATEIOREQ_TAG_DEVICE (Item) - Create
|||	        This specifies the item number of the device that this IOReq
|||	        will be used to communicate with. This item number is obtained
|||	        by calling OpenDeviceStack().
|||
**/

Item
internalCreateIOReq(IOReq *iordummy, TagArg *tagpt)
{
	/* Create an IOReq Item */
	Task   *ct = CURRENTTASK;
	TagArg tagreturn;
	Item   ret;
	Item   devItem;
	Device *dev;
	Msg    *msg = 0;
	IOReq  *ior = iordummy;
	IOReqInfo iri;

	DBUGCIO(("iCreateIOReq(%lx,%lx)\n",(long)ior,(long)tagpt));

	tagreturn.ta_Arg = (void *)0;
	ret = TagProcessorSearch(&tagreturn, tagpt, CREATEIOREQ_TAG_DEVICE);
	if ((int32)ret < 0) return ret;
	devItem = (Item)tagreturn.ta_Arg;

	dev = (Device *)CheckItem(devItem,KERNELNODE,DEVICENODE);
	DBUGCIO(("Validate CreateIOReq dev=%lx\n", dev));
	if (!dev) return BADITEM;

	if (FindItemSlot(ct,devItem | ITEM_WAS_OPENED) < 0)
	{
		DBUGCIO(("WARNING: Device not open!!!\n"));
		return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_ItemNotOpen);
	}

	memset(&iri,0,sizeof(iri));

	if (ior == (IOReq *)-1)
	{
	    DBUGCIO(("ioreqsize = %d\n",(int)dev->dev_Driver->drv_IOReqSize));
	    ior = (IOReq *)AllocateSizedNode(
			(Folio *)&KB,IOREQNODE,dev->dev_Driver->drv_IOReqSize);
	    DBUGCIO(("ior from AllocateSizeNode=%lx\n",(uint32)ior));
	    if (!ior) return NOMEM;
	    ret = TagProcessor(ior, tagpt, icior_c, &iri);
	    if (ret < 0) goto err;
	}
	else
	{
	    /* sanity check here */
	    if (dev->dev_Driver->drv_IOReqSize > ior->io.n_Size)
		return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_BadSize);
	}

	ior->io_Dev = dev;

	if (iri.iri_GotSignal && iri.iri_GotReplyPort)
	{
	    /* can't have both bub */
	    return BADTAGVAL;
	}

	DBUGCIO(("Validate CreateIOReq\n"));
	if (iri.iri_GotReplyPort)
	{
	    TagArg IOMsg[5];
	    IOMsg[0].ta_Tag = TAG_ITEM_PRI;
	    IOMsg[0].ta_Arg = (void *)ior->io.n_Priority;
	    IOMsg[1].ta_Tag = CREATEMSG_TAG_REPLYPORT;
	    IOMsg[1].ta_Arg = (void *)iri.iri_ReplyPort;
	    IOMsg[2].ta_Tag = CREATEMSG_TAG_MSG_IS_SMALL;
	    IOMsg[3].ta_Tag = TAG_ITEM_NAME;
	    IOMsg[3].ta_Arg = (void *)ior->io.n_Name;
	    IOMsg[4].ta_Tag = TAG_END;
	    if (ior->io.n_Name == 0) IOMsg[3].ta_Tag = TAG_END;

	    ret = externalCreateMsg((Msg *)-1,IOMsg);
	    DBUGCIO(("msgItem=%lx\n",ret));
	    if ((int32)ret < 0) goto err;

	    msg=(Msg *)LookupItem(ret);
	    msg->msg_DataPtr = ior;	/* put pointer to IOReq into message */
	    ior->io_MsgItem  = msg->msg.n_Item;
	    ior->io_Flags   |= IO_MESSAGEBASED;
	}
	else
	{
	    if (iri.iri_GotSignal)
	    {
		ret = ValidateSignal(iri.iri_Signal);
		if (ret < 0) return ret;
		ior->io_Signal = iri.iri_Signal;
	    }
	    else
	    {
                ior->io_Signal = SIGF_IODONE;
            }
	}

	DBUGCIO(("internalCreateIO: devcreate=%lx\n",(long)dev->dev_Driver->drv_CreateIOReq));
	if (dev->dev_Driver->drv_CreateIOReq)
	{
	    ret = (*dev->dev_Driver->drv_CreateIOReq)(ior);
	    if ((int32)ret < 0) goto err;
	}
	ior->io_Flags |= (IO_DONE|IO_QUICK);
	/* This list is not searched */
	ADDTAIL(&dev->dev_IOReqs,(Node *)(&ior->io_Link));
	return ior->io.n_Item;

err:
	if (msg)
	{
	    internalDeleteMsg(msg, ct);
	    FreeNode((Folio *)&KB, msg);
	}
	if (ior != iordummy)
	{
	    /* an ior was allocated by this routine */
	    FreeNode((Folio *)&KB, ior);
	}
	return ret;
}

/**
|||	AUTODOC -public -class Kernel -group IO -name AbortIO
|||	Aborts an I/O operation.
|||
|||	  Synopsis
|||
|||	    Err AbortIO(Item ior);
|||
|||	  Description
|||
|||	    This function aborts an I/O operation. If the I/O operation has
|||	    already completed, calling AbortIO() has no effect. If it is not
|||	    complete, it will be aborted.
|||
|||	    A task should call WaitIO() immediately after calling AbortIO().
|||	    When WaitIO() returns, the task knows that the I/O operation is no
|||	    longer active. It can then confirm that the I/O operation was
|||	    aborted before it was finished by checking the io_Error field of
|||	    the IOReq structure. If the operation aborted, the value of this
|||	    field is ER_Aborted.
|||
|||	  Arguments
|||
|||	    ior
|||	        The item number of the I/O request to abort.
|||
|||	  Return Value
|||
|||	    Returns 0 if the I/O operation was successfully aborted or
|||	    a negative error code for failure. Possible error codes currently
|||	    include:
|||
|||	    BADITEM
|||	        The ior argument is not an IOReq.
|||
|||	    NOTOWNER
|||	        The ior argument is an IOReq but the calling task is not its
|||	        owner.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/io.h>, libc.a
|||
|||	  See Also
|||
|||	    CheckIO(), CreateIOReq(), DeleteIOReq(), DoIO(), SendIO(), WaitIO()
|||
**/

void internalAbortIO(IOReq *ior)
{
uint32 oldints;
void (*abrt)(IOReq *);

    abrt = ior->io_Dev->dev_Driver->drv_AbortIO;
    oldints = Disable();

    LogIOReqAborted(ior);

    if (CHECKIO(ior) == 0)
        (*abrt)(ior);

    Enable(oldints);
}

Err externalAbortIO(Item iori)
{
IOReq *ior;

    /* check the IOReq item */
    ior = (IOReq *)CheckItem(iori,KERNELNODE,IOREQNODE);
    if (!ior)
        return BADITEM;

    /* can only abort if we're the owner */
    if (ior->io.n_Owner != CURRENTTASKITEM)
        return NOTOWNER;

    internalAbortIO(ior);
    return 0;
}


/* Abort an IO that may belong to a foreign task (ie: not the current task) */
static int32 internalAbortForeignIO(IOReq *ior)
{
uint32 oldints;

    DBUGWAIO(("internalAbortForeignIO entered, ior=%lx\n",ior));

    oldints = Disable();

    internalAbortIO(ior);
    while (CHECKIO(ior) == 0)
    {
         ior->io_SigItem = CURRENTTASKITEM;
         SleepTask(CURRENTTASK);
         ior->io_SigItem = 0;
    }

    Enable(oldints);

    DBUGWAIO(("internalAbortForeignIO exiting, ior=%lx\n",ior));

    return 0;
}

/**
|||	AUTODOC -public -class Kernel -group IO -name DeleteIOReq
|||	Deletes an I/O request.
|||
|||	  Synopsis
|||
|||	    Err DeleteIOReq(Item ior);
|||
|||	  Description
|||
|||	    This macro deletes an I/O request item. You can use this macro in
|||	    place of DeleteItem() to delete the item. If there was any
|||	    outstanding I/O with this IOReq, it will be aborted first.
|||
|||	  Arguments
|||
|||	    ior
|||	        The item number of the I/O request to delete.
|||
|||	  Return Value
|||
|||	    Returns 0 if the I/O request was successfully deleted or a negative
|||	    error code for failure.
|||
|||	  Implementation
|||
|||	    Macro implemented in <kernel/io.h> V20.
|||
|||	  Associated Files
|||
|||	    <kernel/io.h>, libc.a
|||
|||	  See Also
|||
|||	    CreateIOReq()
|||
**/

int32 internalDeleteIOReq(IOReq *ior, Task *t)
{
	Device *dev;
	Msg *msg;

	DBUG(("internalDeleteIOreq(%lx,%lx)\n",ior,t));
	if (CHECKIO(ior) == 0)
	{
		/* abort the thing... */
		internalAbortForeignIO(ior);
	}

	dev = ior->io_Dev;
	if (dev->dev_Driver->drv_DeleteIOReq)
	{
		(*dev->dev_Driver->drv_DeleteIOReq)(ior);
	}

	REMOVENODE((Node *)&ior->io_Link);

	if (ior->io_Flags & IO_MESSAGEBASED)
	{
            DBUG(("Calling internalDeleteMsg\n"));
            msg = (Msg *)LookupItem(ior->io_MsgItem);
            if (msg)
            {
                internalDeleteMsg(msg,t);
                FreeNode((Folio *)&KB,msg);
            }
        }

	return 0;
}


/**
|||	AUTODOC -private -class Kernel -group IO -name CompleteIO
|||	Completes an I/O operation.
|||
|||	  Synopsis
|||
|||	    void CompleteIO(IOReq *ior);
|||
|||	  Description
|||
|||	    The task using this swi must be privileged.
|||	    This function causes the system to do the proper
|||	    endaction for this iorequest.
|||
|||	    1) Send a message back
|||
|||	    2) Send a signal
|||
|||	    3) Call the callback hook.
|||
|||	    After calling this routine, the device task should
|||	    recheck pending work queues since if the IORequest
|||	    was completed via the callback mechanism there may be
|||	    new work that now needs to be done.
|||
|||	  Arguments
|||
|||	    ior
|||	        Pointer to the IORequest to send back
|||	                                to the user.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V21.
|||
|||	  Associated Files
|||
|||	    <kernel/io.h>
|||
|||	  See Also
|||
|||	    CheckIO(), CreateIOReq(), DeleteIOReq(), DoIO(),
|||	    SendIO(), WaitIO()
|||
**/

void internalCompleteIO(IOReq *ior)
{
Task   *t;
Msg    *msg;
uint32  oldints;

    if (ior->io_Flags & IO_INTERNAL)
	return;

#ifdef BUILD_IODEBUG
    if (KB_FIELD(kb_Flags) & KB_IODEBUG)
        CompleteDebuggedIO(ior);
#endif /* BUILD_IOSTRESS */

again:
    LogIOReqCompleted(ior);

#ifdef BUILD_PARANOIA
    if (CHECKIO(ior))
    {
        printf("WARNING: CompleteIO() was called multiple times for ior $%x, item $%x\n",ior,ior->io.n_Item);
        printf("         command 0x%x, device '%s'\n",ior->io_Info.ioi_Command, ior->io_Dev->dev.n_Name);
    }
#endif

    ior->io_Flags |= IO_DONE;
    if (ior->io_CallBack)
    {
	IOReq *newior = (*ior->io_CallBack)(ior);
	if (newior)
	{
	    int32 ret;
	    newior->io_Flags |= IO_INTERNAL;
	    ret = internalSendIO(newior);
	    newior->io_Flags &= ~IO_INTERNAL;
	    if (ret)
	    {	/* Done with this one too! */
		ior = newior;
		goto again;
	    }
	}
    }
    else if ((ior->io_Flags & IO_QUICK) == 0)
    {
        t = (Task *)LookupItem(ior->io.n_Owner);

        oldints = Disable();

        if (t->t_WaitItem == ior->io.n_Item)
        {
            /* owner is inside of WaitIO() */

            t->t_WaitItem = ior->io_Error;
            NewReadyTask(t);

            if (ior->io_Flags & IO_MESSAGEBASED)
            {
                msg = (Msg *)LookupItem(ior->io_MsgItem);
                if (msg)
                {
                    msg->msg_Result = ior->io_Error;
                    msg->msg_Val1   = ior->io.n_Item;
                    msg->msg_Val2   = (uint32)ior->io_Info.ioi_UserData;
                    msg->msg_Holder = t->t.n_Item;

                    REMOVENODE((Node *)msg);
                    ADDTAIL(&t->t_MessagesHeld, (Node *)msg);
                }
            }
        }
        else
        {
            /* owner is not inside WaitIO(), gotta do asynchronous notification */

            if (ior->io_Flags & IO_MESSAGEBASED)
            {
                msg = (Msg *)LookupItem(ior->io_MsgItem);
                if (msg)
                    internalReplyMsg(msg,ior->io_Error,(void *)ior->io.n_Item,(uint32)ior->io_Info.ioi_UserData);
            }
            else
	    {
	        internalSignal(t, ior->io_Signal);
	    }
	}

	if (ior->io_SigItem)
	    NewReadyTask((Task *)LookupItem(ior->io_SigItem));

	Enable(oldints);
    }
}

void externalCompleteIO(IOReq *ior)
{
    if (CURRENTTASK->t.n_ItemFlags & ITEMNODE_PRIVILEGED)
         internalCompleteIO(ior);
}

/**
|||	AUTODOC -public -class Kernel -group IO -name WaitIO
|||	Waits for an I/O operation to complete.
|||
|||	  Synopsis
|||
|||	    Err WaitIO(Item ior);
|||
|||	  Description
|||
|||	    The function puts the calling task into wait state until the
|||	    specified I/O request signals completion. When a task is in wait
|||	    state, it uses no CPU time.
|||
|||	    If the I/O request has already completed, the function returns
|||	    immediately. If the I/O request never completes and it is not
|||	    aborted, the function never returns.
|||
|||	  Arguments
|||
|||	    ior
|||	        The item number of the I/O request to wait for.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V24.
|||
|||	  Associated Files
|||
|||	    <kernel/io.h>, libc.a
|||
|||	  See Also
|||
|||	    AbortIO(), CheckIO(), DoIO(), SendIO()
|||
**/

Err internalWaitIO(IOReq *ior)
{
Err    result;
uint32 oldints;

    DBUGWIO(("internalWaitIO: ior $%x\n",ior));

    oldints = Disable();
    if (CHECKIO(ior) == 0)
    {
        CURRENTTASK->t_WaitItem = ior->io.n_Item;
        SleepTask(CURRENTTASK);
        result = CURRENTTASK->t_WaitItem;
        CURRENTTASK->t_WaitItem = -1;
    }
    else
    {
        result = ior->io_Error;
    }
    Enable(oldints);

    return result;
}


/*****************************************************************************/


Err externalWaitIO(Item iorItem)
{
IOReq *ior;

    DBUGWIO(("externalWaitIO: iorItem $%x\n",iorItem));

    /* check the IOReq item */
    ior = (IOReq *)CheckItem(iorItem,KERNELNODE,IOREQNODE);
    if (!ior)
        return BADITEM;

    /* can only wait if we're the owner */
    if (ior->io.n_Owner != CURRENTTASKITEM)
        return NOTOWNER;

    return internalWaitIO(ior);
}


/**
|||	AUTODOC -public -class Kernel -group IO -name SendIO
|||	Requests I/O be performed.
|||
|||	  Synopsis
|||
|||	    Err SendIO(Item ior, const IOInfo *ioiP);
|||
|||	  Description
|||
|||	    This function sends an I/O request to a device to initiate an I/O
|||	    operation.
|||
|||	    Call this function after creating the necessary IOReq item (for the
|||	    ior argument, created by calling CreateIOReq() ) and an IOInfo
|||	    structure (for the ioiP argument). The IOReq item specifies the
|||	    device to which to send the request and the way your task will be
|||	    notified when the I/O operation is completed. The IOInfo structure
|||	    you supply specifies the parameters for the I/O operation.
|||
|||	    This function is essentially asynchronous in nature. The I/O request
|||	    is given to the device, and the device indicates completion of the
|||	    operation by sending your task a signal or a message.
|||
|||	    In many cases, it is possible for an I/O operation to be completed
|||	    very quickly. In such a case, the overhead of sending your task a
|||	    signal or a message to indicate that the I/O operation is completed
|||	    can get to be a burden. This is the reason for the IO_QUICK flag.
|||
|||	    If you set the IO_QUICK flag in the ioi_Flags field of the IOInfo
|||	    structure, it indicates that you are prepared to deal with
|||	    synchronous command completion. If the device is able to complete
|||	    the I/O operation "quickly", then SendIO() will return 1, and no
|||	    further notification will be sent to your task about the I/O
|||	    completion (no signal or message).
|||
|||	    If you specify IO_QUICK, it doesn't mean you'll get it however, it
|||	    is merely a way for you to tell the system you are prepared to
|||	    handle synchronous completion if it is possible.
|||
|||	    When SendIO() returns 0, it means that the I/O operation is being
|||	    handled asynchronously, and you will receive notification via signal
|||	    or message when the I/O completes.
|||
|||	    If you insist on synchronous completion, you should use DoIO()
|||	    instead of SendIO().
|||
|||	    The IOInfo structure must be fully initialized before calling this
|||	    function. You can use the ioi_UserData field of the IOInfo structure
|||	    to contain whatever you want. This is a useful place to store a
|||	    pointer to contextual data that needs to be associated with the I/O
|||	    request. If you use message-based notification for your I/O requests,
|||	    the msg_Val2 field of the notification messages will contain the
|||	    value of ioi_UserData from the IOInfo structure.
|||
|||	    When using message-based notification, you must be sure to remove
|||	    the notification messages from your message port between calls to
|||	    SendIO().
|||
|||	  Arguments
|||
|||	    ior
|||	        The item number of the IOReq structure for the request. This
|||	        structure is normally created by calling CreateIOReq().
|||
|||	    ioiP
|||	        A pointer to a fully initialized IOInfo structure.
|||
|||	  Return Value
|||
|||	    Returns 1 if the I/O was completed immediately and no further
|||	    notification will be sent. Returns 0 if the I/O is being completed
|||	    asynchronously, which means a signal or message will be sent to
|||	    your task when the I/O completes. Returns a negative error code
|||	    for failure. Possible error codes currently include:
|||
|||	    BADITEM
|||	        The ior argument does not specify a valid IOReq.
|||
|||	    NOTOWNER
|||	        The I/O request specified by the ior argument is not owned by
|||	        the calling task.
|||
|||	    ER_IONotDone
|||	        The I/O request is already in use for another I/O operation.
|||
|||	    BADPTR
|||	        A pointer is invalid: Either the IOInfo structure specified
|||	        by the ioiP argument is not entirely within the task's memory,
|||	        the IOInfo receive buffer (specified by the ioi_Recv field in
|||	        the IOInfo structure) is not entirely within the task's memory,
|||	        or the IOInfo send buffer (specified by the ioi_Send field in
|||	        the IOInfo structure) is not in legal memory.
|||
|||	    BADCOMMAND
|||	        The device the I/O request is being sent to doesn't implement
|||	        the command specified in the ioi_Command field of the IOInfo
|||	        structure.
|||
|||	    BADIOARG
|||	        Illegal arguments where given in the IOInfo structure for the
|||	        given command.
|||
|||	    MSGSENT
|||	        An attempt was made to start an I/O operation without
|||	        having removed the completion message of a previous I/O
|||	        operation involving this I/O request from the message port
|||	        it is on.
|||
|||	    When SendIO() returns 0 to indicate asynchronous completion, then
|||	    the IOReq's io_Error field will contain a result code for the
|||	    I/O operation. Like all Portfolio error codes, a negative value
|||	    indicates failure, and >= 0 indicates success.
|||	    in io_Error of the IOReq structure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/io.h>, libc.a
|||
|||	  See Also
|||
|||	    AbortIO(), CheckIO(), DoIO(), WaitIO()
|||
**/

/**
|||	AUTODOC -public -class Kernel -group IO -name DoIO
|||	Performs synchronous I/O.
|||
|||	  Synopsis
|||
|||	    Err DoIO(Item ior, const IOInfo *ioiP);
|||
|||	  Description
|||
|||	    This function is the most efficient way to perform synchronous I/O.
|||	    It works like SendIO(), except that it guarantees that the I/O
|||	    operation has been completed before returning.
|||
|||	  Arguments
|||
|||	    ior
|||	        The item number of the I/O request to use.
|||
|||	    ioiP
|||	        A pointer to the IOInfo structure containing
|||	        the I/O command to be executed, input and/or
|||	        output data, and other information.
|||
|||	  Return Value
|||
|||	    Returns >= 0 when the IO operation was successful, or a negative
|||	    error code for failure. This value will be equal to the
|||	    IOReq.io_Error field.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/io.h>, libc.a
|||
|||	  See Also
|||
|||	    AbortIO(), CheckIO(), CreateIOReq(), DeleteIOReq(), SendIO(), WaitIO()
|||
**/

int32 internalSendIO(IOReq *ior)
{
Driver *drvr;
int32   result;
IOInfo *ioi = &ior->io_Info;

    DBUGSIO(("internalSendio(%lx,%lx), ",(uint32)ior));

    drvr = ior->io_Dev->dev_Driver;

    DBUGSIO(("cmd=0x%x dev=%s\n",ior->io_Info.ioi_Command,ior->io_Dev->dev.n_Name));

    ior->io_Error  = 0;
    ior->io_Actual = 0;
    ior->io_Flags &= ~(IO_DONE|IO_QUICK);
    if (ioi->ioi_Flags & IO_QUICK)
        ior->io_Flags |= IO_QUICK;

    LogIOReqStarted(ior);

    if (ior->io_Dev->dev.n_Flags & DEVF_OFFLINE)
    {
	ior->io_Error = MakeKErr(ER_SEVER,ER_C_STND,ER_DeviceOffline);
	internalCompleteIO(ior);
	result = 1;
    } else
    {
	result = (*drvr->drv_DispatchIO)(ior);
    }
    DBUGSIO(("Dispatch returns: %d\n",(int)result));

    /* ret == 0, if deferred */
    /* ret != 0, if done */

#ifdef BUILD_PARANOIA
    if (result == 0)
    {
        if (ior->io_Flags & IO_QUICK)
        {
            printf("WARNING: Device '%s', command 0x%x returned 0, indicating deferred command\n",ior->io_Dev->dev.n_Name,ior->io_Info.ioi_Command);
            printf("         completion, but the device failed to clear IO_QUICK.\n");
        }
    }
    else
    {
        if ((ior->io_Flags & IO_INTERNAL) == 0 && (CHECKIO(ior) == 0))
        {
            printf("WARNING: Device '%s', command 0x%x returned != 0, indicating synchronous command\n",ior->io_Dev->dev.n_Name,ior->io_Info.ioi_Command);
            printf("         completion, but IO_DONE is not set in io_Flags, indicating that\n");
            printf("         CompleteIO() didn't get called by the device.\n");
        }
    }
#endif

    return result;
}


/*****************************************************************************/


static int32 StartIO(IOReq *ior, IOInfo *ioi, bool wait)
{
IOInfo localInfo;

    DBUGSIO(("StartIO(%lx,%lx) %lx\n",ior,(uint32)ioi));
    DBUGSIO(("Owner=%lx ct=%lx\n",(uint32)ior->io.n_Owner,CURRENTTASKITEM));

#ifdef BUILD_IOSTRESS
    if (KB_FIELD(kb_Flags) & KB_IODEBUG)
        CleanupDebuggedIOs();
#endif

    if (!ior)
        return BADITEM;

    if (ior->io.n_Owner != CURRENTTASKITEM)
        return NOTOWNER;

    if (ior->io_Flags & IO_MESSAGEBASED)
    {
    Message *msg;

        /* message-based IO req */
        msg = (Message *)LookupItem(ior->io_MsgItem);
        if (msg == NULL)
            return BADITEM;

        if (msg->msg.n_Flags & MESSAGE_REPLIED)
        {
            /* the message wasn't gotten before the IO was resent */
            return MSGSENT;
        }
    }

    if (CHECKIO(ior) == 0)
        return MAKEKERR(ER_SEVERE,ER_C_STND,ER_IONotDone);

    if (!IsMemReadable(ioi,sizeof(IOInfo)))
        return BADPTR;

    /* copy the structure locally so users can't muck with it anymore */
    localInfo = *ioi;

    if (localInfo.ioi_Flags & ~IO_QUICK)
        return BADIOARG;

    if (localInfo.ioi_Recv.iob_Len)
    {
        if ((localInfo.ioi_Recv.iob_Len != ior->io_Info.ioi_Recv.iob_Len)
         || (localInfo.ioi_Recv.iob_Buffer != ior->io_Info.ioi_Recv.iob_Buffer))
        {
            if (!IsMemWritable(localInfo.ioi_Recv.iob_Buffer, localInfo.ioi_Recv.iob_Len))
            {
#ifdef BUILD_STRINGS
                printf("WARNING: no write permission for receive buffer supplied to DoIO()/SendIO()\n");
                printf("         ioi_Recv.iob_Buffer $%x, ioi_Recv.iob_Len %d\n",localInfo.ioi_Recv.iob_Buffer,localInfo.ioi_Recv.iob_Len);
                printf("         command 0x%x, device %s, calling task '%s'\n",ior->io_Info.ioi_Command,ior->io_Dev->dev.n_Name,CURRENTTASK->t.n_Name);
#endif
                return BADPTR;
            }
        }
    }

    if (localInfo.ioi_Send.iob_Len)
    {
        if ((localInfo.ioi_Send.iob_Len != ior->io_Info.ioi_Send.iob_Len)
         || (localInfo.ioi_Send.iob_Buffer != ior->io_Info.ioi_Send.iob_Buffer))
        {
            if (!IsMemReadable(localInfo.ioi_Send.iob_Buffer,localInfo.ioi_Send.iob_Len))
            {
#ifdef BUILD_STRINGS
                printf("WARNING: no read permission for send buffer supplied to DoIO()/SendIO()\n");
                printf("         ioi_Send.iob_Buffer $%x, ioi_Send.iob_Len %d\n",localInfo.ioi_Send.iob_Buffer,localInfo.ioi_Send.iob_Len);
                printf("         command 0x%x, device %s, calling task '%s'\n",ior->io_Info.ioi_Command,ior->io_Dev->dev.n_Name,CURRENTTASK->t.n_Name);
#endif
                return BADPTR;
            }
        }
    }

    if (wait)
        localInfo.ioi_Flags |= IO_QUICK;

    /* only copy to the IOReq when we know all values are safe */
    ior->io_Info = localInfo;

    ior->io_CallBack = NULL;

#ifdef BUILD_IODEBUG
    if (KB_FIELD(kb_Flags) & KB_IODEBUG)
        return internalSendDebuggedIO(ior);
#endif

    return internalSendIO(ior);
}


/*****************************************************************************/


/* called in supervisor mode */
Err internalDoIO(IOReq *ior)
{
Err ret;

    ret = internalSendIO(ior);

    /* Wait only if the IO was deferred (done asynchronously) */
    if (ret == 0)
        ret = internalWaitIO(ior);

    if (ret >= 0)
        ret = ior->io_Error;

    return ret;
}

/* called in supervisor mode */
Err externalDoIO(Item iorItem, IOInfo *ioInfo)
{
Err    ret;
IOReq *ior;

    ior = (IOReq *)CheckItem(iorItem,KERNELNODE,IOREQNODE);
    ret = StartIO(ior, ioInfo, TRUE);

    /* Wait only if the IO was deferred (done asynchronously) */
    if (ret == 0)
        ret = internalWaitIO(ior);

    if (ret >= 0)
        ret = ior->io_Error;

    return ret;
}

/* called in supervisor mode */
int32 externalSendIO(Item iorItem, IOInfo *ioInfo)
{
    return StartIO((IOReq *)CheckItem(iorItem,KERNELNODE,IOREQNODE), ioInfo, FALSE);
}

/**
|||	AUTODOC -public -class Kernel -group IO -name CheckIO
|||	Checks whether an I/O operation is complete.
|||
|||	  Synopsis
|||
|||	    int32 CheckIO(Item ior);
|||
|||	  Description
|||
|||	    This function checks to see if an I/O operation has completed.
|||
|||	  Arguments
|||
|||	    ior
|||	        The item number of the I/O request to be checked.
|||
|||	  Return Value
|||
|||	    Returns 0 if the I/O is not complete. It returns > 0 if it
|||	    is complete. It returns BADITEM if ior is bad.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V24.
|||
|||	  Associated Files
|||
|||	    <kernel/io.h>, libc.a
|||
|||	  See Also
|||
|||	    AbortIO(), CreateIOReq(), DeleteIOReq(), DoIO(), SendIO(), WaitIO()
|||
**/

/* user-mode vector */
int32 CheckIO(Item iorItem)
{
IOReq *ior;

#ifdef BUILD_PARANOIA
    ior = (IOReq *)CheckItem(iorItem,KERNELNODE,IOREQNODE);
#else
    ior = (IOReq *)LookupItem(iorItem);
#endif

    if (!ior)
        return BADITEM;

    return CHECKIO(ior);
}


/*****************************************************************************/


extern Semaphore *_DevSemaphore;


/* Abort any IO operation affecting the given memory range.
 * We must abort all IOReqs for the family of the supplied task, or
 * for all tasks if t is NULL.
 */
void AbortIOReqs(Task *t, uint8 *p, int32 len)
{
Driver *drv;
Device *dev;
List   *iol;
Node   *n;
IOReq  *ior;
IOBuf  *iob;

    MBUG(("AbortIOReqs(t=%lx p=%lx l=%d\n",t,p,len));

    if (_DevSemaphore) /* might not exist during bootup.... */
        internalLockSemaphore(_DevSemaphore,SEM_WAIT);

rescan:

    ScanList(&KB_FIELD(kb_Drivers),drv,Driver)
    {
        ScanList(&drv->drv_Devices,dev,Device)
        {
            iol = &dev->dev_IOReqs;
            ScanList(iol,n,Node)
            {
                ior = IOReq_Addr(n);

                if ((t == NULL) || (IsSameTaskFamily(t,(Task *)LookupItem(ior->io.n_Owner))))
                {
                    iob = &ior->io_Info.ioi_Recv;

                    /* is this IO performing a write operation? */
                    if (iob->iob_Len)
                    {
                        /* does the address fall within the buffer? */
                        if ((uint32)p + len > (uint32)iob->iob_Buffer)
                        {
                            if ((uint32)p < (uint32)iob->iob_Buffer + (uint32)iob->iob_Len)
                            {
                                /* We must request a revalidation of pointers for
                                 * any future use of this IOReq
                                 */
                                iob->iob_Len = 0;

                                /* Only try to abort IO operations still in progress */
                                if (CHECKIO(ior) == 0)
                                {
                                    /* We found an IOReq to be aborted */
                                    internalAbortForeignIO(ior);

                                    /* We must restart the scan, in case any IO got
                                     * created or deleted while we were waiting.
                                     */
                                    goto rescan;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (_DevSemaphore)
        internalUnlockSemaphore(_DevSemaphore);
}
