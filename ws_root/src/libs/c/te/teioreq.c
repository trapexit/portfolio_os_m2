/* @(#) teioreq.c 96/07/23 1.6 */

#include <kernel/types.h>
#include <kernel/io.h>
#include <device/te.h>


/**
|||	AUTODOC -class GraphicsFolio -group TE -name CreateTEIOReq
|||	Creates an I/O request item for use with the triangle engine device.
|||
|||	  Synopsis
|||
|||	    Item CreateTEIOReq(void);
|||
|||	  Description
|||
|||	    This routine opens the triangle engine device and creates a
|||	    kernel I/O request Item for it.  This IOReq Item may be used to
|||	    issue triangle engine command lists for rendering.
|||
|||	  Return Value
|||
|||	    If successful, the triangle engine device is opened, and the
|||	    Item number of the I/O request is returned.
|||
|||	    If anything goes wrong, a (negative) error code is returned and
|||	    nothing is created/opened.
|||
|||	  Associated Files
|||
|||	    <graphics/graphics.h>, <kernel/io.h>
|||
|||	  See Also
|||
|||	    DeleteTEIOReq()
|||
**/

static Item   dev = -1;
static uint32 count = 0;


Item CreateTEIOReq(void)
{
Item ioreq;

    if (count == 0)
    {
        dev = OpenTEDevice();
        if (dev < 0)
            return dev;

        count = 1;
    }
    else
    {
        count++;
    }

    ioreq = CreateIOReq(NULL, 0, dev, 0);
    if (ioreq < 0)
    {
        count--;
        if (count == 0)
            CloseTEDevice(dev);
    }

    return ioreq;
}


/*****************************************************************************/


/**
|||	AUTODOC -class GraphicsFolio -group TE -name DeleteTEIOReq
|||	Deletes an I/O request Item procured with CreateTEIOReq().
|||
|||	  Synopsis
|||
|||	    Err DeleteTEIOReq(Item ioreq);
|||
|||	  Description
|||
|||	    This function deletes the specified triangle engine device I/O
|||	    request (procured with CreateTEIOReq()), and closes the triangle
|||	    engine device.
|||
|||	  Arguments
|||
|||	    ioreq
|||	        Item to be deleted.
|||
|||	  Return Value
|||
|||	    Returns zero upon success, or a (negative) error code.
|||
|||	  Associated Files
|||
|||	    <graphics/graphics.h>, <kernel/io.h>
|||
|||	  See Also
|||
|||	    CreateTEIOReq()
|||
**/


Err DeleteTEIOReq(Item ioreq)
{
Err result;

    result = DeleteIOReq(ioreq);
    if (result >= 0)
    {
        count--;
        if (count == 0)
            result = CloseTEDevice(dev);
    }

    return result;
}
