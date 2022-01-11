/* @(#) timer.c 96/09/10 1.64 */

/* The only HW timer facilities are the processor decrementer and
 * the Time Base registers.  The decrementer is a 32 bit countdown timer
 * which may generate an interrupt.  The Time Base is two 32 bit registers
 * (counting ticks), which cannot generate an interrupt.
 */

#include <kernel/types.h>
#include <kernel/time.h>
#include <kernel/timer.h>
#include <kernel/kernel.h>
#include <kernel/nodes.h>
#include <kernel/list.h>
#include <kernel/listmacros.h>
#include <kernel/item.h>
#include <kernel/operror.h>
#include <kernel/mem.h>
#include <kernel/panic.h>
#include <kernel/sysinfo.h>
#include <kernel/lumberjack.h>
#include <kernel/super.h>
#include <kernel/internalf.h>
#include <device/mp.h>
#include <hardware/PPC.h>
#include <hardware/PPCasm.h>


/*****************************************************************************/


extern int32 cblank(Timer *);
extern void (*SetExceptionVector(void (*)(), uint32)) ();
extern void decrementerHandler(void);


/*****************************************************************************/


List    _waitq;
List    _expiredq;
uint64  _ticksPerSecond;
Timer  *_quantaclock;


/*****************************************************************************/


/**
|||	AUTODOC -class Kernel -group Timer -name ConvertTimeValToTimerTicks
|||	Convert a time value to a hardware dependant form.
|||
|||	  Synopsis
|||
|||	    void ConvertTimeValToTimerTicks(const TimeVal *tv, TimerTicks *tt);
|||
|||	  Description
|||
|||	    This function converts a TimeVal structure to a hardware-dependant
|||	    representation.
|||
|||	    Do not assume *anything* about the value stored in the
|||	    TimerTicks structure. The meaning and interpretation will change
|||	    based on the CPU performance, and possibly other issues. Only use
|||	    the supplied functions to operate on this structure.
|||
|||	  Arguments
|||
|||	    tv
|||	        The time value to convert.
|||
|||	    tt
|||	        A pointer to a TimerTicks structure
|||	        which will receive the converted value.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V27.
|||
|||	  Warning
|||
|||	    Do not make any assumptions about the internal representation of
|||	    the TimerTicks structure. Only use the supplied functions to
|||	    manipulate the structure.
|||
|||	  Associated Files
|||
|||	    <kernel/time.h>, libc.a
|||
|||	  See Also
|||
|||	    ConvertTimerTicksToTimeVal()
|||
**/

void ConvertTimeValToTimerTicks(const TimeVal *tv, TimerTicks *tt)
{
    *(uint64 *)tt = ((uint64)tv->tv_Seconds * _ticksPerSecond)
                 + (((uint64)tv->tv_Microseconds * _ticksPerSecond) / 1000000);
}


/*****************************************************************************/


/**
|||	AUTODOC -class Kernel -group Timer -name ConvertTimerTicksToTimeVal
|||	Convert a hardware dependant timer tick value to a TimeVal.
|||
|||	  Synopsis
|||
|||	    void ConvertTimerTicksToTimeVal(const TimerTicks *tt, TimeVal *tv);
|||
|||	  Description
|||
|||	    This function converts a hardware-dependant representation of
|||	    system time to a TimeVal structure.
|||
|||	    Do not assume *anything* about the value stored in the
|||	    TimerTicks structure. The meaning and interpretation will change
|||	    based on the CPU performance, and possibly other issues. Only use
|||	    the supplied functions to operate on this structure.
|||
|||	  Arguments
|||
|||	    tt
|||	        The timer tick value to convert.
|||
|||	    tv
|||	        A pointer to a TimeVal structure
|||	        which will receive the converted value.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V27.
|||
|||	  Warning
|||
|||	    Do not make any assumptions about the internal representation of
|||	    the TimerTicks structure. Only use the supplied functions to
|||	    manipulate the structure.
|||
|||	  Associated Files
|||
|||	    <kernel/time.h>, libc.a
|||
|||	  See Also
|||
|||	    ConvertTimeValToTimerTicks()
|||
**/

