/* @(#) host.c 96/07/18 1.36 */

#ifdef BUILD_MACDEBUGGER

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
#include <kernel/timer.h>
#include <hardware/bridgit.h>
#include <hardware/bda.h>
#include <hardware/debugger.h>
#include <hardware/PPCasm.h>
#include <loader/loader3do.h>
#include <string.h>
#include <stdio.h>


/*****************************************************************************/


#define DBUG(x)     /* printf x */


/*****************************************************************************/


/* used to indicate the IOReq can currently be aborted */
#define IO_ABORTABLE 0x80000000


/*****************************************************************************/


typedef struct Buffer
{
    MinNode b_Link;
    uint8   b_Data[DATASIZE];
} Buffer;

#define NUM_BUFFERS 10


/*****************************************************************************/


/* ioreqs waiting to be dispatched */
static List iorqs = PREPLIST(iorqs);

/* ioreqs waiting for unsolicitated data from the host */
static List recviorqs = PREPLIST(recviorqs);

/* buffers for unsolicitated data from the host */
static List   usedBuffers = PREPLIST(usedBuffers);
static List   freeBuffers = PREPLIST(freeBuffers);
static Buffer bufs[NUM_BUFFERS];

static volatile HostPacket * send;
static volatile HostPacket * receive;

static Item    firqItem;
#ifndef USE_BRIDGIT_INTR
static Item    timerItem;
static Timer     *timer;
static TimerTicks oneMilli;
static uint32     pending;
#endif


/*****************************************************************************/


static void AbortHostIO(IOReq *ior)
{
    /* our ioreqs can only be aborted if they are presently queued up in one
     * of our lists.
     */
    if (ior->io_Flags & IO_ABORTABLE)
    {
        RemNode((Node *)ior);
        ior->io_Flags &= (~IO_ABORTABLE);
        ior->io_Error  = ABORTED;
        SuperCompleteIO(ior);
    }
}


/*****************************************************************************/


/* Setup the output channel to send a packet to the host */
static void PostRequest(IOReq *ior)
{
    DBUG(("PostRequest: ior $%x, command $%x\n",ior, ior->io_Info.ioi_Command));

    /* this one's commited, no turning back */
    ior->io_Flags &= (~IO_ABORTABLE);

    send->hp_Busy     = FALSE;
    send->hp_UserData = ior;
    send->hp_Unit     = ior->io_Info.ioi_CmdOptions;
    memcpy(send->hp_Data, ior->io_Info.ioi_Send.iob_Buffer, ior->io_Info.ioi_Send.iob_Len);

    /* We first flush the buffer to memory with the busy flag cleared.
     * This makes sure that everything hits memory before the Mac tries to
     * read any of it. Once we know the data has hit memory, we set the
     * busy flag and flush again.
     */

    WriteBackDCache(0, send, sizeof(*send));
    send->hp_Busy = TRUE;
    FlushDCache(0, send, sizeof(*send));

    /* Now that everything's in memory, wake up that lazy Mac */
    DEBUGGER_CHANNEL_FULL();

#ifndef USE_BRIDGIT_INTR
    if (ior->io_Info.ioi_CmdOptions != HOST_CONSOLE_UNIT)
        pending++;
#endif
}


/*****************************************************************************/


/* Extract packets sent to us through the in channel */
static bool ProcessIncoming(void)
{
IOReq  *ior;
IOReq  *scanior;
bool    result;
Buffer *buf;

    result = FALSE;

    while (TRUE)
    {
        /* make sure we fetch this from memory and not the cache */
        SuperInvalidateDCache(receive, sizeof(*receive));

        /* see if the host is telling us there's a packet waiting for us */
        if (!receive->hp_Busy)
            break;

        ior = (IOReq *)receive->hp_UserData;
        if (!ior)
        {
            /* got an unsolicitated packet from the host, see if we have
             * any IOReq pending for that unit.
             */
            ScanList(&recviorqs,scanior,IOReq)
            {
                if (scanior->io_Info.ioi_CmdOptions == receive->hp_Unit)
                {
                    RemNode((Node *)scanior);
                    ior = scanior;

                    /* can't abort it anymore, it's all done */
                    ior->io_Flags &= (~IO_ABORTABLE);
                    break;
                }
            }
        }

        if (ior)
        {
            memcpy(ior->io_Info.ioi_Recv.iob_Buffer, receive->hp_Data, ior->io_Info.ioi_Recv.iob_Len);
        }
        else
        {
            /* no ioreq is waiting for this data, buffer it */
            buf = (Buffer *)RemHead(&freeBuffers);
            if (!buf)
                buf = (Buffer *)RemHead(&usedBuffers);

            memcpy(buf->b_Data, receive->hp_Data, DATASIZE);
            AddTail(&usedBuffers, (Node *)buf);
        }

        /* channel is no longer busy */
        receive->hp_Busy = FALSE;

        _dcbst(receive);
        _sync();

        if (DEBUGGER_ACK())
        {
            /* If the debugger's not doing any work right now, tell it
             * we've cleared our channel. If the debugger is doing work,
             * it'll notice the cleared channel all by itself.
             */
            DEBUGGER_CHANNEL_EMPTY();
        }

        DBUG(("ProcessIncoming: ior $%x, command $%x\n",ior, ior->io_Info.ioi_Command));

#ifndef USE_BRIDGIT_INTR
        if (ior->io_Info.ioi_CmdOptions != HOST_CONSOLE_UNIT)
            pending--;
#endif

        SuperCompleteIO(ior);

        result = TRUE;
    }

    return result;
}


