/* @(#) deletetimerio.c 95/12/06 1.7 */

#include <kernel/types.h>
#include <kernel/io.h>
#include <kernel/device.h>
#include <kernel/operror.h>
#include <kernel/time.h>


/*****************************************************************************/


/**
|||	AUTODOC -public -class kernel -group Timer -name DeleteTimerIOReq
|||	Delete a timer device I/O request.
|||
|||	  Synopsis
|||
|||	    Err DeleteTimerIOReq(Item ioreq);
|||
|||	  Description
|||
|||	    Frees any resources used by a previous call to CreateTimerIOReq().
|||
|||	  Arguments
|||
|||	    ioreq
|||	        The I/O request item, as returned by a previous call to
|||	        CreateTimerIOReq().
|||
|||	  Return Value
|||
|||	    Return >=0 if successful, or a negative error code for failure.
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
|||	    CreateTimerIOReq(), WaitTime(), WaitUntil()
|||
**/

Err DeleteTimerIOReq(Item ioreq)
{
IOReq  *io;
Device *dev;
Err     result;

    io = (IOReq *)CheckItem(ioreq,KERNELNODE,IOREQNODE);
    if (!io)
        return BADITEM;

    dev = io->io_Dev;
    result = DeleteIOReq(ioreq);
    CloseDeviceStack(dev->dev.n_Item);

    return result;
}
