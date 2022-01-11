/* @(#) microslot.c 96/09/06 1.62 */

/* This driver interfaces to the standard microslot port and provides the
 * low-level communication mechanism to control microcards.
 *
 * The driver supports a small set of I/O commands. These external
 * commands are expanded into a simpler set of microcommands. These
 * microcommands are atomic operations as far as the microcards are
 * concerned.
 *
 * To send a stream of commands to a microslot device, the driver must perform
 * a loop of the form:
 *
 *    while (more commands)
 *    {
 *        while (cde register is still full)
 *            ;
 *
 *        poke a command
 *    }
 *
 * A similar polling loop is needed when reading incoming bytes from a device.
 * This infortunate state of affairs is dealt with through a timer interrupt
 * handler. Before performing many of the microcommands, the state of the HW
 * is checked. If the HW isn't ready, the driver gives up and sets up a timer
 * to wake it up later. When it wakes up, it retries the operation.
 */

#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/device.h>
#include <kernel/dipir.h>
#include <kernel/driver.h>
#include <kernel/io.h>
#include <kernel/mem.h>
#include <kernel/operror.h>
#include <kernel/debug.h>
#include <kernel/kernel.h>
#include <kernel/super.h>
#include <kernel/sysinfo.h>
#include <kernel/timer.h>
#include <kernel/time.h>
#include <device/microslot.h>
#include <hardware/cde.h>
#include <hardware/PPCasm.h>
#include <hardware/microcard.h>
#include <loader/loader3do.h>
#include <string.h>


/*****************************************************************************/


/* #define DEBUG */
/* #define LOG_DEBUG */
/* #define CAN_DELETE_DRIVER */

#ifdef BUILD_STRINGS
#define WARN(x) printf x
#else
#define WARN(x)
#endif

#ifdef DEBUG
#define DBUG(x) printf x
#else
#define DBUG(x)
#endif


#ifdef LOG_DEBUG
#define	LOG_BUF_SIZE	(2*1024*1024)
uint32 *logbuf;
uint32 logbufx = 0;
#define	LOG(p,a,b,c,d)\
{\
    if (logbufx < LOG_BUF_SIZE/(5*sizeof(uint32)))\
    {\
        logbuf[logbufx++] = (p[0]<<24)|(p[1]<<16)|(p[2]<<8)|p[3]; _dcbf(&logbuf[logbufx-1]); \
        logbuf[logbufx++] = a; _dcbf(&logbuf[logbufx-1]); logbuf[logbufx++] = b; _dcbf(&logbuf[logbufx-1]); logbuf[logbufx++] = c; _dcbf(&logbuf[logbufx-1]); logbuf[logbufx++] = d; _dcbf(&logbuf[logbufx-1]);\
    }\
}
#else
#define LOG(p,a,b,c,d)
#endif


/*****************************************************************************/


/* number of slots we support */
#define NUM_USLOTS 4

static const uint32 CDEMicroReg[4] =
{
    CDE_MICRO_RWS,
    CDE_MICRO_WI,
    CDE_MICRO_WOB,
    CDE_MICRO_WO
};

#define INMICROSLOT(r)	CDE_READ(CDE_BASE, r)
#define OUTMICROSLOT(b)	CDE_WRITE(CDE_BASE, CDEMicroReg[(b)>>8], ((b) & 0xFF))

#define READBYTE()	INMICROSLOT(CDE_MICRO_RWS)
#define WRITEBYTE(b)	OUTMICROSLOT(MC_OUTBYTE | ((b) & 0xFF))
#define OUTSYNCH()	OUTMICROSLOT(MC_OUTSYNCH)
#define OUTSELECT(addr)	OUTMICROSLOT(MC_OUTSELECT | ((addr) << 2))
#define OUTRESET()	OUTMICROSLOT(MC_OUTRESET)
#define OUTIDENTIFY()	OUTMICROSLOT(MC_OUTIDENTIFY)
#define OUTDOWNLOAD()	OUTMICROSLOT(MC_OUTDOWNLOAD)



/*****************************************************************************/


/* attributes describing the general nature of a microcommand */
#define MICRO_ATTR_OUTPUT   (1<<0)   /* microcommand that performs output      */
#define MICRO_ATTR_INPUT    (1<<1)   /* microcommand that performs input       */
#define MICRO_ATTR_NEEDSLOT (1<<2)   /* needs a selected slot in order to run  */
#define MICRO_ATTR_NEEDCARD (1<<3)   /* needs an inserted card in order to run */

typedef enum
{
    /* 00 */  MICRO_SYNCH,      /* send an OutSynch packet, arg unused           */
    /* 01 */  MICRO_PREPSELECT, /* prep for a select opt, arg is slot #          */
    /* 02 */  MICRO_SELECT,     /* send an OutSelect packet, arg is slot #       */
    /* 03 */  MICRO_RESET,      /* send an OutReset packet, arg unused           */
    /* 04 */  MICRO_CDERESET,   /* toggle the CDE reset bit, arg unused          */
    /* 05 */  MICRO_IDENTIFY,   /* send an OutIdentify packet, arg unused        */
    /* 06 */  MICRO_GETID,      /* receive the result of OutIdentify, arg unused */
    /* 07 */  MICRO_PREPREAD,   /* prep for a read, arg is target IOBuf          */
    /* 08 */  MICRO_READ,       /* do a read operation, arg unused               */
    /* 09 */  MICRO_PREPWRITE,  /* prep for a write, arg is source IOBuf         */
    /* 0a */  MICRO_WRITE,      /* do a write operation, arg unused              */
    /* 0b */  MICRO_PREPVERIFY, /* prep for a verify, arg is reference IOBuf     */
    /* 0c */  MICRO_VERIFY,     /* do a verify operation, arg unused             */
    /* 0d */  MICRO_STARTDLD,   /* start a ROM download operation, arg unused    */
    /* 0e */  MICRO_STATUS,     /* fill in status info, arg unused               */
    /* 0f */  MICRO_COMPLETEIO, /* complete the current IOReq, arg unused        */
    /* 10 */  MICRO_PINGDONE,   /* clear ds_Ping, arg unused                     */
    /* 11 */  MICRO_END         /* end of a batch of commands, arg unused        */
} MicroCommands;


