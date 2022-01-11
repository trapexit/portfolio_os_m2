/* @(#) ports.c 96/09/24 1.48 */

#include <kernel/types.h>
#include <kernel/nodes.h>
#include <kernel/item.h>
#include <kernel/list.h>
#include <kernel/listmacros.h>
#include <kernel/kernelnodes.h>
#include <kernel/task.h>
#include <kernel/msgport.h>
#include <kernel/kernel.h>
#include <kernel/mem.h>
#include <kernel/operror.h>
#include <kernel/lumberjack.h>
#include <hardware/PPCasm.h>
#include <string.h>
#include <stdio.h>
#include <kernel/internalf.h>


/*****************************************************************************/


#ifdef BUILD_STRINGS
#define INFO(x) printf x
#else
#define INFO(x)
#endif


/*****************************************************************************/


#define DBUGDEL(x)	/*printf x*/
#define DBUG(x)	/*printf x*/

#ifdef MASTERDEBUG
#define DBUGSM(x)	if (CheckDebug(KernelBase,16)) printf x
#define DBUGRM(x)	if (CheckDebug(KernelBase,18)) printf x
#define DBUGWP(x)	if (CheckDebug(KernelBase,17)) printf x
#define DBUGGM(x)	if (CheckDebug(KernelBase,19)) printf x
#else
#define DBUGSM(x)
#define DBUGRM(x)
#define DBUGWP(x)
#define DBUGGM(x)
#endif


/*****************************************************************************/


/**
|||	AUTODOC -class Kernel -group Messaging -name SendMsg
|||	Sends a message.
|||
|||	  Synopsis
|||
|||	    Err SendMsg(Item msgPort, Item msg,
|||	                const void *dataptr, int32 datasize);
|||
|||	  Description
|||
|||	    This function sends a message to the specified message port. (To
|||	    send a small message use SendSmallMsg().)
|||
|||	    The message is queued on the message port's list of messages
|||	    according to its priority. Messages that have the same priority
|||	    are queued in FIFO order.
|||
|||	    Whether a message is standard or buffered is determined by
|||	    how it was created: CreateMsg() creates a standard message,
|||	    while CreateBufferedMsg() creates a buffered message.
|||
|||	    A standard message is one whose message data belongs to the sending
|||	    task. The receiving task just gets pointers to the data and reads
|||	    from there, which means the data area for a standard message
|||	    must remain valid until the receiving task is done reading from
|||	    it.
|||
|||	    A buffered message contains a data buffer within the message packet
|||	    itself. When you send such a message, the data you supply is
|||	    copied into the buffer. The receiving task then reads the data
|||	    from the message's buffer, which means that you can immediately
|||	    reuse your data buffer after having sent a buffered message since
|||	    the receiving task will be reading the data from the message's
|||	    buffer and not from your data buffer.
|||
|||	    When sending a buffered message, SendMsg() checks the size of
|||	    the data block to see whether it will fit in the message's buffer.
|||	    If it won't fit, SendMsg() returns an error.
|||
|||	    If a message is created without a reply port, then the act of
|||	    sending the message also transfers the ownership of the message
|||	    to the recipient task.
|||
|||	  Arguments
|||
|||	    msgPort
|||	        The item number of the message port to which to send the
|||	        message.
|||
|||	    msg
|||	        The item number of the message to send.
|||
|||	    dataptr
|||	        A pointer to the message data, or NULL if there is no data to
|||	        include in the message. For a standard message, the pointer
|||	        specifies a data block owned by the sending task, which can
|||	        later be read by the receiving task. For a buffered message,
|||	        the pointer specifies a block of data to be copied into the
|||	        message's internal data buffer.
|||
|||	    datasize
|||	        The size of the message data, in bytes, or 0 if there is no
|||	        data to include in the message.
|||
|||	  Return Value
|||
|||	    >= 0 if the message was sent successfully or a negative error
|||	    code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/msgport.h>, libc.a
|||
|||	  See Also
|||
|||	    GetMsg(), GetThisMsg(), ReplyMsg(), ReplySmallMsg(), SendSmallMsg(),
|||	    WaitPort()
|||
**/

/**
|||	AUTODOC -class Kernel -group Messaging -name SendSmallMsg
|||	Sends a small message.
|||
|||	  Synopsis
|||
|||	    Err SendSmallMsg(Item msgPort, Item msg, uint32 val1, uint32 val2);
|||
|||	  Description
|||
|||	    This function sends a small message. Small messages contain
|||	    only 8 bytes of data. This function call is a synonym for
|||	    SendMsg() and is provided merely as a convenience to avoid the
|||	    need to cast the two data values to the types needed by SendMsg().
|||	    See the documentation for SendMsg() for more details.
|||
|||	  Arguments
|||
|||	    msgPort
|||	        The item number of the message port to which to send the
|||	        message.
|||
|||	    msg
|||	        The item number of the message to send.
|||
|||	    val1
|||	        The first four bytes of message data. This data is put into
|||	        the msg_DataPtr field of the message structure.
|||
|||	    val2
|||	        The last four bytes of message data. This data is put into
|||	        the msg_DataSize field of the message structure.
|||
|||	  Return Value
|||
|||	    >= 0 if the message is sent successfully or a negative error
|||	    code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/msgport.h>, libc.a
|||
|||	  See Also
|||
|||	    GetMsg(), GetThisMsg(), ReplyMsg(), ReplySmallMsg(), SendMsg(),
|||	    WaitPort()
|||
**/

/* WARNING: Can be called from interrupt code. This will usually happen
 *          indirectly when the interrupt code calls CompleteIO(), which
 *          calls internalReplyMsg(), which finally calls this function.
 *
 * NOTE: When calling from interrupt code, the message must have a valid
 *       reply port. Otherwise, an attempt will be made to transfer the
 *       ownership of the message to the owner of the target message port.
 *       Changing the ownership is not a legal operation from interrupt code.
 */
