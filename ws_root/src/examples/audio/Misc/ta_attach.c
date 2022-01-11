
/******************************************************************************
**
**  @(#) ta_attach.c 96/07/31 1.17
**  $Id: ta_attach.c,v 1.21 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC -public -class examples -group Audio -name ta_attach
|||	Experiments with sample attachments.
|||
|||	  Format
|||
|||	    ta_attach
|||
|||	  Description
|||
|||	    Creates a pair of software-generated samples and attaches them to a sample
|||	    player instrument. The attachments are then linked to one another using
|||	    LinkAttachments() such that they play in a loop.
|||
|||	    This demonstrates how multiple attachments can be linked together to form
|||	    a queue of sample buffers to play and how a client can receive
|||	    notification of when each sample buffer has finished playing. This is the
|||	    technique that the Sound Spooler uses to queue sound buffers for playback.
|||
|||	  Associated Files
|||
|||	    ta_attach.c
|||
|||	  Location
|||
|||	    Examples/Audio/Misc
|||
|||	  See Also
|||
|||	    ta_spool(@), ssplCreateSoundSpooler(), LinkAttachments()
**/

#include <audio/audio.h>
#include <kernel/mem.h>
#include <kernel/operror.h>
#include <kernel/task.h>        /* WaitSignal() */
#include <stdio.h>

/* sample parameters */
#define SAMPWIDTH sizeof(int16)
#define NUM_CHANNELS (1)   /* Stereo or Mono */
#define NUM_FRAMES ((128*1024) + 1)
#define SAMPSIZE (SAMPWIDTH*NUM_FRAMES*NUM_CHANNELS)

/* assorted macros */
#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */

#define NUM_LOOPS  (7)

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		PrintError(0,"\\failure in",name,Result); \
		goto cleanup; \
	}

/* Local function prototypes. */
static int32 BuildSample ( Item *SampPtr, Item *CuePtr, int32 Octave );

int main( void )
{
/* Declare local variables */
	Item OutputIns = 0;
	Item Samp1 = 0, Samp2 = 0;
	Item Att1 = 0, Att2 = 0;
	int32 Signal1, Signal2, SignalHit;
	Item Cue1 = 0, Cue2 = 0;
	Item SamplerIns = 0;
	int32 atnum, i;
	int32 Result;

/* Initialize audio, return if error. */
	if ((Result = OpenAudioFolio()) < 0)
	{
		ERR(("Audio Folio could not be opened!\n"));
		goto cleanup_nofolio;
	}

#ifdef MEMDEBUG
	if ((Result = CreateMemDebug ( NULL )) < 0) goto cleanup;

	if ((Result = ControlMemDebug ( MEMDEBUGF_ALLOC_PATTERNS |
	/**/                            MEMDEBUGF_FREE_PATTERNS |
	/**/                            MEMDEBUGF_PAD_COOKIES |
	/**/                            MEMDEBUGF_CHECK_ALLOC_FAILURES |
	/**/                            MEMDEBUGF_KEEP_TASK_DATA)) < 0) goto cleanup;
#else
	TOUCH(OutputIns);   /* silence a compiler warning when MEMDEBUG isn't on */
#endif

/* Use directout instead of mixer. */
	OutputIns = LoadInstrument("line_out.dsp",  0,  100);
	CHECKRESULT(OutputIns,"LoadInstrument");
	StartInstrument( OutputIns, NULL );

/* Load fixed rate stereo sample player instrument */
	SamplerIns = LoadInstrument("sampler_16_f2.dsp",  0, 100);
	CHECKRESULT(SamplerIns,"LoadInstrument");

/* Connect Sampler Instrument to DirectOut */
	Result = ConnectInstrumentParts (SamplerIns, "Output", 0, OutputIns, "Input", 0);
	CHECKRESULT(Result,"ConnectInstrumentParts");
	Result = ConnectInstrumentParts (SamplerIns, "Output", 1, OutputIns, "Input", 1);
	CHECKRESULT(Result,"ConnectInstrumentParts");

/* Build samples via software synthesis. */
	Result = BuildSample( &Samp1, &Cue1, 1 );
	CHECKRESULT(Result,"BuildSample");
	Result = BuildSample( &Samp2, &Cue2, 0 );
	CHECKRESULT(Result,"BuildSample");

/* Attach the samples to the instrument. */
	Att1 = CreateAttachment(SamplerIns, Samp1, NULL);
	CHECKRESULT(Att1,"CreateAttachment");
	Att2 = CreateAttachment(SamplerIns, Samp2, NULL);
	CHECKRESULT(Att2,"CreateAttachment");

/* Get signals from Cues so we can call WaitSignal() */
	Signal1 = GetCueSignal( Cue1 );
	Signal2 = GetCueSignal( Cue2 );

/* Link the Attachments in a circle to simulate double buffering. */
	LinkAttachments( Att1, Att2 );
	LinkAttachments( Att2, Att1 );

/* Request a signal when the sample ends. */
	Result = MonitorAttachment( Att1, Cue1, CUE_AT_END );
	CHECKRESULT(Result,"MonitorAttachment");

	Result = MonitorAttachment( Att2, Cue2, CUE_AT_END );
	CHECKRESULT(Result,"MonitorAttachment");

	Result = StartInstrument( SamplerIns, NULL );
	CHECKRESULT(Result,"StartInstrument");

/* Let it loop several times. */
	for (i=0; i<NUM_LOOPS; i++)
	{
		SignalHit = WaitSignal( Signal1|Signal2 );
		atnum = (SignalHit & Signal1) ? 1 : 2 ;
		PRT(("Received signal %d of %d from attachment %d\n", i+1, NUM_LOOPS, atnum));
	}

/* Unlink so that current one just finishes. */
	LinkAttachments( Att1, 0 );
	LinkAttachments( Att2, 0 );

/* Wait for current one to finish. */
	SignalHit = WaitSignal( Signal1|Signal2 );
	atnum = (SignalHit & Signal1) ? 1 : 2 ;
	PRT(("Received FINAL signal from attachment %d!\n", atnum));

	ReleaseInstrument( SamplerIns, NULL);

cleanup:
	UnloadInstrument( OutputIns );
	UnloadInstrument( SamplerIns );
	DeleteCue ( Cue1 );
	DeleteCue ( Cue2 );
	DeleteAttachment( Att1 );
	DeleteAttachment( Att2 );
	DeleteSample( Samp1 );
	DeleteSample( Samp2 );
	CloseAudioFolio();

cleanup_nofolio:
#ifdef MEMDEBUG
	DumpMemDebugVA (
		DUMPMEMDEBUG_TAG_SUPER, TRUE,
		TAG_END);
	DeleteMemDebug();
#endif

	PRT(("All done.\n"));
	return((int) Result);
}

