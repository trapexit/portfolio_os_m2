/* @(#) startmetronomevbl.c 95/08/14 1.2 */

#include <kernel/types.h>
#include <kernel/io.h>
#include <kernel/time.h>
#include <string.h>


/*****************************************************************************/


/**
|||	AUTODOC -public -class kernel -group Timer -name StartMetronomeVBL
|||	Start a metronome counter.
|||
|||	  Synopsis
|||
|||	    Err StartMetronomeVBL(Item ioreq, uint32 fields, int32 signal);
|||
|||	  Description
|||
|||	    Starts a metronome timer. Once this call returns, a signal
|||	    will be sent to your task on a fixed interval until you call
|||	    StopMetronomeVBL(). The signalling interval is specified with the
|||	    fields argument. The signal to send is specified with the signal
|||	    argument.
|||
|||	  Arguments
|||
|||	    ioreq
|||	        An active timer device I/O request, as obtained from
|||	        CreateTimerIOReq().
|||
|||	    fields
|||	        The number of VBLs between signals of the metronome.
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
|||	    StopMetronomeVBL() function.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libc.a V27.
|||
|||	  Associated Files
|||
|||	    <kernel/time.h>, libc.a
|||
|||	  See Also
|||
|||	    CreateTimerIOReq(), DeleteTimerIOReq(), StopMetronomeVBL()
|||
**/

Err StartMetronomeVBL(Item ioreq, uint32 fields, int32 signal)
{
IOInfo ioInfo;

    memset(&ioInfo,0,sizeof(IOInfo));

    ioInfo.ioi_Command    = TIMERCMD_METRONOME_VBL;
    ioInfo.ioi_Offset     = fields;
    ioInfo.ioi_CmdOptions = signal;

    return SendIO(ioreq, &ioInfo);
}
