/* @(#) mp.c 96/11/13 1.12 */

#include <kernel/io.h>
#include <kernel/driver.h>
#include <kernel/device.h>
#include <kernel/task.h>
#include <kernel/mem.h>
#include <kernel/debug.h>
#include <kernel/interrupts.h>
#include <kernel/operror.h>
#include <kernel/super.h>
#include <kernel/cache.h>
#include <kernel/kernel.h>
#include <kernel/sysinfo.h>
#include <kernel/monitor.h>
#include <hardware/PPCasm.h>
#include <hardware/bda.h>
#include <device/mp.h>
#include <loader/loader3do.h>
#include <string.h>
#include <stdio.h>


/*****************************************************************************/


extern void Dbgr_MPIOReqCreated(IOReq *ior);
extern void Dbgr_MPIOReqDeleted(IOReq *ior);


/*****************************************************************************/


/* ioreqs waiting to be dispatched */
static List   ioreqs = PREPLIST(ioreqs);

static IOReq *activeIOR;


/*****************************************************************************/


static void AbortMPIO(IOReq *ior)
{
    if (ior == activeIOR)
    {
        SuperSlaveRequest(SLAVE_ABORT, 0, 0, 0);
    }
    else
    {
        RemNode((Node *)ior);
        ior->io_Error = ABORTED;
        SuperCompleteIO(ior);
    }
}


/*****************************************************************************/


static void ScheduleMPIO(IOReq *ior)
{
MPPacket *p;
PagePool *pp;
Task     *slaveTask;

    activeIOR = ior;

    slaveTask = TASK(KB_FIELD(kb_CurrentSlaveTask));
    if (!slaveTask || !IsSameTaskFamily(slaveTask, TASK(ior->io.n_Owner)))
    {
        KB_FIELD(kb_CurrentSlaveTask) = ior->io.n_Owner;
        pp = TASK(KB_FIELD(kb_CurrentSlaveTask))->t_PagePool;
        FlushDCache(0, pp->pp_WritablePages, pp->pp_MemRegion->mr_NumPages / 8);
        SuperSlaveRequest(SLAVE_UPDATEMMU, (uint32)pp->pp_WritablePages, 0, 0);
    }
    KB_FIELD(kb_CurrentSlaveIO) = ior;

    p = (MPPacket *)ior->io_Info.ioi_Send.iob_Buffer;
    SuperSlaveRequest(SLAVE_DISPATCH, (uint32)p->mp_Code,
                                      (uint32)p->mp_Data,
                                      (uint32)p->mp_Stack);
}


/*****************************************************************************/


static void SlaveCompleteHandler(void *unused, int32 status)
{
IOReq *ior;
Err   *result;

    TOUCH(unused);

    if (activeIOR)
    {
        result = (Err *)activeIOR->io_Info.ioi_Recv.iob_Buffer;
        if (result)
            *result = status;

        SuperCompleteIO(activeIOR);

        /* see if there's anything else to do */
        ior = (IOReq *)RemHead(&ioreqs);
        if (ior)
        {
            ScheduleMPIO(ior);
        }
        else
        {
            activeIOR = NULL;
            KB_FIELD(kb_CurrentSlaveIO) = NULL;
        }
    }
}


/*****************************************************************************/


static int32 MPCmdDispatch(IOReq *ior)
{
uint32 oldints;

    if ((ior->io_Info.ioi_Send.iob_Len != sizeof(MPPacket))
     || (ior->io_Info.ioi_Recv.iob_Len != sizeof(Err)))
    {
        ior->io_Error = BADIOARG;
        return 1;
    }

    ior->io_Flags &= (~IO_QUICK);

    oldints = Disable();
    if (activeIOR)
    {
        InsertNodeFromTail(&ioreqs, (Node *)ior);
    }
    else
    {
        ScheduleMPIO(ior);
    }
    Enable(oldints);

    return 0;
}


/*****************************************************************************/


static Item CreateMPDriver(Driver *drv)
{
Err result;

    result = InstallMasterHandler(MASTER_SLAVECOMPLETE, SlaveCompleteHandler, NULL);
    if (result >= 0)
        return drv->drv.n_Item;

    return result;
}


/*****************************************************************************/


static Err DeleteMPDriver(Driver *drv)
{
    TOUCH(drv);
    KB_FIELD(kb_CurrentSlaveTask) = -1;
    InstallMasterHandler(MASTER_SLAVECOMPLETE, NULL, NULL);
    return 0;
}


/*****************************************************************************/


static Item CreateMPIO(IOReq *ior)
{
uint32 oldints;

    oldints = Disable();
    Dbgr_MPIOReqCreated(ior);
    Enable(oldints);

    return ior->io.n_Item;
}


/*****************************************************************************/


static Err DeleteMPIO(IOReq *ior)
{
uint32 oldints;

    oldints = Disable();
    Dbgr_MPIOReqDeleted(ior);
    Enable(oldints);

    return 0;
}


/*****************************************************************************/


static const DriverCmdTable cmdTable[] =
{
    {MP_CMD_DISPATCH, MPCmdDispatch}
};

int32 main(void)
{
    return CreateItemVA(MKNODEID (KERNELNODE, DRIVERNODE),
                        TAG_ITEM_NAME,              "mp",
                        TAG_ITEM_PRI,               1,
                        CREATEDRIVER_TAG_CMDTABLE,  cmdTable,
                        CREATEDRIVER_TAG_NUMCMDS,   sizeof(cmdTable) / sizeof(cmdTable[0]),
                        CREATEDRIVER_TAG_ABORTIO,   AbortMPIO,
                        CREATEDRIVER_TAG_CREATEDRV, CreateMPDriver,
                        CREATEDRIVER_TAG_DELETEDRV, DeleteMPDriver,
                        CREATEDRIVER_TAG_CRIO,      CreateMPIO,
                        CREATEDRIVER_TAG_DLIO,      DeleteMPIO,
                        CREATEDRIVER_TAG_MODULE,    FindCurrentModule(),
                        TAG_END);
}
