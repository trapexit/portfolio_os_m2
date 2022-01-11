
/******************************************************************************
**
**  @(#) ta_tuning.c 96/03/19 1.15
**  $Id: ta_tuning.c,v 1.19 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC -public -class examples -group Audio -name ta_tuning
|||	Demonstrates custom tuning a DSP instrument.
|||
|||	  Format
|||
|||	    ta_tuning
|||
|||	  Description
|||
|||	    Demonstrates how to create a tuning table, how to create a tuning, and how
|||	    to apply a tuning to an instrument.
|||
|||	  Associated Files
|||
|||	    ta_tuning.c
|||
|||	  Location
|||
|||	    Examples/Audio/Misc
|||
**/

#include <audio/audio.h>
#include <kernel/operror.h>
#include <kernel/types.h>
#include <misc/frac16.h>
#include <stdio.h>

#define TEST_TEMPLATE
#define OSCINSNAME "triangle.dsp"
#define OUTPUTNAME "line_out.dsp"

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	PRT(x)

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		ERR(("Failure in %s: $%x\n", name, val)); \
		PrintfSysErr( val ); \
		goto cleanup; \
	}

#define NUMINTERVALS (5)
#define NOTESPEROCTAVE NUMINTERVALS
#define BASENOTE (AF_A440_PITCH)
#define BASEFREQ (440.0)  /* A440 */
float32 TuningTable[NOTESPEROCTAVE];

/***************************************************************/
int32 PlayPitchNote ( Item Instrument, int32 Note, int32 Velocity, Item timerCue )
{
	/*
		Notes:
			. Error trapping has been removed for brevity.
	*/

	StartInstrumentVA (Instrument,
	                   AF_TAG_VELOCITY, Velocity,
	                   AF_TAG_PITCH,    Note,
	                   TAG_END);
	SleepUntilTime (timerCue, GetAudioTime()+20);

	ReleaseInstrument( Instrument, NULL);
	SleepUntilTime (timerCue, GetAudioTime()+30);

	return 0;
}

/***************************************************************/
int main( int argc, char *argv[] )
{
	Item  OscTmp = 0;
	Item  OscIns = 0;
	Item  OutputIns;
	Item  Slendro = -1;
	Item  timerCue = -1;
	int32 Result;
	int32 i;
	float32 BaseFreq;

	TOUCH(argc);

	PRT(("\n%s V27.0\n", argv[0]));

/* Initialize audio, return if error. */
	if ((Result = OpenAudioFolio()) < 0)
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(Result);
	}

/* Use directout instead of mixer. */
	OutputIns = LoadInstrument( OUTPUTNAME,  0, 100);
	CHECKRESULT(OutputIns,"LoadInstrument");
	StartInstrument( OutputIns, NULL );

/* Load template of Oscillator instrument */
	OscTmp = LoadInsTemplate( OSCINSNAME, NULL );
	CHECKRESULT(OscTmp,"LoadInsTemplate");

/* Make an instrument based on template. */
	OscIns = CreateInstrument(OscTmp, NULL);
	CHECKRESULT(OscIns,"CreateInstrument");

/* Connect output of oscillator to left and right inputs. */
	Result = ConnectInstrumentParts (OscIns, "Output", 0, OutputIns, "Input", 0);
	CHECKRESULT(Result,"ConnectInstrumentParts");
	Result = ConnectInstrumentParts (OscIns, "Output", 0, OutputIns, "Input", 1);
	CHECKRESULT(Result,"ConnectInstrumentParts");

/* make timer cue for PlayPitchNote() */
	timerCue = CreateCue(NULL);
	CHECKRESULT(timerCue,"CreateCue");

/* Play an ascending scale using the default 12 toned equal tempered tuning. */
	PRT(("12 tone equal tempered scale.\n"));
	for (i=(BASENOTE - (2*NOTESPEROCTAVE)); i<(BASENOTE + (2*NOTESPEROCTAVE)); i++)
	{
		PlayPitchNote( OscIns, i, 80, timerCue );
	}

/* Create a custom just intoned pentatonic tuning. */
/* Calculate frequencies as ratios from the base frequency. */
	BaseFreq = BASEFREQ;
	TuningTable[0] = BaseFreq;   /* 1:1 */
	TuningTable[1] = BaseFreq * 8.0 / 7.0;
	TuningTable[2] = BaseFreq * 5.0 / 4.0;
	TuningTable[3] = BaseFreq * 3.0 / 2.0;
	TuningTable[4] = BaseFreq * 7.0 / 4.0;

/* Create a tuning item that can be used with many instruments. */
	Slendro = CreateTuning( TuningTable, NUMINTERVALS, NOTESPEROCTAVE, BASENOTE );
	CHECKRESULT(Slendro,"CreateTuning");

/* Tell an instrument to use this tuning. */
#ifdef TEST_TEMPLATE
	PRT(("Tune Template\n"));
	Result = TuneInsTemplate( OscTmp, Slendro );
	CHECKRESULT(Result,"TuneInsTemplate");
/* Make a new instrument based on template. */
	DeleteInstrument( OscIns );
	OscIns = CreateInstrument(OscTmp, NULL);
	CHECKRESULT(OscIns,"CreateInstrument");
/* Connect output of oscillator to left and right inputs. */
	Result = ConnectInstrumentParts (OscIns, "Output", 0, OutputIns, "Input", 0);
	CHECKRESULT(Result,"ConnectInstrumentParts");
	Result = ConnectInstrumentParts (OscIns, "Output", 0, OutputIns, "Input", 1);
	CHECKRESULT(Result,"ConnectInstrumentParts");

#else
	PRT(("Tune Instrument\n"));
	Result = TuneInstrument( OscIns, Slendro );
	CHECKRESULT(Result,"TuneInstrument");
#endif

/* Play the same ascending scale using the custom tuning. */
	PRT(("Custom pentatonic scale.\n"));
	for (i=50; i<(BASENOTE + (2*NOTESPEROCTAVE)); i++)
	{
		PlayPitchNote( OscIns, i, 80, timerCue );
	}

cleanup:
	DeleteTuning (Slendro);
	DeleteCue (timerCue);
	DeleteInstrument( OscIns );
	UnloadInsTemplate( OscTmp );
	UnloadInstrument( OutputIns );
	PRT(("%s all done.\n", argv[0]));
	CloseAudioFolio();
	return((int) Result);
}
