/* @(#) taskexit.c 96/09/06 1.20 */

#include <kernel/types.h>
#include <kernel/kernel.h>
#include <kernel/task.h>
#include <kernel/internalf.h>
#include <device/mp.h>


/**
|||	AUTODOC -class Kernel -group Tasks -name exit
|||	Exits from a task or thread.
|||
|||	  Synopsis
|||
|||	    void exit( int status )
|||
|||	  Description
|||
|||	    This function serves two purposes:
|||
|||	    When executed on the master CPU, this function deletes the calling
|||	    task or thread. If the CREATETASK_TAG_MSGFROMCHILD tag was set when
|||	    creating the calling task, then the status is sent to the parent
|||	    through a message.
|||
|||	    When executed on the slave CPU, this function terminates execution
|||	    of the current job on the slave and completes the IOReq back to the
|||	    master. The status value is available as result code in the
|||	    ioi_Recv buffer of the IOReq.
|||
|||	  Arguments
|||
|||	    status
|||	        The status to be returned to the parent of
|||	        the calling task or thread. Negative status
|||	        is reserved for system use.
|||
|||	  Return Value
|||
|||	    This call never returns.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <stdlib.h>
|||
|||	  See Also
|||
|||	    CreateThread(), DeleteThread(), CreateTask(), DeleteTask()
|||
**/


void exit(int32 status)
{
    if (IsSlaveCPU())
        SlaveExit(status);

    SetExitStatus(status);
    KillSelf();
}
