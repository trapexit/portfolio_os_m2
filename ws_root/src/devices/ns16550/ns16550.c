/* @(#) ns16550.c 96/08/21 1.23 */

#include <kernel/io.h>
#include <kernel/driver.h>
#include <kernel/device.h>
#include <kernel/task.h>
#include <kernel/debug.h>
#include <kernel/interrupts.h>
#include <kernel/operror.h>
#include <kernel/super.h>
#include <kernel/kernel.h>
#include <kernel/timer.h>
#include <kernel/time.h>
#include <kernel/mem.h>
#include <kernel/sysinfo.h>
#include <hardware/PPCasm.h>
#include <hardware/ns16550.h>
#include <hardware/cde.h>
#include <dipir/hw.pc16550.h>
#include <device/serial.h>
#include <device/slot.h>
#include <loader/loader3do.h>
#include <string.h>
#include <stdio.h>


/*****************************************************************************/


/* The 16550 documentation explains that to send out a break, one must wait
 * for the output FIFO to be empty, must write a 0 byte to the FIFO, wait for
 * the transmit buffer to be empty, set the break bit, wait for however
 * long the break should last, and finally clear the break bit. These states
 * are used to perform each step.
 */
typedef enum
{
    BREAK_PUSH_0,
    BREAK_SET_LCR,
    BREAK_WAIT_TEMT,
    BREAK_CLEAR_LCR
} BreakState;

typedef enum
{
    SWHAND_NOP,
    SWHAND_SEND_XOFF,
    SWHAND_SENT_XOFF,
    SWHAND_SEND_XON
} SWHandshakeState;

/* byte codes for xon/xoff handshaking */
#define XON  0x11
#define XOFF 0x13

typedef struct
{
    volatile NS16550 *ds_16550;
    uint32            ds_ClockFrequency;
    uint32            ds_FirqNum;
    Item              ds_SlotDev;

    IOReq            *ds_CurrentRead;
    IOReq            *ds_CurrentWrite;
    IOReq            *ds_CurrentBreak;

    List              ds_PendingReads;
    List              ds_PendingWrites;
    List              ds_PendingBreaks;
    List              ds_EventWaiters;

    /* used to time out read operations */
    Item              ds_ReadTimerItem;
    Timer            *ds_ReadTimer;
    TimerTicks        ds_ReadTimeout;

    /* used to calculate the duration of a break signal */
    Item              ds_BreakTimerItem;
    Timer            *ds_BreakTimer;

    uint8            *ds_OvflBuffer;           /* memory to use as overflow buffer   */
    uint32            ds_OvflBufferSize;       /* total size in bytes of buffer      */
    uint32            ds_OvflBufferStart;      /* where useful data starts in buffer */
    uint32            ds_OvflBufferAmount;     /* # bytes of useful data in buffer   */

    Item              ds_Firq;
    SerStatus         ds_Status;
    SerConfig         ds_Config;
    BreakState        ds_BreakState;
    SWHandshakeState  ds_SWHandshakeState;
    uint8             ds_LastMSR;
    uint8             ds_FCR;
    bool              ds_XOFF;
} DeviceState;

#define IER_FULL      (IER_EDSSI | IER_ELSI | IER_ERBFI | IER_ETBEI)
#define IER_NO_TXFIFO (IER_EDSSI | IER_ELSI | IER_ERBFI)


/*****************************************************************************/


/* default state when first opened */
static const SerConfig defaultConfig =
{
    57600,
    SER_HANDSHAKE_NONE,
    SER_WORDLENGTH_8,
    SER_PARITY_NONE,
    SER_STOPBITS_1
};


/*****************************************************************************/


static void FillOutputFIFO(DeviceState *ds);
static void DrainInputFIFO(DeviceState *ds);
static void HandleEventWaiters(DeviceState *ds, uint32 events);


/*****************************************************************************/


static void Abort16550IO(IOReq *ior)
{
DeviceState      *ds;
volatile NS16550 *regs;
uint8             lcr;

    ds = (DeviceState *)ior->io_Extension[0];
    if (ior == ds->ds_CurrentWrite)
    {
        ds->ds_CurrentWrite = NULL;
    }
    else if (ior == ds->ds_CurrentRead)
    {
        if (ior->io_Info.ioi_CmdOptions)
        {
            /* cancel time out */
            (* ds->ds_ReadTimer->tm_Unload)(ds->ds_ReadTimer);
        }

        ds->ds_CurrentRead = NULL;
    }
    else if (ior == ds->ds_CurrentBreak)
    {
        regs                = ds->ds_16550;
        lcr                 = regs->ns_LCR;
        lcr                &= ~(LCR_BREAK);
        regs->ns_LCR        = lcr;
        ds->ds_CurrentBreak = (IOReq *)RemHead(&ds->ds_PendingBreaks);
        ds->ds_BreakState   = BREAK_PUSH_0;
        (* ds->ds_BreakTimer->tm_Unload)(ds->ds_BreakTimer);
    }
    else
    {
        RemNode((Node *)ior);
    }

    ior->io_Error = SER_ERR_ABORTED;
    SuperCompleteIO(ior);
    DrainInputFIFO(ds);
    FillOutputFIFO(ds);
}


/*****************************************************************************/


static void BreakTimeoutHandler(const Timer *timer)
{
DeviceState      *ds;
volatile NS16550 *regs;
uint8             lcr;

    ds = (DeviceState *)timer->tm_UserData;
    if (ds->ds_BreakState == BREAK_CLEAR_LCR)
    {
        regs                = ds->ds_16550;
        lcr                 = regs->ns_LCR;
        lcr                &= ~(LCR_BREAK);
        regs->ns_LCR        = lcr;
        ds->ds_CurrentBreak = (IOReq *)RemHead(&ds->ds_PendingBreaks);
        ds->ds_BreakState   = BREAK_PUSH_0;
    }
    FillOutputFIFO(ds);
    DrainInputFIFO(ds);
}


/*****************************************************************************/


/* Handle a timeout on a CMD_STREAMREAD. The I/O is just completed and
 * returned to the client. We also drain the read FIFO as usual...
 */
