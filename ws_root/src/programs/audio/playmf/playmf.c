
/******************************************************************************
**
**  @(#) playmf.c 96/08/28 1.26
**
******************************************************************************/

/* @@@ Note: since this program is primarily placed into System/Programs, it is
       considered a shell program and belongs in the tpg/shell/ documentation
       family, and not examples/. The fact that we also ship the source code in
       examples doesn't affect this (also true of ls).
*/
/**
|||	AUTODOC -class Shell_Commands -group Audio -name playmf
|||	Plays a standard MIDI file.
|||
|||	  Format
|||
|||	    playmf <MIDI file> <PIMap file> [-n<NumReps>] [-a<Amplitude>] [-verbose]
|||
|||	  Description
|||
|||	    Loads a standard MIDI format file, loads instruments and AIFF samples
|||	    described in a PIMap(@) file, and plays the MIDI file the specified number
|||	    of times. Demonstrates use of the Juggler and the score playing routines.
|||
|||	    This program is implemented as both a shell program (in System.m2/Programs)
|||	    and as an example.
|||
|||	  Arguments
|||
|||	    <MIDI file>
|||	        File name of a Format 0 or Format 1 MIDI file.
|||
|||	    <PIMap file>
|||	        File name of a PIMap (Program-Instrument Map) text file. This file is
|||	        parsed at run-time to associate MIDI program numbers with audio folio
|||	        Instrument Templates, Samples, etc.
|||
|||	    -n<NumReps>
|||	        NumReps is number of times to play the MIDI file. Defaults to 1.
|||	        For example:  -n10  for 10 repetitions.
|||
|||	    -a<Amplitude>
|||	        Amplitude per voice that is passed to CreateScoreMixer().
|||	        The value is expressed as a percentage of maximum.
|||	        Defaults to 12. For example:  -a40  for louder music.
|||	        Warning if you set this too high you will cause clipping
|||	        of the audio.
|||
|||	    -verbose
|||	        When specified, causes numerous informational messages to be printed.
|||
|||	  Implementation
|||
|||	    Released as an example in V20.
|||
|||	    Also implemented as a command in V24.
|||
|||	  Location
|||
|||	    System.m2/Programs/playmf, Examples/Audio/Score
|||
|||	  See Also
|||
|||	    PIMap(@), minmax_audio(@)
**/

#include <audio/audio.h>
#include <audio/juggler.h>      /* juggler */
#include <audio/score.h>        /* score player */
#include <ctype.h>              /* toupper() */
#include <kernel/mem.h>         /* memdebug */
#include <kernel/operror.h>
#include <kernel/task.h>
#include <misc/event.h>         /* control pad */
#include <stdio.h>
#include <stdlib.h>             /* atoi() */

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */

/*****************************************************************/

#define PLAYMF_VERSION  PORTFOLIO_OS_VERSION

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		TOUCH(Result); \
		PrintError(0,"\\failure in",name,Result); \
		goto cleanup; \
	}

#define MAXPROGRAMNUM (128)  /* Save memory by lowering this to highest program number. */
#define MAX_SCORE_VOICES  (16)

/*
    Raise this value till the sound clips then drop it till you feel safe.
    (MAXDSPAMPLITUDE/MAX_SCORE_VOICES) is guaranteed safe if all you are
    doing is playing scores
*/
#define MIXER_AMPLITUDE (2.0/MAX_SCORE_VOICES)

/* Prototypes */
ScoreContext *pmfSetupScoreContext( char *mapfile, float32 mixerAmplitude );
Jugglee *pmfLoadScore( ScoreContext *scon, Item scoreClock, char *scorefile );
Err pmfPlayScore( Jugglee *JglPtr, Item scoreClock, uint32 NumReps );
Err pmfCleanupScoreContext( ScoreContext *scon );
Err pmfUnloadScore( ScoreContext *scon, Jugglee *CollectionPtr );
Err pmfPlayScoreMute ( Jugglee *JglPtr, Item scoreClock, uint32 NumReps );
Err PlayMIDIFile( char *scorefile, char *mapfile, int32 NumReps, float32 mixerAmplitude);