/* attribute table for all microcommands */
static const uint8 microAttrs[] =
{
    /* MICRO_SYNCH        */	(MICRO_ATTR_OUTPUT),
    /* MICRO_PREPSELECT   */	(MICRO_ATTR_OUTPUT),
    /* MICRO_SELECT       */	(MICRO_ATTR_OUTPUT),
    /* MICRO_RESET        */	(MICRO_ATTR_OUTPUT),
    /* MICRO_CDERESET     */	(0),
    /* MICRO_IDENTIFY     */	(MICRO_ATTR_OUTPUT | MICRO_ATTR_NEEDSLOT | MICRO_ATTR_NEEDCARD),
    /* MICRO_GETID        */	(MICRO_ATTR_INPUT | MICRO_ATTR_NEEDSLOT | MICRO_ATTR_NEEDCARD),
    /* MICRO_PREPREAD     */	(MICRO_ATTR_NEEDSLOT | MICRO_ATTR_NEEDCARD),
    /* MICRO_READ         */	(MICRO_ATTR_INPUT | MICRO_ATTR_NEEDSLOT | MICRO_ATTR_NEEDCARD),
    /* MICRO_PREPWRITE    */	(MICRO_ATTR_NEEDSLOT | MICRO_ATTR_NEEDCARD),
    /* MICRO_WRITE        */	(MICRO_ATTR_OUTPUT | MICRO_ATTR_NEEDSLOT | MICRO_ATTR_NEEDCARD),
    /* MICRO_PREPVERIFY   */	(MICRO_ATTR_NEEDSLOT | MICRO_ATTR_NEEDCARD),
    /* MICRO_VERIFY       */	(MICRO_ATTR_INPUT | MICRO_ATTR_NEEDSLOT | MICRO_ATTR_NEEDCARD),
    /* MICRO_STARTDLD     */	(MICRO_ATTR_OUTPUT | MICRO_ATTR_NEEDSLOT | MICRO_ATTR_NEEDCARD),
    /* MICRO_STATUS       */    (MICRO_ATTR_NEEDSLOT | MICRO_ATTR_NEEDCARD),
    /* MICRO_COMPLETEIO   */	(0),
    /* MICRO_PINGDONE     */	(0),
    /* MICRO_END          */    (0)
};


#ifdef DEBUG
/* name table for all microcommands */
static const char *microNames[] =
{
    "MICRO_SYNCH",
    "MICRO_PREPSELECT",
    "MICRO_SELECT",
    "MICRO_RESET",
    "MICRO_CDERESET",
    "MICRO_IDENTIFY",
    "MICRO_GETID",
    "MICRO_PREPREAD",
    "MICRO_READ",
    "MICRO_PREPWRITE",
    "MICRO_WRITE",
    "MICRO_PREPVERIFY",
    "MICRO_VERIFY",
    "MICRO_STARTDLD",
    "MICRO_STATUS",
    "MICRO_COMPLETEIO",
    "MICRO_PINGDONE",
    "MICRO_END"
};
#endif


/* one microcommand and its sole argument */
typedef struct
{
    MicroCommands mc_Command;      /* command code     */
    uint32        mc_Arg;          /* command argument */
} MicroCommand;

/* the maximum number of microcommands that can be queued at once */
#define MAX_MICROCOMMANDS 48

/* the maximum number of micro command executed in one go */
#define MAX_MICROCOMMANDS_EXECUTED 128


/*****************************************************************************/


typedef struct
{
    uint32 ss_CardChangeCount;
    uint8  ss_CardID;
} SlotState;

typedef struct
{
    IOReq        *ds_CurrentIOReq;
    List          ds_PendingIOReqs;
    uint32        ds_ClockSpeed;

    TimerTicks    ds_ShortTimeOut;
    TimerTicks    ds_LongTimeOut;
    Item          ds_CommandTimerItem;
    Timer        *ds_CommandTimer;
    Item          ds_PingTimerItem;

    TimerTicks    ds_PollDelay;
    IOBuf        *ds_CurrentBuf;
    uint32        ds_BufferIndex;

    MicroCommand *ds_RepeatCommand;
    MicroCommand  ds_Commands[MAX_MICROCOMMANDS];
    uint32        ds_NextCommand;
    uint32        ds_NextNewCommand;

    int32         ds_SelectedSlot;
    SlotState     ds_Slots[NUM_USLOTS];

    uint16        ds_OutFullCount;

    bool          ds_Dipir;
    bool          ds_Ping;
} DriverState;

/* which slot number is a particular IOReq targeted to */
#define SLOT(ior) ((ior)->io_Extension[0])


/*****************************************************************************/


static uint32      CDE_BASE;
static DriverState driverState;


/*****************************************************************************/


static bool CardPresent(Slot slot)
{
    HWResource hwr;

    hwr.hwr_InsertID = 0;
    while (NextHWResource(&hwr) >= 0)
    {
	if (hwr.hwr_Channel == CHANNEL_MICROCARD && hwr.hwr_Slot == slot)
	    return TRUE;
    }
    return FALSE;
}