static void ReadTimeoutHandler(const Timer *timer)
{
DeviceState *ds;
IOReq       *ior;

    ds                 = (DeviceState *)timer->tm_UserData;
    ior                = ds->ds_CurrentRead;
    ds->ds_CurrentRead = NULL;
    SuperCompleteIO(ior);
    FillOutputFIFO(ds);
    DrainInputFIFO(ds);
}


/*****************************************************************************/


/* Read the Line Status Register and deal with errors in the process */
static uint8 ReadLSR(DeviceState *ds)
{
volatile NS16550 *regs;
uint8             lsr;
uint32            events;

    regs = ds->ds_16550;
    lsr  = regs->ns_LSR;

    if ((lsr & (LSR_OE | LSR_FE | LSR_PE | LSR_BI)) == 0)
    {
        /* fast path */
        return lsr;
    }

    events = 0;

    if (lsr & LSR_OE)
    {
        ds->ds_Status.ss_NumOverflowErrors++;
        events |= SER_EVENT_OVERFLOW_ERROR;
    }

    if (lsr & LSR_FE)
    {
        ds->ds_Status.ss_NumFramingErrors++;
        events |= SER_EVENT_FRAMING_ERROR;
    }

    if (lsr & LSR_PE)
    {
        ds->ds_Status.ss_NumParityErrors++;
        events |= SER_EVENT_PARITY_ERROR;
    }

    if (lsr & LSR_BI)
    {
        ds->ds_Status.ss_NumBreaks++;
        events |= SER_EVENT_BREAK;
    }

    HandleEventWaiters(ds, events);

    return lsr;
}


/*****************************************************************************/


static uint8 ReadMSR(DeviceState *ds)
{
volatile NS16550 *regs;
uint8             msr;
uint32            events;

    regs   = ds->ds_16550;
    msr    = regs->ns_MSR;
    events = 0;

    if (msr & MSR_DCTS)
    {
        /* CTS state changed */

        if (msr & MSR_CTS)
        {
            events |= SER_EVENT_CTS_SET;
        }
        else
        {
            events |= SER_EVENT_CTS_CLEAR;
        }
    }

    if (msr & MSR_DDSR)
    {
        /* DSR state changed */

        if (msr & MSR_DSR)
            events |= SER_EVENT_DSR_SET;
        else
            events |= SER_EVENT_DSR_CLEAR;
    }

    if (msr & MSR_DDCD)
    {
        /* DCD state changed */

        if (msr & MSR_DCD)
            events |= SER_EVENT_DCD_SET;
        else
            events |= SER_EVENT_DCD_CLEAR;
    }

    if ((msr & MSR_RING) != (ds->ds_LastMSR & MSR_RING))
    {
        /* MSR_RING state changed */

        if (msr & MSR_RING)
            events |= SER_EVENT_RING_SET;
        else
            events |= SER_EVENT_RING_CLEAR;
    }

    ds->ds_LastMSR = msr;

    HandleEventWaiters(ds, events);

    return msr;
}


/*****************************************************************************/


/* When a THRE interrupt occurs, it means that the output FIFO is empty
 * and should be refilled. We scrouge as many bytes as possible from write-mode
 * I/O requests and shuve them into the FIFO.
 */
static void FillOutputFIFO(DeviceState *ds)
{
volatile NS16550 *regs;
uint8            *buffer;
uint8             lcr;
int32             len;
int32             room;
TimerTicks        tt;
TimeVal           tv;

    if ((ReadLSR(ds) & LSR_THRE) == 0)
    {
        /* Output FIFO is not empty, can't do anything. You know, this bit
         * really ought to mean "room available in FIFO" instead...
         */
        return;
    }

    regs = ds->ds_16550;

    if (ds->ds_CurrentBreak)
    {
        switch (ds->ds_BreakState)
        {
            case BREAK_PUSH_0   : regs->ns_THR      = 0;
                                  ds->ds_BreakState = BREAK_SET_LCR;
                                  break;

            case BREAK_SET_LCR  : lcr               = regs->ns_LCR;
                                  lcr              |= LCR_BREAK;
                                  regs->ns_LCR      = lcr;

#if 0
                                  /* According to the 16550 docs, after setting
                                   * LCR_BREAK, I should wait until LSR_TEMT is
                                   * set, and only then start to count a timeout.
                                   *
                                   * Unfortunately, short of polling, there's
                                   * no way to wait for TEMT to be clear. So
                                   * I'm ignoring what the docs say, and I'm
                                   * starting my timeout right now.
                                   *
                                   * I guess I could pad the timeout by an
                                   * amount equal to the amount of time
                                   * it takes to transmit one byte (plus stop
                                   * bits, parity bits, and all) at the
                                   * current baud rate. Maybe later.
                                   */

                                  ds->ds_BreakState = BREAK_WAIT_TEMT;
                                  break;

            case BREAK_WAIT_TEMT: lsr = ReadLSR(ds);
                                  if (lsr & LSR_TEMT)
#endif
                                  {
                                      ds->ds_BreakState  = BREAK_CLEAR_LCR;
                                      tv.tv_Seconds      = ds->ds_CurrentBreak->io_Info.ioi_CmdOptions / 1000000;
                                      tv.tv_Microseconds = ds->ds_CurrentBreak->io_Info.ioi_CmdOptions % 1000000;
                                      ConvertTimeValToTimerTicks(&tv, &tt);
                                      (* ds->ds_BreakTimer->tm_Load)(ds->ds_BreakTimer, &tt);
                                  }
                                  break;
        }
        return;
    }

    room = NS16550_OUTPUT_FIFO_SIZE;  /* we want to fill 'er up */

    if (ds->ds_SWHandshakeState == SWHAND_SEND_XOFF)
    {
        regs->ns_THR = XOFF;
        ds->ds_SWHandshakeState = SWHAND_SENT_XOFF;
        room--;
    }
    else if (ds->ds_SWHandshakeState == SWHAND_SEND_XON)
    {
        regs->ns_THR = XON;
        ds->ds_SWHandshakeState = SWHAND_NOP;
        room--;
    }

    /* see if the receiver can handle more data */
    if (ds->ds_Config.sc_Handshake == SER_HANDSHAKE_HW)
    {
        if ((ReadMSR(ds) & MSR_CTS) == 0)
            return;
    }
    else if (ds->ds_XOFF)
    {
        return;
    }

    do
    {
        if (ds->ds_CurrentWrite == NULL)
        {
            while (TRUE)
            {
                /* fetch the next packet to send... */
                ds->ds_CurrentWrite = (IOReq *)RemHead(&ds->ds_PendingWrites);

                if (ds->ds_CurrentWrite == NULL)
                {
                    regs->ns_IER = IER_NO_TXFIFO; /* don't need to hear when TX FIFO is empty */
                    return;
                }

                regs->ns_IER = IER_FULL;    /* need to get an int when the TX FIFO is empty */

                if (ds->ds_CurrentWrite->io_Info.ioi_Command == CMD_STREAMFLUSH)
                {
                    if ((ReadLSR(ds) & LSR_THRE) == 0)
                    {
                        /* The FIFO contains some bytes, so requeue this IOReq
                         * for now. When the FIFO empties, an int will occur
                         * and we'll process this IOReq again at that time.
                         */
                        AddHead(&ds->ds_PendingWrites, (Node *)ds->ds_CurrentWrite);
                        ds->ds_CurrentWrite = NULL;
                        return;
                    }
                    SuperCompleteIO(ds->ds_CurrentWrite);
                    continue;
                }
                break;
            }
        }

        buffer = ds->ds_CurrentWrite->io_Info.ioi_Send.iob_Buffer;
        len    = ds->ds_CurrentWrite->io_Info.ioi_Send.iob_Len - ds->ds_CurrentWrite->io_Actual;

        if (len > room)
        {
            do
            {
                regs->ns_THR = buffer[ds->ds_CurrentWrite->io_Actual++];
            }
            while (--room);

            /* FIFO full */
            return;
        }

        room -= len;

        do
        {
            regs->ns_THR = buffer[ds->ds_CurrentWrite->io_Actual++];
        }
        while (--len);

        SuperCompleteIO(ds->ds_CurrentWrite);
        ds->ds_CurrentWrite = (IOReq *)RemHead(&ds->ds_PendingWrites);
    }
    while (room);
}


