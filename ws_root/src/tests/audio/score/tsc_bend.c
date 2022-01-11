/***************************************************************
**
** @(#) tsc_bend.c 95/08/30 1.3
**
** Test pitch bend in music library - tsc_bend.c
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
***************************************************************/

/*
** 921116 PLB Modified for explicit mixer connect.
** 921118 PLB Added ChangeDirectory("/remote") for filesystem.
** 921202 PLB Converted to LoadInstrument and GrabKnob.
** 921203 PLB Use AUDIODATADIR instead of /remote.
** 930315 PLB Conforms to new API
*/

#include <kernel/types.h>
#include <file/filefunctions.h>
#include <kernel/debug.h>
#include <kernel/operror.h>
#include <stdio.h>
#include <stdlib.h>

#include <audio/audio.h>
#include <audio/music.h>

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	PRT(x)

#define  VERSION "V0.1.3"

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		ERR(("Failure in %s: $%x\n", name, val)); \
		PrintfSysErr(Result);  \
		goto cleanup; \
	}

#define MAXPROGRAMNUM (32)  /* Could be as high as 128 */
#define kMaxScoreVoices  (8)

ScoreContext MyScoreCon;

#define CENTER_BEND		(0x2000)
#define MAX_BEND		(0x3FFF)
#define NUM_NOTES		(8)
#define DURATION		(60)

/* Define Tags for StartInstrument */
TagArg SamplerTags[] =
	{
		{ AF_TAG_VELOCITY, 0},
		{ AF_TAG_PITCH, 0},
        { TAG_END, 0 }
    };

#define PLAYNOTE1 \
		Result = StartScoreNote( scon, 0, 60, 64 ); \
		CHECKRESULT(Result, "StartScoreNote"); \
		SleepUntilTime( MyCue, GetAudioTime() + DURATION ); \
		Result = ReleaseScoreNote( scon, 0, 60, 64 ); \
		CHECKRESULT(Result, "ReleaseScoreNote"); \
		SleepUntilTime( MyCue, GetAudioTime() + DURATION );


#define PLAYNOTE \
		PLAYNOTE1; \
		FreeChannelInstruments( scon, 0 ); \
		PLAYNOTE1;

/*****************************************************************/
int32 TestScoreBend( char *mapfile )
{
	int32 Result;
	Item MyCue = 0;
	int32 i;
	ScoreContext *scon;
	int32 Bend, BendRange = 7;
	float32 Fraction;

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

/* Set bend range to a fifth. */
	SetScoreBendRange( scon, BendRange );

PRT(("Play notes with no bend.\n"));

	for( i=0; i<2; i++)
	{
		PLAYNOTE;
	}

/* Restore Pitch Bend */
		Result = ChangeScorePitchBend( scon, 0, CENTER_BEND );
		CHECKRESULT(Result, "ChangeScorePitchBend");

PRT(("Play notes with center bend. Should be same pitch.\n"));

	for( i=0; i<2; i++)
	{
		PLAYNOTE;
	}

PRT(("Play pairs of notes with increasing PitchBend.\n"));
PRT(("Notes in pair should be equal.\n"));

	for( i=0; i<NUM_NOTES; i++)
	{
		Bend = (i * (MAX_BEND+1) / NUM_NOTES);
		ConvertPitchBend( Bend, BendRange, &Fraction );
		PRT(("i = %d, Bend = 0x%x, Fraction = %g\n", i, Bend, Fraction ));

		Result = ChangeScorePitchBend( scon, 0, Bend);
		CHECKRESULT(Result, "ChangeScorePitchBend");

		PLAYNOTE;

	}

/* Restore Pitch Bend */
		Result = ChangeScorePitchBend( scon, 0, CENTER_BEND );
		CHECKRESULT(Result, "ChangeScorePitchBend");

PRT(("Play notes with no bend. Should be original pitch.\n"));

	for( i=0; i<2; i++)
	{
		PLAYNOTE;
	}

cleanup:
	DeleteItem( MyCue );
	UnloadPIMap( scon );
	DeleteScoreMixer( scon );
	DeleteScoreContext( scon );
	return Result;
}

/*****************************************************************/
int main (int argc, char *argv[])
{
	char *mapfile = "/remote/examples/audio/songs/zap/zap.pimap";
	int32 Result;

	PRT(("Play MIDI File, %s\n", VERSION));
	PRT(("Usage: %s <PIMapFilename>\n", argv[0]));

/* Initialize audio, return if error. */
	if (OpenAudioFolio())
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(-1);
	}
	InitJuggler();

/* Get files from command line. */
	if (argc > 1 ) mapfile = argv[1];

	Result = TestScoreBend(  mapfile );
	CHECKRESULT( Result, "PlayMIDIFile" );

cleanup:
	TermJuggler();
	CloseAudioFolio();
	PRT(("%s finished.\n", argv[0]));
	return (int) Result;
}
