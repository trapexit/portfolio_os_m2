
/******************************************************************************
**
**  @(#) timersleep.c 95/12/05 1.8
**
******************************************************************************/

/**
|||	AUTODOC -public -class examples -name timersleep
|||	Demonstrates how to use the timer device to wait for an
|||	    amount of time specified on the command-line.
|||
|||	  Synopsis
|||
|||	    timersleep <num seconds> <num microseconds>
|||
|||	  Description
|||
|||	    This program demonstrates how to use the timer device to wait for a
|||	    certain amount of time.
|||
|||	    The program does the following:
|||
|||	    * Parses the command-line arguments
|||
|||	    * Opens the timer device
|||
|||	    * Creates an IOReq
|||
|||	    * Initializes an IOInfo structure
|||
|||	    * Calls DoIO() to perform the wait operation
|||
|||	    * Cleans up
|||
|||	    Note that Portfolio provides convenience routines to make using the timer
|||	    device easier. For example, CreateTimerIOReq(), DeleteTimerIOReq(), and
|||	    WaitTime(). This example intends to show how in general one can
|||	    communicate with devices in the Portfolio environment. For more
|||	    information on the timer convenience routines, see the timer device
|||	    documentation.
|||
|||	  Arguments
|||
|||	    <num seconds>
|||	        Number of seconds to wait.
|||
|||	    <num microseconds>
|||	        Number of microseconds to wait.
|||
|||	  Associated Files
|||
|||	    timersleep.c
|||
|||	  Location
|||
|||	    examples/Kernel
|||
**/

#include <kernel/types.h>
#include <kernel/io.h>
#include <kernel/device.h>
#include <kernel/item.h>
#include <kernel/time.h>
#include <kernel/operror.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int32 argc, char **argv)
{
Item    ioreqItem;
IOInfo  ioInfo;
TimeVal tv;
Err     err;

    if (argc == 3)
    {
        tv.tv_Seconds      = strtoul(argv[1],0,0);
        tv.tv_Microseconds = strtoul(argv[2],0,0);

            ioreqItem = CreateTimerIOReq();
            if (ioreqItem >= 0)
            {
                memset(&ioInfo,0,sizeof(ioInfo));
                ioInfo.ioi_Command         = TIMERCMD_DELAY_USEC;
                ioInfo.ioi_Send.iob_Buffer = &tv;
                ioInfo.ioi_Send.iob_Len    = sizeof(tv);

                err = DoIO(ioreqItem,&ioInfo);
                if (err >= 0)
                {
                    printf("slept\n");
                }
                else
                {
                    printf("DoIO() failed: ");
                    PrintfSysErr(err);
                }
                DeleteIOReq(ioreqItem);
            }
            else
            {
                printf("CreateIOReq() failed: ");
                PrintfSysErr(ioreqItem);
            }
    }
    else
    {
        printf("Usage: timersleep <num seconds> <num microseconds>\n");
    }

    return 0;
}