/*****************************************************************************/


/* This routine is called to notify any event waiters that something
 * happened. Only the waiters waiting for any of the events that occured
 * get woken up. We also make sure that they only get informed of the events
 * they wanted.
 */
static void HandleEventWaiters(DeviceState *ds, uint32 events)
{
IOReq *waiter;
IOReq *next;

    for (waiter = (IOReq *)FirstNode(&ds->ds_EventWaiters);
         IsNode(&ds->ds_EventWaiters, waiter);
         waiter = next)
    {
        next = (IOReq *)NextNode(waiter);

        if (waiter->io_Info.ioi_CmdOptions & events)
        {
            RemNode((Node *)waiter);
            waiter->io_Actual = (events & waiter->io_Info.ioi_CmdOptions);
            SuperCompleteIO(waiter);
        }
    }
}


/*****************************************************************************/


/* set the receive interrupt trigger */
static void ResetIntTrigger(DeviceState *ds, int32 len)
{
uint8 fcr;

    if (ds->ds_CurrentRead->io_Info.ioi_CmdOptions)
    {
        (* ds->ds_ReadTimer->tm_Load)(ds->ds_ReadTimer, &ds->ds_ReadTimeout);
    }

    /* Set things up so we get an interrupt when a reasonable
     * number of bytes become available. We request to be woken
     * up earlier for smaller transfers so that we can complete
     * the IOReqs as fast as possible. This helps the MIDI folks
     * which often deal with small data transfers.
     */

    fcr  = ds->ds_FCR;
    fcr &= ~(FCR_RCVRTRIG0 | FCR_RCVRTRIG1);
    if (len >= 14)
    {
        fcr |= FCR_TRIG_14;
    }
    else if (len >= 8)
    {
        fcr |= FCR_TRIG_8;
    }
    else if (len >= 4)
    {
        fcr |= FCR_TRIG_4;
    }
    else
    {
        fcr |= FCR_TRIG_1;
    }
    ds->ds_FCR           = fcr;
    ds->ds_16550->ns_FCR = fcr;
}


/*****************************************************************************/


static void HandleOverflow(DeviceState *ds)
{
volatile NS16550 *regs;
uint8             mcr;
uint8             byte;
uint8             lsr;

    regs = ds->ds_16550;
    if (ds->ds_Config.sc_Handshake == SER_HANDSHAKE_HW)
    {
        /* can't take more data, turn off RTS */
        mcr          = regs->ns_MCR;
        mcr         &= ~(MCR_RTS);
        regs->ns_MCR = mcr;
    }
    else if (ds->ds_Config.sc_Handshake == SER_HANDSHAKE_SW)
    {
        /* can't take more data, XOFF the sender */
        ds->ds_SWHandshakeState = SWHAND_SEND_XOFF;
        FillOutputFIFO(ds);
    }

    /* since there's nobody waiting for this data, wake up as little as possible */
    ds->ds_FCR  |= FCR_TRIG_14;
    regs->ns_FCR = ds->ds_FCR;

    while (TRUE)
    {
        lsr = ReadLSR(ds);
        if ((lsr & LSR_DR) == 0)
        {
            /* FIFO is empty */
            return;
        }

        /* read a byte */
        byte = regs->ns_RBR;

        if (ds->ds_Config.sc_Handshake == SER_HANDSHAKE_SW)
        {
            if (byte == XOFF)
            {
                ds->ds_XOFF = TRUE;
                continue;
            }
            else if (byte == XON)
            {
                ds->ds_XOFF = FALSE;
                FillOutputFIFO(ds);
                continue;
            }
        }

        if (ds->ds_OvflBuffer)
        {
            ds->ds_OvflBuffer[(ds->ds_OvflBufferStart + ds->ds_OvflBufferAmount) % ds->ds_OvflBufferSize] = byte;
            if (ds->ds_OvflBufferAmount < ds->ds_OvflBufferSize)
            {
                ds->ds_OvflBufferAmount++;
                continue;
            }

            /* buffer is full, drop the oldest byte in the buffer */
            ds->ds_OvflBufferStart++;
        }

        /* no place to put the byte, drop it */
        ds->ds_Status.ss_NumDroppedBytes++;
    }
}