/*****************************************************************************/


/* add a new microcommand to the queue */
static void AddMicroCommand(DriverState *ds, MicroCommands cmd, uint32 arg)
{
    LOG("addm", cmd, arg, ds->ds_NextNewCommand, ds->ds_NextCommand);

    ds->ds_Commands[ds->ds_NextNewCommand].mc_Command = cmd;
    ds->ds_Commands[ds->ds_NextNewCommand].mc_Arg     = arg;

    ds->ds_NextNewCommand++;
    if (ds->ds_NextNewCommand == MAX_MICROCOMMANDS)
    {
        ds->ds_NextNewCommand = 0;
    }

    if (ds->ds_NextNewCommand == ds->ds_NextCommand)
    {
        WARN(("MICROSLOT: ran out of microcommand expansion area while\n"));
        WARN(("           adding command %d, arg 0x%x\n",cmd,arg));
    }
}


/*****************************************************************************/


/* add a new microcommand with a 0 argument to the queue */
static void AddMicroCommand0(DriverState *ds, MicroCommands mc)
{
    AddMicroCommand(ds, mc, 0);
}


/*****************************************************************************/


/* fetch the next queued microcommand */
static MicroCommand *FetchMicroCommand(DriverState *ds)
{
MicroCommand *mc;

    if (ds->ds_NextCommand == ds->ds_NextNewCommand)
    {
        /* no more commands */
        return NULL;
    }

    mc = &ds->ds_Commands[ds->ds_NextCommand++];
    if (ds->ds_NextCommand == MAX_MICROCOMMANDS)
        ds->ds_NextCommand = 0;

    return mc;
}


/*****************************************************************************/


static void FlushMicroCommands(DriverState *ds, bool all)
{
MicroCommand *mc;

    ds->ds_RepeatCommand = NULL;
    if (all)
    {
        /* flush all microcommands */
        ds->ds_NextCommand    = 0;
        ds->ds_NextNewCommand = 0;
        ds->ds_Ping           = FALSE;
    }
    else
    {
        /* flush the current microcommand batch */
        do
        {
            mc = FetchMicroCommand(ds);
        }
        while (mc && (mc->mc_Command != MICRO_END));
    }
}


/*****************************************************************************/


static void CompleteMicroslotIO(DriverState *ds, Err result)
{
    if (ds->ds_CurrentIOReq)
    {
        ds->ds_CurrentIOReq->io_Error = result;
        SuperCompleteIO(ds->ds_CurrentIOReq);
        ds->ds_CurrentIOReq = NULL;
    }
}


/*****************************************************************************/


static void CmdSeq(DriverState *ds, IOReq *ior)
{
MicroSlotSeq *seq;
uint32        len;

    len = ior->io_Info.ioi_Send.iob_Len;

    if (len % sizeof(MicroSlotSeq))
    {
        CompleteMicroslotIO(ds, BADIOARG);
    }
    else if (len / sizeof(MicroSlotSeq) > 5)
    {
        /* we don't do more than 5 commands in one sequence */
        CompleteMicroslotIO(ds, BADSIZE);
    }
    else
    {
        for (seq = (MicroSlotSeq *)ior->io_Info.ioi_Send.iob_Buffer;
             seq->uss_Cmd != 0;
             seq++)
        {
            len -= sizeof(MicroSlotSeq);

            switch (seq->uss_Cmd)
            {
                case USSCMD_READ  : if (seq->uss_Len)
                                    {
                                        AddMicroCommand (ds, MICRO_PREPREAD, (uint32)&seq->uss_Buffer);
                                        AddMicroCommand0(ds, MICRO_READ);
                                    }
                                    break;

                case USSCMD_WRITE : if (seq->uss_Len)
                                    {
                                        AddMicroCommand (ds, MICRO_PREPWRITE, (uint32)&seq->uss_Buffer);
                                        AddMicroCommand0(ds, MICRO_WRITE);
                                    }
                                    break;

                case USSCMD_VERIFY: if (seq->uss_Len)
                                    {
                                        AddMicroCommand (ds, MICRO_PREPVERIFY, (uint32)&seq->uss_Buffer);
                                        AddMicroCommand0(ds, MICRO_VERIFY);
                                    }
                                    break;

                case USSCMD_STARTDOWNLOAD:
                                    AddMicroCommand0(ds, MICRO_STARTDLD);
                                    break;

                default           : CompleteMicroslotIO(ds, BADIOARG);
                                    FlushMicroCommands(ds, TRUE);
                                    return;
            }
        }

        AddMicroCommand0(ds, MICRO_COMPLETEIO);
        AddMicroCommand0(ds, MICRO_END);
    }
}


/*****************************************************************************/


static void CmdSetClock(DriverState *ds, IOReq *ior)
{
uint32 speed;
Err    result;

    speed = ior->io_Info.ioi_CmdOptions;
    if (speed > USLOT_CLKSPEED_8MHZ)
    {
	result = BADIOARG;
    }
    else
    {
        if (ds->ds_ClockSpeed != speed)
        {
            ds->ds_ClockSpeed = speed;
            CDE_WRITE(CDE_BASE, CDE_MICRO_STATUS, speed);
        }
        result = 0;
    }
    CompleteMicroslotIO(ds, result);
}


/*****************************************************************************/


typedef enum
{
    EXEC_FETCH,                 /* fetch the next micro command     */
    EXEC_REPEAT,                /* repeat the current micro command */
    EXEC_REPEAT_WITH_TIMEOUT,   /* repeat the current micro command after a timeout */
    EXEC_TIMEOUT_THEN_FETCH     /* wait for a timeout and then fetch the next micro command */
} ExecResults;

