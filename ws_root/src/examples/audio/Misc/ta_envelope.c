
/******************************************************************************
**
**  @(#) ta_envelope.c 96/08/27 1.24
**
******************************************************************************/

/**
|||	AUTODOC -public -class examples -group Audio -name ta_envelope
|||	Tests various envelope options by passing test index.
|||
|||	  Format
|||
|||	    ta_envelope [test code]
|||
|||	  Description
|||
|||	    Demonstrates creating, attaching, and modifying two envelopes which are
|||	    attached to triangle instruments.
|||
|||	  Arguments
|||
|||	     [test code]
|||	        Integer from 1 to 13, indicating the number of the test. See the source
|||	        code for what each test actually does. Defaults to 1.
|||
|||	  Controls
|||
|||	    A
|||	        Starts and releases voice A.
|||
|||	    B
|||	        Starts and releases voice B.
|||
|||	    C
|||	        Toggles the states of voices A and B.
|||
|||	    Up/Down
|||	        Change the time scaling of the envelope.
|||
|||	  Associated Files
|||
|||	    ta_envelope.c
|||
|||	  Location
|||
|||	    Examples/Audio/Misc
|||
**/

#include <audio/audio.h>
#include <audio/parse_aiff.h>
#include <kernel/task.h>
#include <kernel/operror.h>
#include <kernel/types.h>
#include <misc/event.h>
#include <stdio.h>
#include <stdlib.h>

#define MODULATE_FREQ

#define ENVINSNAME "envelope.dsp"
#define OSCINSNAME "sampler_16_v1.dsp"

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	DBUG(("%s, val = 0x%x\n", name, val)); \
	if (val < 0) \
	{ \
		Result = val; \
		PrintError(0,"\\failure in",name,val); \
		goto cleanup; \
	}

typedef struct EnvVoice
{
/* Declare local variables */
	Item  envv_OscTmp;
	Item  envv_OscIns;
	Item  envv_Sample;
	Item  envv_Att;
	Item  envv_OutputIns;
	Item  envv_EnvIns;
	Item  envv_EnvCue;
	Item  envv_Envelope;
	Item  envv_EnvAttachment;
	int32 envv_EnvCueSignal;
} EnvVoice;

#define NUM_ENV_VOICES (2)

EnvVoice  EnvVoices[NUM_ENV_VOICES];

/* Variables local to this file. */
static	float32 TimeScale = 1.0;

/* Times are durations of each segment. */
static	EnvelopeSegment	EnvPoints1[] =
	{   /*    Value,  Duration */
		{  0.00, 1.0 }, /* 0 */
	 	{  0.90, 0.3 }, /* 1 */
		{  0.80, 0.3 }, /* 2 */
		{  0.04, 0.4 }, /* 3 */
		{  0.95, 0.5 }, /* 4 */
		{  0.20, 0.5 }, /* 5 */
		{  0.95, 1.0 }, /* 6 */
		{  0.80, 6.0 }, /* 7 */
		{  0.00, 0.0 }  /* 8 */
	};
#define kNumEnvPoints1 (sizeof(EnvPoints1)/sizeof(EnvelopeSegment))


/* Piano style envelope with release jump at 2. */
static	EnvelopeSegment	EnvPoints2[] =
	{   /*    Value,  Duration */
		{  0.00, 1.00 }, /* 0 */
	 	{  0.90, 9.00 }, /* 1 */
		{  0.00, 1.00 }, /* 2 */
		{  0.00, 0.00 }  /* 3 */
	};
#define kNumEnvPoints2 (sizeof(EnvPoints2)/sizeof(EnvelopeSegment))

/* Piano style envelope with release jump at 2 followed by wiggle. */
static	EnvelopeSegment	EnvPoints3[] =
	{   /*    Value,  Duration */
		{  0.00, 1.0 }, /* 0 */
	 	{  0.90, 9.0 }, /* 1 */
		{  0.00, 1.0 }, /* 2 */
		{  0.55, 1.0 }, /* 3 */
		{  0.00, 0.0 }  /* 4 */
	};
#define kNumEnvPoints3 (sizeof(EnvPoints3)/sizeof(EnvelopeSegment))


