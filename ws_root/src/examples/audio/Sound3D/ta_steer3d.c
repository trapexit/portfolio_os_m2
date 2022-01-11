/***************************************************************
**
** @(#) ta_steer3d.c 96/08/27 1.14
**
** Test sound3d library:
** Animate a walking sound.  When the sound stops, play a sound
** at the stopped position; select the sound based on the distance.
**
** - use new API
**
** By:  Robert Marsanyi
**
** Copyright (c) 1996, 3DO Company.
** This program is proprietary and confidential.
**
***************************************************************/

/**
|||	AUTODOC -public -class examples -group Audio -name ta_steer3d
|||	Control a walking man in three-space using the 3DSound API.
|||
|||	  Format
|||
|||	    ta_steer3d [reverb [cuelist]]
|||
|||	  Description
|||
|||	    With this program, you steer the sound of a walking man around
|||	    a plane using the control pad.  By varying the command line
|||	    arguments you can hear the effect of various cues on the
|||	    three-dimensional illusion.
|||
|||	    The "man" starts moving at a velocity of about 1 m/s from where
|||	    you're standing straight forward.  Each press on the arrow keys
|||	    changes his velocity.  The control pad buttons work as follows:
|||
|||	    Up
|||	        Add 1 m/s in the forward direction
|||	    Down
|||	        Add 1 m/s in the backward direction
|||	    Left
|||	        Add 1 m/s in the leftward direction
|||	    Right
|||	        Add 1 m/s in the rightward direction
|||	    Stop
|||	        Stop walking, play a sound at the current position
|||	    Play
|||	        Start walking again, with a 1 m/s forward velocity
|||	    A
|||	        End the program
|||
|||	    With a little adept button pressing, you can make the man walk
|||	    by your shoulder, or around you in a circle.
|||
|||	    Note: to use this example, you first need to install the General MIDI
|||	    Sample Library in /remote/Samples, and then run the shell script
|||	    "makepatch.script" in the directory Examples/Audio/Sound3D to build the
|||	    set of patches loaded by the program.
|||
|||	  Arguments
|||
|||	    reverb
|||	        A floating point number from 0.0 to 1.0, determining the global
|||	        reverberation amount in the final mix.  For example, a value of
|||	        0.25 means that 0.25 of the output signal for a given sound
|||	        comes from the reverb and 0.75 from the dry signal.  The overall
|||	        amplitude is controlled by the distance of the sound from the
|||	        listener.
|||
|||	    cuelist
|||	        a string of digits selected from the list below.  Including
|||	        a digit in the string enables the corresponding cue.  The default
|||	        cuelist is equivalent to the string 147.
|||
|||	    1
|||	        Interaural Time Delay
|||	    2
|||	        Interaural Amplitude Difference
|||	    3
|||	        Pseudo-HRTF Filter
|||	    4
|||	        Distance-squared rule for amplitude attenuation
|||	    5
|||	        Distance-cubed rule for amplitude attenuation
|||	    7
|||	        Doppler shift
|||
|||	  Associated Files
|||
|||	    patches/sound3dspace_nice_stereo.mp, patches/walking.mp,
|||	    makepatch.script
|||
|||	  Location
|||
|||	    Examples/Audio/Sound3D
|||
**/

#include <kernel/types.h>
#include <kernel/debug.h>
#include <kernel/operror.h>
#include <kernel/mem.h>
#include <audio/audio.h>
#include <audio/music.h>
#include <audio/parse_aiff.h>
#include <audio/patchfile.h>
#include <misc/event.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */
#define	DBUG2(x) /* PRT(x) */
#define MAX_WETNESS (0.05)	/* default maximum amount of reverb */
#define TIME_INCR (20)	/* delta(t) for velocity calculations, in audio ticks */
#define X_INCR (11)
#define Y_INCR (11)	/* default walking rate  =  128 frames/m / 240 tps * 20 */