static ExecResults ExecuteMicroCommand(DriverState *ds, MicroCommand *mc)
{
uint32     status;
SlotState *slotState;
TimerTicks current, max;

    status = INMICROSLOT(CDE_MICRO_STATUS);

    LOG("exec", mc->mc_Command, mc->mc_Arg, ds->ds_SelectedSlot, status);
    DBUG(("\nMICROSLOT: %s, %x, slot %d, status 0x%02x\n", microNames[mc->mc_Command], mc->mc_Arg, ds->ds_SelectedSlot, status));

    if ((status & (CDE_MICRO_STAT_ATTN | CDE_MICRO_STAT_CARDLESS)) ==
		CDE_MICRO_STAT_ATTN)
    {
        /* a new card has been inserted, get DIPIR in the act */
        ds->ds_Dipir = TRUE;
	LOG("attn", 0, 0, ds->ds_SelectedSlot, status);
	WARN(("MICROSLOT: slot %d ATTN\n", ds->ds_SelectedSlot));
        TriggerDeviceRescan();
        DBUG(("MICROSLOT: exiting (ATTN bit set)\n"));
        return EXEC_REPEAT_WITH_TIMEOUT;
    }

    if (ds->ds_SelectedSlot >= 0)
    {
        if (status & CDE_MICRO_STAT_CARDLESS)
        {
            DBUG(("MICROSLOT: CARDLESS bit set\n"));

            if (CardPresent(ds->ds_SelectedSlot))
            {
		ds->ds_Dipir = TRUE;
		LOG("remv", 0, 0, ds->ds_SelectedSlot, status);
		WARN(("MICROSLOT: slot %d, CARDLESS\n", ds->ds_SelectedSlot));
                TriggerDeviceRescan();
                DBUG(("MICROSLOT: card removal detected, slot %d\n",ds->ds_SelectedSlot));
            }

            if (microAttrs[mc->mc_Command] & MICRO_ATTR_NEEDCARD)
            {
                if (ds->ds_CurrentIOReq)
                {
                    FlushMicroCommands(ds, FALSE);
                    CompleteMicroslotIO(ds, USLOT_ERR_OFFLINE);
                }

                DBUG(("MICROSLOT: exiting (no card present), slot %d\n", ds->ds_SelectedSlot));
                return EXEC_FETCH;
            }
        }
        else
        {
            if (!CardPresent(ds->ds_SelectedSlot))
            {
                DBUG(("MICROSLOT: card insertion detected, slot %d\n", ds->ds_SelectedSlot));
		LOG("insr", 0, 0, ds->ds_SelectedSlot, status);
                slotState = &ds->ds_Slots[ds->ds_SelectedSlot];
                slotState->ss_CardChangeCount++;
            }
        }
    }
    else
    {
        if (microAttrs[mc->mc_Command] & (MICRO_ATTR_NEEDCARD | MICRO_ATTR_NEEDSLOT))
        {
            DBUG(("MICROSLOT: exiting (dispatching %s with no slot selected)\n", microNames[mc->mc_Command]));
            return EXEC_FETCH;
        }
    }

    if (microAttrs[mc->mc_Command] & MICRO_ATTR_OUTPUT)
    {
        if (status & CDE_MICRO_STAT_OUTFULL)
        {
            ds->ds_OutFullCount++;
            if (ds->ds_OutFullCount == 3)
            {
                /* since the outfull register isn't getting cleared, seems like
                 * this is a single slot system that doesn't have anything
                 * inserted...
                 */
                FlushMicroCommands(ds, TRUE);
                if (ds->ds_CurrentIOReq)
                    CompleteMicroslotIO(ds, USLOT_ERR_OFFLINE);

                DBUG(("MICROSLOT: exiting (single slot with no card present), slot %d\n", ds->ds_SelectedSlot));
                ds->ds_OutFullCount = 0;

                return EXEC_FETCH;
            }

            SampleSystemTimeTT(&max);
            AddTimerTicks(&max, &ds->ds_PollDelay, &max);
            while (INMICROSLOT(CDE_MICRO_STATUS) & CDE_MICRO_STAT_OUTFULL)
            {
                SampleSystemTimeTT(&current);
                if (CompareTimerTicks(&current, &max) >= 0)
                {
                    DBUG(("MICROSLOT: exiting (output full, timeout set), slot %d\n",ds->ds_SelectedSlot));
                    return EXEC_REPEAT_WITH_TIMEOUT;
                }
            }

            DBUG(("MICROSLOT: exiting (output full), slot %d\n",ds->ds_SelectedSlot));
            return EXEC_REPEAT;
        }
        ds->ds_OutFullCount = 0;
    }
    else if (microAttrs[mc->mc_Command] & MICRO_ATTR_INPUT)
    {
        if ((status & CDE_MICRO_STAT_INFULL) == 0)
        {
            DBUG(("MICROSLOT: exiting (input empty), slot %d\n",ds->ds_SelectedSlot));
            return EXEC_REPEAT;
        }
    }

    switch (mc->mc_Command)
    {
        case MICRO_SYNCH         : OUTSYNCH();
                                   ds->ds_SelectedSlot = -1;
                                   break;

        case MICRO_PREPSELECT    : OUTSELECT(mc->mc_Arg);
                                   return EXEC_TIMEOUT_THEN_FETCH;

        case MICRO_SELECT        : OUTSELECT(mc->mc_Arg);
                                   ds->ds_SelectedSlot = mc->mc_Arg;
                                   break;

        case MICRO_RESET         : OUTRESET();
                                   ds->ds_SelectedSlot = -1;
                                   break;

        case MICRO_CDERESET      : CDE_WRITE(CDE_BASE, CDE_MICRO_STATUS, CDE_MICRO_STAT_RESET | status);
                                   CDE_WRITE(CDE_BASE, CDE_MICRO_STATUS, status);
                                   ds->ds_SelectedSlot = -1;
                                   break;

        case MICRO_IDENTIFY      : OUTIDENTIFY();
                                   break;

        case MICRO_GETID         : ds->ds_Slots[ds->ds_SelectedSlot].ss_CardID = READBYTE();
                                   break;

        case MICRO_PREPREAD      : ds->ds_CurrentBuf  = (IOBuf *)mc->mc_Arg;
                                   ds->ds_BufferIndex = 0;
                                   break;

        case MICRO_READ          : ds->ds_CurrentIOReq->io_Actual++;
                                   ((uint8 *)ds->ds_CurrentBuf->iob_Buffer)[ds->ds_BufferIndex++] = READBYTE();
                                   if (ds->ds_BufferIndex != ds->ds_CurrentBuf->iob_Len)
                                   {
                                       SampleSystemTimeTT(&max);
                                       AddTimerTicks(&max, &ds->ds_PollDelay, &max);
                                       while ((INMICROSLOT(CDE_MICRO_STATUS) & CDE_MICRO_STAT_INFULL) == 0)
                                       {
                                           SampleSystemTimeTT(&current);
                                           if (CompareTimerTicks(&current, &max) >= 0)
                                           {
                                               DBUG(("MICROSLOT: exiting (read timeout set), slot %d\n", ds->ds_SelectedSlot));
                                               return EXEC_REPEAT_WITH_TIMEOUT;
                                           }
                                       }
                                       return EXEC_REPEAT;
                                   }
                                   break;

        case MICRO_PREPWRITE     : ds->ds_CurrentBuf   = (IOBuf *)mc->mc_Arg;
                                   ds->ds_BufferIndex  = 0;
                                   break;

        case MICRO_WRITE         : if (ds->ds_BufferIndex != ds->ds_CurrentBuf->iob_Len)
                                   {
                                       ds->ds_CurrentIOReq->io_Actual++;
                                       WRITEBYTE(((uint8 *)ds->ds_CurrentBuf->iob_Buffer)[ds->ds_BufferIndex]);
				       ds->ds_BufferIndex++;

                                       SampleSystemTimeTT(&max);
                                       AddTimerTicks(&max, &ds->ds_PollDelay, &max);
                                       while (INMICROSLOT(CDE_MICRO_STATUS) & CDE_MICRO_STAT_OUTFULL)
                                       {
                                           SampleSystemTimeTT(&current);
                                           if (CompareTimerTicks(&current, &max) >= 0)
                                           {
                                               DBUG(("MICROSLOT: exiting (write timeout set), slot %d\n", ds->ds_SelectedSlot));
                                               return EXEC_REPEAT_WITH_TIMEOUT;
                                           }
                                       }
                                       return EXEC_REPEAT;
                                   }
                                   break;

        case MICRO_PREPVERIFY    : ds->ds_CurrentBuf  = (IOBuf *)mc->mc_Arg;
                                   ds->ds_BufferIndex = 0;
                                   break;

        case MICRO_VERIFY        : ds->ds_CurrentIOReq->io_Actual++;
                                   if (((uint8 *)ds->ds_CurrentBuf->iob_Buffer)[ds->ds_BufferIndex++] != READBYTE())
                                   {
                                       CompleteMicroslotIO(ds, USLOT_ERR_BADCARD);
                                       FlushMicroCommands(ds, FALSE);
                                   }
                                   else if (ds->ds_BufferIndex != ds->ds_CurrentBuf->iob_Len)
                                   {
                                       SampleSystemTimeTT(&max);
                                       AddTimerTicks(&max, &ds->ds_PollDelay, &max);
                                       while ((INMICROSLOT(CDE_MICRO_STATUS) & CDE_MICRO_STAT_INFULL) == 0)
                                       {
                                           SampleSystemTimeTT(&current);
                                           if (CompareTimerTicks(&current, &max) >= 0)
                                           {
                                               DBUG(("MICROSLOT: exiting (verify timeout set), slot %d\n", ds->ds_SelectedSlot));
                                               return EXEC_REPEAT_WITH_TIMEOUT;
                                           }
                                       }
                                       return EXEC_REPEAT;
                                   }
                                   break;

        case MICRO_STARTDLD      : OUTDOWNLOAD();
        			   ds->ds_CurrentIOReq->io_Actual++;
                                   break;

        case MICRO_STATUS        : {
                                   int32           len;
                                   MicroSlotStatus ms;

                                       memset(&ms, 0, sizeof(ms));
                                       ms.us_ds.ds_DriverIdentity    = DI_OTHER;
                                       ms.us_ds.ds_FamilyCode        = DS_DEVTYPE_OTHER;
                                       ms.us_ds.ds_MaximumStatusSize = sizeof(MicroSlotStatus);
                                       ms.us_ds.ds_DeviceUsageFlags  = (DS_USAGE_PRIVACCESS | DS_USAGE_EXTERNAL | DS_USAGE_REMOVABLE);

                                       slotState = &ds->ds_Slots[SLOT(ds->ds_CurrentIOReq)];

                                       ms.us_ds.ds_DeviceMediaChangeCntr = slotState->ss_CardChangeCount;
                                       ms.us_CardType                    = MC_CARDTYPE(slotState->ss_CardID);
                                       ms.us_CardVersion                 = MC_CARDVERSION(slotState->ss_CardID);
                                       ms.us_CardDownloadable            = MC_CARDDOWNLOADABLE(slotState->ss_CardID) ? TRUE : FALSE;
                                       ms.us_ClockSpeed                  = ds->ds_ClockSpeed;

                                       len = ds->ds_CurrentIOReq->io_Info.ioi_Recv.iob_Len;
                                       if (len > sizeof(MicroSlotStatus))
                                           len = sizeof(MicroSlotStatus);

                                       memcpy(ds->ds_CurrentIOReq->io_Info.ioi_Recv.iob_Buffer, &ms, len);

                                       ds->ds_CurrentIOReq->io_Actual = len;
                                   }
                                   break;

        case MICRO_COMPLETEIO    : CompleteMicroslotIO(ds, 0);
                                   break;

        case MICRO_PINGDONE      : ds->ds_Ping = FALSE;
                                   break;

        case MICRO_END           : /* noop */
                                   break;
    }

    DBUG(("MICROSLOT: exiting (normal), slot %d\n", ds->ds_SelectedSlot));
    return EXEC_FETCH;
}


