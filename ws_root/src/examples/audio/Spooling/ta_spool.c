
/******************************************************************************
**
**  @(#) ta_spool.c 96/08/27 1.18
**  $Id: ta_spool.c,v 1.16 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC -public -class examples -group Audio -name ta_spool
|||	Demonstrates the libmusic.a sound spooler.
|||
|||	  Format
|||
|||	    ta_spool
|||
|||	  Description
|||
|||	    Uses the libmusic.a sound spooler to fill buffers, parse information in the
|||	    buffers, signal our task when a buffer has been exhausted, and refill the
|||	    buffers. Use this sample code as a basis for developing your own routines
|||	    for playing large sampled files, or handling other kinds of buffered data.
|||
|||	  Controls
|||
|||	    A
|||	        Simulates a hold on data delivery. The application continues to consume
|||	        information in its buffers.
|||
|||	    B
|||	        Stops the playback function. Releasing it restarts playback. Holding
|||	        down either shift button causes the spooler to be aborted instead of
|||	        merely stopped.
|||
|||	    C
|||	        Enters one-shot mode. Each C button press starts a ssplPlayData()
|||	        sequence. Press the play button returns to normal mode.
|||
|||	    Start
|||	        Pauses the playback function. Releasing it lets the application
|||	        continue.
|||
|||	    Stop (X)
|||	        Quit.
|||
|||	  Associated Files
|||
|||	    ta_spool.c
|||
|||	  Location
|||
|||	    Examples/Audio/Spooling
|||
|||	  See Also
|||
|||	    ssplCreateSoundSpooler(), tsp_spoolsoundfile(@)
**/

#include <audio/audio.h>
#include <audio/soundspooler.h>
#include <kernel/mem.h>
#include <kernel/operror.h>
#include <kernel/task.h>
#include <kernel/types.h>
#include <misc/event.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define DEBUG_Status    0           /* turns on SoundSpooler status display for every cycle thru event loop */
#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
/*	PRT(("Ran %s, got 0x%x\n", name, val)); */ \
	if (val < 0) \
	{ \
		Result = val; \
		PrintError(0,name,NULL,Result); \
		goto cleanup; \
	}

#define NUM_BUFFERS   (4)
#define NUMCHANNELS (2)   /* Stereo */
#define NUMFRAMES (8*1024)
#define SAMPSIZE (sizeof(int16)*NUMFRAMES*NUMCHANNELS)

/************************************************************************
** Fill in a sample with sine waves with a sinewave in different octaves.
************************************************************************/
void FillBufferWithSine (int16 *Data, int32 NumFrames, int32 NumChannels, int32 Octave)
{
	int32 i, j;

	for (i=0; i<NumFrames; i++) {
		const uint16 phase = (i << (8+Octave)) & 0xFFFF;
		const int16 sample = (int16)(sinf (2*M_PI * (float32)phase/65536.0) * 32767);

		for (j=0; j<NumChannels; j++) {
			*Data++ = sample;
		}
	}
}

/************************************************************************
** Callback function for buffer completion processor.
************************************************************************/
int32 MySoundBufferFunc( SoundSpooler *sspl, SoundBufferNode *sbn, int32 msg )
{
    char *msgdesc;

	switch (msg) {
        case SSPL_SBMSG_INITIAL_START:      msgdesc = "Initial Start";      break;
        case SSPL_SBMSG_LINK_START:         msgdesc = "Link Start";         break;
        case SSPL_SBMSG_STARVATION_START:   msgdesc = "Starvation Start";   break;
	    case SSPL_SBMSG_COMPLETE:           msgdesc = "Complete";           break;
	    case SSPL_SBMSG_ABORT:              msgdesc = "Abort";              break;
	    default:                            msgdesc = "<unknown>";          break;
	}

	PRT(("spool: %s #%d\n", msgdesc, ssplGetSequenceNum(sspl,sbn)));

	return 0;
}

