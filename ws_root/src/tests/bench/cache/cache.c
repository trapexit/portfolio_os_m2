/* @(#) cache.c 96/07/24 1.3 */

#include <kernel/types.h>
#include <kernel/time.h>
#include <kernel/super.h>
#include <kernel/task.h>
#include <kernel/cache.h>
#include <kernel/debug.h>
#include <stdio.h>


/*****************************************************************************/


void DirtyTest(RegBlock *rb, TimerTicks *start, TimerTicks *end, void *buffer);
void CleanTest(RegBlock *rb, TimerTicks *start, TimerTicks *end, void *buffer);

static uint8 buffer[4096];


/*****************************************************************************/


static void CacheTest(void)
{
uint32     oldints;
RegBlock   rb;
uint32     micros;
TimerTicks start, end, diff;
TimeVal    tv;

    oldints = Disable();
    DirtyTest(&rb, &start, &end, buffer);
    Enable(oldints);
    SubTimerTicks(&start, &end, &diff);
    ConvertTimerTicksToTimeVal(&diff, &tv);
    micros = (tv.tv_Seconds * 1000000 + tv.tv_Microseconds);
    printf("Dirty Test: %u usecs\n", micros);

    oldints = Disable();
    CleanTest(&rb, &start, &end, buffer);
    Enable(oldints);
    SubTimerTicks(&start, &end, &diff);
    ConvertTimerTicksToTimeVal(&diff, &tv);
    micros = (tv.tv_Seconds * 1000000 + tv.tv_Microseconds);
    printf("Clean Test: %u usecs\n", micros);
}


/*****************************************************************************/


void main(void)
{
    CallBackSuper((CallBackProcPtr)CacheTest, 0, 0, 0);
}