/*****************************************************************************/


/* Dispatch a microcommand. If none are queued, fetch another IOReq, and
 * expand it into a sequence of microcommands.
 */
static void DispatchMicroCommands(DriverState *ds)
{
MicroCommand *mc;
IOReq        *ior;
uint32        numMicroCommands;

    if (ds->ds_Dipir)
    {
        /* about to dive into DIPIR, defer any work till DIPIR's done */
        return;
    }

    numMicroCommands = 0;
    while (TRUE)
    {
        /* see if any microcommands are currently queued */

        while (TRUE)
        {
            mc = ds->ds_RepeatCommand;
            if (!mc)
            {
                mc = FetchMicroCommand(ds);
                if (!mc)
                    break;
            }

            switch (ExecuteMicroCommand(ds, mc))
            {
                case EXEC_FETCH              : ds->ds_RepeatCommand = NULL;
                                               break;

                case EXEC_REPEAT             : ds->ds_RepeatCommand = mc;
                                               break;

                case EXEC_REPEAT_WITH_TIMEOUT: (* ds->ds_CommandTimer->tm_Load)(ds->ds_CommandTimer, &ds->ds_LongTimeOut);
                                               ds->ds_RepeatCommand = mc;
                                               LOG("time", 0, 0, 0, 0);
                                               return;

                case EXEC_TIMEOUT_THEN_FETCH : (* ds->ds_CommandTimer->tm_Load)(ds->ds_CommandTimer, &ds->ds_ShortTimeOut);
                                               ds->ds_RepeatCommand = NULL;
                                               LOG("time", 1, 0, 0, 0);
                                               return;
            }

            numMicroCommands++;
            if (numMicroCommands == MAX_MICROCOMMANDS_EXECUTED)
            {
                /* only run this loop at most MAX_MICROCOMMANDS_EXECUTED times */
                LOG("time", 2, 0, 0, 0);
                (* ds->ds_CommandTimer->tm_Load)(ds->ds_CommandTimer, &ds->ds_ShortTimeOut);
                return;
            }
        }

        /* see if there are any IOReqs to process */

        ds->ds_CurrentIOReq = (IOReq *)RemHead(&ds->ds_PendingIOReqs);
        ior                 = ds->ds_CurrentIOReq;
        if (!ior)
        {
            /* nothing to do */
            return;
        }

        /* convert the IOReq into a series of microcommands */

        if (ds->ds_SelectedSlot != SLOT(ior))
        {
            /* select the slot of interest */
            AddMicroCommand0(ds, MICRO_SYNCH);
            AddMicroCommand (ds, MICRO_PREPSELECT, SLOT(ior));
            AddMicroCommand (ds, MICRO_SELECT,     SLOT(ior));
        }

        switch (ior->io_Info.ioi_Command)
        {
            case USLOTCMD_SEQ      : CmdSeq(ds, ior);
                                     break;

            case CMD_STREAMREAD    : if (ior->io_Info.ioi_Recv.iob_Len)
                                     {
                                         AddMicroCommand (ds, MICRO_PREPREAD, (uint32)&ior->io_Info.ioi_Recv);
                                         AddMicroCommand0(ds, MICRO_READ);
                                         AddMicroCommand0(ds, MICRO_COMPLETEIO);
                                         AddMicroCommand0(ds, MICRO_END);
                                     }
                                     else
                                     {
                                         CompleteMicroslotIO(ds, 0);
                                     }
                                     break;

            case CMD_STREAMWRITE   : if (ior->io_Info.ioi_Send.iob_Len)
                                     {
                                         AddMicroCommand (ds, MICRO_PREPWRITE, (uint32)&ior->io_Info.ioi_Send);
                                         AddMicroCommand0(ds, MICRO_WRITE);
                                         AddMicroCommand0(ds, MICRO_COMPLETEIO);
                                         AddMicroCommand0(ds, MICRO_END);
                                     }
                                     else
                                     {
                                         CompleteMicroslotIO(ds, 0);
                                     }
                                     break;

            case USLOTCMD_RESET    : AddMicroCommand0(ds, MICRO_RESET);
                                     AddMicroCommand0(ds, MICRO_CDERESET);
                                     AddMicroCommand0(ds, MICRO_COMPLETEIO);
                                     AddMicroCommand0(ds, MICRO_END);
                                     break;

            case USLOTCMD_SETCLOCK : CmdSetClock(ds, ior);
                                     break;

            case CMD_STATUS        : AddMicroCommand0(ds, MICRO_IDENTIFY);
                                     AddMicroCommand0(ds, MICRO_GETID);
                                     AddMicroCommand0(ds, MICRO_STATUS);
                                     AddMicroCommand0(ds, MICRO_COMPLETEIO);
                                     AddMicroCommand0(ds, MICRO_END);
                                     break;

            default                : CompleteMicroslotIO(ds, BADCOMMAND);
                                     break;
        }
    }
}


