/******************************************************************************
**
**  @(#) markov_music.c 96/08/27 1.6
**
**	Markov Music
**	An example of using Markov chains with the Advanced Sound Player to
**	make an interactive, constantly varying soundtrack.
**
**	Author: rnm
**
******************************************************************************/

/**
|||	AUTODOC -public -class examples -group Audio -name MarkovMusic
|||	Constantly-varying environmentally-aware soundtrack.
|||
|||	  Format
|||
|||	    MarkovMusic
|||
|||	  Description
|||
|||	    This program generates a soundtrack from a set of four AIFF data files.
|||	    The data files can be characterized by their "tension": one is fairly
|||	    static, the next moves along fairly briskly, the third is more driving
|||	    still, and the fourth is a transition segment.
|||
|||	    The program switches between segments based on a global tension variable
|||	    controlled by the joypad.  The "Up" button increases the tension, the
|||	    "Down" button decreases it.  The "Stop" button terminates the program.
|||
|||	    The switching takes place on musical bar boundaries, and the program
|||	    attempts to switch to a corresponding bar in a four-bar group in the
|||	    target segment.  When tension is decreased, the program transitions
|||	    through the transition segment.
|||
|||	    In addition to switching based on tension, the program branches around
|||	    within a segment, making branching decisions based on Markov transition
|||	    tables.  This can provide variation for a given tension level.
|||
|||	    By following marker naming conventions other AIFF files may be used.
|||	    Markers are divided into two kinds: major and minor.  Major markers
|||	    (those indicating divisions between types of musical material within
|||	    a file; for example, theme, variationA, rhythm only) are named
|||
|||	      branchx
|||
|||	    where x is a number from 1 to the number of major markers.  They serve
|||	    to signal a decision to change tensions, if necessary, and to signal
|||	    a decision to (possibly) introduce a variation.  Each major marker
|||	    corresponds to a row in the Markov transition tables for the sound.
|||
|||	    Minor markers are established wherever a decision to change tensions needs
|||	    to be made: in the example, this is done on every bar so that the
|||	    latency between changing tensions with the joypad and hearing the
|||	    change effected is fairly small.  Minor markers are named by appending
|||	    a counter to the corresponding major marker; for example,
|||
|||	      branch2:13
|||
|||	    indicates marker 13 in major section 2.
|||
|||	    After placing markers, you need to change the constant array
|||	    SoundNumBlocks to reflect how many major markers you've set in each
|||	    soundfile.
|||
|||	    In addition to placing markers in your soundfiles, you need to alter
|||	    the transition tables in the associated file markov_tables.c.  There is
|||	    one table for each potential transition, with names that reflect the
|||	    soundfile to transition from and the soundfile to transition to.  For
|||	    example, the table "mt13" holds probabilities for moving from a marker
|||	    in soundfile 1 (spA.cbd2) to soundfile 3 (spC.cbd2).  For each major
|||	    marker in the "from" soundfile, there's a corresponding row in the table.
|||	    For each major marker in the "to" soundfile, there's a column. The
|||	    numbers in each column of that row represent the probability that
|||	    the program will branch from that row's marker to that column's marker.
|||	    The probabilities for a given row sum to 1.0.  For example, if there
|||	    are four columns in a given table, and for a given row each contains the
|||	    number 0.25, then there is a 1 in 4 chance that the marker for that row
|||	    will branch to any of "to" soundfile's markers, effectively a completely
|||	    random choice.  If one column contains the number 1.0, and the others
|||	    0.0, then the program will always branch from that row's marker to that
|||	    column's marker, a completely determined choice.  You can alter the
|||	    numbers in the tables at any time, so that, for example, every time the
|||	    program selects a branch, the corresponding probability is decreased
|||	    and the other columns increased to lessen the likelihood that that
|||	    branch will happen again.
|||
|||	  Associated Files
|||
|||	    spA.cbd2, spB.cbd2, spC.cbd2, spD.cbd2, markov_music.c, markov_music.h,
|||	    markov_tables.c, markov_tables.h
|||
|||	  Location
|||
|||	    Examples/Audio/MarkovMusic
|||
**/

