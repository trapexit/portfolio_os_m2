/***************************************************************
**
** @(#) ta_leslie.c 96/08/27 1.9
**
** Test sound3d library:
** Test directional sound by emulating a "leslie", a device with
** two speakers mounted on a horizontal shaft facing opposite
** directions.  The shaft is mounted on a vertical axis, and
** rotated at variable rates.  I must be getting old, I remember
** this sound pretty well...
**
** Author: rnm
**
** Copyright (c) 1995, 3DO Company.
** This program is proprietary and confidential.
**
***************************************************************/

/**
|||	AUTODOC -public -class examples -group Audio -name ta_leslie
|||	Demonstrates directional sound with the 3DSound API.
|||
|||	  Format
|||
|||	    ta_leslie [backamp [cuelist]]
|||
|||	  Description
|||
|||	    This example badly simulates a leslie, an electroacoustic sound
|||	    processor commonly used with electric organs.  The simulation
|||	    routes a source sound (an organ sample) through two 3D Sound
|||	    objects positioned back to back some distance apart from each
|||	    other in front of the observer and rotating about their mutual
|||	    center.
|||
|||	    The speed of rotation can be controlled with the control pad.
|||
|||	    Up
|||	        fast rotation
|||	    Down
|||	        slow rotation
|||
|||	    Note: to use this example, you first need to install the General MIDI
|||	    Sample Library in /remote/Samples, and then run the shell script
|||	    "makepatch.script" in the directory Examples/Audio/Sound3D to build the
|||	    set of patches loaded by the program.
|||
|||	  Arguments
|||
|||	    backamp
|||	        A floating point number between 0.0 and 1.0, supplied as the
|||	        tag argument TAG_S3D_BACK_AMPLITUDE to the function
|||	        Create3DSound().
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
|||	    8
|||	        Smooth amplitude changes
|||
|||	        The last option, "smooth amplitude changes", makes a marked
|||	        improvement in the sound.
|||
|||	  Associated Files
|||
|||	    patches/ta_leslie.mp, makepatch.script
|||
|||	  Location
|||
|||	    Examples/Audio/Sound3D
|||
**/

#include <kernel/types.h>
#include <kernel/debug.h>
#include <kernel/operror.h>
#include <audio/audio.h>
#include <audio/music.h>
#include <misc/event.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */
#define FRAMERATE (44100.0)
#define REFRESHRATE (80.0)
#define CUE_MASK (S3D_F_OUTPUT_HEADPHONES \
  | S3D_F_PAN_DELAY \
  | S3D_F_DOPPLER \
  | S3D_F_DISTANCE_AMPLITUDE_SQUARE)

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		ERR(("Failure in %s: $%x\n", name, val)); \
		goto cleanup; \
	}

/* Structure to keep track of the source sound items */
typedef struct Sound3DSource
{
	Item s3ds_Template;
	Item s3ds_Instrument;
	Item s3ds_FreqKnob;
	Item s3ds_NominalFrequency;
	Item s3ds_EnvKnob;

} Sound3DSource;

typedef struct CartesianCoords
{
	float32 xyz_X;
	float32 xyz_Y;
	float32 xyz_Z;
	float32 xyz_Time;
} CartesianCoords;

/* function declarations */
Err TestS3DInit(Sound3DSource* SourceSoundPtr, Item* OutputInst,
  Sound3D** s3dContextHandle, float32 s3dBackAmplitude, uint32 s3dCues);
uint32 TestS3DSetCues( char* cueList );
Err TestS3DAnimate(Sound3DSource* SourceSoundPtr, Item* OutputInst,
  Sound3D** s3dContextPtr);
void TestS3DTerm(Sound3DSource* SourceSoundPtr, Item* OutputInst,
  Sound3D** s3dContextPtr);

