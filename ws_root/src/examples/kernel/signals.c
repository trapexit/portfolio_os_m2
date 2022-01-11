
/******************************************************************************
**
**  @(#) signals.c 95/10/15 1.5
**
******************************************************************************/

/**
|||	AUTODOC -public -class examples -name signals
|||	Demonstrates how to use signals.
|||
|||	  Synopsis
|||
|||	    signals
|||
|||	  Description
|||
|||	    Demonstrates the use of threads and signals.
|||
|||	    The main() routine launches two threads. These threads sit in a loop and
|||	    count. After a given number of iterations through their loop, they send a
|||	    signal to the parent task. When the parent task gets a signal, it wakes up
|||	    and prints the current counters of the threads to show how much they were
|||	    able to count.
|||
|||	  Associated Files
|||
|||	    signals.c
|||
|||	  Location
|||
|||	    examples/Kernel
|||
**/

#include <kernel/types.h>
#include <kernel/task.h>
#include <kernel/operror.h>
#include <stdio.h>


/*****************************************************************************/


/* Global variables shared by all threads. */
static int32  thread1Sig;
static int32  thread2Sig;
static Item   parentItem;
static uint32 thread1Cnt;
static uint32 thread2Cnt;


/*****************************************************************************/


/* This routine shared by both threads */
static void DoThread(int32 signal, uint32 amount, uint32 *counter)
{
uint32 i;

    while (TRUE)
    {
        for (i = 0; i < amount; i++)
        {
            (*counter)++;
            SendSignal(parentItem,signal);
        }
    }
}


/*****************************************************************************/


static void Thread1Func(void)
{
    DoThread(thread1Sig, 100000, &thread1Cnt);
}


/*****************************************************************************/


static void Thread2Func(void)
{
    DoThread(thread2Sig, 200000,&thread2Cnt);
}


/*****************************************************************************/


int main(void)
{
Item   thread1Item;
Item   thread2Item;
uint32 count;
int32  sigs;

    /* get the item number of the parent task */
    parentItem = CURRENTTASKITEM;

    /* allocate one signal bits for each thread */
    thread1Sig = AllocSignal(0);
    thread2Sig = AllocSignal(0);

    /* spawn two threads that will run in parallel */
    thread1Item = CreateThread(Thread1Func, "Thread1", 0, 2048, NULL);
    thread2Item = CreateThread(Thread2Func, "Thread2", 0, 2048, NULL);

    /* enter a loop until we receive 10 signals */
    count = 0;
    while (count < 10)
    {
        sigs = WaitSignal(thread1Sig | thread2Sig);

        printf("Thread 1 at %d, thread 2 at %d\n",thread1Cnt,thread2Cnt);

        if (sigs & thread1Sig)
            printf("Signal from thread 1\n");

        if (sigs & thread2Sig)
            printf("Signal from thread 2\n");

        count++;
    }

    /* nuke both threads */
    DeleteThread(thread1Item);
    DeleteThread(thread2Item);

    return 0;
}