static Err internalDeliverMsg(MsgPort *port, Message *msg, void *dataptr, int32 datasize,
                              bool reply, bool superDuperPrivilege)
{
Err	result;
uint32  oldints;
Task   *t;

    if (port->mp.n_ItemFlags & ITEMNODE_DELETED)
    {
        /* the port is in the process of going away... */
        return BADITEM;
    }

    if (msg->msg_ReplyPort == 0)
    {
#ifdef BUILD_PARANOIA
        if (_mfmsr() & MSR_EE == 0)
        {
            printf("ERROR: attempting to send a message with no reply port\n");
            printf("       while interrupts are disabled\n");
            return BADPRIV;
        }
#endif

        /* If the message doesn't have a reply port, we transfer its
         * ownership to the owner of the target message port.
         */
        result = externalSetItemOwner(msg->msg.n_Item,port->mp.n_Owner);
        if (result < 0)
        {
            if (!superDuperPrivilege)
                return result;
        }
    }

    t = (Task *)LookupItem(port->mp.n_Owner);

    oldints = Disable();

    if (msg->msg.n_Flags & (MESSAGE_SENT | MESSAGE_REPLIED))
    {
        /* message is already sitting in a message port */
        Enable(oldints);
        return MSGSENT;
    }

    /* remove from the current messages-held queue */
    REMOVENODE((Node *)msg);

#ifdef BUILD_LUMBERJACK
    if (reply)
        LogMessageReplied(msg,dataptr,datasize);
    else
        LogMessageSent(msg, port, dataptr, datasize);
#endif

    if (t->t_WaitItem >= 0)
    {
        /* recipient task may be inside WaitPort() */

        if (t->t_WaitItem == port->mp.n_Item)
        {
            /* task is waiting for any message to come in to the port */

            t->t_WaitItem   = msg->msg.n_Item;
            msg->msg_Holder = t->t.n_Item;
            ADDTAIL(&t->t_MessagesHeld,(Node *)msg);
            NewReadyTask(t);
            result = 0;
        }
        else if (t->t_WaitItem == msg->msg.n_Item)
        {
            /* task is waiting for this particular message */

            msg->msg_Holder = t->t.n_Item;
            ADDTAIL(&t->t_MessagesHeld,(Node *)msg);
            NewReadyTask(t);
            result = 0;
        }
        else
        {
            /* task is waiting for something else, send a signal */

            if (reply)
                msg->msg.n_Flags |= MESSAGE_REPLIED;
            else
                msg->msg.n_Flags |= MESSAGE_SENT;

            msg->msg_Holder = port->mp.n_Item;
            InsertNodeFromTail(&port->mp_Msgs, (Node *)msg);

            result = internalSignal(t, port->mp_Signal);
        }
    }
    else
    {
        /* not inside WaitPort(), send a signal */
        if (reply)
            msg->msg.n_Flags |= MESSAGE_REPLIED;
        else
            msg->msg.n_Flags |= MESSAGE_SENT;

        msg->msg_Holder = port->mp.n_Item;
        InsertNodeFromTail(&port->mp_Msgs, (Node *)msg);

        result = internalSignal(t, port->mp_Signal);
    }

    Enable(oldints);

    msg->msg_DataSize = datasize;
    if (msg->msg.n_Flags & MESSAGE_PASS_BY_VALUE)
    {
        /* copy the data */
        memcpy(msg->msg_DataPtr,dataptr,datasize);
    }
    else
    {
        msg->msg_DataPtr = dataptr;
    }

    DBUGSM(("SendMsg: returning %d\n",result));

    return result;
}


/*****************************************************************************/


Err internalSendMsg(MsgPort *port, Message *msg, void *dataptr, int32 datasize)
{
    return internalDeliverMsg(port,msg,dataptr,datasize, FALSE, FALSE);
}


/*****************************************************************************/


Err externalSendMsg(Item portItem, Item msgItem, void *dataptr, int32 datasize)
{
MsgPort *port;
Message *msg;
Task    *task;

    DBUGSM(("externalSendMsg(%lx,%lx)" ,imp,imsg));

    port = (MsgPort *)CheckItem(portItem,KERNELNODE,MSGPORTNODE);
    if (!port)
    {
        /* bad port item */
        return BADITEM;
    }

    msg = (Message *)CheckItem(msgItem,KERNELNODE,MESSAGENODE);
    if (!msg)
    {
        /* bad message item */
        return BADITEM;
    }

    if (msg->msg.n_Flags & MESSAGE_PASS_BY_VALUE)
    {
        /* Validate the message size */
        if (datasize > msg->msg_DataPtrSize)
        {
            return BADSIZE;
        }

        /* Validate incoming message data ptr */
        if (!IsMemReadable(dataptr,datasize))
        {
            INFO(("WARNING: SendMsg() with bad data (dataptr=$%x, datasize=%d)\n", dataptr, datasize));
            return BADPTR;
        }
    }
    else if ((msg->msg.n_Flags & MESSAGE_SMALL) == 0)
    {
        /* Validate incoming message data ptr */
        if (!IsMemReadable(dataptr,datasize))
        {
            INFO(("WARNING: SendMsg() with bad data (dataptr=$%x, datasize=%d)\n", dataptr, datasize));
            return BADPTR;
        }
    }

    if (msg->msg_Holder != CURRENTTASKITEM)
    {
        /* the current task is not the current holder of the message */
        task = (Task *)CheckItem(msg->msg_Holder,KERNELNODE,TASKNODE);
        if (!task)
        {
            /* The message is already on a message port, since msg_Holder
             * is not a task item.
             */
            return MSGSENT;
        }

        if (!IsSameTaskFamily(task,CURRENTTASK))
        {
            /* Can only send a message held by a member of the same
             * task family.
             */
            return NOTOWNER;
        }
    }

    msg->msg_Result = 0;

    return internalDeliverMsg(port,msg,dataptr,datasize, FALSE, FALSE);
}


/*****************************************************************************/


Err superinternalSendMsg(MsgPort *mp, Message *msg, void *dataptr, int32 datasize)
{
    return internalDeliverMsg(mp, msg, dataptr, datasize, FALSE, TRUE);
}


/*****************************************************************************/