/* Envelope suggested by Don Veca. */
static	EnvelopeSegment	EnvPoints4[] =
	{   /*    Value,  Duration */
		{  0.00, 1.00 }, /* 0 */
	 	{  0.90, 2.00 }, /* 1 */
		{  0.30, 0.50 }, /* 2 SUSTAIN POINT and RELEASE JUMP */
		{  0.10, 0.50 }, /* 3 */
		{  0.00, 0.00 }  /* 4 */
	};
#define kNumEnvPoints4 (sizeof(EnvPoints4)/sizeof(EnvelopeSegment))


/* Piano style envelope with VERY SHORT ATTACK release jump at 2. */
static	EnvelopeSegment	EnvPoints5[] =
	{   /*    Value,  Duration */
		{  0.00, 0.001 }, /* 0 */
	 	{  0.90, 0.600 }, /* 1 */
		{  0.00, 0.400 }, /* 2 */
		{  0.00, 0.000 }  /* 3 */
	};
#define kNumEnvPoints5 (sizeof(EnvPoints5)/sizeof(EnvelopeSegment))

/* Simple envelope that starts != 0. */
static	EnvelopeSegment	EnvPoints6[] =
	{   /*    Value,  Duration */
	 	{  0.90, 0.60 }, /* 0 */
		{  0.00, 0.00 }, /* 1 */
	};
#define kNumEnvPoints6 (sizeof(EnvPoints6)/sizeof(EnvelopeSegment))

/* Long envelope that starts != 0. */
static	EnvelopeSegment	EnvPoints7[] =
	{   /*    Value,  Duration */
	 	{  0.90, 5.0 }, /* 0 */
		{  0.15, 7.0 }, /* 1 */
		{  0.00, 0.0 }, /* 2 */
	};
#define kNumEnvPoints7 (sizeof(EnvPoints7)/sizeof(EnvelopeSegment))

/* Envelope with an extra point to experiment with over indexing. */
static	EnvelopeSegment	EnvPoints8[] =
	{   /*    Value,  Duration */
	 	{  0.00, 1.0 }, /* 0 */
		{  0.40, 1.0 }, /* 1 */
		{  0.00, 1.0 }, /* 2 */
		{  0.99, 0.0 }, /* 3 - NOT REALLY A POINT */
	};
#define kNumEnvPoints8 ((sizeof(EnvPoints8)/sizeof(EnvelopeSegment))-1)

#define SEG_DUR  (1.0)
/* Recognizable Loop. */
static	EnvelopeSegment	EnvPoints9[] =
	{   /*    Value,  Duration */
		{  0.00, SEG_DUR }, /* 0 */
	 	{  0.50, SEG_DUR }, /* 1 */
		{  0.00, SEG_DUR }, /* 2 */
		{  0.50, SEG_DUR }, /* 3 */
		{  0.50, SEG_DUR }, /* 4 */
		{  0.99, SEG_DUR }, /* 5 */
		{  0.00, SEG_DUR }, /* 6 */
		{  0.75, SEG_DUR }, /* 7 */
		{  0.00, 0.0 }      /* 8 */
	};
#define kNumEnvPoints9 (sizeof(EnvPoints9)/sizeof(EnvelopeSegment))

Item MakeEnvelopes( int32 TestIndex );

/***************************************************************************/
Err CleanupEnvVoice( EnvVoice *envv )
{
	UnloadSample( envv->envv_Sample );
	DeleteInstrument( envv->envv_OscIns );
	UnloadInsTemplate( envv->envv_OscTmp );
	DeleteEnvelope( envv->envv_Envelope );
	DeleteCue( envv->envv_EnvCue );
	UnloadInstrument( envv->envv_EnvIns );
	UnloadInstrument( envv->envv_OutputIns );

	return 0;
}

