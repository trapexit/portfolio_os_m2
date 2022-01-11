/* @(#) waittime.c 95/08/14 1.7 */

#include <kernel/types.h>
#include <kernel/io.h>
#include <kernel/time.h>
#include <string.h>


/*****************************************************************************/


/**
|||	AUTODOC -public -class kernel -group Timer -name WaitTime
|||	Waits for a given amount of time to pass.
|||
|||	  Synopsis
|||
|||	    Err WaitTime(Item ioreq, uint32 seconds, uint32 micros);
|||
|||	  Description
|||
|||	    Puts the current context to sleep for a specific amount of time.
|||
|||	  Arguments
|||
|||	    ioreq
|||	        An active timer device I/O request, as obtained from
|||	        CreateTimerIOReq().
|||
|||	    seconds
|||	        The number of seconds to wait for.
|||
|||	    micros
|||	        The number of microseconds to wait for.
|||
|||	  Return Value
|||
|||	    Returns >= 0 if successful, or a negative error code if it fails.
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
|||	    CreateTimerIOReq(), DeleteTimerIOReq(), WaitUntil()
|||
**/

Err WaitTime(Item ioreq, uint32 seconds, uint32 micros)
{
IOInfo  ioInfo;
TimeVal tv;

    memset(&ioInfo,0,sizeof(IOInfo));

    tv.tv_Seconds              = seconds;
    tv.tv_Microseconds         = micros;
    ioInfo.ioi_Command         = TIMERCMD_DELAY_USEC;
    ioInfo.ioi_Send.iob_Buffer = &tv;
    ioInfo.ioi_Send.iob_Len    = sizeof(TimeVal);

    return DoIO(ioreq, &ioInfo);
}
