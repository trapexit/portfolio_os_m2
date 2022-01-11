
/******************************************************************************
**
**  @(#) tsp_spoolsoundfile.c 96/05/14 1.20
**
******************************************************************************/

/**
|||	AUTODOC -public -class examples -group Audio -name tsp_spoolsoundfile
|||	Plays an AIFF sound file from a thread using the Sound Player.
|||
|||	  Format
|||
|||	    tsp_spoolsoundfile <sound file> [num repeats]
|||
|||	  Description
|||
|||	    Plays an AIFF sound file using a thread to manage playback. The file is
|||	    played as a continuous loop.
|||
|||	  Arguments
|||
|||	    <sound file>
|||	        Name of an AIFF file to play.
|||
|||	    [num repeats]
|||	        Optional number of times to repeat the sound file. Defaults to 1.
|||
|||	  Associated Files
|||
|||	    tsp_spoolsoundfile.c
|||
|||	  Location
|||
|||	    Examples/Audio/Spooling
|||
|||	  See Also
|||
|||	    spCreatePlayer(), tsp_algorithmic(@), tsp_rooms(@)
**/

#include <audio/audio.h>
#include <audio/musicerror.h>
#include <audio/soundplayer.h>
#include <audio/parse_aiff.h>
#include <kernel/task.h>
#include <kernel/mem.h>
#include <stdio.h>
#include <stdlib.h>


/* -------------------- Parameters */

#define NUMBLOCKS (32)
#define BLOCKSIZE (2048)
#define BUFSIZE (NUMBLOCKS*BLOCKSIZE)
#define NUMBUFFS  (4)
#define MAXAMPLITUDE (1.0)

/*
** Allocate enough space so that you don't get stack overflows.
** An overflow will be characterized by seemingly random crashes
** that defy all attempts at logical analysis.  You might want to
** start big then reduce the size till you crash, then double it.
*/
#define STACKSIZE (10000)
#define PRIORITY  (180)


/* -------------------- Macros */

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		TOUCH(Result); \
		PrintError(0,"\\failure in",name,Result); \
		goto error; \
	}

#define CHECKSIGNAL(val,name) \
	if (val <= 0) \
	{ \
		Result = val ? val : AF_ERR_NOSIGNAL; \
		TOUCH(Result); \
		PrintError (NULL,name,NULL,Result); \
		goto error; \
	}

#define CHECKPTR(val,name) \
	if (val == 0) \
	{ \
		Result = -1; \
		ERR(("Failure in %s\n", name)); \
		goto error; \
	}


/* -------------------- Globals for SpoolSoundFileThread */

char *gFileName;
int32 gSignal1;
Item gMainTaskItem;
int32 gNumReps;


/* -------------------- Functions */

void SpoolSoundFile (char *fileName, int32 numRepeats);
void SpoolSoundFileThread (void);


/* -------------------- main() */

int main(int argc, char *argv[])
{
	int32 Result;

  #ifdef MEMDEBUG
	Result = CreateMemDebug (NULL);
	CHECKRESULT (Result, "CreateMemDebug");

	Result = ControlMemDebug (MEMDEBUGF_ALLOC_PATTERNS |
	/**/                      MEMDEBUGF_FREE_PATTERNS |
	/**/                      MEMDEBUGF_PAD_COOKIES |
	/**/                      MEMDEBUGF_CHECK_ALLOC_FAILURES |
	/**/                      MEMDEBUGF_KEEP_TASK_DATA);
	CHECKRESULT (Result, "ControlMemDebug");
  #endif

		/* Initialize audio, return if error. */
	Result = OpenAudioFolio();
	CHECKRESULT (Result, "open audio folio");

		/* Get sample name from command line. */
	if (argc < 2) {
		PRT(("Usage: %s <samplefile> [numreps]\n", argv[0]));
		goto error;
	}

		/* do the demo */
	SpoolSoundFile (argv[1], argc > 2 ? atoi (argv[2]) : 1);

error:
	CloseAudioFolio();

  #ifdef MEMDEBUG
	DumpMemDebug(NULL);
	DeleteMemDebug();
  #endif

	return 0;
}

