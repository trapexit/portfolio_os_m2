/* @(#) deletempio.c 96/06/30 1.2 */

#include <kernel/types.h>
#include <kernel/io.h>
#include <kernel/device.h>
#include <kernel/operror.h>
#include <device/mp.h>


/*****************************************************************************/


/**
|||	AUTODOC -class kernel -group MP -name DeleteMPIOReq
|||	Deletes an MP device I/O request.
|||
|||	  Synopsis
|||
|||	    Err DeleteMPIOReq(Item ioreq);
|||
|||	  Description
|||
|||	    Frees any resources used by a previous call to CreateMPIOReq().
|||
|||	  Arguments
|||
|||	    ioreq
|||	        The I/O request item, as returned by a previous call to
|||	        CreateMPIOReq().
|||
|||	  Return Value
|||
|||	    Return >= 0 if successful, or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libc.a V31.
|||
|||	  Associated Files
|||
|||	    <device/mp.h>
|||
|||	  See Also
|||
|||	    CreateMPIOReq(), DispatchMPFunc()
|||
**/

Err DeleteMPIOReq(Item ioreq)
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
