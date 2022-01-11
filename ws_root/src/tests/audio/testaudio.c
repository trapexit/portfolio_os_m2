/* @(#) testaudio.c 96/08/27 1.26 */
/* $Id: testaudio.c,v 1.21 1994/08/30 22:36:38 jbyrd Exp $ */
/****************************************************************
**
** Audio Folio interactive all in one test.
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
****************************************************************/
/*
** 930126 PLB Change to use .dsp files instead of .ofx
** 930315 PLB Conforms to new API
** 950322 PLB Remove dependance on audiodemo.lib.
*/

#include <kernel/types.h>
#include <kernel/nodes.h>
#include <kernel/kernelnodes.h>
#include <file/filefunctions.h>
#include <kernel/debug.h>
#include <kernel/operror.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <misc/event.h>

/* Include this when using the Audio Folio */
#include <audio/audio.h>
#include <audio/parse_aiff.h>

#define WORKAROUND_DIAB_BUG (0)

#define MAXVOICES 24
#define NUMVOICES 12
#define NUMER 5
#define DENOM 4
#define STARTFREQ 0x2000
#define NUMSECS            (1)
#define SAMPPRIORITY      (50)
#define MIXPRIORITY      (100)
#define TOGETHER
#define COARSE_SCALAR    (0.9)
#define FINE_SCALAR     (0.99)
#define MAXAMPLITUDE     (0.9)
#define FREQSTEP     (9.0/8.0)
#define MINSR         (1000.0)
#define MAXSR        (88000.0)

#define INSNAME "sampler_16_v1.dsp"

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */

int32 PlayFreqNote ( Item Instrument, int32 Freq, int32 Duration );
int32 TestSaw (void );
int32 TestPoly( void );
int32 TestSteady( void );

int32 SleepSeconds( int32 Secs );

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		PrintError(0,"\\failure in",name,val); \
		goto cleanup; \
	}


static MixerSpec gMixerSpec;
static Item  gMixerTmp = -1;
static Item  gMixerIns = -1;
static Item  gGainKnob = -1;
static int32 SetupMixer( int32 NumInputs );
static int32 TermMixer( void );
static int32 gNumVoices;

/******************************************************************/
int32 WaitControlPad( void )
{
	ControlPadEventData cped;
        static int32 oldbutn = 0;
        int32 Result;
        int32 butn;

        do
        {
                Result = GetControlPad (1, TRUE, &cped);
                if (Result < 0)
                {
                        PrintError(0,"read control pad in","WaitControlPad",Result);
                        return Result;
                }
                butn = cped.cped_ButtonBits;
                if (butn == 0) oldbutn = 0;
        } while(butn == oldbutn);

        oldbutn = butn;
DBUG(("butn = 0x%x\n", butn));
        return butn;
}

/******************************************************************/
int main(int argc, char *argv[])
{
	int32 Result;
	uint32 butn;
	int32 doit;

	PRT(("Usage: %s\n", argv[0]));

	gNumVoices = (argc > 1) ? atoi( argv[1] ) : NUMVOICES ;
	if(gNumVoices > MAXVOICES) gNumVoices = MAXVOICES;

/* Initialize audio, return if error. */
	if ((Result = OpenAudioFolio()) < 0)
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(Result);
	}

	Result = InitEventUtility(1, 0, TRUE);
	CHECKRESULT(Result, "InitEventUtility");

/* Allow user to select test. */
	doit = TRUE;
	do
	{
		PRT(("---------------------------------------------------\n"));
		PRT(("Use Joypad to select audio test...\n"));
		PRT(("   A = Sawtooth on left and right channel.\n"));
		PRT(("   B = %d voice polyphony.\n", gNumVoices));
		PRT(("   C = Steady sinewave.\n"));
		PRT(("   X = quit.\n"));
		PRT(("---------------------------------------------------\n"));

		butn = WaitControlPad();
		switch (butn)
		{
		case ControlA:
			Result = TestSaw();
			CHECKRESULT(Result, "TestSaw");
			break;

		case ControlB:
			Result = TestPoly();
			CHECKRESULT(Result, "TestPoly");
			break;

		case ControlC:
			Result = TestSteady();
			CHECKRESULT(Result, "TestSteady");
			break;

		case ControlX:
			doit = FALSE;
			break;
		}
	} while (doit);