/**
|||	AUTODOC -class Kernel -group Messaging -name ReplyMsg
|||	Sends a reply to a message.
|||
|||	  Synopsis
|||
|||	    Err ReplyMsg(Item msg, int32 result,
|||	                 const void *dataptr, int32 datasize);
|||
|||	  Description
|||
|||	    This function sends a reply to a standard or buffered message.
|||
|||	    When a message is first created, a reply port may be provided by
|||	    the creator. When you reply a message, it is like sending it to the
|||	    reply port.
|||
|||	    When replying a message, you supply a result code which typically
|||	    indicates whether the operation triggered by the message was
|||	    successful.
|||
|||	    The meaning of the dataptr and datasize arguments depend on the
|||	    type of message being replied. You can find out what type of
|||	    message it is by looking at its msg.n_Flags field. If it is a small
|||	    message, the value of the field is MESSAGE_SMALL. If it is a
|||	    buffered message, the value of the field is MESSAGE_PASS_BY_VALUE.
|||	    If neither bit is set, the message is a standard message.
|||
|||	    If the message is a standard message, the dataptr and datasize
|||	    arguments refer to a data block that your task allocates for
|||	    returning reply data. For standard messages, the sending task and
|||	    the replying task must each allocate their own memory blocks for
|||	    message data. If the message is buffered, the data is copied into
|||	    the internal buffer of the message. To reply a small message, use
|||	    the ReplySmallMsg() function.
|||
|||	    If a task has gotten messages and proceeds to exit without replying
|||	    them, the kernel will automatically reply the messages on behalf of
|||	    the task. The messages will have a result code of KILLED.
|||
|||	  Arguments
|||
|||	    msg
|||	        The message to reply, as gotten from GetMsg() or
|||	        GetThisMsg().
|||
|||	    result
|||	        A result code. This code is placed in the msg_Result field of
|||	        the Message data structure before the reply is sent. The
|||	        meaning of this code is undefined from the system's
|||	        perspective; the code must be a value whose meaning is agreed
|||	        upon by the calling and receiving tasks. In general, you should
|||	        use negative values to specify errors and 0 to specify that no
|||	        error occurred.
|||
|||	    dataptr
|||	        See the "Description" section above for a description of this
|||	        argument.
|||
|||	    datasize
|||	        See the "Description" section above for a description of this
|||	        argument.
|||
|||	  Return Value
|||
|||	    >= 0 if the reply was sent successfully or a negative error
|||	    code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/msgport.h>, libc.a
|||
|||	  See Also
|||
|||	    GetMsg(), GetThisMsg(), ReplySmallMsg(), SendMsg(), SendSmallMsg(),
|||	    WaitPort()
|||
**/

/**
|||	AUTODOC -class Kernel -group Messaging -name ReplySmallMsg
|||	Sends a reply to a small message.
|||
|||	  Synopsis
|||
|||	    Err ReplySmallMsg(Item msg, int32 result, uint32 val1, uint32 val2);
|||
|||	  Description
|||
|||	    This function sends a reply to a small message.
|||
|||	    When a message is first created, a reply port may be provided by
|||	    the creator. When you reply a message, it is like sending it to the
|||	    reply port.
|||
|||	    When replying a message, you supply a result code which typically
|||	    indicates whether the operation triggered by the message was
|||	    successful.
|||
|||	    If a task has gotten messages and proceeds to exit without replying
|||	    them, the kernel will automatically reply the messages on behalf of
|||	    the task. The messages will have a result code of KILLED.
|||
|||	  Arguments
|||
|||	    msg
|||	        The message to reply, as gotten from GetMsg() or
|||	        GetThisMsg().
|||
|||	    result
|||	        A result code. This code is placed in the msg_Result field of
|||	        the Message data structure before the reply is sent. The
|||	        meaning of this code is undefined from the system's
|||	        perspective; the code must be a value whose meaning is agreed
|||	        upon by the calling and receiving tasks. In general, you should
|||	        use negative values to specify errors and 0 to specify that no
|||	        error occurred.
|||
|||	    val1
|||	        The first four bytes of data for the reply. This data is put
|||	        into the msg_DataPtr field of the message structure.
|||
|||	    val2
|||	        The last four bytes of data for the reply. This data is put
|||	        into the msg_DataSize field of the message structure.
|||
|||	  Return Value
|||
|||	    >= 0 if the reply was sent successfully or a negative error
|||	    code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/msgport.h>, libc.a
|||
|||	  See Also
|||
|||	    GetMsg(), GetThisMsg(), ReplyMsg(), SendMsg(), SendSmallMsg(),
|||	    WaitPort()
|||
**/

/* WARNING: Can be called from interrupt code, typically as a result of
 *          CompleteIO().
 */

Err internalReplyMsg(Message *msg, int32 result, void *dataptr, int32 datasize)
{
MsgPort *port;

    port = (MsgPort *)CheckItem(msg->msg_ReplyPort,KERNELNODE,MSGPORTNODE);
    if (!port)
    {
        /* bad reply port */
        return NOREPLYPORT;
    }

    msg->msg_Result = result;

    return internalDeliverMsg(port,msg,dataptr,datasize,TRUE, FALSE);
}


/*****************************************************************************/


Err externalReplyMsg(Item msgItem, int32 result, void *dataptr, int32 datasize)
{
Message *msg;
Task    *task;

    DBUGRM(("ReplyMsg($%x,$%x,$%x,$%x)\n",msgItem,result,dataptr,datasize));

    msg = (Message *)CheckItem(msgItem,KERNELNODE,MESSAGENODE);
    if (!msg)
    {
        /* not a valid message item */
        return BADITEM;
    }

    if (msg->msg_ReplyPort == 0)
    {
        /* can't reply if there's no reply port */
 	return REPLYPORTNEEDED;
    }

    if (msg->msg.n_Flags & MESSAGE_PASS_BY_VALUE)
    {
        /* Validate incoming message data size */
        if (datasize > msg->msg_DataPtrSize)
        {
            INFO(("WARNING: bad datasize for ReplyMsg() (datasize=%d, maximum=%d)\n",datasize,msg->msg_DataPtrSize));
            return BADSIZE;
        }

        /* Validate incoming message data ptr */
        if (!IsMemReadable(dataptr,datasize))
        {
            INFO(("WARNING: ReplyMsg() with bad data (dataptr=$%x, datasize=%d)\n", dataptr, datasize));
            return BADPTR;
        }
    }
    else if ((msg->msg.n_Flags & MESSAGE_SMALL) == 0)
    {
        /* Validate incoming message data ptr */
        if (!IsMemReadable(dataptr,datasize))
        {
            INFO(("WARNING: ReplyMsg() with bad data (dataptr=$%x, datasize=%d)\n", dataptr, datasize));
            return BADPTR;
        }
    }

    if (msg->msg_Holder != CURRENTTASKITEM)
    {
        /* the current task is not the current holder of the message */
        task = (Task *)CheckItem(msg->msg_Holder,KERNELNODE,TASKNODE);
        if (!task)
        {
            /* The message is already on a message port, since msg_Holder
             * is not a task item.
             */
            return MSGSENT;
        }

        if (!IsSameTaskFamily(task,CURRENTTASK))
        {
            /* Can only reply a message held by a member of the same
             * task family.
             */
            return NOTOWNER;
        }
    }

    return internalReplyMsg(msg,result,dataptr,datasize);
}