#define WALKING (0)
#define WHISPERING (1)
#define TALKING (2)
#define YELLING (3)
#define NUM_SOUNDS (4)

#define WHISPER_RADIUS (350)
#define TALKING_RADIUS (2000)

#define CUE_MASK (S3D_F_PAN_FILTER \
  | S3D_F_PAN_DELAY \
  | S3D_F_OUTPUT_HEADPHONES \
  | S3D_F_DISTANCE_AMPLITUDE_SQUARE)

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		PrintError (NULL, name, NULL, val); \
		goto cleanup; \
	}

typedef struct CartesianCoords
{
	float32 xyz_X;
	float32 xyz_Y;
	float32 xyz_Z;
	float32 xyz_Time;
} CartesianCoords;

/* Structure to keep track of the source sound items */
typedef struct TS3DSound
{
	Item ts3d_Source;
	Sound3D* ts3d_Sound3DPtr;
	Item ts3d_FreqKnob;
	Item ts3d_NominalFrequency;
	Item ts3d_Samples[NUM_SOUNDS];
	Item ts3d_Attachment;
	CartesianCoords ts3d_lastTarget;
} TS3DSound;

/* Structure to keep track of environmental items */
typedef struct Sound3DSpace
{
	Item s3ds_OutputTemplate;
	Item s3ds_OutputInst;
	Item s3ds_LineOut;
	Item s3ds_WetKnob;
	Item s3ds_DryKnob;
	Item s3ds_MixerTemplate;
	Item s3ds_Mixer;
} Sound3DSpace;

/* function declarations */
uint32 t3dsSetCues( char* cueList );
Sound3DSpace* CreateSoundSpace( void );
void DeleteSoundSpace( Sound3DSpace *Snd3DSpace );
Err TestS3DInit(TS3DSound sounds[], Sound3DSpace** s3dSpaceHandle, uint32 t3dsCues);
Err TestS3DAnimate(TS3DSound sounds[], Sound3DSpace* s3dSpace);
void TestS3DTerm(TS3DSound sounds[], Sound3DSpace* s3dSpace);

/* Globals */
float32 t3dsGlobalReverb;

int32 main( int32 argc, char *argv[])
{

	int32 Result;
	TS3DSound s3dSounds[4];
	Sound3DSpace* s3dSpace = NULL;
	uint32 t3dsCues;

	PRT(("Usage: ta_steer3d [reverb [cuelist]]\n\n"));

	PRT(("This test lets you steer a walking sound around.  Use left/right/up/down\n"));
	PRT(("to change the velocity of the walking by about 1 m/s, STOP to hear a sample\n"));
	PRT(("at the point you've reached, START to restart the walking at a velocity of\n"));
	PRT(("1 m/s relative north, and A to stop the program.\n\n"));

	PRT(("reverb = global reverberation amount (0.0 to 1.0), defaults to 0.05\n"));
	PRT(("cuelist = abc..., where each digit corresponds to one of the following:\n"));
	PRT(("  1: Interaural Time Delay\n"));
	PRT(("  2: Interaural Amplitude Difference\n"));
	PRT(("  3: Pseudo-HRTF Filter\n"));
	PRT(("  4: Distance-squared rule for amplitude attenuation\n"));
	PRT(("  5: Distance-cubed rule for amplitude attenuation\n"));
	PRT(("  7: Doppler shift\n"));

	/* Retrieve command line arguments */
	t3dsGlobalReverb = (argc > 1) ? strtof( argv[1], NULL ) : MAX_WETNESS;
	t3dsCues = (argc > 2) ? t3dsSetCues( argv[2] ) : CUE_MASK ;
DBUG(("Cues: %x\n",t3dsCues));

	if ((Result = InitEventUtility (1, 0, TRUE)) < 0)
	{
		PrintError (NULL, "init event utility", NULL, Result);
		goto cleanup;
	}

	Result = TestS3DInit(s3dSounds, &s3dSpace, t3dsCues);
	CHECKRESULT(Result, "TestS3DInit");

	Result = TestS3DAnimate(s3dSounds, s3dSpace);
	CHECKRESULT(Result, "TestS3DAnimate");

cleanup:
	TestS3DTerm(s3dSounds, s3dSpace);
	KillEventUtility();
	printf("ta_steer3d done\n");

	return((int32) Result);

}