#include <audio/audio.h>
#include <audio/soundplayer.h>
#include <audio/parse_aiff.h>
#include <kernel/task.h>
#include <kernel/mem.h>
#include <kernel/msgport.h>
#include <misc/event.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "markov_music.h"
#include "markov_tables.h"

	/* Soundtrack thead */
#define MARKOV_PRIORITY 150
#define MARKOV_STACK    4096

    /* Soundtrack SPPlayer audio buffers */
#define BUF_COUNT   4
#define BUF_SIZE    22000

#define TestLeadingEdge(newset,oldset,mask)  ( (newset) & ~(oldset) & (mask) )
#define MakeUErr(svr,class,err) MakeErr(ER_USER,0,svr,ER_E_USER,class,err)

/*
	Macro to simplify error checking.
	expression is only evaluated once and should return an int32
	(or something that can be cast to it sensibly)
*/
#define TRAP_ERROR(_exp,_desc) \
	do \
	{ \
		errcode = (_exp); \
		if (errcode < 0) \
		{ \
			PrintError (NULL, _desc, NULL, errcode); \
			goto clean; \
		} \
	} while (0)
	
/* ---- global variables */

volatile int32 glTension=0;
volatile int32 oldTension=0;

const char * const marmSoundFilenames[NUMBER_OF_SOUNDS] = {
	"spB.cbd2",	/* stopped sound */
	"spA.cbd2",	/* moving sound */
	"spD.cbd2", /* fast sound */
	"spC.cbd2", /* transition sound */
};

SPSound* marmSounds[NUMBER_OF_SOUNDS] = 
{
	NULL, NULL, NULL, NULL
};

/* ---- functions */

Err HandleInput (void);

    /* thread */
int32 marmMain (void);
/******************************************************************************/
int main (int argc, char *argv[])
{
	Item replyport = 0;
	Item soundtrackthread = 0;
	Err errcode;

	TOUCH(argc);

	printf ("%s: start.\n", argv[0]);

	if ((errcode = InitEventUtility (1, 0, TRUE)) < 0) goto clean;

        /* create soundtrack thread's completion reply port */
	if ((errcode = replyport = CreateMsgPort ("MarkovReply", 0, 0)) < 0)
	goto clean;

        /* start soundtrack thread, instruct thread to send message on completion. */
	if ((errcode = soundtrackthread = CreateThreadVA (
	  (void (*)())marmMain, "Markov",
 	  MARKOV_PRIORITY, MARKOV_STACK,
	  CREATETASK_TAG_MSGFROMCHILD, replyport,
	  TAG_END)) < 0) goto clean;

	/* process control pad events. returns when user presses X button */
	if ((errcode = HandleInput()) < 0) goto clean;

	/* wait for completion message from soundtrack thread to arrive */
	WaitPort (replyport, 0);

DBUG(("Thread finished, terminating main()\n"));

clean:
	if (errcode < 0) PrintError (NULL, NULL, NULL, errcode);

	/* forcibly kill soundtrack thread in case of failure from above */DeleteThread (soundtrackthread);
	DeleteMsgPort (replyport);
	KillEventUtility();

	printf ("%s: done.\n", argv[0]);

	return 0;
}

/******************************************************************************/
/*
  This function processes control pad events.

  Up, Down - change the tension
  X       - exits gracefully
  Shift-X - terminates forcefully
*/
Err HandleInput (void)
{
	ControlPadEventData cped;
	int32 joy, oldjoy = 0;
	Err errcode;

	for (;;)
	{
		/* wait for a control pad event */
		if ((errcode = GetControlPad (1, TRUE, &cped)) < 0) goto clean;
		joy = cped.cped_ButtonBits;

		/* X (Stop) button: exit */
		if (TestLeadingEdge (joy, oldjoy, ControlX))
		{
			glTension = -1;  /* signal a nice exit */

			/* If either shift button is down, return an error code
			   to cause main() to exit without waiting for thread
			   to shut down. */

			errcode = (joy & (ControlLeftShift | ControlRightShift))
			  ? MakeUErr (ER_INFO, ER_C_STND, ER_Aborted) : 0;

			goto clean;
		}

		/* scale tension up or down */
		if (TestLeadingEdge (joy, oldjoy, ControlUp))
		{
			glTension = MIN(glTension+1, 2);
			PRT(("Tension to %i\n", glTension));
		}
		
		if (TestLeadingEdge (joy, oldjoy, ControlDown))
		{
			glTension = MAX(glTension-1, 0);
			PRT(("Tension to %i\n", glTension));
		}

		oldjoy = joy;
	}

clean:
	return errcode;
}

