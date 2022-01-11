/* @(#) timer.s 96/09/10 1.17 */

/* Low level hardware specific timer routines for PowerPC. */

#include <hardware/PPCMacroequ.i>


/*****************************************************************************/


/**
|||	AUTODOC -class Kernel -group Timer -name SampleSystemTimeTT
|||	Samples the system time with very low overhead.
|||
|||	  Synopsis
|||
|||	    void SampleSystemTimeTT( TimerTicks *time )
|||
|||	  Description
|||
|||	    This function records the current system time in a hardware
|||	    dependant format. This is an extremely low overhead call
|||	    giving a very high-accuracy timing.
|||
|||	    You can convert the hardware dependant TimerTicks structure
|||	    to a more abstract form by using the ConvertTimerTicksToTimeVal()
|||	    function.
|||
|||	    Do not assume *anything* about the value stored in the
|||	    TimerTicks structure. The meaning and interpretation will change
|||	    based on the CPU performance, and possibly other issues. Only use
|||	    the supplied functions to operate on this structure.
|||
|||	  Arguments
|||
|||	    time
|||	        A pointer to a TimerTicks structure
|||	        which will receive the current system time.
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

	DECFN	SampleSystemTimeTT
	mftbu	r11			/* read TBU - upper 32 bits	*/
	mftb	r12			/* read TBL - lower 32 bits	*/
	mftbu	r5			/* read TBU again		*/
	cmp	r5,r11			/* see if 'old' == 'new'	*/
	bne-	SampleSystemTimeTT	/* loop if a carry occurred	*/
	stw	r11,0(r3)		/* return time base TBU and TBL	*/
	stw	r12,4(r3)
	blr


/*****************************************************************************/


/**
|||	AUTODOC -class Kernel -group Timer -name AddTimerTicks
|||	Adds two TimerTicks values together.
|||
|||	  Synopsis
|||
|||	    void AddTimerTicks(const TimerTicks *tt1, const TimerTicks *tt2,
|||	                       TimerTicks *result);
|||
|||	  Description
|||
|||	    Adds two hardware-dependant timer ticks values together.
|||
|||	  Arguments
|||
|||	    tt1
|||	        The first time value to add.
|||
|||	    tt2
|||	        The second time value to add.
|||
|||	    result
|||	        A pointer to the location where the
|||	        resulting time value will be stored.
|||	        This pointer can match either tt1 or tt2.
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
|||	    SubTimerTicks(), CompareTimerTicks()
|||
**/

	DECFN	AddTimerTicks
	lwz	r6,4(r3)	/* get sum[32-63]		*/
	lwz	r7,4(r4)	/* get operand[32-63]		*/
	addco.	r6,r6,r7	/* add				*/
	stw	r6,4(r5)	/* store bits [32-63] of sum	*/
	lwz	r6,0(r3)	/* get sum[0-31]		*/
	lwz	r7,0(r4)	/* get operand[0-31]		*/
	adde	r6,r6,r7	/* add with carry		*/
	stw	r6,0(r5)	/* store sum[0-31]		*/
	blr


/*****************************************************************************/


/**
|||	AUTODOC -class Kernel -group Timer -name SubTimerTicks
|||	Subtracts one TimerTicks value from another.
|||
|||	  Synopsis
|||
|||	    void SubTimerTicks(const TimerTicks *tt1, const TimerTicks *tt2,
|||	                       TimerTicks *result);
|||
|||	  Description
|||
|||	    Subtracts one hardware-dependant timer tick value from another.
|||	    The value stored corresponds to (tt2 - tt1). If the subtraction
|||	    would yield a negative time value, a zero timer tick value is
|||	    returned instead.
|||
|||	  Arguments
|||
|||	    tt1
|||	        The first time value.
|||
|||	    tt2
|||	        The second time value.
|||
|||	    result
|||	        A pointer to the location where the
|||	        resulting time value will be stored.
|||	        This pointer can match either of
|||	        tt1 or tt2. The value stored corresponds to (tt2 - tt1).
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
|||	    AddTimerTicks(), CompareTimerTicks()
|||
**/

	DECFN	SubTimerTicks
	lwz	r7,4(r3)	/* get operand 1 [32-63]	*/
	lwz	r8,4(r4)	/* get operand 2 [32-63]	*/
	subfco.	r6,r7,r8	/* sub operand 1 from operand 2	*/
	lwz	r7,0(r3)	/* get operand 1 [0-31]		*/
	lwz	r8,0(r4)	/* get operand 2 [0-31]		*/
	subfe.	r7,r7,r8	/* sub with carry		*/
	bge	0$		/* positive result?		*/
	li	r6,0		/* return zero result if < 0	*/
	mr	r7,r6
