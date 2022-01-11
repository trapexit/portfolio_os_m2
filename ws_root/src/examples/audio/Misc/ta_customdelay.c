
/******************************************************************************
**
**  @(#) ta_customdelay.c 96/08/22 1.14
**  $Id: ta_customdelay.c,v 1.15 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC -public -class examples -group Audio -name ta_customdelay
|||	Demonstrates a delay line attachment.
|||
|||	  Format
|||
|||	    ta_customdelay [<sample file> [delay ticks]]
|||
|||	  Description
|||
|||	    Demonstrates how to create and use a delay line to get real-time echo
|||	    effects in your program. Loads the specified AIFF file and plays it into a
|||	    delay line. By tweaking the knobs on the output mixer, you can control the
|||	    mix of delay sound versus original sound, and the speed at which the echo
|||	    will die down.
|||
|||	    Please note that this example was written before we had the Patch Compiler.
|||	    If you wish to use reverb or delay effects, I would encourage you to
|||	    consider using the examples in Examples/Audio/Patches/Reverb.
|||
|||	  Arguments
|||
|||	    <sample file>
|||	        Name of an AIFF file to play.
|||
|||	    <delay ticks>
|||	        Amount of time, in audio clock ticks, to hold each note. Defaults to 240
|||	        ticks.
|||
|||	  Associated Files
|||
|||	    ta_customdelay.c
|||
|||	  Location
|||
|||	    Examples/Audio//Misc
|||
**/

#include <audio/audio.h>
#include <audio/parse_aiff.h>
#include <kernel/operror.h>
#include <kernel/types.h>
#include <stdio.h>
#include <stdlib.h>

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	PRT(x)

#define INSNAME "sampler_16_v1.dsp"

#define NUM_MIXER_INPUTS    (4)
#define MIXERSPEC           MakeMixerSpec (NUM_MIXER_INPUTS, 2, 0)

int32 PlayFreqNote ( Item Instrument, int32 Freq, int32 Duration );

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		PrintError(0,"\\failure in",name,Result); \
		goto cleanup; \
	}


int32 PlayPitchNote ( Item Instrument, int32 Note, int32 Velocity, int32 Duration, Item timerCue);

typedef struct DelayContext
{
	Item dlc_MixerTmp;
	Item dlc_MixerIns;
	Item dlc_OutputIns;
	Item dlc_DelayIns;
	Item dlc_DelayLine;
	Item dlc_TapIns;
	Item dlc_GainKnob;
	Item dlc_OriginalMix;
	Item dlc_DelayedSend;
	Item dlc_DelayedMix;
} DelayContext;

int32 InitCustomDelay( DelayContext *dlc, int32 DelaySize, int32 DelayFrames);
int32 TermCustomDelay( DelayContext *dlc );

#define DELAY_FRAMES   (100000)

/********************************************************************/
int main(int argc, char *argv[])
{
	Item SamplerIns = 0;
	Item SampleItem = 0;
	Item Attachment = 0;
	Item timerCue = -1;
	DelayContext DelayCon;
	int32 Duration;
	int32 Result;
	char *SampleName;
	int32 i;

	PRT(("%s [<sample file> [<delay ticks>]]\n", argv[0]));

/* Initialize audio, return if error. */
	if ((Result = OpenAudioFolio()) < 0)
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(Result);
	}

	Result = InitCustomDelay( &DelayCon, DELAY_FRAMES*sizeof(int16), DELAY_FRAMES - 100);
	CHECKRESULT(Result,"InitCustomDelay");

	DelayCon.dlc_OutputIns = LoadInstrument("line_out.dsp", 0, 100);
	CHECKRESULT(DelayCon.dlc_OutputIns,"LoadInstrument");

/* Load Sampler instrument */
	SamplerIns = LoadInstrument(INSNAME, 0, 100);
	CHECKRESULT(SamplerIns,"LoadInstrument");

/* Load digital audio Sample from disk. */
	SampleName = (argc > 1) ? argv[1] : "/remote/Samples/PitchedL/Violin/Violin.D4LM44k.aiff";
	SampleItem = LoadSample(SampleName);
	CHECKRESULT(SampleItem,"LoadSample");

	Duration = (argc > 2) ? atoi(argv[2]) : 240 ;
	PRT(("Duration = %d\n", Duration));