cleanup:
	KillEventUtility();
	CloseAudioFolio();
	PRT(("Audio Test Complete\n"));
	return (Result != 0);
}

/****************************************************************************/
int32 TestSaw (void )
{
	Item OutputIns;
	Item SawIns = 0;
	Item FreqKnob = 0;
	Item LoudKnob;
	int32 Result;

/* Use directout instead of mixer. */
	OutputIns = LoadInstrument("line_out.dsp",  0, 100);
	CHECKRESULT(OutputIns,"LoadInstrument");

/* Load Sawtooth instrument */
	SawIns = LoadInstrument("sawtooth.dsp",  0, 100);
	CHECKRESULT(SawIns,"LoadInstrument");

/* Attach the Frequency knob. */
	FreqKnob = CreateKnob( SawIns, "Frequency", NULL );
	CHECKRESULT(FreqKnob,"CreateKnob");
	Result = SetKnob(FreqKnob, 200.0);
	CHECKRESULT(Result,"SetKnob");

/* Attach the Amplitude knob. */
	LoudKnob = CreateKnob( SawIns, "Amplitude", NULL );
	CHECKRESULT(LoudKnob,"CreateKnob");
	SetKnob(LoudKnob, 0.4);
	CHECKRESULT(Result,"SetKnob");

/* Connect Sampler to DirectOut */
	StartInstrument(SawIns, NULL);
	StartInstrument( OutputIns, NULL );

	Result = ConnectInstrumentParts (SawIns, "Output", 0, OutputIns, "Input", 0);
	CHECKRESULT(Result,"ConnectInstruments");
	PRT(("Left Channel\n"));
	PRT(("Hit joypad to continue.\n"));
	WaitControlPad();

/*	SleepSeconds(4); */
	DisconnectInstrumentParts ( OutputIns, "Input", 0);

	Result = SetKnob(FreqKnob, 300.0);
	CHECKRESULT(Result,"SetKnob");

	Result = ConnectInstrumentParts (SawIns, "Output", 0, OutputIns, "Input", 1);
	CHECKRESULT(Result,"ConnectInstruments");
	PRT(("Right Channel\n"));
	PRT(("Hit joypad to continue.\n"));
	WaitControlPad();
/*	SleepSeconds(4); */
	Result = DisconnectInstrumentParts ( OutputIns, "Input", 1);

	StopInstrument(SawIns, NULL);

cleanup:
/* The Audio Folio is immune to passing NULL values as Items. */
	DeleteKnob( FreqKnob);
	UnloadInstrument( SawIns );
	UnloadInstrument( OutputIns );
	PRT(("Sawtooth Test Complete.\n"));
	return Result;
}

