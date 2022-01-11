/* @(#) ControlPort.c 96/08/23 1.70 */

/* The driver for the low-level DMA-driven Control Port device. */

#include <kernel/types.h>
#include <kernel/nodes.h>
#include <kernel/mem.h>
#include <kernel/list.h>
#include <kernel/listmacros.h>
#include <kernel/device.h>
#include <kernel/driver.h>
#include <kernel/kernelnodes.h>
#include <kernel/io.h>
#include <kernel/operror.h>
#include <kernel/interrupts.h>
#include <kernel/super.h>
#include <kernel/sysinfo.h>
#include <kernel/cache.h>
#include <misc/event.h>
#include <hardware/bda.h>
#include <hardware/PPCasm.h>
#include <loader/loader3do.h>
#include <stdio.h>
#include <string.h>


#define DBUG(x)            /* printf x */


/* CPBUFSIZE absolutely MUST be a multiple of 32 bytes! */
#define CPBUFSIZE 192

/* #define CAPTURE_LINES */
#define FILTERFLAGS (POD_StateFilterOK | POD_EventFilterOK)
#define CURRENT_VCNT  ((BDA_READ(BDAVDU_VLOC)>>3) & 0x1ff)

/* The number of pod samples per field - 1*/
#define POD_SAMPLES 3

static List readerList;
static List writerList;

static uint32 *controlPortOut;
static uint32 *controlPortIn;
static uint32 *controlPortCompare;
static uint32 *controlPortFlip;

static bool doFlipping;
static bool missedFlip;
static bool controlPortRunning;

static uint32 controlPortDones;

static int32 cpDMASize;

static int32 controlPortConfig1;
static int32 controlPortConfig1Fast;
static int32 controlPortConfig2;

static uint32 controlPortConfigurationFlags;

static Item vblFirqItem;
static Item dmaFirqItem;

static uint32 linesPerField;
static uint32 linesBetweenSamples;
#ifdef CAPTURE_LINES
#define CAPTURE_SIZE 512
static uint32 lineBuff[CAPTURE_SIZE];
static uint32 lineBuffIndex;
#define CAPTURE_LINE(x) {lineBuff[lineBuffIndex] = (x); lineBuffIndex++; lineBuffIndex &= (CAPTURE_SIZE - 1);}
#else
#define CAPTURE_LINE(x)
#endif


/*****************************************************************************/


static void ControlPortAbortIO(IOReq * ior)
{
    REMOVENODE((Node *) ior);
    ior->io_Error = ABORTED;
    SuperCompleteIO(ior);
}


/*****************************************************************************/


static void EnableControlPort(int32 startingLine)
{
    int32 interrupts;

    interrupts = Disable();

    BDA_WRITE(BDACP_CPST, (uint32) startingLine);
    BDA_WRITE(BDACP_CPCT, (uint32) cpDMASize);
    BDA_WRITE(BDACP_CPIA, (uint32) controlPortIn);
    BDA_WRITE(BDACP_CPOA, (uint32) controlPortOut);

/*
  Use the standard-speed clock if any device on the chain requires it,
  and for a few frames every few seconds (to catch newly-connected
  devices which can't even identify themselves correctly at the higher
  speed.
  */

    if ((controlPortConfigurationFlags & POD_FastClockOK) &&
        (controlPortDones & 0xFF) > 2)
    {
        BDA_WRITE(BDACP_CPCFG1, (uint32) controlPortConfig1Fast);
    }
    else
    {
        BDA_WRITE(BDACP_CPCFG1, (uint32) controlPortConfig1);
    }

    BDA_WRITE(BDACP_CPIE, (uint32) (BDACP_INT_INPUT_OVER_MASK |
                                    BDACP_INT_OUTPUT_UNDER_MASK |
                                    BDACP_INT_DMA_COMPLETE_MASK));
    BDA_WRITE(BDACP_CPCT, (uint32) (cpDMASize | (uint32) 0x80000000));
    controlPortRunning = TRUE;
    Enable(interrupts);
}


/*****************************************************************************/