/* Look at sample information. */
	/* DebugSample(SampleItem); */

/* Connect Sampler to Mixer */
	Result = ConnectInstrumentParts (SamplerIns, "Output", 0,
		DelayCon.dlc_MixerIns, "Input", 0);
	CHECKRESULT(Result,"ConnectInstrumentParts");
	Result = ConnectInstrumentParts (DelayCon.dlc_MixerIns, "Output", 1,
		DelayCon.dlc_OutputIns, "Input", 0);
	CHECKRESULT(Result,"ConnectInstrumentParts");
	Result = ConnectInstrumentParts (DelayCon.dlc_MixerIns, "Output", 1,
		DelayCon.dlc_OutputIns, "Input", 1);
	CHECKRESULT(Result,"ConnectInstrumentParts");

/* Attach the sample to the instrument. */
	Attachment = CreateAttachment(SamplerIns, SampleItem, NULL);
	CHECKRESULT(Attachment,"CreateAttachment");

/* make timer Cue */
	timerCue = CreateCue(NULL);
	CHECKRESULT(timerCue,"CreateCue");

/* Instruments must be started */
	Result = StartInstrument( DelayCon.dlc_MixerIns, NULL );
	CHECKRESULT(Result,"StartInstrument");
/* Start Delay first to test START_AT.  It is safer to start
** the tap after the delay.
*/
	Result = StartInstrument( DelayCon.dlc_DelayIns, NULL );
	CHECKRESULT(Result,"StartInstrument");
	Result = StartInstrument( DelayCon.dlc_TapIns, NULL );
	CHECKRESULT(Result,"StartInstrument");
	Result = StartInstrument( DelayCon.dlc_OutputIns, NULL );
	CHECKRESULT(Result,"StartInstrument");

/* Play several notes using a conveniance routine. */
	PlayPitchNote( SamplerIns, 40, 100, Duration, timerCue );
	PlayPitchNote( SamplerIns, 44,  64, Duration, timerCue );
	PlayPitchNote( SamplerIns, 47,  64, Duration, timerCue );
	PlayPitchNote( SamplerIns, 52,  64, Duration, timerCue );
	PlayPitchNote( SamplerIns, 59, 100, Duration, timerCue );
	PlayPitchNote( SamplerIns, 64,  64, Duration*2, timerCue );
	for( i=0; i<16; i++)
	{
		PlayPitchNote( SamplerIns, i + 48,  64, Duration>>2, timerCue );
	}

/* Let echoes die down. */
	SleepUntilTime(timerCue, GetAudioTime()+8000);

cleanup:
	TermCustomDelay( &DelayCon );
	DeleteCue (timerCue);
	DeleteAttachment( Attachment );
	UnloadInstrument( SamplerIns );
	UnloadSample( SampleItem );

PRT(("All Done---------------------------------\n"));
	CloseAudioFolio();
	return((int) Result);
}

/********************************************************************/
/***** Play a note based on MIDI pitch. *****************************/
/********************************************************************/
int32 PlayPitchNote (Item Instrument, int32 Note, int32 Velocity, int32 Duration, Item timerCue)
{
	/*
		Notes:
			. Error trapping has been removed for brevity.
	*/

	StartInstrumentVA (Instrument,
	                   AF_TAG_VELOCITY, Velocity,
	                   AF_TAG_PITCH,    Note,
	                   TAG_END);
	SleepUntilTime (timerCue, GetAudioTime()+(Duration/2));

	ReleaseInstrument( Instrument, NULL);
	SleepUntilTime (timerCue, GetAudioTime()+(Duration/2));

	return 0;
}

