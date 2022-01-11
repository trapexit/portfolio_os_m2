
/******************************************************************************
**
**  @(#) tone.c 96/03/19 1.13
**  $Id: beep.c,v 1.6 1995/01/16 19:48:35 vertex Exp phil $
**
******************************************************************************/

/**
|||	AUTODOC -public -class examples -group Audio -name tone
|||	Simple audio demonstration.
|||
|||	  Format
|||
|||	    tone
|||
|||	  Description
|||
|||	    Plays synthetic waveform for 2 seconds. This demonstrates loading,
|||	    connecting and playing instruments. It also demonstrates use of the audio
|||	    timer for time delays.
|||
|||	  Associated Files
|||
|||	    tone.c
|||
|||	  Location
|||
|||	    Examples/Audio/Misc
|||
**/

#include <audio/audio.h>
#include <kernel/operror.h>
#include <kernel/types.h>
#include <stdio.h>

#define	PRT(x)	{ printf x; }

#define INS_NAME "triangle.dsp"

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		PrintError(NULL, name, NULL, Result); \
		goto cleanup; \
	}

int main( int32 argc, char *argv[])
{
	Item OscIns = -1;
	Item OutputIns = -1;
	Item SleepCue = -1;
	float32 TicksPerSecond;
	int32 Result;

	PRT(("%s\n", argv[0]));

/* Eliminate compiler warning because argc is never used. */
	TOUCH(argc);

/* Initialize audio, return if error. */
	if (OpenAudioFolio() < 0)
	{
		PRT(("Audio Folio could not be opened!\n"));
		return(-1);
	}
/*
** The audio clock rate is usually around 240 ticks per second.
** It is possible to change the rate using SetAudioClockRate().
** We can query the audio rate by calling GetAudioClockRate().
*/
	Result = GetAudioClockRate( AF_GLOBAL_CLOCK, &TicksPerSecond );
	CHECKRESULT(Result,"GetAudioClockRate");

/*
** Create a Cue item that we can use with the Audio Timer functions.
** It contains a Signal that is used to wake us up.
*/
	SleepCue = CreateCue( NULL );
	CHECKRESULT(SleepCue,"CreateCue");

/*
** Load "line_out" for connecting to DAC.
** You must connect to a "line_out.dsp" or a mixer for
** the sound to be heard.
*/
	OutputIns = LoadInstrument("line_out.dsp",  0,  100);
	CHECKRESULT(OutputIns,"LoadInstrument");

/* Load description of synthetic waveform instrument */
	OscIns = LoadInstrument( INS_NAME, 0, 100);
	CHECKRESULT(OscIns,"LoadInstrument");

/* Connect output of sawtooth to left and right inputs. */
	Result = ConnectInstrumentParts (OscIns, "Output", 0, OutputIns, "Input", 0);
	CHECKRESULT(Result,"ConnectInstrumentParts");
	Result = ConnectInstrumentParts (OscIns, "Output", 0, OutputIns, "Input", 1);
	CHECKRESULT(Result,"ConnectInstrumentParts");

/*
** Start the line_out instrument so that you hear all of the
** sawtooth output.
*/
	Result = StartInstrument( OutputIns, NULL );
	CHECKRESULT(Result,"StartInstrument OutputIns");

/*
** Play a note using StartInstrument.
** You can pass optional TagArgs to control pitch or amplitude.
*/
	Result = StartInstrument( OscIns, NULL );
	CHECKRESULT(Result,"StartInstrument OscIns");

/*
** Go to sleep for about 2 seconds.
*/
	SleepUntilTime( SleepCue, GetAudioTime() + ( 2.0 * TicksPerSecond ) );

/* Now stop the sound. */
	StopInstrument(OscIns, NULL);
	StopInstrument(OutputIns, NULL);


cleanup:
	DisconnectInstrumentParts (OutputIns, "Input", 0);
	DisconnectInstrumentParts (OutputIns, "Input", 1);
	UnloadInstrument( OscIns );
	UnloadInstrument( OutputIns );
	DeleteCue( SleepCue );
	CloseAudioFolio();
	PRT(("%s all done now.\n", argv[0]));
	return((int) Result);
}