/**
|||	AUTODOC -class Kernel -group Messaging -name GetThisMsg
|||	Gets a specific message.
|||
|||	  Synopsis
|||
|||	    Item GetThisMsg(Item msg);
|||
|||	  Description
|||
|||	    This function gets a specific message and removes it from the
|||	    message port it is currently on.
|||
|||	    A task that is receiving messages can use GetThisMsg() to get an
|||	    incoming message. Unlike GetMsg() which gets the first message on a
|||	    specific message port, GetThisMsg() can get any message from any of
|||	    the task's message ports.
|||
|||	    A task that has sent a message can use GetThisMsg() to get it back.
|||	    If the receiving task hasn't already taken the message from its
|||	    message port, GetThisMsg() removes it from the port. If the message
|||	    is not on any port an error is returned.
|||
|||	    In order to get a message, one or more of the following must be
|||	    true:
|||
|||	      - The current task owns the message.
|||
|||	      - The current task owns the port the message is on.
|||
|||	      - The current task is a member of the same task family as the
|||	        owner of the message.
|||
|||	      - The current task is a member of the same task family as the
|||	        owner of the port the message is on.
|||
|||	    If a task has gotten messages and proceeds to exit without replying
|||	    them, the kernel will automatically reply the messages on behalf of
|||	    the task. The messages will have a result code of KILLED.
|||
|||	  Arguments
|||
|||	    msg
|||	        The item number of the message to get.
|||
|||	  Return Value
|||
|||	    The item number of the message or a negative error code for
|||	    failure. Possible error codes currently include:
|||
|||	    BADITEM
|||	        The message argument does not specify a message.
|||
|||	    BADPRIV
|||	        The calling task is not allowed to get this message.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/msgport.h>, libc.a
|||
|||	  Caveats
|||
|||	    Prior to V21, if the message was not on any message port this
|||	    function still returned the item number of the message. In V21
|||	    and beyond, if the message is not on any port, the function returns
|||	    an error.
|||
|||	  See Also
|||
|||	    CreateMsg(), GetMsg(), ReplyMsg(), ReplySmallMsg(), SendMsg(),
|||	    SendSmallMsg(), WaitPort()
|||
**/

Item internalGetThisMsg(Message *msg)
{
Item     result;
uint32   oldints;
MsgPort *port;
Task    *ct;

    DBUGGM(("GetThisMsg msg =%lx\n",msg));

    ct = CURRENTTASK;

    port = (MsgPort *)CheckItem(msg->msg_Holder,KERNELNODE,MSGPORTNODE);
    if (port)
    {
        /* In order to get a message, one of the following must be true:
         *
         *   - The current task owns the message.
         *
         *   - The current task owns the port the message is on.
         *
         *   - The current task is a member of the same task family as the owner
         *     of the message.
         *
         *   - The current task is a member of the same task family as the owner
         *     of the port the message is on.
         */

        if ((ct->t.n_Item == msg->msg.n_Owner)
         || (ct->t.n_Item == port->mp.n_Owner)
         || (IsSameTaskFamily(ct,(Task *)LookupItem(msg->msg.n_Owner)))
         || (IsSameTaskFamily(ct,(Task *)LookupItem(port->mp.n_Owner))))
        {
            LogMessageGotten(msg,port);

            oldints = Disable();
            msg->msg.n_Flags &= ~(MESSAGE_SENT | MESSAGE_REPLIED);
            REMOVENODE((Node *)msg);
            ADDTAIL(&ct->t_MessagesHeld,(Node *)msg);
            Enable(oldints);

            msg->msg_Holder = ct->t.n_Item;
            result          = msg->msg.n_Item;
        }
        else
        {
            result = NOTOWNER;
        }
    }
    else
    {
        /* message is not on a message port */
	result = MSGNOTONPORT;
    }

    return result;
}


/*****************************************************************************/


Item externalGetThisMsg(Item msgItem)
{
Message *msg;

    msg = (Message *)CheckItem(msgItem,KERNELNODE,MESSAGENODE);
    if (!msg)
    {
        /* not a valid message item */
        return BADITEM;
    }

    return internalGetThisMsg(msg);
}


/*****************************************************************************/


/**
|||	AUTODOC -class Kernel -group Messaging -name GetMsg
|||	Gets a message from a message port.
|||
|||	  Synopsis
|||
|||	    Item GetMsg(Item msgPort);
|||
|||	  Description
|||
|||	    This function gets the first message in the message queue for the
|||	    specified message port and removes the message from the queue.
|||
|||	    Once you have gotten a message, you can do a number of things. If
|||	    the message is a reply to a message you sent out previously, you
|||	    can either reuse the message, or delete it. If this a message
|||	    being sent to you by another task, you can use LookupItem() to
|||	    obtain a pointer to the Message structure and look at the
|||	    various field therein to get more information.
|||
|||	    If a task has gotten messages and proceeds to exit without replying
|||	    them, the kernel will automatically reply the messages on behalf of
|||	    the task. The messages will have a result code of KILLED.
|||
|||	  Arguments
|||
|||	    msgPort
|||	        The item number of the message port from which to get the message.
|||
|||	  Return Value
|||
|||	    The item number of the first message in the message queue or a
|||	    negative error code for failure. Possible error codes currently
|||	    include:
|||
|||	    0
|||	        The queue is empty.
|||
|||	    BADITEM
|||	        The mp argument does not specify a message port.
|||
|||	    NOTOWNER
|||	        The message port specified by the mp
|||	        argument is not owned by this task.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/msgport.h>, libc.a
|||
|||	  See Also
|||
|||	    DeleteMsg(), GetThisMsg(), ReplyMsg(), ReplySmallMsg(), SendMsg(),
|||	    SendSmallMsg(), WaitPort()
|||
**/

