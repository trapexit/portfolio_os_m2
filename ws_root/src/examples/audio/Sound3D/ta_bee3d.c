/***************************************************************
**
** @(#) ta_bee3d.c 96/08/27 1.14
**
** Test sound3d library: a small swarm of bees
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
|||	AUTODOC -public -class examples -group Audio -name ta_bee3d
|||	Three sounds in a navigable space using 3DSound API.
|||
|||	  Format
|||
|||	    ta_bee3d [reverb [cuelist]]
|||
|||	  Description
|||
|||	    This program positions three sounds in a three-dimensional space,
|||	    and allows the user to move about in the space using the control
|||	    pad.  The sounds are positioned and moved using the 3DSound functions
|||	    in the music library.  By varying the command line arguments, the
|||	    user can hear the relative effect of the various 3DSound cues on the
|||	    three-dimensional illusion.
|||
|||	    To move around using the control pad:
|||
|||	    Up
|||	        moves you forward
|||	    Down
|||	        moves you backward
|||	    Left
|||	        rotates you anticlockwise
|||	    Right
|||	        rotates you clockwise
|||	    A
|||	        repositions you under the "bee" sound.
|||
|||	    Two of the sounds are stationary: a looped "clonking" sample and a
|||	    synthetic telephone.  The third, a synthetic bee, does a random walk
|||	    confined to a predetermined distance from its initial location.
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
|||	        reverberation amount in the final mix.  The amount of sound
|||	        sent to the reverb is independently calculated for each sound,
|||	        based on the distance of the sound from the observer, and the
|||	        overall wet/dry mix is controlled with this parameter.  By
|||	        default, the value is 0.05.  The higher the number, the harder
|||	        it becomes to accurately determine the position of a distant
|||	        sound.
|||
|||	    cuelist
|||	        a string of digits selected from the list below.  Including
|||	        a digit in the string enables the corresponding cue.  The default
|||	        cuelist is equivalent to the string 134.
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
|||	    patches/bee3d.mp, patches/bee3dspace.mp, patches/clonk.mp,
|||	    patches/phone.mp, makepatch.script
|||
|||	  Location
|||
|||	    Examples/Audio/Sound3D
|||
|||	  See Also
|||
|||	    SeeSound(@)
|||
**/

#include <kernel/types.h>
#include <kernel/debug.h>
#include <kernel/operror.h>
#include <audio/audio.h>
#include <audio/music.h>
#include <audio/parse_aiff.h>
#include <misc/event.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	PRT(x)
#define	DBUG2(x) /* PRT(x) */
#define MAX_WETNESS (0.05)	/* default maximum amount of reverb */
#define TIME_INCR (20)	/* delta(t) for velocity calculations, in audio ticks */

#define NUMBER_OF_BEES (1)
#define NUMBER_OF_SOUNDS (NUMBER_OF_BEES + 2)
#define BEE_MAX_RADIUS (10 * S3D_DISTANCE_TO_EAR)
#define BEE_CHANGE_DIRECTION_THRESHOLD (0.1)
#define BEE_MAX_DELTA_RADIUS (S3D_DISTANCE_TO_EAR)
#define BEE_MAX_DELTA_THETA (M_PI / 6.0)
#define OBSERVER_ANGLE_INCR (M_PI / 12.0)
#define OBSERVER_RADIUS_INCR (BEE_MAX_RADIUS)

#define CUE_MASK (S3D_F_PAN_FILTER \
  | S3D_F_PAN_DELAY \
  | S3D_F_OUTPUT_HEADPHONES \
  | S3D_F_DISTANCE_AMPLITUDE_SQUARE)

#ifndef SIGNEXTEND
#define SIGNEXTEND(n) (((n) & 0x8000) ? (n)|0xFFFF0000 : (n))
#endif

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		ERR(("Failure in %s: $%x\n", name, val)); \
		goto cleanup; \
	}

/* Structure to keep track of the source sound items */
typedef struct beeSound
{
	Item bee_Source;
	Sound3D* bee_Sound3DPtr;
	Item bee_FreqKnob;
	Item bee_NominalFrequency;
	PolarPosition4D bee_Target;
} beeSound;