/****************************************************************************/
int32 TestPoly( void )
{
	Item SamplerTmp = 0;
	Item SamplerIns[MAXVOICES];
	Item SampleItem = 0;
	int32 i, Result;
	float32 Freq;

	PRT(("Play %d channels of sine wave in ascending frequency.\n", gNumVoices));

	PRT(("Please wait a few seconds for samples and instruments to load.\n"));
	Result = SetupMixer( gNumVoices );
	CHECKRESULT(Result, "SetupMixer");

/* Set each channel to be panned left to right. */
	for (i=0; i<gNumVoices; i++)
	{
		SetKnobPart ( gGainKnob, CalcMixerGainPart(gMixerSpec,i,AF_PART_LEFT), (((float32)(i))/gNumVoices)/gNumVoices );
		SetKnobPart ( gGainKnob, CalcMixerGainPart(gMixerSpec,i,AF_PART_RIGHT), (((float32)(gNumVoices - 1 - i))/gNumVoices)/gNumVoices );
	}

/* Load Sampler instrument definition/template */
	PRT(("   use %s\n", INSNAME));
	SamplerTmp = LoadInsTemplate(INSNAME, NULL);
	CHECKRESULT(SamplerTmp,"LoadInsTemplate");

/* Make Sampler instruments based on template. */
	for (i=0; i<gNumVoices; i++)
	{
		SamplerIns[i] = CreateInstrument(SamplerTmp, NULL);
		CHECKRESULT(SamplerIns[i],"CreateInstrument");
	}

/* Load digital audio Sample from disk. */
	SampleItem = LoadSystemSample("sinewave.aiff");
	CHECKRESULT(SampleItem,"LoadSystemSample");

/* Look at sample information. */
/*	DebugSample(SampleItem); */

/* Attach the sample to the instrument. */
	for (i=0; i<gNumVoices; i++)
	{
		Result = CreateAttachment( SamplerIns[i], SampleItem, NULL );
		CHECKRESULT(Result,"CreateAttachment");
		Result = ConnectInstrumentParts (SamplerIns[i], "Output", 0,
			gMixerIns, "Input", i);
		CHECKRESULT(Result,"ConnectInstruments");
	}

	Freq = 70.0;
PRT(("Freq = "));
	for (i=0; i<gNumVoices; i++)
	{
		PRT((" %g, ", Freq));
		Result = StartInstrumentVA( SamplerIns[i], AF_TAG_FREQUENCY_FP, ConvertFP_TagData(Freq), TAG_END );
		Freq = Freq * FREQSTEP;
		SleepSeconds( NUMSECS );
#ifdef TOGETHER
	}

	PRT(("Hit any button to continue.\n"));
	WaitControlPad();

	PRT(("Turning off voices.\n"));
	for (i=0; i<gNumVoices; i++)
	{
#endif
		ReleaseInstrument( SamplerIns[i],  NULL );
#ifdef TOGETHER
		SleepSeconds( NUMSECS );
#endif
	}

cleanup:

	for(i=0; i<gNumVoices; i++)
	{
		DeleteInstrument( SamplerIns[i] );
	}
	UnloadInsTemplate( SamplerTmp );
	UnloadSample( SampleItem );
	TermMixer();
	PRT(("Polyphony Test Complete.\n"));
	return((int) Result);
}
/*********************************************************************/
int32 SetupMixer( int32 NumInputs )
{
	int32 Result;

	gMixerSpec = MakeMixerSpec (NumInputs, 2, AF_F_MIXER_WITH_LINE_OUT);
	gMixerTmp = CreateMixerTemplate( gMixerSpec, NULL );
	CHECKRESULT(gMixerTmp,"CreateMixerTemplate");

/* Make an instrument based on template. */
	gMixerIns = CreateInstrument(gMixerTmp, NULL );
	CHECKRESULT(gMixerIns,"CreateInstrument");

/* Attach the Gain knobs. */
	gGainKnob = CreateKnob( gMixerIns, "Gain", NULL );
	CHECKRESULT(gGainKnob, "CreateKnob");

/* Mixer must be started */
	Result = StartInstrument( gMixerIns, NULL );
	return Result;

cleanup:
	TermMixer();
	return Result;
}

/*********************************************************************/
int32 TermMixer()
{
	DeleteKnob(gGainKnob);
	DeleteInstrument( gMixerIns );
	UnloadInsTemplate( gMixerTmp );
	return 0;
}

/*********************************************************************/
int32 SleepSeconds( int32 Secs )
{
	float32 Rate;
	Item timerCue;
	Err errcode;

	GetAudioClockRate( AF_GLOBAL_CLOCK, &Rate );
	if ((errcode = timerCue = CreateCue (NULL)) < 0) goto clean;
	errcode = SleepUntilTime (timerCue, GetAudioTime() + (Secs * Rate));

clean:
	DeleteCue (timerCue);
	return errcode;
}


/****************************************************************************/
/*********** Steady Sine Wave until joy pad hit. ****************************/
/****************************************************************************/
#if WORKAROUND_DIAB_BUG
void PrintDecibels( float32 Amplitude )
{
	float32 decibels;
	decibels = 10.0 * log10f( Amplitude );
	PRT(("Amplitude = %e, Decibels = %g\n", Amplitude, decibels ));
}
#endif