/***************************************************************************/
Err SetupEnvVoice( EnvVoice *envv, int32 Channel, int32 TestIndex)
{
	int32 Result;

/* Load description of Oscillator instrument */
	envv->envv_OscTmp = LoadInsTemplate( OSCINSNAME, NULL );
	CHECKRESULT(envv->envv_OscTmp,"LoadInsTemplate");

/* Make an instrument based on template. */
	envv->envv_OscIns = CreateInstrument(envv->envv_OscTmp, NULL);
	CHECKRESULT(envv->envv_OscIns,"CreateInstrument");

/* Load sinewave sample, which is a standard system sample. If you want to load
** your own sample, use LoadSample() instead. */
	envv->envv_Sample = LoadSystemSample( "sinewave.aiff" );
	CHECKRESULT(envv->envv_Sample,"LoadSystemSample");

/* Attach sinewave sample to oscillator. */
	envv->envv_Att = CreateAttachment( envv->envv_OscIns, envv->envv_Sample, NULL );
	CHECKRESULT(envv->envv_Att,"CreateAttachment");

/* Load Envelope dsp instrument. */
	envv->envv_EnvIns = LoadInstrument( ENVINSNAME, 0, 100 );
	CHECKRESULT(envv->envv_EnvIns,"LoadInstrument");

/* Create output instrument so we hear sound. */
	envv->envv_OutputIns = LoadInstrument("line_out.dsp",  0,  100);
	CHECKRESULT(envv->envv_OutputIns,"LoadInstrument");

/* Connect Envelope to amplitude of oscillator. */
	Result = ConnectInstruments (envv->envv_EnvIns, "Output", envv->envv_OscIns, "SampleRate");
	CHECKRESULT(Result,"ConnectInstruments");

/* Create envelope using one of many techniques. */
	envv->envv_Envelope = MakeEnvelopes( TestIndex );
	CHECKRESULT(envv->envv_Envelope,"MakeEnvelopes");

/* Attach envelope to envelope player. */
	envv->envv_EnvAttachment = CreateAttachmentVA( envv->envv_EnvIns, envv->envv_Envelope,
		AF_TAG_SET_FLAGS, AF_ATTF_FATLADYSINGS, TAG_END );
	CHECKRESULT(envv->envv_EnvAttachment,"CreateAttachment");

/* Create a Cue to monitor Envelope Attachment */
	envv->envv_EnvCue = CreateCue(NULL);
	CHECKRESULT(envv->envv_EnvCue,"CreateCue");
	envv->envv_EnvCueSignal = GetCueSignal(envv->envv_EnvCue);
	Result = MonitorAttachment( envv->envv_EnvAttachment, envv->envv_EnvCue, CUE_AT_END );
	CHECKRESULT(Result,"MonitorAttachment");

/* Connect output of triangle to output channel. */
	Result = ConnectInstrumentParts (envv->envv_OscIns, "Output", 0, envv->envv_OutputIns, "Input", Channel);
	CHECKRESULT(Result,"ConnectInstrumentParts");

/* Start oscillator and line_out to running continuously. */
	Result = StartInstrument( envv->envv_OutputIns, NULL );
	CHECKRESULT(Result,"StartInstrument OutputIns");
	Result = StartInstrument (envv->envv_OscIns, NULL);
	CHECKRESULT(Result,"StartInstrument Osc");
	return Result;
cleanup:

	CleanupEnvVoice( envv );

	return Result;
}
/******************************************************************/
int main( int argc, char *argv[] )
{

	Item 			TimerCue;
	int32 			Result;
	int32			i;
	int32			TestIndex;
	int32			IQuit, NoteOnA, NoteOnB;
	uint32			Buttons;
	ControlPadEventData cped;

	PRT(("\nta_envelope\n"));

/* Initialize the EventBroker. */
	Result = InitEventUtility(1, 0, TRUE);
	if (Result < 0)
	{
		PrintError(0,"init event utility",0,Result);
		goto cleanup;
	}

/* Initialize audio, return if error. */
	if ((Result = OpenAudioFolio()) < 0)
	{
		PrintError (NULL, "open audio folio", NULL, Result);
		return(Result);
	}

	TestIndex = (argc > 1) ? atoi(argv[1]) : 1;

	for( i=0; i<NUM_ENV_VOICES; i++ )
	{
		Result = SetupEnvVoice( &EnvVoices[i], i, TestIndex);
		CHECKRESULT(Result,"SetupEnvVoice");
	}

	TimerCue = CreateCue(NULL);
	CHECKRESULT(TimerCue,"CreateCue");

	IQuit = FALSE;
	NoteOnA = FALSE;
	NoteOnB = FALSE;

	do
	{
/* #define MONITOR_ENV */
#ifdef MONITOR_ENV
/* Read current state of Control Pad. */
		do
		{
/* Sleep to give other tasks time. */
			SleepUntilTime( TimerCue, GetAudioTime() + 10 );

			Result = GetControlPad (1, FALSE, &cped);
			if (Result < 0) {
				PrintError(0,"read control pad",0,Result);
			}
			Buttons = cped.cped_ButtonBits;

			if( GetCurrentSignals() & EnvCueSignal )
			{
				PRT(("Received signal from MonitorAttachment!\n"));
				WaitSignal( EnvCueSignal ); /* Clear the signal */
			}

/* Print progress of envelope. */
			PRT(("Envelope at %d\n", WhereAttachment( EnvAttachment ) ));

		} while( Buttons == 0 );
#else
		Result = GetControlPad (1, TRUE, &cped);
		if (Result < 0) {
			PrintError(0,"read control pad",0,Result);
		}
		Buttons = cped.cped_ButtonBits;
#endif

		if((Buttons & ControlA) || (Buttons & ControlC))
		{
			if(NoteOnA)
			{
				PRT(("Release A.\n"));
				Result = ReleaseInstrumentVA ( EnvVoices[0].envv_EnvIns,
                                               AF_TAG_TIME_SCALE_FP, ConvertFP_TagData(TimeScale),
                                               TAG_END );
				CHECKRESULT(Result,"ReleaseInstrument Env");
				NoteOnA = FALSE;
			}
			else
			{
				PRT(("Start A.\n"));
				Result = StartInstrumentVA ( EnvVoices[0].envv_EnvIns,
                                             AF_TAG_TIME_SCALE_FP, ConvertFP_TagData(TimeScale),
                                             TAG_END );
				CHECKRESULT(Result,"StartInstrument Env");
				NoteOnA = TRUE;
			}
		}
		if((Buttons & ControlB) || (Buttons & ControlC))
		{
			if(NoteOnB)
			{
				PRT(("Release B.\n"));
				Result = ReleaseInstrumentVA ( EnvVoices[1].envv_EnvIns,
                                               AF_TAG_TIME_SCALE_FP, ConvertFP_TagData(TimeScale),
                                               TAG_END );
				CHECKRESULT(Result,"ReleaseInstrument Env");
				NoteOnB = FALSE;
			}
			else
			{
				PRT(("Start B.\n"));
				Result = StartInstrumentVA ( EnvVoices[1].envv_EnvIns,
                                             AF_TAG_TIME_SCALE_FP, ConvertFP_TagData(TimeScale),
                                             TAG_END );
				CHECKRESULT(Result,"StartInstrument Env");
				NoteOnB = TRUE;
			}
		}

		if( Buttons & ControlUp )
		{
			TimeScale = TimeScale * 5.0 / 4.0;
			PRT(("TimeScale = %g\n", TimeScale));
		}
		if( Buttons & ControlDown )
		{
			TimeScale = TimeScale * 4.0 / 5.0;
			PRT(("TimeScale = %g\n", TimeScale));
		}

		if( Buttons & ControlX)
		{
			IQuit = TRUE;
		}
	} while (!IQuit);


cleanup:
	for( i=0; i<NUM_ENV_VOICES; i++ )
	{
		CleanupEnvVoice( &EnvVoices[i] );
	}

	CloseAudioFolio();
	KillEventUtility();
	PRT(("%s all done.\n", argv[0]));
	return((int) Result);
}

