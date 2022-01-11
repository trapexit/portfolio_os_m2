/* @(#) timer.c 96/09/10 1.40 */

#include <kernel/types.h>
#include <kernel/io.h>
#include <kernel/driver.h>
#include <kernel/device.h>
#include <kernel/kernelnodes.h>
#include <kernel/debug.h>
#include <kernel/interrupts.h>
#include <kernel/kernel.h>
#include <kernel/timer.h>
#include <kernel/time.h>
#include <kernel/operror.h>
#include <kernel/super.h>
#include <kernel/listmacros.h>
#include <loader/loader3do.h>
#include <string.h>


#define DBUG(x)   /* printf x */
#define DBUGM(x)  /* printf x  */


/*****************************************************************************/


/* used in io_Flags of IO requests to mark them as metronome requests */
#define TIMERIOFLAGS_METRONOME 0x80000000

typedef struct TimerIOReq
{
    IOReq      timer_IOReq;

    /* amount of time between ticks of a metronome */
    TimerTicks timer_Interval;
} TimerIOReq;


/*****************************************************************************/


static List   vblWaiters = PREPLIST(vblWaiters);
static List   usecWaiters = PREPLIST(usecWaiters);
static Timer *timer;
static Item vblFirq = -1;
static Item timerItem = -1;


/*****************************************************************************/


static bool CmpTicks(Node *mm, Node *nm)
{
IOReq *m,*n;

    /* mm is already in the list, nm is the node we want to insert */

    DBUG(("mm=%lx nm=%lx\n",mm,nm));
    m = (IOReq *)mm;
    n = (IOReq *)nm;

    if (CompareTimerTicks((TimerTicks *)m->io_Extension, (TimerTicks *)n->io_Extension) <= 0)
        return TRUE;

    return FALSE;
}


/*****************************************************************************/


static void ProcessUSecWaiters(void)
{
IOReq      *ior;
TimerTicks  tt;
TimerTicks *target;
TimerTicks  delta;
Err         err;

    while (!IsEmptyList(&usecWaiters))
    {
        ior    = (IOReq *)FirstNode(&usecWaiters);
        target = (TimerTicks *)ior->io_Extension;

        /* sample the current time */
        SampleSystemTimeTT(&tt);

        /* If our target time comes before or is equal to the current time */
        if (CompareTimerTicks(target,&tt) <= 0)
        {
            REMOVENODE((Node *)ior);

            if (ior->io_Flags & TIMERIOFLAGS_METRONOME)
            {
                AddTimerTicks(target, &((TimerIOReq *)ior)->timer_Interval, target);

                /* ping our client */
                err = SuperInternalSignal((Task *)LookupItem(ior->io.n_Owner),
                                          ior->io_Info.ioi_CmdOptions);
                if (err >= 0)
                {
                    UniversalInsertNode(&usecWaiters,(Node *)ior,CmpTicks);
                    continue;
                }

                /* couldn't ping the client, we're done for... */
                ior->io_Error = err;
            }

            SuperCompleteIO(ior);
            continue;
        }
        else
        {
            /* Time not expired  -  reload the timer */
            SubTimerTicks(&tt,target,&delta);
            (*timer->tm_Load)(timer,&delta);
            break;
        }
    }
}


/*****************************************************************************/


static int32 VBLHandler(void)
{
IOReq *ior;
IOReq *nior;
Err    ret;

    for (ior = (IOReq *)FirstNode(&vblWaiters); ISNODE(&vblWaiters,ior); ior = nior)
    {
    int32 cnt;

        /* io_Offset indicates how many vblanks to wait for, while
         * io_Actual indicates how many have gone by so far
         */
        cnt = ior->io_Actual;
        cnt++;
        ior->io_Actual = cnt;

        nior = (IOReq *)NextNode(ior);
        if ((uint32)cnt >= (uint32)ior->io_Info.ioi_Offset)
        {
            if (ior->io_Flags & TIMERIOFLAGS_METRONOME)
            {
                /* resume the countdown... */
                ior->io_Actual = 0;

                /* ping our client */
                ret = SuperInternalSignal((Task *)LookupItem(ior->io.n_Owner),
                                          ior->io_Info.ioi_CmdOptions);
                if (ret < 0)
                {
                    /* couldn't ping the client, we're done for... */
                    ior->io_Error = ret;
                    REMOVENODE((Node *)ior);
                    SuperCompleteIO(ior);
                }
            }
            else
            {
                REMOVENODE((Node *)ior);
                SuperCompleteIO(ior);
            }
        }
    }

    return 0;
}


/*****************************************************************************/


static int32 USecHandler(void)
{
    ProcessUSecWaiters();	/* ignore returns, this should always ret 0 */
    return 0;
}


/*****************************************************************************/


/* Abort a command
 *
 * Remove the I/O request from the queue and set the error to ABORTED.
 * Call CompleteIO to finish the request.
 */

