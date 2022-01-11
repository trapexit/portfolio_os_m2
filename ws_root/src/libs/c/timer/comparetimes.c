/* @(#) comparetimes.c 95/06/20 1.6 */

#include <kernel/types.h>
#include <kernel/time.h>


/*****************************************************************************/


/**
|||	AUTODOC -public -class kernel -group Timer -name CompareTimes
|||	Compares two time values.
|||
|||	  Synopsis
|||
|||	    int32 CompareTimes(const TimeVal *tv1, const TimeVal *tv2);
|||
|||	  Description
|||
|||	    Compares two time values to determine which came first.
|||
|||	  Arguments
|||
|||	    tv1
|||	        The first time value.
|||
|||	    tv2
|||	        The second time value.
|||
|||	  Return Value
|||
|||	    < 0
|||	        if (tv1 < tv2)
|||
|||	    0
|||	        if (tv1 == tv2)
|||
|||	    > 0
|||	        if (tv1 > tv2)
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
|||	    AddTimes(), SubTimes(), TimeLaterThan(), TimeLaterThanOrEqual()
|||
**/

/**
|||	AUTODOC -public -class kernel -group Timer -name TimeLaterThan
|||	Returns whether a time value comes before another.
|||
|||	  Synopsis
|||
|||	    bool TimeLaterThan(const TimeVal *tv1, const TimeVal *tv2);
|||
|||	  Description
|||
|||	    Returns whether tv1 comes chronologically after tv2.
|||
|||	  Arguments
|||
|||	    tv1
|||	        The first time value.
|||
|||	    tv2
|||	        The second time value.
|||
|||	  Return Value
|||
|||	    TRUE
|||	        if tv1 comes after tv2
|||
|||	    FALSE
|||	        if tv1 comes before or is the same as tv2
|||
|||	  Implementation
|||
|||	    Macro implemented in <kernel/time.h> V24.
|||
|||	  Associated Files
|||
|||	    <kernel/time.h>, libc.a
|||
|||	  See Also
|||
|||	    CompareTimes(), TimeLaterThanOrEqual()
|||
**/

/**
|||	AUTODOC -public -class kernel -group Timer -name TimeLaterThanOrEqual
|||	Returns whether a time value comes before or at the same time as
|||	another.
|||
|||	  Synopsis
|||
|||	    bool TimeLaterThanOrEqual(const TimeVal *tv1,const TimeVal *tv2);
|||
|||	  Description
|||
|||	    Returns whether tv1 comes chronologically after tv2, or is the same
|||	    as tv2.
|||
|||	  Arguments
|||
|||	    tv1
|||	        The first time value.
|||
|||	    tv2
|||	        The second time value.
|||
|||	  Return Value
|||
|||	    TRUE
|||	        if tv1 comes after tv2 or is the same as tv2
|||
|||	    FALSE
|||	        if tv1 comes before tv2
|||
|||	  Implementation
|||
|||	    Macro implemented in <kernel/time.h> V24.
|||
|||	  Associated Files
|||
|||	    <kernel/time.h>, libc.a
|||
|||	  See Also
|||
|||	    CompareTimes(), TimeLaterThan()
|||
**/

int32 CompareTimes(const TimeVal *tv1, const TimeVal *tv2)
{
    if (tv1->tv_Seconds < tv2->tv_Seconds)
        return (-1);

    if (tv1->tv_Seconds > tv2->tv_Seconds)
        return (1);

    if (tv1->tv_Microseconds < tv2->tv_Microseconds)
        return (-1);

    if (tv1->tv_Microseconds > tv2->tv_Microseconds)
        return (1);

    return (0);
}
