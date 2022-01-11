/* @(#) audio_timer.c 96/09/04 1.69 */
/* $Id: audio_timer.c,v 1.66 1995/03/14 06:40:54 phil Exp phil $ */
/****************************************************************
**
** Timer for AudioFolio
**
** Includes a generalized Event Buffer that performs a function at a given time.
** It is a simple sorted list of events.
**
** Event types include:
**    AudioCue Items which can send signal to waiting task.
**    EnvelopeService events which maintain envelope real time progress.
**
** Multiple Independent Audio Clocks
**
** The folio support multiple clocks running at different rates.  In order
** to schedule events using these clocks, we must use a common time base
** which is the audio frame clock.  This clock wraps every 26 minutes so
** events that are more than 13 minutes in the future must have special
** handling.
**
** We will maintain two lists.  One list is for events less than ~5 minutes
** into the future.  They will be stamped with a frame count.
** Most events will go into that list. Other events for the far future will
** be stamped with the equivalent global audio clock time.  Since the lists
** are updated at least every second, the events will always end up
** on the fast list before executing.
**
** Posting an event.
**
** Determine whether time is <MAX_FRAMES in the future.
** If so, port to fast list sorted by frame time.
** If not, post to slow list sorted by global time.
**
** Handling an event interrupt.
**
** Pull events off fast list that are current.
** Pull events off slow list that are <MAX_FRAMES and post
** to fast list.
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
****************************************************************/
/*
** 930211 PLB Fixed crash upon delete. Set InList after UniversalInsertNode
** 930708 PLB Use circular comparison for times for when clock wraparounds.
** 930830 PLB Check for too long a Duration.
** 940126 PLB Use actual ticks elapsed so that lost interrupts don't matter.
** 940413 PLB Fixed bad list walker in HandleTimerSignal()
** 940601 WJB Fixed warnings.
** 940608 WJB Replaced LaterThan...() macros with AudioTimeLaterThan...() macros.
** 940608 WJB Added autodocs for AudioTime macros.
** 940802 WJB Added support for demand loading.
** 940809 WJB Tweaked autodoc prefix.
** 940811 WJB Fixed AllocSignal() usage to correctly detect a 0 result.
** 941116 PLB Check for SIGF_ABORT signal.
** 941209 PLB Added support for DuckAndCover.
** 950131 WJB Fixed includes.
** 950217 PLB Moved interrupt stuff to dspp_irq_anvil.c
** 950512 WJB Added af_TriggerSignal support - call HandleTriggerSignal().
** 950612 WJB Call to DSPP_InitIMemAccess() is now Opera-only.
** 950615 PLB Debugged on real BDA.
** 950627 PLB fixed bug involving multiple Cue, call SetWakeUpTime
**            from ScheduleNextAudioEvent
** 950725 WJB Retired SleepAudioTicks().
** 950810 PLB Call InvalidateFPState() before WaitSignal() for faster task switch
** 951121 PLB Implement multiple audio clocks with different rates.
** 960131 PLB Reenabled DuckAndCover
** 960409 PLB Add 0.5 to default duration calculation.
** 960528 WJB Moved af_GlobalClock and af_EnvelopeClock initialization to CreateSystemClocks() in audio_folio.c.
** 960531 WJB Moved RunAudioSignalTask() to audio_folio.c
** 960603 PLB Activate ResyncAudioCLock() code that was in DBUG statement.
** 960904 PLB Set first wakeup in near future to enable timer interrupt. CR6178 & CR10931
*****************************************************************/

#include <dspptouch/dspp_touch.h>
#include <kernel/kernel.h>

#include "audio_folio_modes.h"
#include "audio_internal.h"

/* Macros for debugging. */

#define DEBUG_Event     0
#define DEBUG_Interrupt 0

#define DBUG(x)  /* PRT(x) */
#define DBUGX(x)  /* PRT(x) */

#if DEBUG_Event
#define DBUGEVENT(x)    PRT(x)
#else
#define DBUGEVENT(x)    /* PRT(x) */
#endif

#if DEBUG_Interrupt
#define DBUGINT(x)      PRT(x)
#else
#define DBUGINT(x)      /* PRT(x) */
#endif

#define MIN_DSP_DURATION (0x0001)
#define MAX_DSP_DURATION (0x7FFF)
/* Max_positive_integer/2 */
#define MAX_FUTURE_FRAMES  (0x3FFFFFFF)

/* SYNC_INTERVAL could actually be a really big number but if there was a bug
** in the resync code then it may not show up until after six hours
** of continuous game play!
*/
#define SYNC_INTERVAL    (44100*10)

/* Prototypes. ******************************************************/
static bool  MatchTimerNode( Node *NewNode, Node *ListNode);
static uint32 ConvertClockTimeToFrameTime( AudioClock *aclk, AudioTime time );
static void  PostEventToFastList( AudioEvent *aevt );
static void  PostEventToSlowList( AudioEvent *aevt, AudioTime localTime );
static void  ScheduleNextAudioTimerSignal( void );
static void  MoveTimerEventsFromSlowToFast( void );
static void  TriggerAllClockEvents( List *list, AudioClock *aclk );

/* -------------------- AudioTime macro autodocs */

 /**
 |||	AUTODOC -public -class audio -group Timer -name CompareAudioTimes
 |||	Compare two AudioTime values with wraparound.
 |||
 |||	  Synopsis
 |||
 |||	    int32 CompareAudioTimes (AudioTime t1, AudioTime t2)
 |||
 |||	  Description
 |||
 |||	    CompareAudioTimes() compares two AudioTime values taking into account
 |||	    wraparound. The two time values are assumed to be within 0x7fffffff ticks
 |||	    of each other. Time differences larger than this value will produce
 |||	    incorrect comparisons.
 |||
 |||	  Arguments
 |||
 |||	    t1
 |||	        First AudioTime value to compare.
 |||
 |||	    t2
 |||	        Second AudioTime value to compare.
 |||
 |||	  Return Value
 |||
 |||	    > 0
 |||	        If t1 is later than t2.
 |||
 |||	    0
 |||	        If t1 equals t2.
 |||
 |||	    < 0
 |||	        If t1 is earlier than t2.
 |||
 |||	  Implementation
 |||
 |||	    Macro implemented in <audio/audio.h> V22.
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>
 |||
 |||	  See Also
 |||
 |||	    AudioTimeLaterThan(), AudioTimeLaterThanOrEqual()
 **/

 /**
 |||	AUTODOC -public -class audio -group Timer -name AudioTimeLaterThan
 |||	Compare two AudioTime values with wraparound.
 |||
 |||	  Synopsis
 |||
 |||	    int AudioTimeLaterThan (AudioTime t1, AudioTime t2)
 |||
 |||	  Description
 |||
 |||	    AudioTimeLaterThan() compares two AudioTime values, taking into account
 |||	    wraparound.
 |||
 |||	    The two time values are assumed to be within 0x7fffffff ticks of each other.
 |||	    The time differences larger than this number will produce incorrect
 |||	    comparisons.
 |||
 |||	  Arguments
 |||
 |||	    t1
 |||	        First AudioTime value to compare.
 |||
 |||	    t2
 |||	        Second AudioTime value to compare.
 |||
 |||	  Return Value
 |||
 |||	    TRUE
 |||	        If t1 is later than t2.
 |||
 |||	    FALSE
 |||	        If t1 is earlier than or equal to t2.
 |||
 |||	  Implementation
 |||
 |||	    Macro implemented in <audio/audio.h> V22.
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>
 |||
 |||	  See Also
 |||
 |||	    AudioTimeLaterThanOrEqual(), CompareAudioTimes()
 **/

 /**
 |||	AUTODOC -public -class audio -group Timer -name AudioTimeLaterThanOrEqual
 |||	Compare two AudioTime values with wraparound.
 |||
 |||	  Synopsis
 |||
 |||	    int AudioTimeLaterThanOrEqual (AudioTime t1, AudioTime t2)
 |||
 |||	  Description
 |||
 |||	    AudioTimeLaterThanOrEqual() compares two AudioTime values, taking into
 |||	    account wraparound.
 |||
 |||	    The two time values are assumed to be within 0x7fffffff ticks of each other.
 |||	    Time differences larger than that will produce incorrect comparisons.
 |||
 |||	  Arguments
 |||
 |||	    t1
 |||	        First AudioTime value to compare.
 |||
 |||	    t2
 |||	        Second AudioTime value to compare.
 |||
 |||	  Return Value
 |||
 |||	    TRUE
 |||	        If t1 is later than or equal to t2.
 |||
 |||	    FALSE
 |||	        If t1 is earlier than t2.
 |||
 |||	  Implementation
 |||
 |||	    Macro implemented in <audio/audio.h> V22.
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>
 |||
 |||	  See Also
 |||
 |||	    AudioTimeLaterThan(), CompareAudioTimes()
 **/