/*****************************************************************/
int main (int argc, char *argv[])
{
/*****************************************************************/
	int32 Result;
	char    *midiFileName = NULL, *s;
	char    *pimapFileName = NULL;
	float32  mixerAmplitude = MIXER_AMPLITUDE;
	int32    ifVerbose = 0;
	int32    numReps = 1;
	int32    i;


#ifdef MEMDEBUG
	if (CreateMemDebug ( NULL ) < 0) return 0;

	if (ControlMemDebug ( MEMDEBUGF_ALLOC_PATTERNS |
	/**/                  MEMDEBUGF_FREE_PATTERNS |
	/**/                  MEMDEBUGF_PAD_COOKIES |
	/**/                  MEMDEBUGF_CHECK_ALLOC_FAILURES |
	/**/                  MEMDEBUGF_KEEP_TASK_DATA ) < 0) return 0;
#endif

	PRT(("Play MIDI File V%d\n", PLAYMF_VERSION));
	if (argc < 3) {
		PRT(("Usage: %s <MIDI file> <PIMap file> [-n<NumReps>] [-a<Amplitude>] [-verbose]\n", argv[0]));
		return 0;
	}

/* Initialize the EventBroker. */
	Result = InitEventUtility(1, 0, TRUE);
	if (Result < 0)
	{
		PrintError(0,"init event utility",0,Result);
		goto cleanup;
	}

/* Initialize audio, return if error. */
	if ((Result = OpenAudioFolio()) < 0)
	{
		PrintError(NULL,"open audio folio",NULL,Result);
		return(-1);
	}

/* Required before playing scores. */
	InitJuggler();

/* Parse command line. */
	for( i=1; i<argc; i++ )
	{
		s = argv[i];
		if( s[0] == '-' )
		{
			switch(toupper(s[1]))
			{
				case 'N':
					numReps = atoi(&s[2]);
					break;
				case 'V':
					ifVerbose = TRUE;
					break;
				case 'A':
					mixerAmplitude = (float32)atoi(&s[2]) / 100.0;
					break;
				default:
					ERR(("%s - unsupported option = %s\n", argv[0], s));
					goto cleanup;
					break;

			}
		}
		else
		{
			if( midiFileName == NULL )
			{
				midiFileName = s;
			}
			else if ( pimapFileName == NULL )
			{
				pimapFileName = s;
			}
			else
			{
				ERR(("%s - extra parameter = %s\n", argv[0], s));
				goto cleanup;
			}
		}
	}

	DBUG(("Play %s using pimap %s, amplitude = %g\n", midiFileName, pimapFileName, mixerAmplitude ));

/* Don't disable printing messages when verbose arg is specified */
	DisableScoreMessages (!ifVerbose);

/* Play MIDI file */
	Result = PlayMIDIFile ( midiFileName, pimapFileName, numReps, mixerAmplitude );
	CHECKRESULT( Result, "PlayMIDIFile" );

cleanup:
	TermJuggler();
	CloseAudioFolio();
	KillEventUtility();
	PRT(("%s finished.\n", argv[0]));
#ifdef MEMDEBUG
	DumpMemDebug(NULL);
	DeleteMemDebug();
#endif
	return (int) Result;
}

/******************************************************************
** Create a ScoreContext, load a MIDIFile and play it.
******************************************************************/
Err PlayMIDIFile( char *scorefile, char *mapfile, int32 NumReps, float32 mixerAmplitude )
{
	Jugglee *CollectionPtr = NULL;
	int32    Result = -1;
	ScoreContext *scon = NULL;
	Item     scoreClock;

/* Create a custom clock that we can vary the rate of without messing up other tasks. */
	scoreClock = CreateAudioClock(NULL);
	CHECKRESULT( scoreClock, "CreateAudioClock" );

	scon = pmfSetupScoreContext( mapfile, mixerAmplitude );
	if( scon == NULL ) goto cleanup;

	CollectionPtr = pmfLoadScore( scon, scoreClock, scorefile );
	if( CollectionPtr == NULL) goto cleanup;

/*
** Play the score collection.  Alternatively, you could use the
** pmfPlayScoreMute function instead to mute channels.
*/
	Result = pmfPlayScore( CollectionPtr, scoreClock, NumReps );

cleanup:
	DeleteAudioClock( scoreClock );
	pmfUnloadScore( scon, CollectionPtr );
	pmfCleanupScoreContext( scon );
	return Result;
}