int main( int32 argc, char *argv[])
{

	int32 Result, i;
	Sound3D* s3dContextPtr[2];
	Sound3DSource SourceSound[2];
	Item OutputInstrument[2];
	uint32 s3dCues;
	float32 s3dBackAmplitude;

	for (i=0;i<2;i++)
	{
		SourceSound[i].s3ds_Instrument = 0;
		SourceSound[i].s3ds_Template = 0;
		s3dContextPtr[i] = NULL;
	}

	PRT(("Usage: ta_leslie [backamp [cuelist]]\n\n"));
	PRT(("backamp = back amplitude factor for unidirectional speakers.\n"));
	PRT(("cuelist = abc... where a, b, c ... are digits representing bit\n"));
	PRT(("  positions for corresponding flags.\n\n"));
	PRT(("Controls: Use Up for fast, Down for slow.\n"));

	s3dBackAmplitude = (argc > 1) ? strtof( argv[1], NULL ) : 0.0 ;
	s3dCues = (argc > 2) ? TestS3DSetCues( argv[2]) : CUE_MASK ;

	if ((Result = InitEventUtility (1, 0, TRUE)) < 0)
	{
		PrintError (NULL, "init event utility", NULL, Result);
		goto cleanup;
	}

	Result = TestS3DInit(SourceSound, OutputInstrument, s3dContextPtr,
	  s3dBackAmplitude, s3dCues);
	CHECKRESULT(Result, "TestS3DInit");

DBUG(("Successfully initialized ta_leslie\n"));

	Result = TestS3DAnimate(SourceSound, OutputInstrument, s3dContextPtr);
	CHECKRESULT(Result, "TestS3DAnimate");

cleanup:
DBUG(("Cleaning up\n"));
	TestS3DTerm(SourceSound, OutputInstrument, s3dContextPtr);
	KillEventUtility();
	printf("ta_leslie done\n");

	return((int) Result);

}

/****************************************************************/
uint32 TestS3DSetCues( char* cueList )
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
Err TestS3DInit(Sound3DSource* SourceSoundPtr, Item* OutputInst,
  Sound3D** s3dContextHandle, float32 s3dBackAmplitude, uint32 s3dCues)
{
	Err Result, i;
	Item s3dInst;
	float32 NominalFreq;
	TagArg Tags[] = { { S3D_TAG_FLAGS }, { S3D_TAG_BACK_AMPLITUDE }, TAG_END };

	/* Initialize audio, return if error. */
	if ((Result = OpenAudioFolio()) < 0)
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(Result);
	}

	for (i=0;i<2;i++)
	{
		/* Allocate and set up the 3D context */
		Tags[0].ta_Arg = (TagData) s3dCues;
		Tags[1].ta_Arg = ConvertFP_TagData( s3dBackAmplitude );

		Result = Create3DSound( &(s3dContextHandle[i]), Tags );
		CHECKRESULT(Result, "Create3DSound");

DBUG(("Here it is: 0x%x\n", *s3dContextHandle[i]));

		/* Set up the source instrument */
		SourceSoundPtr[i].s3ds_Template = LoadScoreTemplate("ta_leslie.patch");
		CHECKRESULT(SourceSoundPtr[i].s3ds_Template, "LoadScoreTemplate");

		SourceSoundPtr[i].s3ds_Instrument = CreateInstrument(SourceSoundPtr->s3ds_Template, 0);
		CHECKRESULT(SourceSoundPtr[i].s3ds_Instrument, "CreateInstrument");

DBUG(("Made source instrument, item %i\n", SourceSoundPtr->s3ds_Instrument));

		/* Connect the source to the 3D instrument context*/
		s3dInst = Get3DSoundInstrument( s3dContextHandle[i] );
		CHECKRESULT(s3dInst, "Get3DSoundInstrument");

		Result = ConnectInstruments(SourceSoundPtr[i].s3ds_Instrument, "Output", s3dInst,
		  "Input");
		CHECKRESULT(Result, "ConnectInstrument");

		/* Connect the Frequency knob, for doppler */
		SourceSoundPtr[i].s3ds_FreqKnob = CreateKnob(SourceSoundPtr[i].s3ds_Instrument,
		  "SampleRate", NULL);
		CHECKRESULT(SourceSoundPtr[i].s3ds_FreqKnob,
		  "CreateKnob: can't connect to doppler");

		/* Read the default knob value, assume that's nominal */
		Result = ReadKnob(SourceSoundPtr[i].s3ds_FreqKnob, &NominalFreq);
		CHECKRESULT(Result, "ReadKnob");
		SourceSoundPtr[i].s3ds_NominalFrequency = NominalFreq;

		/* Connect the EnvRate knob, for doppler ramp */
		SourceSoundPtr[i].s3ds_EnvKnob = CreateKnob(SourceSoundPtr[i].s3ds_Instrument,
		  "EnvRate", NULL);
		CHECKRESULT(SourceSoundPtr[i].s3ds_EnvKnob,
		  "CreateKnob: can't connect to envelope");

		/* Load the output instrument */
		OutputInst[i] = LoadInstrument("line_out.dsp", 0, 100);
		CHECKRESULT(OutputInst[i], "CreateInstrument");

		/* Connect the 3D processors to the output instrument */
		Result = ConnectInstrumentParts( s3dInst, "Output", 0, OutputInst[i],
		  "Input", 0);
		CHECKRESULT(Result, "ConnectInstrumentParts");
		Result = ConnectInstrumentParts( s3dInst, "Output", 1, OutputInst[i],
		  "Input", 1);
		CHECKRESULT(Result, "ConnectInstrumentParts");
	}

