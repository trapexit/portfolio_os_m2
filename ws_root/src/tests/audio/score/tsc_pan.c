/*
** @(#) tsc_pan.c 96/03/27 1.5
**
** Test Volume and Pan Control
** Demonstrate direct use of ScoreContext
** By:  Phil Burk
*/

/*
** Copyright (C) 1992, 3DO Company.
** All Rights Reserved
** Confidential and Proprietary
*/

#include <audio/audio.h>
#include <audio/music.h>
#include <kernel/operror.h>
#include <kernel/types.h>
#include <misc/event.h>
#include <stdio.h>

#define  VERSION "V0.2"

/* #define USEJOYPAD */
#define DURATION (20)

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */

/*****************************************************************/

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		ERR(("Failure in %s: $%x\n", name, val)); \
		PrintfSysErr(Result); \
		goto cleanup; \
	}

#define MAXPROGRAMNUM (32)  /* Could be as high as 128 */
#define kMaxScoreVoices  (8)

ScoreContext MyScoreCon;

int32 TestPan( char *mapfile );

/*****************************************************************/
int main (int argc, char *argv[])
{
	char *mapfile = "/remote/examples/audio/songs/zap/zap.pimap";
	int32 Result;

	PRT(("Test Score Panning, %s\n", VERSION));
	PRT(("Usage: %s <PIMapFilename>\n", argv[0]));

/* Initialize audio, return if error. */
	if ((Result = OpenAudioFolio() < 0))
	{
		PrintError (NULL, "Open audio folio", NULL, Result);
		TOUCH(Result);
		return(-1);
	}

/* Get files from command line. */
	if (argc > 1 ) mapfile = argv[1];

	Result = TestPan(  mapfile );
	CHECKRESULT( Result, "PlayMIDIFile" );

cleanup:
	CloseAudioFolio();
	PRT(("%s finished.\n", argv[0]));
	return (int) Result;
}

/*****************************************************************/
int32 TestPan( char *mapfile )
{
	int32 Result;
	Item MyCue = 0;
	int32 i, j, Channel;
	ScoreContext *scon;

/* Create a context for interpreting a MIDI score and tracking notes. */
	scon = CreateScoreContext ( MAXPROGRAMNUM );
	if( scon == NULL )
	{
		Result = AF_ERR_NOMEM;
		goto cleanup;
	}

	Result = CreateScoreMixer( scon, MakeMixerSpec(kMaxScoreVoices,2,AF_F_MIXER_WITH_LINE_OUT),
		kMaxScoreVoices, (2.0/kMaxScoreVoices));
	CHECKRESULT(Result,"CreateScoreMixer");

/* Load Instrument Templates from disk and fill Program Instrument Map. */
	Result = LoadPIMap ( scon, mapfile );
	CHECKRESULT(Result, "LoadPIMap");

	MyCue = CreateItem ( MKNODEID(AUDIONODE,AUDIO_CUE_NODE), NULL );
	CHECKRESULT(MyCue, "CreateItem Cue");

/* Test Volume */
	for( i=0; i<16; i++)
	{
		Result = ChangeScoreControl( scon, 0, 7, (i * 127) >> 4 );
		CHECKRESULT(Result, "ChangeScoreControl");

		for( j=0; j<2; j++)
		{
			Result = StartScoreNote( scon, 0, 60, 64 );
			CHECKRESULT(Result, "StartScoreNote");
			SleepUntilTime( MyCue, GetAudioTime() + DURATION );

			Result = ReleaseScoreNote( scon, 0, 60, 64 );
			CHECKRESULT(Result, "ReleaseScoreNote");
			SleepUntilTime( MyCue, GetAudioTime() + DURATION );
		}

	}

/* Test Pan */
	for( Channel=0; Channel<8; Channel++)
	{
		for( i=0; i<8; i++)
		{
			Result = ChangeScoreControl( scon, Channel, 10, (i * 127) >> 3 );
			CHECKRESULT(Result, "ChangeScoreControl");

			for( j=0; j<1; j++)
			{
				Result = StartScoreNote( scon, Channel, 60, 64 );
				CHECKRESULT(Result, "StartScoreNote");
				SleepUntilTime( MyCue, GetAudioTime() + DURATION );

				Result = ReleaseScoreNote( scon, Channel, 60, 64 );
				CHECKRESULT(Result, "ReleaseScoreNote");
				SleepUntilTime( MyCue, GetAudioTime() + DURATION );
			}
		}
	}

cleanup:
	DeleteItem( MyCue );
	UnloadPIMap( scon );
	DeleteScoreMixer( scon );
	DeleteScoreContext( scon );
	return Result;
}