/*****************************************************************************/


static void DrainInputFIFO(DeviceState *ds)
{
volatile NS16550 *regs;
uint8            *buffer;
int32             len;
uint8             lsr;
uint8             mcr;
TimeVal           tv;
uint8             byte;

    regs = ds->ds_16550;

    while (TRUE)
    {
        if (ds->ds_CurrentRead == NULL)
        {
            ds->ds_CurrentRead = (IOReq *)RemHead(&ds->ds_PendingReads);
            if (ds->ds_CurrentRead == NULL)
            {
                /* no IOReq for the incoming data */
                HandleOverflow(ds);
                return;
            }

            if (ds->ds_Config.sc_Handshake == SER_HANDSHAKE_HW)
            {
                /* we can take more data, turn on RTS */
                mcr          = regs->ns_MCR;
                mcr         |= MCR_RTS;
                regs->ns_MCR = mcr;
            }
            else if (ds->ds_Config.sc_Handshake == SER_HANDSHAKE_SW)
            {
                if (ds->ds_SWHandshakeState == SWHAND_SENT_XOFF)
                {
                    ds->ds_SWHandshakeState = SWHAND_SEND_XON;
                    FillOutputFIFO(ds);
                }
            }

            if (ds->ds_CurrentRead->io_Info.ioi_CmdOptions)
            {
                tv.tv_Seconds      = ds->ds_CurrentRead->io_Info.ioi_CmdOptions / 1000000;
                tv.tv_Microseconds = ds->ds_CurrentRead->io_Info.ioi_CmdOptions % 1000000;
                ConvertTimeValToTimerTicks(&tv, &ds->ds_ReadTimeout);
            }
        }

        buffer = ds->ds_CurrentRead->io_Info.ioi_Recv.iob_Buffer;
        len    = ds->ds_CurrentRead->io_Info.ioi_Recv.iob_Len - ds->ds_CurrentRead->io_Actual;

        /* first off, drain anything in the overflow buffer */
        do
        {
            if (ds->ds_OvflBufferAmount == 0)
                break;

            byte = ds->ds_OvflBuffer[ds->ds_OvflBufferStart % ds->ds_OvflBufferSize];
            buffer[ds->ds_CurrentRead->io_Actual++] = byte;
            ds->ds_OvflBufferStart++;
            ds->ds_OvflBufferAmount--;
        }
        while (--len);

        if (ds->ds_Config.sc_Handshake == SER_HANDSHAKE_SW)
        {
            while (len--)
            {
                lsr = ReadLSR(ds);
                if ((lsr & LSR_DR) == 0)
                {
                    /* no more data to read */
                    ResetIntTrigger(ds, len);
                    return;
                }

                /* read a byte */
                byte = regs->ns_RBR;

                if (byte == XOFF)
                {
                    ds->ds_XOFF = TRUE;
                }
                else if (byte == XON)
                {
                    ds->ds_XOFF = FALSE;
                    FillOutputFIFO(ds);
                }
                else
                {
                    buffer[ds->ds_CurrentRead->io_Actual++] = byte;
                }
            }
        }
        else
        {
            while (len--)
            {
                lsr = ReadLSR(ds);
                if ((lsr & LSR_DR) == 0)
                {
                    /* no more data to read */
                    ResetIntTrigger(ds, len);
                    return;
                }

                /* read a byte */
                buffer[ds->ds_CurrentRead->io_Actual++] = regs->ns_RBR;
            }
        }

        if (ds->ds_CurrentRead->io_Info.ioi_CmdOptions)
        {
            /* cancel time out */
            (* ds->ds_ReadTimer->tm_Unload)(ds->ds_ReadTimer);
        }

        SuperCompleteIO(ds->ds_CurrentRead);
        ds->ds_CurrentRead = (IOReq *)RemHead(&ds->ds_PendingReads);
    }
}


/*****************************************************************************/


static int32 IntHandler(FirqNode *firq)
{
DeviceState      *ds;
volatile NS16550 *regs;
uint8             intSource;

    ds   = (DeviceState *)firq->firq_Data;
    regs = ds->ds_16550;

    /* clear interrupt source */
    ClearInterrupt(ds->ds_FirqNum);

    while (TRUE)
    {
        intSource = regs->ns_IIR;
        if (intSource & IIR_NOTPENDING)
        {
            /* no more ints pending, we're all done */
            return 0;
        }

        switch (intSource & (IIR_INT_ID0 | IIR_INT_ID1 | IIR_INT_ID2))
        {
            case IIR_ID_TXFIFOEMPTY    : FillOutputFIFO(ds);
                                         break;

            case IIR_ID_RXDATAAVAILABLE:
            case IIR_ID_CHARTIMEOUT    : DrainInputFIFO(ds);
                                         break;

            case IIR_ID_RXLINESTATUS   : /* process errors and event waiters */
                                         ReadLSR(ds);
                                         break;

            case IIR_ID_MODEMSTATUS    : ReadMSR(ds);         /* process event waiters */
                                         FillOutputFIFO(ds);  /* in case CTS went high */
                                         break;

            default                    : /* wow, how did we get here? */
                                         break;
        }
    }
}


/*****************************************************************************/