/************************************************************************
** Main test.
************************************************************************/
int main(int argc, char *argv[])
{
/* Declare local variables */
	Item OutputIns = -1;
	Item SamplerIns = -1;
	int32 Result;
	int32 SignalMask;
	SoundSpooler *sspl = NULL;
	char *Data[NUM_BUFFERS];
	int32 MySignal[NUM_BUFFERS], CurSignals;
	int32 i;
	int32 BufIndex, Joy;
	ControlPadEventData cped;

	TOUCH(argc);        /* not used */

	PRT(("%s started.\n", argv[0]));

/* Initalize local variables */
	memset (Data, 0, sizeof Data);

  #ifdef MEMDEBUG
	if ((Result = CreateMemDebug ( NULL )) < 0) goto cleanup;

	if ((Result = ControlMemDebug ( MEMDEBUGF_ALLOC_PATTERNS |
	/**/                            MEMDEBUGF_FREE_PATTERNS |
	/**/                            MEMDEBUGF_PAD_COOKIES |
	/**/                            MEMDEBUGF_CHECK_ALLOC_FAILURES |
	/**/                            MEMDEBUGF_KEEP_TASK_DATA)) < 0) goto cleanup;
  #endif

/* Initialize the EventBroker. */
	Result = InitEventUtility(1, 0, TRUE);
	CHECKRESULT (Result, "init EventUtility");

/* Initialize audio, return if error. */
	Result = OpenAudioFolio();
	CHECKRESULT (Result, "open audio folio");

/* Use directout instead of mixer. */
	OutputIns = LoadInstrument("line_out.dsp",  0, 100);
	CHECKRESULT(OutputIns,"LoadInstrument");
	Result = StartInstrument( OutputIns, NULL );
	CHECKRESULT(Result,"StartInstrument: OutputIns");

/* Load fixed rate stereo sample player instrument */
	SamplerIns = LoadInstrument("sampler_16_f2.dsp",  0, 100);
	CHECKRESULT(SamplerIns,"LoadInstrument");

/* Connect Sampler Instrument to DirectOut */
	Result = ConnectInstrumentParts (SamplerIns, "Output", 0, OutputIns, "Input", 0);
	CHECKRESULT(Result,"ConnectInstruments");
	Result = ConnectInstrumentParts (SamplerIns, "Output", 1, OutputIns, "Input", 1);
	CHECKRESULT(Result,"ConnectInstruments");

/* Allocate and fill buffers with arbitrary test data. */
	for( i=0; i<NUM_BUFFERS; i++ )
	{
		Data[i] = (char *) AllocMem( SAMPSIZE, MEMTYPE_ANY );
		if( Data[i] == NULL )
		{
			ERR(("Not enough memory for sample .\n"));
			goto cleanup;
		}
		FillBufferWithSine( (int16 *) Data[i], NUMFRAMES, NUMCHANNELS, i );
	}

/* Create SoundSpooler data structure. */
	sspl = ssplCreateSoundSpooler( NUM_BUFFERS, SamplerIns );
	if( sspl == NULL )
	{
		ERR(("ssplCreateSoundSpooler failed!\n"));
		goto cleanup;
	}
	ssplSetSoundBufferFunc (sspl, MySoundBufferFunc);

/* Fill the sound queue by queuing up all the buffers. "Preroll" */
	BufIndex = 0;
	SignalMask = 0;
	for( i=0; i<NUM_BUFFERS; i++)
	{
/* Dispatch buffers full of sound to spooler. Set User Data to BufIndex.
** ssplSpoolData returns a signal which can be checked to see when the data
** has completed it playback. If it returns 0, there were no buffers available.
*/
        Result = ssplSpoolData( sspl, Data[BufIndex], SAMPSIZE, NULL );
        CHECKRESULT (Result, "spool data");
		if (!Result)
		{
		    ERR(("Out of buffers\n"));
			goto cleanup;
		}
        MySignal[BufIndex] = Result;
		SignalMask |= MySignal[BufIndex];
		BufIndex++;
		if(BufIndex >= NUM_BUFFERS) BufIndex = 0;
	}

	ssplDumpSoundSpooler (sspl);

/* Start Spooler instrument. Will begin playing any queued buffers. */
	Result = ssplStartSpoolerTagsVA (sspl,
		AF_TAG_AMPLITUDE_FP, ConvertFP_TagData(1.0),
		TAG_END);
	CHECKRESULT(Result,"ssplStartSpoolerTags");

/* Play buffers loop. */
	do
	{
/* Wait for some buffer(s) to complete. */
DBUG(("WaitSignal(0x%x)\n", SignalMask ));
		CurSignals = WaitSignal( SignalMask );
DBUG(("WaitSignal() got 0x%x\n", CurSignals ));

/* Tell sound spooler that the buffer(s) have completed. */
		Result = ssplProcessSignals( sspl, CurSignals, NULL );
		CHECKRESULT(Result,"ssplProcessSignals");

/* Read current state of Control Pad. */
		Result = GetControlPad (1, FALSE, &cped);
		CHECKRESULT (Result, "GetControlPad()");
		Joy = cped.cped_ButtonBits;

/* Simulate data starvation. Hang if we hit A button. */
		if(Joy & ControlA)
		{
			PRT(("Hang spooler.\n"));
			do
			{
				GetControlPad (1, TRUE, &cped);
				Joy = cped.cped_ButtonBits;
			} while(Joy & ControlA);
			PRT(("Unhang spooler.\n"));
		}

/* Stop on B button (abort on Shift+B), restart on release */
		if(Joy & ControlB)
		{
			if (Joy & (ControlLeftShift | ControlRightShift)) {
				PRT(("Abort.\n"));
				Result = ssplAbort (sspl, NULL);
				CHECKRESULT(Result,"ssplAbort");
			}
			else {
				PRT(("Stop.\n"));
				Result = ssplStopSpooler (sspl);
				CHECKRESULT(Result,"ssplStopSpooler");
			}
			do
			{
				GetControlPad (1, TRUE, &cped);
				Joy = cped.cped_ButtonBits;
			} while(Joy & ControlB);
			Result = ssplStartSpoolerTags (sspl, NULL);
			CHECKRESULT(Result,"ssplStartSpoolerTags");
			PRT(("Restart.\n"));
		}

/* Single shot mode. Send single buffers by hiting C, until Start hit. */
		if(Joy & ControlC)
		{
			PRT(("Single shot mode.\n"));
			do
			{
				GetControlPad (1, TRUE, &cped);
				Joy = cped.cped_ButtonBits;
				if(Joy & ControlC)
				{
				    Result = ssplPlayData (sspl, Data[BufIndex], SAMPSIZE);
				    CHECKRESULT (Result, "play data");
                    BufIndex++;
                    if(BufIndex >= NUM_BUFFERS) BufIndex = 0;
				}
			} while((Joy & ControlStart) == 0);
		}

/* Pause while holding play/pause button. */
		if(Joy & ControlStart)
		{
			PRT(("Pause.\n"));
			ssplPause( sspl );
			do
			{
				GetControlPad (1, TRUE, &cped);
				Joy = cped.cped_ButtonBits;
			} while(Joy & ControlStart);
			ssplResume( sspl );
			PRT(("Resume.\n"));
		}

      #if DEBUG_Status
		ssplDumpSoundSpooler (sspl);
      #endif

/*
** Spool as many buffers as are available.
** ssplSpoolData will return positive signals as long as it accepted the data.
*/
		while ((Result = ssplSpoolData (sspl, Data[BufIndex], SAMPSIZE, NULL)) > 0)
		{
/* INSERT YOUR CODE HERE TO FILL UP THE NEXT BUFFER */
			BufIndex++;
			if(BufIndex >= NUM_BUFFERS) BufIndex = 0;
		}
		CHECKRESULT (Result, "spool data");

	} while( (Joy & ControlX) == 0 );

/* Stop Spooler. */
	Result = ssplAbort( sspl, NULL );
	CHECKRESULT(Result,"StopSoundSpooler");

cleanup:
	ssplDeleteSoundSpooler( sspl );
	for (i=0; i<NUM_BUFFERS; i++) {
		if (Data[i]) FreeMem (Data[i], SAMPSIZE);
	}
	UnloadInstrument( SamplerIns );
	UnloadInstrument( OutputIns );
	CloseAudioFolio();
	KillEventUtility();
	PRT(("All done.\n"));

  #ifdef MEMDEBUG
	DumpMemDebugVA (
		DUMPMEMDEBUG_TAG_SUPER, TRUE,
		TAG_END);
	DeleteMemDebug();
  #endif

	return((int) Result);
}
