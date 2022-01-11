
/******************************************************************************
**
**  @(#) playsample.c 96/08/27 1.19
**  $Id: playsample.c,v 1.12 1995/01/16 19:48:35 vertex Exp phil $
**
******************************************************************************/

/**
|||	AUTODOC -public -class examples -group Audio -name playsample
|||	Plays an AIFF sample in memory using the control pad.
|||
|||	  Format
|||
|||	    playsample [<sample file> [rate]]
|||
|||	  Description
|||
|||	    This program shows how to load an AIFF sample file and play it using the
|||	    control pad. Use the A button to start the sample, the B button to release
|||	    the sample, and the C button to stop the sample. The X button quits the
|||	    program.
|||
|||	    The playback rate will default to the sample rate at which the
|||	    sample was recorded.  Thus it should sound normal.
|||
|||	  Arguments
|||
|||	    <sample file>
|||	        Name of a compatible AIFF sample file. If not specified, defaults to
|||	        loading the standard system sample sinewave.aiff.
|||
|||	    [rate]
|||	        Sample rate in Hertz (e.g., 22050). Defaults to the sample rate stored
|||	        in the sample file.
|||
|||	  Associated Files
|||
|||	    playsample.c
|||
|||	  Location
|||
|||	    Examples/Audio/Misc
**/

#include <audio/audio.h>
#include <audio/parse_aiff.h>
#include <kernel/operror.h>
#include <misc/event.h>
#include <stdio.h>
#include <stdlib.h>

/* Handy printing and debugging macros. */
#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		PrintError(0,"\\failure in",name,val); \
		goto cleanup; \
	}

