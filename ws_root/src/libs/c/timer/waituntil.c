/* @(#) waituntil.c 95/08/14 1.7 */

#include <kernel/types.h>
#include <kernel/io.h>
#include <kernel/time.h>
#include <string.h>


/*****************************************************************************/


/**
|||	AUTODOC -public -class kernel -group Timer -name WaitUntil
|||	Waits for a given amount of time to arrive.
|||
|||	  Synopsis
|||
|||	    Err WaitUntil(Item ioreq, uint32 seconds, uint32 micros);
|||
|||	  Description
|||
|||	    Puts the current context to sleep until the system clock reaches a
|||	    given time.
|||
|||	  Arguments
|||
|||	    ioreq
|||	        An active timer device I/O request, as obtained from
|||	        CreateTimerIOReq().
|||
|||	    seconds
|||	        The seconds value that the timer must reach.
|||
|||	    micros
|||	        The microseconds value that the timer must reach.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libc.a V24.
|||
|||	  Associated Files
|||
|||	    <kernel/time.h>, libc.a
|||
|||	  See Also
|||
|||	    CreateTimerIOReq(), DeleteTimerIOReq(), WaitTime()
|||
**/

Err WaitUntil(Item ioreq, uint32 seconds, uint32 micros)
{
IOInfo  ioInfo;
TimeVal tv;

    memset(&ioInfo,0,sizeof(IOInfo));

    tv.tv_Seconds              = seconds;
    tv.tv_Microseconds         = micros;
    ioInfo.ioi_Command         = TIMERCMD_DELAYUNTIL_USEC;
    ioInfo.ioi_Send.iob_Buffer = &tv;
    ioInfo.ioi_Send.iob_Len    = sizeof(TimeVal);

    return DoIO(ioreq, &ioInfo);
}
