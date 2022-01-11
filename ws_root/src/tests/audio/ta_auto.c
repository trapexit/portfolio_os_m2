
/******************************************************************************
**
**  @(#) ta_auto.c 96/08/13 1.43
**
******************************************************************************/

/*
** ta_auto.c - automatic tests of DSPP function that do not require listening.
*
** Author: Phil Burk
** Copyright 3DO 1995-
**
** 960522 PLB Added test for pitch and velocity based multisample selection.
*/

#include <audio/audio.h>
#include <audio/parse_aiff.h>
#include <kernel/mem.h>         /* memdebug */
#include <kernel/operror.h>
#include <stdio.h>
#include <stdlib.h>             /* malloc()*/
#include <math.h>               /* powf()*/

/* Handy printing and debugging macros. */
#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */

#define MIN(a,b)    ((a)<(b)?(a):(b))
#define MAX(a,b)    ((a)>(b)?(a):(b))
#define ABS(x)      ((x)<0?(-(x)):(x))

#define TEST_LOAD_ALL     1
#define TEST_KNOBS        1
#define TEST_TIMER        1
#define TEST_SAMPLE       1
#define TEST_MULTISAMPLE  1
#define TEST_MIXER        1
#define TEST_ENVELOPES    1

static Err tMultipleCues( void );
static Err tChangeClockrate( void );
static int32 tPredictEnvelopeValue( EnvelopeSegment *EnvPoints,
	int32 NumPoints, float32 EnvTime, float32 *CurValuePtr );
Err tEnvelope1 ( int32 Divider, float32 timeScale, int32 ifLockTimeScale,
	int32 BaseNote, int32 NotesPerDouble, int32 Pitch, int32 startAt );
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
			TOUCH(Result); \
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
** Test loading an instrument.
************************************************************/
Err tLoadInstrument( const char *insName )
{
	Item    insTmp;
	Item    ins0, ins1;
	Err     Result;

	if ((Result = insTmp = LoadInsTemplate (insName, NULL)) < 0) goto clean;
	if ((Result = ins0 = CreateInstrument (insTmp, NULL)) < 0) goto clean;
	if ((Result = ins1 = CreateInstrument (insTmp, NULL)) < 0) goto clean;
	CALL_CHECK( (DeleteInstrument(ins0)), "DeleteInstrument ins0");
	CALL_CHECK( (DeleteInstrument(ins1)), "DeleteInstrument ins1");
	CALL_CHECK( (UnloadInsTemplate(insTmp)), "UnloadInsTemplate");
	return 0;

clean:
	UnloadInsTemplate(insTmp);
	return Result;
}

/*************************************************************
** Test loading all instrument.
************************************************************/
Err tAllInstruments( void )
{
	Err Result = 0;

	#define CHECK_LOAD(name)                                        \
	{                                                               \
		Err errcode;                                                \
																	\
		if ((errcode = tLoadInstrument(name)) < 0) {                \
			PrintError (NULL, "load instrument", (name), errcode);  \
			Result = -1;                                            \
		}                                                           \
		TOUCH(errcode);                                             \
	}

	CHECK_LOAD("add.dsp");
	CHECK_LOAD("benchmark.dsp");
	CHECK_LOAD("chaos_1d.dsp");
	CHECK_LOAD("cubic_amplifier.dsp");
	CHECK_LOAD("deemphcd.dsp");
	CHECK_LOAD("delay4.dsp");
	CHECK_LOAD("delay_f1.dsp");
	CHECK_LOAD("delay_f2.dsp");
	CHECK_LOAD("depopper.dsp");
	CHECK_LOAD("envelope.dsp");
	CHECK_LOAD("envfollower.dsp");
	CHECK_LOAD("expmod_unsigned.dsp");
	CHECK_LOAD("filter_1o1z.dsp");
	CHECK_LOAD("filter_3d.dsp");
	CHECK_LOAD("impulse.dsp");
	CHECK_LOAD("integrator.dsp");
	CHECK_LOAD("latch.dsp");
/*	CHECK_LOAD("line_in.dsp");   !!! Need EnableAudioIn() */
	CHECK_LOAD("line_out.dsp");
	CHECK_LOAD("maximum.dsp");
	CHECK_LOAD("minimum.dsp");
	CHECK_LOAD("multiply.dsp");
	CHECK_LOAD("multiply_unsigned.dsp");
	CHECK_LOAD("noise.dsp");
	CHECK_LOAD("pulse.dsp");
	CHECK_LOAD("pulse_lfo.dsp");
	CHECK_LOAD("randomhold.dsp");
	CHECK_LOAD("rednoise.dsp");
	CHECK_LOAD("rednoise_lfo.dsp");
	CHECK_LOAD("sampler_16_f1.dsp");
	CHECK_LOAD("sampler_16_f2.dsp");
	CHECK_LOAD("sampler_16_v1.dsp");
	CHECK_LOAD("sampler_16_v2.dsp");
	CHECK_LOAD("sampler_8_f1.dsp");
	CHECK_LOAD("sampler_8_f2.dsp");
	CHECK_LOAD("sampler_8_v1.dsp");
	CHECK_LOAD("sampler_8_v2.dsp");
	CHECK_LOAD("sampler_adp4_v1.dsp");
	CHECK_LOAD("sampler_adp4_v2.dsp");
	CHECK_LOAD("sampler_cbd2_f1.dsp");
	CHECK_LOAD("sampler_cbd2_f2.dsp");
	CHECK_LOAD("sampler_cbd2_v1.dsp");
	CHECK_LOAD("sampler_cbd2_v2.dsp");
	CHECK_LOAD("sampler_cbd2_v4.dsp");
	CHECK_LOAD("sampler_drift_v1.dsp");
	CHECK_LOAD("sampler_raw_f1.dsp");
	CHECK_LOAD("sampler_sqs2_f1.dsp");
	CHECK_LOAD("sampler_sqs2_v1.dsp");
	CHECK_LOAD("sawtooth.dsp");
	CHECK_LOAD("sawtooth_lfo.dsp");
	CHECK_LOAD("schmidt_trigger.dsp");
	CHECK_LOAD("slew_rate_limiter.dsp");
	CHECK_LOAD("square.dsp");
	CHECK_LOAD("square_lfo.dsp");
	CHECK_LOAD("subtract.dsp");
	CHECK_LOAD("svfilter.dsp");
	CHECK_LOAD("tapoutput.dsp");
	CHECK_LOAD("times_256.dsp");
	CHECK_LOAD("timesplus.dsp");
	CHECK_LOAD("timesplus_noclip.dsp");
	CHECK_LOAD("triangle.dsp");
	CHECK_LOAD("triangle_lfo.dsp");

	return Result;
}

