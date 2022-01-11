/* @(#) mpsim.c 96/09/11 1.3 */

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
#include <hardware/PPCasm.h>
#include <hardware/bda.h>
#include <device/mp.h>
#include <loader/loader3do.h>
#include <string.h>
#include <stdio.h>


/*****************************************************************************/


#define TRACE(x) printf x


static Item controlThread;
static Item controlLock;
static List ioreqs = PREPLIST(ioreqs);


/*****************************************************************************/


static void AbortMPIO(IOReq *ior)
{
    SuperInternalDeleteItem(ior->io_Extension[0]);
}


/*****************************************************************************/


static void ControlThread(void)
{
int32    sigs;
Item     slave;
Item     slaveMsg;
Item     port;
IOReq   *ior;
Message *msg;
Err      result;
uint32   oldints;

    TRACE(("ControlThread: entering\n"));

    port = CURRENTTASK->t_DefaultMsgPort;
    while (TRUE)
    {
        /* wait for something to happen */
        sigs = WaitSignal(MSGPORT(port)->mp_Signal | SIGF_ABORT);

        /* are we being killed? */
        if (sigs & SIGF_ABORT)
            return;

        while (TRUE)
        {
            /* see if any termination messages came in */
            slaveMsg = GetMsg(port);
            if (slaveMsg <= 0)
                break;

            /* extract info */
            msg    = MESSAGE(slaveMsg);
            slave  = (Item)msg->msg_DataPtr;
            result = (Err)msg->msg_Result;

            oldints = Disable();
            ScanList(&ioreqs, ior, IOReq)
            {
                if (ior->io_Extension[0] == (uint32)slave)
                {
                    RemNode((Node *)ior);
                    *(Err *)ior->io_Info.ioi_Recv.iob_Buffer = result;
                    SuperCompleteIO(ior);
                    break;
                }
            }
            Enable(oldints);
            SuperInternalDeleteItem(slaveMsg);
        }
    }
}


/*****************************************************************************/


static Err SlaveThread(MPPacket *p)
{
Err result;

    TRACE(("SlaveThread: slave thread entering with p 0x%x\n", p));

    LockSemaphore(controlLock, SEM_WAIT);
    result = (* p->mp_Code)(p->mp_Data);
    UnlockSemaphore(controlLock);

    TRACE(("SlaveThread: slave thread exiting with %d\n", result));

    return result;
}


/*****************************************************************************/


static int32 MPCmdDispatch(IOReq *ior)
{
Item      thread;
MPPacket *p;
uint32    oldints;

    TRACE(("MPCmdDispatch: entering with ior 0x%x\n", ior));

    if ((ior->io_Info.ioi_Send.iob_Len != sizeof(MPPacket))
     || (ior->io_Info.ioi_Recv.iob_Len != sizeof(Err)))
    {
        ior->io_Error = BADIOARG;
        return 1;
    }

    p = ior->io_Info.ioi_Send.iob_Buffer;
    ior->io_Flags &= (~IO_QUICK);

    /* make sure thread doesn't complete before we're done using the ior */
    LockSemaphore(controlLock, SEM_WAIT);

    thread = CreateItemVA(MKNODEID(KERNELNODE,TASKNODE),
                          TAG_ITEM_NAME,                  "MP Simulation",
                          CREATETASK_TAG_PC,              SlaveThread,
                          CREATETASK_TAG_STACKSIZE,       p->mp_StackSize,
                          CREATETASK_TAG_SP,              p->mp_Stack,
                          CREATETASK_TAG_ARGC,            p,
                          CREATETASK_TAG_THREAD,          0,
                          CREATETASK_TAG_MSGFROMCHILD,    THREAD(controlThread)->t_DefaultMsgPort,
                          TAG_END);

    if (thread >= 0)
    {
        ior->io_Extension[0] = thread;

        oldints = Disable();
        AddTail(&ioreqs, (Node *)ior);
        Enable(oldints);
    }

    UnlockSemaphore(controlLock);

    if (thread < 0)
    {
        ior->io_Error = thread;
        return 1;
    }

    return 0;
}


/*****************************************************************************/


static const DriverCmdTable cmdTable[] =
{
    {MP_CMD_DISPATCH, MPCmdDispatch}
};

int32 main(int32 argc)
{
Err result;

    TRACE(("main: entering with argc %d\n", argc));

    result = 0;
    if (argc == DEMANDLOAD_MAIN_CREATE)
    {
        controlThread = result = CreateThreadVA(ControlThread, "MP Control", 0, 2048,
                              CREATETASK_TAG_PRIVILEGED,      TRUE,
                              CREATETASK_TAG_SINGLE_STACK,    TRUE,
                              CREATETASK_TAG_SUPERVISOR_MODE, TRUE,
                              CREATETASK_TAG_DEFAULTMSGPORT,  TRUE,
                              TAG_END);

        if (controlThread >= 0)
        {
            controlLock = result = CreateSemaphore("MP Control", 0);
            if (controlLock >= 0)
                return 0;

            DeleteThread(controlThread);
        }
    }
    else if (argc == DEMANDLOAD_MAIN_DELETE)
    {
        DeleteSemaphore(controlLock);
        DeleteThread(controlThread);
    }
    else if (argc >= 0)
    {
        result =  CreateItemVA(MKNODEID(KERNELNODE, DRIVERNODE),
                            TAG_ITEM_NAME,              "mpsim",
                            CREATEDRIVER_TAG_CMDTABLE,  cmdTable,
                            CREATEDRIVER_TAG_NUMCMDS,   sizeof(cmdTable) / sizeof(cmdTable[0]),
                            CREATEDRIVER_TAG_ABORTIO,   AbortMPIO,
                            CREATEDRIVER_TAG_MODULE,    FindCurrentModule(),
                            TAG_END);
    }

    return result;
}
