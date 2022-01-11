/* @(#) samplesystemtimetv.c 95/06/20 1.3 */

#include <kernel/types.h>
#include <kernel/time.h>

/**
|||	AUTODOC -public -class kernel -group Timer -name SampleSystemTimeTV
|||	Samples the system time with very low overhead.
|||
|||	  Synopsis
|||
|||	    void SampleSystemTimeTV( TimeVal *time )
|||
|||	  Description
|||
|||	    This function records the current system time in the supplied
|||	    TimeVal structure. This is a very low overhead call giving a very
|||	    high-accuracy timing.
|||
|||	    The time value returned by this function corresponds to the time
|||	    maintained by the TIMER_UNIT_USEC unit of the timer device.
|||
|||	    For an even faster routine, see SampleSystemTimeTT(), which
|||	    deals in a hardware-dependant representation of time.
|||
|||	  Arguments
|||
|||	    time
|||	        A pointer to a TimeVal structure which will receive the
|||	        current system time.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libc.a V24.
|||
|||	  Associated Files
|||
|||	    <kernel/time.h>, libc.a
|||
**/

void SampleSystemTimeTV(TimeVal *tv)
{
TimerTicks tt;

    SampleSystemTimeTT(&tt);
    ConvertTimerTicksToTimeVal(&tt,tv);
}