Item internalGetMsg(MsgPort *port)
{
Item     result;
uint32   oldints;
Message *msg;

    DBUGGM(("internalGetMsg: port $%x\n",port));

    oldints = Disable();
    msg = (Message *)RemHead(&port->mp_Msgs);

    if (msg)
    {
        LogMessageGotten(msg,port);

        ADDTAIL(&CURRENTTASK->t_MessagesHeld,(Node *)msg);
        msg->msg.n_Flags &= ~(MESSAGE_SENT | MESSAGE_REPLIED);
        Enable(oldints);

        msg->msg_Holder = CURRENTTASKITEM;
        result          = msg->msg.n_Item;
    }
    else
    {
        Enable(oldints);

        /* no message on the port */
        result = 0;
    }

    DBUGGM(("internalGetMsg: returning $%x\n",result));

    return result;
}


/*****************************************************************************/


Item externalGetMsg(Item portItem)
{
MsgPort *port;

    port = (MsgPort *)CheckItem(portItem,KERNELNODE,MSGPORTNODE);
    if (!port)
    {
        /* not a valid message port item */
        return BADITEM;
    }

    if (port->mp.n_Owner != CURRENTTASKITEM)
    {
        /* can't get a message if we're not the owner of the port */
        return NOTOWNER;
    }

    return internalGetMsg(port);
}

/**
|||	AUTODOC -class Kernel -group Messaging -name WaitPort
|||	Waits for a message to arrive at a message port.
|||
|||	  Synopsis
|||
|||	    Item WaitPort(Item msgPort, Item msg);
|||
|||	  Description
|||
|||	    The function puts the calling task into wait state until a message
|||	    arrives at the specified message port. When a task is in wait state,
|||	    it uses no CPU time.
|||
|||	    The task can wait for a specific message by providing the item number
|||	    of the message as the msg argument. To wait for any incoming message,
|||	    the task uses 0 as the value of the msg argument.
|||
|||	    If the desired message is already in the message queue for the
|||	    specified message port, the function returns immediately. If the
|||	    message never arrives, the function may never return.
|||
|||	    When the message arrives, the task is moved from the wait queue to
|||	    the ready queue, the item number of the message is returned as the
|||	    result, and the message is removed from the message port.
|||
|||	    If a task has gotten messages and proceeds to exit without replying
|||	    them, the kernel will automatically reply the messages on behalf of
|||	    the task. The messages will have a result code of KILLED.
|||
|||	  Arguments
|||
|||	    msgPort
|||	        The item number of the message port to check for incoming messages.
|||
|||	    msg
|||	        The item number of the message to wait for, or 0 if
|||	        any message will do.
|||
|||	  Return Value
|||
|||	    The item number of the incoming message or a negative error code
|||	    for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/msgport.h>, libc.a
|||
|||	  See Also
|||
|||	    GetMsg(), ReplyMsg(), SendMsg()
|||
**/

Item internalWaitPort(MsgPort *port, Message *msg)
{
Item   result;
uint32 oldints;

    DBUGWP(("internalWaitPort: port $%x, msg $%x\n",port,msg));

    oldints = Disable();

    if (msg)
    {
        /* we want to wait for a particular message to arrive */

        if (msg->msg_Holder == port->mp.n_Item)
        {
            /* the message is already on the port, yank it */
            Enable(oldints);
            return internalGetThisMsg(msg);
        }

        /* the message is not on this port, wait for it... */
        LogMessageWait(msg,port);
        CURRENTTASK->t_WaitItem = msg->msg.n_Item;
    }
    else
    {
        /* we want any message that comes into the port */

        if (IsEmptyList(&port->mp_Msgs) == 0)
        {
            /* there are messages on this port */
            Enable(oldints);
            return internalGetMsg(port);
        }

        /* no messages on the port, wait for some... */
        LogMessageWait(NULL,port);
        CURRENTTASK->t_WaitItem = port->mp.n_Item;
    }

    SleepTask(CURRENTTASK);
    result = CURRENTTASK->t_WaitItem;
    CURRENTTASK->t_WaitItem = -1;
    Enable(oldints);

#ifdef BUILD_LUMBERJACK
    if (result >= 0)
        LogMessageGotten(MESSAGE(result),port);
#endif

    return result;
}


/*****************************************************************************/


Item externalWaitPort(Item portItem, Item msgItem)
{
Message *msg;
MsgPort *port;

    DBUGWP(("externalWaitPort: portItem $%x, msgItem $%x\n",portItem,msgItem));

    /* check the message port */
    port = (MsgPort *)CheckItem(portItem,KERNELNODE,MSGPORTNODE);
    if (!port)
        return BADITEM;

    /* can only wait if we're the owner of the port */
    if (port->mp.n_Owner != CURRENTTASKITEM)
        return NOTOWNER;

    /* check the message item */
    if (msgItem != 0)
    {
        msg = (Message *)CheckItem(msgItem,KERNELNODE,MESSAGENODE);
        if (!msg)
            return BADITEM;
    }
    else
    {
        msg = NULL;
    }

    return internalWaitPort(port,msg);
}


/*****************************************************************************/


static int32 icmp_c(MsgPort *mp, void *p, uint32 tag, uint32 arg)
{
    TOUCH(p);

    switch (tag)
    {
        case  CREATEPORT_TAG_SIGNAL  : mp->mp_Signal = (uint32)arg;
                                       break;

        case  CREATEPORT_TAG_USERDATA: mp->mp_UserData = (void *)arg;
                                       break;

        default                      : return BADTAG;
    }

    return 0;
}


/*****************************************************************************/