/* Structure to keep track of environmental items */
typedef struct Sound3DSpace
{
	Item bees_Template;
	Item bees_ClonkTemplate;
	Item bees_PhoneTemplate;
	Item bees_OutputTemplate;
	Item bees_OutputInst;
	Item bees_LineOut;
	Item bees_MixerTemplate;
	Item bees_Mixer;
	Item bees_GainKnob;
	Item bees_PresendKnob;
} Sound3DSpace;

typedef struct CartesianCoords
{
	float32 xyz_X;
	float32 xyz_Y;
	float32 xyz_Z;
	float32 xyz_Time;
} CartesianCoords;

/* Globals */
float32 beeGlobalReverb;

/* Functions */
uint32 beeSetCues( char* cueList );
Sound3DSpace *CreateSoundSpace( void );
int32 DeleteSoundSpace( Sound3DSpace *Snd3DSpace );
Err beeLoadSourcePatch(beeSound* sound, uint32 beeCues, char* patchName );
Err beeInit(beeSound sounds[], Sound3DSpace** beespaceHandle, uint32 beeCues);
void beeTerm(beeSound sounds[], Sound3DSpace* beespace);
Err beeAnimate(beeSound sounds[], Sound3DSpace* beeSpace);
Err beeSteer(beeSound sounds[], Sound3DSpace* beeSpace);

int32 main( int32 argc, char *argv[])
{

	int32 Result, i;
	beeSound beeSounds[NUMBER_OF_SOUNDS];
	Sound3DSpace* beespace = NULL;
	uint32 beeCues;

	PRT(("Usage: ta_bee3d [reverb [cuelist]]\n"));
	PRT(("reverb: 0.0-1.0, global reverberation level\n"));
	PRT(("cuelist: abc.., where a,b,c, and so on are digits from the\n"));
	PRT(("  list below.\n"));
	PRT(("  1: Interaural Time Delay\n"));
	PRT(("  2: Interaural Amplitude Difference\n"));
	PRT(("  3: Pseudo-HRTF Filter\n"));
	PRT(("  4: Distance-squared rule for amplitude attenuation\n"));
	PRT(("  5: Distance-cubed rule for amplitude attenuation\n"));
	PRT(("  7: Doppler shift\n"));
	PRT(("There are three sounds in the space: a moving bee, a clanking\n"));
	PRT(("machine and a telephone.  Use left/right to turn, up to move\n"));
	PRT(("forward, down to move backward, A to position back under the bee.\n"));

	/* Retrieve command line arguments */
	beeGlobalReverb = (argc > 1) ? strtof( argv[1], NULL ) : MAX_WETNESS;
	beeCues = (argc > 2) ? beeSetCues( argv[2] ) : CUE_MASK ;
DBUG(("Cues: %x\n",beeCues));

	if ((Result = InitEventUtility (1, 0, TRUE)) < 0)
	{
		PrintError (NULL, "init event utility", NULL, Result);
		goto cleanup;
	}

	for (i=0;i<NUMBER_OF_SOUNDS;i++)
		beeSounds[i].bee_Sound3DPtr = NULL;

	Result = beeInit(beeSounds, &beespace, beeCues);
	CHECKRESULT(Result, "beeInit");

	Result = beeAnimate(beeSounds, beespace);
	CHECKRESULT(Result, "beeAnimate");

cleanup:
	beeTerm(beeSounds, beespace);
	KillEventUtility();
	printf("ta_bee3d done\n");

	return((int32) Result);

}

/****************************************************************/
void CheckResources( void )
{
	AudioResourceInfo* info;

	/* check resource usage */
	if ((info = malloc(sizeof(AudioResourceInfo))) != NULL)
	{
		GetAudioResourceInfo (info, sizeof(AudioResourceInfo), AF_RESOURCE_TYPE_TICKS);
		PRT(("Free ticks: %i\n", info->rinfo_Free));
		GetAudioResourceInfo (info, sizeof(AudioResourceInfo), AF_RESOURCE_TYPE_CODE_MEM);
		PRT(("Free code: %i\n", info->rinfo_Free));
		GetAudioResourceInfo (info, sizeof(AudioResourceInfo), AF_RESOURCE_TYPE_DATA_MEM);
		PRT(("Free data: %i\n", info->rinfo_Free));
		GetAudioResourceInfo (info, sizeof(AudioResourceInfo), AF_RESOURCE_TYPE_FIFOS);
		PRT(("Free fifos: %i\n", info->rinfo_Free));
		free(info);
	}
}

