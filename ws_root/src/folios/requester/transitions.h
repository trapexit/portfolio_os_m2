/* @(#) transitions.h 96/09/07 1.2 */

#ifndef __TRANSITIONS_H
#define __TRANSITIONS_H


/*****************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_NODES_H
#include <kernel/nodes.h>
#endif

#ifndef __KERNEL_TIME_H
#include <kernel/time.h>
#endif


/*****************************************************************************/


typedef enum
{
    TT_NONE,
    TT_LINEAR,
    TT_ACCELERATED,
    TT_LOOPING,
    TT_PINGPONG
} TransitionTypes;

typedef struct
{
    MinNode         ti;
    TransitionTypes ti_Type;
    int16           ti_Start;
    int16           ti_End;
    int16           ti_Current;
    int16           ti_Speed;        /* increments/millisecond */
    TimeVal         ti_StartTime;
} TransitionInfo;

#define ti_Acceleration ti_Speed


/*****************************************************************************/


void PrepTransition(TransitionInfo *ti, TransitionTypes tt,
                    int16 start, int16 end, int16 speed,
                    const TimeVal *startTime);
void UnprepTransition(TransitionInfo *ti);
void StepTransition(TransitionInfo *ti, const TimeVal *currentTime);


/*****************************************************************************/


#endif /* __TRANSITIONS_H */