void ConvertTimerTicksToTimeVal(const TimerTicks *tt, TimeVal *tv)
{
uint64 ticks;

    ticks = *(uint64 *)tt;

    tv->tv_Seconds      = ticks / _ticksPerSecond;
    tv->tv_Microseconds = ((ticks % _ticksPerSecond) * 1000000) / _ticksPerSecond;
}


/*****************************************************************************/


/* comparison function for use with UniversalInsertNode() */
static bool CmpWakeTimes(Node *n, Node *m)
{
    if (CompareTimerTicks(&((Timer *)n)->tm_WakeTime,&((Timer *)m)->tm_WakeTime) < 0)
	return TRUE;

    return FALSE;
}


/*****************************************************************************/


/* Load the decrementer with a new countdown value based on the wake time
 * of the given timer node.
 */
static void StartDecrementer(Timer *timer)
{
TimerTicks tt;

    /* Calculate the countdown value for the decrementer by subtracting the
     * current time base from the first node's wake time.  This maintains a
     * self correction for delay times and minimizes time drift.  If the wake
     * time has already expired, SubTimerTicks() will return a zero instead
     * of a negative result, so a decrementer exception will occur immediately.
     */

    SampleSystemTimeTT(&tt);
    SubTimerTicks(&tt,&timer->tm_WakeTime,&tt);  /* tt = waketime - tt */

    /* Load the decrementer with a new countdown value.  Since the decrementer
     * is 32 bits, and the delay time may be a 64 bit value, the decrementer
     * may have to be reloaded several times.  The exception handler manages
     * the reloading of the decrementer.
     */

    if (tt.tt_Hi)
        _mtdec(0x7fffffff);
    else
        _mtdec(tt.tt_Lo);
}


/*****************************************************************************/


/* Process expired timer nodes. This routine is called from the decrementer
 * exception handler, so it runs with interrupts disabled.
 *
 * Examine the timer queue and run any code whose wake time has
 * expired. If the node is flagged as periodic, recalcuate it's
 * wake time and reinserted it into the queue.
 */

void ServiceDecrementer(void)
{
TimerTicks tt;
Timer     *timer;

    if (IsSlaveCPU())
        return;

    LogInterruptStart(0, "Decrementer");

    while (TRUE)
    {
        timer = (Timer *) FirstNode(&_waitq);

        if (!IsNode(&_waitq, (Node *)timer))
        {
            /* q is empty */
            break;
        }

        SampleSystemTimeTT(&tt);

        if (CompareTimerTicks(&tt,&timer->tm_WakeTime) < 0)
        {
            /* No expired waiters, so reset the decrementer to wake us
             * up when the first waiter is ready.
             */
            StartDecrementer(timer);
            break;
        }

        /* remove from wait q */
        REMOVENODE((Node *) timer);

        if (timer->tm_Periodic)
        {
            /* reschedule this one */
            AddTimerTicks(&timer->tm_WakeTime, &timer->tm_Period, &timer->tm_WakeTime);
            UniversalInsertNode(&_waitq, (Node *) timer, CmpWakeTimes);
        }
        else
        {
            /* add to expired q */
            ADDTAIL(&_expiredq,(Node *)timer);
        }

        /* invoke handler */
        (timer->tm_Code)(timer);
    }

    LogInterruptDone();
}


/*****************************************************************************/


/* Insert a node into the timer queue and load the timer with the
 * delay value if it is the first element in the ordered queue.
 */