/******************************************************************/
static Item tCreateSustainEnvelope (const EnvelopeSegment *points, int32 numPoints, int32 sustainBegin, int32 sustainEnd);

Item MakeEnvelopes( int32 TestIndex )
{
/* Create Envelope Item. */
	switch( TestIndex )
	{
		case 1:
			PRT(("Sustain loop, no release loop.\n"));
			return CreateEnvelopeVA (EnvPoints1, kNumEnvPoints1,
				AF_TAG_SUSTAINBEGIN,    1,
				AF_TAG_SUSTAINEND,      3,
				TAG_END);

		case 2:
			PRT(("Sustain loop, Release loop.\n"));
			return CreateEnvelopeVA (EnvPoints1, kNumEnvPoints1,
				AF_TAG_SUSTAINBEGIN,    1,
				AF_TAG_SUSTAINEND,      3,
				AF_TAG_RELEASEBEGIN,    4,
				AF_TAG_RELEASEEND,      6,
				TAG_END);

		case 3:
			PRT(("Release jump.\n"));
			return CreateEnvelopeVA (EnvPoints2, kNumEnvPoints2,
				AF_TAG_RELEASEJUMP,     2,
				TAG_END);

		case 4:
			PRT(("Release jump with wiggle and release time.\n"));
			return CreateEnvelopeVA (EnvPoints3, kNumEnvPoints3,
				AF_TAG_RELEASEJUMP,     2,
				AF_TAG_RELEASEBEGIN,    3,
				AF_TAG_RELEASEEND,      4,
				AF_TAG_RELEASETIME_FP,  ConvertFP_TagData(2.0),
				TAG_END);

		case 5:
			PRT(("Release jump with sustain at 1.\n"));
			return CreateEnvelopeVA (EnvPoints2, kNumEnvPoints2,
				AF_TAG_RELEASEJUMP,     2,
				AF_TAG_SUSTAINBEGIN,    1,
				AF_TAG_SUSTAINEND,      1,
				TAG_END);

		case 6:
			PRT(("Just sustain at 1, no jump.\n"));
			return CreateEnvelopeVA (EnvPoints2, kNumEnvPoints2,
				AF_TAG_SUSTAINBEGIN,    1,
				AF_TAG_SUSTAINEND,      1,
				TAG_END);

		case 7:
			PRT(("Release jump = sustain at 2.\n"));
			return CreateEnvelopeVA (EnvPoints4, kNumEnvPoints4,
				AF_TAG_RELEASEJUMP,     2,
				AF_TAG_SUSTAINBEGIN,    2,
				AF_TAG_SUSTAINEND,      2,
				TAG_END);

		case 8:
			PRT(("Test FLS bit.\n"));
			return CreateEnvelopeVA (EnvPoints4, kNumEnvPoints4,
				AF_TAG_SET_FLAGS,       AF_ENVF_FATLADYSINGS,
				AF_TAG_SUSTAINBEGIN,    2,
				AF_TAG_SUSTAINEND,      2,
				TAG_END);

		case 9:
			PRT(("Release jump to last point.\n"));
			return CreateEnvelopeVA (EnvPoints2, kNumEnvPoints2,
				AF_TAG_RELEASEJUMP,     kNumEnvPoints2 - 1,
				TAG_END);

		case 10:
			PRT(("Release jump to last point with VERY SHORT ATTACK.\n"));
			return CreateEnvelopeVA (EnvPoints5, kNumEnvPoints5,
				AF_TAG_RELEASEJUMP,     kNumEnvPoints5 - 2,
				TAG_END);

		case 11:
			PRT(("Envelope that starts at 30000\n"));
			return CreateEnvelopeVA (EnvPoints6, kNumEnvPoints6,
				TAG_END);

		case 12:
			PRT(("Long Envelope that starts at 30000\n"));
			return CreateEnvelopeVA (EnvPoints7, kNumEnvPoints7,
				TAG_END);

		case 13:
/* This is illegal and should return an error. */
			PRT(("tCreateSustainEnvelope(d,n,2,1)"));
			return tCreateSustainEnvelope( EnvPoints1, kNumEnvPoints1, 2,1 );

		case 14:
/* This is illegal and should return an error. */
			PRT(("tCreateSustainEnvelope(d,n,0,3)"));
			return tCreateSustainEnvelope( EnvPoints8, kNumEnvPoints8, 1,2 );

		case 15:
			PRT(("tCreateSustainEnvelope(d,9,0,6)\n"));
			return tCreateSustainEnvelope( EnvPoints9, kNumEnvPoints9, 0,6 );

		case 16:
			PRT(("tCreateSustainEnvelope(d,9,1,6)\n"));
			return tCreateSustainEnvelope( EnvPoints9, kNumEnvPoints9, 1,6 );

		case 17:
			PRT(("tCreateSustainEnvelope(d,9,2,7)\n"));
			return tCreateSustainEnvelope( EnvPoints9, kNumEnvPoints9, 2,7 );

		case 18:
			PRT(("tCreateSustainEnvelope(d,9,2,8)\n"));
			return tCreateSustainEnvelope( EnvPoints9, kNumEnvPoints9, 2,8 );

		case 19:
/* Should be illegal. */
			PRT(("tCreateSustainEnvelope(d,9,2,9)\n"));
			return tCreateSustainEnvelope( EnvPoints9, kNumEnvPoints9, 2,9 );

		default:
			ERR(("Invalid envelope test index = %d\n", TestIndex ));
			return -1;
	}
}

static Item tCreateSustainEnvelope (const EnvelopeSegment *points, int32 numPoints, int32 sustainBegin, int32 sustainEnd)
{
	return CreateEnvelopeVA (points, numPoints,
		AF_TAG_SUSTAINBEGIN, sustainBegin,
		AF_TAG_SUSTAINEND,   sustainEnd,
		TAG_END);
}