/* ---- thread functions */

Err marmInit( Item* samplerins, Item* outputins, Item* sample, SPPlayer** player );
void marmTerm( Item samplerins, Item outputins, Item sample, SPPlayer* player );
Err marmRun( SPPlayer* player );
Err variationDecisionFunction (SPAction *action,
  void* decisiondata,
  SPSound *sound, const char *markername);
Err tensionDecisionFunction (SPAction *action,
  void* decisiondata,
  SPSound *sound, const char *markername);
Err marmSetTensionMarkers ( void );
Err marmSetVariationMarkers ( void );
Err markerNumbertoName( int32 block, int32 subblock, char* name );
Err markerNametoNumber( char* name, int32* block, int32* subblock );

/******************************************************************************/
int32 marmMain ( void )
{
	Err errcode;
	Item samplerins = 0;
	Item outputins = 0;
	Item transitionSample = 0;
	SPPlayer* player = NULL;

DBUG(("Thread says: I'm alive!!\n"));

	TRAP_ERROR( OpenAudioFolio(), "open audio folio" );

	TRAP_ERROR( marmInit( &samplerins, &outputins, &transitionSample,
	  &player ), "initialize Markov music" );

	TRAP_ERROR( marmRun( player ), "run Markov music" );

clean:
	marmTerm( samplerins, outputins, transitionSample, player );
	CloseAudioFolio();

	return errcode;
}

/******************************************************************************/
Err marmInit( Item* samplerins, Item* outputins, Item* sample, SPPlayer** player )
{
	#define SAMPLER_NAME_LENGTH 40

	Err errcode;
	SampleInfo* aSampleInfo = NULL;
	char samplerName[SAMPLER_NAME_LENGTH];
	
	/* look at sample to discover what type of playback instrument is required */
	TRAP_ERROR ( GetAIFFSampleInfo (&aSampleInfo, marmSoundFilenames[STOPPED_SOUND],
	  ML_GETSAMPLEINFO_F_SKIP_DATA), "get sample information" );
	TRAP_ERROR ( SampleFormatToInsName (aSampleInfo->smpi_CompressionType,
	  FALSE, aSampleInfo->smpi_Channels, samplerName, SAMPLER_NAME_LENGTH),
	  "derive sampler filename" );

	/* make output instruments */
	TRAP_ERROR ( *samplerins = LoadInstrument (samplerName, 0, 100),
	  "Load sample player instrument" );
	TRAP_ERROR ( *outputins = LoadInstrument ("line_out.dsp", 0, 100),
	  "Load output instrument" );

	/* connect instruments */
	TRAP_ERROR ( ConnectInstrumentParts (*samplerins, "Output", 0, *outputins,
	  "Input", 0), "Connect sampler to output" );
	TRAP_ERROR ( ConnectInstrumentParts (*samplerins, "Output", 1, *outputins,
	  "Input", 1), "Connect sampler to output" );
	TRAP_ERROR ( StartInstrument (*outputins, NULL), "Start output" );

	/* create player */
	TRAP_ERROR ( spCreatePlayer (player, *samplerins, BUF_COUNT, BUF_SIZE, NULL),
	  "create SPPlayer" );

	/* add all the disc-based sound files */
	TRAP_ERROR ( spAddSoundFile (&marmSounds[STOPPED_SOUND], *player,
	  marmSoundFilenames[STOPPED_SOUND]), "add sound STOPPED" );
	TRAP_ERROR ( spAddSoundFile (&marmSounds[MOVING_SOUND], *player,
	  marmSoundFilenames[MOVING_SOUND]), "add sound MOVING" );
	TRAP_ERROR ( spAddSoundFile (&marmSounds[FAST_SOUND], *player,
	  marmSoundFilenames[FAST_SOUND]), "add sound FAST" );

	/* preload transition and add it to player as in-memory sample */
	TRAP_ERROR ( *sample = LoadSample (marmSoundFilenames[TRANSITION_SOUND]),
	  "load TRANSITION" );
	TRAP_ERROR ( spAddSample (&marmSounds[TRANSITION_SOUND], *player, *sample),
	  "add sound TRANSITION" );

	/* spDumpPlayer( *player ); */
	
	/* set up all the markers */
	TRAP_ERROR ( marmSetTensionMarkers(), "Set tension markers" );
	TRAP_ERROR ( marmSetVariationMarkers(), "Set variation markers" );

	/* set tension to 0 */
	glTension = 0;

	return (0);

clean:
	DeleteSampleInfo ( aSampleInfo );
	return errcode;
}

