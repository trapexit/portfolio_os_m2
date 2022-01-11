/******************************************************************************
**
**  @(#) playpimap.c 96/08/27 1.8
**
******************************************************************************/

/**
|||	AUTODOC -public -class examples -group Audio -name playpimap
|||	Listen to the instruments assigned in a pimap.
|||
|||	  Format
|||
|||	    playpimap [pimap file]
|||
|||	  Description
|||
|||	    Play each voice in pimap.  This is useful for quickly testing
|||	    a pimap without having to actually play a song.
|||	    Playback can be stopped by hitting the 'X' button.
|||
|||	  Arguments
|||
|||	     [pimap file]
|||	        Name of a PIMAP file to use. Required.
|||
|||	  Associated Files
|||
|||	    playpimap.c, /remote/Samples
|||
|||	  Location
|||
|||	    Examples/Audio/Score
**/

#include <audio/audio.h>
#include <audio/music.h>
#include <kernel/operror.h>
#include <kernel/types.h>
#include <misc/event.h>
#include <stdio.h>

#define  VERSION "V0.4"

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */

/*****************************************************************/

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	DBUG(("try %s\n", name )); \
	if (val < 0) \
	{ \
		Result = val; \
		ERR(("Failure in %s: $%x\n", name, val)); \
		PrintfSysErr(Result); \
		goto cleanup; \
	}

#define kMaxProgramNum   (128)  /* Can be between 1 and 128 */
#define kMaxScoreVoices  (  8)
#define kTicksPerBeat    ( 50)
#define kNumChannels     ( 16)
#define kLoudness        (1.0)

static int32 PlayPIMap( char *mapfile );

/*****************************************************************/
int main (int argc, char *argv[])
{
	char *mapfile;
	int32 Result;

	PRT(("PlayPIMap %s\n", VERSION));
	PRT(("Usage: %s <PIMapFilename>\n", argv[0]));

/* Get files from command line. */
	if (argc > 1 )
	{
		mapfile = argv[1];
	}
	else
	{
		ERR(("Required PIMap filename was not specified!\n"));
		return -1;
	}

/* Initialize audio, return if error. */
	if ((Result = OpenAudioFolio()) < 0)
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(Result);
	}

/* Initialize the EventBroker. */
	Result = InitEventUtility(1, 0, TRUE);
	if (Result < 0)
	{
		PrintError(0,"InitEventUtility",0,Result);
		goto cleanup;
	}


	Result = PlayPIMap(  mapfile );
	CHECKRESULT( Result, "PlayMIDIFile" );

cleanup:
/* Cleanup the EventBroker. */
	Result = KillEventUtility();
	if (Result < 0)
	{
		PrintError(0,"KillEventUtility",0,Result);
	}
	CloseAudioFolio();
	PRT(("%s finished.\n", argv[0]));
	return (int) Result;
}

/*****************************************************************/
static int32 PlayPIMap( char *mapfile )
{
	int32 Result;
	Item MyCue=0;
	int32 i, j, note;
	ScoreContext *scon;
	AudioTime NextTime;
	ControlPadEventData cped;

/* Create a context for interpreting a MIDI score and tracking notes. */
	scon = CreateScoreContext ( kMaxProgramNum );
	if( scon == NULL )
	{
		Result = AF_ERR_NOMEM;
		goto cleanup;
	}

	Result = CreateScoreMixer( scon, MakeMixerSpec(kMaxScoreVoices,2,AF_F_MIXER_WITH_LINE_OUT),
		kMaxScoreVoices, kLoudness);
	CHECKRESULT(Result,"CreateScoreMixer");

/* Load Instrument Templates from disk and fill Program Instrument Map. */
	Result = LoadPIMap ( scon, mapfile );
	CHECKRESULT(Result, "LoadPIMap");

	MyCue = CreateItem ( MKNODEID(AUDIONODE,AUDIO_CUE_NODE), NULL );
	CHECKRESULT(MyCue, "CreateItem Cue");

/* Listen to each channel to see if we like it. */
	NextTime = GetAudioTime();
	for( i=0; i<kNumChannels; i++ )
	{
		PRT(("\nChannel %d\n   Note ", i+1 ));
		for( j=0; j<3; j++ )
		{
			note = 48 + (j*12);
			PRT(("%d  ", note ));
			Result = StartScoreNote( scon, i, note, 64 );
			CHECKRESULT(Result, "StartScoreNote");
			SleepUntilTime( MyCue, NextTime );
			NextTime += kTicksPerBeat/2;

			Result = ReleaseScoreNote( scon, i, note, 64 );
			CHECKRESULT(Result, "StartScoreNote");
			SleepUntilTime( MyCue, NextTime );
			NextTime += kTicksPerBeat/2;
		}

/* Stop if ControlX hit. */
		Result = GetControlPad (1, TRUE, &cped);
		if (Result < 0) {
			PrintError(0,"read control pad in","PlayPIMap",Result);
		}
		if( cped.cped_ButtonBits & ControlX ) break;
	}
	PRT(("\nDone\n"));

cleanup:
	DeleteItem( MyCue );
	UnloadPIMap( scon );
	DeleteScoreMixer( scon );
	DeleteScoreContext( scon );
	return Result;
}
