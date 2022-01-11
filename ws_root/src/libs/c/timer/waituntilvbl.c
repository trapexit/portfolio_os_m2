/* @(#) waituntilvbl.c 95/08/14 1.2 */

#include <kernel/types.h>
#include <kernel/io.h>
#include <kernel/time.h>
#include <string.h>


/*****************************************************************************/


/**
|||	AUTODOC -public -class kernel -group Timer -name WaitUntilVBL
|||	Waits for a given vblank count to be reached.
|||
|||	  Synopsis
|||
|||	    Err WaitUntilVBL(Item ioreq, uint32 fields);
|||
|||	  Description
|||
|||	    Puts the current context to sleep until the system clock reaches a
|||	    given time, specified in VBLs.
|||
|||	  Arguments
|||
|||	    ioreq
|||	        An active timer device I/O request, as obtained from
|||	        CreateTimerIOReq().
|||
|||	    fields
|||	        The vblank count value that the timer must reach.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success, or a negative error code for failure.
|||
|||	  Warning
|||
|||	    The VBlank timer runs at either 50Hz or 60Hz depending on whether
|||	    the system is displaying PAL or NTSC.
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
|||	    CreateTimerIOReq(), DeleteTimerIOReq(), WaitTimeVBL()
|||
**/

Err WaitUntilVBL(Item ioreq, uint32 fields)
{
IOInfo ioInfo;

    memset(&ioInfo,0,sizeof(IOInfo));

    ioInfo.ioi_Command = TIMERCMD_DELAYUNTIL_VBL;
    ioInfo.ioi_Offset  = fields;

    return DoIO(ioreq, &ioInfo);
}