static void SetConfig(DeviceState *ds, const SerConfig *config)
{
volatile NS16550 *regs;
uint8             lcr;
uint16            divisor;

    regs = ds->ds_16550;
    lcr  = 0;

    switch (config->sc_WordLength)
    {
        case SER_WORDLENGTH_5: lcr |= LCR_5BIT; break;
        case SER_WORDLENGTH_6: lcr |= LCR_6BIT; break;
        case SER_WORDLENGTH_7: lcr |= LCR_7BIT; break;
        case SER_WORDLENGTH_8: lcr |= LCR_8BIT; break;
    }

    switch (config->sc_Parity)
    {
        case SER_PARITY_NONE : break;
        case SER_PARITY_EVEN : lcr |= LCR_PEN | LCR_EPS;             break;
        case SER_PARITY_ODD  : lcr |= LCR_PEN;                       break;
        case SER_PARITY_MARK : lcr |= LCR_PEN | LCR_STICK;           break;
        case SER_PARITY_SPACE: lcr |= LCR_PEN | LCR_EPS | LCR_STICK; break;
    }

    switch (config->sc_StopBits)
    {
        case SER_STOPBITS_1  : break;
        case SER_STOPBITS_2  : lcr |= LCR_STB; break;
    }

    /* reset the FIFOs to make sure no corrupt data will be sent out */
    regs->ns_FCR = 0;
    regs->ns_FCR = ds->ds_FCR;

    divisor = (uint16)(((float)ds->ds_ClockFrequency / (float)(config->sc_BaudRate*16)) + 0.5);

    regs->ns_LCR = lcr | LCR_DLAB;   /* set DLAB so we can access divisor latches */
    eieio();                         /* guarantee previous write completes        */
    regs->ns_DLL = divisor & 0xff;   /* update the divisor latches                */
    regs->ns_DLM = divisor >> 8;
    eieio();                         /* guarantees previous writes complete       */
    regs->ns_LCR = lcr;              /* reset DLAB to get back to normal          */

    /* note that we have just reset LCR_BREAK so any break signal being sent
     * out when the configuration gets changed will be blown away
     */

    ds->ds_XOFF             = FALSE;
    ds->ds_SWHandshakeState = SWHAND_NOP;
    ds->ds_OvflBufferStart  = 0;
    ds->ds_OvflBufferAmount = 0;
}


/*****************************************************************************/


static int32 CmdStreamWrite(IOReq *ior)
{
uint32            oldints;
DeviceState      *ds;
volatile NS16550 *regs;

    if (ior->io_Info.ioi_Send.iob_Len <= 0)
    {
        ior->io_Error = 0;
        return 1;
    }

    ds             = (DeviceState *)ior->io_Extension[0];
    regs           = ds->ds_16550;
    ior->io_Flags &= (~IO_QUICK);

    oldints = Disable();
    regs->ns_IER = IER_FULL;   /* need to get an int when the TX FIFO is empty */
    InsertNodeFromTail(&ds->ds_PendingWrites, (Node *)ior);
    FillOutputFIFO(ds);
    DrainInputFIFO(ds);
    Enable(oldints);

    return 0;
}


/*****************************************************************************/


static int32 CmdStreamFlush(IOReq *ior)
{
uint32            oldints;
DeviceState      *ds;
volatile NS16550 *regs;

    ds             = (DeviceState *)ior->io_Extension[0];
    regs           = ds->ds_16550;
    ior->io_Flags &= (~IO_QUICK);

    oldints = Disable();
    regs->ns_IER = IER_FULL;   /* need to get an int when the TX FIFO is empty */
    InsertNodeFromTail(&ds->ds_PendingWrites, (Node *)ior);
    FillOutputFIFO(ds);
    DrainInputFIFO(ds);
    Enable(oldints);

    return 0;
}


/*****************************************************************************/


static int32 CmdBreak(IOReq *ior)
{
DeviceState      *ds;
volatile NS16550 *regs;
uint32            oldints;

    ds   = (DeviceState *)ior->io_Extension[0];
    regs = ds->ds_16550;

    ior->io_Flags &= (~IO_QUICK);

    oldints = Disable();
    if (ds->ds_CurrentBreak == NULL)
    {
        regs->ns_IER        = IER_FULL;   /* need to get an int when the TX FIFO is empty */
        ds->ds_CurrentBreak = ior;
        ds->ds_BreakState   = BREAK_PUSH_0;
        FillOutputFIFO(ds);
    }
    else
    {
        InsertNodeFromTail(&ds->ds_PendingBreaks, (Node *)ior);
    }
    Enable(oldints);

    return 0;
}


/*****************************************************************************/


static int32 CmdStreamRead(IOReq *ior)
{
uint32       oldints;
DeviceState *ds;

    if (ior->io_Info.ioi_Recv.iob_Len <= 0)
    {
        ior->io_Error = 0;
        return 1;
    }

    ds             = (DeviceState *)ior->io_Extension[0];
    ior->io_Flags &= (~IO_QUICK);

    oldints = Disable();
    InsertNodeFromTail(&ds->ds_PendingReads, (Node *)ior);
    DrainInputFIFO(ds);
    FillOutputFIFO(ds);
    Enable(oldints);

    return 0;
}


/*****************************************************************************/


