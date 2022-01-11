/* @(#) subtimes.c 95/06/20 1.6 */

#include <kernel/types.h>
#include <kernel/time.h>


/*****************************************************************************/


/**
|||	AUTODOC -public -class kernel -group Timer -name SubTimes
|||	Subtracts one time value from another.
|||
|||	  Synopsis
|||
|||	    void SubTimes(const TimeVal *tv1, const TimeVal *tv2,TimeVal *result);
|||
|||	  Description
|||
|||	    Subtracts two time values, yielding the difference in time between
|||	    the two.
|||
|||	  Arguments
|||
|||	    tv1
|||	        The first time value.
|||
|||	    tv2
|||	        The second time value.
|||
|||	    result
|||	        A pointer to the location where the resulting time value will
|||	        be stored. This pointer can match either of tv1 or tv2. The
|||	        value stored corresponds to (tv2 - tv1).
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
|||	    AddTimes(), CompareTimes()
|||
**/

void SubTimes(const TimeVal *tv1, const TimeVal *tv2, TimeVal *result)
{
    if (tv2->tv_Microseconds >= tv1->tv_Microseconds)
    {
        result->tv_Seconds      = tv2->tv_Seconds - tv1->tv_Seconds;
        result->tv_Microseconds = tv2->tv_Microseconds - tv1->tv_Microseconds;
    }
    else
    {
        result->tv_Seconds      = tv2->tv_Seconds - tv1->tv_Seconds - 1;
        result->tv_Microseconds = 1000000 - (tv1->tv_Microseconds - tv2->tv_Microseconds);
    }
}
