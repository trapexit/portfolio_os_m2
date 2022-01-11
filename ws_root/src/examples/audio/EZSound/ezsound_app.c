
/******************************************************************************
**
**  @(#) ezsound_app.c 96/09/10 1.9
**
******************************************************************************/

/*
** Generate sound effects using EZSound toolbox.
** Summary of steps:
**    Initialize toolbox using CreateEZSoundEnvironment()
**    Load sound effects using LoadEZSound()
**    Play sounds effects using StartEZSound() and StopEZSound()
**    Unload sound effects using UnloadEZSound() -- optional
**    Cleanup using DeleteEZSoundEnvironment()
*/

#include <audio/audio.h>
#include <kernel/nodes.h>       /* MKNODEID() */
#include <kernel/operror.h>
#include <kernel/types.h>
#include <misc/event.h>
#include <stdlib.h>
#include <stdio.h>

#include "ezsound.h"

#define  EXAMPLE_VERSION "V0.5"

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */

/* Max # of simultaneous voices */
#define TEZS_MAX_VOICES   (10)
#define TEZS_LOUDNESS     (0.7)
#define TEZS_NUM_SHOTS    (6)

/* You can substitute your own sample names here.  */
#define SNARE_NAME      "/remote/Samples/GMPercussion44K/Snare.M44k.aiff"
#define CYMBAL_NAME     "/remote/Samples/Unpitched/CymbalCrash/CrashCymbal1.M22k.aiff"
#define SHOT_NAME       CYMBAL_NAME
#define ENGINE_NAME     "idling_engine.patch"
#define SQUIGGLE_NAME   "ez_squiggle.patch"

/*****************************************************************/

/* Macro to simplify error checking. */
#define CALL_CHECK(_exp,msg) \
	DBUG(("%s\n",msg)); \
	do \
	{ \
		if ((Result = (Err) (_exp)) < 0) \
		{ \
			PrintError(0,"\\failure in",msg,Result); \
			goto clean; \
		} \
	} while(0)

/* -------------------- Bit twiddling macros */

#define TestLeadingEdge(newset,oldset,mask)  ( (newset) & ~(oldset) & (mask) )
#define TestTrailingEdge(newset,oldset,mask) TestLeadingEdge ((oldset),(newset),(mask))


int32 TestEZSound( void );

/*****************************************************************/
int main (int argc, char *argv[])
{
	int32 Result;

/* Prevent compiler warning. */
	TOUCH(argc);

	PRT(("%s, %s\n", argv[0], EXAMPLE_VERSION));

/* Initialize audio, return if error. */
	if ((Result = OpenAudioFolio()) < 0)
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(Result);
	}

/* Initialise control pad. */
	CALL_CHECK( (Result = InitEventUtility(1, 0, TRUE)) ,"InitEventUtility");

/* Run test. */
	CALL_CHECK( (Result = TestEZSound( )) , "TestEZSound" );

clean:

	KillEventUtility();
	CloseAudioFolio();
	PRT(("%s finished.\n", argv[0]));

	return (int) Result;
}