/****************************************************************/
uint32 t3dsSetCues( char* cueList )
{
	int32 i, result=0;
	char digit;

	for (i=0;i<strlen(cueList);i++)
	{
		digit = *(cueList+i);
		result |= 1<<(atoi(&digit)-1);
	}

	return (result);
}

/****************************************************************/
Sound3DSpace *CreateSoundSpace( void )
/* Allocate in this routine in case size changes. */
{
	Sound3DSpace *Snd3DSpace;
	int32 Result;

	Snd3DSpace = (Sound3DSpace *) AllocMem(sizeof(Sound3DSpace), MEMTYPE_NORMAL);
	if (Snd3DSpace != NULL)
	{
		Snd3DSpace->s3ds_OutputTemplate = LoadPatchTemplate("sound3dspace_nice_stereo.patch");
		CHECKRESULT(Snd3DSpace->s3ds_OutputTemplate, "LoadPatchTemplate sound3dspace_nice_stereo.patch");

		Snd3DSpace->s3ds_OutputInst = CreateInstrument(Snd3DSpace->s3ds_OutputTemplate, NULL);
		CHECKRESULT(Snd3DSpace->s3ds_OutputInst, "CreateInstrument");

/* Connect wet/dry gain knobs to the space's output mixer */
		Snd3DSpace->s3ds_WetKnob = CreateKnob(Snd3DSpace->s3ds_OutputInst, "Wet",
		  NULL);
		CHECKRESULT(Snd3DSpace->s3ds_WetKnob, "CreateKnob");

		Snd3DSpace->s3ds_DryKnob = CreateKnob(Snd3DSpace->s3ds_OutputInst, "Dry",
		  NULL);
		CHECKRESULT(Snd3DSpace->s3ds_DryKnob, "CreateKnob");

/* Load the output instrument */
		Snd3DSpace->s3ds_LineOut = LoadInstrument("line_out.dsp", 0, 100);
		CHECKRESULT(Snd3DSpace->s3ds_LineOut, "LoadInstrument");

/* Connect the space to the output instrument */
		Result = ConnectInstrumentParts(
		   Snd3DSpace->s3ds_OutputInst, "Output", 0,
		   Snd3DSpace->s3ds_LineOut, "Input", 0);
		CHECKRESULT(Result, "ConnectInstrumentParts");
		Result = ConnectInstrumentParts(
		   Snd3DSpace->s3ds_OutputInst, "Output", 1,
		   Snd3DSpace->s3ds_LineOut, "Input", 1);
		CHECKRESULT(Result, "ConnectInstrumentParts");

		return Snd3DSpace;
	}
	else
		return NULL;

cleanup:
	DeleteSoundSpace( Snd3DSpace );
	TOUCH(Result);
	return NULL;
}

/****************************************************************/
void DeleteSoundSpace( Sound3DSpace *Snd3DSpace )
{
	if (Snd3DSpace)
	{
		UnloadPatchTemplate( Snd3DSpace->s3ds_OutputTemplate );
		UnloadInstrument( Snd3DSpace->s3ds_LineOut );

		FreeMem (Snd3DSpace, sizeof(Snd3DSpace));
	}

	return;
}