/*****************************************************************************/


/* either a new IO needs to be scheduled, or the int handler just woke up and
 * we need to see if any previous request has returned from the host.
 */
static void ScheduleIO(IOReq *ior)
{
bool didwork;

    do
    {
        didwork = FALSE;

        if (DEBUGGER_ACK())
        {
            /* The host can accept a new request */

            /* force the packet data to come from memory */
            SuperInvalidateDCache(send, sizeof(*send));

            if (send->hp_Busy == FALSE)
            {
                /* The channel is free for use */

                if (!IsEmptyList(&iorqs))
                {
                    /* If there are other IOReqs queued for service, insert any
                     * new one in the queue in priority order, and grab the
                     * first one off the list for dispatching to the host.
                     */

                    if (ior)
                    {
                        ior->io_Flags |= IO_ABORTABLE;
                        InsertNodeFromTail(&iorqs,(Node *)ior);
                    }

                    ior = (IOReq *)RemHead(&iorqs);
                }

                if (ior)
                {
                    PostRequest(ior);
                    didwork = TRUE;
                }
            }
            else if (ior)
            {
                /* The channel isn't free, so just queue the request for later */
                ior->io_Flags |= IO_ABORTABLE;
                InsertNodeFromTail(&iorqs,(Node *)ior);
            }
        }
        else if (ior)
        {
            /* The debugger ain't ready, so just queue the request for later */
            ior->io_Flags |= IO_ABORTABLE;
            InsertNodeFromTail(&iorqs,(Node *)ior);
        }
        ior = NULL;

        /* see if anything has come back from the debugger */
        while (ProcessIncoming())
        {
            didwork = TRUE;
        }
    }
    while (didwork);

#ifndef USE_BRIDGIT_INTR
    if (pending)
    {
        /* kick into hyperactive mode when we're waiting for a packet to come
         * back.
         */
        (*timer->tm_Load)(timer, &oneMilli);
    }
    else
    {
        (*timer->tm_Unload)(timer);
    }
#endif
}


/*****************************************************************************/


/* wake up and get some work done */
static int32 HostIntHandler(void)
{
#if 0
    /* rearm the interrupt */
    BRIDGIT_WRITE(BRIDGIT_BASE,(BR_INT_STS | BR_CLEAR_OFFSET),BR_NUBUS_INT);
    BDA_CLR(BDAPCTL_ERRSTAT,BDAINT_BRDG_MASK);
#endif

    ScheduleIO(NULL);

    return 0;
}


/*****************************************************************************/


static int32 CmdStatus(IOReq *ior)
{
DeviceStatus stat;
int32        len;

    memset(&stat,0,sizeof(stat));
    stat.ds_DriverIdentity    = DI_OTHER;
    stat.ds_MaximumStatusSize = sizeof(DeviceStatus);
    stat.ds_FamilyCode        = DS_DEVTYPE_OTHER;

    len = ior->io_Info.ioi_Recv.iob_Len;
    if (len > sizeof(stat))
        len = sizeof(stat);

    memcpy(ior->io_Info.ioi_Recv.iob_Buffer,&stat,len);

    ior->io_Actual = len;

    return 1;
}


/*****************************************************************************/


static int32 HostCmdSend(IOReq *ior)
{
uint32 oldints;

    if ((ior->io_Info.ioi_Send.iob_Len > DATASIZE)
     || (ior->io_Info.ioi_Recv.iob_Len > DATASIZE))
    {
        ior->io_Error = BADIOARG;
        return 1;
    }

    DBUG(("HostCmdSend: ior $%x, command $%x\n",ior, ior->io_Info.ioi_Command));

    ior->io_Flags &= (~IO_QUICK);
    oldints = Disable();
    ScheduleIO(ior);
    Enable(oldints);

    return 0;
}


