/* @(#) addtimes.c 95/06/20 1.6 */

#include <kernel/types.h>
#include <kernel/time.h>


/*****************************************************************************/


/**
|||	AUTODOC -public -class kernel -group Timer -name AddTimes
|||	Adds two time values together.
|||
|||	  Synopsis
|||
|||	    void AddTimes(const TimeVal *tv1, const TimeVal *tv2,
|||	                  TimeVal *result);
|||
|||	  Description
|||
|||	    Adds two time values together, yielding the total time for both.
|||
|||	  Arguments
|||
|||	    tv1
|||	        The first time value to add.
|||
|||	    tv2
|||	        The second time value to add.
|||
|||	    result
|||	        A pointer to the location where the resulting time value will
|||	        be stored. This pointer can match either tv1 or tv2.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libc.a V24.
|||
|||	  Associated Files
|||
|||	    <kernel/time.h>, libc.a
|||
|||	  See Also
|||
|||	    SubTimes(), CompareTimes()
|||
**/

void AddTimes(const TimeVal *tv1, const TimeVal *tv2, TimeVal *result)
{
uint32 secs;
uint32 micros;

    secs   = tv1->tv_Seconds + tv2->tv_Seconds;
    micros = tv1->tv_Microseconds + tv2->tv_Microseconds;

    if (micros >= 1000000)
    {
	secs++;
	micros -= 1000000;
    }

    result->tv_Seconds      = secs;
    result->tv_Microseconds = micros;
}
