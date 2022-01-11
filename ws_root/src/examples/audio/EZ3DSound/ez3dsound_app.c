
/******************************************************************************
**
**  @(#) ez3dsound_app.c 96/08/27 1.5
**
******************************************************************************/

/*
   Generate sound effects using EZ3DSound toolbox.
   
   Well, it's a game environment, right?  What's a game environment without
   blowing things up?
   
   Some of these sounds are stationary.  Some are moving, relative to the
   listener.
*/

#include <audio/audio.h>
#include <kernel/nodes.h>       /* MKNODEID() */
#include <kernel/operror.h>
#include <kernel/types.h>
#include <misc/event.h>
#include <stdlib.h>
#include <stdio.h>

#include "ez3dsound.h"

#define  EXAMPLE_VERSION "V0.0"

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */

/* Max # of simultaneous voices */
#define TEZS_MAX_VOICES   (4)
#define TEZS_LOUDNESS     (1.0)
#define TEZS_FX_LOUDNESS  (0.1)

/* You can substitute your own sample names here.  */
#define MOVING_NAME   "bee.patch"
#define STILL_NAME    "yoohoo.aiff"

struct MovingSound
{
	float32	    X;
	float32	    Y;
	float32	    Z;
	Boolean	    moving;
	EZ3DSound*  sound;
};

#define RANDOM_0_1  ((float32) (rand()&0xFFFF) / (float32) 0x10000)
#define SPACE_LIMIT (10)     /* define the space to be +/-10m on a side */
#define RANDOM_SPACE ((RANDOM_0_1 > 0.5) ? (SPACE_LIMIT * RANDOM_0_1) : -(SPACE_LIMIT * RANDOM_0_1))
#define DELTA_MOVE (0.1) /* move 1/10m per 1/50s = 18 k/hr */

#define FULL_3D_PROCESS (EZ3D_DEFAULT_FLAGS)
#define LOW_OVERHEAD_3D_PROCESS (S3D_F_PAN_AMPLITUDE \
	| S3D_F_PAN_FILTER \
	| S3D_F_DISTANCE_AMPLITUDE_SQUARE \
	| S3D_F_OUTPUT_HEADPHONES )

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


int32 TestEZ3DSound( void );

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
	CALL_CHECK( (Result = TestEZ3DSound( )) , "TestEZ3DSound" );

clean:

	KillEventUtility();
	CloseAudioFolio();
	PRT(("%s finished.\n", argv[0]));

	return (int) Result;
}

/*****************************************************************/
int32 TestEZ3DSound( void )
{
	int32        Result;
	static ControlPadEventData cped;
	uint32       Joy=0, OldJoy;
	int32        DoIt = TRUE;
	struct MovingSound moving;
	EZ3DSound   *still, *still2;
	Item         aCue = 0;
	AudioTime    timenow;
	TagArg       Tags[] = { { S3D_TAG_FLAGS }, TAG_END };

/* Create environment for loading and playing sounds. */
	CALL_CHECK( (Result = CreateEZ3DSoundEnvironment( TEZS_MAX_VOICES, TEZS_LOUDNESS,
		TEZS_FX_LOUDNESS, "reverb.patch", NULL )), "CreateEZ3DSoundEnvironment" );

/* Print instructions. */
	PRT(("\n\n"));
	PRT(("Button A = moving bee - hold down while using direction buttons.\n"));
	PRT(("Button B = trigger yoohoo, with low-overhead 3D processing.\n"));
	PRT(("Button C = trigger yoohoo with high-overhead 3D processing.\n"));
	PRT(("X         = Quit\n"));

/* Load all the sounds, using appropriate level of 3D processing. */
	Tags[0].ta_Arg = (TagData) LOW_OVERHEAD_3D_PROCESS;
	CALL_CHECK( (LoadEZ3DSound( &still, STILL_NAME, Tags )),
			"Load still sound" );

	Tags[0].ta_Arg = (TagData) FULL_3D_PROCESS;
	CALL_CHECK( (LoadEZ3DSound( &(moving.sound), MOVING_NAME, Tags )),
			"Load moving sound" );
	CALL_CHECK( (LoadEZ3DSound( &still2, STILL_NAME, Tags )),
			"Load still sound high-overhead" );
			
	moving.moving = FALSE;
	moving.X = 1.0;
	moving.Y = 0.0; /* moving is above ground level */
	moving.Z = 0.0;


/* Create a cue for delaying the loop */
	CALL_CHECK( (aCue = CreateCue( NULL )), "Create a cue" );
	
/* Trigger sounds from control pad */
	while( DoIt )
	{
/* Get user input. */
		CALL_CHECK( (Result = GetControlPad (1, FALSE, &cped)), "GetControlPad");
		OldJoy = Joy;
		Joy = cped.cped_ButtonBits;

/* Read arrow buttons, change position accordingly */
		if (Joy & ControlUp) moving.Z -= DELTA_MOVE;
		if (Joy & ControlDown) moving.Z += DELTA_MOVE;
		if (Joy & ControlRight) moving.X += DELTA_MOVE;
		if (Joy & ControlLeft) moving.X -= DELTA_MOVE;

/* Move the sound */
		if (moving.moving == TRUE)
		{
			CALL_CHECK( (MoveEZ3DSound( moving.sound, moving.X, moving.Y, moving.Z )),
				"MoveEZ3DSound - moving sound" );
		}
		
/* Play moving sound with Button A. */
		if (TestLeadingEdge (Joy,OldJoy,ControlA))
		{			
			moving.moving = TRUE;
			CALL_CHECK( (StartEZ3DSound( moving.sound, moving.X, moving.Y,
				moving.Z, NULL )), "start moving sound." );
		}

/* Stop moving sound when Button A released */
		if (TestTrailingEdge (Joy,OldJoy,ControlA))
		{
			moving.moving = FALSE;
			CALL_CHECK( (StopEZ3DSound( moving.sound, NULL )), "stop moving sound." );
		}

/* Play the low-overhead still sound at a random position */
		if (TestLeadingEdge (Joy,OldJoy,ControlB))
		{
			CALL_CHECK( (StartEZ3DSound( still, RANDOM_SPACE, RANDOM_SPACE,
				RANDOM_SPACE, NULL )), "start low-overhead still sound." );
		}
					
/* Play the high-overhead still sound at a random position */
		if (TestLeadingEdge (Joy,OldJoy,ControlC))
		{
			CALL_CHECK( (StartEZ3DSound( still2, RANDOM_SPACE, RANDOM_SPACE,
				RANDOM_SPACE, NULL )), "start high-overhead still sound." );
		}
					
/* Time to quit? */
		DoIt = !(Joy & ControlX);
		
/* Wait 1/50s */
		CALL_CHECK( (ReadAudioClock( AF_GLOBAL_CLOCK, &timenow )), "read the current time" );
		CALL_CHECK( (SleepUntilAudioTime( AF_GLOBAL_CLOCK, aCue, timenow + 240/50 )),
			"wait for 1/50s" );
	}

clean:
	DeleteEZ3DSoundEnvironment();  /* Clean up everything. */
	DeleteCue( aCue );
	return Result;
}