void LoadTimer(Timer *timer, const TimerTicks *tt)
{
uint32 oldints;

    oldints = Disable();

    /* Read the current time base value.  Using that as the timestamp, add
     * the delay time to get the absolute wake time for this node.
     */

    SampleSystemTimeTT(&timer->tm_WakeTime);
    AddTimerTicks(&timer->tm_WakeTime,tt,&timer->tm_WakeTime);

    REMOVENODE((Node *) timer);
    UniversalInsertNode(&_waitq, (Node *) timer, CmpWakeTimes);

    if (timer == (Timer *) FirstNode(&_waitq))
    {
       /* If we inserted a new head in the timer queue, then we must load the
        * decrementer with the new head's wake time (which will be
        * sooner than the previous head's).
        */
        StartDecrementer(timer);
    }

    Enable(oldints);                       /* restore interrupts       */
}


/*****************************************************************************/


/* Remove a node from the timer queue */
void UnloadTimer(Timer *timer)
{
uint32  oldints;
Timer  *head;

    oldints = Disable();

    head = (Timer *) FirstNode(&_waitq);
    REMOVENODE((Node *)timer);
    ADDTAIL(&_expiredq,(Node *)timer);

    if (timer == head)
    {
       /* If we removed the head of the timer queue, then we must load the
        * decrementer with the new head's wake time
        */
        head = (Timer *)FirstNode(&_waitq);
        if (IsNode(&_waitq,head))
        {
            StartDecrementer(head);
        }
        else
        {
            /* Nobody else waiting, load it with something big so it won't
             * trigger for a long time
             */
            _mtdec(0x7fffffff);
        }
    }

    Enable(oldints);                       /* restore interrupts       */
}


/*****************************************************************************/


static int32 ict_c(Timer *timer, uint32 *flags, uint32 tag, uint32 arg)
{
    TOUCH(flags);

    switch (tag)
    {
	case CREATETIMER_TAG_HNDLR : timer->tm_Code = (int32 (*)(Timer *)) arg;
                                     break;

	case CREATETIMER_TAG_PERIOD: timer->tm_Periodic = TRUE;
				     timer->tm_Period   = *(TimerTicks *)arg;
                                     break;

        case CREATETIMER_TAG_USERDATA: timer->tm_UserData = (void *)arg;
                                       break;

	default                      : return BADTAG;
    }

    return 0;
}


/*****************************************************************************/


Item internalCreateTimer(Timer *timer, TagArg *tags)
{
int32  err;
uint32 oldints;

    if (CURRENTTASK)
    {
        if ((CURRENTTASK->t.n_ItemFlags & ITEMNODE_PRIVILEGED) == 0)
            return BADPRIV;
    }

    err = TagProcessor(timer, tags, ict_c, NULL);
    if (err < 0)
        return (err);

    if (timer->tm_Code == NULL)
        return BADTAGVAL;

    timer->tm_Load   = LoadTimer;
    timer->tm_Unload = UnloadTimer;

    oldints = Disable();
    ADDTAIL(&_expiredq,(Node *)timer);
    Enable(oldints);

    return timer->tm.n_Item;
}


/*****************************************************************************/


int32 internalDeleteTimer(Timer *timer, Task *t)
{
uint32 oldints;

    TOUCH(t);

    oldints = Disable();

    REMOVENODE((Node *) timer);

    if (!IsEmptyList(&_waitq))
        StartDecrementer((Timer *)FirstNode(&_waitq));

    Enable(oldints);

    return 0;
}


/*****************************************************************************/


void SuperSetSystemTimeTT(TimerTicks *new)
{
uint32  oldints;
Timer  *waiter;

    /* This function would really need to be more sophisticated in order
     * to handle clients correctly. As it is, when the time is changed,
     * clients will get incorrect timing results for any outstanding requests.
     *
     * But since we know this is only called upon bootup, it doesn't really
     * matter.
     */

    oldints = Disable();
    _mttbu(new->tt_Hi);
    _mttbl(new->tt_Lo);
    _mtdec(1);           /* trigger a pass through ServiceDecrementer() */

    ScanList(&_waitq, waiter, Timer)
    {
        if (waiter->tm_Periodic)
            waiter->tm_WakeTime = *new;
    }

    Enable(oldints);
}
