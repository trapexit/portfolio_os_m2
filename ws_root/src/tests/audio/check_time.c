
/******************************************************************************
**
**  @(#) check_time.c 96/06/03 1.2
**
******************************************************************************/

/*
** check_time.c - Test multiple timer Cues.
*
** Author: Phil Burk
** Copyright 3DO 1995-
*/

#include <audio/audio.h>
#include <audio/parse_aiff.h>
#include <kernel/mem.h>         /* memdebug */
#include <kernel/operror.h>
#include <kernel/time.h>
#include <stdio.h>
#include <stdlib.h>             /* malloc()*/

/* Handy printing and debugging macros. */
#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */

#define MIN(a,b)    ((a)<(b)?(a):(b))
#define MAX(a,b)    ((a)>(b)?(a):(b))
#define ABS(x)      ((x)<0?(-(x)):(x))

static Err CalibrateTimer( void );

/* Macro to simplify error checking. */
#define CALL_CHECK(_exp,msg) \
	DBUG(("%s\n",msg)); \
	do \
	{ \
		if ((Result = (Err) (_exp)) < 0) \
		{ \
			PRT(("error code = 0x%x\n",Result)); \
			PrintError(0,"\\failure in",msg,Result); \
			goto clean; \
		} \
	} while(0)

/* Macro to simplify error checking. */
#define CALL_REPORT(_exp,msg) \
	do \
	{ \
		int32 err; \
		if ((err = (Err) (_exp)) < 0) \
		{ \
			PrintError(0,"\\failure in",msg, err); \
		} \
	} while(0)


/* Macro to simplify error checking. */
#define PASS_FAIL(_exp,msg) \
	do \
	{ \
		PRT(("============================================\n")); \
		PRT(("Please wait for test to complete. %s\n", msg)); \
		if ((Result = (Err) (_exp)) < 0) \
		{ \
			PrintError(0,"\\ERROR in",msg,Result); \
		} else { \
			PRT(("SUCCESS in %s\n", msg )); \
		} \
	} while(0)


/****************************************************************
** Sleep on multiple Cues
****************************************************************/
static Err CalibrateTimer( void )
{
#define NUM_CUES   (10)
#define DELAY_TOLERANCE   (2)
#define DELAY_DURATION    (240)
	Item  MyCues[NUM_CUES];
	int32 MySignals[NUM_CUES], i;
	int32 SignalMask = 0, Signals;
	uint32 StartTime, EndTime = 0;
	TimeVal         oldTime, newTime, deltaTime;
	int32           elapsedUSecs;
	float32   clockRate, audioSeconds, realSeconds;
	int32 Result;

	CALL_CHECK( (GetAudioClockRate( AF_GLOBAL_CLOCK, &clockRate )), "GetAudioClockRate");
	PRT(("Clock Rate = %g\n", clockRate));

/* Create several Cues. */
	for( i=0; i<NUM_CUES; i++ )
	{
		CALL_CHECK( (MyCues[i] = CreateCue( NULL )), "create another Cue");
		DBUG(("MyCues[%d] = 0x%x\n", i, MyCues[i]));
		CALL_CHECK( (MySignals[i] = GetCueSignal( MyCues[i] )), "get signal from Cue");
		SignalMask |= MySignals[i];
	}

	PRT(("Calibrate Timer\n"));

/* Schedule wakeup calls at various times in the future. */
	StartTime = GetAudioTime();
	SampleSystemTimeTV(&oldTime);

	for( i=0; i<NUM_CUES; i++ )
	{
		CALL_CHECK( (Result = SignalAtTime( MyCues[i], StartTime + ((i+1)*DELAY_DURATION) )), "call SignalAtTime");
	}

	while( SignalMask != 0 )
	{
		Signals = WaitSignal( SignalMask );
		EndTime = GetAudioTime();
		PRT(("Signal=0x%x\n", Signals, EndTime ));
		SignalMask &= ~Signals;
	}
		
	SampleSystemTimeTV(&newTime);
	SubTimes(&oldTime,&newTime,&deltaTime);
	oldTime = newTime;
	elapsedUSecs = deltaTime.tv_usec + deltaTime.tv_sec*1000000;
	PRT(("StartTime = 0x%x, EndTime = 0x%x, SignalMask = 0x%x\n", StartTime, EndTime, SignalMask));
	PRT(("Elapsed ticks = %d, elapsed microseconds = %d\n", EndTime - StartTime, elapsedUSecs));
	audioSeconds = (EndTime - StartTime) / clockRate;
	realSeconds = elapsedUSecs / 1000000.0;
	PRT(("Seconds: audio = %g, real = %g, ratio = %g\n",
		audioSeconds, realSeconds,  (audioSeconds/realSeconds) ));
clean:

	for( i=0; i<NUM_CUES; i++ )
	{
		DeleteCue( MyCues[i] );
	}
	return Result;
}


/************************************************************/
int main(int argc, char *argv[])
{
	int32 Result;

	PRT(("Begin %s\n", argv[0] ));
	TOUCH(argc); /* Eliminate anal compiler warning. */

/* Initialize audio, return if error. */
	Result = OpenAudioFolio();
	if (Result < 0)
	{
		PrintError(0,"Audio Folio could not be opened.",0,Result);
		return(-1);
	}

	PASS_FAIL( (CalibrateTimer()), "Test multiple Cue delays." );

	CloseAudioFolio();
	PRT(( "%s finished.\n", argv[0] ));
	return((int) Result);
}
