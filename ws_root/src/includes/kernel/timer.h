#ifndef __KERNEL_TIMER_H
#define __KERNEL_TIMER_H


/******************************************************************************
**
**  @(#) timer.h 96/02/20 1.32
**
******************************************************************************/


#ifdef EXTERNAL_RELEASE
#error "This file may not be used in externally released source code or link lib"
#endif

#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_ITEM_H
#include <kernel/item.h>
#endif

#ifndef __KERNEL_NODES_H
#include <kernel/nodes.h>
#endif

#ifndef __KERNEL_TIME_H
#include <kernel/time.h>
#endif

#ifndef __KERNEL_KERNEL_H
#include <kernel/kernel.h>
#endif

#ifndef __HARDWARE_PPC_H
#include <hardware/PPC.h>
#endif


/*****************************************************************************/


/* for controlling the hardware timers */
typedef struct Timer
{
    ItemNode    tm;
    TimerTicks  tm_WakeTime;
    int32     (*tm_Code)(struct Timer *);
    void       *tm_UserData;

    void      (*tm_Load)(struct Timer *,const TimerTicks *);
    void      (*tm_Unload)(struct Timer *);

    TimerTicks  tm_Period;
    bool        tm_Periodic;
} Timer;

enum timer_tags
{
    CREATETIMER_TAG_HNDLR = TAG_ITEM_LAST+1, /* handler addr  */
    CREATETIMER_TAG_PERIOD,                  /* a periodic (repeating) timer */
    CREATETIMER_TAG_USERDATA                 /* value for tm_UserData */
};


/*****************************************************************************/


/* kernel private */
void LoadTimer(Timer *timer, const TimerTicks *tt);
void UnloadTimer(Timer *timer);
void LoadDecrementer(uint32 decValue);


/*****************************************************************************/


#endif	/* __KERNEL_TIMER_H */