static int32 CmdSetConfig(IOReq *ior)
{
SerConfig    newConfig;
DeviceState *ds;
int32        len;
Err          result;
uint32       oldints;
uint32       perms;

    ds        = (DeviceState *)ior->io_Extension[0];
    newConfig = ds->ds_Config;

    len = ior->io_Info.ioi_Send.iob_Len;
    if (len > sizeof(newConfig))
        len = sizeof(newConfig);

    memcpy(&newConfig, ior->io_Info.ioi_Send.iob_Buffer, len);

    /* validate configuration parameters */
    result = QUERY_SYS_INFO(SYSINFO_TAG_DEVICEPERMS, perms);
    if (result < 0)
	return result;

    if (((perms & SYSINFO_DEVICEPERM_SERIAL_HI) == 0) && (newConfig.sc_BaudRate > 64000))
    {
        result = SER_ERR_BADBAUDRATE;
    }
    else if ((newConfig.sc_Handshake < SER_HANDSHAKE_NONE) || (newConfig.sc_Handshake > SER_HANDSHAKE_SW))
    {
        result = SER_ERR_BADHANDSHAKE;
    }
    else if ((newConfig.sc_WordLength < SER_WORDLENGTH_5) || (newConfig.sc_WordLength > SER_WORDLENGTH_8))
    {
        result = SER_ERR_BADWORDLENGTH;
    }
    else if ((newConfig.sc_Parity < SER_PARITY_NONE) || (newConfig.sc_Parity > SER_PARITY_SPACE))
    {
        result = SER_ERR_BADPARITY;
    }
    else if ((newConfig.sc_StopBits < SER_STOPBITS_1) || (newConfig.sc_StopBits > SER_STOPBITS_2))
    {
        result = SER_ERR_BADSTOPBITS;
    }
    else if (newConfig.sc_OverflowBufferSize != ds->ds_OvflBufferSize)
    {
        SuperFreeMem(ds->ds_OvflBuffer, ds->ds_OvflBufferSize);
        ds->ds_OvflBufferSize = newConfig.sc_OverflowBufferSize;
        ds->ds_OvflBuffer = SuperAllocMem(newConfig.sc_OverflowBufferSize, MEMTYPE_NORMAL);
        if (!ds->ds_OvflBuffer)
            result = SER_ERR_NOMEM;
    }

    if (result >= 0)
    {
        ds->ds_Config = newConfig;
        oldints = Disable();
        SetConfig(ds, &ds->ds_Config);
        Enable(oldints);
        ior->io_Actual = len;
    }

    ior->io_Error = result;
    return 1;
}


/*****************************************************************************/


static int32 CmdGetConfig(IOReq *ior)
{
DeviceState *ds;
int32        len;

    ds = (DeviceState *)ior->io_Extension[0];

    len = ior->io_Info.ioi_Recv.iob_Len;
    if (len > sizeof(SerConfig))
        len = sizeof(SerConfig);

    memcpy(ior->io_Info.ioi_Recv.iob_Buffer, &ds->ds_Config, len);
    ior->io_Actual = len;

    return 1;
}


/*****************************************************************************/


static int32 CmdStatus(IOReq *ior)
{
DeviceState      *ds;
volatile NS16550 *regs;
int32             len;
uint32            oldints;
uint8             mcr;

    ds   = (DeviceState *)ior->io_Extension[0];
    regs = ds->ds_16550;
    len  = ior->io_Info.ioi_Recv.iob_Len;

    if (len > sizeof(SerStatus))
        len = sizeof(SerStatus);

    ds->ds_Status.ss_Flags = 0;

    oldints = Disable();

    mcr = regs->ns_MCR;

    if (ds->ds_LastMSR & MSR_CTS)
        ds->ds_Status.ss_Flags |= SER_STATE_CTS;

    if (ds->ds_LastMSR & MSR_DCD)
        ds->ds_Status.ss_Flags |= SER_STATE_DCD;

    if (ds->ds_LastMSR & MSR_DSR)
        ds->ds_Status.ss_Flags |= SER_STATE_DSR;

    if (ds->ds_LastMSR & MSR_RING)
        ds->ds_Status.ss_Flags |= SER_STATE_RING;

    if (mcr & MCR_LOCALLOOPBACK)
        ds->ds_Status.ss_Flags |= SER_STATE_LOOPBACK;

    if (mcr & MCR_RTS)
        ds->ds_Status.ss_Flags |= SER_STATE_RTS;

    if (mcr & MCR_DTR)
        ds->ds_Status.ss_Flags |= SER_STATE_DTR;

    memcpy(ior->io_Info.ioi_Recv.iob_Buffer, &ds->ds_Status, len);

    Enable(oldints);

    ior->io_Actual = len;

    return 1;
}


/*****************************************************************************/


static int32 CmdWaitEvent(IOReq *ior)
{
uint32       oldints;
DeviceState *ds;

    if (ior->io_Info.ioi_CmdOptions & ~(SER_EVENT_CTS_SET |
                                        SER_EVENT_DSR_SET |
                                        SER_EVENT_DCD_SET |
                                        SER_EVENT_RING_SET |
                                        SER_EVENT_CTS_CLEAR |
                                        SER_EVENT_DSR_CLEAR |
                                        SER_EVENT_DCD_CLEAR |
                                        SER_EVENT_RING_CLEAR |
                                        SER_EVENT_OVERFLOW_ERROR |
                                        SER_EVENT_PARITY_ERROR |
                                        SER_EVENT_FRAMING_ERROR |
                                        SER_EVENT_BREAK))
    {
        ior->io_Error = SER_ERR_BADEVENT;
        return 1;
    }

    ds             = (DeviceState *)ior->io_Extension[0];
    ior->io_Flags &= (~IO_QUICK);

    oldints = Disable();
    AddTail(&ds->ds_EventWaiters, (Node *)ior);
    Enable(oldints);

    return 0;
}


/*****************************************************************************/


static int32 CmdSetRTS(IOReq *ior)
{
DeviceState      *ds;
volatile NS16550 *regs;
uint32            oldints;
uint8             mcr;

    ds   = (DeviceState *)ior->io_Extension[0];
    regs = ds->ds_16550;

    if (ds->ds_Config.sc_Handshake == SER_HANDSHAKE_HW)
        return SER_ERR_BADHANDSHAKE;

    oldints = Disable();

    mcr = regs->ns_MCR;
    if (ior->io_Info.ioi_CmdOptions)
        mcr |= MCR_RTS;
    else
        mcr &= ~(MCR_RTS);
    regs->ns_MCR = mcr;

    Enable(oldints);

    return 1;
}


/*****************************************************************************/


static int32 CmdSetDTR(IOReq *ior)
{
DeviceState      *ds;
volatile NS16550 *regs;
uint32            oldints;
uint8             mcr;

    ds   = (DeviceState *)ior->io_Extension[0];
    regs = ds->ds_16550;

    oldints = Disable();

    mcr = regs->ns_MCR;
    if (ior->io_Info.ioi_CmdOptions)
        mcr |= MCR_DTR;
    else
        mcr &= ~(MCR_DTR);
    regs->ns_MCR = mcr;

    Enable(oldints);

    return 1;
}


/*****************************************************************************/