/**
|||	AUTODOC -class Kernel -group Messaging -name CreateMsgPort
|||	Creates a message port.
|||
|||	  Synopsis
|||
|||	    Item CreateMsgPort(const char *name, uint8 pri, int32 sigMask);
|||
|||	  Description
|||
|||	    This function creates a message port item, which is used
|||	    to receive messages from other tasks and threads.
|||
|||	    You usually give a name to your message ports. Named ports
|||	    can be used as a rendez-vous between tasks, since other tasks
|||	    are able to find the message by name by using FindMsgPort().
|||	    The priority value you supply to this function is used to
|||	    determine the sorting order of message ports in the list of
|||	    ports maintained by the kernel. When you call FindMsgPort(),
|||	    if there are multiple ports with the desired name, the one with
|||	    the highest priority will be returned. See the CreateUniqueMsgPort()
|||	    function for a way to create a message port with a name that is
|||	    guaranteed to be unique.
|||
|||	    When you no longer need a message port you use DeleteMsgPort() to
|||	    delete it.
|||
|||	  Arguments
|||
|||	    name
|||	        Optional name of the message port, or NULL if the port should
|||	        be unnamed. Unnamed ports cannot be found with FindMsgPort().
|||
|||	    pri
|||	        The priority of the message port. If you don't care,
|||	        supply 0 for this argument.
|||
|||	    sigMask
|||	        Whenever a message arrives at a message port, the kernel
|||	        sends a signal to the owner of the message port. This
|||	        argument lets you specify which signals the kernel should
|||	        send to your task whenever a message arrives at the port
|||	        being created. If you supply 0 for this argument, the kernel
|||	        will allocate a signal bit automatically, and will free this
|||	        bit when the message port is later deleted. You can look
|||	        at the mp_Signal field of the MsgPort structure to determine
|||	        which signals are sent when a message arrives at the port.
|||
|||	  Return Value
|||
|||	    The item number of the new message port or a negative error
|||	    code for failure.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libc.a V20.
|||
|||	  Associated Files
|||
|||	    <kernel/msgport.h>, libc.a
|||
|||	  See Also
|||
|||	    CreateMsg(), DeleteMsg(), DeleteMsgPort(), SendMsg(), SendSmallMsg()
|||
**/

/**
|||	AUTODOC -class Kernel -group Messaging -name CreateUniqueMsgPort
|||	Creates a message port with a unique name.
|||
|||	  Synopsis
|||
|||	    Item CreateUniqueMsgPort(const char *name, uint8 pri, int32 sigMask);
|||
|||	  Description
|||
|||	    This function works like CreateMsgPort(), except that it guarantees
|||	    that no other message port item of the same name already exists,
|||	    and that once this port created, no other port of the same name
|||	    will be allowed to be created.
|||
|||	    Refer to the CreateMsgPort() documentation for more information.
|||
|||	  Arguments
|||
|||	    name
|||	        The name of the message port.
|||
|||	    pri
|||	        The priority of the message port.
|||
|||	    sigMask
|||	        The signal to send when a message arrives at the port.
|||
|||	  Return Value
|||
|||	    The item number of the new message port or a negative error
|||	    code for failure. If a port of the same name already existed when
|||	    this call was made, the UNIQUEITEMEXISTS error will be
|||	    returned.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libc.a V24.
|||
|||	  Associated Files
|||
|||	    <kernel/msgport.h>, libc.a
|||
|||	  See Also
|||
|||	    CreateMsg(), DeleteMsg(), DeleteMsgPort(), SendMsg(), SendSmallMsg()
|||
**/

/**
|||	AUTODOC -class Items -name MsgPort
|||	An item through which a task receives messages.
|||
|||	  Description
|||
|||	    A message port is an item through which a task receives messages.
|||
|||	  Folio
|||
|||	    Kernel
|||
|||	  Item Type
|||
|||	    MSGPORTNODE
|||
|||	  Create
|||
|||	    CreateMsgPort(), CreateUniqueMsgPort(), CreateItem()
|||
|||	  Delete
|||
|||	    DeleteMsgPort(), DeleteItem()
|||
|||	  Query
|||
|||	    FindMsgPort(), FindItem(), FindNamedItem()
|||
|||	  Use
|||
|||	    WaitPort(), GetMsg(), SendMsg(), SendSmallMsg(), ReplyMsg(),
|||	    ReplySmallMsg()
|||
|||	  Tags
|||
|||	    CREATEPORT_TAG_SIGNAL (int32) - Create
|||	        Whenever a message arrives at a message port, the kernel sends
|||	        a signal to the owner of the message port. This tag lets you
|||	        specify which signal should be sent. If this tag is not
|||	        provided, the kernel will automatically allocate a signal for
|||	        the message port.
|||
|||	    CREATEPORT_TAG_USERDATA (void *) - Create
|||	        Lets you specify the 32-bit value that gets put into the
|||	        mp_UserData field of the MsgPort structure. This can be
|||	        anything you want, and is sometimes useful to idenify a
|||	        message port between tasks.
|||
**/

Item internalCreateMsgPort(MsgPort *mp, TagArg *tagpt)
{
	/* Create a message Port */
	Item ret;

	ret = TagProcessor(mp, tagpt, icmp_c, 0);
	if (ret < 0) return ret;

	if (mp->mp_Signal == 0)
	{
		mp->mp_Signal = externalAllocSignal(0);
		if (!mp->mp_Signal)
			return MAKEKERR(ER_SEVER,ER_C_STND,ER_NoSignals);

		mp->mp.n_Flags |= MSGPORT_SIGNAL_ALLOCATED;
	}
	else
	{
		ret = ValidateSignal(mp->mp_Signal);
		if (ret < 0) return ret;
	}

	PrepList(&mp->mp_Msgs);

	/* Add to list of public ports */
	InsertNodeFromTail(&KB_FIELD(kb_MsgPorts),(Node *)mp);

	return  mp->mp.n_Item;
}

/**
|||	AUTODOC -class Kernel -group Messaging -name DeleteMsg
|||	Deletes a message.
|||
|||	  Synopsis
|||
|||	    Err DeleteMsg(Item msg);
|||
|||	  Description
|||
|||	    Deletes the specified message and any resources that were
|||	    allocated for it.
|||
|||	  Arguments
|||
|||	    msg
|||	        The item number of the message to be deleted.
|||
|||	  Return Value
|||
|||	    >= 0 if successful or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Macro implemented in <kernel/msgport.h> V20.
|||
|||	  Associated Files
|||
|||	    <kernel/msgport.h>, libc.a
|||
|||	  See Also
|||
|||	    CreateMsg()
|||
**/

Err internalDeleteMsg(Message *msg, Task *t)
{
	uint32 oldints;
	DBUGDEL(("DeleteMsg(%lx,%lx)\n",msg,(uint32)ct));

	TOUCH(t);

	oldints = Disable();

        /* remove from either a port's list, or a task's held message list */
	REMOVENODE((Node *)msg);

        /* Mark as being in use, which will prevent this message
         * from being sent or replied while it is being deleted.
         */
	msg->msg.n_Flags |= MESSAGE_SENT;
	Enable(oldints);

	{
	List *l = &KB_FIELD(kb_Tasks);
	Node *n;
	Task *task;
	uint32 oldints;

            /* If there are folks waiting on this message, notify them that
             * the message just went *poof*
             */

	    ScanList(l,n,Node)
	    {
		task = Task_Addr(n);

		if (task->t_WaitItem == msg->msg.n_Item)
		{
		    task->t_WaitItem = ABORTED;
		    oldints = Disable();
		    NewReadyTask(task);
		    Enable(oldints);
		}
	    }
	}

	return 0;
}