/************************************************************************/
/* Fill in a sample with sawtooth waves. */
static	int32 BuildSample ( Item *SampPtr, Item *CuePtr, int32 Octave )
{
	int16 *Data;
	Item Samp = 0, Cue = 0;
	int32 Result;

/* Allocate memory for sample. Use MEMTYPE_TRACKSIZE in order to use
** AF_TAG_AUTO_FREE_DATA to create the Sample Item */
	if ((Data = (int16 *)AllocMem (SAMPSIZE, MEMTYPE_NORMAL | MEMTYPE_TRACKSIZE)) == NULL)
	{
		Result = MakeErr (ER_USER, 0, ER_SEVERE, ER_E_USER, ER_C_STND, ER_NoMem);
		goto cleanup;
	}

/* Create a Cue for signalback */
	Cue = CreateCue (NULL);
	CHECKRESULT(Cue, "CreateItem Cue");

/* Fill sample with a sawtooth at a high and low frequency in different channels. */
	{
		int32 i;
		int16 *tdata = Data;

		for (i=0; i<NUM_FRAMES; i++)
		{
#if NUM_CHANNELS == 1
		*tdata++ = (int16) (((i << (8+Octave)) & 0xFFFF));
#else
		*tdata++ = (int16) ((((i << (8+Octave)) & 0xFFFF) * i) / NUM_FRAMES);
		*tdata++ = (int16) ((((i << (6+Octave)) & 0xFFFF) * (NUM_FRAMES - i)) / NUM_FRAMES);  /* Lower frequency. */
#endif
		}
	}

/* Create sample item from sample parameters.
** Create the sample item after the data has been written so that
** the samples will be flushed from the data cache and can be read by
** the audio DMA hardware.
**
** Use AF_TAG_AUTO_FREE_DATA so that sample data will be freed
** automatically when the Sample Item is deleted.
*/
	Samp = CreateSampleVA ( AF_TAG_ADDRESS,        Data,
	/**/                    AF_TAG_WIDTH,          SAMPWIDTH,
	/**/                    AF_TAG_CHANNELS,       NUM_CHANNELS,
	/**/                    AF_TAG_FRAMES,         NUM_FRAMES,
	/**/                    AF_TAG_AUTO_FREE_DATA, TRUE,
	/**/                    TAG_END );
	CHECKRESULT(Samp,"CreateSample");
		/* Since sample data now belongs to Samp, be sure not to free it
		** below (only if something else in here fails). */
	Data = NULL;
	TOUCH(Data);    /* Silence compiler warning when Data is used again. */

/* Return values to caller. */
	*SampPtr = Samp;
	*CuePtr = Cue;
	return 0;

cleanup:
	DeleteCue (Cue);
	DeleteSample (Samp);
	FreeMem (Data, TRACKED_SIZE);
	return Result;
}
