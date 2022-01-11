
/******************************************************************************
**
**  @(#) ta_pitchnotes.c 96/08/22 1.16
**  $Id: ta_pitchnotes.c,v 1.31 1995/01/16 19:48:35 vertex Exp phil $
**
******************************************************************************/

/**
|||	AUTODOC -public -class examples -group Audio -name ta_pitchnotes
|||	Plays a sample at different MIDI pitches.
|||
|||	  Format
|||
|||	    ta_pitchnotes [<sample file> [duration]]
|||
|||	  Description
|||
|||	    This program loads and plays the AIFF sample file at several different
|||	    pitches. It does this by selecting a MIDI note number, which the audio
|||	    folio maps to a frequency.
|||
|||	  Arguments
|||
|||	    <sample file>
|||	        Name of a sample to be played. The sample should be compatible with
|||	        sampler_16_v1.dsp(@) (16-bit monophonic).
|||
|||	    [duration]
|||	        Duration of each note, in audio ticks. Defaults to 240.
|||
|||	  Associated Files
|||
|||	    ta_pitchnotes.c
|||
|||	  Location
|||
|||	    Examples/Audio/Misc
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
#define STARTNOTE (30)
#define ENDNOTE   (90)

int32 PlayFreqNote ( Item Instrument, int32 Freq, int32 Duration );

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		PrintError(0,"\\failure in",name,Result); \
		goto cleanup; \
	}


int32 PlayPitchNote (Item Instrument, int32 Note, int32 Velocity, int32 Duration, Item timerCue);

int main(int argc, char *argv[])
{
	Item    SamplerIns;
	Item    SampleItem = 0;
	Item    OutputIns = 0;
	Item    Attachment = 0;
	Item    timerCue = -1;
	int32   Duration;
	int32   Result;
	int32   topNote;
	int32   i;

	PRT(("ta_pitchnotes <samplefile>\n"));

/* Initialize audio, return if error. */
	if ((Result = OpenAudioFolio()) < 0)
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(Result);
	}

/* Load Sampler instrument */
	SamplerIns = LoadInstrument(INSNAME, 0, 100);
	CHECKRESULT(SamplerIns,"LoadInstrument");

/* Load digital audio Sample from disk. */
	if (argc > 1)
	{
		SampleItem = LoadSample(argv[1]);
	}
	else
	{
		SampleItem = LoadSample("/remote/Samples/PitchedL/SynthBass/BassSynth.C3LM44k.aiff");
	}
	CHECKRESULT(SampleItem,"LoadSample");

	Duration = (argc > 2) ? atoi(argv[2]) : 240 ;
	PRT(("Duration = %d\n", Duration));

/* Look at sample information. */
	DebugSample(SampleItem);

/* Figure out which note is the top note that can be played using this sample. */
	{
		TagArg tags[2];
		tags[0].ta_Tag = AF_TAG_BASENOTE;
		tags[1].ta_Tag = TAG_END;
		Result = GetAudioItemInfo( SampleItem, tags );
		CHECKRESULT(Result,"GetAudioItemInfo");
/*
 * AudioFolio will play up to one octave above base note.
 * For 12 tone equal tempered tuning, that is 12 semitones.
 */
		topNote = ((uint32) tags[0].ta_Arg) + 12;
	}

	OutputIns = LoadInstrument("line_out.dsp",  0,  100);
	CHECKRESULT(OutputIns,"LoadInstrument");

/* Connect Sampler to Output */
	Result = ConnectInstrumentParts (SamplerIns, "Output", 0, OutputIns, "Input", 0);
	CHECKRESULT(Result,"ConnectInstrumentParts");
	Result = ConnectInstrumentParts (SamplerIns, "Output", 0, OutputIns, "Input", 1);
	CHECKRESULT(Result,"ConnectInstrumentParts");

/* Attach the sample to the instrument. */
	Attachment = CreateAttachment(SamplerIns, SampleItem, NULL);
	CHECKRESULT(Attachment,"CreateAttachment");

/* make timer Cue */
	timerCue = CreateCue(NULL);
	CHECKRESULT(timerCue,"CreateCue");

	Result = StartInstrument( OutputIns, NULL );
	CHECKRESULT(Result,"StartInstrument OutputIns");

/* Play several notes using a conveniance routine. */
	for(i=STARTNOTE; i<ENDNOTE; i++)
	{
		PRT(( "Pitch = %d\n", i ));
		if( i == topNote )
		{
			PRT(("AudioFolio will not play more than 1 octave above BASENOTE!\n"));
		}
		PlayPitchNote( SamplerIns, i, 100, Duration, timerCue );
	}

cleanup:
	DeleteCue (timerCue);
	DeleteAttachment( Attachment );
	UnloadInstrument( SamplerIns );
	UnloadSample( SampleItem );

	UnloadInstrument( OutputIns );
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