/*****************************************************************/
int32 TestEZSound( void )
{
	int32 Result;
	static ControlPadEventData cped;
	uint32 Joy=0, OldJoy;
	int32     i, DoIt = TRUE;
	EZSound   *engineSound;
	EZSound   *squiggleSound;
	EZSound   *bangSound;
	int32     curShot = 0;
	EZSound   *shotSound[TEZS_NUM_SHOTS];
	float32    fxSend;

/* Create environment for loading and playing sounds. */
/* Creates a mixer and optional effects patch. */
	CALL_CHECK( (Result = CreateEZSoundEnvironment( TEZS_MAX_VOICES, TEZS_LOUDNESS,
		"reverb.patch", NULL )), "CreateEZSoundEnvironment" );

/* Print instructions. */
	PRT(("Buttons A,B,C,D = trigger samples.\n"));
	PRT(("LeftShift = turn off echo effect.\n"));
	PRT(("X         = Quit\n"));

/* Load single sound for sustained sound like an engine. */
	CALL_CHECK( (LoadEZSound( &engineSound, ENGINE_NAME, NULL )), "Load engine sound." );

/* Load single sound for sustained sound like an engine. */
	CALL_CHECK( (LoadEZSound( &squiggleSound, SQUIGGLE_NAME, NULL )), "Load squiggle sound." );

/* Load single bang sound that will cut off previous when retriggered. */
	CALL_CHECK( (LoadEZSound( &bangSound, SNARE_NAME, NULL )), "Load bang sound." );

/* Load multiple shot sounds to allow overlapping sound. */
/* First load will read from disk. Subsequent loads will read from memory. */
	for( i=0; i<TEZS_NUM_SHOTS; i++ )
	{
		CALL_CHECK( (LoadEZSound( &shotSound[i], SHOT_NAME, NULL )), "Load shot sound." );
	}

/* Trigger sounds from control pad. */
	while( DoIt )
	{
/* Get user input. */
		CALL_CHECK( (Result = GetControlPad (1, TRUE, &cped)), "GetControlPad");
		OldJoy = Joy;
		Joy = cped.cped_ButtonBits;

/* Control dry or wet mix with LeftShift */
		fxSend = (Joy & ControlLeftShift ) ? 0.0 : 0.9;

/* Play overlapping shots at random amplitude and pan between 0.0 and 1.0. */
/* Use "round robin" voice allocation scheme. */
		if (TestLeadingEdge (Joy,OldJoy,ControlA))
		{
			float32  amplitude;
			float32  pan;
#define RANDOM_0_1  ((float32) (rand()&0xFFFF) / (float32) 0x10000)
			amplitude = RANDOM_0_1;
			pan = RANDOM_0_1;
			CALL_CHECK( (SetEZSoundMix( shotSound[curShot], amplitude, pan, fxSend )), "Mix shot sound." );
			CALL_CHECK( (StartEZSound( shotSound[curShot], NULL )), "Fire shot sound." );
			curShot = (curShot < (TEZS_NUM_SHOTS-1)) ? curShot++ : 0;
		}

/* Play bang at random amplitude and pan between 0.0 and 1.0. */
		if (TestLeadingEdge (Joy,OldJoy,ControlB))
		{
			float32  amplitude;
			float32  pan;
			amplitude = RANDOM_0_1;
			pan = RANDOM_0_1;
			CALL_CHECK( (SetEZSoundMix( bangSound, amplitude, pan, fxSend )), "Mix bang sound." );
			CALL_CHECK( (StartEZSound( bangSound, NULL )), "Fire bang sound." );
		}

/* Turn engine on and off with Button C. */
		if (TestLeadingEdge (Joy,OldJoy,ControlC))
		{
			CALL_CHECK( (SetEZSoundMix( engineSound, 1.0, 0.2, fxSend )), "mix engine sound." );
			CALL_CHECK( (StartEZSound( engineSound, NULL )), "start engine sound." );
		}
		else if (TestTrailingEdge (Joy,OldJoy,ControlC))
		{
/* Stop sound immediately by using Stop instead of Release. */
			CALL_CHECK( (StopEZSound( engineSound, NULL )), "stop engine sound." );
		}

/* Turn squiggle on and off with Button D. */
		if (TestLeadingEdge (Joy,OldJoy,ControlD))
		{
			CALL_CHECK( (SetEZSoundMix( squiggleSound, 0.2, 0.8, fxSend )), "mix squiggle sound." );
			CALL_CHECK( (StartEZSound( squiggleSound, NULL )), "start squiggle sound." );
		}
		else if (TestTrailingEdge (Joy,OldJoy,ControlD))
		{
/* Allow sound to die out gradually by using Release instead of Stop. */
			CALL_CHECK( (ReleaseEZSound( squiggleSound, NULL )), " release squiggle sound." );
		}

/* Time to quit? */
		DoIt = !(Joy & ControlX);
	}

clean:
	DeleteEZSoundEnvironment();  /* Clean up everything. */
	return Result;
}