/****************************************************************/
Err TestS3DLoadSourceSounds(TS3DSound* sounds, uint32 t3dsCues)
{
	Err Result;
	float32 NominalFreq;
	Item s3dInst;

	/* Allocate and set up the 3D context */
	Result = Create3DSoundVA( &(sounds->ts3d_Sound3DPtr),
	  S3D_TAG_FLAGS, t3dsCues, TAG_END );
	CHECKRESULT(Result, "Create3DSound");

	/* Set up the source instrument */
	sounds->ts3d_Source = LoadInstrument("sampler_16_v1.dsp", 1, 100);
	CHECKRESULT(sounds->ts3d_Source, "LoadInstrument");

DBUG(("Made source instrument, item %i\n", sounds->ts3d_Source));

	/* Connect the source to the 3D instrument context*/
	s3dInst = Get3DSoundInstrument( sounds->ts3d_Sound3DPtr );
	CHECKRESULT(s3dInst, "Get3DSoundInstrument");

	Result = ConnectInstruments(sounds->ts3d_Source, "Output", s3dInst,
	  "Input");
	CHECKRESULT(Result, "ConnectInstrument");

	/* Connect the Frequency knob, for doppler */
	sounds->ts3d_FreqKnob = CreateKnob(sounds->ts3d_Source, "SampleRate",
	  NULL);
	CHECKRESULT(sounds->ts3d_FreqKnob,
	  "CreateKnob: can't connect to doppler");

	/* Read the default knob value, assume that's nominal */
	Result = ReadKnob(sounds->ts3d_FreqKnob, &NominalFreq);
	CHECKRESULT(Result, "ReadKnob");
	sounds->ts3d_NominalFrequency = NominalFreq;

	/* Read in all the sample items, attach WALKING */
	sounds->ts3d_Samples[WALKING] = LoadSample(
	  "FootStepLoop.44m.aiff" );
	CHECKRESULT(Result, "LoadSample");

	sounds->ts3d_Samples[WHISPERING] = LoadSample(
	  "whispering.aiff" );
	CHECKRESULT(Result, "LoadSample");

	sounds->ts3d_Samples[TALKING] = LoadSample(
	  "GarbleTalk2Short.44m.aiff" );
	CHECKRESULT(Result, "LoadSample");

	sounds->ts3d_Samples[YELLING] = LoadSample(
	  "Scream.44m.aiff" );
	CHECKRESULT(Result, "LoadSample");

	sounds->ts3d_Attachment = CreateAttachment( sounds->ts3d_Source,
	  sounds->ts3d_Samples[WALKING], NULL );
	CHECKRESULT(sounds->ts3d_Attachment, "AttachSample");

cleanup:
	return (Result);
}

/****************************************************************/
Err TestS3DInit(TS3DSound* sounds, Sound3DSpace** s3dSpaceHandle, uint32 t3dsCues)
{
	Err Result;
	Item Inst3D, SpaceInst;

	*s3dSpaceHandle = NULL;

	/* Initialize audio, return if error. */
	if ((Result = OpenAudioFolio()) < 0)
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(Result);
	}

	/* Allocate and set up the sounds */
	Result = TestS3DLoadSourceSounds( sounds, t3dsCues );
	CHECKRESULT(Result, "TestS3DLoadSourceSound");

	/* Allocate and set up the space */
	if ((*s3dSpaceHandle = CreateSoundSpace()) == NULL)
	{
		ERR(("Couldn't allocate 3D sound space!\n"));
		return(-1);
	}

	SpaceInst = (*s3dSpaceHandle)->s3ds_OutputInst;

	/* Connect the sounds to the space */
	Inst3D = Get3DSoundInstrument( sounds->ts3d_Sound3DPtr );
	CHECKRESULT(Inst3D, "Get3DSoundInstrument");

	Result = ConnectInstrumentParts(Inst3D, "Output", 0,
	  SpaceInst, "Input", 0);
	CHECKRESULT(Result, "ConnectInstrumentParts");
	Result = ConnectInstrumentParts(Inst3D, "Output", 1,
	  SpaceInst, "Input", 1);
	CHECKRESULT(Result, "ConnectInstrumentParts");

cleanup:
	return(Result);
}