/* -------------------- Code */

/***************************************************************/
/*
	Initializes Audio Timer system. Called by InitAudioFolio()
	(which is part of the audio daemon).

	Notes
		. Expects caller to clean up.
		. The signal allocated by this function is never explicitly
		  freed, because task deletion automatically takes care of
		  that.
*/
Err InitAudioTimer (void)
{
		/* Init list for holding timer requests. */
	PrepList( &AB_FIELD(af_FastTimerList) );
	PrepList( &AB_FIELD(af_SlowTimerList) );

		/* Allocate signal for timer interrupt to signal audio daemon if it is NextTime. */
	{
		const int32 sig = AllocSignal(0);

		if (sig <= 0) return sig ? sig : AF_ERR_NOSIGNAL;
		AB_FIELD(af_TimerSignal) = sig;
	}

/* Set time to next resync clocks to avoid wraparound problem. */
	AB_FIELD(af_NextSyncFrame) = dsppGetCurrentFrameCount() + SYNC_INTERVAL;

/* Set first wakeup in near future to enable timer interrupt.
** Fixes CR6178 & CR10931 */
	dsppSetWakeupFrame( dsppGetCurrentFrameCount() + 1000 );
	return 0;
}

/*****************************************************************/
static bool MatchTimerNode( Node *NewNode, Node *ListNode)
{
	AudioEvent *ln, *nn;

	ln = (AudioEvent *) ListNode;
	nn = (AudioEvent *) NewNode;
	return (AudioTimeLaterThanOrEqual(ln->aevt_ListTime,nn->aevt_ListTime));
}

#define ConvertFramesToClockTicks(clock,numFrames) ((numFrames)/(clock)->aclk_Duration)
/*****************************************************************/
AudioTime GetAudioClockTime( AudioClock *aclk )
{
	uint32 NewFrames;
	int32  ElapsedTicks;
	int32  ElapsedFrames;

	NewFrames = dsppGetCurrentFrameCount();

/* Calculate elapsed frames since last calculation. */
	ElapsedFrames = NewFrames - aclk->aclk_OldFrames;

/* Save ourselves a divide. */
	if( ElapsedFrames == 0) return aclk->aclk_Time;

/* Include remainder frames to avoid roundoff error. */
	ElapsedFrames += aclk->aclk_RemainderFrames;

/* Advance official audio time. */
	ElapsedTicks = ConvertFramesToClockTicks(aclk,ElapsedFrames);
	aclk->aclk_RemainderFrames = ElapsedFrames - (ElapsedTicks * aclk->aclk_Duration);
	aclk->aclk_Time += ElapsedTicks;

/* Save for next time. */
	aclk->aclk_OldFrames = NewFrames;

	return aclk->aclk_Time;
}

/*****************************************************************/
static uint32 ConvertClockTimeToFrameTime( AudioClock *aclk, AudioTime time )
{
	int32  ElapsedFrames;
	int32  ElapsedTicks;

	ElapsedTicks = time - aclk->aclk_Time;
	ElapsedFrames = (ElapsedTicks * aclk->aclk_Duration) - aclk->aclk_RemainderFrames;
	return ElapsedFrames + aclk->aclk_OldFrames;
}

/*****************************************************************/
static void PostEventToFastList( AudioEvent *aevt )
{
	AudioEvent *nextEvt;

/* Get Frame Count for fast queue. */
	aevt->aevt_ListTime = ConvertClockTimeToFrameTime( aevt->aevt_Clock, aevt->aevt_TriggerAt );

/* If list is empty, or new event is earliest event, set wakeup time in hardware. */
	nextEvt = (AudioEvent *) FIRSTNODE( &AB_FIELD(af_FastTimerList) );
	if ( (!ISNODE(&AB_FIELD(af_FastTimerList), nextEvt)) ||
	      AudioTimeLaterThan(nextEvt->aevt_ListTime,aevt->aevt_ListTime) )
	{
DBUGX(("PostEventToFastList: wakeup 0x%x at frame 0x%x\n", aevt, aevt->aevt_ListTime ));
		dsppSetWakeupFrame(aevt->aevt_ListTime);
	}
/*
** Insert request into timer chain in sorted order.
** Since the timer interrupt only sends a signal to the folio, we don't have to
** worry so much about race conditions.  The list won't be traversed by
** any other task.
*/
DBUGEVENT(("PostEventToFastList: insert node 0x%x at frame 0x%x\n", aevt, aevt->aevt_ListTime ));
	UniversalInsertNode(&AB_FIELD(af_FastTimerList), (Node *) aevt, MatchTimerNode);
	aevt->aevt_InList = &AB_FIELD(af_FastTimerList);
}

/*****************************************************************/
static void PostEventToSlowList( AudioEvent *aevt, AudioTime localTime )
{
	int32 localTicksDelta, globalTicksDelta;

/* Set list time for sorting.*/
/* Get Global Time for slow queue by scaling delta ticks. */
	localTicksDelta = aevt->aevt_TriggerAt - localTime;

/* The loss of precision caused by the divide is not important because we are doing
** a rough sort into the slow list. The important thing is that we do not overflow
** intermediate results. */
	globalTicksDelta = (localTicksDelta / AB_FIELD(af_GlobalClock)->aclk_Duration) * aevt->aevt_Clock->aclk_Duration;
	aevt->aevt_ListTime = GetAudioClockTime( AB_FIELD(af_GlobalClock) ) + globalTicksDelta;

/*
** Insert request into timer chain in sorted order.
** Since the timer interrupt only sends a signal to the folio, we don't have to
** worry so much about race conditions.  The list won't be traversed by
** any other task.
*/
DBUGEVENT(("PostEventToSlowList: insert node 0x%x at time 0x%x\n", aevt, aevt->aevt_ListTime ));
	UniversalInsertNode(&AB_FIELD(af_SlowTimerList), (Node *) aevt, MatchTimerNode);
	aevt->aevt_InList = &AB_FIELD(af_SlowTimerList);
}