/******************************************************************************/
void marmTerm( Item samplerins, Item outputins, Item sample, SPPlayer* player )
{
DBUG(("Unloading sample\n"));
	UnloadSample( sample );
DBUG(("Unloading player\n"));
	spDeletePlayer (player);
DBUG(("Unloading outputIns\n"));
    UnloadInstrument (outputins);
DBUG(("Unloading samplerIns\n"));
	UnloadInstrument (samplerins);
	
	return;
}

/******************************************************************************/
Err marmRun( SPPlayer* player )
{
	Err errcode;
	
	/* start playing */
	TRAP_ERROR ( spStartReading (marmSounds[glTension], SP_MARKER_NAME_BEGIN),
	  "Start reading" );
        
DBUG(("Reading ..."));

	TRAP_ERROR ( spStartPlayingVA (player, AF_TAG_AMPLITUDE_FP,
	  ConvertFP_TagData(1.0), TAG_END), "Start playing" );

DBUG(("Playing\n"));

	/* service player until it's done */
	{
		const int32 playersigs = spGetPlayerSignalMask (player);

		while (spGetPlayerStatus (player) & SP_STATUS_F_BUFFER_ACTIVE)
		{
			const int32 sigs = WaitSignal (playersigs);
			if ((errcode = spService (player, sigs)) < 0) goto clean;
		}
	}

	/* all done, stop player and clean up */
	TRAP_ERROR ( spStop (player), "Stop" );

DBUG(("Stopping\n"));

	return 0;
	
clean:
	return errcode;
}

/******************************************************************************/
Err variationDecisionFunction (SPAction *action, void* decisiondata,
  SPSound *sound, const char *markername)
{
	Err errcode;
	char toMarker[80];

	if (glTension != oldTension)
	{
		errcode = tensionDecisionFunction( action, decisiondata, sound, markername );
	}
	else
	{

DBUG(("Variation: picking markovian destination for sound %i, marker %s\n",
  glTension, markername));
  
		mtChoose( glTension, glTension, markername, toMarker );
		errcode = spSetBranchAction (action, sound, toMarker );

PRT(("Variation: branching from sound %i, marker %s to marker %s\n",
  glTension, markername, toMarker));

	}
	
	return errcode;
}

/******************************************************************************/
Err tensionDecisionFunction (SPAction *action, void* decisiondata,
  SPSound *sound, const char *markername)
{
	Err errcode;
	char toMarker[80];
	SPSound* toSound;
	
	TOUCH(decisiondata);
	TOUCH(sound);
	
	if (glTension != oldTension)
	{
		if (glTension < oldTension)    /* we're slowing down, go through transition */
		{

DBUG(("tension: going down from %i to %i\n", oldTension, glTension));

			strcpy( toMarker, SP_MARKER_NAME_BEGIN );
			toSound = marmSounds[TRANSITION_SOUND];
		}
		else
		{

DBUG(("tension: going up from %i to %i\n", oldTension, glTension));

			mtChoose( oldTension, glTension, markername, toMarker );		  
			if ((glTension != FAST_SOUND) || (oldTension != MOVING_SOUND))
			{
				/* jump to opening bar, not next bar */
				int32 block, bar;
				
				markerNametoNumber( toMarker, &block, &bar );
				markerNumbertoName( block, 0, toMarker );
			}
			
			toSound = marmSounds[glTension];

PRT(("tension: picked %s\n", toMarker));

			oldTension = glTension;
		}
					
		errcode = spSetBranchAction( action, toSound, toMarker );
	}
	else errcode = 0;
		
	return errcode;
}

