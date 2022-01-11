/* @(#) dispatchmpfunc.c 96/09/11 1.3 */

#include <kernel/types.h>
#include <kernel/io.h>
#include <device/mp.h>
#include <string.h>


/*****************************************************************************/


/**
|||	AUTODOC -class Kernel -group MP -name DispatchMPFunc
|||	Start code running on the slave processor.
|||
|||	  Synopsis
|||
|||	    Err DispatchMPFunc(Item mpioreq, MPFunc *func, void *data,
|||	                       uint32 *stack, uint32 stackSize, Err *result);
|||
|||	  Description
|||
|||	    Starts a slave CPU executing code. If the slave is already busy,
|||	    the request will be queued and will be serviced as soon as the
|||	    slave completes its current work.
|||
|||	  Arguments
|||
|||	    mpioreq
|||	        An active MP device I/O request, as obtained from
|||	        CreateMPIOReq().
|||
|||	    func
|||	        The function to dispatch on the slave processor.
|||
|||	    data
|||	        A 32-bit pointer which is given as parameter to the function
|||	        being started. This lets you pass context information to the
|||	        slave.
|||
|||	    stack
|||	        A pointer to the stack for the function to use. This is a
|||	        pointer to the top of the stack + 1.
|||
|||	    stackSize
|||	        The number of bytes allocated for the stack.
|||
|||	    result
|||	        A pointer to an Err value where the return value of "func"
|||	        will be stored. When the slave completes the IO operation,
|||	        you can look at this Err value to determine what the slave
|||	        returned.
|||
|||	  Return Value
|||
|||	    >= 0 for success, or a negative error code for failure.
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
|||	    CreateMPIOReq(), DeleteMPIOReq()
|||
**/

Err DispatchMPFunc(Item ioreq, MPFunc func, void *data,
                   uint32 *stack, uint32 stackSize, Err *result)
{
IOInfo   ioInfo;
MPPacket packet;

    memset(&ioInfo, 0, sizeof(IOInfo));
    memset(&packet, 0, sizeof(MPPacket));

    packet.mp_Code      = func;
    packet.mp_Data      = data;
    packet.mp_Stack     = stack;
    packet.mp_StackSize = stackSize;

    ioInfo.ioi_Command         = MP_CMD_DISPATCH;
    ioInfo.ioi_Send.iob_Buffer = &packet;
    ioInfo.ioi_Send.iob_Len    = sizeof(MPPacket);
    ioInfo.ioi_Recv.iob_Buffer = result;
    ioInfo.ioi_Recv.iob_Len    = sizeof(Err);

    return SendIO(ioreq, &ioInfo);
}