/* called by interrupt routine at end of DMA, or by leech */
static void ControlPortDone(void)
{
    IOReq *ior;
    int32 indexVal;
    bool identical;

    BDA_WRITE(BDACP_CPIS, 0);
    BDA_WRITE(BDACP_CPIE, 0);
    controlPortRunning = FALSE;
    controlPortDones++;

    SuperInvalidateDCache(controlPortIn, 192);

    identical = TRUE;

    for (indexVal = 0;
         indexVal < CPBUFSIZE / sizeof *controlPortIn;
         indexVal++)
    {
        if (controlPortIn[indexVal] != controlPortCompare[indexVal])
        {
            identical = FALSE;
        }
        controlPortCompare[indexVal] = controlPortIn[indexVal];
    }

    if (!identical || ((controlPortConfigurationFlags & FILTERFLAGS) != FILTERFLAGS))
    {
        ior = (IOReq *) RemHead(&readerList);
        if (ior)
        {
            uint32 len;
            char *buf;

            buf = (char *) ior->io_Info.ioi_Recv.iob_Buffer;
            len = ior->io_Info.ioi_Recv.iob_Len;
            if (len > cpDMASize)
            len = cpDMASize;

            SampleSystemTimeTT(&((ControlPortInputHeader *) buf)->cpih_TimerTicks);
            SampleSystemTimeVBL(&((ControlPortInputHeader *) buf)->cpih_VBL);
            ((ControlPortInputHeader *) buf)->cpih_ScanLine = CURRENT_VCNT;
            ((ControlPortInputHeader *) buf)->cpih_IsIdentical = identical;
            memcpy(buf + sizeof(ControlPortInputHeader), controlPortIn, len - sizeof(ControlPortInputHeader));
            ior->io_Actual = len;
            SuperCompleteIO(ior);
        }
    }
}


/*****************************************************************************/


static bool ControlPortCheck(void)
{
    if ((BDA_READ(BDACP_CPIS) & (BDACP_INT_INPUT_OVER_MASK |
                                 BDACP_INT_OUTPUT_UNDER_MASK)) != 0)
    {
        BDA_WRITE(BDACP_CPIE, 0);
        BDA_WRITE(BDACP_CPIS, 0);
        controlPortRunning = FALSE;
        return FALSE;
    }
    if ((BDA_READ(BDACP_CPIS) & BDACP_INT_DMA_COMPLETE_MASK) == 0)
    {
        return TRUE;
    }
    return FALSE;
}


/*****************************************************************************/

#define TOP_LINE 20

