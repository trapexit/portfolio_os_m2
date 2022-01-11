/* @(#) mp_master.c 96/11/13 1.11 */

#include <kernel/types.h>
#include <kernel/kernel.h>
#include <kernel/super.h>
#include <kernel/cache.h>
#include <loader/loader3do.h>
#include <kernel/monitor.h>
#include <kernel/internalf.h>
#include <device/mp.h>
#include <hardware/PPCasm.h>
#include <hardware/bda.h>
#include <stdio.h>


/*****************************************************************************/


#define TRACE(x) /*printf x*/


/*****************************************************************************/


typedef void (* MasterHandlerFunc)(void *userData, uint32 arg);


/*****************************************************************************/


static MasterHandlerFunc mpHandler;
static void             *mpHandlerData;
static bool              inMonitor;


/*****************************************************************************/


Err InstallMasterHandler(MasterActions action, void *handler, void *handlerData)
{
    TRACE(("InstallMasterHandler: entering with action %d, handler %x\n", action, handler));

    if (action != MASTER_SLAVECOMPLETE)
    {
        /* can currently only listen to these events */
        return PARAMERROR;
    }

    if (handler && mpHandler)
    {
        /* already a handler */
        return -1;
    }

    if (KB_FIELD(kb_NumCPUs) < 2)
    {
        /* only 1 CPU */
        return NOHARDWARE;
    }

    mpHandler     = handler;
    mpHandlerData = handlerData;

    return 0;
}


/*****************************************************************************/


uint32 ServiceMasterExceptions(void)
{
CPUActionReq *mr;
CPUActionReq  req;

    TRACE(("ServiceMasterExceptions: entering\n"));

    /* clear interrupt source */
    BDA_CLR(BDAPCTL_ERRSTAT, 0x1);

    /* fetch the packet info */
    mr = KB_FIELD(kb_MasterReq);
    _dcbi(mr);
    req = *mr;

    switch (req.car_Action)
    {
        case MASTER_SLAVECOMPLETE: if (mpHandler)
                                   {
                                       /* clear the way for more packets */
                                       mr->car_Action = MASTER_NOP;
                                       _dcbf(mr);

                                       (* mpHandler)(mpHandlerData, req.car_Arg1);
                                       return 0;
                                   }
                                   break;

        case MASTER_PUTSTR       : FlushDCacheAll(0);
                                   internalDebugPutStr((char *)req.car_Arg1);
                                   break;

        case MASTER_NOP          : return 0;

        case MASTER_MONITOR      : if (!inMonitor)
                                   {
                                       inMonitor = TRUE;
                                       FlushDCacheAll(0);
                                       Dbgr_MPIOReqCrashed(KB_FIELD(kb_CurrentSlaveIO), req.car_Arg1, req.car_Arg2);
                                       FlushDCacheAll(0);
                                       inMonitor = FALSE;
                                   }
                                   break;

        default                  : break;
    }

    /* clear the way for more packets */
    mr->car_Action = MASTER_NOP;
    _dcbf(mr);

    return 0;
}


/*****************************************************************************/


/* This call is invoked when the master wants the slave to do something */
void SuperSlaveRequest(SlaveActions action, uint32 arg1, uint32 arg2, uint32 arg3)
{
CPUActionReq *mr;
CPUActionReq *sr;
uint32        oldints;

    TRACE(("SuperSlaveRequest: entering with action %d, arg1 0x%x, arg2 0x%x, arg3 0x%x\n", action, arg1, arg2, arg3));

    if ((KB_FIELD(kb_Flags) & KB_MPACTIVE) == 0)
        return;

    mr = KB_FIELD(kb_MasterReq);
    sr = KB_FIELD(kb_SlaveReq);

    oldints = Disable();

    /* send the packet to the slave */
    _dcbi(sr);
    sr->car_Arg1   = arg1;
    sr->car_Arg2   = arg2;
    sr->car_Arg3   = arg3;
    _dcbf(sr);      /* make sure the args hit memory before the action code */
    sr->car_Action = action;
    _dcbf(sr);

    /* interrupt the slave to let it know there's something to do */
    BDA_WRITE(BDAMCTL_MREF, (BDA_READ(BDAMCTL_MREF) | BDAMREF_GPIO3_GP | BDAMREF_GPIO3_OUT) & ~BDAMREF_GPIO3_VALUE);

    /* wait until the slave picks up the packet */
    while (TRUE)
    {
        _dcbi(sr);
        if (sr->car_Action == SLAVE_NOP)
            break;

        _dcbi(mr);
        if (mr->car_Action != MASTER_NOP)
            ServiceMasterExceptions();
    }

    Enable(oldints);
}
