
/******************************************************************************
**
**  @(#) msgpassing.c 95/10/15 1.6
**
******************************************************************************/

/**
|||	AUTODOC -public -class examples -name msgpassing
|||	Demonstrates sending and receiving messages between two threads.
|||
|||	  Synopsis
|||
|||	    msgpassing
|||
|||	  Description
|||
|||	    Demonstrates how to send messages between threads or tasks.
|||
|||	    The main() routine of the program creates a message port where it
|||	    can receive messages. It then spawns a thread. This thread creates
|||	    its own message port, and a message. The thread then sends the
|||	    message to the parent's message port. Once the parent receives the
|||	    message, it replies it to the thread.
|||
|||	  Associated Files
|||
|||	    msgpassing.c
|||
|||	  Location
|||
|||	    examples/Kernel
|||
**/

#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/task.h>
#include <kernel/msgport.h>
#include <kernel/operror.h>
#include <stdio.h>


/*****************************************************************************/


/* a signal mask used to sync the thread with the parent */
int32 parentSig;


/*****************************************************************************/


static void ThreadFunction(void)
{
Item     childPortItem;
Item     childMsgItem;
Item     parentPortItem;
Err      err;
Message *msg;

    printf("Child thread is running\n");

    childPortItem = CreateMsgPort("ChildPort",0,0);
    if (childPortItem >= 0)
    {
        childMsgItem = CreateSmallMsg("ChildMsg",0,childPortItem);
        if (childMsgItem >= 0)
        {
            parentPortItem = FindMsgPort("ParentPort");
            if (parentPortItem >= 0)
            {
                /* tell the parent we're done initializing */
                SendSignal(CURRENTTASK->t_ThreadTask->t.n_Item,parentSig);

                err = SendSmallMsg(parentPortItem,childMsgItem,12,34);
                if (err >= 0)
                {
                    err = WaitPort(childPortItem,childMsgItem);
                    if (err >= 0)
                    {
                        msg = MESSAGE(childMsgItem);
                        printf("Child received reply from parent: ");
                        printf("msg_Result %d, msg_DataPtr %d, msg_DataSize %d\n",
                               msg->msg_Result, msg->msg_DataPtr, msg->msg_DataSize);
                    }
                    else
                    {
                        printf("WaitPort() failed: ");
                        PrintfSysErr(err);
                    }
                }
                else
                {
                    printf("SendSmallMsg() failed: ");
                    PrintfSysErr(err);
                }

                SendSignal(CURRENTTASK->t_ThreadTask->t.n_Item,parentSig);
            }
            else
            {
                printf("Could not find parent message port: ");
                PrintfSysErr(parentPortItem);
            }
            DeleteMsg(childMsgItem);
        }
        else
        {
            printf("CreateSmallMsg() failed: ");
            PrintfSysErr(childMsgItem);
        }
        DeleteMsgPort(childPortItem);
    }
    else
    {
        printf("CreateMsgPort() failed: ");
        PrintfSysErr(childPortItem);
    }
}


/*****************************************************************************/


int main(void)
{
Item     portItem;
Item     threadItem;
Item     msgItem;
Message *msg;

    parentSig = AllocSignal(0);
    if (parentSig > 0)
    {
        portItem = CreateMsgPort("ParentPort",0,0);
        if (portItem >= 0)
        {
            threadItem = CreateThread(ThreadFunction, "Child", 0, 2048, NULL);
            if (threadItem >= 0)
            {
                /* wait for the child to be ready */
                WaitSignal(parentSig);

                /* confirm that the child initialized correctly */
                if (FindMsgPort("ChildPort") >= 0)
                {
                    printf("Parent waiting for message from child\n");

                    msgItem = WaitPort(portItem,0);
                    if (msgItem >= 0)
                    {
                        msg = MESSAGE(msgItem);
                        printf("Parent got child's message: ");
                        printf("msg_DataPtr %d, msg_DataSize %d\n",msg->msg_DataPtr, msg->msg_DataSize);
                        ReplySmallMsg(msgItem,56,78,90);
                    }
                    else
                    {
                        printf("WaitPort() failed: ");
                        PrintfSysErr(msgItem);
                    }
                }

                /* wait for the thread to tell us it's done before we zap it */
                WaitSignal(parentSig);

                DeleteThread(threadItem);
            }
            else
            {
                printf("CreateThread() failed: ");
                PrintfSysErr(threadItem);
            }
            DeleteMsgPort(portItem);
        }
        else
        {
            printf("CreateMsgPort() failed: ");
            PrintfSysErr(portItem);
        }
        FreeSignal(parentSig);
    }
    else
    {
        printf("AllocSignal() failed\n");
    }

    return 0;
}