/******************************************************************
** Create a ScoreContext, and load a PIMap from a text file.
******************************************************************/
ScoreContext *pmfSetupScoreContext( char *mapfile, float32 mixerAmplitude )
{
	ScoreContext *scon;
	int32 Result;

/* Create a context for interpreting a MIDI score and tracking notes. */
	scon = CreateScoreContext( MAXPROGRAMNUM );
	if( scon == NULL )
	{
		return NULL;
	}

/* Specify a mixer to use for the score voices. */
	Result = CreateScoreMixer( scon, MakeMixerSpec(MAX_SCORE_VOICES,2,AF_F_MIXER_WITH_LINE_OUT),
		MAX_SCORE_VOICES, mixerAmplitude);
	CHECKRESULT(Result,"CreateScoreMixer");

/* Load Instrument Templates from disk and fill Program Instrument Map. */
/* As an alternative, you could use SetPIMapEntry() */
	Result = LoadPIMap ( scon, mapfile );
	CHECKRESULT(Result, "LoadPIMap");

	return scon;

cleanup:
	pmfCleanupScoreContext( scon );
	return NULL;
}

/******************************************************************
** Create a collection and load a score into it from a MIDI File.
******************************************************************/
Jugglee *pmfLoadScore( ScoreContext *scon, Item scoreClock, char *scorefile )
{
	MIDIFileParser MFParser;
	Jugglee *CollectionPtr;
	int32 Result;

/* Create a collection and load MIDI File into it. */
	CollectionPtr = (Jugglee *) CreateObject( &CollectionClass );
	if (CollectionPtr == NULL)
	{
		ERR(("pmfLoadScore: Failure to create Collection\n"));
		goto cleanup;
	}

/* Set context used to play collection. */
    {
        TagArg Tags[2];

        Tags[0].ta_Tag = JGLR_TAG_CONTEXT;
        Tags[0].ta_Arg = (TagData) scon;
        Tags[1].ta_Tag = TAG_END;

        SetObjectInfo(CollectionPtr, Tags);
    }

/* Load it from a MIDIFile */
	Result = MFLoadCollection ( &MFParser, scorefile , (Collection *) CollectionPtr);
	if (Result)
	{
		ERR(("Error loading MIDI File = $%x\n", Result));
		goto cleanup;
	}


/* Change the rate of the custom clock to match the desired tempo. */
	Result = SetAudioClockRate( scoreClock, ConvertF16_FP(MFParser.mfp_Rate) );
	CHECKRESULT(Result, "SetAudioClockRate");
	PRT(("MIDI File Clock Rate = %g\n", ConvertF16_FP(MFParser.mfp_Rate)));

	return CollectionPtr;

cleanup:
	pmfUnloadScore( scon, CollectionPtr );
	return NULL;
}

/******************************************************************
** Unload the collection which frees the sequences, then
** destroy collection.
******************************************************************/
Err pmfUnloadScore( ScoreContext *scon, Jugglee *CollectionPtr )
{
    TOUCH(scon);

	if (CollectionPtr != NULL)
	{
		MFUnloadCollection( (Collection *) CollectionPtr );
		DestroyObject( (COBObject *) CollectionPtr );
	}

	return 0;
}

/******************************************************************
** Unload the PIMap which frees the instruments and samples, then
** delete the ScoreContext.
******************************************************************/
Err pmfCleanupScoreContext( ScoreContext *scon )
{
	if( scon != NULL )
	{
		UnloadPIMap( scon );
		DeleteScoreMixer( scon );
		DeleteScoreContext( scon );
	}

	return 0;
}