/*
	Sets up globals for background thread, starts it,
	then waits for its completion. Instead of just waiting
	for it to complete, this function could do other stuff
	after starting the thread.
*/
void SpoolSoundFile (char *fileName, int32 numRepeats)
{
	Item SpoolerThread=0;
	int32 Result;

		/* put parameters into globals for thread */
	gFileName = fileName;
	gNumReps = numRepeats;

	PRT(("tsp_spoolsoundfile: Play file %s %d times.\n", gFileName, gNumReps));

		/* Get parent task Item so that thread can signal back. */
	gMainTaskItem = CURRENTTASKITEM;

		/* Allocate a signal for each thread to notify parent task. */
	gSignal1 = AllocSignal( 0 );
	CHECKSIGNAL(gSignal1,"AllocSignal");

		/* create the thread. execution can begin immediately */
	SpoolerThread = CreateThread(SpoolSoundFileThread, "SpoolSoundFileThread",
								 PRIORITY, STACKSIZE, NULL);
	CHECKRESULT(SpoolerThread,"CreateThread");

	/*
		Do nothing for now but we could easily go off and do other stuff here!.
		OR together signals from other sources for a multi event top level.
		In this example, we just wait for the thread to finish.
	*/

	PRT(("tsp_spoolsoundfile: Foreground waiting for signal from background spooler.\n"));
	WaitSignal( gSignal1 );
	PRT(("tsp_spoolsoundfile: Background spooler finished.\n"));

error:
	DeleteThread( SpoolerThread );
	PRT(("tsp_spoolsoundfile: Playback complete.\n"));
}


/* -------------------- SpoolSoundFileThread() */

int32 PlaySoundFile (const char *FileName, int32 BufSize, int32 NumReps);
Err LoopDecisionFunction (SPAction *resultAction, int32 *remainingCountP, SPSound *sound, const char *markerName);
static Err SelectSamplerForFile ( const SampleInfo *smpi, char *resultSamplerName, int32 nameSize );

void SpoolSoundFileThread( void )
{
	int32 Result;

		/* open the audio folio for this this thread. */
	Result = OpenAudioFolio();
	CHECKRESULT (Result, "open audio folio");

		/* play the sound file passed thru the globals */
	PlaySoundFile (gFileName, BUFSIZE, gNumReps);

error:
	SendSignal( gMainTaskItem, gSignal1 );
	CloseAudioFolio();
	WaitSignal(0);
	/* Waits forever. Don't return! Thread gets deleted by parent. */
}

