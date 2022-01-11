/* @(#) createtimerio.c 95/12/07 1.8 */

#include <kernel/types.h>
#include <kernel/io.h>
#include <kernel/device.h>
#include <kernel/operror.h>
#include <kernel/time.h>


/*****************************************************************************/


/**
|||	AUTODOC -public -class kernel -group Timer -name CreateTimerIOReq
|||	Creates a timer device I/O request.
|||
|||	  Synopsis
|||
|||	    Item CreateTimerIOReq(void);
|||
|||	  Description
|||
|||	    Creates an I/O request for communication with the timer device.
|||
|||	  Return Value
|||
|||	    Returns a timer I/O request item, or a negative error code for
|||	    failure.
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
|||	    DeleteTimerIOReq(), WaitTime(), WaitUntil()
|||
**/

Item CreateTimerIOReq(void)
{
Item device;
Item ioreq;
List *list;
Err err;

    /*
     * Since the caller doesn't tell us which timer commands he wants,
     * we need a device that supports them all.
     */
    err = CreateDeviceStackListVA(&list,
		"cmds", DDF_EQ, DDF_INT, 10,
			TIMERCMD_GETTIME_VBL, 
			TIMERCMD_SETTIME_VBL,
			TIMERCMD_DELAY_VBL, 
			TIMERCMD_DELAYUNTIL_VBL,
			TIMERCMD_METRONOME_VBL,
			TIMERCMD_GETTIME_USEC, 
			TIMERCMD_SETTIME_USEC,
			TIMERCMD_DELAY_USEC, 
			TIMERCMD_DELAYUNTIL_USEC,
			TIMERCMD_METRONOME_USEC,
		NULL);
    if (err < 0)
	return err;
    if (IsEmptyList(list))
    {
	DeleteDeviceStackList(list);
	return MAKEKERR(ER_SEVER,ER_C_STND,ER_NoHardware);
    }
    /* Take the first device. */
    device = OpenDeviceStack((DeviceStack *) FirstNode(list));
    DeleteDeviceStackList(list);
    if (device < 0)
        return device;

    ioreq = CreateIOReq(NULL,0,device,0);
    if (ioreq < 0)
        CloseDeviceStack(device);

    return ioreq;
}
