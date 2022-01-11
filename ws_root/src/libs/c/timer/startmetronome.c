/* @(#) startmetronome.c 95/08/14 1.4 */

#include <kernel/types.h>
#include <kernel/io.h>
#include <kernel/time.h>
#include <string.h>


/*****************************************************************************/


/**
|||	AUTODOC -public -class kernel -group Timer -name StartMetronome
|||	Start a metronome counter.
|||
|||	  Synopsis
|||
|||	    Err StartMetronome(Item ioreq, uint32 seconds, uint32 micros,
|||	                       int32 signal);
|||
|||	  Description
|||
|||	    Starts a metronome timer. Once this call returns, a signal will be
|||	    sent to your task on a fixed interval until you call
|||	    StopMetronome(). The signalling interval is specified with the
|||	    seconds and micros argument. The signal to send is specified with
|||	    the signal argument.
|||
|||	  Arguments
|||
|||	    ioreq
|||	        An active timer device I/O request, as obtained from
|||	        CreateTimerIOReq().
|||
|||	    seconds
|||	        The number of seconds between signals of the metronome.
|||
|||	    micros
|||	        The number of microseconds between signals of the metronome.
|||
|||	    signal
|||	        The signal mask to send whenever the requested interval of time
|||	        passes.
|||
|||	  Return Value
|||
|||	    >= 0 for success, or a negative error code for failure. Once the
|||	    metronome is started, you will receive signals on the requested
|||	    interval. To stop the metronome, you should call the
|||	    StopMetronome() function.
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
|||	    CreateTimerIOReq(), DeleteTimerIOReq(), StopMetronome()
|||
**/

Err StartMetronome(Item ioreq, uint32 seconds, uint32 micros, int32 signal)
{
IOInfo  ioInfo;
TimeVal tv;

    memset(&ioInfo,0,sizeof(IOInfo));

    tv.tv_Seconds              = seconds;
    tv.tv_Microseconds         = micros;
    ioInfo.ioi_Command         = TIMERCMD_METRONOME_USEC;
    ioInfo.ioi_CmdOptions      = signal;
    ioInfo.ioi_Send.iob_Buffer = &tv;
    ioInfo.ioi_Send.iob_Len    = sizeof(TimeVal);

    return SendIO(ioreq, &ioInfo);
}