/****************************************************************/
void TestS3DTerm(TS3DSound* sounds, Sound3DSpace* s3dSpace)
{
	int32 i;

	UnloadInstrument(sounds->ts3d_Source);
	DeleteAttachment(sounds->ts3d_Attachment);

	for (i=0;i<NUM_SOUNDS;i++)
		DeleteSample(sounds->ts3d_Samples[i]);

	Delete3DSound(&(sounds->ts3d_Sound3DPtr));
	DeleteSoundSpace(s3dSpace);
	CloseAudioFolio();

	return;
}

/****************************************************************/
void PolarToXYZ( PolarPosition4D* polar, CartesianCoords* cart )
{
	cart->xyz_X = polar->pp4d_Radius * sinf(polar->pp4d_Theta);
	cart->xyz_Y = polar->pp4d_Radius * cosf(polar->pp4d_Theta);
	cart->xyz_Z = 0;
	cart->xyz_Time = polar->pp4d_Time;

	return;
}

/****************************************************************/
void XYZToPolar( CartesianCoords* cart, PolarPosition4D* polar )
{
	polar->pp4d_Radius = sqrtf(cart->xyz_X * cart->xyz_X +
	  cart->xyz_Y * cart->xyz_Y);
	polar->pp4d_Theta = atan2f(cart->xyz_X, cart->xyz_Y);
	polar->pp4d_Phi = 0.0;
	polar->pp4d_Time = cart->xyz_Time;

	return;
}

/****************************************************************/
void TestCoordConversion( void )
{
	PolarPosition4D polar, pback;
	CartesianCoords cart;
	int32 i;

	polar.pp4d_Radius = 1000;

	for (i=0;i<=12;i++)
	{
		polar.pp4d_Theta = s3dNormalizeAngle( i * S3D_FULLCIRCLE / 12 );
		PolarToXYZ( &polar, &cart );
		XYZToPolar( &cart, &pback );
		PRT(("%2i2PI/6: %i  %f  %f  %f  %i  %f\n",
		  i,
		  polar.pp4d_Radius, polar.pp4d_Theta,
		  cart.xyz_X, cart.xyz_Y,
		  pback.pp4d_Radius, pback.pp4d_Theta));
	}

	return;
}

/***********************************************************************/
Err TestS3DUpdateHost( TS3DSound* aSound, Sound3DSpace* s3dSpace )
{
	Err Result;

	float32 Wetness, Dryness, Amplitude, Doppler;
	Sound3D* s3dContext;
	Sound3DParms s3dParms;

/* Do all the host updates that the API doesn't do */

	s3dContext = aSound->ts3d_Sound3DPtr;

	Result = Get3DSoundParms( s3dContext, &s3dParms, sizeof(s3dParms) );
	CHECKRESULT(Result, "Get3DSoundParms");
DBUG(("Parms: Doppler %f, DF %f\n", s3dParms.s3dp_Doppler, s3dParms.s3dp_DistanceFactor));

	Doppler = s3dParms.s3dp_Doppler;
	Result = SetKnob(aSound->ts3d_FreqKnob, aSound->ts3d_NominalFrequency
	  * Doppler);
	CHECKRESULT(Result, "SetKnob");

/*
   Reverb calculations (from Dodge and Jerse):
   Dry signal attenuates proportionally to distance rule (1/DF).  Wet signal
   attenuates proportionally to the square root of distance (1/sqrt(DF)).
   We don't differentiate between local and global reverberation here.
*/

	Amplitude = s3dParms.s3dp_DistanceFactor;
	Dryness = (1.0 - t3dsGlobalReverb) * Amplitude;
	Wetness = t3dsGlobalReverb * sqrtf( Amplitude );

	Result = SetKnob(s3dSpace->s3ds_DryKnob, Dryness);
	CHECKRESULT(Result, "SetKnob");

	Result = SetKnob(s3dSpace->s3ds_WetKnob, Wetness);
	CHECKRESULT(Result, "SetKnob");

cleanup:
	return (Result);
}

