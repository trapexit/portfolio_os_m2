
/******************************************************************************
**
**  @(#) defaultport.c 95/10/15 1.5
**
******************************************************************************/

/**
|||	AUTODOC -public -class examples -name defaultport
|||	Demonstrates using a task's default message port to communicate
|||	with a child thread.
|||
|||	  Synopsis
|||
|||	    defaultport
|||
|||	  Description
|||
|||	    Demonstrates how to start a thread having a default message port,
|||	    and use the port for communication.
|||
|||	  Associated Files
|||
|||	    defaultport.c
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
#include <kernel/mem.h>
#include <stdio.h>


/*****************************************************************************/


static void ThreadFunction(void)
{
Item     msgItem;
Message *msg;

    printf("Child thread is running\n");

    msgItem = WaitPort(CURRENTTASK->t_DefaultMsgPort,0);
    if (msgItem >= 0)
    {
        msg = MESSAGE(msgItem);
        printf("Child received message from parent: ");
        printf("msg_Result %d, msg_DataPtr %d, msg_DataSize %d\n",
               msg->msg_Result, msg->msg_DataPtr, msg->msg_DataSize);

        ReplySmallMsg(msgItem,0,56,78);
    }
    else
    {
        printf("WaitPort() failed: ");
        PrintfSysErr(msgItem);
    }
}


/*****************************************************************************/


int main(void)
{
Item     portItem;
Item     threadItem;
Item     msgItem;
Message *msg;
Err      err;

    portItem = CreateMsgPort("ParentPort",0,0);
    if (portItem >= 0)
    {
        threadItem = CreateThreadVA(ThreadFunction, "Child", 0, 2048,
	                            CREATETASK_TAG_DEFAULTMSGPORT, 0,
	                            TAG_END);
        if (threadItem >= 0)
        {
            msgItem = CreateSmallMsg(NULL,0,portItem);
            if (msgItem >= 0)
            {
                err = SendSmallMsg(THREAD(threadItem)->t_DefaultMsgPort,msgItem,12,34);
                if (err >= 0)
                {
                    /* wait for the thread to reply */
                    printf("Parent waiting for reply from child\n");

                    err = WaitPort(portItem,msgItem);
                    if (err >= 0)
                    {
                        msg = MESSAGE(msgItem);
                        printf("Parent got child's reply: ");
                        printf("msg_DataPtr %d, msg_DataSize %d\n",msg->msg_DataPtr, msg->msg_DataSize);
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
                DeleteMsg(msgItem);
            }
            else
            {
                printf("CreateSmallMsg failed: ");
                PrintfSysErr(msgItem);
            }
            DeleteThread(threadItem);
        }
        else
        {
            printf("CreateThreadVA() failed: ");
            PrintfSysErr(threadItem);
        }
        DeleteMsgPort(portItem);
    }
    else
    {
        printf("CreateMsgPort() failed: ");
        PrintfSysErr(portItem);
    }

    return 0;
}