/*****************************************************************************/


/* invoked on startup and then every second or so to detect card state */
static void HandlePing(DriverState *ds)
{
uint32 i;
bool   needDispatch;

    if (ds->ds_Dipir || ds->ds_Ping)
        return;

    /* This prevents this code from executing again while a previous batch
     * of ping-related microcommands still hasn't been processed. This ensures
     * we don't queue up too many microcommands.
     */
    ds->ds_Ping = TRUE;

    /* there's no sense in calling the dispatcher if we're only queuing our
     * commands behind others...
     */
    if (ds->ds_NextNewCommand == ds->ds_NextCommand)
        needDispatch = TRUE;
    else
        needDispatch = FALSE;

    AddMicroCommand0(ds, MICRO_CDERESET);
    for (i = 0; i < NUM_USLOTS; i++)
    {
        AddMicroCommand0(ds, MICRO_SYNCH);
        AddMicroCommand (ds, MICRO_PREPSELECT, i);
        AddMicroCommand (ds, MICRO_SELECT, i);
        AddMicroCommand0(ds, MICRO_IDENTIFY);
        AddMicroCommand0(ds, MICRO_GETID);
    }
    AddMicroCommand0(ds, MICRO_PINGDONE);
    AddMicroCommand0(ds, MICRO_END);

    if (needDispatch)
        DispatchMicroCommands(ds);
}


