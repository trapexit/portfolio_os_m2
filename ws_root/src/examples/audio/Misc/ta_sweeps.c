
/******************************************************************************
**
**  @(#) ta_sweeps.c 96/03/19 1.15
**  $Id: ta_sweeps.c,v 1.25 1995/01/16 19:48:35 vertex Exp phil $
**
******************************************************************************/

/**
|||	AUTODOC -public -class examples -group Audio -name ta_sweeps
|||	Demonstrates adjusting knobs.
|||
|||	  Format
|||
|||	    ta_sweeps
|||
|||	  Description
|||
|||	    This program quickly modulates the amplitude and frequency of a sawtooth
|||	    instrument via tweaking the control knobs repeatedly. The program runs
|||	    for only a few seconds, and cannot be aborted via the control pad.
|||
|||	    Sweep the frequency smoothly as fast as possible to test Knob speed.
|||
|||	  Caveats
|||
|||	    This specific sound effect could have been performed entirely within the
|||	    DSP, freeing up the main processor for other purposes.
|||
|||	  Associated Files
|||
|||	    ta_sweeps.c
|||
|||	  Location
|||
|||	    Examples/Audio/Misc
|||
|||	  See Also
|||
|||	    SetKnob(), triangle.dsp(@)
**/

#include <audio/audio.h>
#include <kernel/operror.h>
#include <kernel/types.h>
#include <stdio.h>

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	PRT(x)

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		PrintError(0,"\\failure in",name,val); \
		goto cleanup; \
	}


int main(int argc, char *argv[])
{
	Item SawIns = 0, OutputIns;
	Item FreqKnob = 0, LoudKnob = 0;
	int32 Result;
	uint32 StartTime, EndTime;
	int32 i, j;

/* Prevent compiler warning. */
	TOUCH(argc);

	PRT(("%s\n", argv[0]));

/* Initialize audio, return if error. */
	if ((Result = OpenAudioFolio()) < 0)
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(Result);
	}

/* Load "directout" for connecting to DAC. */
	OutputIns = LoadInstrument("line_out.dsp",  0,  100);
	CHECKRESULT(OutputIns,"LoadInstrument");

/* Load Sawtooth instrument */
	SawIns = LoadInstrument("sawtooth.dsp",  0,  100);
	CHECKRESULT(SawIns,"LoadInstrument");

/* Connect output of sawtooth to left and right inputs. */
	Result = ConnectInstrumentParts (SawIns, "Output", 0, OutputIns, "Input", 0);
	CHECKRESULT(Result,"ConnectInstrumentParts");
	Result = ConnectInstrumentParts (SawIns, "Output", 0, OutputIns, "Input", 1);
	CHECKRESULT(Result,"ConnectInstrumentParts");

/* Attach the Frequency knob. */
	FreqKnob = CreateKnob( SawIns, "Frequency", NULL );
	CHECKRESULT(FreqKnob,"CreateKnob");
/* Attach the Amplitude knob. */
	LoudKnob = CreateKnob( SawIns, "Amplitude", NULL );
	CHECKRESULT(LoudKnob,"CreateKnob");

/* Start playing without tags. */
	SetKnob( LoudKnob, 0 );
	StartInstrument(SawIns, NULL);
	StartInstrument( OutputIns, NULL );

/* Sweep the frequency while increasing the loudness. */
	StartTime = GetAudioTime();
#define NUM_SWEEPS (50)
#define NUM_SETS (4000)
#define FREQ_START (80.0)
#define FREQ_END (800.0)
#define FREQ_INC ((FREQ_END - FREQ_START)/NUM_SETS)
	for (i=0; i<NUM_SWEEPS; i++)
	{
		float32 Freq;
/* Lower volume to avoid blowing eardrums. */
		SetKnob( LoudKnob, ((float32) i)/NUM_SWEEPS );
		Freq = FREQ_START;
		for(j=0; j<NUM_SETS; j++)
		{
			SetKnob(FreqKnob, Freq);
			Freq += FREQ_INC;
		}
	}
	EndTime = GetAudioTime();

/* Stop all voices of that instrument. */
	StopInstrument(SawIns, NULL);
	StopInstrument(OutputIns, NULL);

	PRT(("%d knob updates completed in %d ticks.\n",
		NUM_SWEEPS*NUM_SETS, EndTime-StartTime ));

cleanup:
/* The Audio Folio is immune to passing NULL values as Items. */
	DeleteKnob( FreqKnob );
	DeleteKnob( LoudKnob );
	UnloadInstrument( SawIns );
	UnloadInstrument( OutputIns );
	CloseAudioFolio();
	return((int) Result);
}