/*****************************************************************
** Play a Collection, a Sequence or any other Juggler object.
** This routine is likely to be carved up for use in your application.
*****************************************************************/
Err pmfPlayScore( Jugglee *JglPtr, Item scoreClock, uint32 NumReps )
{
	AudioTime CurTime, NextTime;
	int32 Result;
	int32 NextSignals, CueSignal, SignalsGot;
	Item MyCue;
	uint32 Joy;
	int32 QuitNow;
	int32 IfTimerPending;
	ControlPadEventData cped;

	QuitNow = 0;
	NextSignals=0;
	IfTimerPending = FALSE;

/* Create a Cue for timing the score. */
	MyCue = CreateCue( NULL );
	CHECKRESULT(MyCue, "CreateCue");
	CueSignal = GetCueSignal ( MyCue );

/* Drive Juggler using Audio timer. */
	Result = ReadAudioClock( scoreClock, &NextTime );
	CHECKRESULT(Result, "ReadAudioClock");
/* Delay start by adding ticks to avoid stutter on startup. Optional. */
	NextTime += 40;
	CHECKRESULT(Result, "ReadAudioClock");
	CurTime = NextTime;

DBUG(("pmfPlayScore: Start at time %d\n", NextTime ));
/* This tells the Juggler to process this object when BumpJuggler() is
** later called.  Multiple objects could be started and will be
** juggled by Juggler.
*/
	StartObject ( JglPtr , NextTime, NumReps, NULL );

	do
	{
/* Read current state of Control Pad. */
		Result = GetControlPad (1, FALSE, &cped);
		if (Result < 0) {
			PrintError(0,"read control pad in","pmfPlayScore",Result);
		}
		Joy = cped.cped_ButtonBits;

/* Forced Quit by hitting ControlX */
		if(Joy & ControlX)
		{
			StopObject( JglPtr, CurTime );
			QuitNow = TRUE;
		}
		else
		{
/* Request a timer wake up at the next event time. */
			if( !IfTimerPending )
			{
				Result = SignalAtAudioTime( scoreClock, MyCue, NextTime);
				if (Result < 0) return Result;
				IfTimerPending = TRUE;
			}

/* Wait for timer signal or signal(s) from BumpJuggler */
			SignalsGot = WaitSignal( CueSignal | NextSignals );

/* Did we wake up because of a timer signal? */
			if( SignalsGot & CueSignal )
			{
				IfTimerPending = FALSE;
				CurTime = NextTime;
			}
			else
			{
/* Get current time to inform Juggler. */
				ReadAudioClock( scoreClock, &CurTime );
			}

/* Tell Juggler to process any pending events, eg. Notes. Result > 0 if done. */
			Result = BumpJuggler( CurTime, &NextTime, SignalsGot, &NextSignals );
		}

	} while ( (Result == 0) && (QuitNow == 0));

	DeleteCue( MyCue );

cleanup:
	return Result;
}

/************** YOU MAY IGNORE THE FOLLOWING CODE ****************
** It was written to illustrate muting and pause functions and is
** not currently called from the above code.
*****************************************************************/

/*****************************************************************
** Set the Mute flag for the Nth child of a collection.
*****************************************************************/
int32 MuteNthChild(  Jugglee *JglPtr, int32 SeqNum, int32 IfMute )
{
	Jugglee *Child;
	int32 Result;

DBUG(("Set mute of #%d to %d\n", SeqNum, IfMute));

	Result = GetNthFromObject( JglPtr, SeqNum, &Child);
	if(Result < 0)
	{
		ERR(("Jugglee has no %dth child.\n", SeqNum));
		return 0;
	}

    {
        TagArg Tags[2];

        Tags[0].ta_Tag = JGLR_TAG_MUTE;
        Tags[0].ta_Arg = (TagData)IfMute;
        Tags[1].ta_Tag = TAG_END;

        Result = SetObjectInfo(Child, Tags);
    }

	return Result;
}