cleanup:
	return(Result);

}

/****************************************************************/
void TestS3DTerm(Sound3DSource* SourceSoundPtr, Item* OutputInst, Sound3D* s3dContext[])
{
	int32 i;

	for (i=0;i<2;i++)
	{
		UnloadScoreTemplate(SourceSoundPtr[i].s3ds_Template);
		Delete3DSound(&(s3dContext[i]));
		UnloadInstrument(OutputInst[i]);
	}
	CloseAudioFolio();
	return;
}

/***********************************************************************/
Err TestS3DUpdateHost( Sound3D* s3dContext, Sound3DSource* source )
{
	Err Result;

	float32 Doppler;
	Sound3DParms s3dParms;

/* Do all the host updates that the API doesn't do */

	Result = Get3DSoundParms( s3dContext, &s3dParms, sizeof(s3dParms) );
	CHECKRESULT(Result, "Get3DSoundParms");

/* Doppler */

	Doppler = s3dParms.s3dp_Doppler;
	Result = SetKnob(source->s3ds_FreqKnob, Doppler * source->s3ds_NominalFrequency);
	CHECKRESULT(Result, "SetKnob Doppler");

cleanup:
	return (Result);
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
Err TestS3DAnimate(Sound3DSource* ssPtr, Item* OutputInst, Sound3D* s3dContext[])
{
	Err Result;
	float32 tps;	/* ticks per second of audio clock */
	AudioTime TimeNow;
	uint16 FrameNow;
	Item SleepCue;
	PolarPosition4D soundPos = { 100.0, 0.0, 0.0, 0, M_PI, 0.0 };
	  /* sound is 100 units in front of observer, facing observer */
	PolarPosition4D otherPos = { 200.0, 0.0, 0.0, 0, 0.0, 0.0 };
	  /* sound is 200 units in front of observer, facing away from observer */
	CartesianCoords soundCartesian, otherCartesian;

	float32 DopplerRate, rotationAngle=M_PI, RotationRate=1.0, dAngle;
	ControlPadEventData cped;
	uint32 joy;
	int32 doit=TRUE;

DBUG(("Starting everything up\n"));

	/* Set up the timer */
	SleepCue = CreateCue( NULL );
	CHECKRESULT(SleepCue, "CreateCue");
	Result = GetAudioClockRate(AF_GLOBAL_CLOCK, &tps);
	CHECKRESULT(Result, "GetAudioClockRate");

	/* Start the output instruments */
	Result = StartInstrument(OutputInst[0], 0);
	CHECKRESULT(Result, "StartInstrument");

	Result = StartInstrument(OutputInst[1], 0);
	CHECKRESULT(Result, "StartInstrument");

	/* Start the 3D processing */
	Result = Start3DSound(s3dContext[0], &soundPos);
	CHECKRESULT(Result, "Start3DSound");

	Result = Start3DSound(s3dContext[1], &soundPos);
	CHECKRESULT(Result, "Start3DSound");

	/* Start the source instruments */
	Result = StartInstrument(ssPtr[0].s3ds_Instrument, 0);
	CHECKRESULT(Result, "StartInstrument");

	Result = StartInstrument(ssPtr[1].s3ds_Instrument, 0);
	CHECKRESULT(Result, "StartInstrument");

	DopplerRate = 1.0;  /* not using this, for now */

DBUG(("Doppler Rate: %f\n", DopplerRate));

	Result = SetKnob( ssPtr[0].s3ds_EnvKnob, DopplerRate );
	CHECKRESULT(Result, "SetKnob");

	Result = SetKnob( ssPtr[1].s3ds_EnvKnob, DopplerRate );
	CHECKRESULT(Result, "SetKnob");

	/* Get the current audio time */
	soundPos.pp4d_Time = (uint16)(GetAudioFrameCount() & 0xFFFF);

DBUG(("Rotating sources.\n"));

	while (doit)
	{
		Result = GetControlPad (1, FALSE, &cped);
		if (Result < 0)
		{
			PrintError(0,"get control pad data in","ta_leslie3d",Result);
		}

		joy = cped.cped_ButtonBits;

/* Stop */
		if (joy & ControlX)
		{
			doit = FALSE;
		}

/* Up, Down */
		if (joy & ControlUp)
		{
			RotationRate = 5.0;	/* 5 Hz */
			PRT(("Fast leslie\n"));
		}
		if (joy & ControlDown)
		{
			RotationRate = 1.0;	/* 1 Hz */
			PRT(("Slow leslie\n"));
		}

/* Rotate the sounds once around their axes at the rotation rate */
		TimeNow = GetAudioTime();
		FrameNow = GetAudioFrameCount();

		dAngle = (M_PI * 2.0 / REFRESHRATE) * RotationRate;

		soundPos.pp4d_or_Theta = s3dNormalizeAngle(
		  soundPos.pp4d_or_Theta + dAngle );
		otherPos.pp4d_or_Theta = s3dNormalizeAngle(
		  otherPos.pp4d_or_Theta + dAngle );

		rotationAngle = s3dNormalizeAngle(rotationAngle + dAngle);
		soundCartesian.xyz_X = 50 * sinf(rotationAngle);
		soundCartesian.xyz_Y = 150 + 50 * cosf(rotationAngle);
		otherCartesian.xyz_X = -(50 * sinf(rotationAngle));
		otherCartesian.xyz_Y = 150 - 50 * cosf(rotationAngle);
		XYZToPolar(&soundCartesian, &soundPos);
		XYZToPolar(&otherCartesian, &otherPos);

DBUG(("sp1: (%f) pointing at (%f)\nsp2: (%f) pointing at (%f)\n",
soundPos.pp4d_Theta, soundPos.pp4d_or_Theta,
otherPos.pp4d_Theta, otherPos.pp4d_or_Theta));

		otherPos.pp4d_Time = soundPos.pp4d_Time =
		  (uint16)((FrameNow + (uint16)(FRAMERATE/REFRESHRATE)) & 0xFFFF);

		Result = Move3DSoundTo( s3dContext[0], &soundPos );
		CHECKRESULT(Result, "Move3DSoundTo");
		TestS3DUpdateHost( s3dContext[0], &(ssPtr[0]) );

		Result = Move3DSoundTo( s3dContext[1], &otherPos );
		CHECKRESULT(Result, "Move3DSoundTo");
		TestS3DUpdateHost( s3dContext[1], &(ssPtr[1]) );

		SleepUntilTime( SleepCue, TimeNow + tps/REFRESHRATE );
	}

cleanup:

DBUG(("Stopping everything\n"));

	StopInstrument(OutputInst[0], 0);
	StopInstrument(OutputInst[1], 0);
	Stop3DSound(s3dContext[0]);
	Stop3DSound(s3dContext[1]);
	StopInstrument(ssPtr[0].s3ds_Instrument, 0);
	StopInstrument(ssPtr[1].s3ds_Instrument, 0);
	DeleteCue( SleepCue );

	return(Result);
}
