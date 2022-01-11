
/******************************************************************************
**
**  @(#) ta_bugs.c 96/03/28 1.2
**
******************************************************************************/

/*
** ta_auto.c - automatic tests of DSPP function that do not require listening.
*
** Author: Phil Burk
** Copyright 3DO 1995-
*/

#include <audio/audio.h>
#include <audio/parse_aiff.h>
#include <kernel/mem.h>         /* memdebug */
#include <kernel/operror.h>
#include <stdio.h>
#include <stdlib.h>             /* malloc()*/

/* Handy printing and debugging macros. */
#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */

#define MIN(a,b)    ((a)<(b)?(a):(b))
#define MAX(a,b)    ((a)>(b)?(a):(b))
#define ABS(x)      ((x)<0?(-(x)):(x))

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
		TOUCH(err); \
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

/*************************************************************
** Wait some number of audio frames to allow DSPP to propagate
** values across several instruments.
************************************************************/
int32 WaitAudioFrames( int32 NumFrames )
{
	int32 Start, Current, Elapsed;

	if( NumFrames > 32767 )
	{
		ERR(("WaitAudioFrames: Frame count is 16 bit, give me a break!\n"));
		return -1;
	}
	Start = GetAudioFrameCount();
	do
	{
		Yield();
		Current = GetAudioFrameCount();
		Elapsed = (Current - Start) & 0xFFFF;
	} while ( Elapsed < NumFrames );
	return 0;
}

/*************************************************************
** Compare floats within range.
************************************************************/
int32  MatchFloats( float32 f1, float32 f2, float32 tolerance )
{
	float32 margin;

	margin = ABS(f1*tolerance);
	return( (f1 > (f2-margin)) && (f1 < (f2+margin)) );
}

/*************************************************************
** Test to see whether a knob range is set correctly.
************************************************************/
Err tKnobRange( void )
{
	Item TimesPlusIns;
	Item KnobA=0;
	Item KnobC=0;
	Item AddProbe=0;
	float32 Value;
	Err Result;

	PRT(("Reproduce CR #5833.\n"));
	PRT(("Default clip range is used instead of coerced type range.\n"));
	CALL_CHECK( (TimesPlusIns = LoadInstrument("timesplus_noclip.dsp",  0, 0)), "load add.dsp");
	CALL_CHECK( (StartInstrument( TimesPlusIns, NULL )), "start TimesPlusIns");
	CALL_CHECK( (KnobA = CreateKnob( TimesPlusIns, "InputA", NULL  )), "grab InputA");
	CALL_CHECK( (SetKnob( KnobA, 0.0  )), "Set InputA to 0.0");
	CALL_CHECK( (KnobC = CreateKnobVA( TimesPlusIns, "InputC",
		AF_TAG_TYPE, AF_SIGNAL_TYPE_GENERIC_UNSIGNED, TAG_END  )), "grab InputC");
	CALL_CHECK( (AddProbe = CreateProbeVA(TimesPlusIns, "Output",
		AF_TAG_TYPE, AF_SIGNAL_TYPE_GENERIC_UNSIGNED, TAG_END)), "CreateProbe" );

/* Test KnobA1 */
#define TRD_KNOB_PROBE(_knob,_testval) \
	CALL_CHECK( (SetKnob( (_knob) , (_testval) )), "tweak Knob" ); \
	WaitAudioFrames(6); \
	CALL_CHECK( (ReadProbe( AddProbe, &Value )), "check Probe" ); \
	if( !MatchFloats( Value, (_testval), 0.01 ) ) \
	{ \
		ERR(( "Probe does not match input. Got %g , expected %g\n", Value, (_testval) )); \
		Result = -1; \
		goto clean; \
	}

	TRD_KNOB_PROBE( KnobC,  0.123 );
	TRD_KNOB_PROBE( KnobC,  0.789 );
	TRD_KNOB_PROBE( KnobC,  1.234 );
	TRD_KNOB_PROBE( KnobC,  1.987 );
clean:

	CALL_REPORT( (DeleteProbe( AddProbe )), "DeleteProbe" );
	if( TimesPlusIns )
	{
		CALL_REPORT( (DeleteKnob( KnobA )), "DeleteKnob" );
		CALL_REPORT( (DeleteKnob( KnobC )), "DeleteKnob" );
		CALL_REPORT( (UnloadInstrument( TimesPlusIns )), "UnloadInstrument" );
	}

PRT(("tKnobRange returns 0x%x\n", Result ));
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

	PASS_FAIL( (tKnobRange()), "Range check knobs." );

	CloseAudioFolio();
	PRT(( "%s finished.\n", argv[0] ));
	return((int) Result);
}
