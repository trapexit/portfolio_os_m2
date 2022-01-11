#ifndef __KERNEL_TIME_H
#define __KERNEL_TIME_H


/******************************************************************************
**
**  @(#) time.h 95/11/26 1.15
**
**  Kernel time management definitions
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_DEVICE_H
#include <kernel/device.h>
#endif


/*****************************************************************************/


/* For use for microsecond timing */
typedef struct timeval
{
    int32 tv_sec;         /* seconds          */
    int32 tv_usec;        /* and microseconds */
} TimeVal;

/* synonyms */
#define tv_Seconds      tv_sec
#define tv_Microseconds tv_usec


/* For use for vblank timing */
#ifndef NO_64_BIT_SCALARS
typedef uint64 TimeValVBL;
#else
typedef struct TimeValVBL
{
    uint32 tv_Hi;
    uint32 tv_Lo;
} TimeValVBL;
#endif


/* TimerTicks are used for very high accuracy timing. They provide a
 * hardware dependant representation of time. You can use timer ticks
 * to sample the current system time with almost no overhead. Once you
 * have some samples to print, you can convert from TimerTicks to
 * TimeVal structures, in order to get real-world time values.
 *
 * WARNING: Do not assume *anything* about the value stored in this structure.
 *          The meaning and interpretation will change based on the CPU
 *          performance, and possibly other issues. Only use the supplied
 *          functions to operate on this structure.
 */
typedef struct TimerTicks
{
    uint32 tt_Hi;
    uint32 tt_Lo;
} TimerTicks;


/*****************************************************************************/


#ifdef __cplusplus
extern "C" {
#endif


/* timer device convenience routines */
Item CreateTimerIOReq(void);
Err DeleteTimerIOReq(Item ioreq);

/* microsecond timer unit utilities */
Err WaitTime(Item ioreq, uint32 seconds, uint32 micros);
Err WaitUntil(Item ioreq, uint32 seconds, uint32 micros);
Err StartMetronome(Item ioreq, uint32 seconds, uint32 micros, int32 signal);
Err StopMetronome(Item ioreq);

/* VBL timer unit utilities */
Err WaitTimeVBL(Item ioreq, uint32 fields);
Err WaitUntilVBL(Item ioreq, uint32 fields);
Err StartMetronomeVBL(Item ioreq, uint32 fields, int32 signal);
Err StopMetronomeVBL(Item ioreq);

/* TimeVal routines */
void SampleSystemTimeTV(TimeVal *tv);
void AddTimes(const TimeVal *tv1, const TimeVal *tv2, TimeVal *result);
void SubTimes(const TimeVal *tv1, const TimeVal *tv2, TimeVal *result);
int32 CompareTimes(const TimeVal *tv1, const TimeVal *tv2);
void ConvertTimeValToTimerTicks(const TimeVal *tv, TimerTicks *tt);

/* TimeValVBL routines */
void SampleSystemTimeVBL(TimeValVBL *tv);

/* TimerTicks routines */
void SampleSystemTimeTT(TimerTicks *tt);
void AddTimerTicks(const TimerTicks *tt1, const TimerTicks *tt2, TimerTicks *result);
void SubTimerTicks(const TimerTicks *tt1, const TimerTicks *tt2, TimerTicks *result);
int32 CompareTimerTicks(const TimerTicks *tt1, const TimerTicks *tt2);
void ConvertTimerTicksToTimeVal(const TimerTicks *tt, TimeVal *tv);


#ifdef __cplusplus
}
#endif


/*****************************************************************************/


#define TimeLaterThan(t1,t2)        (CompareTimes((t1),(t2)) > 0)
#define TimeLaterThanOrEqual(t1,t2) (CompareTimes((t1),(t2)) >= 0)

#define TimerTicksLaterThan(t1,t2)        (CompareTimerTicks((t1),(t2)) > 0)
#define TimerTicksLaterThanOrEqual(t1,t2) (CompareTimerTicks(t1),(t2)) >= 0)


/*****************************************************************************/


#ifdef __DCC__
#pragma pure_function   CompareTimes, CompareTimerTicks
#pragma no_side_effects AddTimes(3), SubTimes(3), SampleSystemTimeTV(1)
#pragma no_side_effects AddTimerTicks(3), SubTimerTicks(3), SampleSystemTimeTT(1)
#pragma no_side_effects SampleSystemTimeVBL(1)
#endif


/*****************************************************************************/


#endif /* __KERNEL_TIME_H */