static void AbortTimerIO(IOReq *ior)
{
    /* If we get to here, the ior is in progress and interrupts are disabled.
     * All we have to do is take it off the list and complete it.
     */

    REMOVENODE((Node *)ior);
    ior->io_Error  = ABORTED;
    ior->io_Flags &= (~TIMERIOFLAGS_METRONOME);
    SuperCompleteIO(ior);

    /* We leave the timer alone since all it does is wake us up and cause us to
     * recheck the queue of pending requests.
     */
}


/*****************************************************************************/


static int32 CmdGetTimeVBL(IOReq *ior)
{
TimeValVBL *tvvbl;

    tvvbl = (TimeValVBL *)ior->io_Info.ioi_Recv.iob_Buffer;

    if (ior->io_Info.ioi_Recv.iob_Len >= sizeof(TimeValVBL))
    {
        SampleSystemTimeVBL(tvvbl);
        ior->io_Actual = sizeof(TimeValVBL);

        return 1;
    }

    ior->io_Error = BADIOARG;
    return 1;
}


/*****************************************************************************/


static int32 CmdGetTimeUSec(IOReq *ior)
{
TimeVal *tv;

    tv = (TimeVal *)ior->io_Info.ioi_Recv.iob_Buffer;

    if (ior->io_Info.ioi_Recv.iob_Len >= sizeof(TimeVal))
    {
        SampleSystemTimeTV(tv);
        DBUGM(("sec=%ld usec=%ld\n",tv->tv_Seconds,tv->tv_Microseconds));

        ior->io_Actual = sizeof(TimeVal);

        return 1;
    }

    ior->io_Error = BADIOARG;
    return 1;
}


/*****************************************************************************/


static int32 CmdSetTimeVBL(IOReq *ior)
{
    /* not yet, when we get the clock chip HW */
    ior->io_Error = NOSUPPORT;
    return 1;
}


/*****************************************************************************/


static int32 CmdSetTimeUSec(IOReq *ior)
{
TimeVal   *tv;
TimerTicks tt;

    if (ior->io_Info.ioi_Send.iob_Len == sizeof(TimeVal))
    {
        tv = (TimeVal *)ior->io_Info.ioi_Send.iob_Buffer;

        if (tv->tv_Microseconds <= 1000000)
        {
            ConvertTimeValToTimerTicks(tv, &tt);
            SuperSetSystemTimeTT(&tt);
            return 1;
        }
    }

    ior->io_Error = BADIOARG;
    return 1;
}


/*****************************************************************************/


static int32 CmdDelayVBL(IOReq *ior)
{
uint32 oldints;

    DBUG((" CmdDelay for %ld\n", ior->io_Info.ioi_Offset));
    oldints = Disable();
    ADDTAIL(&vblWaiters,(Node *)ior);
    ior->io_Flags &= ~IO_QUICK;
    Enable(oldints);

    return 0;
}


/*****************************************************************************/


static int32 CmdDelayUSec(IOReq *ior)
{
TimeVal *tv;
uint32   oldints;

    if (ior->io_Info.ioi_Send.iob_Len == sizeof(TimeVal))
    {
        tv = (TimeVal *)ior->io_Info.ioi_Send.iob_Buffer;

        DBUGM((" CmdDelay:%lx.%lx\n",tv->tv_Seconds,tv->tv_Microseconds));

        if (tv->tv_Microseconds <= 1000000)
        {
            /* Compute target time */
            ConvertTimeValToTimerTicks(tv,&((TimerIOReq *)ior)->timer_Interval);
            SampleSystemTimeTT((TimerTicks *)ior->io_Extension);
            AddTimerTicks((TimerTicks *)ior->io_Extension,&((TimerIOReq *)ior)->timer_Interval,(TimerTicks *)ior->io_Extension);

            DBUGM(("CmdMDelay target:ex=%lx.%lx\n", ior->io_Extension[0], ior->io_Extension[1]));

            /* Now have target, schedule it */
            oldints = Disable();
            UniversalInsertNode(&usecWaiters,(Node *)ior,CmpTicks);
            ior->io_Flags &= ~IO_QUICK;
            ProcessUSecWaiters();
            Enable(oldints);

            return 0;
        }
    }

    ior->io_Error = BADIOARG;
    return 1;
}


/*****************************************************************************/


static int32 CmdDelayUntilVBL(IOReq *ior)
{
TimeValVBL tvvbl;
uint32     oldints;

    oldints = Disable();
    SampleSystemTimeVBL(&tvvbl);
    ior->io_Info.ioi_Offset -= tvvbl & 0xffffffff;
    ADDTAIL(&vblWaiters,(Node *)ior);
    ior->io_Flags &= ~IO_QUICK;
    Enable(oldints);

    return 0;
}


/*****************************************************************************/