/*****************************************************************************/


static void AbortMicroslotIO(IOReq *ior)
{
DriverState *ds;

    ds = &driverState;

    if (ds->ds_CurrentIOReq == ior)
    {
        FlushMicroCommands(ds, TRUE);
        ds->ds_CurrentIOReq = NULL;
        ds->ds_SelectedSlot = -1;

        /* reset the hardware to prevent mass confusion */
        AddMicroCommand0(ds, MICRO_RESET);
        AddMicroCommand0(ds, MICRO_SYNCH);
        AddMicroCommand0(ds, MICRO_CDERESET);
        AddMicroCommand0(ds, MICRO_END);
        DispatchMicroCommands(ds);
    }
    else
    {
        /* it's in our pending queue, just yank it */
        RemNode((Node *)ior);
    }
    ior->io_Error = ABORTED;
    SuperCompleteIO(ior);
}


/*****************************************************************************/


/* called right before DIPIR is entered */
static void MicroslotDuck(DriverState *ds)
{
uint32 oldints;

    DBUG(("MICROSLOT: DUCK!\n"));

    oldints = Disable();

    /* zap everything that's queued, we either don't care about the
     * microcommands (those that come from a ping event), or the microcommands
     * will be redispatched because we're requeueing their causal IOReq.
     */
    FlushMicroCommands(ds, TRUE);

    /* requeue the current IOReq so it'll get reissued after we come back
     * from DIPIR.
     */
    if (ds->ds_CurrentIOReq)
    {
        AddHead(&ds->ds_PendingIOReqs, (Node *)ds->ds_CurrentIOReq);
        ds->ds_CurrentIOReq = NULL;
    }

    /* DIPIR is gonna screw up the current slot selection, so make sure
     * we resynchronize before we do anything else.
     */
    ds->ds_SelectedSlot = -1;
    Enable(oldints);
}


/*****************************************************************************/


/* called right after DIPIR exits */
static void MicroslotRecover(DriverState *ds)
{
    DBUG(("MICROSLOT: RECOVER!\n"));

    /* Let microcommands execute. The dispatcher is gonna startup again
     * the next time a ping event occurs, or the next time an IOReq is
     * dispatched, whichever occurs first.
     */
    ds->ds_Dipir = FALSE;
}


/*****************************************************************************/


static int32 MicroslotCommandTimerHandler(Timer *timer)
{
    DispatchMicroCommands((DriverState *)timer->tm_UserData);
    return 0;
}


/*****************************************************************************/


static int32 MicroslotPingTimerHandler(Timer *timer)
{
    HandlePing((DriverState *)timer->tm_UserData);
    return 0;
}


/*****************************************************************************/


static Err DispatchMicroslotIO(IOReq *ior)
{
uint32       oldints;
DriverState *ds;

    ds = &driverState;

    /* Only privileged code can use the microslot driver.
     *
     * If the callback is set, then the caller has enough privilege.
     * If the callback is not set, check to see if the owner of the
     * IOReq is privileged.
     */
    if (ior->io_CallBack == NULL)
    {
        if (!IsPriv((Task *)LookupItem(ior->io.n_Owner)))
        {
            ior->io_Error = BADPRIV;
            SuperCompleteIO(ior);
            return 1;
        }
    }

    ior->io_Flags &= ~IO_QUICK;
    oldints = Disable();
    InsertNodeFromTail(&ds->ds_PendingIOReqs, (Node *)ior);

    if ((ior->io_Flags & IO_INTERNAL) == 0)
        DispatchMicroCommands(ds);

    Enable(oldints);

    return 0;
}


/*****************************************************************************/


static Item CreateMicroslotIO(IOReq *ior)
{
HWResource hwr;
Err        result;

    result = FindHWResource(ior->io_Dev->dev_HWResource, &hwr);
    if (result < 0)
        return result;

    ior->io_Extension[0] = hwr.hwr_Slot;

    return ior->io.n_Item;
}


