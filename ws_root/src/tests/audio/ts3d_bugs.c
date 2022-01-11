/***************************************************************
**
** @(#) ts3d_bugs.c 96/06/26 1.2
**
** Test sound3d library to make sure bugs corrected.
**
** Author: rnm
**
** Copyright (c) 1995, 3DO Company.
** This program is proprietary and confidential.
**
***************************************************************/

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
		PrintError (NULL, name, NULL, val); \
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
  Sound3D** s3dContextHandle, uint32 s3dCues);
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

	SourceSound.s3ds_Instrument = 0;
	SourceSound.s3ds_Template = 0;
	s3dContextPtr = NULL;

	s3dCues = (argc > 1) ? TestS3DSetCues( argv[1]) : CUE_MASK ;

	Result = TestS3DInit(&SourceSound, &OutputInstrument, &s3dContextPtr,
	  s3dCues);
	CHECKRESULT(Result, "TestS3DInit");

DBUG(("Successfully initialized %s\n", argv[0]));

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
  Sound3D** s3dContextHandle, uint32 s3dCues)
{
	Err Result;
	Item s3dInst;
	float32 NominalFreq;

	/* Initialize audio, return if error. */
	if ((Result = OpenAudioFolio()) < 0)
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(Result);
	}

	Result = Create3DSoundVA( s3dContextHandle, S3D_TAG_FLAGS, s3dCues, TAG_END );
	CHECKRESULT(Result, "Create3DSoundVA");

DBUG(("Here it is: 0x%x\n", *s3dContextHandle));

	/* Set up the source instrument */
	SourceSoundPtr->s3ds_Template = LoadScoreTemplate("Patches/Test3DSound.patch");
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
	AudioTime TimeNow;
	Item SleepCue;
	PolarPosition4D soundPosFrom = { 0.0, 0.0, 0.0, 0, 0.0, 0.0 };
	PolarPosition4D soundPosTo   = { 100.0, 0.0, 0.0, 0, 0.0, 0.0 };
	PolarPosition4D whereami;
	float32 f;
	
	  /* sound is 100 units in front of observer */

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
	Result = Start3DSound(s3dContext, &soundPosTo);
	CHECKRESULT(Result, "Start3DSound");

	/* Start the source instrument */
	Result = StartInstrument(ssPtr->s3ds_Instrument, 0);
	CHECKRESULT(Result, "StartInstrument");

DBUG(("Rotating source.\n"));

	/* Rotate the sound once around user in 30 degree increments, 0 dT */
	for (f = 0.0;f < M_PI * 2.0; f += M_PI / 6.0 )
	{
		TimeNow = GetAudioTime();

		soundPosTo.pp4d_Theta = s3dNormalizeAngle( f );
		
		/* Make dT = 0 */
		soundPosTo.pp4d_Time = (uint16)(GetAudioFrameCount() & 0xFFFF);
		soundPosFrom.pp4d_Time = soundPosTo.pp4d_Time;

		Result = Move3DSound( s3dContext, &soundPosFrom, &soundPosTo );
		CHECKRESULT( Result, "Move3DSound" );
		
		PRT(("Angle is %f\n", f * 360.0 / (M_PI * 2.0)));
		
		SleepUntilTime( SleepCue, TimeNow + tps * 2.0 );

		Result = Get3DSoundPos( s3dContext, &whereami );
		CHECKRESULT( Result, "Get3DSoundPos" );
		
		PRT(("Reported angle is %f\n", whereami.pp4d_Theta * 360.0 / (M_PI * 2.0)));
		PRT(("Reported radius is %f\n", whereami.pp4d_Radius));
	}

cleanup:

DBUG(("Stopping everything\n"));

	StopInstrument(OutputInst, 0);
	Stop3DSound(s3dContext);
	StopInstrument(ssPtr->s3ds_Instrument, 0);
	DeleteCue( SleepCue );

	return(Result);
}