/**
|||	AUTODOC -class Kernel -group Messaging -name DeleteMsgPort
|||	Deletes a message port.
|||
|||	  Synopsis
|||
|||	    Err DeleteMsgPort(Item msgPort)
|||
|||	  Description
|||
|||	    Deletes the specified message port. Any messages still
|||	    left on the port are replied with a result code of BADITEM.
|||
|||	  Arguments
|||
|||	    msgPort
|||	        The item number of the message port to be deleted.
|||
|||	  Return Value
|||
|||	    >= 0 if successful or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Macro implemented in <kernel/msgport.h> V20.
|||
|||	  Associated Files
|||
|||	    <kernel/msgport.h>, libc.a
|||
|||	  See Also
|||
|||	    CreateMsg(), CreateMsgPort()
|||
**/

Err internalDeleteMsgPort(MsgPort *port, Task *task)
{
Message *msg;
Item     msgItem;

    DBUGDEL(("DeleteMsgPort(%d,%lx)\n",mp,(uint32)ct));

    REMOVENODE((Node *)port);

    while (TRUE)
    {
        msgItem = internalGetMsg(port);
        if (msgItem == 0)
            break;

        msg = (Message *)LookupItem(msgItem);
        if (msg->msg.n_Owner != task->t.n_Item)
	{
	    /* return to sender */
            internalReplyMsg(msg,BADITEM,NULL,0);
        }
    }

    task = (Task *)LookupItem(port->mp.n_Owner);
    if (task)
    {
#ifdef BUILD_PARANOIA
        if ((port->mp_Signal & task->t_AllocatedSigs) != port->mp_Signal)
        {
            printf("WARNING: signal bits freed before the message port was deleted!\n");
            printf("         (port name '%s', port bits $%08lx, task bits $%08lx)\n",
                   (port->mp.n_Name ? port->mp.n_Name : "<null>"), port->mp_Signal,task->t_AllocatedSigs);
        }
#endif

        if (port->mp.n_Flags & MSGPORT_SIGNAL_ALLOCATED)
            internalFreeSignal(port->mp_Signal,task);
    }

    return 0;
}


/*****************************************************************************/


Err internalSetMsgPortOwner(MsgPort *port, Item newOwner)
{
Task  *task;
int32  sigs;
Err    result;
int32  oldsigs;
uint32 oldints;

    /* Only allow the ownership to be changed when it is a port with an
     * automatically allocated signal, and there are no messages in the
     * message queue.
     */

    if ((port->mp.n_Flags & MSGPORT_SIGNAL_ALLOCATED) == 0)
    {
        /* can't change ownership if not automatic signal */
        return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_CantSetOwner);
    }

    task = (Task *)LookupItem(newOwner);
    sigs = internalAllocSignal(0,task);
    if (!sigs)
        sigs = MAKEKERR(ER_SEVER,ER_C_STND,ER_NoSignals);

    if (sigs < 0)
    {
        /* can't change ownership if we can't allocate signals for the recipient task */
        return sigs;
    }

    oldsigs = port->mp_Signal;

    oldints = Disable();
    if (IsEmptyList(&port->mp_Msgs))
    {
        port->mp_Signal = sigs;
        result = 0;
    }
    else
    {
        /* port not empty, can't change ownership */
        result = MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_MsgPortNotEmpty);
    }
    Enable(oldints);

    if (result >= 0)
        externalFreeSignal(oldsigs);
    else
        internalFreeSignal(sigs,task);

    return result;
}


/*****************************************************************************/


static int32 icm_c(Message *msg, void *p, uint32 tag, uint32 arg)
{
    TOUCH(p);

    switch (tag)
    {
        case CREATEMSG_TAG_REPLYPORT   : msg->msg_ReplyPort = (Item)arg;
                                         break;

        case CREATEMSG_TAG_MSG_IS_SMALL: msg->msg.n_Flags |= MESSAGE_SMALL;
                                         break;

        case CREATEMSG_TAG_DATA_SIZE   : msg->msg_DataPtrSize = (long)arg;
                                         msg->msg_DataPtr     = msg+1;
                                         msg->msg.n_Flags    |= MESSAGE_PASS_BY_VALUE;
                                         break;

        case CREATEMSG_TAG_USERDATA    : msg->msg_UserData = (void *)arg;
                                         break;

        default                        : return BADTAG;
    }

    return 0;
}

/**
|||	AUTODOC -class Kernel -group Messaging -name CreateMsg
|||	Creates a standard message.
|||
|||	  Synopsis
|||
|||	    Item CreateMsg(const char *name, uint8 pri, Item msgPort);
|||
|||	  Description
|||
|||	    One of the ways tasks communicate is by sending messages to
|||	    other. This function creates a standard message. Standard
|||	    messages require that data data to be communicated to the
|||	    receiving task be contained in memory belonging to the sending
|||	    task.
|||
|||	    When you create a message, you can provide an optional reply port.
|||	    Any task you send the message to will be able to reply it to you
|||	    by calling ReplyMsg(). If you don't supply a reply port, then
|||	    the act of sending the message to another task will also transfer
|||	    the ownership of the message to the receiving task.
|||
|||	  Arguments
|||
|||	    name
|||	        Optional name for the message. Use NULL for no name.
|||
|||	    pri
|||	        The priority of the message. This determines the position of
|||	        the message in the receiving task's message queue and thus,
|||	        how soon it is likely to be handled. A larger number
|||	        specifies a higher priority.
|||
|||	    msgPort
|||	        The item number of the message port at which to receive the
|||	        reply, or 0 for no reply port.
|||
|||	  Return Value
|||
|||	    The item number of the message or a negative error code for
|||	    failure.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libc.a V20.
|||
|||	  Associated Files
|||
|||	    <kernel/msgport.h>, libc.a
|||
|||	  See Also
|||
|||	    CreateMsgPort(), CreateBufferedMsg(), CreateSmallMsg(), DeleteMsg(),
|||	    DeleteMsgPort(), SendMsg()
|||
**/