static int32 CmdSetLoopback(IOReq *ior)
{
DeviceState      *ds;
volatile NS16550 *regs;
uint32            oldints;
uint8             mcr;

    ds   = (DeviceState *)ior->io_Extension[0];
    regs = ds->ds_16550;

    oldints = Disable();

    mcr = regs->ns_MCR;
    if (ior->io_Info.ioi_CmdOptions)
        mcr |= MCR_LOCALLOOPBACK;
    else
        mcr &= ~(MCR_LOCALLOOPBACK);
    regs->ns_MCR = mcr;

    Enable(oldints);

    return 1;
}


/*****************************************************************************/


static Item SetupHW(HardwareID resID, HWResource *res, MapRangeResponse *mrres,
                    SlotDeviceStatus *sds)
{
Err             result;
Item            slotDev;
Item            slotIO;
IOReq          *ior;
MapRangeRequest mrreq;
SlotTiming      slotTiming;
CDEInfo         ci;

    if (SuperQuerySysInfo(SYSINFO_TAG_CDE, &ci, sizeof(ci)) != SYSINFO_SUCCESS)
        return NOSUPPORT;

    result = FindHWResource(resID, res);
    if (result >= 0)
    {
        slotDev = result = OpenSlotDevice(resID);
        if (slotDev >= 0)
        {
            slotIO = result = CreateIOReq(NULL, 0, slotDev, 0);
            if (slotIO >= 0)
            {
                ior = IOREQ(slotIO);

                mrreq.mrr_Flags      = (MM_READABLE | MM_WRITABLE);
                mrreq.mrr_BytesToMap = sizeof(NS16550);

                memset(&ior->io_Info, 0, sizeof(IOInfo));
                ior->io_Info.ioi_Command         = CMD_MAPRANGE;
                ior->io_Info.ioi_Send.iob_Buffer = &mrreq;
                ior->io_Info.ioi_Send.iob_Len    = sizeof(MapRangeRequest);
                ior->io_Info.ioi_Recv.iob_Buffer = mrres;
                ior->io_Info.ioi_Recv.iob_Len    = sizeof(MapRangeResponse);
                result = SuperInternalDoIO(ior);
                if (result >= 0)
                {
                    memset(&ior->io_Info, 0, sizeof(IOInfo));
                    ior->io_Info.ioi_Command         = CMD_STATUS;
                    ior->io_Info.ioi_Recv.iob_Buffer = sds;
                    ior->io_Info.ioi_Recv.iob_Len    = sizeof(SlotDeviceStatus);
                    result = SuperInternalDoIO(ior);
                    if (result >= 0)
                    {
                        slotTiming.st_Flags             = 0;
                        slotTiming.st_DeviceWidth       = 8;
                        slotTiming.st_CycleTime         = ((HWResource_PC16550 *)res->hwr_DeviceSpecific)->pcs_CycleTime;
                        slotTiming.st_PageModeCycleTime = ST_NOCHANGE;
                        slotTiming.st_ReadHoldTime      = ST_NOCHANGE;
                        slotTiming.st_ReadSetupTime     = ST_NOCHANGE;
                        slotTiming.st_WriteHoldTime     = ST_NOCHANGE;
                        slotTiming.st_WriteSetupTime    = ST_NOCHANGE;
                        slotTiming.st_IOReadSetupTime   = ST_NOCHANGE;

                        memset(&ior->io_Info, 0, sizeof(IOInfo));
                        ior->io_Info.ioi_Command         = SLOTCMD_SETTIMING;
                        ior->io_Info.ioi_Send.iob_Buffer = &slotTiming;
                        ior->io_Info.ioi_Send.iob_Len    = sizeof(SlotTiming);
                        result = SuperInternalDoIO(ior);
                        if (result >= 0)
                            result = slotDev;
                    }
                }
                DeleteIOReq(slotIO);
            }

            if (result < 0)
                CloseItem(slotDev);
        }
    }

    return result;
}


/*****************************************************************************/