/*****************************************************************
** Play a Collection, a Sequence or any other Juggler object.
** Fancy version incorporates Muting and Pause tests.
*****************************************************************/
Err pmfPlayScoreMute ( Jugglee *JglPtr, Item scoreClock, uint32 NumReps )
{
	AudioTime CurTime, NextTime, PauseTime, ResumeTime, DeltaTime;
	int32 Result;
	int32 NextSignals, CueSignal, SignalsGot;
	Item MyCue;
	uint32 Joy;
	int32 QuitNow, i;
	int32 Muted[8];  /* Keep track of which ones are muted. */
	ControlPadEventData cped;

	for( i=0; i<8; i++) Muted[i] = 0;
	QuitNow = 0;
	NextSignals=0;

	MyCue = CreateItem ( MKNODEID(AUDIONODE,AUDIO_CUE_NODE), NULL );
	CHECKRESULT(MyCue, "CreateItem Cue");
	CueSignal = GetCueSignal ( MyCue );

/* Drive Juggler using Audio timer. */
	Result = ReadAudioClock( scoreClock, &NextTime );
	CHECKRESULT(Result, "ReadAudioClock");
/* Delay start by adding ticks to avoid stutter on startup. Optional. */
	NextTime += 40;
	CurTime = NextTime;
	DeltaTime = 0;

DBUG(("pmfPlayScoreMute: Start at time %d\n", NextTime ));
	StartObject ( JglPtr , NextTime, NumReps, NULL );

	do
	{
/* Use Control Pad to experiment with Muting (optional). */
/* Read current state of Control Pad. */
		Result = GetControlPad (1, FALSE, &cped);
		if (Result < 0) {
			ERR(("Error in GetControlPad\n"));
			PrintError(0,"read control pad in","pmfPlayScoreMute",Result);
		}
		Joy = cped.cped_ButtonBits;

/* Define a macro to simplify code. */
#define TestMute(Mask,SeqNum) \
	if((Joy & Mask) && !Muted[SeqNum]) \
	{ \
		MuteNthChild( JglPtr, SeqNum, TRUE); \
		Muted[SeqNum] = TRUE; \
	} \
	else if( !(Joy & Mask) && Muted[SeqNum] ) \
	{ \
		MuteNthChild( JglPtr, SeqNum, FALSE); \
		Muted[SeqNum] = FALSE; \
	}
		TestMute(ControlA,     0);
		TestMute(ControlB,     1);
		TestMute(ControlC,     2);
		TestMute(ControlUp,    3);
		TestMute(ControlRight, 4);
		TestMute(ControlDown,  5);
		TestMute(ControlLeft,  6);

/* Pause by slipping Juggler time against clock time. */
		if(Joy & ControlRightShift)
		{
			ReadAudioClock( scoreClock, &PauseTime );
			PRT(("Pause at %d\n", PauseTime ));
			do
			{
				Result = GetControlPad (1, TRUE, &cped);
				if (Result < 0) {
					PrintError(0,"read control pad in","pmfPlayScoreMute",Result);
				}
			} while( (cped.cped_ButtonBits & ControlRightShift) == 0);
			ReadAudioClock( scoreClock, &ResumeTime );
			PRT(("Resume at %d\n", ResumeTime ));
			DeltaTime += (ResumeTime - PauseTime);
			GetControlPad (1, TRUE, &cped);
		}

		if(Joy & ControlX)      /* Forced Quit */
		{
			StopObject( JglPtr, CurTime );
			QuitNow = TRUE;
		}
		else
		{
/* Request a timer wake up at the next event time. */
			Result = SignalAtAudioTime( scoreClock, MyCue, NextTime + DeltaTime);
			if (Result < 0) return Result;

/* Wait for timer signal or signal(s) from BumpJuggler */
			SignalsGot = WaitSignal( CueSignal | NextSignals );
/* Sleeping now until we get signalled. */
			if (SignalsGot & CueSignal)
			{
				CurTime = NextTime;
			}
			else
			{
				ReadAudioClock( scoreClock, &CurTime );
				CurTime -= DeltaTime;
			}

/* Tell Juggler to do its thing. Result > 0 if done. */
			Result = BumpJuggler( CurTime, &NextTime, SignalsGot, &NextSignals );
		}

	} while ( (Result == 0) && (QuitNow == 0));

	DeleteItem( MyCue );

cleanup:
	return Result;
}
