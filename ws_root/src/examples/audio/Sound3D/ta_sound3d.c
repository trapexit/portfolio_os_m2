/***************************************************************
**
** @(#) ta_sound3d.c 96/08/22 1.10
**
** Test sound3d library.
**
** Author: rnm
**
** Copyright (c) 1995, 3DO Company.
** This program is proprietary and confidential.
**
** Modified to test directionality, use new API
***************************************************************/

/**
|||	AUTODOC -public -class examples -group Audio -name ta_sound3d
|||	Simple directional sound using 3DSound API.
|||
|||	  Format
|||
|||	    ta_sound3d [backamp [cuelist]]
|||
|||	  Description
|||
|||	    This example rotates a sound through a complete circle over a
|||	    five-second time span.  It is intended to show the effect of
|||	    the tag TAG_S3D_BACK_AMPLITUDE and the flag S3D_F_SMOOTH_AMPLITUDE
|||	    on a 3DSound object.
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
|||	        cuelist is equivalent to the string 14.
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
|||	  Associated Files
|||
|||	    patches/test3dsound.mp, makepatch.script
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */
#define FRAMERATE (44100.0)
#define CUE_MASK (S3D_F_OUTPUT_HEADPHONES \
  | S3D_F_PAN_DELAY \
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
} Sound3DSource;

/* function declarations */
Err TestS3DInit(Sound3DSource* SourceSoundPtr, Item *OutputInst,
  Sound3D** s3dContextHandle, float32 s3dBackAmplitude, uint32 s3dCues);
uint32 TestS3DSetCues( char* cueList );
Err TestS3DAnimate(Sound3DSource* SourceSoundPtr, Item OutputInst,
  Sound3D* s3dContextPtr);
void TestS3DTerm(Sound3DSource* SourceSoundPtr, Item OutputInst,
  Sound3D** s3dContextHandle);

int main( int32 argc, char *argv[])
{

	int32 Result;
	Sound3D* s3dContextPtr;
	Sound3DSource SourceSound;
	Item OutputInstrument;
	uint32 s3dCues;
	float32 s3dBackAmplitude;

	SourceSound.s3ds_Instrument = 0;
	SourceSound.s3ds_Template = 0;
	s3dContextPtr = NULL;

	s3dBackAmplitude = (argc > 1) ? strtof( argv[1], NULL ) : 0.0 ;
	s3dCues = (argc > 2) ? TestS3DSetCues( argv[2]) : CUE_MASK ;

	Result = TestS3DInit(&SourceSound, &OutputInstrument, &s3dContextPtr,
	  s3dBackAmplitude, s3dCues);
	CHECKRESULT(Result, "TestS3DInit");

DBUG(("Successfully initialized ta_sound3d\n"));

	Result = TestS3DAnimate(&SourceSound, OutputInstrument, s3dContextPtr);
	CHECKRESULT(Result, "TestS3DAnimate");

cleanup:
DBUG(("Cleaning up\n"));
	TestS3DTerm(&SourceSound, OutputInstrument, &s3dContextPtr);
	printf("ta_sound3d done\n");

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
	Err Result;
	Item s3dInst;
	float32 NominalFreq;
	TagArg Tags[] = { { S3D_TAG_FLAGS }, { S3D_TAG_BACK_AMPLITUDE }, TAG_END };

	/* Initialize audio, return if error. */
	if ((Result = OpenAudioFolio()) < 0)
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(Result);
	}

	/* Allocate and set up the 3D context */
	Tags[0].ta_Arg = (TagData) s3dCues;
	Tags[1].ta_Arg = ConvertFP_TagData( s3dBackAmplitude );

	Result = Create3DSound( s3dContextHandle, Tags );
	CHECKRESULT(Result, "Create3DSound");

DBUG(("Here it is: 0x%x\n", *s3dContextHandle));

	/* Set up the source instrument */
	SourceSoundPtr->s3ds_Template = LoadScoreTemplate("Test3DSound.patch");
	CHECKRESULT(SourceSoundPtr->s3ds_Template, "LoadScoreTemplate");

	SourceSoundPtr->s3ds_Instrument = CreateInstrument(SourceSoundPtr->s3ds_Template, 0);
	CHECKRESULT(SourceSoundPtr->s3ds_Instrument, "CreateInstrument");