/****************************************************************************/
int32 TestSteady( void )
{
	Item SamplerIns = 0;
	Item SampleItem = 0;
	Item EnvLeft = 0, EnvRight = 0;
	Item LeftGain = 0, RightGain = 0;
	Item LeftIncr = 0, RightIncr = 0;
	Item OutputIns;
	Item SampleRateKnob;
	int32 Result;
	float32 SampleRate;
	int32 Pan, doit;
	float32 Amplitude = 0.5;
	int32 i;
	uint32 butn;

	PRT(("Steady sine tone.\n"));
	PRT(("Please wait a few seconds for samples and instruments to load.\n"));

/* Load "line_out" for connecting to DAC. */
	OutputIns = LoadInstrument("line_out.dsp",  0,  100);
	CHECKRESULT(OutputIns,"LoadInstrument");

	for (i=0; i<(gNumVoices*2); i++)
	{
		SetKnobPart ( gGainKnob, i, 0.0 );
	}

/* Load Sampler instrument definition/template */
	SamplerIns = LoadInstrument(INSNAME,  0, SAMPPRIORITY);
	CHECKRESULT(SamplerIns,"LoadInstrument");

/* Load envelopes to smooth out amplitude changes. */
	EnvLeft = LoadInstrument("envelope.dsp",  0, 100);
	CHECKRESULT(SamplerIns,"LoadInstrument EnvLeft");
	EnvRight = LoadInstrument("envelope.dsp",  0, 100);
	CHECKRESULT(SamplerIns,"LoadInstrument EnvRight");

/* Connect Sampler to Envelopes Amplitude so they get multiplied together. */
	Result = ConnectInstruments(SamplerIns, "Output", EnvLeft, "Amplitude" );
	CHECKRESULT(Result,"ConnectInstruments");
	Result = ConnectInstruments(SamplerIns, "Output", EnvRight, "Amplitude" );
	CHECKRESULT(Result,"ConnectInstruments");

/* Connect output of envelopes to DAC. */
	Result = ConnectInstrumentParts (EnvLeft, "Output", 0, OutputIns, "Input", 0);
	CHECKRESULT(Result,"ConnectInstrumentParts 0");
	Result = ConnectInstrumentParts (EnvRight, "Output", 0, OutputIns, "Input", 1);
	CHECKRESULT(Result,"ConnectInstrumentParts 1");

	Result = StartInstrument( EnvLeft, NULL );
	CHECKRESULT(Result,"StartInstrument");
	Result = StartInstrument( EnvRight, NULL );
	CHECKRESULT(Result,"StartInstrument");
	Result = StartInstrument( OutputIns, NULL );
	CHECKRESULT(Result,"StartInstrument");

/* Load digital audio Sample from disk. */
	SampleItem = LoadSystemSample("sinewave.aiff");
	CHECKRESULT(SampleItem,"LoadSystemSample");

	Result = CreateAttachment(SamplerIns, SampleItem, NULL);
	CHECKRESULT(Result,"CreateAttachment");

/* Create Knobs */
	SampleRateKnob = CreateKnob(SamplerIns, "SampleRate", NULL);
	CHECKRESULT(SampleRateKnob,"CreateKnob");
	LeftGain = CreateKnob(EnvLeft, "Env.request", NULL);
	CHECKRESULT(LeftGain,"CreateKnob");
	RightGain = CreateKnob(EnvRight, "Env.request", NULL);
	CHECKRESULT(RightGain,"CreateKnob");
	LeftIncr = CreateKnob(EnvLeft, "Env.incr", NULL);
	CHECKRESULT(LeftIncr,"CreateKnob");
	RightIncr = CreateKnob(EnvRight, "Env.incr", NULL);
	CHECKRESULT(RightIncr,"CreateKnob");

/* Set Mixer Levels */
	Pan = 1;
	SetKnob ( LeftGain, Amplitude );
	SetKnob ( RightGain, Amplitude );
/* Adjust this for smooth amplitude changes. */
	SetKnob ( LeftIncr, 0.001 );
	SetKnob ( RightIncr, 0.001 );

	SampleRate = 44100.0;
	Result = StartInstrumentVA( SamplerIns, AF_TAG_SAMPLE_RATE_FP, ConvertFP_TagData(SampleRate), TAG_END  );

/* Process JoyPad */
	PRT(("Use Joypad:\n"));
	PRT(("  JOYLEFT, JOYRIGHT - control left/right panning.\n"));
	PRT(("  JOYUP, JOYDOWN - control loudness.\n"));
	PRT(("  FIREA - raises frequency.\n"));
	PRT(("  FIREB - lowers frequency.\n"));

	doit = TRUE;
	while (doit)
	{
		butn = WaitControlPad();

		if (butn & ControlStart)
		{
			doit = FALSE;
		}

		if (butn & ControlA)
		{
			SampleRate /= ((butn & ControlLeftShift) ?
				FINE_SCALAR : COARSE_SCALAR);
			if (SampleRate > MAXSR) SampleRate = MAXSR;
			PRT(("SampleRate = %g\n", SampleRate));
			SetKnob(SampleRateKnob, SampleRate);
		}

		if (butn & ControlB)
		{
			SampleRate *= ((butn & ControlLeftShift) ?
				FINE_SCALAR : COARSE_SCALAR);
			if (SampleRate < MINSR) SampleRate = MINSR;
			PRT(("SampleRate = %g\n", SampleRate));
			SetKnob(SampleRateKnob, SampleRate);
		}

		if (butn & ControlUp)
		{
			Amplitude /= ((butn & ControlLeftShift) ?
				FINE_SCALAR : COARSE_SCALAR);
			if (Amplitude > MAXAMPLITUDE) Amplitude = MAXAMPLITUDE;
		}
		if (butn & ControlDown)
		{
			Amplitude *= ((butn & ControlLeftShift) ?
				FINE_SCALAR : COARSE_SCALAR);
			if (Amplitude < 0.0) Amplitude = 0.0;
		}

		if (butn & ControlLeft)
		{
			if (Pan > 0)
			{
				Pan -= 1;
			}
		}

		if (butn & ControlRight)
		{
			if (Pan < 2)
			{
				Pan += 1;
			}
		}

		if ((butn & (ControlRight|ControlLeft|ControlUp|ControlDown|ControlC)) && !(butn & ControlB))
		{
#if WORKAROUND_DIAB_BUG
			PrintDecibels( Amplitude );
#else
/* This gives the wrong answer, sometimes. */
			float32 decibels;
			decibels = 10.0 * log10f( Amplitude );
			PRT(("Amplitude = %e, Decibels = %g\n", Amplitude, decibels ));
#endif
			switch(Pan)
			{
			case 0:
				SetKnob ( LeftGain, Amplitude );
				SetKnob ( RightGain, 0.0 );
				PRT(("Left only.\n"));
				break;
			case 1:
				SetKnob ( LeftGain, Amplitude );
				SetKnob ( RightGain, Amplitude );
				PRT(("Left and Right.\n"));
				break;
			case 2:
				SetKnob ( LeftGain, 0.0 );
				SetKnob ( RightGain, Amplitude );
				PRT(("Right only.\n"));
				break;
			default:
				ERR(("Illegal Pan = 0x%x\n", Pan));
				break;
			}
		}
	}

	ReleaseInstrument( SamplerIns,  NULL );

cleanup:
	DeleteKnob( LeftGain );
	DeleteKnob( RightGain );
	DeleteKnob( LeftIncr );
	DeleteKnob( RightIncr );
	UnloadInstrument( EnvLeft );
	UnloadInstrument( EnvRight );
	UnloadInstrument( OutputIns );
	UnloadInstrument( SamplerIns );
	UnloadSample( SampleItem );
	PRT(("Steady Sine Test Complete.\n"));
	return((int) Result);
}