/*****************************************************************************/


static Item CreateMicroslotDriver(Driver *drv)
{
CDEInfo      ci;
Err          result;
uint32       oldints;
Timer       *timer;
TimeVal      tv;
TimerTicks   tt;
DriverState *ds;

    ds = &driverState;

#ifdef LOG_DEBUG
    logbuf = SuperAllocMem(LOG_BUF_SIZE, MEMTYPE_FILL);
    printf("logbuf %x\n", logbuf);
#endif

    if (SuperQuerySysInfo(SYSINFO_TAG_CDE, &ci, sizeof(ci)) != SYSINFO_SUCCESS)
        return NOSUPPORT;

    CDE_BASE = (uint32) ci.cde_Base;

    memset(ds, 0, sizeof(DriverState));

    result = RegisterDuck(MicroslotDuck, (uint32)ds);
    if (result >= 0)
    {
        result = RegisterRecover(MicroslotRecover, (uint32)ds);
        if (result >= 0)
        {
            ds->ds_CommandTimerItem = result = CreateItemVA(MKNODEID(KERNELNODE, TIMERNODE),
                                                TAG_ITEM_NAME,            "microslot command",
                                                CREATETIMER_TAG_HNDLR,    MicroslotCommandTimerHandler,
                                                CREATETIMER_TAG_USERDATA, ds,
                                                TAG_END);
            if (ds->ds_CommandTimerItem >= 0)
            {
                tv.tv_Seconds      = 0;
                tv.tv_Microseconds = 40;
                ConvertTimeValToTimerTicks(&tv, &ds->ds_PollDelay);

                tv.tv_Seconds      = 0;
                tv.tv_Microseconds = 30;
                ConvertTimeValToTimerTicks(&tv, &ds->ds_ShortTimeOut);

                tv.tv_Seconds      = 0;
                tv.tv_Microseconds = 7900;
                ConvertTimeValToTimerTicks(&tv, &ds->ds_LongTimeOut);

                tv.tv_Seconds      = 1;
                tv.tv_Microseconds = 0;
                ConvertTimeValToTimerTicks(&tv, &tt);

                ds->ds_PingTimerItem = result = CreateItemVA(MKNODEID(KERNELNODE, TIMERNODE),
                                                    TAG_ITEM_NAME,            "microslot ping",
                                                    CREATETIMER_TAG_HNDLR,    MicroslotPingTimerHandler,
                                                    CREATETIMER_TAG_USERDATA, ds,
                                                    CREATETIMER_TAG_PERIOD,   &tt,
                                                    TAG_END);
                if (ds->ds_PingTimerItem >= 0)
                {
                    ds->ds_CommandTimer = (Timer *)LookupItem(ds->ds_CommandTimerItem);
                    ds->ds_ClockSpeed   = USLOT_CLKSPEED_2MHZ;
                    ds->ds_SelectedSlot = -1;
                    PrepList(&ds->ds_PendingIOReqs);

                    oldints = Disable();
                    CDE_WRITE(CDE_BASE, CDE_MICRO_STATUS, ds->ds_ClockSpeed);
                    HandlePing(ds);
                    Enable(oldints);

                    timer = (Timer *)LookupItem(ds->ds_PingTimerItem);
                    (* timer->tm_Load)(timer, &tt);

                    return drv->drv.n_Item;
                }
                SuperDeleteItem(ds->ds_CommandTimerItem);
            }
            UnregisterRecover(MicroslotRecover);
        }
        UnregisterDuck(MicroslotDuck);
    }

    return result;
}


/*****************************************************************************/


static Err ChangeMicroslotDriverOwner(Driver *drv, Item newOwner)
{
DriverState *ds;

    TOUCH(drv);

    ds = &driverState;
    SetItemOwner(ds->ds_CommandTimerItem, newOwner);
    SetItemOwner(ds->ds_PingTimerItem, newOwner);
    return 0;
}


/*****************************************************************************/


#ifdef CAN_DELETE_DRIVER
static int32 DeleteMicroslotDriver(Driver *drv)
{
DriverState *ds;

    TOUCH(drv);

    ds = &driverState;

    UnregisterDuck(MicroslotDuck);
    UnregisterRecover(MicroslotRecover);
    SuperDeleteItem(ds->ds_CommandTimerItem);
    SuperDeleteItem(ds->ds_PingTimerItem);

    return 0;
}
#endif /* CAN_DELETE_DRIVER */


/*****************************************************************************/


Item MicroslotDriverMain(void)
{
Item driver;

    driver = CreateItemVA(MKNODEID(KERNELNODE, DRIVERNODE),
                        TAG_ITEM_NAME,              "microslot",
                        CREATEDRIVER_TAG_CREATEDRV, CreateMicroslotDriver,
#ifdef CAN_DELETEDRIVER
                        CREATEDRIVER_TAG_DELETEDRV, DeleteMicroslotDriver,
#endif
                        CREATEDRIVER_TAG_CRIO,      CreateMicroslotIO,
                        CREATEDRIVER_TAG_ABORTIO,   AbortMicroslotIO,
                        CREATEDRIVER_TAG_DISPATCH,  DispatchMicroslotIO,
                        CREATEDRIVER_TAG_CHOWN_DRV, ChangeMicroslotDriverOwner,
                        CREATEDRIVER_TAG_MODULE,    FindCurrentModule(),
                        TAG_END);
    if (driver < 0)
	return driver;

    return OpenItem(driver, 0);
}
