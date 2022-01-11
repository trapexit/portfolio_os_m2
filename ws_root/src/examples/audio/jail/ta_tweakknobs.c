
/******************************************************************************
**
**  @(#) ta_tweakknobs.c 96/02/22 1.8
**  $Id: ta_tweakknobs.c,v 1.24 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/* !!! uses GetNumKnobs(), which is broken as of 25 Jul 1995 */
/**
|||	!!! disabled AUTODOC -public -class examples -name ta_tweakknobs
|||	Tweaks the knobs available on an instrument.
|||
|||	  Synopsis
|||
|||	    ta_tweakknobs <dsp instrument>
|||
|||	  Description
|||
|||	    Finds the names of all knobs on an instrument and tweaks them to NUMSTEPS
|||	    * (number of knobs) possible permutations. The program demonstrates how to
|||	    find and tweak knobs on an instrument, and is useful for testing
|||	    instrument templates.
|||
|||	    Similar functionality is implemented in the Aria tool.
|||
|||	  Arguments
|||
|||	     <dsp instrument>
|||	        Name of an DSP instrument to tweak. (e.g.
|||	        sawtooth.dsp).
|||
|||	  Associated Files
|||
|||	    ta_tweakknobs.c
|||
|||	  Location
|||
|||	    examples/Audio
|||
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
		PrintError(0,"\\failure in",name,Result); \
		goto cleanup; \
	}

#define MAX_KNOBS 32

Item MixerIns = -1;
Item LeftGainKnob = -1;
Item RightGainKnob = -1;
int32 SetupMixer( void );

int main(int argc, char *argv[])
{
	Item TestIns;
	Item NoiseIns;
	Item timerCue = -1;
	int32 i,j, Val, OldVal;
	int32 NumKnobs, Result = -1;
	char *Name[MAX_KNOBS];
	Item TestKnob[MAX_KNOBS];
	int32 Min[MAX_KNOBS], Max[MAX_KNOBS], Default[MAX_KNOBS], err;

	PRT(("ta_tweakknobs --> mixer\n"));

/* Initialize audio, return if error. */
	if (OpenAudioFolio())
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(-1);
	}

	if (SetupMixer()) return -1;

/* Load instrument definition/template */
	if (argc < 2)
	{
		PRT(("Usage:   ta_tweaknobs <filename.dsp>\n"));
		goto cleanup;
	}

	TestIns = LoadInstrument(argv[1], 0, 100);
	CHECKRESULT(TestIns,"LoadInstrument");
	NoiseIns = LoadInstrument("noise.dsp",  0, 100);
	CHECKRESULT(TestIns,"LoadInstrument noise");

	PRT(("Connect Instruments, Noise -> Input\n"));
	Result = ConnectInstruments (NoiseIns, "Output", TestIns, "Input");
	if (Result < 0) PRT(("%s has no Input\n", argv[1]));

/* Connect to Mixer */
DBUG(("Connect Instrument -> Mixer\n"));
	Result = ConnectInstruments (TestIns, "Output", MixerIns, "Input0");
/*	CHECKRESULT(Result,"ConnectInstruments"); */

/* make timer Cue */
	timerCue = CreateCue(NULL);
	CHECKRESULT(timerCue,"CreateCue");

/* Play a note using StartInstrument and default Freq and Amplitude */
	Result = StartInstrument( TestIns, NULL );

	NumKnobs = GetNumKnobs(TestIns);        /* !!! this function is broken */
	PRT(("%d knobs.\n", NumKnobs));

/* Attach all available knobs */
	for (i=0; i<NumKnobs; i++)
	{
		Name[i] = GetKnobName( TestIns, i);
		if (Name[i] != NULL)
		{
/* Attach knob so we can see what's there. */
			TestKnob[i] = GrabKnob (TestIns, Name[i] );
			CHECKRESULT(TestKnob[i],"GrabKnob");

/* Get attributes of knob. */
            {
                TagArg Tags[] =
                {
                    { AF_TAG_MIN },
                    { AF_TAG_DEFAULT },
                    { AF_TAG_MAX },
                    TAG_END
                };

                err = GetAudioItemInfo ( TestKnob[i], Tags);
                CHECKRESULT(err, "GetKnobInfo");
/* Now Pull Values from TagList */
                Min[i]     = (int32) Tags[0].ta_Arg;
                Default[i] = (int32) Tags[1].ta_Arg;
                Max[i]     = (int32) Tags[2].ta_Arg;
            }
		}
	}

#define NUMSTEPS 1000
/* Slide the value of each knob up with other knobs at default. */
	for (i=0; i<NumKnobs; i++)
	{
/* Slide selected knobs values from Min to Max */
		PRT(("Tweak Knob = %s\n", Name[i]));
		PRT(("   $%x, $%x, $%x\n", Min[i], Default[i], Max[i]));

		OldVal = 123456789;
		for (j=0; j<NUMSTEPS; j++)
		{
			Val = Min[i] + (((Max[i] - Min[i])*j)/NUMSTEPS);
			if (Val != OldVal)
			{
				TweakRawKnob(TestKnob[i], Val);
				OldVal = Val;
			}
			SleepUntilTime (timerCue, GetAudioTime()+1);
		}

/* Restore default. */
		TweakRawKnob(TestKnob[i], Default[i]);
	}

	StopInstrument(TestIns, NULL);

/* The Audio Folio is immune to passing NULL values as Items. */
	for (i=0; i<NumKnobs; i++)
	{
		ReleaseKnob(TestKnob[i]);
	}
	Result = UnloadInstrument( TestIns );
	CHECKRESULT(Result,"UnloadInstrument");
	Result = UnloadInstrument( NoiseIns );
	CHECKRESULT(Result,"UnloadInstrument");
	Result = UnloadInstrument( MixerIns );
	CHECKRESULT(Result,"UnloadInstrument");

	ReleaseKnob( LeftGainKnob );
	ReleaseKnob( RightGainKnob );
cleanup:
	DeleteCue (timerCue);
	CloseAudioFolio();
	PRT(("%s complete\n", argv[0]));
	return((int) Result);
}

/*********************************************************************/
int32 SetupMixer(void)
{
	int32 Result;

	MixerIns = LoadInstrument("mixer4x2.dsp",  0, 100);
	CHECKRESULT(MixerIns,"LoadInstrument");

/* Attach the Left and Right gain knobs. */
	LeftGainKnob = GrabKnob( MixerIns, "LeftGain0" );
	CHECKRESULT(LeftGainKnob,"GrabKnob");
	RightGainKnob = GrabKnob( MixerIns, "RightGain0" );
	CHECKRESULT(RightGainKnob,"GrabKnob");

/* Set Mixer Levels */
	TweakKnob ( LeftGainKnob, 0x2000 );
	TweakKnob ( RightGainKnob, 0x2000 );
/* Mixer must be started */
	Result = StartInstrument( MixerIns, NULL );
	return Result;

cleanup:
	ReleaseKnob( LeftGainKnob );
	ReleaseKnob( RightGainKnob );
	UnloadInstrument( MixerIns );
	return Result;
}