/* Define various part numbers for the gain knob. */
#define PART_ORIGINAL_SEND CalcMixerGainPart( MIXERSPEC, 0, 0 )
#define PART_ORIGINAL_MIX  CalcMixerGainPart( MIXERSPEC, 0, 1 )
#define PART_DELAYED_SEND  CalcMixerGainPart( MIXERSPEC, 1, 0 )
#define PART_DELAYED_MIX   CalcMixerGainPart( MIXERSPEC, 1, 1 )
/*********************************************************************/
int32 InitCustomDelay( DelayContext *dlc, int32 DelaySize, int32 DelayFrames)
{
	int32 Result;
	Item Att;

/*
** Create a delay line.  This must be allocated by the AudioFolio
** because the memory is written to by hardware.  A delay line
** is just a sample with a special write permission.
** 1=channel, TR loop
*/
#define NUM_CHANNELS (1)
#define IF_LOOP      (TRUE)
	dlc->dlc_DelayLine = CreateDelayLine( DelaySize, NUM_CHANNELS, IF_LOOP );
	CHECKRESULT(dlc->dlc_DelayLine,"CreateDelayLine");

/*
** Load the basic delay instrument which just writes data to
** an output DMA channel of the DSP.
*/
	dlc->dlc_DelayIns = LoadInstrument("delay_f1.dsp", 0, 100);
	CHECKRESULT(dlc->dlc_DelayIns,"LoadInstrument");

/* Attach the delay line to the delay instrument output. */
	Att = CreateAttachment( dlc->dlc_DelayIns, dlc->dlc_DelayLine, NULL );
	CHECKRESULT(Att,"AttachDelay");
	Result = SetAudioItemInfoVA( Att,
                                 AF_TAG_START_AT, DelayFrames,
                                 TAG_END );
	CHECKRESULT(Result,"SetAudioItemInfo: START_AT");

/*
** Load an instrument to read the output of the delay.
*/
	dlc->dlc_TapIns = LoadInstrument("sampler_16_f1.dsp", 0, 100);
	CHECKRESULT(dlc->dlc_TapIns,"LoadInstrument");

/* Attach the delay line to the delay tap. */
	Att = CreateAttachment( dlc->dlc_TapIns, dlc->dlc_DelayLine, NULL );
	CHECKRESULT(Att,"CreateAttachment");
/*
** Load a submixer that we can use to mix delayed and original signal.
** We will use the left side to mix for the delay, and the right side
** for the output from the circuit.
*/
	dlc->dlc_MixerTmp = CreateMixerTemplate(MIXERSPEC, NULL);
	CHECKRESULT(dlc->dlc_MixerTmp,"CreateMixerTemplate");
	dlc->dlc_MixerIns = CreateInstrument(dlc->dlc_MixerTmp, NULL);
	CHECKRESULT(dlc->dlc_MixerIns,"CreateInstrument");

/* Create the Gain knob. */
	dlc->dlc_GainKnob = CreateKnob( dlc->dlc_MixerIns, "Gain", NULL );
	CHECKRESULT(dlc->dlc_GainKnob,"CreateKnob");

/* Connect the output of Tap0 to channel 1 of the mixer. */
	Result = ConnectInstrumentParts (dlc->dlc_TapIns, "Output", 0,
		dlc->dlc_MixerIns, "Input", 1 );
	CHECKRESULT(Result,"ConnectInstrumentParts");

/* Connect the left output of the mixer to the delay. */
	Result = ConnectInstrumentParts (dlc->dlc_MixerIns, "Output", 0,
		dlc->dlc_DelayIns, "Input", 0);
	CHECKRESULT(Result,"ConnectInstrumentParts");

/* Mix for the Delay Line connected to mixer channel 0. */
	SetKnobPart( dlc->dlc_GainKnob, PART_ORIGINAL_SEND, 0.4 );
/* Use negative gain to reduce feedback. */
	SetKnobPart( dlc->dlc_GainKnob, PART_DELAYED_SEND, -0.8 );

/* Mix for the Output instrument connected to Right channel. */
	SetKnobPart( dlc->dlc_GainKnob, PART_ORIGINAL_MIX, 0.49 );
	SetKnobPart( dlc->dlc_GainKnob, PART_DELAYED_MIX, 0.5 );

	return Result;

cleanup:
	TermCustomDelay( dlc );
	return Result;
}

int32 TermCustomDelay( DelayContext *dlc )
{
	DeleteInstrument( dlc->dlc_MixerIns );
	DeleteMixerTemplate( dlc->dlc_MixerTmp );
	UnloadInstrument( dlc->dlc_OutputIns );
	UnloadInstrument( dlc->dlc_DelayIns );
	UnloadInstrument( dlc->dlc_TapIns );
	UnloadSample( dlc->dlc_DelayLine );
	DeleteKnob( dlc->dlc_GainKnob );
	return 0;
}














