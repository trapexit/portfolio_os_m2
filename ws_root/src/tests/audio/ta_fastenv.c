
/******************************************************************************
**
**  @(#) ta_fastenv.c 96/08/27 1.2
**
******************************************************************************/

/*
** ta_fastenv.c - test maximum value for envelope with fast attack.
** Enter attack time in milliseconds.
*
** Author: Phil Burk
** Copyright 3DO 1996-
**
*/

#include <audio/audio.h>
#include <audio/parse_aiff.h>
#include <kernel/mem.h>         /* memdebug */
#include <kernel/operror.h>
#include <stdio.h>
#include <misc/event.h>
#include <stdlib.h>             /* malloc()*/
#include <math.h>               /* powf()*/

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
			TOUCH(Result); \
			PrintError(0,"\\ERROR in",msg,Result); \
		} else { \
			PRT(("SUCCESS in %s\n", msg )); \
		} \
	} while(0)

/*************************************************************
** Test to see whether envelopes with fast attack reach
** maximum value using probe.
*************************************************************/

Err tFastEnvelope ( int32 Divider, float32 attackTime )
{
/* simple envelope */
	EnvelopeSegment EnvPoints[] =
	{
        	{ 0.000, (0.0 /* attackTime */) },
        	{ 1.0, 0.5 },
        	{ 0.000, 0.0 }
    	};

/* misc items and such */
	Item EnvIns = 0;
	Item EnvData = 0;
	Item MaxIns = 0;
	float32  fgotval;
	Item EnvAtt = 0;
	Item   SleepCue;
	Item EnvProbe = 0;
	int32 Result;
	int32 waitTicks;

	EnvPoints[0].envs_Duration = attackTime;

/* Create cue. */
	CALL_CHECK( (SleepCue = CreateCue( NULL )), "CreateCue");

	CALL_CHECK( (MaxIns = LoadInstrument("maximum.dsp", 1, 20)), "load maximum.dsp ins");
	CALL_CHECK( (EnvIns = LoadInstrument("envelope.dsp", Divider, 30)), "load envelope.dsp ins");

/* create envelope */
	CALL_CHECK( (EnvData =
		CreateItemVA ( MKNODEID(AUDIONODE,AUDIO_ENVELOPE_NODE),
			AF_TAG_ADDRESS,        EnvPoints,
			AF_TAG_FRAMES,         sizeof EnvPoints / sizeof EnvPoints[0],
			TAG_END )), "create envelope" );

	CALL_CHECK( (EnvAtt = CreateAttachment (EnvIns, EnvData, NULL)), "CreateAttachment" );

	CALL_CHECK( (Result = ConnectInstruments(EnvIns, "Output", MaxIns, "InputA")), "ConnectInstruments e-m" );
/* Connect to self for historical maximum. */
	CALL_CHECK( (Result = ConnectInstruments(MaxIns, "Output", MaxIns, "InputB")), "ConnectInstruments m-m" );

	CALL_CHECK( (EnvProbe = CreateProbe(MaxIns, "Output", NULL)), "CreateProbe" );

/* Start playing */
	CALL_CHECK( (StartInstrument ( MaxIns, NULL )), "start max" );
	CALL_CHECK( (StartInstrument ( EnvIns, NULL )), "start envelope" );

/* Sleep for a while to let envelope peak. */
	waitTicks = (attackTime + 0.25) * 240.0;
	CALL_CHECK( (Result = SleepUntilTime(SleepCue, GetAudioTime()+ waitTicks)), "SleepUntilTime");

/* Read current output of instrument. */
	CALL_CHECK( (Result = ReadProbe(EnvProbe, &fgotval)), "ReadProbe");

	PRT(("Peak of envelope = %g. Should be about 0.9999\n", fgotval ));

clean:
	DeleteItem(EnvData);
	DeleteAttachment(EnvAtt);
	UnloadInstrument(EnvIns);
	UnloadInstrument(MaxIns);
	DeleteCue( SleepCue );
	DeleteProbe(EnvProbe);
	DBUG(("tEnvelope1: Result = 0x%x\n", Result ));
	return Result;
}

/*******************************************************************/
int main( int32 argc, char *argv[])
{
	int32 Result;
	ControlPadEventData cped;
	float32  attackTime;

	TOUCH(argc);

	PRT(("%s\n", argv[0]));

	PRT(("%s {attack_in_msec}\n", argv[0]));

	attackTime = (argc > 1) ? (atoi(argv[1])/1000.0) : (1.8/240.0);

	PRT(("Attack Time = %g msec\n", attackTime*1000.0 ));

/* Initialize audio, return if error. */
	if ((Result = OpenAudioFolio()) < 0)
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(Result);
	}

/* Initialize the EventBroker. */
	Result = InitEventUtility(1, 0, TRUE);
	if (Result < 0)
	{
		PrintError(0,"InitEventUtility",0,Result);
		return Result;
	}

	PRT(("Hit A to trigger envelope. X to quit.\n"));
	do
	{
		Result = GetControlPad (1, TRUE, &cped);
		if (Result < 0)
		{
			PrintError(0,"read control pad in","TestAudioInput",Result);
			return Result;
		}
		if( cped.cped_ButtonBits & ControlA )
		{
			if( (Result = tFastEnvelope( 1, attackTime )) < 0 ) break;
		}
	} while( (cped.cped_ButtonBits & ControlX) == 0);

	KillEventUtility();
	CloseAudioFolio();

	PRT(("%s all done.\n", argv[0]));

	return Result;
}
