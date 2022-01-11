
/******************************************************************************
**
**  @(#) minmax_audio.c 96/03/19 1.10
**  $Id: minmax_audio.c,v 1.7 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC -public -class examples -group Audio -name minmax_audio
|||	Measures the maximum and minimum output from the DSP.
|||
|||	  Format
|||
|||	    minmax_audio
|||
|||	  Description
|||
|||	    Samples the output of the DSP and returns its maximum and minimum output
|||	    values. You can use minmax_audio to check that your program outputs are
|||	    reasonable, non-clipping levels of audio.
|||
|||	    The program creates instances of probes from the instruments minimum.dsp(@)
|||	    and maximum.dsp(@). The probes are queried with ReadProbe() every five
|||	    seconds, or 5*240 audio ticks (5 seconds by default).
|||
|||	    Signal range is -1.0 to 1.0.
|||
|||	    This program runs until it is killed.
|||
|||	  Caveats
|||
|||	    The loudness of audio is often subjective. Check the loudness of your
|||	    program by first playing a standard audio CD on a development station, and
|||	    adjusting the volume of your sound system to a reasonable level. The 3DO
|||	    program should then produce sounds at a similar volume.
|||
|||	  Associated Files
|||
|||	    minmax_audio.c
|||
|||	  Location
|||
|||	    Examples/Audio/Misc
**/

#include <audio/audio.h>
#include <kernel/mem.h>
#include <kernel/operror.h>
#include <kernel/types.h>
#include <stdarg.h>
#include <stdio.h>

#define NUMCHANNELS (2)   /* Stereo */

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		PrintError(0,"\\failure in",name,Result); \
		goto cleanup; \
	}

/***********************************************************************/
int main(int argc, char *argv[])
{
/* Declare local variables */
	Item SleepCue = 0;
	int32 Result;
	Item TapIns;
	Item LeftMaxIns = 0, LeftMinIns = 0;
	Item LeftMaxProbe = 0, LeftMinProbe = 0;
	float32 LeftMaxVal = 0, LeftMinVal = 0;

	Item RightMaxIns = 0, RightMinIns = 0;
	Item RightMaxProbe = 0, RightMinProbe = 0;
	float32 RightMaxVal = 0, RightMinVal = 0;

	PRT(("%s\n", argv[0] ));

/* Prevent compiler warning. */
	TOUCH(argc);

/* Initialize audio, return if error. */
	if ((Result = OpenAudioFolio()) < 0)
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(Result);
	}

/* Load instrument to tap output.  Zero priority to be at end. */
	TapIns = LoadInstrument( "tapoutput.dsp",  0, 0);
	CHECKRESULT(TapIns,"LoadInstrument");

/* Create a Cue for signalback */
	SleepCue = CreateCue( NULL );
	CHECKRESULT(SleepCue, "CreateCue");

	LeftMinIns = LoadInstrument("minimum.dsp", 0, 50);
	CHECKRESULT(LeftMinIns,"LoadInstrument");
	LeftMaxIns = LoadInstrument("maximum.dsp", 0, 50);
	CHECKRESULT(LeftMaxIns,"LoadInstrument");

	LeftMaxProbe = CreateProbe( LeftMaxIns, "Output", NULL );
	CHECKRESULT(LeftMaxProbe,"CreateProbe");
	LeftMinProbe = CreateProbe( LeftMinIns, "Output", NULL );
	CHECKRESULT(LeftMinProbe,"CreateProbe");

	Result = ConnectInstrumentParts( TapIns, "Output", 0, LeftMaxIns, "InputA", 0 );
	CHECKRESULT(Result,"ConnectInstrumentParts");
	Result = ConnectInstruments( LeftMaxIns, "Output", LeftMaxIns, "InputB" );
	CHECKRESULT(Result,"ConnectInstruments");
	Result = ConnectInstrumentParts( TapIns, "Output", 0, LeftMinIns, "InputA", 0 );
	CHECKRESULT(Result,"ConnectInstrumentParts");
	Result = ConnectInstruments( LeftMinIns, "Output", LeftMinIns, "InputB" );
	CHECKRESULT(Result,"ConnectInstruments");


	RightMinIns = LoadInstrument("minimum.dsp", 0, 50);
	CHECKRESULT(RightMinIns,"LoadInstrument");
	RightMaxIns = LoadInstrument("maximum.dsp", 0, 50);
	CHECKRESULT(RightMaxIns,"LoadInstrument");

	RightMaxProbe = CreateProbe( RightMaxIns, "Output", NULL );
	CHECKRESULT(RightMaxProbe,"CreateProbe");
	RightMinProbe = CreateProbe( RightMinIns, "Output", NULL );
	CHECKRESULT(RightMinProbe,"CreateProbe");

	Result = ConnectInstrumentParts( TapIns, "Output", 1, RightMaxIns, "InputA", 0 );
	CHECKRESULT(Result,"ConnectInstrumentParts");
	Result = ConnectInstruments( RightMaxIns, "Output", RightMaxIns, "InputB" );
	CHECKRESULT(Result,"ConnectInstruments");
	Result = ConnectInstrumentParts( TapIns, "Output", 1, RightMinIns, "InputA", 0 );
	CHECKRESULT(Result,"ConnectInstrumentParts");
	Result = ConnectInstruments( RightMinIns, "Output", RightMinIns, "InputB" );
	CHECKRESULT(Result,"ConnectInstruments");

/* Start capturing sound. */
	Result = StartInstrument( TapIns, NULL );
	CHECKRESULT(Result,"StartInstrument");

	Result = StartInstrument( LeftMaxIns, NULL );
	CHECKRESULT(Result,"StartInstrument");
	Result = StartInstrument( LeftMinIns, NULL );
	CHECKRESULT(Result,"StartInstrument");

	Result = StartInstrument( RightMaxIns, NULL );
	CHECKRESULT(Result,"StartInstrument");
	Result = StartInstrument( RightMinIns, NULL );
	CHECKRESULT(Result,"StartInstrument");

/* Loop while reading probe. */
	while(1)
	{
/* This is the right way. */
		Result = ReadProbe( LeftMaxProbe, &LeftMaxVal );
		CHECKRESULT( Result, "ReadProbe" );
		Result = ReadProbe( LeftMinProbe, &LeftMinVal );
		CHECKRESULT( Result, "ReadProbe" );
		Result = ReadProbe(RightMaxProbe, &RightMaxVal );
		CHECKRESULT( Result, "ReadProbe" );
		Result = ReadProbe(RightMinProbe, &RightMinVal );
		CHECKRESULT( Result, "ReadProbe" );

		PRT(("\n----\nLeftMin  = %8g, LeftMax  = %8g\n", LeftMinVal, LeftMaxVal ));
		PRT(("RightMin = %8g, RightMax = %8g\n", RightMinVal,RightMaxVal ));
		SleepUntilTime( SleepCue, GetAudioTime() + 5*240 );
	}

cleanup:
	UnloadInstrument( LeftMaxIns );
	UnloadInstrument( LeftMinIns );
	DeleteProbe( LeftMaxProbe );
	DeleteProbe( LeftMinProbe );

	UnloadInstrument( RightMaxIns );
	UnloadInstrument( RightMinIns );
	DeleteProbe( RightMaxProbe );
	DeleteProbe( RightMinProbe );

	DeleteCue( SleepCue );
	UnloadInstrument( TapIns );

	CloseAudioFolio();
	PRT(("All done.\n"));
	return((int) Result);
}