static Item Create16550Device(Device *dev)
{
HWResource        hwr;
MapRangeResponse  mrres;
SlotDeviceStatus  sds;
DeviceState      *ds;
volatile NS16550 *regs;
Err               result;
uint8             oldPriv;

    oldPriv = PromotePriv(CURRENTTASK);

    ds = (DeviceState *)dev->dev_DriverData;

    ds->ds_SlotDev = result = SetupHW(dev->dev_HWResource, &hwr, &mrres, &sds);
    if (ds->ds_SlotDev >= 0)
    {
        ds->ds_BreakTimerItem = result = CreateItemVA(MKNODEID(KERNELNODE, TIMERNODE),
                                           TAG_ITEM_NAME,            "ns16550 break timeout",
                                           CREATETIMER_TAG_HNDLR,    BreakTimeoutHandler,
                                           CREATETIMER_TAG_USERDATA, ds,
                                           TAG_END);
        if (ds->ds_BreakTimerItem >= 0)
        {
            ds->ds_ReadTimerItem = result = CreateItemVA(MKNODEID(KERNELNODE, TIMERNODE),
                                           TAG_ITEM_NAME,            "ns16550 read timeout",
                                           CREATETIMER_TAG_HNDLR,    ReadTimeoutHandler,
                                           CREATETIMER_TAG_USERDATA, ds,
                                           TAG_END);
            if (ds->ds_ReadTimerItem >= 0)
            {
                ds->ds_16550          = mrres.mrr_MappedArea;
                ds->ds_ClockFrequency = ((HWResource_PC16550 *)hwr.hwr_DeviceSpecific)->pcs_Clock;
                ds->ds_FirqNum        = sds.sds_IntrNum;

                PrepList(&ds->ds_PendingReads);
                PrepList(&ds->ds_PendingWrites);
                PrepList(&ds->ds_PendingBreaks);
                PrepList(&ds->ds_EventWaiters);

                ds->ds_ReadTimer  = (Timer *)LookupItem(ds->ds_ReadTimerItem);
                ds->ds_BreakTimer = (Timer *)LookupItem(ds->ds_BreakTimerItem);
                ds->ds_Config     = defaultConfig;
                ds->ds_FCR        = FCR_FIFOENABLE | FCR_TRIG_14;

                /* set the 16550 to a known safe state */
                regs         = ds->ds_16550;
                regs->ns_IER = 0;       /* make sure ints are turned off */
                regs->ns_LCR = 0;       /* reset break signal            */
                regs->ns_MCR = MCR_DTR; /* tell the world we're here     */

                SetConfig(ds, &ds->ds_Config);

                ds->ds_Firq = result = CreateItemVA(MKNODEID(KERNELNODE, FIRQNODE),
                                                    TAG_ITEM_NAME,       "ns16550",
                                                    CREATEFIRQ_TAG_CODE, IntHandler,
                                                    CREATEFIRQ_TAG_DATA, ds,
                                                    CREATEFIRQ_TAG_NUM,  ds->ds_FirqNum,
                                                    TAG_END);
                if (ds->ds_Firq >= 0)
                {
                    /* ask to receive a bunch of interrupts */
                    regs->ns_IER = IER_NO_TXFIFO;

                    if (ds->ds_Config.sc_Handshake == SER_HANDSHAKE_SW)
                    {
                        /* on startup, we ain't got no receive buffers */
                        ds->ds_SWHandshakeState = SWHAND_SEND_XOFF;
                        FillOutputFIFO(ds);
                    }

                    SetItemOwner(ds->ds_BreakTimerItem, KB_FIELD(kb_OperatorTask));
                    SetItemOwner(ds->ds_ReadTimerItem,  KB_FIELD(kb_OperatorTask));
                    SetItemOwner(ds->ds_Firq,           KB_FIELD(kb_OperatorTask));

                    /* get the system to see interrupts from the 16550 */
                    EnableInterrupt(ds->ds_FirqNum);

                    DemotePriv(CURRENTTASK, oldPriv);

                    return dev->dev.n_Item;
                }
                DeleteItem(ds->ds_ReadTimerItem);
            }
            DeleteItem(ds->ds_BreakTimerItem);
        }
        DeleteItem(ds->ds_SlotDev);
    }

    DemotePriv(CURRENTTASK, oldPriv);

    return result;
}


/*****************************************************************************/


static Err Delete16550Device(Device *dev)
{
DeviceState      *ds;
volatile NS16550 *regs;

    ds   = (DeviceState *)dev->dev_DriverData;
    regs = ds->ds_16550;

    regs->ns_IER = 0; /* disable interrupts from the 16550       */
    regs->ns_MCR = 0; /* drop DTR, RTS, OUT1, OUT2, and LOOPBACK */
    regs->ns_FCR = 0; /* clear and shutdown the FIFOs            */
    regs->ns_LCR = 0; /* turn off BREAK                          */

    SuperInternalCloseItem(ds->ds_SlotDev, TASK(dev->dev.n_Owner));
    SuperInternalDeleteItem(ds->ds_BreakTimerItem);
    SuperInternalDeleteItem(ds->ds_ReadTimerItem);
    SuperInternalDeleteItem(ds->ds_Firq);
    SuperFreeMem(ds->ds_OvflBuffer, ds->ds_OvflBufferSize);

    return 0;
}


/*****************************************************************************/


static int32 SetOwner16550Device(Device *dev, Item newOwner)
{
DeviceState *ds;
Err          result;

    ds = (DeviceState *)dev->dev_DriverData;

    result = OpenItemAsTask(ds->ds_SlotDev, NULL, newOwner);
    if (result < 0)
        return result;

    return CloseItem(ds->ds_SlotDev);
}


/*****************************************************************************/


static Item Open16550Device(Device *dev)
{
Err    result;
uint32 perms;

    result = QUERY_SYS_INFO(SYSINFO_TAG_DEVICEPERMS, perms);
    if (result < 0)
	return result;

    if ((perms & SYSINFO_DEVICEPERM_SERIAL) == 0)
	return SER_ERR_BADPRIV;

    return dev->dev.n_Item;
}


/*****************************************************************************/


static Item Create16550IO(IOReq *ior)
{
    ior->io_Extension[0] = (uint32)ior->io_Dev->dev_DriverData;
    return ior->io.n_Item;
}


/*****************************************************************************/


static const DriverCmdTable cmdTable[] =
{
    {CMD_STREAMREAD,      CmdStreamRead},
    {CMD_STREAMWRITE,     CmdStreamWrite},
    {CMD_STREAMFLUSH,     CmdStreamFlush},
    {SER_CMD_BREAK,       CmdBreak},
    {SER_CMD_WAITEVENT,   CmdWaitEvent},
    {SER_CMD_SETRTS,      CmdSetRTS},
    {SER_CMD_SETDTR,      CmdSetDTR},
    {SER_CMD_SETLOOPBACK, CmdSetLoopback},
    {SER_CMD_SETCONFIG,   CmdSetConfig},
    {SER_CMD_GETCONFIG,   CmdGetConfig},
    {SER_CMD_STATUS,      CmdStatus}
};

int32 main(void)
{
    return CreateItemVA(MKNODEID(KERNELNODE,DRIVERNODE),
              TAG_ITEM_NAME,                   "ns16550",
              CREATEDRIVER_TAG_CMDTABLE,       cmdTable,
              CREATEDRIVER_TAG_NUMCMDS,        sizeof(cmdTable) / sizeof(cmdTable[0]),
              CREATEDRIVER_TAG_CREATEDEV,      Create16550Device,
              CREATEDRIVER_TAG_DELETEDEV,      Delete16550Device,
              CREATEDRIVER_TAG_CHOWN_DEV,      SetOwner16550Device,
              CREATEDRIVER_TAG_OPENDEV,        Open16550Device,
              CREATEDRIVER_TAG_ABORTIO,        Abort16550IO,
              CREATEDRIVER_TAG_CRIO,           Create16550IO,
              CREATEDRIVER_TAG_DEVICEDATASIZE, sizeof(DeviceState),
              CREATEDRIVER_TAG_MODULE,         FindCurrentModule(),
              TAG_END);
}