/*****************************************************************/
int32 PostAudioEvent( AudioEvent *aevt, AudioTime WakeupTime )
{
	int32 MaxTicks;
	uint32 CurrentTime;

/* Check to see if event in use. */
	if (aevt->aevt_InList != NULL)
	{
		ERR(("PostAudioEvent: Event in use!\n"));
		return AF_ERR_INUSE;
	}

/* Set trigger time. */
	aevt->aevt_TriggerAt = WakeupTime;

/* Calculate CriticalTime in time of local clock. */
	MaxTicks = ConvertFramesToClockTicks( aevt->aevt_Clock, MAX_FUTURE_FRAMES );
	CurrentTime = GetAudioClockTime( aevt->aevt_Clock );

/* Is this event less than critical time in future? */
	if( ((int32)(WakeupTime - CurrentTime)) < MaxTicks )
	{
		PostEventToFastList( aevt );
	}
	else
	{
		PostEventToSlowList( aevt, CurrentTime );
	}

	return 0;
}

/*****************************************************************/
int32 UnpostAudioEvent( AudioEvent *aevt )
{

/* Check to see if in use. */
	if (aevt->aevt_InList != NULL)
	{
		ParanoidRemNode( (Node *) aevt );   /* Remove from timer chain. */
		aevt->aevt_InList = NULL;
	}
	return 0;
}

/*****************************************************************/
/* Set timer to signal on next matching time. */
static void ScheduleNextAudioTimerSignal( void )
{
	AudioEvent *aevt;

/* Tell interrupt when to interrupt next. */
	aevt = (AudioEvent *) FIRSTNODE( &AB_FIELD(af_FastTimerList) );
	if ( ISNODE(&AB_FIELD(af_FastTimerList), aevt) )
	{
DBUGX(("ScheduleNextAudioTimerSignal: event = 0x%x, next frame =  0x%x\n",
		aevt, aevt->aevt_ListTime));
		dsppSetWakeupFrame(aevt->aevt_ListTime);
	}
}

/*****************************************************************/
/**
 |||	AUTODOC -public -class Items -group Audio -name AudioClock
 |||	Audio virtual timer item.
 |||
 |||	  Description
 |||
 |||	    An AudioClock is a virtual timer that runs at a rate independent of other
 |||	    AudioClock. All AudioClocks are derived from the Codec Frame Clock which
 |||	    typically runs at 44100 Hz. An arbitrary number of independent AudioClocks
 |||	    may be created.
 |||
 |||	    Most AudioClock functions accept an AudioClock Item or the constant
 |||	    AF_GLOBAL_CLOCK, which refers to a system-wide clock. This clock runs at
 |||	    a constant rate of approximately, but not exactly, 240 Hz.
 |||
 |||	  Folio
 |||
 |||	    audio
 |||
 |||	  Item Type
 |||
 |||	    AUDIO_CLOCK_NODE
 |||
 |||	  Create
 |||
 |||	    CreateAudioClock()
 |||
 |||	  Delete
 |||
 |||	    DeleteAudioClock()
 |||
 |||	  Use
 |||
 |||	    SignalAtAudioTime(), AbortTimerCue(), SetAudioClockDuration(),
 |||	    SetAudioClockRate(), ReadAudioClock()
 |||
 |||	  See Also
 |||
 |||	    Cue(@)
 **/

/*****************************************************************/
 /**
 |||	AUTODOC -public -class audio -group Timer -name SignalAtTime
 |||	Requests a wake-up call at a given time from AF_GLOBAL_CLOCK.
 |||
 |||	  Synopsis
 |||
 |||	    Err SignalAtTime (Item cue, AudioTime time)
 |||
 |||	  Description
 |||
 |||	    Conveniance macro that calls SignalAtAudioTime() with AF_GLOBAL_CLOCK.
 |||
 |||	  Arguments
 |||
 |||	    cue
 |||	        Item number of a cue.
 |||
 |||	    time
 |||	        The time at which to send a signal to the calling task.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error
 |||	    code (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Macro implemented in <audio/audio.h> V29.
 |||
 |||	  Notes
 |||
 |||	    This macro is equivalent to:
 |||
 |||	  -preformatted
 |||
 |||	        SignalAtAudioTime (AF_GLOBAL_CLOCK, cue, time)
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>
 |||
 |||	  See Also
 |||
 |||	    AbortTimerCue(), CreateCue(), GetCueSignal(), SleepUntilTime(),
 |||	    GetAudioClockRate(), SignalAtAudioTime()
 **/

/*****************************************************************/
 /**
 |||	AUTODOC -public -class audio -group Timer -name SignalAtAudioTime
 |||	Requests a wake-up call at a given time.
 |||
 |||	  Synopsis
 |||
 |||	    Err SignalAtAudioTime ( Item clock, Item cue, AudioTime time)
 |||
 |||	  Description
 |||
 |||	    This procedure requests that a signal be sent to the calling item at the
 |||	    specified audio time. The cue item can be created using CreateCue().
 |||
 |||	    A task can get the cue's signal mask using GetCueSignal(). The task can then
 |||	    call WaitSignal() to enter wait state until the cue's signal is sent at the
 |||	    specified time.
 |||
 |||	    If you need to, you can call AbortTimerCue() to cancel the timer request
 |||	    before completion.
 |||
 |||	    See the example program ta_timer(@) for a complete example.
 |||
 |||	  Arguments
 |||
 |||	    clock
 |||	        Item number of an AudioClock or AF_GLOBAL_CLOCK.
 |||
 |||	    cue
 |||	        Item number of a cue.
 |||
 |||	    time
 |||	        The time at which to send a signal to the calling task.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error
 |||	    code (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V29.
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>
 |||
 |||	  See Also
 |||
 |||	    AbortTimerCue(), CreateCue(), GetCueSignal(), SleepUntilAudioTime(),
 |||	    GetAudioClockRate()
 **/
