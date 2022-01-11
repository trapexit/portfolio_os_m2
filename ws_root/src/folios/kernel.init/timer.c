/* @(#) timer.c 96/08/16 1.5 */

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
#include <hardware/PPC.h>
#include <hardware/PPCasm.h>


/*****************************************************************************/


extern int32 _cblank(Timer *);


/*****************************************************************************/

extern Timer       *_quantaclock;

extern List  _waitq;
extern List  _expiredq;
extern uint64 _ticksPerSecond;


/*****************************************************************************/


static const TagArg TaskQuantaTags[] =
{
    CREATETIMER_TAG_HNDLR, (void *) _cblank,
    TAG_ITEM_NAME,         "Quantum Timeout",
    TAG_END
};


/* get the timer subsystem setup and ready for action */
void start_timers(void)
{
SystemInfo si;

    SuperQuerySysInfo(SYSINFO_TAG_SYSTEM, (void *)&si, sizeof(si));
    KB_FIELD(kb_BusClk) = si.si_BusClkSpeed;
    _ticksPerSecond      = si.si_TicksPerSec;

    /* Setup the timer queue, which is serviced by the decrementer exception
     * handler.
     */
    PrepList(&_waitq);
    PrepList(&_expiredq);

    _quantaclock = (Timer *)SuperAllocNode((Folio *) KernelBase,TIMERNODE);
    if (_quantaclock)
    {
        if (SuperInternalCreateTimer(_quantaclock,TaskQuantaTags) >= 0)
        {
            return;
        }
    }

    PANIC(ER_Kr_CantCreate);
}
