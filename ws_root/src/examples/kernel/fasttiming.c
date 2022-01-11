
/******************************************************************************
**
**  @(#) fasttiming.c 95/06/08 1.3
**
******************************************************************************/

/**
|||	AUTODOC -public -class examples -name fasttiming
|||	Demonstrates how to use the high accuracy kernel timing
|||	             services.
|||
|||	  Synopsis
|||
|||	    fasttiming
|||
|||	  Description
|||
|||	    This program demonstrates how to use the high accuracy low overhead
|||	    timing services provided by the kernel.
|||
|||	    Note that this particular example will only give a relatively
|||	    accurate reading of how much time it takes to call Yield() if
|||	    the shell is running in foreground mode. Otherwise, the shell
|||	    task will interfere with the execution of this program, and will
|||	    therefore skew the results.
|||
|||	  Associated Files
|||
|||	    fasttiming.c
|||
|||	  Location
|||
|||	    examples/Kernel
|||
**/

#include <kernel/types.h>
#include <kernel/time.h>
#include <kernel/operror.h>
#include <kernel/task.h>
#include <stdio.h>


int main(void)
{
TimerTicks startTT;
TimerTicks endTT;
TimerTicks resultTT;
uint32     i;
TimeVal    tv;

    /* sample the start time */
    SampleSystemTimeTT(&startTT);

    for (i = 0; i < 100; i++)
        Yield();

    /* sample the end time */
    SampleSystemTimeTT(&endTT);

    /* calculate the difference between the start and end times */
    SubTimerTicks(&startTT,&endTT,&resultTT);

    /* convert the difference to TimeVal format */
    ConvertTimerTicksToTimeVal(&resultTT,&tv);

    /* print the result */
    printf("Calling Yield 100 times took %d.%06d seconds\n",tv.tv_Seconds,tv.tv_Microseconds);

    return 0;
}