int32 swiSignalAtAudioTime( Item clock, Item Cue, AudioTime Time)
/* Send a signal back to this task at the desired Time.
** This allows multiple signal waits.
*/
{
	AudioCue *acue;
	AudioClock *aclk;
	int32 Result;

DBUGX(("swiSignalAtAudioTime (  0x%x, 0x%x, 0x%x )\n", clock, Cue, Time));

	CHECKAUDIOOPEN;

/* Lookup Clock or substitute global clock. */
	if( clock == AF_GLOBAL_CLOCK )
	{
		aclk = AB_FIELD(af_GlobalClock);
	}
	else
	{
		aclk = (AudioClock *)CheckItem( clock, AUDIONODE, AUDIO_CLOCK_NODE);
		if (aclk == NULL)
		{
			return AF_ERR_BADITEM;
		}
	}

	acue = (AudioCue *)CheckItem(Cue, AUDIONODE, AUDIO_CUE_NODE);
	if (acue == NULL)
	{
		Result = AF_ERR_BADITEM;
		goto done;
	}

DBUGX(("swiSignalAtAudioTime:  aclk = 0x%x, acue = 0x%x, time = 0x%x\n", aclk, acue, Time ));

/* Don't let another task wait on this cuz it will never wake up. */
	if(!OWNEDBYCALLER(acue))
	{
		ERR(("swiSignalAtAudioTime: Cue 0x%x must be owned by caller.\n", Cue));
		Result = AF_ERR_NOTOWNER;
		goto done;
	}

	acue->acue_Event.aevt_Clock = aclk;
	Result = PostAudioEvent( (AudioEvent *) &acue->acue_Event, Time );

done:
DBUGX(("swiSignalAtAudioTime returns  0x%x\n", Result));
	return Result;
}

/*****************************************************************/
/* Cancel a request to SignalAtAudioTime */
 /**
 |||	AUTODOC -public -class audio -group Timer -name AbortTimerCue
 |||	Cancels a timer request enqueued with SignalAtAudioTime().
 |||
 |||	  Synopsis
 |||
 |||	    Err AbortTimerCue (Item Cue)
 |||
 |||	  Description
 |||
 |||	    Cancels a timer request enqueued with SignalAtAudioTime(). The signal
 |||	    associated with the cue will not be sent if the request is aborted before
 |||	    completion. Aborting a timer request after completion does nothing
 |||	    harmful. Aborting a cue that has not been posted as a timer request is
 |||	    harmless.
 |||
 |||	  Arguments
 |||
 |||	    Cue
 |||	        Cue item to abort. The task calling this function must own the Cue. The
 |||	        Cue may have already completed - nothing bad will happen.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error
 |||	    code (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V21.
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>
 |||
 |||	  See Also
 |||
 |||	    SignalAtAudioTime()
 **/
int32 swiAbortTimerCue( Item Cue )
{
	AudioCue *acue;
	int32 Result;

TRACEE(TRACE_INT, TRACE_TIMER, ("swiAbortTimerCue (  0x%x )\n", Cue ));

	CHECKAUDIOOPEN;

	acue = (AudioCue *)CheckItem(Cue, AUDIONODE, AUDIO_CUE_NODE);
	if (acue == NULL)
	{
		Result = AF_ERR_BADITEM;
		goto done;
	}

TRACEB(TRACE_INT, TRACE_TIMER, ("swiAbortTimerCue:  acue = 0x%x\n", acue));

/* Don't let another task abort cuz owner will never wake up. */
	if(!OWNEDBYCALLER(acue))
	{
		ERR(("swiAbortTimerCue: Cue 0x%x must be owned by caller.\n", Cue));
		Result = AF_ERR_NOTOWNER;
		goto done;
	}

	Result = UnpostAudioEvent( (AudioEvent *) &acue->acue_Event );

	ScheduleNextAudioTimerSignal();

done:
TRACER(TRACE_INT, TRACE_TIMER, ("swiAbortTimerCue returns  0x%x\n", Result));
	return Result;
}


/*****************************************************************/
 /**
 |||	AUTODOC -public -class audio -group Cue -name GetCueSignal
 |||	Returns a signal mask of a Cue(@).
 |||
 |||	  Synopsis
 |||
 |||	    int32 GetCueSignal (Item cue)
 |||
 |||	  Description
 |||
 |||	    This procedure returns a signal mask containing the signal bit allocated for
 |||	    an audio Cue item. The mask can be passed to WaitSignal() so a task can
 |||	    enter wait state until a Cue completes.
 |||
 |||	  Arguments
 |||
 |||	    cue
 |||	        Item number of Cue.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a signal mask (a positive value) if successful
 |||	    or a non-positive value (0 or an error code) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V20.
 |||
 |||	  Caveats
 |||
 |||	    This function has always returned 0 when a bad item is passed in. It
 |||	    should probably have returned AF_ERR_BADITEM, but will continue to
 |||	    return 0 for this case.
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>
 |||
 |||	  See Also
 |||
 |||	    CreateCue()
 **/
int32 GetCueSignal( Item Cue )
{
	AudioCue *acue;
	int32 Sig;
TRACEE(TRACE_INT, TRACE_TIMER, ("GetCueSignal ( 0x%x)\n",
	Cue));

	acue = (AudioCue *)CheckItem(Cue, AUDIONODE, AUDIO_CUE_NODE);
	if (acue == NULL) return 0;

	Sig = acue->acue_Signal;
TRACER(TRACE_INT, TRACE_TIMER, ("GetCueSignal returns 0x%x\n", Sig));
	return Sig;
}

/*****************************************************************/
 /**
 |||	AUTODOC -public -class audio -group Timer -name SleepUntilTime
 |||	Enters wait state until AF_GLOBAL_CLOCK reaches a specified AudioTime.
 |||
 |||	  Synopsis
 |||
 |||	    Err SleepUntilTime (Item cue, AudioTime time)
 |||
 |||	  Description
 |||
 |||	    Convenience macro that calls SleepUntilAudioTime() for AF_GLOBAL_CLOCK.
 |||
 |||	  Arguments
 |||
 |||	    cue
 |||	        The item number of the cue used to wait. This Cue shouldn't
 |||	        be used for anything else, or this function might return early.
 |||
 |||	    time
 |||	        AudioTime to wait for.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error
 |||	    code (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Macro implemented in <audio/audio.h> V29.
 |||
 |||	  Examples
 |||
 |||	    // If you want to wait for a specific number of ticks to go by,
 |||	    // you can get the current audio time from GetAudioTime() and
 |||	    // add your delay:
 |||	    SleepUntilTime (cue, GetAudioTime() + DELAYTICKS);
 |||
 |||	  Notes
 |||
 |||	    This macro is equivalent to:
 |||
 |||	  -preformatted
 |||
 |||	        SleepUntilAudioTime (AF_GLOBAL_CLOCK, cue, time)
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>
 |||
 |||	  See Also
 |||
 |||	    SleepUntilAudioTime(), GetAudioTime()
 **/

