
/******************************************************************************
**
**  @(#) idle_audio.c 96/08/07 1.6
**  $Id: idle_audio.c,v 1.1 1995/03/23 05:37:00 phil Exp phil $
**
******************************************************************************/

/* Make the simulator spin by calling ReadProbe repeatedly. */

#include <audio/audio.h>
#include <kernel/operror.h>
#include <kernel/task.h>
#include <stdio.h>

#define	PRT(x)	{ printf x; }

#define INS_NAME "sawtooth.dsp"

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
	Item OscIns;
	Item ProbeItem;
	float32 Val;
	int32 Result;

	TOUCH(argc);

	PRT(("%s\n", argv[0]));

/* Initialize audio, return if error. */
	if (OpenAudioFolio() < 0)
	{
		PRT(("Audio Folio could not be opened!\n"));
		return(-1);
	}

/* Load description of synthetic waveform instrument */
	OscIns = LoadInstrument( "sawtooth.dsp", 0, 100 );
	CHECKRESULT(OscIns,"LoadInstrument");

/* Create Probe so we can get output. */
	ProbeItem = CreateProbe( OscIns, "Output", NULL );
	CHECKRESULT(ProbeItem,"CreateProbe");

/* Probe output and print result. */
	while(1)
	{
		Result = ReadProbe( ProbeItem, &Val );
		CHECKRESULT(Result,"ReadProbe");
		Yield();
	}

cleanup:
	UnloadInstrument( OscIns );
	CloseAudioFolio();
	return((int) Result);
}
