
/******************************************************************************
**
**  @(#) metronome.c 95/06/08 1.2
**
******************************************************************************/

/**
|||	AUTODOC -public -class examples -name metronome
|||	Demonstrates how to use the metronome timer feature.
|||
|||	  Synopsis
|||
|||	    metronome
|||
|||	  Description
|||
|||	    Demonstrates the use of the metronome timer. The metronome is
|||	    a mechanism that sends you a signal at a regular interval until
|||	    you tell it to stop.
|||
|||	    The code creates a metronome timer and waits to receive 10 signals
|||	    from it. The signals are paced at one per second. The pacing is
|||	    specified when StartMetronome() is called.
|||
|||	  Associated Files
|||
|||	    metronome.c
|||
|||	  Location
|||
|||	    examples/Kernel
|||
**/


#include <kernel/types.h>
#include <kernel/time.h>
#include <kernel/item.h>
#include <kernel/task.h>
#include <kernel/operror.h>
#include <stdio.h>


int main(void)
{
int32  sigs;
Item   timerIO;
uint32 i;
Err    err;

    sigs = AllocSignal(0);
    if (sigs > 0)
    {
        timerIO = CreateTimerIOReq();
        if (timerIO >= 0)
        {
            err = StartMetronome(timerIO,1,0,sigs);
            if (err >= 0)
            {
                for (i = 0; i < 10; i++)
                {
                    WaitSignal(sigs);
                    printf("Got a signal, i = %d\n",i);
                }

                StopMetronome(timerIO);
            }
            else
            {
                printf("StartMetronome() failed: ");
                PrintfSysErr(err);
            }
            DeleteTimerIOReq(timerIO);
        }
        else
        {
            printf("CreateTimerIOReq() failed: ");
            PrintfSysErr(timerIO);
        }
        FreeSignal(sigs);
    }
    else
    {
        printf("AllocSignal() failed");
    }

    return 0;
}