DBUG(("Made source instrument, item %i\n", SourceSoundPtr->s3ds_Instrument));

	/* Connect the source to the 3D instrument context*/
	s3dInst = Get3DSoundInstrument( *s3dContextHandle );
	CHECKRESULT(s3dInst, "Get3DSoundInstrument");

	Result = ConnectInstruments(SourceSoundPtr->s3ds_Instrument, "Output", s3dInst,
	  "Input");
	CHECKRESULT(Result, "ConnectInstrument");

	/* Load the output instrument */
	*OutputInst = LoadInstrument("line_out.dsp", 0, 100);
	CHECKRESULT(*OutputInst, "LoadInstrument");

	/* Connect the 3D processor to the output instrument */
	Result = ConnectInstrumentParts( s3dInst, "Output", 0, *OutputInst,
	  "Input", 0);
	CHECKRESULT(Result, "ConnectInstrumentParts");
	Result = ConnectInstrumentParts( s3dInst, "Output", 1, *OutputInst,
	  "Input", 1);
	CHECKRESULT(Result, "ConnectInstrumentParts");

	/* Connect the Frequency knob, for doppler */
	SourceSoundPtr->s3ds_FreqKnob = CreateKnob(SourceSoundPtr->s3ds_Instrument,
	  "SampleRate", NULL);
	CHECKRESULT(SourceSoundPtr->s3ds_FreqKnob,
	  "CreateKnob: can't connect to doppler");

	/* Read the default knob value, assume that's nominal */
	Result = ReadKnob(SourceSoundPtr->s3ds_FreqKnob, &NominalFreq);
	CHECKRESULT(Result, "ReadKnob");
	SourceSoundPtr->s3ds_NominalFrequency = NominalFreq;

cleanup:
	return(Result);

}

/****************************************************************/
void TestS3DTerm(Sound3DSource* SourceSoundPtr, Item OutputInst,
  Sound3D** s3dContextHandle)
{
	UnloadInstrument(OutputInst);
	UnloadScoreTemplate(SourceSoundPtr->s3ds_Template);
	Delete3DSound(s3dContextHandle);
	CloseAudioFolio();
	return;
}

/****************************************************************/
Err TestS3DAnimate(Sound3DSource* ssPtr, Item OutputInst, Sound3D* s3dContext)
{
	Err Result;
	float32 tps;	/* ticks per second of audio clock */
	AudioTime StartTime, TimeNow;
	Item SleepCue;
	PolarPosition4D soundPos = { 100.0, 0.0, 0.0, 0, M_PI, 0.0 }
	  /* sound is 100 units in front of observer, facing observer */

DBUG(("Starting everything up\n"));

	/* Set up the timer */
	SleepCue = CreateCue( NULL );
	CHECKRESULT(SleepCue, "CreateCue");
	Result = GetAudioClockRate(AF_GLOBAL_CLOCK, &tps);
	CHECKRESULT(Result, "GetAudioClockRate");

	/* Start the output instrument */
	Result = StartInstrument(OutputInst, 0);
	CHECKRESULT(Result, "StartInstrument");

	/* Start the 3D processing */
	Result = Start3DSound(s3dContext, &soundPos);
	CHECKRESULT(Result, "Start3DSound");

	/* Start the source instrument */
	Result = StartInstrument(ssPtr->s3ds_Instrument, 0);
	CHECKRESULT(Result, "StartInstrument");

	/* Get the current audio time */
	StartTime = TimeNow = GetAudioTime();
	soundPos.pp4d_Time = (uint16)(GetAudioFrameCount() & 0xFFFF)

DBUG(("Rotating source.\n"));

	/* Rotate the sound once around its axis in 5 seconds */
	while( TimeNow < StartTime + 5*tps )
	{
		TimeNow = GetAudioTime();

		soundPos.pp4d_or_Theta = s3dNormalizeAngle(
		  soundPos.pp4d_or_Theta + M_PI * 2.0 / 50.0 );
		soundPos.pp4d_Time = (uint16)(((uint16)soundPos.pp4d_Time +
		  (uint16)(FRAMERATE/10)) & 0xFFFF);

		Result = Move3DSoundTo( s3dContext, &soundPos );

		SleepUntilTime( SleepCue, TimeNow + tps/10 );
	}

cleanup:

DBUG(("Stopping everything\n"));

	StopInstrument(OutputInst, 0);
	Stop3DSound(s3dContext);
	StopInstrument(ssPtr->s3ds_Instrument, 0);
	DeleteCue( SleepCue );

	return(Result);
}