/****************************************************************/
uint32 beeSetCues( char* cueList )
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

	Snd3DSpace = (Sound3DSpace *) malloc(sizeof(Sound3DSpace));
	if (Snd3DSpace != NULL)
	{
		Snd3DSpace->bees_OutputTemplate = LoadScoreTemplate("bee3dspace.patch");
		CHECKRESULT(Snd3DSpace->bees_OutputTemplate, "LoadScoreTemplate");

		Snd3DSpace->bees_OutputInst = CreateInstrument(Snd3DSpace->bees_OutputTemplate, NULL);
		CHECKRESULT(Snd3DSpace->bees_OutputInst, "CreateInstrument");

/* Load the output instrument */
		Snd3DSpace->bees_LineOut = LoadInstrument("line_out.dsp", 0, 100);
		CHECKRESULT(Snd3DSpace->bees_LineOut, "LoadInstrument");

/* Connect the space to the output instrument */
		Result = ConnectInstrumentParts(
		   Snd3DSpace->bees_OutputInst, "Output", 0,
		   Snd3DSpace->bees_LineOut, "Input", 0);
		CHECKRESULT(Result, "ConnectInstrumentParts");
		Result = ConnectInstrumentParts(
		   Snd3DSpace->bees_OutputInst, "Output", 1,
		   Snd3DSpace->bees_LineOut, "Input", 1);
		CHECKRESULT(Result, "ConnectInstrumentParts");

		Snd3DSpace->bees_GainKnob = CreateKnob(
		  Snd3DSpace->bees_OutputInst, "Gain", 0 );
		CHECKRESULT(Snd3DSpace->bees_GainKnob, "CreateKnob");

		Snd3DSpace->bees_PresendKnob = CreateKnob(
		  Snd3DSpace->bees_OutputInst, "Presend", 0 );
		CHECKRESULT(Snd3DSpace->bees_GainKnob, "CreateKnob");

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
int32 DeleteSoundSpace( Sound3DSpace *Snd3DSpace )
{
	int32 Result=0;

	if (Snd3DSpace)
	{
		UnloadScoreTemplate( Snd3DSpace->bees_OutputTemplate );
		UnloadInstrument( Snd3DSpace->bees_LineOut );

		free (Snd3DSpace);
	}

	return Result;
}

/****************************************************************/
Err beeMakeSource(beeSound* sound, uint32 beeCues, Item beeTemplate )
{
	Err Result;
	float32 NominalFreq;
	Item s3dInst;
	TagArg Tags[] = { { S3D_TAG_FLAGS }, TAG_END };

	/* Allocate and set up the 3D context */
	Tags[0].ta_Arg = (TagData) beeCues;
	Result = Create3DSound( &(sound->bee_Sound3DPtr), Tags );
	CHECKRESULT(Result, "Create3DSound");

	/* Set up the source instrument */
	sound->bee_Source = CreateInstrument(beeTemplate, 0);
	CHECKRESULT(sound->bee_Source, "CreateInstrument");

DBUG(("Made source sound, item %i\n", sound->bee_Source));

	/* Connect the source to the 3D instrument context*/
	s3dInst = Get3DSoundInstrument( sound->bee_Sound3DPtr );
	CHECKRESULT(s3dInst, "Get3DSoundInstrument");

	Result = ConnectInstruments(sound->bee_Source, "Output", s3dInst,
	  "Input");
	CHECKRESULT(Result, "ConnectInstrument");

	/* Connect the Frequency knob, for doppler */
	sound->bee_FreqKnob = CreateKnob(sound->bee_Source, "SampleRate",
	  NULL);
	CHECKRESULT(sound->bee_FreqKnob,
	  "CreateKnob: can't connect to doppler");

	/* Read the default knob value, assume that's nominal */
	Result = ReadKnob(sound->bee_FreqKnob, &NominalFreq);
	CHECKRESULT(Result, "ReadKnob");
	sound->bee_NominalFrequency = NominalFreq;

cleanup:

DBUG(("New 3DSound at 0x%x\n", sound->bee_Sound3DPtr));

	return (Result);
}

/****************************************************************/
Err beeInit(beeSound sounds[], Sound3DSpace** beespaceHandle, uint32 beeCues)
{
	Err Result;
	Item Inst3D, SpaceInst, beeTemplate;
	int32 i;

	CheckResources();

	/* Initialize audio, return if error. */
	if ((Result = OpenAudioFolio()) < 0)
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(Result);
	}

	/* Allocate and set up the space */
	if ((*beespaceHandle = CreateSoundSpace()) == NULL)
	{
		ERR(("Couldn't allocate 3D sound space!\n"));
		return(-1);
	}

	CheckResources();

	SpaceInst = (*beespaceHandle)->bees_OutputInst;

	/* Allocate and set up the sounds */
	beeTemplate = LoadScoreTemplate("bee3d.patch");
	CHECKRESULT(beeTemplate, "LoadScoreTemplate");

	/* Save the source template info so we can destroy it on exit */
	(*beespaceHandle)->bees_Template = beeTemplate;

	for (i=0;i<NUMBER_OF_BEES;i++)
	{
		Result = beeMakeSource( &(sounds[i]), beeCues, beeTemplate );
		CHECKRESULT(Result, "beeMakeSource");

		Inst3D = Get3DSoundInstrument( sounds[i].bee_Sound3DPtr );
		CHECKRESULT(Inst3D, "Get3DSoundInstrument");

		Result = ConnectInstrumentParts(Inst3D, "Output", 0,
		  SpaceInst, "Input", i*2);
		CHECKRESULT(Result, "ConnectInstrumentParts");
		Result = ConnectInstrumentParts(Inst3D, "Output", 1,
		  SpaceInst, "Input", (i*2)+1);
		CHECKRESULT(Result, "ConnectInstrumentParts");

		CheckResources();
	}

	/* Other sounds */
	beeTemplate = LoadScoreTemplate("clonk.patch");
	CHECKRESULT(beeTemplate, "LoadScoreTemplate");

	(*beespaceHandle)->bees_ClonkTemplate = beeTemplate;
	Result = beeMakeSource( &(sounds[NUMBER_OF_BEES]), beeCues,
	  beeTemplate );
	CHECKRESULT(Result, "beeMakeSource");

	Inst3D = Get3DSoundInstrument( sounds[NUMBER_OF_BEES].bee_Sound3DPtr );
	CHECKRESULT(Inst3D, "Get3DSoundInstrument");

	Result = ConnectInstrumentParts(Inst3D, "Output", 0,
	  SpaceInst, "Input", NUMBER_OF_BEES*2);
	CHECKRESULT(Result, "ConnectInstrumentParts");
	Result = ConnectInstrumentParts(Inst3D, "Output", 1,
	  SpaceInst, "Input", (NUMBER_OF_BEES*2)+1);
	CHECKRESULT(Result, "ConnectInstrumentParts");

	CheckResources();

	beeTemplate = LoadScoreTemplate("phone.patch");
	CHECKRESULT(beeTemplate, "LoadScoreTemplate");

	(*beespaceHandle)->bees_PhoneTemplate = beeTemplate;
	Result = beeMakeSource( &(sounds[NUMBER_OF_BEES+1]), beeCues,
	  beeTemplate );
	CHECKRESULT(Result, "beeMakeSource");

	Inst3D = Get3DSoundInstrument( sounds[NUMBER_OF_BEES+1].bee_Sound3DPtr );
	CHECKRESULT(Inst3D, "Get3DSoundInstrument");

	Result = ConnectInstrumentParts(Inst3D, "Output", 0,
	  SpaceInst, "Input", (NUMBER_OF_BEES+1)*2);
	CHECKRESULT(Result, "ConnectInstrumentParts");
	Result = ConnectInstrumentParts(Inst3D, "Output", 1,
	  SpaceInst, "Input", ((NUMBER_OF_BEES+1)*2)+1);
	CHECKRESULT(Result, "ConnectInstrumentParts");

	CheckResources();
cleanup:
	return(Result);
}

