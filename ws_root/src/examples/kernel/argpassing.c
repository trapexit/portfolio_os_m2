
/******************************************************************************
**
**  @(#) argpassing.c 96/06/05 1.1
**
******************************************************************************/

/**
|||	AUTODOC -class Examples -name argpassing
|||	Demonstrates how to pass startup parameters to a thread.
|||
|||	  Synopsis
|||
|||	    argpassing
|||
|||	  Description
|||
|||	    Demonstrates how to pass startup parameters to a thread.
|||	    A thread is spawned and passed two parameters (123 and 456).
|||	    The thread simply prints out the value of these parameters and
|||	    exits.
|||
|||	  Associated Files
|||
|||	    argpassing.c
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


static void ThreadFunction(uint32 arg1, uint32 arg2)
{
    printf("Child thread got arg1=%u and arg2=%u\n", arg1, arg2);
}


/*****************************************************************************/


int main(void)
{
Item threadItem;

    threadItem = CreateThreadVA(ThreadFunction, "Child", 0, 2048,
	                        CREATETASK_TAG_ARGC, 123,
	                        CREATETASK_TAG_ARGP, 456,
	                        TAG_END);
    if (threadItem >= 0)
    {
        WaitSignal(SIGF_DEADTASK);
        DeleteThread(threadItem);
    }
    else
    {
        printf("CreateThread() failed: ");
        PrintfSysErr(threadItem);
    }

    return 0;
}