static int32 CmdDelayUntilUSec(IOReq *ior)
{
TimeVal *tv;
uint32   oldints;

    if (ior->io_Info.ioi_Send.iob_Len == sizeof(TimeVal))
    {
        tv = (TimeVal *)ior->io_Info.ioi_Send.iob_Buffer;

        if (tv->tv_Microseconds <= 1000000)
        {
            /* Compute target time */
            ConvertTimeValToTimerTicks(tv,(TimerTicks *)ior->io_Extension);

            /* Now have target, schedule it */
            oldints = Disable();
            UniversalInsertNode(&usecWaiters,(Node *)ior,CmpTicks);
            ior->io_Flags &= ~IO_QUICK;
            ProcessUSecWaiters();
            Enable(oldints);

            return 0;
        }
    }

    ior->io_Error = BADIOARG;
    return 1;
}


/*****************************************************************************/


static int32 CmdMetronomeVBL(IOReq *ior)
{
    /* do we have a valid signal mask? */
    if (ior->io_Info.ioi_CmdOptions & 0x800000ff)
    {
        /* only allowed bits 8-30 */
        ior->io_Error = BADIOARG;
        return 1;
    }

    /* mark this puppy as a repeating metronome request */
    ior->io_Flags |= TIMERIOFLAGS_METRONOME;

    return CmdDelayVBL(ior);
}


/*****************************************************************************/


static int32 CmdMetronomeUSec(IOReq *ior)
{
    /* do we have a valid signal mask? */
    if (ior->io_Info.ioi_CmdOptions & 0x800000ff)
    {
        /* only allowed bits 8-30 */
        ior->io_Error = BADIOARG;
        return 1;
    }

    /* mark this puppy as a repeating metronome request */
    ior->io_Flags |= TIMERIOFLAGS_METRONOME;

    return CmdDelayUSec(ior);
}


/*****************************************************************************/


static const TagArg timerTags[] =
{
    TAG_ITEM_NAME,         "utimer",
    CREATETIMER_TAG_HNDLR, (void *)USecHandler,
    TAG_END,               0
};


static Item CreateTimerDriver(Driver *drv)
{

    DBUG(("vblWaiters=%lx usecWaiters=%lx\n",&vblWaiters,&usecWaiters));

    timerItem = CreateItem(MKNODEID(KERNELNODE,TIMERNODE), timerTags);
    if (timerItem < 0)
        return timerItem;

    timer = (Timer *)LookupItem(timerItem);

    vblFirq = CreateFIRQ("vblTimer", 200, VBLHandler, INT_V1);
    if (vblFirq < 0)
    {
        DeleteItem(timerItem);
	timerItem = -1;
        return vblFirq;
    }

    EnableInterrupt(INT_V1);

    return drv->drv.n_Item;
}


/*****************************************************************************/


static Err ChangeTimerDriverOwner(Driver *drv, Item newOwner)
{
    TOUCH(drv);
    SetItemOwner(timerItem, newOwner);
    SetItemOwner(vblFirq, newOwner);
    return 0;
}


/*****************************************************************************/


static const DriverCmdTable CmdTable[] =
{
    TIMERCMD_DELAY_VBL,	      CmdDelayVBL,
    TIMERCMD_DELAY_USEC,      CmdDelayUSec,
    TIMERCMD_DELAYUNTIL_VBL,  CmdDelayUntilVBL,
    TIMERCMD_DELAYUNTIL_USEC, CmdDelayUntilUSec,
    TIMERCMD_METRONOME_VBL,   CmdMetronomeVBL,
    TIMERCMD_METRONOME_USEC,  CmdMetronomeUSec,
    TIMERCMD_GETTIME_VBL,     CmdGetTimeVBL,
    TIMERCMD_GETTIME_USEC,    CmdGetTimeUSec,
    TIMERCMD_SETTIME_VBL,     CmdSetTimeVBL,
    TIMERCMD_SETTIME_USEC,    CmdSetTimeUSec
};

Item TimerDriverMain(void)
{
Item driver;

    driver = CreateItemVA(MKNODEID(KERNELNODE,DRIVERNODE),
	    TAG_ITEM_NAME,              "timer",
	    TAG_ITEM_PRI,               1,
	    CREATEDRIVER_TAG_CMDTABLE,  CmdTable,
	    CREATEDRIVER_TAG_NUMCMDS,   sizeof(CmdTable)/sizeof(CmdTable[0]),
	    CREATEDRIVER_TAG_CREATEDRV, CreateTimerDriver,
	    CREATEDRIVER_TAG_ABORTIO,   AbortTimerIO,
	    CREATEDRIVER_TAG_IOREQSIZE, sizeof(TimerIOReq),
	    CREATEDRIVER_TAG_CHOWN_DRV, ChangeTimerDriverOwner,
	    CREATEDRIVER_TAG_MODULE,    FindCurrentModule(),
	    TAG_END);

    if (driver < 0)
	return driver;

    return OpenItem(driver, 0);
}