static void ControlPortSetup(bool atVBL)
{
    IOReq *ior;
    uint32 writerVCNT;
    int32 i;
    uint32 *out, *flip;
    bool didWrite;
    int32 startingLine;
    int32 controlPortLine;

    writerVCNT = 0;
    didWrite = FALSE;

    controlPortLine = CURRENT_VCNT;
    CAPTURE_LINE(controlPortLine);

    if (atVBL)
    {
        if (controlPortLine >= TOP_LINE)
        {
            if (doFlipping)
            {
                missedFlip = !missedFlip;
            }
            return;
        }
        startingLine = TOP_LINE;
    }
    else
    {
        if (controlPortLine > linesPerField || controlPortLine < 18)
        {
            startingLine = TOP_LINE;    /* Don't start it in or near VBL */
        }
        else
        {
                /* What's the next line we want to start sampling at? */
            startingLine = (((((controlPortLine - TOP_LINE) / linesBetweenSamples) + 1) *
                             linesBetweenSamples) + TOP_LINE);
        }
    }

    if (IsEmptyList(&writerList) && !doFlipping)
    {                /** Fast path **/
        CAPTURE_LINE(0x80000000 | startingLine);
        EnableControlPort(startingLine);
        return;
    }

    while (TRUE)
    {
        ior = (IOReq *) RemHead(&writerList);
        if (!ior)
        break;

        {
            uint32 len, newDMASize;
            char *buf;

            DBUG(("Doing CPORT_CMD_WRITE on control port\n"));
            buf = (char *) ior->io_Info.ioi_Send.iob_Buffer;
            len = ior->io_Info.ioi_Send.iob_Len;
            writerVCNT = ior->io_Info.ioi_CmdOptions;
            newDMASize = writerVCNT >> 16;
            if (newDMASize > 8 && newDMASize <= CPBUFSIZE &&
                (newDMASize & 0x3) == 0)
            {
                if (cpDMASize != newDMASize)
                {
                    DBUG(("CP dma size set to %d\n", newDMASize));
                }
                cpDMASize = newDMASize;
            }
            didWrite = TRUE;
            missedFlip = FALSE;
            if (len == CPBUFSIZE)
            {
                memcpy(controlPortOut, buf, len);
                memset(controlPortFlip, 0, len);
                doFlipping = FALSE;
                ior->io_Actual = len;
            }
            else
            {
                memcpy(controlPortOut, buf, CPBUFSIZE);
                memcpy(controlPortFlip, CPBUFSIZE + (char *) buf,
                       CPBUFSIZE);
                doFlipping = TRUE;
                ior->io_Actual = len;
            }
            SuperCompleteIO(ior);
        }
    }

        /*
         * Flip-bits processing is done if either of the following is true:
         *
         * [1] If there was no new data written down by the Event Broker.
         *
         * [2] If the VCNT field indicates that the Event Broker is running an odd
         * number of fields behind.  Specifically:  the CP driver stores the
         * current VCNT into the ioi_CmdOptions of each Read request when the
         * read is processed ("This data was read at the beginning of the even
         * field").  The Event Broker echoes this count into the ioi_CmdOptions
         * of the Write request that it generates as a result of the read
         * completion ("This output data is being generated during an even field
         * and is intended to be written out to the port during the subsequent
         * odd field").
         *
         * If, when the write is actually processed, the field bit in the write
         * ioi_Cmdoptions _matches_ the field bit in the current VCNT, then it
         * indicates that the data is being written one field _later_ than it
         * should have been, and the bit-flipping must be performed.
         *
         * We apply a slight inhibition to this, though, because we sometimes get
         * into vblank so late that we can not send a new frame out.  We keep
         * track of the number of times that this has happened, and skip the
         * bitflip if we're an odd number of frames behind.
         *
         * It'd probably be better to keep _explicit_ track of which field the
         * output data is configured for, and flip this state (and the bits) when
         * we need to.
         */

    if (doFlipping && !missedFlip)
    {
        if (!didWrite || !((writerVCNT ^ CURRENT_VCNT) & 0x00000800))
        {
            i = CPBUFSIZE / sizeof(uint32);
            out = (uint32 *) controlPortOut;
            flip = (uint32 *) controlPortFlip;
            do
            {
                *out = *out ^ *flip;
                out++;
                flip++;
            } while (--i > 0);
        }
    }

    missedFlip = FALSE;

    FlushDCache(0, controlPortOut, 192);

    if (didWrite || !IsEmptyList(&readerList) || !IsEmptyList(&writerList))
    {
        CAPTURE_LINE(0x40000000 | startingLine);
        EnableControlPort(startingLine);
    }
}

/*****************************************************************************/
static int32 ControlPortVBL(void)
{
    if (controlPortRunning)
    {
        if (ControlPortCheck())
        {
            DBUG(("CP VBL entered while still running!\n"));
            return 0;
        }
        ControlPortDone();
    }

    CAPTURE_LINE(-1);
    ControlPortSetup(TRUE);

    return 0;
}


/*****************************************************************************/


static int32 ControlPortDMADone(void)
{
    ControlPortDone();
    if (controlPortConfigurationFlags & POD_MultipleFramesOK)
    {
        ControlPortSetup(FALSE);
    }
    return 0;
}

/*****************************************************************************/


static int32 CmdConfigure(IOReq * ior)
{
    DBUG(("Control Port config 0x%X\n", controlPortConfigurationFlags));
    controlPortConfigurationFlags = ior->io_Info.ioi_CmdOptions;
    return 1;
}


/*****************************************************************************/


static int32 CmdRead(IOReq * ior)
{
    char *buf;
    uint32 len;
    uint32 oldints;

    buf = (char *) ior->io_Info.ioi_Recv.iob_Buffer;
    len = ior->io_Info.ioi_Recv.iob_Len;
    if (buf == NULL ||
        len > (CPBUFSIZE + sizeof(ControlPortInputHeader)) ||
        len < sizeof(ControlPortInputHeader) ||
        (len & 0x3) != 0)
    {
        ior->io_Error = MakeKErr(ER_SEVERE, ER_C_STND, ER_BadIOArg);
        return 1;
    }

    ior->io_Flags &= ~IO_QUICK;
    oldints = Disable();
    InsertNodeFromTail(&readerList, (Node *) ior);
    Enable(oldints);

    return 0;
}


/*****************************************************************************/