/***********************************************************************/
Err TestS3DUpdateFromVelocity( CartesianCoords* velocity, TS3DSound* aSound,
  Sound3DSpace* s3dSpace )
{
	int32 Result=0;
	PolarPosition4D target, start;
	CartesianCoords start_xyz, target_xyz;
	Sound3D* s3dContext;

	s3dContext = aSound->ts3d_Sound3DPtr;

/* Old end becomes start */
	start_xyz.xyz_X = aSound->ts3d_lastTarget.xyz_X;
	start_xyz.xyz_Y = aSound->ts3d_lastTarget.xyz_Y;

	start_xyz.xyz_Time = GetAudioTime();

DBUG2(("WhereamI: %i, %i\n",(int32)start_xyz.xyz_X, (int32)start_xyz.xyz_Y));
DBUG2(("Velocity: %i, %i\n",(int32)velocity->xyz_X, (int32)velocity->xyz_Y));

	target_xyz.xyz_X = start_xyz.xyz_X + velocity->xyz_X;
	target_xyz.xyz_Y = start_xyz.xyz_Y + velocity->xyz_Y;
	target_xyz.xyz_Time = (uint16)(((uint16)start_xyz.xyz_Time + (uint16)velocity->xyz_Time) & 0xFFFF);

DBUG2(("Target: %i, %i\n",(int32)target_xyz.xyz_X, (int32)target_xyz.xyz_Y));

	XYZToPolar( &target_xyz, &target );
	XYZToPolar( &start_xyz, &start );

	Move3DSound( s3dContext, &start, &target );
	TestS3DUpdateHost( aSound, s3dSpace );

/* Save target for next time */
	aSound->ts3d_lastTarget.xyz_X = target_xyz.xyz_X;
	aSound->ts3d_lastTarget.xyz_Y = target_xyz.xyz_Y;

	return Result;
}