/*****************************************************************/
 /**
 |||	AUTODOC -public -class audio -group Timer -name SleepUntilAudioTime
 |||	Enters wait state until AudioClock(@) reaches a specified AudioTime.
 |||
 |||	  Synopsis
 |||
 |||	    Err SleepUntilAudioTime (Item clock, Item cue, AudioTime time)
 |||
 |||	  Description
 |||
 |||	    This procedure will not return until the specified audio time is reached.
 |||	    If that time has already passed, it will return immediately. If it needs
 |||	    to wait, your task will enter wait state so that it does not consume CPU
 |||	    time.
 |||
 |||	  Arguments
 |||
 |||	    clock
 |||	        Item number of an AudioClock or AF_GLOBAL_CLOCK.
 |||
 |||	    cue
 |||	        The item number of the cue used to wait. This Cue shouldn't
 |||	        be used for anything else, or this function might return early.
 |||
 |||	    time
 |||	        AudioTime to wait for.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error
 |||	    code (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V29.
 |||
 |||	  Examples
 |||
 |||	    // If you want to wait for a specific number of ticks to go by,
 |||	    // you can get the current audio time from ReadAudioClock() and
 |||	    // add your delay:
 |||
 |||	    AudioTime time;
 |||
 |||	    ReadAudioClock (clock, &time);
 |||	    SleepUntilAudioTime (clock, cue, time + DELAYTICKS);
 |||
 |||	  Notes
 |||
 |||	    This function is equivalent to (shown with error handling removed):
 |||
 |||	  -preformatted
 |||
 |||	        SignalAtAudioTime (clock, cue, time);
 |||	        WaitSignal (GetCueSignal(cue));
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>
 |||
 |||	  See Also
 |||
 |||	    SignalAtAudioTime(), CreateCue(), GetCueSignal(), GetAudioTime(),
 |||	    GetAudioClockRate()
 **/
int32 SleepUntilAudioTime( Item clock, Item Cue, AudioTime Time )
{
	uint32 CueSignal;
	int32 Result;

DBUG(("SleepUntilTime ( 0x%x, 0x%x )\n", Cue, Time));
	Result = SignalAtAudioTime( clock, Cue, Time );
	if (Result < 0) return Result;

	CueSignal = GetCueSignal ( Cue );
DBUG(("SleepUntilTime: Signal = 0x%x\n", CueSignal));
	WaitSignal( CueSignal );

DBUG(("SleepUntilTime returns 0x%x\n", 0));
	return 0;
}

/************************************************************************/
/****** Rate Control  ***************************************************/
/************************************************************************/
static Err lowSetAudioDuration ( AudioClock *aclk, int32 Duration )
{
	if((Duration < MIN_DSP_DURATION) || (Duration > MAX_DSP_DURATION))
	{
		ERR(("lowSetAudioDuration: Duration out of range = %d\n", Duration));
		return AF_ERR_OUTOFRANGE;
	}

/* Update clock state. */
	GetAudioClockTime( aclk );
	aclk->aclk_Duration = Duration;
	return 0;
}

/************************************************************************/
 /**
 |||	AUTODOC -public -class audio -group Timer -name SetAudioClockDuration
 |||	Changes duration of AudioClock(@) tick.
 |||
 |||	  Synopsis
 |||
 |||	    Err SetAudioClockDuration (Item clock, int32 numFrames)
 |||
 |||	  Description
 |||
 |||	    This procedure changes the rate of an audio clock by specifying a
 |||	    duration for the clock's tick. The clock is driven by a countdown
 |||	    timer in the DSP. When the DSP timer reaches zero, it increments the
 |||	    audio clock by one. The DSP timer is decremented at the sample rate
 |||	    of 44,100 Hz. Thus tick duration is given in units of sample frames.
 |||
 |||	    The current audio clock duration can be read with GetAudioClockDuration().
 |||
 |||	  Arguments
 |||
 |||	    clock
 |||	        Item number of an AudioClock.
 |||
 |||	    numFrames
 |||	        Duration of one audio clock tick in sample frames at 44,100 Hz.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error
 |||	    code (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V29.
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>
 |||
 |||	  See Also
 |||
 |||	    GetAudioClockDuration(), SetAudioClockRate(), GetAudioClockRate(),
 |||	    SignalAtAudioTime(), SleepUntilAudioTime(), GetAudioFolioInfo()
 |||
 **/
Err swiSetAudioClockDuration ( Item clock, int32 numFrames )
{
	AudioClock *aclk;

/* Lookup Clock. */
	aclk = (AudioClock *)CheckItem( clock, AUDIONODE, AUDIO_CLOCK_NODE);
	if (aclk == NULL)
	{
		return AF_ERR_BADITEM;
	}

	return lowSetAudioDuration( aclk, numFrames );
}


/************************************************************************/
 /**
 |||	AUTODOC -public -class audio -group Timer -name SetAudioClockRate
 |||	Changes the rate of the AudioClock(@).
 |||
 |||	  Synopsis
 |||
 |||	    Err SetAudioClockRate (Item clock, float32 hertz)
 |||
 |||	  Description
 |||
 |||	    This procedure changes the rate of the audio clock. The default rate is
 |||	    240 Hz. The range of the clock rate is 60 to system sample rate.
 |||
 |||	    The current audio clock rate can be read with GetAudioClockRate().
 |||
 |||	  Arguments
 |||
 |||	    clock
 |||	        Item number of an AudioClock.
 |||
 |||	    hertz
 |||	        Rate value in Hz. Must be in the range of 60 Hz to system sample rate.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error
 |||	    code (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V29.
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>
 |||
 |||	  See Also
 |||
 |||	    GetAudioClockRate(), SetAudioClockDuration(), GetAudioClockDuration(),
 |||	    SignalAtAudioTime(), SleepUntilAudioTime()
 |||
 **/
Err swiSetAudioClockRate ( Item clock, float32 hertz )
{
	AudioClock *aclk;
	int32 Dur;

/* Lookup Clock. */
	aclk = (AudioClock *)CheckItem(clock, AUDIONODE, AUDIO_CLOCK_NODE);
	if (aclk == NULL)
	{
		return AF_ERR_BADITEM;
	}

/* Integerize with rounding by adding 0.5 before conversion. */
	Dur = (DSPPData.dspp_SampleRate / hertz) + 0.5;
	return lowSetAudioDuration ( aclk, Dur );
}

/************************************************************************/
 /**
 |||	AUTODOC -public -class audio -group Timer -name GetAudioTime
 |||	Reads AF_GLOBAL_CLOCK time.
 |||
 |||	  Synopsis
 |||
 |||	    AudioTime GetAudioTime (void)
 |||
 |||	  Description
 |||
 |||	    This procedure returns the current 32-bit audio time from AF_GLOBAL_CLOCK.
 |||	    This timer is derived from the Audio DAC's 44100 Hz clock so it can be used
 |||	    to synchronize events with an audio stream. This timer typically runs at
 |||	    approximately, but not exactly, 240 Hz. Use GetAudioDuration() for an
 |||	    exact correlation between sample frames and audio timer ticks.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns the current audio time expressed in audio ticks.
 |||
 |||	  Implementation
 |||
 |||	    Convenience call implemented in audio folio V20.
 |||
 |||	  Notes
 |||
 |||	    This function is similar to:
 |||
 |||	  -preformatted
 |||
 |||	        ReadAudioClock (AF_GLOBAL_CLOCK, &time)
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>
 |||
 |||	  See Also
 |||
 |||	    SignalAtAudioTime(), GetAudioClockRate(), GetAudioClockDuration(),
 |||	    GetAudioFolioInfo(), ReadAudioClock()
 |||
 **/
AudioTime swiGetAudioTime (void)
{
	return GetAudioClockTime( AB_FIELD(af_GlobalClock) );
}


