
/******************************************************************************
**
**  @(#) timerread.c 95/12/05 1.7
**
******************************************************************************/

/**
|||	AUTODOC -public -class examples -name timerread
|||	Demonstrates how to use the timer device to read the
|||	            current system time.
|||
|||	  Synopsis
|||
|||	    timerread
|||
|||	  Description
|||
|||	    This program demonstrates how to use the timer device to read the
|||	    current system time.
|||
|||	    The program does the following:
|||
|||	    * Opens the timer device
|||
|||	    * Creates an IOReq
|||
|||	    * Initializes an IOInfo structure
|||
|||	    * Calls DoIO() to perform the read operation
|||
|||	    * Prints out the current time
|||
|||	    * Cleans up
|||
|||	    Note that Portfolio provides convenience routines to make using the
|||	    timer device easier. For example, CreateTimerIOReq() and
|||	    DeleteTimerIOReq(). This example intends to show how in general one
|||	    can communicate with devices in the Portfolio environment. For more
|||	    information on the timer convenience routines, see the timer device
|||	    documentation.
|||
|||	    Also note that Portfolio provides an alternate higher performance
|||	    means of sample the current system time. See the documentation
|||	    on SampleSystemTimeTV() and SampleSystemTimeTT() for more
|||	    information.
|||
|||	  Associated Files
|||
|||	    timerread.c
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
#include <string.h>


int main(void)
{
Item    ioreqItem;
IOInfo  ioInfo;
TimeVal tv;
Err     err;

        ioreqItem = CreateTimerIOReq();
        if (ioreqItem >= 0)
        {
            memset(&ioInfo,0,sizeof(ioInfo));
            ioInfo.ioi_Command         = TIMERCMD_GETTIME_USEC;
            ioInfo.ioi_Recv.iob_Buffer = &tv;
            ioInfo.ioi_Recv.iob_Len    = sizeof(tv);

            err = DoIO(ioreqItem,&ioInfo);
            if (err >= 0)
            {
                printf("Seconds %u, microseconds %u\n",tv.tv_Seconds,tv.tv_Microseconds);
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

    return 0;
}