0$:	stw	r6,4(r5)	/* store bits [32-63] of sum	*/
	stw	r7,0(r5)	/* store sum[0-31]		*/
	blr


/*****************************************************************************/


/**
|||	AUTODOC -class Kernel -group Timer -name CompareTimerTicks
|||	Compares two timer ticks values.
|||
|||	  Synopsis
|||
|||	    int32 CompareTimerTicks(const TimerTicks *tt1, const TimerTicks *tt2);
|||
|||	  Description
|||
|||	    Compares two hardware-dependant timer ticks values to determine
|||	    which came first.
|||
|||	  Arguments
|||
|||	    tt1
|||	        The first time value.
|||
|||	    tt2
|||	        The second time value.
|||
|||	  Return Value
|||
|||	    < 0
|||	        if (tt1 < tt2)
|||
|||	    0
|||	        if (tt1 == tt2)
|||
|||	    > 0
|||	        if (tt1 > tt2)
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
|||	    AddTimerTicks(), SubTimerTicks(), TimerTicksLaterThan(),
|||	    TimerTicksLaterThanOrEqual()
|||
**/

/**
|||	AUTODOC -class Kernel -group Timer -name TimerTicksLaterThan
|||	Returns whether a time tick value comes before another.
|||
|||	  Synopsis
|||
|||	    bool TimerTicksLaterThan(const TimerTicks *tt1, const TimerTicks *tt2);
|||
|||	  Description
|||
|||	    Returns whether tt1 comes chronologically after tt2.
|||
|||	  Arguments
|||
|||	    tt1
|||	        The first time value.
|||
|||	    tt2
|||	        The second time value.
|||
|||	  Return Value
|||
|||	    TRUE
|||	        if tt1 comes after tv2
|||
|||	    FALSE
|||	        if tt1 comes before or is the same as tt2
|||
|||	  Implementation
|||
|||	    Macro implemented in <kernel/time.h> V27.
|||
|||	  Associated Files
|||
|||	    <kernel/time.h>, libc.a
|||
|||	  See Also
|||
|||	    CompareTimerTicks(), TimerTicksLaterThanOrEqual()
|||
**/

/**
|||	AUTODOC -class Kernel -group Timer -name TimerTicksLaterThanOrEqual
|||	Returns whether a time tick value comes before or at the same time as
|||	another.
|||
|||	  Synopsis
|||
|||	    bool TimerTicksLaterThanOrEqual(const TimerTicks *tt1,
|||	                                    const TimerTicks *tt2);
|||
|||	  Description
|||
|||	    Returns whether tt1 comes chronologically after tv2, or is the same as
|||	    tt2.
|||
|||	  Arguments
|||
|||	    tt1
|||	        The first time value.
|||
|||	    tt2
|||	        The second time value.
|||
|||	  Return Value
|||
|||	    TRUE
|||	        if tt1 comes after tt2 or is the same as tt2
|||
|||	    FALSE
|||	        if tt1 comes before tt2
|||
|||	  Implementation
|||
|||	    Macro implemented in <kernel/time.h> V27.
|||
|||	  Associated Files
|||
|||	    <kernel/time.h>, libc.a
|||
|||	  See Also
|||
|||	    CompareTimerTicks(), TimerTicksLaterThan()
|||
**/

	DECFN	CompareTimerTicks
	lwz	r5,0(r3)	/* get high order words	*/
	lwz	r6,0(r4)
	lwz	r7,4(r3)	/* get low order words	*/
	lwz	r8,4(r4)
	cmpl	0,r5,r6		/* compare high words	*/
	blt	0$
	bgt	1$
	cmpl	0,r7,r8		/* high words equal - compare low words */
	blt	0$
	bgt	1$
	li	r3,0		/* operands are equal	*/
	blr
0$:
	li	r3,-1		/* less than		*/
	blr
1$:
	li	r3,1		/* greater than		*/
	blr