/************************************************************************/
 /**
 |||	AUTODOC -public -class audio -group Timer -name ReadAudioClock
 |||	Reads AudioTime from an AudioClock.
 |||
 |||	  Synopsis
 |||
 |||	    Err ReadAudioClock ( Item clock, AudioTime *timePtr )
 |||
 |||	  Description
 |||
 |||	    This procedure returns the current 32-bit audio time. This timer is
 |||	    derived from the Audio DAC's 44100 Hz clock so it can be used to synchronize
 |||	    events with an audio stream. This timer typically runs at approximately,
 |||	    but not exactly, 240 Hz. Use GetAudioClockDuration() for an exact correlation
 |||	    between sample frames and audio timer ticks.
 |||
 |||	  Arguments
 |||
 |||	    clock
 |||	        Item number of an AudioClock or AF_GLOBAL_CLOCK.
 |||
 |||	    timePtr
 |||	        Pointer to AudioTime variable that will be set to the current time.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error
 |||	    code (a negative value) if an error occurs.
 |||
 |||	    Writes AudioTime to *timePtr if successful. Writes nothing to *timePtr
 |||	    this function fails.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V29.
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>
 |||
 |||	  See Also
 |||
 |||	    SignalAtAudioTime(), GetAudioClockRate(), GetAudioClockDuration(),
 |||	    GetAudioFolioInfo(), GetAudioTime()
 |||
 **/
Err swiReadAudioClock( Item clock, AudioTime *timePtr )
{
	AudioClock *aclk;

/* Validate valuePtr */
	if( !IsMemWritable(timePtr,sizeof(AudioTime)) )
	{
		return AF_ERR_SECURITY;
	}

/* Lookup Clock or substitute global clock. */
	if( clock == AF_GLOBAL_CLOCK )
	{
		aclk = AB_FIELD(af_GlobalClock);
	}
	else
	{
		aclk = (AudioClock *)CheckItem( clock, AUDIONODE, AUDIO_CLOCK_NODE);
		if (aclk == NULL)
		{
			return AF_ERR_BADITEM;
		}
	}

	*timePtr = GetAudioClockTime( aclk );

	return 0;
}


/************************************************************************/
 /**
 |||	AUTODOC -public -class audio -group Timer -name GetAudioClockRate
 |||	Asks for AudioClock(@) rate in Hertz.
 |||
 |||	  Synopsis
 |||
 |||	    Err GetAudioClockRate( Item clock, float32 *hertz )
 |||
 |||	  Description
 |||
 |||	    This procedure gets an AudioClock rate in Hertz.
 |||
 |||	  Arguments
 |||
 |||	    clock
 |||	        AudioClock item to query, or AF_GLOBAL_CLOCK to query system-wide
 |||	        clock.
 |||
 |||	    hertz
 |||	        Pointer to float32 variable to store resulting clock rate in Hertz.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error
 |||	    code (a negative value) if an error occurs.
 |||
 |||	    Writes the clock rate to *hertz if successful. Writes nothing if this
 |||	    function returns an error.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V29.
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>
 |||
 |||	  See Also
 |||
 |||	    SignalAtAudioTime(), GetAudioClockDuration(), SetAudioClockRate()
 **/
Err GetAudioClockRate( Item clock, float32 *hertzPtr )
{
	AudioClock *aclk;

/* Validate valuePtr */
	if( !IsMemWritable(hertzPtr,sizeof(float32)) )
	{
		return AF_ERR_SECURITY;
	}

/* Lookup Clock or substitute global clock. */
	if( clock == AF_GLOBAL_CLOCK )
	{
		aclk = AB_FIELD(af_GlobalClock);
	}
	else
	{
		aclk = (AudioClock *)CheckItem( clock, AUDIONODE, AUDIO_CLOCK_NODE);
		if (aclk == NULL)
		{
			return AF_ERR_BADITEM;
		}
	}

	*hertzPtr = DSPPData.dspp_SampleRate / aclk->aclk_Duration;

DBUG(("GetAudioClockRate: Duration = %d, Rate = %g\n", aclk->aclk_Duration, *hertzPtr ));

	return 0;
}

/************************************************************************/
 /**
 |||	AUTODOC -public -class audio -group Timer -name GetAudioDuration
 |||	Asks for the duration of an AF_GLOBAL_CLOCK tick in frames.
 |||
 |||	  Synopsis
 |||
 |||	    int32 GetAudioDuration (void)
 |||
 |||	  Description
 |||
 |||	    This is a convenience macro that calls GetAudioClockDuration() for
 |||	    AF_GLOBAL_CLOCK.
 |||
 |||	  Return Value
 |||
 |||	    Current audio clock duration expressed as the actual number of sample
 |||	    frames between clock interrupts, or a negative error.
 |||
 |||	  Implementation
 |||
 |||	    Macro implemented in <audio/audio.h> V29.
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>
 |||
 |||	  See Also
 |||
 |||	    GetAudioClockDuration()
 **/
/************************************************************************/
 /**
 |||	AUTODOC -public -class audio -group Timer -name GetAudioClockDuration
 |||	Asks for the duration of an AudioClock(@) tick in frames.
 |||
 |||	  Synopsis
 |||
 |||	    int32 GetAudioClockDuration( Item clock )
 |||
 |||	  Description
 |||
 |||	    This procedure asks for the current duration of an AudioClock tick
 |||	    measured in DSP sample frames. The DSP typically runs at 44,100 Hz.
 |||
 |||	  Arguments
 |||
 |||	    clock
 |||	        AudioClock item to query, or AF_GLOBAL_CLOCK to query system-wide
 |||	        clock.
 |||
 |||	  Return Value
 |||
 |||	    Current audio clock duration expressed as the actual number of sample
 |||	    frames between clock interrupts, or a negative error.
 |||
 |||	  Implementation
 |||
 |||	    Folio call implemented in audio folio V29.
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>
 |||
 |||	  See Also
 |||
 |||	    SignalAtAudioTime(), GetAudioClockRate(), SetAudioClockDuration(),
 |||	    GetAudioFolioInfo()
 |||
 **/
int32   GetAudioClockDuration( Item clock )
{
	AudioClock *aclk;

/* Lookup Clock or substitute global clock. */
	if( clock == AF_GLOBAL_CLOCK )
	{
		aclk = AB_FIELD(af_GlobalClock);
	}
	else
	{
		aclk = (AudioClock *)CheckItem( clock, AUDIONODE, AUDIO_CLOCK_NODE);
		if (aclk == NULL)
		{
			return AF_ERR_BADITEM;
		}
	}

DBUG(("GetAudioClockDuration: Duration = %d\n", aclk->aclk_Duration ));

	return aclk->aclk_Duration;
}

/*****************************************************************/
int32 PerformCueSignal( AudioCue *acue )
{
TRACEB(TRACE_INT, TRACE_TIMER, ("PerformCueSignal: Signal =  0x%x\n", acue->acue_Signal));
DBUGEVENT(("PerformCueSignal: Task=0x%x Signal=0x%x\n", acue->acue_Task, acue->acue_Signal));
	return SuperinternalSignal( acue->acue_Task, acue->acue_Signal );
}

