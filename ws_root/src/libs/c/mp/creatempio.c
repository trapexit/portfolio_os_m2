/* @(#) creatempio.c 96/07/18 1.2 */

#include <kernel/types.h>
#include <kernel/io.h>
#include <kernel/device.h>
#include <kernel/operror.h>
#include <device/mp.h>


/*****************************************************************************/


/**
|||	AUTODOC -class Kernel -group MP -name CreateMPIOReq
|||	Creates a multiprocessor device I/O request.
|||
|||	  Synopsis
|||
|||	    Item CreateMPIOReq(bool simulation);
|||
|||	  Description
|||
|||	    Creates an I/O request for communication with slave processors.
|||
|||	  Arguments
|||
|||	    simulation
|||	        Set this to TRUE if you wish to simulate the use of a second
|||	        processor by using a thread to run the code instead of by
|||	        really dispatching to the slave processor. This is very handy
|||	        when trying to debug code that runs on the slave.
|||
|||	  Return Value
|||
|||	    Returns an MP I/O request item, or a negative error code for
|||	    failure.
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
|||	    DeleteMPIOReq(), DispatchMPFunc()
|||
**/

Item CreateMPIOReq(bool simulation)
{
Item  device;
Item  ioreq;
List *list;
Err   err;

    if (simulation)
    {
        err = CreateDeviceStackListVA(&list, "cmds", DDF_EQ, DDF_INT, 1,
                                                     MP_CMD_DISPATCH,
                                              "simulation", DDF_EQ, DDF_STRING, 1, "TRUE",
                                              NULL);
    }
    else
    {
        err = CreateDeviceStackListVA(&list, "cmds", DDF_EQ, DDF_INT, 1,
                                                     MP_CMD_DISPATCH,
                                              NULL);
    }

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
