/* @(#) mp_slave.c 96/11/07 1.21 */

#include <kernel/types.h>
#include <kernel/kernel.h>
#include <kernel/super.h>
#include <kernel/cache.h>
#include <kernel/debug.h>
#include <kernel/internalf.h>
#include <device/mp.h>
#include <hardware/PPCasm.h>
#include <hardware/bda.h>
#include <stdio.h>
#include <string.h>


/*****************************************************************************/


/* #define DEBUG */


/*****************************************************************************/


extern void SlaveIdleLoop(void);
void SuperMasterRequest(MasterActions action, uint32 arg1, uint32 arg2, uint32 arg3);

#ifdef __DCC__
#pragma no_return SlaveIdleLoop
#endif


/*****************************************************************************/


#ifdef DEBUG
static void PutStr(const char *str)
{
CPUActionReq *mr;

    FlushDCache(0, str, strlen(str) + 1);

    mr = KB_FIELD(kb_MasterReq);

    _dcbi(mr);
    mr->car_Arg1   = (uint32)str;
    mr->car_Arg2   = 0;
    mr->car_Arg3   = 0;
    _dcbf(mr);      /* make sure the args hit memory before the action code */
    mr->car_Action = MASTER_PUTSTR;
    _dcbf(mr);

    /* wait until the master has picked up the packet */
    while (mr->car_Action != MASTER_NOP)
        _dcbi(mr);
}
#endif


/*****************************************************************************/


void internalSlaveExit(Err status)
{
    SuperMasterRequest(MASTER_SLAVECOMPLETE, status, 0, 0);
    SlaveIdleLoop();
}


/*****************************************************************************/


static void internalSlaveDebugPutStr(const char *str)
{
    FlushDCache(0, str, strlen(str) + 1);
    SuperMasterRequest(MASTER_PUTSTR, (uint32)str, 0, 0);
}


/*****************************************************************************/


void *SlaveGetSysCallFunction(uint32 callNum)
{
char buffer[80];

    switch (callNum)
    {
        case 0          : return (void *)internalSlaveExit;
        case 0x10000+36 : return (void *)internalSlaveDebugPutStr;

        default: /* unknown system call, kill the slave context */
                 sprintf(buffer, "Illegal system call 0x%x attempted on the slave\n", callNum);
                 internalDebugPutStr(buffer);
                 internalSlaveExit(-1);
    }
}


/*****************************************************************************/


static void SlaveDispatch(void *data, MPFunc func)
{
    SlaveExit((*func)(data));
}


/*****************************************************************************/