/*****************************************************************/
static void MoveTimerEventsFromSlowToFast( void )
{
	AudioTime  CriticalTime;
	AudioEvent *aevt;

/* Calculate critical time in future. */
	CriticalTime = GetAudioClockTime( AB_FIELD(af_GlobalClock) ) +
		ConvertFramesToClockTicks( AB_FIELD(af_GlobalClock), MAX_FUTURE_FRAMES );

/* Check list to see who's next. */
	aevt = (AudioEvent *) FIRSTNODE( &AB_FIELD(af_SlowTimerList) );
DBUG(("MoveTimerEventsFromSlowToFast: aevt =  0x%x\n", aevt));
/* Process all nodes scheduled for critical time or earlier. */
	while ( ISNODE(&AB_FIELD(af_SlowTimerList), aevt) &&
		AudioTimeLaterThanOrEqual( CriticalTime, aevt->aevt_ListTime) )
	{
/* Remove and process node.*/
 		UnpostAudioEvent( aevt );         /* Remove from list. */

		PostEventToFastList( aevt );

/* Since old one is removed, just keep working on first node. */
		aevt = (AudioEvent *) FIRSTNODE( &AB_FIELD(af_SlowTimerList) );
	}
}

/*****************************************************************/
static void ResyncAudioClocks( void )
{
	Node *n;
	DBUG(("Resyncing all Audio Clocks\n"));

	for( n = (Node *) FirstNode( &AB_FIELD(af_ClockList) );
	     ISNODE( &AB_FIELD(af_ClockList), n);
	     n = (Node *) NextNode(n) )
	{
		GetAudioClockTime( (AudioClock *) n ); /* 960603 */
	}
}

/*****************************************************************/
/* Process Timer events. Called when timer interrupt occurs. */
void HandleTimerSignal( void )
{
	AudioEvent *aevt;
	uint32 currentFrameCount;

	currentFrameCount = dsppGetCurrentFrameCount();

DBUGINT(("HandleTimerSignal: currentFrameCount =  0x%x\n", currentFrameCount));

/* Check list to see who's next. */
	aevt = (AudioEvent *) FIRSTNODE( &AB_FIELD(af_FastTimerList) );
DBUGX(("HandleTimerSignal: aevt =  0x%x\n", aevt));

/*
** 940413 Fixed list walker.  The old code relied on NEXTNODE to traverse the list.
** This caused a problem if an instrument had two envelopes and one ended with a FLS
** before the other one.  It would stop the instrument which removed the nextnode
** from the list causing it to leap into outer space.  FIRSTNODE only gets valid nodes.
*/
/* Process all nodes scheduled for this time or earlier. */
	while ( ISNODE(&AB_FIELD(af_FastTimerList), aevt) &&
		AudioTimeLaterThanOrEqual(currentFrameCount,aevt->aevt_ListTime) )
	{
		DBUGEVENT(("HandleTimerSignal: notify event 0x%x\n", aevt));
/* Remove and process node.*/
 		UnpostAudioEvent( aevt );         /* Remove from list. */
		aevt->aevt_Perform( aevt );       /* Execute callback function. */
/* Since old one is removed, just keep working on first node. */
		aevt = (AudioEvent *) FIRSTNODE( &AB_FIELD(af_FastTimerList) );
	}

	MoveTimerEventsFromSlowToFast();

	ScheduleNextAudioTimerSignal();

/* Periodically resync each clock so that it is not affected by wraparound
** in the FrameCounter.
*/
	if( AudioTimeLaterThanOrEqual(currentFrameCount, AB_FIELD(af_NextSyncFrame)) )
	{
		ResyncAudioClocks();
		AB_FIELD(af_NextSyncFrame) = currentFrameCount + SYNC_INTERVAL;
	}
}

/*****************************************************************/
/****** Item Creation ********************************************/
/*****************************************************************/

 /**
 |||	AUTODOC -public -class Items -group Audio -name Cue
 |||	Audio asynchronous notification item.
 |||
 |||	  Description
 |||
 |||	    "Cue the organist."
 |||
 |||	    This Item type is used to receive asynchronous notification of some event
 |||	    from the audio folio:
 |||
 |||	    - sample playback completion
 |||
 |||	    - envelope completion
 |||
 |||	    - audio timer request completion
 |||
 |||	    - trigger going off
 |||
 |||	    Each Cue has a signal bit associated with it. Because signals are
 |||	    task-relative, Cues cannot be shared between multiple tasks.
 |||
 |||	  Folio
 |||
 |||	    audio
 |||
 |||	  Item Type
 |||
 |||	    AUDIO_CUE_NODE
 |||
 |||	  Create
 |||
 |||	    CreateCue(), CreateItem()
 |||
 |||	  Delete
 |||
 |||	    DeleteCue(), DeleteItem()
 |||
 |||	  Query
 |||
 |||	    GetCueSignal()
 |||
 |||	  Use
 |||
 |||	    AbortTimerCue(), ArmTrigger(), MonitorAttachment(), SignalAtAudioTime(),
 |||	    SleepUntilAudioTime()
 |||
 |||	  See Also
 |||
 |||	    Attachment(@), AudioClock(@)
 **/

 /**
 |||	AUTODOC -public -class audio -group Cue -name CreateCue
 |||	Creates an audio Cue(@).
 |||
 |||	  Synopsis
 |||
 |||	    Item CreateCue (const TagArg *tagList)
 |||
 |||	    Item CreateCueVA (uint32 tag1, ...)
 |||
 |||	  Description
 |||
 |||	    Create an audio cue, which is an item associated with a system signal. A
 |||	    task can get the signal mask of the cue's signal by calling GetCueSignal().
 |||
 |||	    When a task uses an audio timing call such as SignalAtAudioTime(), it passes
 |||	    the item number of the audio cue to the procedure. The task then calls
 |||	    WaitSignal() to enter wait state, where it waits for the cue's signal
 |||	    at the specified time.
 |||
 |||	    Since each task has its own set signals, Cues cannot be shared among tasks.
 |||
 |||	    Call DeleteCue() to dispose of a Cue item.
 |||
 |||	  Tag Arguments
 |||
 |||	    None
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns the item number of the cue (a positive value) or an
 |||	    error code (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Macro implemented in the <audio/audio.h> V21.
 |||
 |||	  Notes
 |||
 |||	    This macro is equivalent to:
 |||
 |||	  -preformatted
 |||
 |||	        CreateItem (MKNODEID(AUDIONODE,AUDIO_CUE_NODE), tagList)
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>
 |||
 |||	  See Also
 |||
 |||	    DeleteCue(), GetCueSignal(), MonitorAttachment(), SignalAtAudioTime(),
 |||	    ArmTrigger(), Cue(@)
 **/
