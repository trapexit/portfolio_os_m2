/* @(#) leftright.c 95/12/07 1.8 */
/* $Id: leftright.c,v 1.3 1994/10/25 00:13:00 phil Exp phil $ */
/***************************************************************
**
** Test Left/Right channel and phase using impulse train.
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
***************************************************************/

#include <kernel/types.h>
#include <kernel/debug.h>
#include <kernel/operror.h>
#include <file/filefunctions.h>
#include <audio/audio.h>
#include <stdio.h>
#include <stdlib.h>

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	PRT(x)

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		ERR(("Failure in %s: $%x\n", name, val)); \
		goto cleanup; \
	}

int main( int32 argc, char *argv[])
{
	Item PulseIns = 0;
	Item FreqKnob = 0;
	Item timerCue = -1;
	int32 Result;
	Item OutputIns;
	int32 Duration;

	PRT(("%s <ticks>\n", argv[0]));

	Duration = (argc > 1) ? atoi( argv[1] ) : 1000;

/* Initialize audio, return if error. */
	if ((Result = OpenAudioFolio()) < 0)
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(Result);
	}

/* Load "directout" for connecting to DAC. */
	OutputIns = LoadInstrument("line_out.dsp",  0,  100);
	CHECKRESULT(OutputIns,"LoadInstrument");

/* Make an instrument based on template. */
	PulseIns = LoadInstrument("impulse.dsp", 0, 100);
	CHECKRESULT(PulseIns,"LoadInstrument");

/* make timer Cue */
	timerCue = CreateCue(NULL);
	CHECKRESULT(timerCue,"CreateCue");

/* Attach the Frequency knob. */
	FreqKnob = CreateKnob( PulseIns, "Frequency", NULL );
	CHECKRESULT(FreqKnob,"CreateKnob");

	Result = SetKnob(FreqKnob, 200.0 );
	CHECKRESULT(Result,"SetKnob");

/* Play a note using StartInstrument */
	Result = StartInstrument( PulseIns, NULL );
	CHECKRESULT(Result,"StartInstrument");
	Result = StartInstrument( OutputIns, NULL );
	CHECKRESULT(Result,"StartInstrument");

/* Connect output of oscillator to left. */
	PRT(("Left\n"));
	Result = ConnectInstrumentParts (PulseIns, "Output", 0, OutputIns, "Input", 0);
	CHECKRESULT(Result,"ConnectInstrumentParts");
	SleepUntilTime(timerCue, GetAudioTime()+500);
	Result = DisconnectInstrumentParts (OutputIns, "Input", 0 );
	CHECKRESULT(Result,"DisconnectInstrumentParts");

/* Raise the pitch for the right side */
	Result = SetKnob(FreqKnob, 300.0 );
	CHECKRESULT(Result,"SetKnob");
	SleepUntilTime(timerCue, GetAudioTime()+60);

	PRT(("Right\n"));
	Result = ConnectInstrumentParts (PulseIns, "Output", 0, OutputIns, "Input", 1);
	CHECKRESULT(Result,"ConnectInstrumentParts");
	SleepUntilTime(timerCue, GetAudioTime()+500);
	Result = DisconnectInstrumentParts (OutputIns, "Input", 1 );
	CHECKRESULT(Result,"DisconnectInstrumentParts");


/* Raise the pitch for both */
	Result = SetKnob(FreqKnob, 400.0 );
	CHECKRESULT(Result,"SetKnob");
	SleepUntilTime(timerCue, GetAudioTime()+60);

	PRT(("Both\n"));
	Result = ConnectInstrumentParts (PulseIns, "Output", 0, OutputIns, "Input", 0);
	CHECKRESULT(Result,"ConnectInstrumentParts");
	Result = ConnectInstrumentParts (PulseIns, "Output", 0, OutputIns, "Input", 1);
	CHECKRESULT(Result,"ConnectInstrumentParts");

	SleepUntilTime(timerCue, GetAudioTime()+Duration);

	PRT(("%s all done.\n", argv[0]));
	StopInstrument(PulseIns, NULL);
	StopInstrument(OutputIns, NULL);

cleanup:
/* The Audio Folio is immune to passing NULL values as Items. */
	DeleteCue (timerCue);
	DeleteKnob( FreqKnob);
	UnloadInstrument( PulseIns );
	UnloadInstrument( OutputIns );
	CloseAudioFolio();
	return((int) Result);
}