/******************************************************************************/
Err transitionDecisionFunction (SPAction *action, void* decisiondata,
  SPSound *sound, const char *markername)
{
	Err errcode;
	char toMarker[80];
	const char* fromMarker = "block2";
	/* !!! kludge - spAddSound doesn't see these markers !!! */
	
DBUG(("transition: tension is %i\n", glTension));

	TOUCH(decisiondata);
	TOUCH(sound);
	TOUCH(markername);
	
	if (glTension < 0) errcode = spSetStopAction( action );
	   /* stop playing the soundtrack */	
	else
	{
		mtChoose( TRANSITION_SOUND, glTension, fromMarker, toMarker );	
		errcode = spSetBranchAction( action, marmSounds[glTension],
		  toMarker );

PRT(("transition: going to %i marker %s\n", glTension, toMarker));

		oldTension = glTension;
	}
	
	return errcode;
}

/******************************************************************************/
void marmUseMarkov ( SPSound* asound, SPDecisionFunction decisionFunction, int32
  includeSubs )
{
	char markerName[80];
	int32 i;

	i = 1;
	markerNumbertoName( i, 0, markerName );

	while ( spFindMarkerName( asound, markerName ) )
	{

DBUG(("Setting markov function for marker %s\n", markerName));

		if (includeSubs)
		{
			int32 j;
			
			j = 1;
			markerNumbertoName( i, j, markerName );
			
			while (spFindMarkerName( asound, markerName ))
			{
				spSetMarkerDecisionFunction( asound, markerName, decisionFunction, NULL );
				j += 1;
				markerNumbertoName( i, j, markerName );
			}
		}
		else spSetMarkerDecisionFunction( asound, markerName, decisionFunction, NULL );

		i += 1;
		markerNumbertoName( i, 0, markerName );
	}

	return;
}

/******************************************************************************/
Err marmSetTensionMarkers ( void )
{
	int32 i;
	Err errcode;
	
	for (i=0;i<NUMBER_OF_SOUNDS-1;i++)
	{
		marmUseMarkov( marmSounds[i], (SPDecisionFunction)tensionDecisionFunction,
		  TRUE );
	}
	
	/* set special transition function */
	TRAP_ERROR( spSetMarkerDecisionFunction( marmSounds[TRANSITION_SOUND], SP_MARKER_NAME_END,
	  (SPDecisionFunction) transitionDecisionFunction, NULL ),
	  "set transition decision function" );

clean:
	return errcode;
}

/******************************************************************************/
Err marmSetVariationMarkers ( void )
{	
	/* STOPPED sound: use a Markov chain */
	marmUseMarkov( marmSounds[STOPPED_SOUND], (SPDecisionFunction)
	  variationDecisionFunction, FALSE );
	
	/* MOVING sound: use a Markov chain */
	marmUseMarkov( marmSounds[MOVING_SOUND], (SPDecisionFunction)
	  variationDecisionFunction, FALSE );
		
	/* FAST sound: use a Markov chain */
	marmUseMarkov( marmSounds[FAST_SOUND], (SPDecisionFunction)
	  variationDecisionFunction, FALSE );
	
	return 0;
}

/******************************************************************************/
Err markerNametoNumber ( char* name, int32* block, int32* sub )
{
	char* subpos;
	char buf[6]='\0', delim[2]='\0';

	*block = atoi( name+5 );
	
	*sub = 0;
	if (subpos = strchr( name, ':')) *sub = atoi( subpos+1 );

	if (*block == 0) return(-1);
	else return(0);
}

/******************************************************************************/
Err markerNumbertoName ( int32 block, int32 sub, char* name )
{
	if (sub == 0) sprintf( name, "block%i", block );
	else sprintf( name, "block%i:%i", block, sub );

	return 0;
}