/***********************************************************************/
Err TestS3DSteer( TS3DSound* sounds, Sound3DSpace* s3dSpace )
{
	int32 Result, doit=TRUE, stopped, stopSound;
	ControlPadEventData cped;
	uint32 joy;
	PolarPosition4D whereamI = {S3D_DISTANCE_TO_EAR, 0.0, 0.0, 0};
	CartesianCoords velocity;
	Item sleepCue;
	float32 tps;

	/* Allocate cue for waiting */
	sleepCue = CreateCue( NULL );
	CHECKRESULT(sleepCue, "CreateCue");

	velocity.xyz_X = 0;
	velocity.xyz_Y = Y_INCR;
	Result = GetAudioClockRate( AF_GLOBAL_CLOCK, &tps );
	CHECKRESULT(Result, "GetAudioClockRate");
	velocity.xyz_Time = (int32)TIME_INCR * tps;

	stopped = FALSE;

	PolarToXYZ(&whereamI, &(sounds->ts3d_lastTarget));
	whereamI.pp4d_Time = GetAudioFrameCount();
	Start3DSound( sounds->ts3d_Sound3DPtr, &whereamI );
	TestS3DUpdateHost( sounds, s3dSpace );
	StartInstrument( sounds->ts3d_Source, NULL );
	Get3DSoundPos( sounds->ts3d_Sound3DPtr, &whereamI );

DBUG(("I'm starting at (radius: %i, theta: %f)\n",
  (int32)whereamI.pp4d_Radius, whereamI.pp4d_Theta));

	while (doit)
	{
		Result = GetControlPad (1, FALSE, &cped);
		if (Result < 0)
		{
			PrintError(0,"get control pad data in","ta_steer3d",Result);
		}

		joy = cped.cped_ButtonBits;

		if (joy & ControlA)
		{
			doit = FALSE;
		}

/* Left, Right, Up, Down */
		if ((joy & ControlLeft) && !stopped)
		{
			velocity.xyz_X -= X_INCR;
			PRT(("Left: (%i, %i) frames\n", (int32)velocity.xyz_X, (int32)velocity.xyz_Y));
		}
		if ((joy & ControlRight) && !stopped)
		{
			velocity.xyz_X += X_INCR;
			PRT(("Right: (%i, %i) frames\n", (int32)velocity.xyz_X, (int32)velocity.xyz_Y));
		}
		if ((joy & ControlUp) && !stopped)
		{
			velocity.xyz_Y += Y_INCR;
			PRT(("Up: (%i, %i) frames\n", (int32)velocity.xyz_X, (int32)velocity.xyz_Y));
		}
		if ((joy & ControlDown) && !stopped)
		{
			velocity.xyz_Y -= Y_INCR;
			PRT(("Down: (%i, %i) frames\n", (int32)velocity.xyz_X, (int32)velocity.xyz_Y));
		}

/* Stop */
		if ((joy & ControlX) && !stopped)
		{
			Get3DSoundPos( sounds->ts3d_Sound3DPtr, &whereamI );
			PRT(("Stopped at (%i frames, %f radians)...",(int32)whereamI.pp4d_Radius, whereamI.pp4d_Theta));

			/* Stop moving */
			Move3DSound( sounds->ts3d_Sound3DPtr, &whereamI, &whereamI );

			StopInstrument( sounds->ts3d_Source, NULL );

			velocity.xyz_X = 0;
			velocity.xyz_Y = 0;

			if (whereamI.pp4d_Radius < WHISPER_RADIUS) stopSound = WHISPERING;
			else if (whereamI.pp4d_Radius < TALKING_RADIUS) stopSound = TALKING;
			else stopSound = YELLING;

			/* Change the sample */
			DeleteAttachment( sounds->ts3d_Attachment );
			sounds->ts3d_Attachment = CreateAttachment( sounds->ts3d_Source,
			  sounds->ts3d_Samples[stopSound], NULL );
			CHECKRESULT(sounds->ts3d_Attachment, "AttachSample");

			StartInstrument( sounds->ts3d_Source, NULL );

			stopped = TRUE;
		}

/* Start again */
		if ((joy & ControlStart) && stopped)
		{
			Get3DSoundPos( sounds->ts3d_Sound3DPtr, &whereamI );
			PRT(("Started.\n"));

			StopInstrument( sounds->ts3d_Source, NULL );

			velocity.xyz_X = 0;
			velocity.xyz_Y = Y_INCR;

			/* Change the sample */
			DeleteAttachment( sounds->ts3d_Attachment );
			sounds->ts3d_Attachment = CreateAttachment( sounds->ts3d_Source,
			  sounds->ts3d_Samples[WALKING], NULL );
			CHECKRESULT(sounds->ts3d_Attachment, "AttachSample");

			StartInstrument( sounds->ts3d_Source, NULL );

			stopped = FALSE;
		}

/* Update the sound */

		if (!stopped)
		{
			TestS3DUpdateFromVelocity( &velocity, sounds, s3dSpace );
			SleepUntilTime( sleepCue, GetAudioTime() + TIME_INCR );
		}
	}

cleanup:
	StopInstrument( sounds->ts3d_Source, NULL);
	Stop3DSound( sounds->ts3d_Sound3DPtr );
	DeleteCue( sleepCue );

	return (Result);
}

/***********************************************************************/
Err TestS3DAnimate(TS3DSound* sounds, Sound3DSpace* s3dSpace)
{
	Err Result;

DBUG(("Starting everything up\n"));

	/* Start the output instrument */
	Result = StartInstrument(s3dSpace->s3ds_LineOut, NULL);
	CHECKRESULT(Result, "StartInstrument");
	Result = StartInstrument(s3dSpace->s3ds_OutputInst, NULL);
	CHECKRESULT(Result, "StartInstrument");

	/* Go into the movement refresh loop */
	Result = TestS3DSteer( sounds, s3dSpace );

cleanup:

DBUG(("Stopping everything\n"));

	StopInstrument(s3dSpace->s3ds_OutputInst, NULL);
	StopInstrument(s3dSpace->s3ds_LineOut, NULL);

	return(Result);
}
