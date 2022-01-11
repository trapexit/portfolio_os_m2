/* @(#) waittimevbl.c 95/08/14 1.2 */

#include <kernel/types.h>
#include <kernel/io.h>
#include <kernel/time.h>
#include <string.h>


/*****************************************************************************/


/**
|||	AUTODOC -public -class kernel -group Timer -name WaitTimeVBL
|||	Waits for a given number of video fields to pass.
|||
|||	  Synopsis
|||
|||	    Err WaitTimeVBL(Item ioreq, uint32 fields);
|||
|||	  Description
|||
|||	    Puts the current context to sleep until a specified number of
|||	    VBLs occur.
|||
|||	  Arguments
|||
|||	    ioreq
|||	        An active timer device I/O request, as obtained from
|||	        CreateTimerIOReq().
|||
|||	    fields
|||	        The number of fields to wait for.
|||
|||	  Return Value
|||
|||	    Returns >= 0 if successful, or a negative error code if it fails.
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
|||	    CreateTimerIOReq(), DeleteTimerIOReq(), WaitUntilVBL()
|||
**/

Err WaitTimeVBL(Item ioreq, uint32 fields)
{
IOInfo ioInfo;

    memset(&ioInfo,0,sizeof(IOInfo));

    ioInfo.ioi_Command = TIMERCMD_DELAY_VBL;
    ioInfo.ioi_Offset  = fields;

    return DoIO(ioreq, &ioInfo);
}
