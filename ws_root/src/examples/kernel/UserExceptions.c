
/******************************************************************************
**
**  @(#) UserExceptions.c 96/04/05 1.1
**
******************************************************************************/

/**
|||	AUTODOC -class Examples -name UserExceptions
|||	Demonstrates how to trap program exceptions.
|||
|||	  Synopsis
|||
|||	    UserExceptions
|||
|||	  Description
|||
|||	    This program spawns a thread that triggers exceptions by doing
|||	    various dubious operations. The main task installs itself as
|||	    the exception handler so that it receives notification when
|||	    its thread triggers an exception.
|||
|||	  Associated Files
|||
|||	    UserExceptions.c
|||
|||	  Location
|||
|||	    Examples/Kernel
|||
**/

#include <kernel/types.h>
#include <kernel/nodes.h>
#include <kernel/kernelnodes.h>
#include <kernel/task.h>
#include <stdlib.h>
#include <stdio.h>


/*****************************************************************************/


float32 zero = 0.0;


/*****************************************************************************/


static void ThreadFunction(void)
{
Err err;

    printf("Thread running\n");

    err = ControlUserExceptions(USEREXC_FP_ZERODIVIDE, TRUE);
    if (err >= 0)
    {
    }
    else
    {
        printf("ControlUserExceptions() failed: ");
        PrintfSysErr(err);
    }

    printf("Doing divide by zero\n");
    printf("25.0/0.0 = %f\n", 25.0 / zero);
}


/*****************************************************************************/


int main(void)
{
Item                  port;
Item                  thread;
Item                  msg;
Message              *msgPtr;
uint32                sigs;
UserExceptionContext *uec;

    port = CreateMsgPort("ParentPort",0,0);
    if (port >= 0)
    {
        thread = CreateThreadVA(ThreadFunction, "Child", 0, 2048,
                                CREATETASK_TAG_USEREXCHANDLER, port,
                                TAG_END);
        if (thread >= 0)
        {
            printf("Entering wait loop\n");
            while (TRUE)
            {
                sigs = WaitSignal(SIGF_DEADTASK | MSGPORT(port)->mp_Signal);
                if (sigs & SIGF_DEADTASK)
                {
                    /* the thread is dead, so exit */
                    break;
                }

                while (TRUE)
                {
                    msg = GetMsg(port);
                    if (msg <= 0)
                        break;

                    msgPtr = MESSAGE(msg);
                    uec    = msgPtr->msg_DataPtr;

                    printf("Task    %s\n", TASK(uec->uec_Task)->t.n_Name);
                    printf("Trigger 0x%x\n", uec->uec_Trigger);

                    CompleteUserException(thread, NULL, NULL);
                }
            }
            DeleteThread(thread);
        }
        else
        {
            printf("CreateThreadVA() failed: ");
            PrintfSysErr(thread);
        }
        DeleteMsgPort(port);
    }
    else
    {
        printf("CreateMsgPort() failed: ");
        PrintfSysErr(port);
    }

    return 0;
}