/**
|||	AUTODOC -class Items -name Message
|||	The means of sending data from one task to another.
|||
|||	  Description
|||
|||	    Messages are used to communicate amongst tasks. Messages come in three
|||	    flavors: small, standard, and buffered. The flavors each carry their
|||	    own type or amount of information. The small message carries exactly
|||	    eight bytes of data. Standard messages contain a pointer to some data.
|||	    Buffered messages contain an arbitrary amount of data within them.
|||
|||	    Messages are closely associated with message ports. In order to send
|||	    a message, a task must indicate a destination message port.
|||	    Essentially, messages shuttle back and forth between message ports,
|||	    and tasks can get and put messages from and to message ports.
|||
|||	  Folio
|||
|||	    Kernel
|||
|||	  Item Type
|||
|||	    MESSAGENODE
|||
|||	  Create
|||
|||	    CreateMsg(), CreateSmallMsg(), CreateBufferedMsg(), CreateItem()
|||
|||	  Delete
|||
|||	    DeleteMsg(), DeleteItem()
|||
|||	  Query
|||
|||	    FindItem(), FindNamedItem()
|||
|||	  Use
|||
|||	    ReplyMsg(), ReplySmallMsg(), SendMsg(), SendSmallMsg(), GetThisMsg(),
|||	    GetMsg(), WaitPort()
|||
|||	  Tags
|||
|||	    CREATEMSG_TAG_REPLYPORT (Item) - Create
|||	        The item number of the message port to use when replying this
|||	        message. This must be a port that was created by the same task
|||	        or thread that is creating the message. If you do not supply
|||	        this tag, then it will not be possible to reply this
|||	        message. Therefore, the act of sending a message with no reply
|||	        port also transfers the ownership of the message to the
|||	        receiving task.
|||
|||	    CREATEMSG_TAG_MSG_IS_SMALL (void) - Create
|||	        When this tag is present, it specifies that this message
|||	        should be small. This means that this message should be used
|||	        with SendSmallMsg() and ReplySmallMsg(), and can only pass 8
|||	        bytes of information.
|||
|||	    CREATEMSG_TAG_DATA_SIZE (uint32) - Create
|||	        When this tag is present, it specifies that the message should
|||	        be buffered. The argument of this tag specifies the size of the
|||	        buffer that should be allocated.
|||
|||	    CREATEMSG_TAG_SIGNAL (int32) - Create
|||	        Whenever a message arrives at a message port, the kernel sends
|||	        a signal to the owner of the message port. This tag lets you
|||	        specify which signal should be sent. If this tag is not
|||	        provided, the kernel will automatically allocate a signal for
|||	        the message port.
|||
|||	    CREATEMSG_TAG_USERDATA (void *) - Create
|||	        Lets you specify the 32-bit value that gets put into the
|||	        msg_UserData field of the Message structure. This can be
|||	        anything you want, and is sometimes useful to idenify a
|||	        message.
**/

Item externalCreateMsg(Message *mdummy, TagArg *tagpt)
{
	Task *ct = CURRENTTASK;
	Message  *msg;
	Item ret;
	uint32 oldints;

	DBUG(("eCreateMsg(%lx,%lx)\n",(long)mdummy,(long)tagpt));

	if (mdummy != (Message *)-1)
	    return BADSIZE;

	{
	    TagArg tagreturn;
	    int32  size = sizeof(Message);

	    ret= TagProcessorSearch(&tagreturn, tagpt, CREATEMSG_TAG_DATA_SIZE);
	    if (ret < 0) return ret;

	    if (ret)
	    {
		size += (int32)tagreturn.ta_Arg;
		if (size < sizeof(Message))	return BADTAGVAL;
	    }

	    msg= (Message *)AllocateSizedNode((Folio *)&KB,MESSAGENODE,size);
	    if (msg == NULL) return NOMEM;
	}

	ret = TagProcessor(msg, tagpt, icm_c, 0);
	if (ret < 0) goto err;

	/* validation */

	/* a message can't be both small and pass by value */
	if ((msg->msg.n_Flags & (MESSAGE_SMALL|MESSAGE_PASS_BY_VALUE)) ==
	    (MESSAGE_SMALL|MESSAGE_PASS_BY_VALUE))
	{
	    ret = BADTAGVAL;
	    goto err;
	}

	if (msg->msg_ReplyPort)
	{
	    MsgPort *mp;

	    mp = (MsgPort *)CheckItem(msg->msg_ReplyPort,KERNELNODE,MSGPORTNODE);
	    if (!mp)
	    {
		ret = BADITEM;
		goto err;
	    }
	}

	/* the creator is the initial holder of the message */
	oldints = Disable();
        ADDTAIL(&ct->t_MessagesHeld,(Node *)msg);
        Enable(oldints);
	msg->msg_Holder = ct->t.n_Item;

	return msg->msg.n_Item;
err:
	FreeNode((Folio *)&KB, msg);
	return ret;
}


/*****************************************************************************/


Err internalSetMsgOwner(Message *msg, Item newOwner)
{
uint32 oldints;

    /* Only allow the ownership to be changed when the owner holds the
     * message.
     */

    TOUCH(newOwner);

    if (msg->msg_Holder != CURRENTTASKITEM)
        return MSGSENT;

    /* Make it so the new owner will hold the message, otherwise we'd end up
     * with a strange tangled web of goop.
     */
    oldints = Disable();
    msg->msg_Holder = newOwner;
    REMOVENODE((Node *)msg);
    ADDTAIL(&TASK(newOwner)->t_MessagesHeld, (Node *)msg);
    Enable(oldints);

    return 0;
}


/*****************************************************************************/


/* This function is called when a task is dying. Its purpose in life
 * is to reply any messages that a task has gotten with GetMsg() or
 * GetThisMsg(), but didn't get a chance to reply to yet.
 *
 * Since this call is made after all of the task's items have been
 * deleted, we know that the only thing left in t_MessagesHeld are messages
 * that need to be replied.
 */
void ReplyHeldMessages(const Task *t)
{
Message *msg;
Err      result;

again:

    ScanList(&t->t_MessagesHeld, msg, Message)
    {
        /* return to sender */
        result = internalReplyMsg(msg,KILLED,NULL,0);
        if (result >= 0)
            goto again;
    }
}
