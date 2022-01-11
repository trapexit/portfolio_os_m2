/* @(#) transitions.c 96/09/26 1.3 */

#include <kernel/types.h>
#include <kernel/time.h>
#include "transitions.h"


/*****************************************************************************/


void PrepTransition(TransitionInfo *ti, TransitionTypes tt,
                    int16 start, int16 end, int16 speed,
                    const TimeVal *startTime)
{
    ti->ti_Type    = tt;
    ti->ti_Start   = start;
    ti->ti_End     = end;
    ti->ti_Current = start;
    ti->ti_Speed   = speed;

    if (startTime)
    {
        ti->ti_StartTime = *startTime;
    }
    else
    {
        ti->ti_StartTime.tv_Seconds      = 0;
        ti->ti_StartTime.tv_Microseconds = 0;
    }
}


/*****************************************************************************/


void UnprepTransition(TransitionInfo *ti)
{
    ti->ti_Type = TT_NONE;
}


/*****************************************************************************/


void StepTransition(TransitionInfo *ti, const TimeVal *currentTime)
{
TimeVal delta;
uint32  milli;
int32   current;

    SubTimes(&ti->ti_StartTime, currentTime, &delta);
    milli = (delta.tv_Seconds * 1000) + (delta.tv_Microseconds / 1000);

    if (ti->ti_Start < ti->ti_End)
        current = ti->ti_Start + ((ti->ti_Speed * milli) / 1000);
    else
        current = ti->ti_Start - ((ti->ti_Speed * milli) / 1000);

    switch (ti->ti_Type)
    {
        case TT_NONE       : break;

        case TT_LOOPING    : if (ti->ti_Start < ti->ti_End)
                                 ti->ti_Current = (current - ti->ti_Start) % (ti->ti_End - ti->ti_Start) + ti->ti_Start;
                             else
                                 ti->ti_Current = (current - ti->ti_End) % (ti->ti_Start - ti->ti_End) + ti->ti_End;
                             break;

        case TT_PINGPONG   : if (ti->ti_Start < ti->ti_End)
                             {
                                 ti->ti_Current = (current - ti->ti_Start) % (ti->ti_End - ti->ti_Start) + ti->ti_Start;
                                 if ((current / (ti->ti_End - ti->ti_Start)) & 1)
                                     ti->ti_Current = (ti->ti_End - ti->ti_Start) - ti->ti_Current;
                             }
                             else
                             {
                                 ti->ti_Current = (current - ti->ti_End) % (ti->ti_Start - ti->ti_End) + ti->ti_End;

                                 if ((current / (ti->ti_Start - ti->ti_End)) & 1)
                                     ti->ti_Current = (ti->ti_Start - ti->ti_End) - ti->ti_Current;
                             }
                             break;

        case TT_LINEAR     : if (ti->ti_Current == ti->ti_End)
                                 return;

                             ti->ti_Current = current;
                             if (ti->ti_Start < ti->ti_End)
                             {
                                 if (ti->ti_Current > ti->ti_End)
                                 {
                                     ti->ti_Current = ti->ti_End;
                                     ti->ti_Type    = TT_NONE;
                                 }
                             }
                             else
                             {
                                 if (ti->ti_Current < ti->ti_End)
                                 {
                                     ti->ti_Current = ti->ti_End;
                                     ti->ti_Type    = TT_NONE;
                                 }
                             }
                             break;

        case TT_ACCELERATED: if (ti->ti_Start < ti->ti_End)
                                 current = ti->ti_Start + (((ti->ti_Acceleration * milli*milli) / 2) / 1000);
                             else
                                 current = ti->ti_Start - (((ti->ti_Acceleration * milli*milli) / 2) / 1000);

                             ti->ti_Current = current;
                             if (ti->ti_Start < ti->ti_End)
                             {
                                 if (ti->ti_Current > ti->ti_End)
                                 {
                                     ti->ti_Current = ti->ti_End;
                                     ti->ti_Type    = TT_NONE;
                                 }
                             }
                             else
                             {
                                 if (ti->ti_Current < ti->ti_End)
                                 {
                                     ti->ti_Current = ti->ti_End;
                                     ti->ti_Type    = TT_NONE;
                                 }
                             }
                             break;
    }
}