Err tAllIllegalInstruments( void )
{
	Err Result = 0;

	#define CHECK_LOAD_FAIL(name,expectedErr)                       \
	{                                                               \
		Err errcode;                                                \
																	\
		if ((errcode = tLoadInstrument(name)) != (expectedErr)) {   \
			printf ("ERROR in load instrument '%s'\n", (name));     \
			printf ("  expected 0x%x ", (expectedErr));             \
			PrintfSysErr (expectedErr);                             \
			printf ("  got 0x%x ", errcode);                        \
			if (errcode < 0) PrintfSysErr (errcode);                \
			Result = -1;                                            \
		}                                                           \
	}

	CHECK_LOAD_FAIL("nanokernel.dsp", AF_ERR_BADPRIV);
	CHECK_LOAD_FAIL("sub_decode_adp4.dsp", AF_ERR_BADPRIV);
	CHECK_LOAD_FAIL("sub_envelope.dsp", AF_ERR_BADPRIV);
	CHECK_LOAD_FAIL("sub_sampler_16_v1.dsp", AF_ERR_BADPRIV);
	CHECK_LOAD_FAIL("sub_sampler_adp4_v1.dsp", AF_ERR_BADPRIV);

	CHECK_LOAD_FAIL("add_accum.dsp", AF_ERR_PATCHES_ONLY);
	CHECK_LOAD_FAIL("input_accum.dsp", AF_ERR_PATCHES_ONLY);
	CHECK_LOAD_FAIL("multiply_accum.dsp", AF_ERR_PATCHES_ONLY);
	CHECK_LOAD_FAIL("output_accum.dsp", AF_ERR_PATCHES_ONLY);
	CHECK_LOAD_FAIL("subtract_accum.dsp", AF_ERR_PATCHES_ONLY);
	CHECK_LOAD_FAIL("subtract_from_accum.dsp", AF_ERR_PATCHES_ONLY);

	return Result;
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
** Test to see whether a knob is restored after an instrument
** connection is broken.
************************************************************/
Err tRestoreDisconnect( void )
{
	Item AddIns0;
	Item AddIns1=0;
	Item KnobA0=0;
	Item KnobA1=0;
	Item AddProbe=0;
	float32 Value;
	Err Result;

/* Load a directout instrument to send the sound to the DAC. */
	CALL_CHECK( (AddIns0 = LoadInstrument("add.dsp",  0, 0)), "load add.dsp");
	CALL_CHECK( (StartInstrument( AddIns0, NULL )), "start AddIns0");
	CALL_CHECK( (KnobA0 = CreateKnob( AddIns0, "InputA", NULL )), "grab InputA on 0");

	CALL_CHECK( (AddIns1 = LoadInstrument("add.dsp",  0, 0)), "load add.dsp");
	CALL_CHECK( (StartInstrument( AddIns1, NULL )), "start AddIns0");
	CALL_CHECK( (KnobA1 = CreateKnob( AddIns1, "InputA", NULL )), "grab InputA on 1");

	CALL_CHECK( (AddProbe = CreateProbe(AddIns1, "Output", NULL)), "CreateProbe" );

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

	TRD_KNOB_PROBE( KnobA1,  0.123 );
	TRD_KNOB_PROBE( KnobA1, -0.7654 );

/* Connect two instruments. Read knob that is passed through connection. */
	CALL_CHECK( (ConnectInstruments( AddIns0, "Output", AddIns1, "InputA" )), "connect" );
	TRD_KNOB_PROBE( KnobA0, -0.0739 );
	TRD_KNOB_PROBE( KnobA0, 0.373 );

/* Now disconnect and try to use original knob. */
	CALL_CHECK( (DisconnectInstrumentParts( AddIns1, "InputA", 0 )), "disconnect" );
	TRD_KNOB_PROBE( KnobA1, 0.294 );
	TRD_KNOB_PROBE( KnobA1, 0.294 );
	TRD_KNOB_PROBE( KnobA1, -0.499 );

/* Connect two instruments again. Read knob that is passed through connection. */
	CALL_CHECK( (ConnectInstruments( AddIns0, "Output", AddIns1, "InputA" )), "connect" );
	TRD_KNOB_PROBE( KnobA0, 0.971 );
	TRD_KNOB_PROBE( KnobA0, 0.067 );

/* Now unload source instrument which should break connection and try to use original knob. */
	CALL_CHECK( (UnloadInstrument( AddIns0 )), "UnloadInstrument 0" );
	AddIns0 = 0;
	KnobA0 = 0;
	TRD_KNOB_PROBE( KnobA1, 0.1173 );
	TRD_KNOB_PROBE( KnobA1, 0.0882 );

clean:

	CALL_REPORT( (DeleteProbe( AddProbe )), "DeleteProbe" );
	if( AddIns0 )
	{
		CALL_REPORT( (DeleteKnob( KnobA0 )), "DeleteKnob" );
		CALL_REPORT( (UnloadInstrument( AddIns0 )), "UnloadInstrument" );
	}
	CALL_REPORT( (DeleteKnob( KnobA1 )), "DeleteKnob" );
	CALL_REPORT( (UnloadInstrument( AddIns1 )), "UnloadInstrument" );

PRT(("tRestoreDisconnect returns 0x%x\n", Result ));
	return Result;
}


/****************************************************************
** Create clock with a different rate.
****************************************************************/
static Err tChangeClockrate( void )
{
	Item    MyClock, SleepCue = 0;
	int32   SleepSignal;
	float32 globalRate, customRate;
	int32   duration, signals;
	int32   Result;
	AudioTime globalStartTime, globalStopTime, globalElapsedTime;
	AudioTime customStartTime, customStopTime, customElapsedTime;

#define NEW_DURATION  (300)
#define SLEEP_TIME    (175)
#define NEW_RATE  (44100.0/NEW_DURATION)
#define DEFAULT_RATE   (240.0)

	CALL_CHECK( (MyClock = CreateAudioClock(NULL)), "create custom clock");
	CALL_CHECK( (Result = GetAudioClockRate(MyClock, &globalRate)), "get custom clock rate");
	CALL_CHECK( (MatchFloats( globalRate, DEFAULT_RATE, 0.02)), "check default rate");

/* Change rate. */
	CALL_CHECK( (Result = SetAudioClockRate(MyClock, NEW_RATE)), "SET custom clock rate");
	CALL_CHECK( (Result = GetAudioClockRate(MyClock, &customRate)), "get custom clock rate");
	CALL_CHECK( (MatchFloats( customRate, NEW_RATE, 0.02)), "check default rate");
	CALL_CHECK( (duration = GetAudioClockDuration(MyClock)), "get custom clock duration");
	if( duration != NEW_DURATION )
	{
		ERR(("ERROR in tChangeClockrate: duration = %d, != %d\n", duration, NEW_DURATION ));
	}

/* Create Cue for sleeping. */
	CALL_CHECK( (SleepCue = CreateCue( NULL )), "create sleep Cue");
	CALL_CHECK( (SleepSignal = GetCueSignal( SleepCue )), "get signal from Cue");

/* Get start times. */
	CALL_CHECK( (Result = ReadAudioClock(AF_GLOBAL_CLOCK, &globalStartTime)), "read custom clock");
	CALL_CHECK( (Result = ReadAudioClock(MyClock, &customStartTime)), "read custom clock");

/* Sleep for a while under new rate. */
	CALL_CHECK( (Result = SignalAtAudioTime(MyClock, SleepCue, customStartTime + SLEEP_TIME)), "set signal");
	signals = WaitSignal( SleepSignal );
	if( (signals & SleepSignal) == 0 )
	{
		ERR(("ERROR in tChangeClockrate: got wrong signal = 0x%x not 0x%x\n", signals, SleepSignal));
	}

/* check times. */
	CALL_CHECK( (Result = ReadAudioClock(AF_GLOBAL_CLOCK, &globalStopTime)), "read custom clock");
	CALL_CHECK( (Result = ReadAudioClock(MyClock, &customStopTime)), "read custom clock");
	globalElapsedTime = globalStopTime - globalStartTime;
	customElapsedTime = customStopTime - customStartTime;
	CALL_CHECK( (MatchFloats( customElapsedTime/customRate, globalElapsedTime/globalRate, 0.02)), "check sleep time");

/* Make sure cue triggered when Clock deleted. */
	CALL_CHECK( (Result = SignalAtAudioTime(MyClock, SleepCue, customStopTime + 100000)), "set signal for far future");
	CALL_CHECK( (Result = DeleteAudioClock(MyClock)), "delete clock");

	if( GetCurrentSignals() & SleepSignal )
	{
		WaitSignal( SleepSignal );
	}
	else
	{
		ERR(("ERROR in tChangeClockrate: cue not triggered when clock deleted!\n" ));
	}

#undef NEW_RATE

clean:
	DeleteCue(SleepCue);
	DeleteAudioClock(MyClock);

	return Result;
}

/****************************************************************
** Sleep on multiple Cues
****************************************************************/
static Err tMultipleCues( void )
{
#define NUM_CUES   (10)
#define DELAY_TOLERANCE   (2)
	int32 RequestedDelays[NUM_CUES] = { 55, 197, 47, 56, 197, 64, 359, 111, 293, 52 };
#define TIME_SCALAR  (1)
#define REQUESTED_DELAY(n) (RequestedDelays[(n)]*TIME_SCALAR)
	int32 MeasuredDelays[NUM_CUES];
	Item  MyCues[NUM_CUES];
	int32 MySignals[NUM_CUES], i;
	int32 SignalMask = 0, Signals;
	uint32 StartTime, ThisDelay;
	int32 Result, NumErrs;

/* Create several Cues. */
	for( i=0; i<NUM_CUES; i++ )
	{
		CALL_CHECK( (MyCues[i] = CreateCue( NULL )), "create another Cue");
		PRT(("MyCues[%d] = 0x%x\n", i, MyCues[i]));
		CALL_CHECK( (MySignals[i] = GetCueSignal( MyCues[i] )), "get signal from Cue");
		SignalMask |= MySignals[i];
	}

	PRT(("tMultipleCues: Wait for a few seconds at most...\n"));

/* Schedule wakeup calls at various times in the future. */
	StartTime = GetAudioTime();
	for( i=0; i<NUM_CUES; i++ )
	{
		CALL_CHECK( (Result = SignalAtTime( MyCues[i], StartTime + REQUESTED_DELAY(i) )), "call SignalAtTime");
	}

	while( SignalMask != 0 )
	{
		Signals = WaitSignal( SignalMask );
		ThisDelay = GetAudioTime() - StartTime;
		DBUG(("tMultipleCues: Got 0x%08x at time 0x%x\n", Signals, GetAudioTime() ));
		SignalMask &= ~Signals;

/* Scan for hits. */
		for( i=0; i<NUM_CUES; i++ )
		{
			if( Signals & MySignals[i] )
			{
/* Record measured delay for this signal. */
				MeasuredDelays[i] = ThisDelay;
/* Remove signal as processed. */
				Signals &= ~MySignals[i];
			}
		}
		if( Signals != 0 )
		{
			ERR(("ERROR in tMultipleCues: received unexpected signal(s) = 0x%x\n", Signals ));
		}
	}
	PRT(("StartTime = 0x%x\n", StartTime));
	PRT(("tMultipleCues: ....Phew! We're back.\n"));

/* Check measured delays and compare against requested delays. */
	NumErrs = 0;
	for( i=0; i<NUM_CUES; i++ )
	{
		if( (MeasuredDelays[i] > (REQUESTED_DELAY(i) + DELAY_TOLERANCE) ) ||
		    (MeasuredDelays[i] < (REQUESTED_DELAY(i)) ) )
		{
			NumErrs++;
		}
	}

/* If any were bad, dump entire result for analysis. */
	if( NumErrs > 0 )
	{
		ERR(("ERROR in tMultipleCues: Requested versus Measured delays are:\n"));
		for( i=0; i<NUM_CUES; i++ )
		{
			PRT((" %4d => %4d ", REQUESTED_DELAY(i), MeasuredDelays[i] ));
			if( (MeasuredDelays[i] > (REQUESTED_DELAY(i) + DELAY_TOLERANCE) ) ||
		    	(MeasuredDelays[i] < (REQUESTED_DELAY(i) - DELAY_TOLERANCE) ) )
			{
				PRT((" <----BAD!!"));
			}
			PRT(("\n"));
		}
		Result = -1;
	}

clean:

	for( i=0; i<NUM_CUES; i++ )
	{
		DeleteCue( MyCues[i] );
	}
	return Result;
}

/* ================================================================ */
/* ============= ENVELOPES ======================================== */
/* ================================================================ */

/*************************************************************
** tPredictEnvelopeValue()
** Returns -1 if past end of envelope.
*************************************************************/
static int32 tPredictEnvelopeValue( EnvelopeSegment *EnvPoints, int32 NumPoints, float32 EnvTime, float32 *CurValuePtr )
{
	float32 Time1, Time2;
	float32 Value1, Value2;
	int32 i;
	Err Result = -1;
	float32 CurValue;

	*CurValuePtr = EnvPoints[NumPoints-1].envs_Value;
DBUG(("tPredictEnvelopeValue: EnvTime = %g\n", EnvTime ));
	Time1 = 0.0;
	CurValue = 0.0;
	for( i=0; i<NumPoints; i++ )
	{
		Time2 = Time1 + EnvPoints[i].envs_Duration;
		if( Time2 > EnvTime )
		{
/* Get points on either side of data. */
			Value1 = EnvPoints[i].envs_Value;
			Value2 = EnvPoints[i+1].envs_Value;
/* Interpolate current value from current time. */
			CurValue = Value1 + (((Value2 - Value1) *
			                     (EnvTime - Time1)) / (Time2 - Time1));
			Result = 0;
			break;
		}
		Time1 = Time2;
	}
	*CurValuePtr = CurValue;
	return Result;
}

/*************************************************************
** Test to see whether envelopes point is within range.
** Return index.
*************************************************************/
int32 tEnvelopeInRange( EnvelopeSegment *EnvPoints, int32 NumPoints, float32 EnvTime1, float32 EnvTime2 )
{
	int32 i;
	Err Result = -1;
	float32 CurTime;

	CurTime = 0.0;
	for( i=0; i<NumPoints; i++ )
	{
		if( ( CurTime >= EnvTime1 ) && ( CurTime <= EnvTime2 ) )
		{
			Result = i;
			break;
		}
		CurTime += EnvPoints[i].envs_Duration;
	}
	return Result;
}

#define ENV_TIME_SLOP (10)
#define ENV_MAX_ERRORS (10)
Err tTrackEnvelope( EnvelopeSegment *EnvPoints, int32 NumPoints,
	Item EnvProbe, AudioTime StartTime, float32 timeScale  )
{
	AudioTime Time1, Time2;
	float32 EnvTime1, EnvTime2;   /* EnvTime is in seconds. */
	float32 CurValue, Value1, Value2;
	float32 LoValue, HiValue;
	int32   ip;
	Item    SleepCue;
	int32   Result = 0, err = 0;
	int32   IfDone = FALSE;
	int32   NumErrors = 0;
	float32 fAudioRate;

	SleepCue = CreateCue( NULL );
	GetAudioClockRate( AF_GLOBAL_CLOCK, &fAudioRate );

#define AUDIOTIME_TO_ENVTIME(t) ((t-StartTime)/(fAudioRate * timeScale))

	while( !IfDone )
	{
		SleepUntilTime( SleepCue, GetAudioTime() + ENV_TIME_SLOP + 1 );

/* Measure time and value. */
		Time1 = GetAudioTime() - ENV_TIME_SLOP;
		CALL_CHECK( (ReadProbe( EnvProbe, &CurValue )), "check Probe" );
		Time2 = GetAudioTime() + ENV_TIME_SLOP;

/* Generate expected values. */
		EnvTime1 = AUDIOTIME_TO_ENVTIME(Time1);
		EnvTime2 = AUDIOTIME_TO_ENVTIME(Time2);
	DBUG(("AudioTimes are %d and %d\n", Time1, Time2));
	DBUG(("CurValue = %g, EnvTimes are %g and %g\n", CurValue, EnvTime1, EnvTime2));
		tPredictEnvelopeValue( EnvPoints, NumPoints, EnvTime1, &Value1 );
		IfDone = tPredictEnvelopeValue( EnvPoints, NumPoints, EnvTime2, &Value2 );

		LoValue = MIN(Value1, Value2);
		HiValue = MAX(Value1, Value2);

/* Do we span an inflection point? */
		if( (ip = tEnvelopeInRange( EnvPoints, NumPoints, EnvTime1, EnvTime2 )) >= 0)
		{
			LoValue = MIN(LoValue, EnvPoints[ip].envs_Value);
			HiValue = MAX(HiValue, EnvPoints[ip].envs_Value);
		}

/* Expand range so that we don't give false errors on flat parts of envelope. */
		LoValue *= 0.96;
		HiValue *= 1.04;

		if( (CurValue < LoValue) || ( CurValue > HiValue ) )
		{
			ERR(("tTrackEnvelope: Cur = %g, should be between %g and %g\n",
			     CurValue, LoValue, HiValue ));
			ERR(("         between env times %g and %g\n", EnvTime1, EnvTime2 ));
			err = -1;
			if( NumErrors++ > ENV_MAX_ERRORS )
			{
				ERR(("Maximum envelope error count exceeded. Stop test.\n"));
				goto clean;
			}
		}
	}
clean:
	DeleteCue( SleepCue );
	if( (Result == 0) && err ) Result = err;
	DBUG(("tTrackEnvelope: Result = 0x%x\n", Result ));
	return Result;
}

/*************************************************************
** Test to see whether envelopes behave correctly by tracking
** value using Probe.
*************************************************************/

Err tEnvelope1 ( int32 Divider, float32 timeScale, int32 ifLockTimeScale,
	int32 BaseNote, int32 NotesPerDouble, int32 Pitch, int32 startAt )
{
/* simple envelope */
	EnvelopeSegment EnvPoints[] =
	{
        	{ 0.000, 1.0 },
        	{ 0.999, 0.3 },
        	{ 0.500, 0.5 }, /* Flat part. */
        	{ 0.500, 0.1 },
        	{ 0.765, 0.8 },
		{ 0.000, 0.0 }
    	};

/* misc items and such */
	Item EnvIns = 0;
	Item EnvData = 0;
	Item EnvTemplate;
	Item EnvAtt = 0;
	Item EnvProbe = 0;
	AudioTime StartTime;
	int32 Result;
	float32 tscale = 1.0;
	uint32  flags;

	CALL_CHECK( (EnvTemplate = LoadInsTemplate("envelope.dsp", NULL)), "load env template");

	CALL_CHECK( (EnvIns = CreateInstrumentVA( EnvTemplate,
			AF_TAG_CALCRATE_DIVIDE, (void *) Divider, TAG_END )), "CreateInstrumentVA() 'envelope.dsp'" );

/* create envelope */
	flags = ifLockTimeScale ? AF_ENVF_LOCKTIMESCALE : 0;
	CALL_CHECK( (EnvData =
		CreateItemVA ( MKNODEID(AUDIONODE,AUDIO_ENVELOPE_NODE),
			AF_TAG_ADDRESS,        EnvPoints,
			AF_TAG_FRAMES,         sizeof EnvPoints / sizeof EnvPoints[0],
			AF_TAG_BASENOTE, BaseNote,
			AF_TAG_NOTESPEROCTAVE, NotesPerDouble,
			AF_TAG_SET_FLAGS, flags,
			TAG_END )), "create envelope" );

	CALL_CHECK( (EnvAtt = CreateAttachmentVA(EnvIns, EnvData, 
			AF_TAG_START_AT, startAt,
			TAG_END)), "CreateAttachment" );

	CALL_CHECK( (EnvProbe = CreateProbe(EnvIns, "Output", NULL)), "CreateProbe" );

/* Estimate final timescaling. */
	if( (Pitch >= 0) && (NotesPerDouble != 0) )
	{
		tscale = powf(2.0,((float32)(BaseNote - Pitch) / (float32)NotesPerDouble));
	}

	if( !ifLockTimeScale ) tscale *= timeScale;
	PRT(("   final tscale = %g\n", tscale));

/* Start playing */
	CALL_CHECK( (StartInstrumentVA ( EnvIns,
		AF_TAG_PITCH, Pitch,
		AF_TAG_TIME_SCALE_FP, ConvertFP_TagData(timeScale),
		TAG_END )), "start envelope" );
	StartTime = GetAudioTime();


/* Track contour of envelope. */
	CALL_CHECK( (tTrackEnvelope( &EnvPoints[startAt], ((sizeof EnvPoints / sizeof EnvPoints[0]) - startAt),
		EnvProbe, StartTime, tscale )), "Track Env1");
	StopInstrument( EnvIns, NULL );

clean:
	DeleteItem (EnvData);
	DeleteAttachment (EnvAtt);
	DeleteInstrument (EnvIns);
	UnloadInsTemplate( EnvTemplate );
	DeleteProbe(EnvProbe);
	DBUG(("tEnvelope1: Result = 0x%x\n", Result ));
	return Result;
}

/************************************************************/
Err tPlaySample( void )
{
	Item   SamplerIns = 0;
	Item   SampleItem;
	Item   Attachment = 0;
	int32  Result;
	Item   SleepCue = 0, AttCue = 0;
	int32  SleepSignal, AttSignal, Signals;
	int16 *Data;
	int32  Offset;

#define NUM_FRAMES   (34000)
#define NUM_BYTES    (NUM_FRAMES*(sizeof(int16)))

	Data = (int16 *) malloc( NUM_BYTES );
	if( Data == NULL ) return AF_ERR_NOMEM;

	CALL_CHECK( (SampleItem = CreateSampleVA( AF_TAG_ADDRESS, (TagData) Data,
			AF_TAG_FRAMES, (TagData) NUM_FRAMES, TAG_END)), "CreateSample");

/* Load Sampler instrument */
	CALL_CHECK( (SamplerIns = LoadInstrument("sampler_16_f1.dsp",  0, 100)), "LoadInstrument");

/* Create cues and get signals. */
	CALL_CHECK( (SleepCue = CreateCue( NULL )), "CreateCue");
	CALL_CHECK( (SleepSignal = GetCueSignal(SleepCue)), "GetCueSignal");
	CALL_CHECK( (AttCue = CreateCue( NULL )), "CreateCue");
	CALL_CHECK( (AttSignal = GetCueSignal(AttCue)), "GetCueSignal");

/* Attach the sample to the instrument for playback. */
	CALL_CHECK( (Attachment = CreateAttachment(SamplerIns, SampleItem, NULL)), "CreateAttachment");

/* Ask for signal when sample finishes playing. */
	CALL_CHECK( (Result = MonitorAttachment( Attachment, AttCue, CUE_AT_END )), "MonitorAttachment");

/* Set time out in case MonitorAttachment() fails. */
	CALL_CHECK( (Result = SignalAtAudioTime( AF_GLOBAL_CLOCK, SleepCue, GetAudioTime() + 5*240 )), "SignalAtTime");

/* Play sample at originally recorded sample rate. */
	CALL_CHECK( (Result = StartInstrumentVA (SamplerIns,
		AF_TAG_DETUNE_FP, ConvertFP_TagData(1.0),
		TAG_END)), "StartInstrumentVA");

/* Check progress of DMA. */
	CALL_CHECK( (Offset = WhereAttachment( Attachment )), "WhereAttachment");
	if( (Offset < 0) || (Offset > NUM_BYTES) )
	{
		ERR(("tPlaySample - WhereAttachment() returns offset out of range = 0x%x\n", Offset));
		Result = -1;
		goto clean;
	}

	CALL_CHECK( (Result = ReleaseInstrument(SamplerIns, NULL )), "ReleaseInstrument");

/* Wait for sample to finish. */
	Signals = WaitSignal( SleepSignal | AttSignal );
	if ( Signals & SleepSignal )
	{
		ERR(("tPlaySample - MonitorAttachment() timed out!\n"));
		Result = -1;
		goto clean;
	}

	CALL_CHECK( (Result = StopInstrument( SamplerIns, NULL )), "StopInstrument");

clean:
	if( Data ) free( Data );
	DeleteAttachment( Attachment );
	DeleteSample( SampleItem );
	UnloadInstrument( SamplerIns );
	DeleteCue( SleepCue );
	DeleteCue( AttCue );
	return((int) Result);
}

/************************************************************/
Item  tMakeMultiSample( uint16 value, int32 lowPitch, int32 highPitch,
	int32 lowVel, int32 highVel )
{
	Item   SampleItem;
	int32  Result;
	int16 *Data;
	int32  i;

#define NUM_MULTI_FRAMES   (32)
#define NUM_MULTI_BYTES    (NUM_FRAMES*(sizeof(int16)))

	Data = (int16 *) AllocMem( NUM_BYTES, MEMTYPE_TRACKSIZE );
	if( Data == NULL ) return AF_ERR_NOMEM;

	for( i=0; i<NUM_MULTI_FRAMES; i++ ) Data[i] = value;

	CALL_CHECK( (SampleItem = CreateSampleVA(
			AF_TAG_AUTO_FREE_DATA, (TagData) TRUE,
			AF_TAG_ADDRESS, (TagData) Data,
			AF_TAG_FRAMES, (TagData) NUM_FRAMES,
			AF_TAG_SUSTAINBEGIN, (TagData) 0,
			AF_TAG_SUSTAINEND, (TagData) NUM_MULTI_FRAMES,
			AF_TAG_LOWVELOCITY, (TagData) lowVel,
			AF_TAG_HIGHVELOCITY, (TagData) highVel,
			AF_TAG_LOWNOTE, (TagData) lowPitch,
			AF_TAG_HIGHNOTE, (TagData) highPitch,
			TAG_END)), "CreateSampleVA");
clean:
	return SampleItem;
}

/************************************************************/
Err tCheckSelection( Item SamplerIns, int32 pitch, int32 vel, int32 value )
{
	int32  Result;
	TagArg tags[5];
	Item   SleepCue;
	Item   OutProbe = 0;
	int32  i;
	int32  gotval;
	float32  fgotval;
	
/* Create cue. */
	CALL_CHECK( (SleepCue = CreateCue( NULL )), "CreateCue");

/* Connect Probe to output of sampler. */
	CALL_CHECK( (OutProbe = CreateProbeVA(SamplerIns, "Output",
		AF_TAG_TYPE, AF_SIGNAL_TYPE_WHOLE_NUMBER,
		TAG_END )), "CreateProbeVA");

	i=0;
	if( pitch > 0 )
	{
		tags[i].ta_Tag = AF_TAG_PITCH;
		tags[i++].ta_Arg = (TagData) pitch;
	}
	if( vel > 0 )
	{
		tags[i].ta_Tag = AF_TAG_VELOCITY;
		tags[i++].ta_Arg = (TagData) vel;
	}
	tags[i].ta_Tag = TAG_END;

/* Play sample. */
	CALL_CHECK( (Result = StartInstrument (SamplerIns, tags)), "StartInstrument");

/* Sleep for a while to let output settle. */
	CALL_CHECK( (Result = SleepUntilTime(SleepCue, GetAudioTime()+2)), "SleepUntilTime");

/* Read current output of instrument. */
	CALL_CHECK( (Result = ReadProbe(OutProbe, &fgotval)), "ReadProbe");
	
	CALL_CHECK( (Result = StopInstrument( SamplerIns, NULL )), "StopInstrument");

	gotval = (int32) fgotval;
	if( gotval != value )
	{
		ERR(("ERROR - tMultiSample - expected 0x%x, got 0x%x\n", value, gotval ));
		ERR(("        pitch = %d, vel = %d\n", pitch, vel ));
		Result = -1;
		goto clean;
	}
#if 0
	else
	{
		PRT(("MultiSample succeeded.  Got 0x%x\n", value, gotval ));
		PRT(("        pitch = %d, vel = %d\n", pitch, vel ));
	}
#endif

clean:
	DeleteProbe( OutProbe );
	DeleteCue( SleepCue );
	return Result;
}

/************************************************************
** Test MultiSample selection by attaching multiple DC samples
** with various ranges.  Play using various pitches and velocities
** and verify by detecting DC level of sample.
*/
Err tMultiSample( void )
{
/* Select multisample.  Identify sample by DC value. */
	Item   SamplerIns = 0;
	Item   SampleItems[9];
	int32  i;
	Item   Attachment = 0;
	int32  Result;

#define MULTI_VALUE(p,v) ((p) + ((v)<4))
#define INDEX_TO_LOW(p)  ((p)*10 + 20)

/* Make 3 samples that differ by velocity range. */
	CALL_CHECK( (SampleItems[0] = tMakeMultiSample( 0x0000, 10, 29, 10, 14 )), "tMakeMultiSample");
	CALL_CHECK( (SampleItems[1] = tMakeMultiSample( 0x0001, 10, 29, 15, 19 )), "tMakeMultiSample");
	CALL_CHECK( (SampleItems[2] = tMakeMultiSample( 0x0002, 10, 29, 20, 24 )), "tMakeMultiSample");

/* Make 3 samples that differ by pitch range. */
	CALL_CHECK( (SampleItems[3] = tMakeMultiSample( 0x0010, 30, 34, 30, 49 )), "tMakeMultiSample");
	CALL_CHECK( (SampleItems[4] = tMakeMultiSample( 0x0011, 35, 39, 30, 49 )), "tMakeMultiSample");
	CALL_CHECK( (SampleItems[5] = tMakeMultiSample( 0x0012, 40, 44, 30, 49 )), "tMakeMultiSample");
			
/* Make 3 samples that differ by pitch and velocity range. */
	CALL_CHECK( (SampleItems[6] = tMakeMultiSample( 0x0021, 50, 54, 50, 54 )), "tMakeMultiSample");
	CALL_CHECK( (SampleItems[7] = tMakeMultiSample( 0x0022, 55, 59, 55, 59 )), "tMakeMultiSample");
	CALL_CHECK( (SampleItems[8] = tMakeMultiSample( 0x0023, 60, 64, 60, 64 )), "tMakeMultiSample");

/* Load Sampler instrument */
	CALL_CHECK( (SamplerIns = LoadInstrument("sampler_raw_f1.dsp",  0, 100 )), "LoadInstrument");

/* Attach all the samples to the instrument for playback. */
	for( i=0; i<9; i++)
	{
		CALL_CHECK( (Attachment = CreateAttachment(SamplerIns, SampleItems[i], NULL)), "CreateAttachment");
	}

/* Miscellaneous selections. */
	CALL_CHECK( (Result = tCheckSelection(SamplerIns, 20, 17, 0x0001)), "tCheckSelection");
	CALL_CHECK( (Result = tCheckSelection(SamplerIns, 20, 12, 0x0000)), "tCheckSelection");
	CALL_CHECK( (Result = tCheckSelection(SamplerIns, 20, 23, 0x0002)), "tCheckSelection");
	CALL_CHECK( (Result = tCheckSelection(SamplerIns, 56, 57, 0x0022)), "tCheckSelection");

/* Test edge cases. */
	CALL_CHECK( (Result = tCheckSelection(SamplerIns, 50, 50, 0x0021)), "tCheckSelection");
	CALL_CHECK( (Result = tCheckSelection(SamplerIns, 64, 64, 0x0023)), "tCheckSelection");
	CALL_CHECK( (Result = tCheckSelection(SamplerIns, 55, 59, 0x0022)), "tCheckSelection");

/* Test pitch without velocity. */
	CALL_CHECK( (Result = tCheckSelection(SamplerIns, 32, -1, 0x0010)), "tCheckSelection");
	CALL_CHECK( (Result = tCheckSelection(SamplerIns, 43, -1, 0x0012)), "tCheckSelection");
	CALL_CHECK( (Result = tCheckSelection(SamplerIns, 37, -1, 0x0011)), "tCheckSelection");

/* Test velocity without pitch. */
	CALL_CHECK( (Result = tCheckSelection(SamplerIns, -1, 17, 0x0001)), "tCheckSelection");
	CALL_CHECK( (Result = tCheckSelection(SamplerIns, -1, 12, 0x0000)), "tCheckSelection");
	CALL_CHECK( (Result = tCheckSelection(SamplerIns, -1, 24, 0x0002)), "tCheckSelection");


clean:

	DeleteAttachment( Attachment );
	for( i=0; i<9; i++) DeleteSample( SampleItems[i] );
	UnloadInstrument( SamplerIns );
	return((int) Result);
}

/*************************************************************
** Test to see whether FIFO is getting cleared after previous
** playing.  It should get reset to zero.
*************************************************************/
Err tClearFIFO( void )
{
	Item   SamplerIns;
	Item   SampleItem;
	Item   Attachment = 0;
	Item   OutProbe = 0;
	int32  Result;
	Item   SleepCue = 0;
	int32  gotval;
	float32  fgotval;

/* Load 16_V1 Sampler instrument ----------------------------*/
	CALL_CHECK( (SamplerIns = LoadInstrument("sampler_16_v1.dsp",  0, 100)), "LoadInstrument");

/* Create Sample with known value. */
	CALL_CHECK( (SampleItem = tMakeMultiSample( 0x2345, 0, 127, 0, 127 )), "tMakeMultiSample");

/* Create cues and get signals. */
	CALL_CHECK( (SleepCue = CreateCue( NULL )), "CreateCue");

/* Attach the sample to the instrument for playback. */
	CALL_CHECK( (Attachment = CreateAttachment(SamplerIns, SampleItem, NULL)), "CreateAttachment");

/* Play sample at originally recorded sample rate. */
	CALL_CHECK( (Result = StartInstrumentVA (SamplerIns,
		AF_TAG_DETUNE_FP, ConvertFP_TagData(1.0),
		TAG_END)), "StartInstrumentVA");
/* Sleep for a while to let output settle. */
	CALL_CHECK( (Result = SleepUntilTime(SleepCue, GetAudioTime()+2)), "SleepUntilTime");

	CALL_CHECK( (Result = StopInstrument( SamplerIns, NULL )), "StopInstrument");

	DeleteSample( SampleItem );
	UnloadInstrument( SamplerIns );

/* Load RAW_F1 Sampler instrument ----------------------------*/
	CALL_CHECK( (SamplerIns = LoadInstrument("sampler_raw_f1.dsp",  0, 100)), "LoadInstrument");

/* Create Sample with known value. */
	CALL_CHECK( (SampleItem = tMakeMultiSample( 0xABCD, 0, 127, 0, 127 )), "tMakeMultiSample");

/* Connect Probe to output of sampler. */
	CALL_CHECK( (OutProbe = CreateProbeVA(SamplerIns, "Output",
		AF_TAG_TYPE, AF_SIGNAL_TYPE_WHOLE_NUMBER,
		TAG_END )), "CreateProbeVA");

/* Attach the sample with NOAUTOSTART. */
	CALL_CHECK( (Attachment = CreateAttachmentVA(SamplerIns, SampleItem,
		AF_TAG_SET_FLAGS, AF_ATTF_NOAUTOSTART,
		TAG_END)), "CreateAttachment");

/* Play instrument but now sample. */
	CALL_CHECK( (Result = StartInstrument (SamplerIns, NULL)), "StartInstrument");

/* Sleep for a while to let output settle. */
	CALL_CHECK( (Result = SleepUntilTime(SleepCue, GetAudioTime()+2)), "SleepUntilTime");

/* Read current output of instrument. */
	CALL_CHECK( (Result = ReadProbe(OutProbe, &fgotval)), "ReadProbe");
	
	CALL_CHECK( (Result = StopInstrument( SamplerIns, NULL )), "StopInstrument");

	gotval = (int32) fgotval;
	if( gotval != 0 )
	{
		ERR(("ERROR - tClearFIFO - expected 0, got 0x%x\n",  gotval ));
		Result = -1;
		goto clean;
	}
#if 0
	else
	{
		PRT(("tClearFIFO succeeded.  Got 0x%x\n",  gotval ));
	}
#endif

	DeleteSample( SampleItem );
	UnloadInstrument( SamplerIns );
clean:
	DeleteAttachment( Attachment );
	DeleteCue( SleepCue );
	DeleteProbe( OutProbe );
	return((int) Result);
}

/**********************************************************************
** Capture output of instrument and do CheckSum
**********************************************************************/
int32 TestMixer( int32 numInputs, int32 numOutputs, uint32 flags )
{
#define TMIX_MAX_INPUTS   (16)
/* Declare local variables */
	const MixerSpec mixerSpec = MakeMixerSpec(numInputs, numOutputs, flags);
	Item    MixerTmp;
	Item    MixerIns;
	Item    AdderTmp=0;
	Item    Adders[TMIX_MAX_INPUTS];
	Item    AdderKnobs[TMIX_MAX_INPUTS];
	Item    GainKnob;
	Item    AmpKnob;
	Item    OutProbe;
	int32   inputIndex, outputIndex, part;
	int32   Result;

/* Create mixer */
	CALL_CHECK( (MixerTmp = CreateMixerTemplate( mixerSpec, NULL )), "CreateMixerTemplate");

/* Make instrument from template. */
	CALL_CHECK( (MixerIns = CreateInstrument( MixerTmp, NULL )), "CreateInstrument Mixer");
	CALL_CHECK( (Result = StartInstrument( MixerIns, NULL )), "StartInstrument");
	CALL_CHECK( (OutProbe = CreateProbe(MixerIns, "Output", NULL)), "CreateProbe" );
	CALL_CHECK( (GainKnob = CreateKnob( MixerIns, "Gain", NULL )), "CreateKnob");

/* Set Amplitude knob if present. */
	AmpKnob = CreateKnob( MixerIns, "Amplitude", NULL );
	if( AmpKnob > 0 ) SetKnob( AmpKnob, 1.0)

/* Load add instruments. */
	CALL_CHECK( (AdderTmp = LoadInsTemplate( "add.dsp", 0 )), "LoadInsTemplate add.dsp");
	for( inputIndex=0; inputIndex<numInputs; inputIndex++ )
	{
		CALL_CHECK( (Adders[inputIndex] = CreateInstrument( AdderTmp, NULL )), "CreateInstrument");
		CALL_CHECK( (AdderKnobs[inputIndex] = CreateKnob( Adders[inputIndex], "InputA", NULL )), "CreateKnob");
		CALL_CHECK( (Result = StartInstrument( Adders[inputIndex], NULL )), "StartInstrument");
		CALL_CHECK( (Result = ConnectInstrumentParts( Adders[inputIndex], "Output", 0, MixerIns, "Input", inputIndex )), "ConnectInstrumentParts");
	}

/* Scan through various combinations. */
	for( inputIndex=0; inputIndex<numInputs; inputIndex++ )
	{
		CALL_CHECK( (Result = SetKnob( AdderKnobs[inputIndex], 1.0)), "SetKnob");

		for( outputIndex=0; outputIndex<numOutputs; outputIndex++ )
		{
			float32 Gain = ((float32)(outputIndex + 1) / (float32)(numOutputs + 1));
			part = CalcMixerGainPart( mixerSpec, inputIndex, outputIndex );
			CALL_CHECK( (Result = SetKnobPart( GainKnob, part, Gain)), "SetKnob");
		}
		CALL_CHECK( (Result = WaitAudioFrames( 8 )), "WaitAudioFrames");
		for( outputIndex=0; outputIndex<numOutputs; outputIndex++ )
		{
			float32 Val;
			float32 Gain = ((float32)(outputIndex + 1) / (float32)(numOutputs + 1));
			CALL_CHECK( (Result = ReadProbePart( OutProbe, outputIndex, &Val)), "ReadProbePart");

			if( !MatchFloats( Gain, Val, 0.01 ) )
			{
				ERR(( "Mixer Out does not match Gain. Got %g , expected %g\n", Val, Gain ));
				ERR(("   i,o = %d, %d, g,v = %g, %g\n", inputIndex, outputIndex, Gain, Val ));
				Result = -1;
				goto clean;
			}
		}
		CALL_CHECK( (Result = SetKnob( AdderKnobs[inputIndex], 0.0)), "SetKnob");
	}

clean:
	UnloadInsTemplate( MixerTmp );
	UnloadInsTemplate( AdderTmp );
	return Result;
}

/************************************************************/
Err tMixer( void )
{
	int32 Result=0, ni, no;

	for( ni=1; ni<6; ni++ )
	{
		for( no=1; no<4; no++ )
		{
			DBUG(("--- test mixer %d by %d ----\n", ni,no ));
			CALL_CHECK( (Result = TestMixer( ni, no, AF_F_MIXER_WITH_AMPLITUDE )), "TestMixer Amp" );
			CALL_CHECK( (Result = TestMixer( ni, no, 0 )), "TestMixer" );
		}
	}
clean:
	return Result;
}

/************************************************************/
int main(int argc, char *argv[])
{
	int32 Result;

#ifdef MEMDEBUG
	CALL_CHECK( (CreateMemDebug ( NULL )), "CreateMemDebug");

	CALL_CHECK( (ControlMemDebug ( MEMDEBUGF_ALLOC_PATTERNS |
	/**/                           MEMDEBUGF_FREE_PATTERNS |
	/**/                           MEMDEBUGF_PAD_COOKIES |
	/**/                           MEMDEBUGF_CHECK_ALLOC_FAILURES |
	/**/                           MEMDEBUGF_KEEP_TASK_DATA)), "ControlMemDebug");
#endif
	PRT(("Begin %s\n", argv[0] ));
	TOUCH(argc); /* Eliminate anal compiler warning. */

/* Initialize audio, return if error. */
	Result = OpenAudioFolio();
	if (Result < 0)
	{
		PrintError(0,"Audio Folio could not be opened.",0,Result);
		return(-1);
	}

#if TEST_LOAD_ALL
	PASS_FAIL( (tAllInstruments()), "Load All Instruments" );
	PASS_FAIL( (tAllIllegalInstruments()), "Load All Illegal Instruments" );
	#ifdef MEMDEBUG
		DumpMemDebugVA (
			DUMPMEMDEBUG_TAG_SUPER, TRUE,
			TAG_END);
	#endif
#else
	PRT(("Skip 'Load All Instruments' test.\n"));
#endif

#if TEST_KNOBS
	PASS_FAIL( (tRestoreDisconnect()), "Restore knob on disconnect." );
	#ifdef MEMDEBUG
		DumpMemDebugVA (
			DUMPMEMDEBUG_TAG_SUPER, TRUE,
			TAG_END);
	#endif
#else
	PRT(("Skip knob test.\n"));
#endif

#if TEST_TIMER
	PASS_FAIL( (tChangeClockrate()), "Test custom clock." );
	#ifdef MEMDEBUG
		DumpMemDebugVA (
			DUMPMEMDEBUG_TAG_SUPER, TRUE,
			TAG_END);
	#endif
	PASS_FAIL( (tChangeClockrate()), "Test custom clock." );
	#ifdef MEMDEBUG
		DumpMemDebugVA (
			DUMPMEMDEBUG_TAG_SUPER, TRUE,
			TAG_END);
	#endif
	PASS_FAIL( (tMultipleCues()), "Test multiple Cue delays." );
	#ifdef MEMDEBUG
		DumpMemDebugVA (
			DUMPMEMDEBUG_TAG_SUPER, TRUE,
			TAG_END);
	#endif
#else
	PRT(("Skip timer test.\n"));
#endif

#if TEST_SAMPLE
	PASS_FAIL( (tPlaySample()), "Test sample playback." );
	#ifdef MEMDEBUG
		DumpMemDebugVA (
			DUMPMEMDEBUG_TAG_SUPER, TRUE,
			TAG_END);
	#endif
	PASS_FAIL( (tClearFIFO()), "Test clearing of FIFO." );
	#ifdef MEMDEBUG
		DumpMemDebugVA (
			DUMPMEMDEBUG_TAG_SUPER, TRUE,
			TAG_END);
	#endif
#else
	PRT(("Skip sample test.\n"));
#endif

#if TEST_MULTISAMPLE
	PASS_FAIL( (tMultiSample()), "Test multi sample selection." );
	#ifdef MEMDEBUG
		DumpMemDebugVA (
			DUMPMEMDEBUG_TAG_SUPER, TRUE,
			TAG_END);
	#endif
#else
	PRT(("Skip multi sample test.\n"));
#endif

#if TEST_MIXER
	PASS_FAIL( (tMixer()), "Test mixer template." );
	#ifdef MEMDEBUG
		DumpMemDebugVA (
			DUMPMEMDEBUG_TAG_SUPER, TRUE,
			TAG_END);
	#endif
#else
	PRT(("Skip mixer test.\n"));
#endif

#if TEST_ENVELOPES
	PASS_FAIL( (tEnvelope1(1, 1.0, 0, 60, 0, 60, 0)), "Track simple envelope." );
	#ifdef MEMDEBUG
		DumpMemDebugVA (
			DUMPMEMDEBUG_TAG_SUPER, TRUE,
			TAG_END);
	#endif

	PASS_FAIL( (tEnvelope1(2, 1.0, 1, 60, 0, 60, 2)), "Track simple envelope with START_AT = 2" );
	#ifdef MEMDEBUG
		DumpMemDebugVA (
			DUMPMEMDEBUG_TAG_SUPER, TRUE,
			TAG_END);
	#endif

	PASS_FAIL( (tEnvelope1(2, 1.0, 1, 60, 0, 60, 0)), "Track simple envelope at 1/2 rate. LOCKTIMESCALE" );
	#ifdef MEMDEBUG
		DumpMemDebugVA (
			DUMPMEMDEBUG_TAG_SUPER, TRUE,
			TAG_END);
	#endif

	PASS_FAIL( (tEnvelope1(8, 1.0, 0, 60, 0, 60, 0)), "Track simple envelope at 1/8 rate." );
	#ifdef MEMDEBUG
		DumpMemDebugVA (
			DUMPMEMDEBUG_TAG_SUPER, TRUE,
			TAG_END);
	#endif
	PASS_FAIL( (tEnvelope1(1, 1.392, 0, 60, 0, 60, 0)), "Track simple envelope with timeScale of 1.392." );
	#ifdef MEMDEBUG
		DumpMemDebugVA (
			DUMPMEMDEBUG_TAG_SUPER, TRUE,
			TAG_END);
	#endif
	PASS_FAIL( (tEnvelope1(1, 1.392, 1, 60, 0, 60, 0)), "Track simple envelope with timeScale of 1.392 BUT LOCKTIMESCALE." );
	#ifdef MEMDEBUG
		DumpMemDebugVA (
			DUMPMEMDEBUG_TAG_SUPER, TRUE,
			TAG_END);
	#endif
	PASS_FAIL( (tEnvelope1(2, 0.755, 0, 60, 0, 60, 0)), "Track simple envelope at half rate with timeScale of 0.755." );
	#ifdef MEMDEBUG
		DumpMemDebugVA (
			DUMPMEMDEBUG_TAG_SUPER, TRUE,
			TAG_END);
	#endif
	PASS_FAIL( (tEnvelope1(2, 1.0, 0, 60, 12, 72, 0)), "Track simple envelope with pitch 60, 12, 72 with timeScale of 1.0." );
	#ifdef MEMDEBUG
		DumpMemDebugVA (
			DUMPMEMDEBUG_TAG_SUPER, TRUE,
			TAG_END);
	#endif
	PASS_FAIL( (tEnvelope1(2, 0.755, 0, 50, 10, 40, 0)), "Track simple envelope with pitch 50, 10, 40 with timeScale of 0.755." );
	#ifdef MEMDEBUG
		DumpMemDebugVA (
			DUMPMEMDEBUG_TAG_SUPER, TRUE,
			TAG_END);
	#endif
	PASS_FAIL( (tEnvelope1(2, 0.577, 1, 50, -10, 66, 0)), "Track simple envelope with pitch 50, -10, 66 with timeScale of 0.577." );
	#ifdef MEMDEBUG
		DumpMemDebugVA (
			DUMPMEMDEBUG_TAG_SUPER, TRUE,
			TAG_END);
	#endif

#else
	PRT(("Skip envelope test.\n"));
#endif

#ifdef MEMDEBUG
clean:
#endif

	CloseAudioFolio();
#ifdef MEMDEBUG
	DumpMemDebugVA (
		DUMPMEMDEBUG_TAG_SUPER, TRUE,
		TAG_END);
	DeleteMemDebug();
#endif
	PRT(( "%s finished.\n", argv[0] ));
	return((int) Result);
}