int32 PlaySoundFile (const char *FileName, int32 BufSize, int32 NumReps)
{
	int32 Result;
	SampleInfo *smpi = NULL;
	char samplername[AF_MAX_NAME_SIZE];
	Item samplerins=0;
	Item outputins=0;
	SPPlayer *player=NULL;
	SPSound *sound=NULL;
	int32 RemainingCount = NumReps;

	PRT(("tsp_spoolsoundfile thread: NumReps=%ld\n", NumReps));

		/* Parse file just to get sample format. */
	Result = GetAIFFSampleInfo( &smpi, FileName, ML_GETSAMPLEINFO_F_SKIP_DATA );
	CHECKRESULT (Result, "get AIFF info");

		/* select sample player instrument to use */
	Result = SelectSamplerForFile (smpi, samplername, sizeof samplername);
	CHECKRESULT (Result, "select sample player");

	PRT(("tsp_spoolsoundfile thread: play file using '%s' at sample rate of %g Hz\n", samplername, smpi->smpi_SampleRate));

		/* load and connect instruments */
	samplerins = LoadInstrument (samplername, 0, 100);
	CHECKRESULT (samplerins, "load sampler instrument");

	outputins = LoadInstrument ("line_out.dsp", 0, 100);
	CHECKRESULT (outputins, "load output instrument");

	if (smpi->smpi_Channels == 1) {
		Result = ConnectInstrumentParts (samplerins, "Output", 0, outputins, "Input", AF_PART_LEFT);
		CHECKRESULT (Result, "connect left");
		Result = ConnectInstrumentParts (samplerins, "Output", 0, outputins, "Input", AF_PART_RIGHT);
		CHECKRESULT (Result, "connect right");
	}
	else {
		Result = ConnectInstrumentParts (samplerins, "Output", AF_PART_LEFT,  outputins, "Input", AF_PART_LEFT);
		CHECKRESULT (Result, "connect left");
		Result = ConnectInstrumentParts (samplerins, "Output", AF_PART_RIGHT, outputins, "Input", AF_PART_RIGHT);
		CHECKRESULT (Result, "connect right");
	}

		/* start output instrument */
	Result = StartInstrument (outputins, NULL);
	CHECKRESULT (Result, "start output");

		/* create player */
	Result = spCreatePlayer (&player, samplerins, NUMBUFFS, BufSize, NULL);
	CHECKRESULT (Result, "create player");

	Result = spAddSoundFile (&sound, player, FileName);
	if (Result < 0) {
		PrintError (NULL, "add sound", FileName, Result);
		goto error;
	}

		/* set up default action to loop sound */
	Result = spLoopSound (sound);
	CHECKRESULT (Result, "loop sound");

		/* Install decision function. */
	Result = spSetMarkerDecisionFunction (sound, SP_MARKER_NAME_END,
										  (SPDecisionFunction)LoopDecisionFunction,
										  &RemainingCount);
	CHECKRESULT (Result, "set marker decision function");

		/* start playing */
	Result = spStartReading (sound, SP_MARKER_NAME_BEGIN);
	CHECKRESULT (Result, "start reading");

	Result = spStartPlayingVA (player,
		AF_TAG_SAMPLE_RATE_FP, ConvertFP_TagData(smpi->smpi_SampleRate),
		AF_TAG_AMPLITUDE_FP,   ConvertFP_TagData(MAXAMPLITUDE),
		TAG_END);
	CHECKRESULT (Result, "start playing");

	{
		const int32 playersigs = spGetPlayerSignalMask (player);

		while (spGetPlayerStatus(player) & SP_STATUS_F_BUFFER_ACTIVE) {
			const int32 sigs = WaitSignal (playersigs);

			Result = spService (player, sigs);
			CHECKRESULT (Result, "Service");
		}
	}

	spStop (player);
	PRT(("tsp_spoolsoundfile thread: done.\n"));

error:
	spDeletePlayer (player);
	UnloadInstrument (samplerins);
	UnloadInstrument (outputins);
	DeleteSampleInfo (smpi);
	return Result;
}


/*
	This decision function decrements the RemainingCount variable.
	Normally it returns default, but when the count is exhausted it
	returns stop.
*/
Err LoopDecisionFunction (SPAction *resultAction, int32 *remainingCountP, SPSound *sound, const char *markerName)
{
	TOUCH(sound);
	TOUCH(markerName);

		/* decrement remaining repeat count */
	(*remainingCountP)--;

	PRT (("tsp_spoolsoundfile thread: %d more times to go\n", *remainingCountP));

		/* loop back to beginning (default action) or stop if no more repeats */
	if (*remainingCountP <= 0)
		return spSetStopAction (resultAction);
	else
		return 0;
}


static Err SelectSamplerForFile ( const SampleInfo *smpi, char *resultSamplerName, int32 nameSize )
{
	uint32 iSampleRate;
	bool   ifVariable;
	uint32 compressionType;

/* Set ifVariable depending on sample rate and parameter. */
	iSampleRate = smpi->smpi_SampleRate + 0.5; /* Round to nearest integer. */
	ifVariable = (iSampleRate != 44100);

/* If no compression, pass bits. */
	compressionType = smpi->smpi_CompressionType
		? smpi->smpi_CompressionType
		: smpi->smpi_Bits;

/* Build name. */
	return SampleFormatToInsName(
		compressionType,
		ifVariable,
		smpi->smpi_Channels,
		resultSamplerName,
		nameSize);
}