/************************************************************/
int main(int argc, char *argv[])
{
	Item OutputIns = 0;
	Item SamplerIns = 0;
	Item SampleItem = 0;
	Item Attachment = 0;
	char *SampleName = NULL;
	float32 sampleRate = 0;     /* only used if IfVariable is set to TRUE */
	bool IfVariable = FALSE;
	int32 Result;
	int32 DoIt = TRUE;
	char InstrumentName[AF_MAX_NAME_SIZE];
	uint32 Buttons;
	ControlPadEventData cped;

	PRT(("Usage: %s <samplefile> <rate>\n", argv[0]));

/* Get optional argumants from command line. */
	if (argc > 1) SampleName = argv[1];
	if (argc > 2)
	{
		sampleRate = strtof (argv[2], NULL);
		IfVariable = TRUE;
	}

/* Print menu of button commands. */
	PRT(("Button Menu:\n"));
	PRT(("   A = Start\n"));
	PRT(("   B = Release\n"));
	PRT(("   C = Stop\n"));
	PRT(("   X = Exit\n"));

/* Initialize audio, return if error. */
	Result = OpenAudioFolio();
	if (Result < 0)
	{
		PrintError(0,"Audio Folio could not be opened.",0,Result);
		return(-1);
	}

/* Initialize the EventBroker. */
	Result = InitEventUtility(1, 0, TRUE);
	if (Result < 0)
	{
		PrintError(0,"InitEventUtility",0,Result);
		goto cleanup;
	}

/* Load a directout instrument to send the sound to the DAC. */
	OutputIns = LoadInstrument("line_out.dsp", 0, 100);
	CHECKRESULT(OutputIns,"LoadInstrument");
	StartInstrument( OutputIns, NULL );

/* Load digital audio AIFF Sample file from disk. If a sample name was
** specified on the command line, load it. Otherwise load the standard system
** sample sinewave.aiff. */
	if (SampleName) {
		SampleItem = LoadSample(SampleName);
		CHECKRESULT(SampleItem,"LoadSample");
	}
	else {
		SampleItem = LoadSystemSample("sinewave.aiff");
		CHECKRESULT(SampleItem,"Load sinewave.aiff");
	}

/* Look at sample information for fun. */
	DebugSample(SampleItem);

/*
** Select an apropriate sample playing instrument based on sample format. Note
** that we might get a variable rate instrument, even if IfVariable is FALSE,
** because SampleItemToInsName() picks an instrument which is capable of playing
** the sample at its original sample rate. Fixed-rate sample players run at 44100.
*/
	Result = SampleItemToInsName( SampleItem, IfVariable, InstrumentName, AF_MAX_NAME_SIZE );
	if (Result < 0)
	{
		ERR(("No instrument to play that sample.\n"));
		goto cleanup;
	}

/* Load Sampler instrument */
	PRT(("Use instrument: %s\n", InstrumentName));
	SamplerIns = LoadInstrument(InstrumentName,  0, 100);
	CHECKRESULT(SamplerIns,"LoadInstrument");

/* Connect Sampler Instrument to DirectOut. Works for mono or stereo. */
	Result = ConnectInstrumentParts (SamplerIns, "Output", 0, OutputIns, "Input", 0);
	CHECKRESULT(Result,"ConnectInstruments");
	Result = ConnectInstrumentParts (SamplerIns, "Output", 1, OutputIns, "Input", 1);  /* Try stereo. */
	if( Result < 0 )
	{
		Result = ConnectInstrumentParts (SamplerIns, "Output", 0, OutputIns, "Input", 1);
		CHECKRESULT(Result,"ConnectInstruments");
	}

/* Attach the sample to the instrument for playback. */
	Attachment = CreateAttachment(SamplerIns, SampleItem, NULL);
	CHECKRESULT(Attachment,"CreateAttachment");

/*
** Start the instrument at the given sample rate.
** Note that adjusting the rate of a fixed-rate sample player has no effect.
** You could also pass AF_TAG_PITCH and AF_TAG_AMPLITUDE_FP to StartInstrument().
** See StartInstrument() documentation for more detail.
*/
	if( IfVariable )
	{
/*
** Attempt to play sample at the sample rate specified on the command line.
*/
		StartInstrumentVA (SamplerIns,
			AF_TAG_SAMPLE_RATE_FP, ConvertFP_TagData(sampleRate),
			TAG_END);
	}
	else
	{
/*
** Play sample at originally recorded sample rate. Specifying AF_TAG_DETUNE_FP
** in this way causes the sample to be played back at its original sample rate.
** This parameter is ignored in the case where a fixed-rate sample player was
** selected, but is necessary for causing variable-rate sample players to play
** the sample back at its original sample rate.
*/
		StartInstrumentVA (SamplerIns,
			AF_TAG_DETUNE_FP, ConvertFP_TagData(1.0),
			TAG_END);
	}

/* Interactive event loop. */
	while(DoIt)
	{
/* Get User input. */
		Result = GetControlPad (1, TRUE, &cped);
		if (Result < 0) {
			PrintError(0,"read control pad in","PlaySoundFile",Result);
		}
		Buttons = cped.cped_ButtonBits;

/* Process buttons pressed. */
		if(Buttons & ControlX) /* EXIT */
		{
			DoIt = FALSE;
		}
		if(Buttons & ControlA) /* START */
		{
			if( IfVariable )
			{
				Result = StartInstrumentVA (SamplerIns,
					AF_TAG_SAMPLE_RATE_FP, ConvertFP_TagData(sampleRate),
					TAG_END);
			}
			else
			{
					/* Play sample at originally recorded sample rate. */
				Result = StartInstrumentVA (SamplerIns,
					AF_TAG_DETUNE_FP, ConvertFP_TagData(1.0),
					TAG_END);
			}
			CHECKRESULT(Result,"StartInstrument");
		}
		if(Buttons & ControlB) /* RELEASE */
		{
			Result = ReleaseInstrument( SamplerIns, NULL );
			CHECKRESULT(Result,"ReleaseInstrument");
		}
		if(Buttons & ControlC) /* STOP */
		{
			Result = StopInstrument( SamplerIns, NULL );
			CHECKRESULT(Result,"StopInstrument");
		}
	}

	Result = StopInstrument( SamplerIns, NULL );
	CHECKRESULT(Result,"StopInstrument");

cleanup:

	DeleteAttachment( Attachment );
	UnloadSample( SampleItem );
	UnloadInstrument( SamplerIns );
	UnloadInstrument( OutputIns );

/* Cleanup the EventBroker. */
	KillEventUtility();
	CloseAudioFolio();
	PRT(( "%s finished.\n", argv[0] ));
	return((int) Result);
}
