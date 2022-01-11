/* @(#) stopmetronome.c 95/08/14 1.4 */

#include <kernel/types.h>
#include <kernel/io.h>
#include <kernel/time.h>
#include <string.h>


/*****************************************************************************/


/**
|||	AUTODOC -public -class kernel -group Timer -name StopMetronome
|||	Stop a metronome counter.
|||
|||	  Synopsis
|||
|||	    Err StopMetronome(Item ioreq);
|||
|||	  Description
|||
|||	    Stops a metronome counter that was previously started with
|||	    StartMetronome().
|||
|||	  Arguments
|||
|||	    ioreq
|||	        An active timer device I/O request, as obtained from
|||	        CreateTimerIOReq().
|||
|||	  Return Value
|||
|||	    >= 0 for success, or a negative error code for failure.
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
|||	    CreateTimerIOReq(), DeleteTimerIOReq(), StartMetronome()
|||
**/

Err StopMetronome(Item ioreq)
{
    return AbortIO(ioreq);
}