/****************************************************************/
void beeTerm(beeSound sounds[], Sound3DSpace* beespace)
{
	int32 i;

	UnloadScoreTemplate(beespace->bees_Template);
	UnloadScoreTemplate(beespace->bees_ClonkTemplate);
	UnloadScoreTemplate(beespace->bees_PhoneTemplate);

	for (i=0;i<NUMBER_OF_SOUNDS;i++)
	{


DBUG(("Deleting 3DSound at 0x%x\n", sounds[i].bee_Sound3DPtr));

		Delete3DSound(&(sounds[i].bee_Sound3DPtr));
	}

DBUG(("Deleting beespace 0x%x\n", beespace));

	DeleteSoundSpace(beespace);
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

/*******************************************************************/
float32 Choose ( float32 range )
{
        float32 val, r;

        r = (float32)(rand() & 0x0000FFFF);
        val = (r / 65536.0) * range;
        return val;
}

#define wChoose(min, max) (Choose( max - min ) + min)

/****************************************************************/
PolarPosition4D AddPolar(PolarPosition4D p1,
  PolarPosition4D p2, float32 orientation)
{
	PolarPosition4D sumPos;
	CartesianCoords c1, c2, sumPosXYZ;

	PolarToXYZ( &p1, &c1 );
	PolarToXYZ( &p2, &c2 );

	sumPosXYZ.xyz_X = c1.xyz_X + c2.xyz_X;
	sumPosXYZ.xyz_Y = c1.xyz_Y + c2.xyz_Y;

	XYZToPolar( &sumPosXYZ, &sumPos );
	sumPos.pp4d_Theta += orientation;

	return sumPos;
}

/****************************************************************/
PolarPosition4D SubtractPolar(PolarPosition4D p1,
  PolarPosition4D p2, float32 orientation)
{
	PolarPosition4D sumPos;

	p2.pp4d_Radius = -(p2.pp4d_Radius);
	sumPos = AddPolar(p1, p2, orientation);

	return sumPos;
}

/****************************************************************/
Err beeAnimate(beeSound sounds[], Sound3DSpace* beeSpace)
{
	Err Result;

DBUG(("Starting everything up\n"));

	/* Preset the global reverb mixer channel gains */
	Result = SetKnobPart(beeSpace->bees_GainKnob, NUMBER_OF_SOUNDS,
	  beeGlobalReverb);
	CHECKRESULT(Result, "SetKnobPart");

	/* Start the output instrument */
	Result = StartInstrument(beeSpace->bees_LineOut, 0);
	CHECKRESULT(Result, "StartInstrument");
	Result = StartInstrument(beeSpace->bees_OutputInst, 0);
	CHECKRESULT(Result, "StartInstrument");

	/* Go into the movement refresh loop */
	Result = beeSteer( sounds, beeSpace );

cleanup:

DBUG(("Stopping everything\n"));

	StopInstrument(beeSpace->bees_OutputInst, 0);
	StopInstrument(beeSpace->bees_LineOut, 0);

	return(Result);
}

/***********************************************************************/
Err beeUpdateHost( Sound3D* bee3D, Sound3DSpace* beeSpace, int32 soundNum )
{
	Err Result;

	float32 Amplitude, Dry, Wet;
	Sound3DParms s3dParms;

/* Do all the host updates that the API doesn't do */

	Result = Get3DSoundParms( bee3D, &s3dParms, sizeof(s3dParms) );
	CHECKRESULT(Result, "Get3DSoundParms");

/*
   Reverb calculations (from Dodge and Jerse):
   Dry signal attenuates proportionally to distance (1/D).  Wet signal
   attenuates proportionally to the square root of distance (1/sqrt(D)).
   We don't differentiate between local and global reverberation here.
*/

	Amplitude = s3dParms.s3dp_DistanceFactor;
	Dry = (1.0 - beeGlobalReverb) * Amplitude;
	Wet = sqrtf(Amplitude);

	Result = SetKnobPart(beeSpace->bees_PresendKnob, soundNum, Wet);
	CHECKRESULT(Result, "SetKnobPart");

	Result = SetKnobPart(beeSpace->bees_GainKnob, soundNum, Dry);
	CHECKRESULT(Result, "SetKnobPart");

cleanup:
	return (Result);
}

/***********************************************************************/
Err beeSteer(beeSound sounds[], Sound3DSpace* beeSpace)
{
	Err Result = 0;
	int32 doit=TRUE, i;
	ControlPadEventData cped;
	uint32 joy;
	PolarPosition4D oldPos, targetPos, relativeTarget, incrPos;
	Item sleepCue;
	int32 framespertick;
	uint16 timeNow;
	float32 newRadius, newTheta;
	PolarPosition4D observer_Position;
	float32 observer_Orientation;

	/* Allocate cue for waiting */
	sleepCue = CreateCue( NULL );
	CHECKRESULT(sleepCue, "CreateCue");

	framespertick = GetAudioClockDuration( AF_GLOBAL_CLOCK );

	/* Set the observer to absolute position 0,0 facing north */
	observer_Position.pp4d_Radius = 0.0;
	observer_Position.pp4d_Theta = 0.0;
	observer_Orientation = 0.0;

	for (i=0;i<NUMBER_OF_BEES;i++)
	{
		sounds[i].bee_Target.pp4d_Radius = wChoose(S3D_DISTANCE_TO_EAR, BEE_MAX_RADIUS);
		sounds[i].bee_Target.pp4d_Theta = s3dNormalizeAngle( Choose(M_PI * 2.0) );
		sounds[i].bee_Target.pp4d_Time = GetAudioFrameCount();

		targetPos = sounds[i].bee_Target;
		Start3DSound( sounds[i].bee_Sound3DPtr, &targetPos );
		beeUpdateHost(sounds[i].bee_Sound3DPtr, beeSpace, i);
		StartInstrument( sounds[i].bee_Source, 0 );
	}

	sounds[NUMBER_OF_BEES].bee_Target.pp4d_Radius = 5000;
	sounds[NUMBER_OF_BEES].bee_Target.pp4d_Theta = M_PI / 4.0;

	targetPos = sounds[NUMBER_OF_BEES].bee_Target;
	Start3DSound( sounds[NUMBER_OF_BEES].bee_Sound3DPtr, &targetPos );
	beeUpdateHost(sounds[NUMBER_OF_BEES].bee_Sound3DPtr, beeSpace, NUMBER_OF_BEES);
	StartInstrument( sounds[NUMBER_OF_BEES].bee_Source, 0 );

	sounds[NUMBER_OF_BEES+1].bee_Target.pp4d_Radius = 5000;
	sounds[NUMBER_OF_BEES+1].bee_Target.pp4d_Theta = -M_PI / 3.0;

	targetPos = sounds[NUMBER_OF_BEES+1].bee_Target;
	Start3DSound( sounds[NUMBER_OF_BEES+1].bee_Sound3DPtr, &targetPos );
	beeUpdateHost(sounds[NUMBER_OF_BEES+1].bee_Sound3DPtr, beeSpace, NUMBER_OF_BEES+1);
	StartInstrument( sounds[NUMBER_OF_BEES+1].bee_Source, 0 );

	while (doit)
	{
		/* Poll the joypad */
		Result = GetControlPad (1, FALSE, &cped);
		if (Result < 0)
		{
			PrintError(0,"get control pad data in","ta_bees3d",Result);
		}

		joy = cped.cped_ButtonBits;

		if (joy & ControlX)
		{
			doit = FALSE;
		}

		if (joy & ControlLeft)
		{
			observer_Orientation = s3dNormalizeAngle(
			  observer_Orientation + OBSERVER_ANGLE_INCR);
			for (i=0;i<NUMBER_OF_SOUNDS;i++)
			{
				relativeTarget = SubtractPolar( sounds[i].bee_Target,
				  observer_Position, observer_Orientation );
				relativeTarget.pp4d_Time = GetAudioFrameCount();

DBUG(("Turning observer to %f (relative = %f, %f)\n", observer_Orientation, relativeTarget.pp4d_Radius, relativeTarget.pp4d_Theta));

				Result = Move3DSoundTo(sounds[i].bee_Sound3DPtr,
				  &relativeTarget);
				beeUpdateHost(sounds[i].bee_Sound3DPtr, beeSpace, i);
			}
		}

		if (joy & ControlRight)
		{
			observer_Orientation = s3dNormalizeAngle(
			  observer_Orientation - OBSERVER_ANGLE_INCR);
			for (i=0;i<NUMBER_OF_SOUNDS;i++)
			{
				relativeTarget = SubtractPolar( sounds[i].bee_Target,
				  observer_Position, observer_Orientation );
				relativeTarget.pp4d_Time = GetAudioFrameCount();

DBUG(("Turning observer to %f (relative = %f, %f)\n", observer_Orientation, relativeTarget.pp4d_Radius, relativeTarget.pp4d_Theta));

				Result = Move3DSoundTo(sounds[i].bee_Sound3DPtr,
				  &relativeTarget);
				beeUpdateHost(sounds[i].bee_Sound3DPtr, beeSpace, i);
			}
		}

		if (joy & ControlUp)
		{
			incrPos.pp4d_Radius = OBSERVER_RADIUS_INCR;
			incrPos.pp4d_Theta = observer_Orientation;
			observer_Position = AddPolar( incrPos,
			  observer_Position, 0.0);

DBUG(("Moving observer to %f, %f\n", observer_Position.pp4d_Radius,
  observer_Position.pp4d_Theta));

			for (i=0;i<NUMBER_OF_SOUNDS;i++)
			{
				relativeTarget = SubtractPolar( sounds[i].bee_Target,
				  observer_Position, observer_Orientation );
				relativeTarget.pp4d_Time = GetAudioFrameCount();

				Result = Move3DSoundTo(sounds[i].bee_Sound3DPtr,
				  &relativeTarget);
				beeUpdateHost(sounds[i].bee_Sound3DPtr, beeSpace, i);
			}
		}

		if (joy & ControlDown)
		{
			incrPos.pp4d_Radius = -OBSERVER_RADIUS_INCR;
			incrPos.pp4d_Theta = observer_Orientation;
			observer_Position = AddPolar( incrPos,
			  observer_Position, 0.0);

DBUG(("Moving observer back to %f, %f\n", observer_Position.pp4d_Radius,
  observer_Position.pp4d_Theta));

			for (i=0;i<NUMBER_OF_SOUNDS;i++)
			{
				relativeTarget = SubtractPolar( sounds[i].bee_Target,
				  observer_Position, observer_Orientation );
				relativeTarget.pp4d_Time = GetAudioFrameCount();

				Result = Move3DSoundTo(sounds[i].bee_Sound3DPtr,
				  &relativeTarget);
				beeUpdateHost(sounds[i].bee_Sound3DPtr, beeSpace, i);
			}
		}

		if (joy & ControlA)
		{
			observer_Position.pp4d_Radius = 0.0;
			observer_Position.pp4d_Theta = 0.0;
			observer_Orientation = 0.0;

DBUG(("Moving observer back to 0,0,0\n"));

			for (i=0;i<NUMBER_OF_SOUNDS;i++)
			{
				relativeTarget = SubtractPolar( sounds[i].bee_Target,
				  observer_Position, observer_Orientation );
				relativeTarget.pp4d_Time = GetAudioFrameCount();

				Result = Move3DSoundTo(sounds[i].bee_Sound3DPtr,
				  &relativeTarget);
				beeUpdateHost(sounds[i].bee_Sound3DPtr, beeSpace, i);
			}
		}
		/* change a bee's direction stochastically */
		if (Choose(1.0) >= BEE_CHANGE_DIRECTION_THRESHOLD)
		{
			i = (int32)Choose(NUMBER_OF_BEES);	/* pick a bee */
			timeNow = GetAudioFrameCount();
			oldPos = sounds[i].bee_Target;

			newRadius = oldPos.pp4d_Radius
			  + wChoose(-BEE_MAX_DELTA_RADIUS, BEE_MAX_DELTA_RADIUS);
			newTheta = oldPos.pp4d_Theta
			  + wChoose(-BEE_MAX_DELTA_THETA, BEE_MAX_DELTA_THETA);
			if (newRadius > BEE_MAX_RADIUS)
			  newRadius = fmodf(newRadius, BEE_MAX_RADIUS);
			targetPos.pp4d_Radius = newRadius;
			targetPos.pp4d_Theta = newTheta;
			targetPos.pp4d_Time =
			  (uint16)((timeNow + (uint16)(TIME_INCR * framespertick)) & 0xFFFF);

DBUG2(("Moving %i to: %f, %f\n", i, targetPos.pp4d_Radius, targetPos.pp4d_Theta));

			relativeTarget = SubtractPolar( targetPos, observer_Position,
			  observer_Orientation );
			Result = Move3DSoundTo(sounds[i].bee_Sound3DPtr,
			  &relativeTarget);
			beeUpdateHost(sounds[i].bee_Sound3DPtr, beeSpace, i);

			sounds[i].bee_Target = targetPos;
		}

		SleepUntilTime( sleepCue, GetAudioTime() + TIME_INCR );
	}

cleanup:
	for (i=0;i<NUMBER_OF_SOUNDS;i++)
	{
		StopInstrument( sounds[i].bee_Source, 0);
		Stop3DSound( sounds[i].bee_Sound3DPtr );
	}

	DeleteCue( sleepCue );

	return Result;
}