/*****************************************************************************/


static int32 HostCmdRecv(IOReq *ior)
{
uint32  oldints;
Buffer *buf;
int32   result;

    if ((ior->io_Info.ioi_Send.iob_Len != 0)
     || (ior->io_Info.ioi_Recv.iob_Len > DATASIZE))
    {
        ior->io_Error = BADIOARG;
        return 1;
    }

    DBUG(("HostCmdRecv: ior $%x, command $%x\n",ior, ior->io_Info.ioi_Command));

    oldints = Disable();
    if (IsEmptyList(&usedBuffers))
    {
        ior->io_Flags &= (~IO_QUICK);
        ior->io_Flags |= IO_ABORTABLE;
        InsertNodeFromTail(&recviorqs,(Node *)ior);
        result = 0;
    }
    else
    {
        buf = (Buffer *)RemHead(&usedBuffers);
        AddHead(&freeBuffers, (Node *)buf);
        memcpy(ior->io_Info.ioi_Recv.iob_Buffer, buf->b_Data, ior->io_Info.ioi_Recv.iob_Len);
        result = 1;
    }
    Enable(oldints);

    return result;
}


/*****************************************************************************/


static Item CreateHostDriver(Driver *drv)
{
uint32  i;
TimeVal tv;
int32   err;
DebuggerInfo *dbinfo;

    err = QUERY_SYS_INFO(SYSINFO_TAG_DEBUGGERREGION, dbinfo);
    if (err != SYSINFO_SUCCESS)
	return err;
    send    = (HostPacket *) dbinfo->dbg_CommOutPtr;
    receive = (HostPacket *) dbinfo->dbg_CommInPtr;

    for (i = 0; i < NUM_BUFFERS; i++)
        AddTail(&freeBuffers, (Node *)&bufs[i]);

#ifdef USE_BRIDGIT_INTR
    firqItem = SuperCreateFIRQ("host",5,HostIntHandler,INT_BDA_BRIDGIT);
#else
    tv.tv_Seconds      = 0;
    tv.tv_Microseconds = 1000;
    ConvertTimeValToTimerTicks(&tv, &oneMilli);
    timerItem = CreateItemVA(MKNODEID(KERNELNODE, TIMERNODE),
                             TAG_ITEM_NAME,          "host",
                             CREATETIMER_TAG_HNDLR,  HostIntHandler,
                             CREATETIMER_TAG_PERIOD, &oneMilli,
                             TAG_END);
    timer = (Timer *)LookupItem(timerItem);

    firqItem = SuperCreateFIRQ("host",5,HostIntHandler,INT_V1);
#endif

    if (firqItem < 0)
	return firqItem;

#ifdef USE_BRIDGIT_INTR
    /* turn on the bridgit ints */
    old = BRIDGIT_READ(BRIDGIT_BASE,BR_INT_ENABLE);
    BRIDGIT_WRITE(BRIDGIT_BASE,BR_INT_ENABLE,(BR_NUBUS_INT | old) );

    /* allow bridgit ints to make it through */
    EnableInterrupt(INT_BDA_BRIDGIT);
#else
    EnableInterrupt(INT_V1);
#endif
    return drv->drv.n_Item;
}


/*****************************************************************************/


static Item ChangeHostDriverOwner(Driver *drv, Item newOwner)
{
	TOUCH(drv);
	SetItemOwner(firqItem, newOwner);
#ifndef USE_BRIDGIT_INTR
	SetItemOwner(timerItem, newOwner);
#endif
	return 0;
}


/*****************************************************************************/


static DriverCmdTable cmdTable[] =
{
    {HOST_CMD_SEND, HostCmdSend},
    {HOST_CMD_RECV, HostCmdRecv},
    {CMD_STATUS,    CmdStatus},
};

int32 main(void)
{
    return CreateItemVA(MKNODEID(KERNELNODE,DRIVERNODE),
	    TAG_ITEM_NAME,              "host",
	    TAG_ITEM_PRI,               1,
	    CREATEDRIVER_TAG_CMDTABLE,  cmdTable,
	    CREATEDRIVER_TAG_NUMCMDS,   sizeof(cmdTable) / sizeof(cmdTable[0]),
	    CREATEDRIVER_TAG_CREATEDRV, CreateHostDriver,
	    CREATEDRIVER_TAG_ABORTIO,   AbortHostIO,
	    CREATEDRIVER_TAG_CHOWN_DRV,	ChangeHostDriverOwner,
	    CREATEDRIVER_TAG_MODULE,    FindCurrentModule(),
	    TAG_END);
}

#else /* BUILD_MACDEBUGGER */

int main(void)
{
    return 0;
}

#endif