static int32 CmdWrite(IOReq * ior)
{
    char *buf;
    uint32 len;
    uint32 oldints;

    buf = (char *) ior->io_Info.ioi_Send.iob_Buffer;
    len = ior->io_Info.ioi_Send.iob_Len;
    if (buf == NULL || (len != CPBUFSIZE && len != CPBUFSIZE * 2))
    {
        ior->io_Error = MakeKErr(ER_SEVERE, ER_C_STND, ER_BadIOArg);
        return 1;
    }

    ior->io_Flags &= ~IO_QUICK;
    oldints = Disable();
    InsertNodeFromTail(&writerList, (Node *) ior);
    Enable(oldints);

    DBUG(("Queued CPORT_CMD_WRITE for control port\n"));

    return 0;
}


/*****************************************************************************/


static int32 CmdStatus(IOReq * ior)
{
    DeviceStatus status;
    int32 len;

    memset(&status, 0, sizeof status);
    status.ds_MaximumStatusSize = sizeof status;
    status.ds_DeviceBlockSize = sizeof(uint32);
    status.ds_DeviceBlockCount = CPBUFSIZE / sizeof(uint32);
    status.ds_DeviceBlockStart = 0;
    len = ior->io_Info.ioi_Recv.iob_Len;
    if (len > sizeof status)
    {
        len = sizeof status;
    }
    if (len > 0)
    {
        memcpy(ior->io_Info.ioi_Recv.iob_Buffer, &status, len);
    }
    ior->io_Actual = len;
    return 1;
}


/*****************************************************************************/


static Item ControlPortInit(Driver * drv)
{
    Err err;
    uint8 *cpBufferBase;
    ContPortInfo controlPortSysInfo;
    DispModeInfo grafDispSysInfo;

#ifdef CAPTURE_LINES
    printf("--->lineBuff @ 0x%lx\n", lineBuff);
    lineBuffIndex = 0;
#endif

    DBUG(("ControlPortInit\n"));

    err = SuperQuerySysInfo(SYSINFO_TAG_CONTROLPORT, &controlPortSysInfo,
                            sizeof controlPortSysInfo);
    if (err < 0)
    {
        DBUG(("Can't get Control Port sysinfo: "));
        return err;
    }

    DBUG(("Control Port flags 0x%X\n",
          controlPortSysInfo.cpi_CPFlags));

    err = SuperQuerySysInfo(SYSINFO_TAG_GRAFDISP, &grafDispSysInfo,
                            sizeof grafDispSysInfo);
    if (err < 0)
    {
        DBUG(("Can't get GrafDisp sysinfo: "));
        return err;
    }

        /* This isn't the best way to determine lines per field; if the mode
         * changes then linesPerField will be wrong. A check should be made every
         * time this value is needed, but that is more overhead.
         *
         * In all likelihood, this will suffice.
         */
    linesPerField = ((grafDispSysInfo & SYSINFO_NTSC_DFLT) ? 200 : 256);
    linesBetweenSamples = (linesPerField / POD_SAMPLES);

    if (!(controlPortSysInfo.cpi_CPFlags & SYSINFO_CONTROLPORT_FLAG_PRESENT))
    {
        DBUG(("No Control Port!\n"));
        return MakeErr(ER_DEVC, ER_CPORT, ER_SEVERE, ER_E_SSTM, ER_C_STND, ER_NotSupported);
    }

    PrepList(&readerList);
    PrepList(&writerList);

    cpDMASize = CPBUFSIZE;

    cpBufferBase = SuperAllocMemAligned(CPBUFSIZE * 4, MEMTYPE_FILL, 64);
    if (!cpBufferBase)
    return MakeKErr(ER_SEVER, ER_C_STND, ER_NoMem);

    controlPortIn = (uint32 *) & cpBufferBase[0];
    controlPortCompare = (uint32 *) & cpBufferBase[CPBUFSIZE];
    controlPortOut = (uint32 *) & cpBufferBase[CPBUFSIZE * 2];
    controlPortFlip = (uint32 *) & cpBufferBase[CPBUFSIZE * 3];

    DBUG(("Creating FIRQs\n"));

    vblFirqItem = SuperCreateFIRQ("ControlPort setup", 200,
                                  ControlPortVBL, INT_V1);

    if (vblFirqItem < 0)
    {
        SuperFreeMem(cpBufferBase, CPBUFSIZE * 4);
        return vblFirqItem;
    }

    EnableInterrupt(INT_V1);

    dmaFirqItem = SuperCreateFIRQ("ControlPort done", 200,
                                  ControlPortDMADone, INT_BDA_PB);
    if (dmaFirqItem < 0)
    return dmaFirqItem;


    /* Here are the comments that should have accompanied the
     * next few lines:
     * On Jul 25, 12:59pm, Spencer Shanson wrote:
     * } In the ControlPortInit() code, you have
     * }
     * }     Long clock 7: 8 scan lines high, 8 scan lines low
     * }     Short clock: 3 ms / 40.7 ns - 1 = 72.71, make it 73
     * }     controlPortConfig1 = 0x00070049;
     * }     controlPortConfig1Fast = 0x00070012;
     * }
     * } for NTSC. Can you tell us why you are setting the long clock to 16 scan
     * } lines, and what the 3ms value is in the short clock.
     *
     * The 16 scan line period for the long clock is based on the original
     * Opera specs.  It generates a pulse which is long enough that all of
     * the control pads recognize the pulse as a "beginning of field"
     * synchronization.  At the trailing edge of the "high" portion (that is,
     * after 8 scan lines of "high"), all of the devices latch their current
     * inputs and outputs.
     *
     * If this pulse is made too short, the long-pulse detectors in some
     * control pads may not detect the pulse properly, and the devices will
     * not work reliably.
     *
     * The original short-pulse timing is based on the assumption that one
     * byte of data (8 bits) is transferred during each scan line.  Each scan
     * line is roughly 50 microseconds long (that's fixed by NTSC timings),
     * so 50 / 8 / 2 is roughly 3 microseconds high, 3 microseconds low.
     *
     * The pulse can be shortened (by a 4:1 ratio) if all devices on the port
     * work at the higher speed (i.e. they're M2 devices or are known to work
     * correctly).  If the pulse is made too short, data bits will not be
     * shifted correctly, and garbled input or output will occur.
     */
    if (grafDispSysInfo & SYSINFO_PAL_CURDISP)
    {
        DBUG(("Assuming PAL timing for Control Port\n"));
            /* Long clock 7: 8 scan lines high, 8 scan lines low */
            /* Short clock: 3 us / 33.9 ns - 1 = 87.49, make it 87 */
        controlPortConfig1 = 0x00070057;
        controlPortConfig1Fast = 0x00070016;
    }
    else
    {
        DBUG(("Assuming NTSC timing for Control Port\n"));
            /* Long clock 7: 8 scan lines high, 8 scan lines low */
            /* Short clock: 3 us / 40.7 ns - 1 = 72.71, make it 73 */
        controlPortConfig1 = 0x00070049;
        controlPortConfig1Fast = 0x00070012;
    }

    controlPortConfig2 = (uint32) 0x00000000;    /* snoop disable, 0 pulse
                                                  * sep. */

    DBUG(("Enabling Control Port DMA\n"));

    BDA_WRITE(BDACP_CPCFG1, controlPortConfig1);
    BDA_WRITE(BDACP_CPCFG2, controlPortConfig2);

    DBUG(("Enabling Control Port interrupt\n"));
    EnableInterrupt(INT_BDA_PB);

    DBUG(("Control Port initialization complete\n"));

    return drv->drv.n_Item;
}