void ServiceSlaveExceptions(void)
{
CPUActionReq  req;
CPUActionReq *sr;
SlaveState   *ss;
uint32        hid0;
MemRegion    *reg;
bool          suspended;

    /* clear interrupt source */
    BDA_WRITE(BDAMCTL_MREF, BDA_READ(BDAMCTL_MREF) | BDAMREF_GPIO3_VALUE);

    suspended = FALSE;
    do
    {
        /* fetch the packet info */
        sr = KB_FIELD(kb_SlaveReq);
        _dcbi(sr);
        req = *sr;

        ss = KB_FIELD(kb_SlaveState);

#ifdef DEBUG
        {
        char buf[100];

            sprintf(buf, "Slave action %d\n", req.car_Action);
            PutStr(buf);
        }
#endif

        switch (req.car_Action)
        {
            case SLAVE_DISPATCH        : ss->ss_RegSave.rb_PC      = (uint32)SlaveDispatch;
                                         ss->ss_RegSave.rb_MSR     = MSR_EE | MSR_PR | MSR_FP | MSR_ME | MSR_IR | MSR_DR;
                                         ss->ss_RegSave.rb_GPRs[1] = req.car_Arg3 - 8;
                                         ss->ss_RegSave.rb_GPRs[3] = req.car_Arg2;
                                         ss->ss_RegSave.rb_GPRs[4] = req.car_Arg1;
                                         InvalidateICache();
                                         break;

            case SLAVE_ABORT           : sr->car_Action = SLAVE_NOP;
                                         _dcbf(sr);
                                         internalSlaveExit(ABORTED);
                                         break;

            case SLAVE_CONTROLICACHE   : hid0 = _mfhid0();
                                         if (req.car_Arg1)
                                         {
                                             /* Turn on the cache */
                                             if (!(hid0 & HID_ICE))
                                             {
                                                 /* Invalidate and turn on the cache */
                                                 _mthid0(hid0 | HID_ICFI);
                                                 _mthid0(hid0 & ~HID_ICFI);
                                                 _mthid0(hid0 | HID_ICE);
                                             }
                                         }
                                         else
                                         {
                                             /* Turn off the cache */
                                             _mthid0(hid0 & ~HID_ICE);
                                         }
                                         break;

            case SLAVE_INVALIDATEICACHE: InvalidateICache();
                                         break;

            case SLAVE_CONTROLDCACHE   : hid0 = _mfhid0();
                                         if (req.car_Arg1)
                                         {
                                             /* Turn on the cache */
                                             if (!(hid0 & HID_DCE))
                                             {
                                                 /* Invalidate and turn on the cache */
                                                 _mthid0(hid0 | HID_DCI);
                                                 _mthid0(hid0 & ~HID_DCI);
                                                 _mthid0((hid0 | HID_DCE) & ~HID_DCI);
                                             }
                                         }
                                         else
                                         {
                                             /* Turn off the cache */
                                             if (hid0 & HID_DCE)
                                             {
                                                 FlushDCacheAll(0);
                                                 _mthid0(hid0 & ~HID_DCE);
                                             }
                                         }
                                         break;

            case SLAVE_INITMMU         : SetupMMU();
                                         _mtsr11((uint32)KB_FIELD(kb_RAMBaseAddress));
                                         _mtsr12((uint32)KB_FIELD(kb_RAMEndAddress));
                                         _mtsr13((uint32)ss->ss_WritablePages);
                                         _mtser(ss->ss_SlaveSER);
                                         _mtsebr(ss->ss_SlaveSEBR);

                                         sr->car_Action = SLAVE_NOP;
                                         _dcbf(sr);
                                         SlaveIdleLoop();
                                         break;

            case SLAVE_INITVERSION     : ss->ss_SlaveVersion = _mfpvr();
                                         _dcbf(&ss->ss_SlaveVersion);
                                         break;

            case SLAVE_UPDATEMMU       : /* This makes sure the cache doesn't hold
                                          * dirty data for pages the slave is
                                          * about to lose write access to, and
                                          * it also makes sure we can read the
                                          * array pointed by ss_Arg1.
                                          */
                                         FlushDCacheAll(0);

                                         reg = KB_FIELD(kb_PagePool)->pp_MemRegion;
                                         memcpy(ss->ss_WritablePages, (void *)req.car_Arg1, ss->ss_NumPages / 8);
                                         LoadDTLBs(ss->ss_WritablePages, reg->mr_MemBase);
                                         break;

            case SLAVE_SRESETPREP      : if (req.car_Arg1)
                                         {
                                             _mtdec(0x7fffffff);
                                             break;
                                         }

                                         /* Save relevant slave CPU state across soft reset events */
                                         ss->ss_SlaveMSR = _mfmsr();
                                         ss->ss_SlaveHID0 = _mfhid0();

                                         /* Set IP bit so soft reset will work properly */
                                         _mtmsr(_mfmsr() | MSR_IP);

                                         /* Disable the caches so we won't hang executing out of the ROM */
                                         /* Also clean up HID0 so bootcode won't complain */
                                         hid0 = ss->ss_SlaveHID0;
                                         if (ss->ss_SlaveHID0 & HID_DCE)
                                         {
                                             FlushDCacheAll(0);
                                             hid0 &= ~HID_DCE;
                                         }

                                         if (ss->ss_SlaveHID0 & HID_ICE)
                                             hid0 &= ~HID_ICE;

                                         hid0 &= ~HID_WIMG;
                                         hid0 |= HID_WIMG_GUARD;
                                         _mthid0(hid0);

                                         sr->car_Action = SLAVE_NOP;
                                         _dcbf(sr);
                                         SlaveIdleLoop();
                                         break;

            case SLAVE_SRESETRECOVER   : /* Invalidate caches for good measure */
                                         hid0 = _mfhid0();
                                         _mthid0(hid0 | HID_ICFI);
                                         _mthid0(hid0 & ~HID_ICFI);
                                         _mthid0(hid0 | HID_DCI);
                                         _mthid0(hid0 & ~HID_DCI);

                                         /* Restore relevant slave CPU state */
                                         _mtmsr(ss->ss_SlaveMSR);
                                         _mthid0(ss->ss_SlaveHID0);
                                         break;

            case SLAVE_SUSPEND         : suspended = TRUE;
                                         break;

            case SLAVE_CONTINUE        : suspended = FALSE;
                                         break;

            case SLAVE_NOP             : if (suspended)
                                             continue;
                                         else
                                             return;

            default                    : break;
        }

        /* clear the way for more packets */
        sr->car_Action = SLAVE_NOP;
        _dcbf(sr);
    }
    while (suspended);
}


/*****************************************************************************/


/* This call is invoked when the slave wants the master to do something */
void SuperMasterRequest(MasterActions action, uint32 arg1, uint32 arg2, uint32 arg3)
{
CPUActionReq *mr;

    mr = KB_FIELD(kb_MasterReq);

    /* send the packet to the master */
    _dcbi(mr);
    mr->car_Arg1   = arg1;
    mr->car_Arg2   = arg2;
    mr->car_Arg3   = arg3;
    _dcbf(mr);      /* make sure the args hit memory before the action code */
    mr->car_Action = action;
    _dcbf(mr);

    /* interrupt the master to let it know there's something to do */
    BDA_WRITE(BDAPCTL_ERRSTAT, 0x01);
    BDA_WRITE(BDAPCTL_PBINTENSET, 0x01);

    /* wait until the master picks up the packet */
    while (mr->car_Action != MASTER_NOP)
    {
        ServiceSlaveExceptions();
        _dcbi(mr);
    }
}