Item internalCreateAudioCue (AudioCue *acue, const TagArg *args)
{
	int32 Result;

TRACEE(TRACE_INT,TRACE_ITEM,("internalCreateAudioCue(0x%x, 0x%lx)\n", acue, args));

	Result = TagProcessor (acue, args, NULL, NULL);
	if(Result < 0)
	{
		ERR(("internalCreateAudioCue: TagProcessor failed.\n"));
		return Result;
	}

	{
		const int32 sig = SuperAllocSignal( 0 );

		if (sig <= 0)
		{
			ERR(("internalCreateAudioCue could not SuperAllocSignal\n"));
			return sig ? sig : AF_ERR_NOSIGNAL;
		}
		acue->acue_Signal = sig;
	}

	acue->acue_Task = CURRENTTASK;
	acue->acue_Event.aevt_InList = NULL;
	acue->acue_Event.aevt_Perform = (void *)PerformCueSignal;

DBUG(("internalCreateAudioCue: Cue = 0x%x, Task = 0x%x, Signal = 0x%x\n",
	acue->acue_Event.aevt_Node.n_Item, acue->acue_Task, acue->acue_Signal));

	return (acue->acue_Event.aevt_Node.n_Item);
}

/******************************************************************/
 /**
 |||	AUTODOC -public -class audio -group Cue -name DeleteCue
 |||	Deletes an audio Cue(@)
 |||
 |||	  Synopsis
 |||
 |||	    Err DeleteCue (Item Cue)
 |||
 |||	  Description
 |||
 |||	    This macro deletes an audio cue and frees its resources (including its
 |||	    associated signal).
 |||
 |||	  Arguments
 |||
 |||	    Cue
 |||	        The item number of the cue to delete.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns zero if successful or an error code (a negative
 |||	    value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Macro implemented in <audio/audio.h> V21.
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>
 |||
 |||	  See Also
 |||
 |||	    CreateCue()
 **/
int32 internalDeleteAudioCue (AudioCue *acue)
{

	Item CueOwnerItem;
	Task *OwnerTask;
	int32 Result;

TRACEE(TRACE_INT,TRACE_ITEM,("internalDeleteAudioCue(0x%x)\n", acue ));

	CueOwnerItem = acue->acue_Event.aevt_Node.n_Owner;

	OwnerTask = (Task *)LookupItem( CueOwnerItem );
	if (OwnerTask)
	{
		Result = SuperInternalFreeSignal( acue->acue_Signal, OwnerTask );
		if(Result) ERR(("Result of SuperInternalFreeSignal = 0x%x\n", Result));
	}

	UnpostAudioEvent( (AudioEvent *) &acue->acue_Event );
	ScheduleNextAudioTimerSignal();

	acue->acue_Signal = 0;
TRACER(TRACE_INT, TRACE_TIMER, ("internalDeleteAudioCue returns 0x%x\n", 0));
	return (0);
}

 /**
 |||	AUTODOC -public -class audio -group Timer -name CreateAudioClock
 |||	Creates an AudioClock(@).
 |||
 |||	  Synopsis
 |||
 |||	    Item CreateAudioClock (const TagArg *tagList)
 |||
 |||	    Item CreateAudioClockVA (uint32 tag1, ...)
 |||
 |||	  Description
 |||
 |||	    Create a custom audio clock which can run at a different rate than the
 |||	    global audio clock.  Custom clocks are useful if you need to change
 |||	    the tempo of MIDI scores.
 |||
 |||	    Call DeleteAudioClock() to dispose of a clock item.
 |||
 |||	  Tag Arguments
 |||
 |||	    None
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns the item number of the clock (a positive value) or an
 |||	    error code (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Macro implemented in the <audio/audio.h> V29.
 |||
 |||	  Notes
 |||
 |||	    This macro is equivalent to:
 |||
 |||	  -preformatted
 |||
 |||	        CreateItem (MKNODEID(AUDIONODE,AUDIO_CLOCK_NODE), tagList)
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>
 |||
 |||	  See Also
 |||
 |||	    DeleteAudioClock(), ReadAudioClock(), SetAudioClockRate(), SetAudioClockRate(),
 |||	    AudioClock(@)
 **/
Item internalCreateAudioClock (AudioClock *aclk, const TagArg *args)
{
	int32 Result;

TRACEE(TRACE_INT,TRACE_ITEM,("internalCreateAudioClock(0x%x, 0x%lx)\n", aclk, args));

	Result = TagProcessor (aclk, args, NULL, NULL);
	if(Result < 0)
	{
		ERR(("internalCreateAudioClock: TagProcessor failed.\n"));
		return Result;
	}

/* Set up clock internals. */
	aclk->aclk_Duration = (DSPPData.dspp_SampleRate / AF_DEFAULT_CLOCK_RATE) + 0.5; /* 960409 */
	aclk->aclk_OldFrames = dsppGetCurrentFrameCount();

/* Keep list Clock Items. */
	AddTail (&AB_FIELD(af_ClockList), (Node *)aclk);

	return (aclk->aclk_Item.n_Item);
}

/******************************************************************/
 /**
 |||	AUTODOC -public -class audio -group Timer -name DeleteAudioClock
 |||	Deletes an AudioClock(@)
 |||
 |||	  Synopsis
 |||
 |||	    Err DeleteAudioClock (Item AudioClock)
 |||
 |||	  Description
 |||
 |||	    This macro deletes an AudioClock and frees its resources.
 |||	    Any pending timer events for this clock will be triggered as if
 |||	    their time had come in order to prevent deadlocks.
 |||
 |||	  Arguments
 |||
 |||	    AudioClock
 |||	        The item number of the AudioClock to delete.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns zero if successful or an error code (a negative
 |||	    value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Macro implemented in <audio/audio.h> V29.
 |||
 |||	  Associated Files
 |||
 |||	    <audio/audio.h>
 |||
 |||	  See Also
 |||
 |||	    CreateAudioClock()
 **/

/*****************************************************************/
static void TriggerAllClockEvents( List *list, AudioClock *aclk )
{
	AudioEvent *aevt, *nextAevt;

/* Scan list for matching clock. */
	aevt = (AudioEvent *) FIRSTNODE( list );
/* Process all nodes scheduled for critical time or earlier. */
	while ( ISNODE(list, aevt) )
	{
DBUG(("TriggerAllClockEvents: aevt =  0x%x\n", aevt));
/* Get next node before we remove current. */
		nextAevt = (AudioEvent *) NEXTNODE( aevt );

		if( aevt->aevt_Clock == aclk )
		{
/* Remove and process node.*/
 			UnpostAudioEvent( aevt );         /* Remove from list. */
			aevt->aevt_Perform( aevt );       /* Execute callback function. */
		}
		aevt = nextAevt;
	}
}

/*****************************************************************/
int32 internalDeleteAudioClock (AudioClock *aclk)
{

DBUG(("internalDeleteAudioClock(0x%x)\n", aclk ));

/* Scan Fast and Slow lists and trigger all associated cues to prevent deadlock. */
	TriggerAllClockEvents( &AB_FIELD(af_FastTimerList), aclk );
	TriggerAllClockEvents( &AB_FIELD(af_SlowTimerList), aclk );

	ScheduleNextAudioTimerSignal();

	ParanoidRemNode ((Node *)aclk);   /* Remove from AudioFolio list. */
TRACER(TRACE_INT, TRACE_TIMER, ("internalDeleteAudioClock returns 0x%x\n", 0));
	return (0);
}