/*****************************************************************************/


static Err ControlPortChangeDriverOwner(Driver * drv, Item newOwner)
{
    TOUCH(drv);
    SetItemOwner(vblFirqItem, newOwner);
    SetItemOwner(dmaFirqItem, newOwner);
    return 0;
}


/*****************************************************************************/


static const DriverCmdTable cmdTable[] =
{
    {CPORT_CMD_READ, CmdRead},
    {CPORT_CMD_WRITE, CmdWrite},
    {CMD_STATUS, CmdStatus},
    {CPORT_CMD_CONFIGURE, CmdConfigure},
};

int32 main(void)
{
    return CreateItemVA(MKNODEID(KERNELNODE, DRIVERNODE),
                        TAG_ITEM_NAME, "controlport",
                        CREATEDRIVER_TAG_CMDTABLE, cmdTable,
                        CREATEDRIVER_TAG_NUMCMDS, sizeof(cmdTable) / sizeof(cmdTable[0]),
                        CREATEDRIVER_TAG_CREATEDRV, ControlPortInit,
                        CREATEDRIVER_TAG_ABORTIO, ControlPortAbortIO,
                        CREATEDRIVER_TAG_CHOWN_DRV, ControlPortChangeDriverOwner,
                        CREATEDRIVER_TAG_MODULE, FindCurrentModule(),
                        TAG_END);
}
