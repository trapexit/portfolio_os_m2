/*
 *		SoundFX - sfx.c - Sound effect replay routines (looped and triggered)
 *
 *			VERSION BY:		Mark Rearick / The Zoo Crew
 *
 *			PROGRAM HISTORY:
 *
 *		001		07/19/96	Began work based off of music.c (Megademo)...
 *
 *
 *			THINGS TO DO:
 *
 *		.o.		Even more than before
 *
 */


#include "sounds.h"											/* This is our wonderful .h file */

#define MyMixerSFXCHANNELS			(8)
#define MyMixerSFXDEFINE			MakeMixerSpec(MyMixerSFXCHANNELS, 2, AF_F_MIXER_WITH_LINE_OUT)


/*
 * GLOBAL NIGHTMARES
 */
Item MyMixerSFXTemplate;
Item MyMixerSFX;
Item SamplerInsSFX[MyMixerSFXCHANNELS];
Item SampleSound[kSFX_Count];
Item AttachedSamplesSFX[kSFX_Count];

int32 SoundFXChannel[kSFX_Count] = {	0 };

/*
 * For sound effects, I'm going to use 1024 as the max width of the screen ( << 10) to eliminate a divide, and to cover both
 * 320x240 and 640x480 (if we use it).  There will be a #define for the center of my little world, which must be changed if
 * switching modes.  For the Z value I'm going to use 1024 ( << 10) to save on divides (you know me)...  Figure it out...
 */

/*
 *
 */

#define MAXOFFSCREENSOUND		(40)										/* Allow up to 40 pixels of offscreen sound */
#define MIDZDEPTHSOUND			(150)										/* Middle Z depth value for sound */
#define MAXOFFMIDZDEPTHSOUND	(100)										/* Maximum distance from Z value */

#define QUARTERSCREENCOORD		(128)										/* This is 1/4 of the screen width */
#define MIDDLESCREENCOORD		(256)										/* This is the difference between middles */
#define SOUNDSCREENWIDTH		(512)										/* This is what sound thinks the screen width is */

#define MIDDLEZCOORD			(256)										/* This is the difference between middles */
#define SOUNDSCREENDEPTH		(512)										/* This is what sound thinks the screen depth is */

/*
 *
 */
int32 Position3DSound(int32 channel, int32 x, int32 y, int32 z)			/* Positions the 3D sound */
{
	float32 localx = (float32)(((x + MIDDLESCREENCOORD) + (y + MIDDLESCREENCOORD)) >> 1);
	float32 localz = (float32)(z);
	float32 leftgain, rightgain;
	int32 truereturn;
	Item ItemTracker;


	if ((localx < SOUNDSCREENWIDTH) && (localx > 0) &&					/* Test the X bounds */
		(localz < MIDDLEZCOORD) && (localz > -(MIDDLEZCOORD)))			/* Test the Z bounds */
		{
		if (localz < 0)													/* Eliminate sign */
			localz = -localz;

		localz = localz/MIDDLEZCOORD;

		if (localx < QUARTERSCREENCOORD)								/* Far Left */
			{
			leftgain = localx/QUARTERSCREENCOORD;						/* This gives us panning based on location */
			rightgain = 0;
			}
		else
			if (localx < MIDDLESCREENCOORD)								/* Left */
				{
				leftgain = 1;											/* This gives us panning based on location */
				rightgain = (localx-QUARTERSCREENCOORD)/QUARTERSCREENCOORD;
				}
			else
				if (localx < (MIDDLESCREENCOORD + QUARTERSCREENCOORD))	/* Right */
					{
					leftgain = 1.0 - ((localx-MIDDLESCREENCOORD)/QUARTERSCREENCOORD);		/* This gives us panning based on location */
					rightgain = 1;
					}
				else													/* Far Right */
					{
					leftgain = 0;										/* This gives us panning based on location */
					rightgain = 1.0 - ((localx-MIDDLESCREENCOORD-QUARTERSCREENCOORD)/QUARTERSCREENCOORD);
					}

/*		printf("Z = %g	X = %g	LG = %g		RG = %g\n", localz, localx, leftgain, rightgain); */

		if (rightgain > localz)
			rightgain -= localz;
		else
			rightgain = 0;

		if (leftgain> localz)
			leftgain -= localz;
		else
			leftgain = 0;

/*		printf("Z = %g	LG = %g		RG = %g\n", localz, leftgain, rightgain); */

		truereturn = TRUE;
		}
	else
		{
		leftgain = 0;
		rightgain = 0;

		truereturn = FALSE;
		}

	ItemTracker = CreateKnob(MyMixerSFX, "Gain", NULL);
	SetKnobPart(ItemTracker, CalcMixerGainPart(MyMixerSFXDEFINE, channel, 0), leftgain);
	SetKnobPart(ItemTracker, CalcMixerGainPart(MyMixerSFXDEFINE, channel, 1), rightgain);
	DeleteKnob(ItemTracker);

	return truereturn;
}

/*
 *
 */
void LoopSoundFX(int32 noisename)										/* Play looping sound effect with pseudo-3D sound */
{
	Position3DSound(SoundFXChannel[noisename], 0, SOUNDSCREENWIDTH, MIDDLEZCOORD);		/* Can we actually hear this sound? */

	StartAttachment(AttachedSamplesSFX[noisename], NULL);
}

/*
 *
 */
void PlaySoundFX(int32 noisename, int32 x, int32 y, int32 z)					/* Play sound effect with pseudo-3D sound */
{
	if (Position3DSound(SoundFXChannel[noisename], x, y, z) == FALSE)			/* Can we actually hear this sound?*/
		return;

	StartAttachment(AttachedSamplesSFX[noisename], NULL)
;
	ReleaseAttachment(AttachedSamplesSFX[noisename], NULL);
}

/*
 * Initialize the sound effects routines
 */
int32 InitSoundFX(void)
{
	int32 x, y;
	Item ItemTracker;

	/*
	 * Create the mixer Template
	 */
	if ((MyMixerSFXTemplate = CreateMixerTemplate(MyMixerSFXDEFINE, NULL)) < 0)
		printf("CreateMixerTemplate Failed.\n");

	if ((MyMixerSFX = CreateInstrumentVA(MyMixerSFXTemplate, AF_TAG_PRIORITY, PRIO_SFX, TAG_END)) < 0)						/* Creates the actual mixer */
		printf("CreateInstrument Mixer Failed.\n");

	/*
	 * Load all the samples to memory
	 */
	if ((SampleSound[kSFX_Hit] = LoadSample(kSFX_Hit_Filename)) < 0)
		printf("Error loading sound\n");
    if ((SampleSound[kSFX_Block] = LoadSample(kSFX_Block_Filename)) < 0)
		printf("Error loading sound\n");
    if ((SampleSound[kSFX_WeaponWeaponHit] = LoadSample(kSFX_WeaponWeaponHit_Filename)) < 0)
		printf("Error loading sound\n");
     if ((SampleSound[kSFX_Forcefield] = LoadSample(kSFX_Forcefield_Filename)) < 0)
		printf("Error loading sound\n");
     if ((SampleSound[kSFX_Blood] = LoadSample(kSFX_Blood_Filename)) < 0)
		printf("Error loading sound\n");

	for(x=0; x < MyMixerSFXCHANNELS; x++)
		{
		if ((SamplerInsSFX[x] = LoadInstrument("sampler_16_f1.dsp", 0, PRIO_SFX)) < 0)
 			/*
			 * Load sampler instrument
			 */
			printf("Error Loading DSP Instrument.\n");

			/*
			 * Connect the dsp instruments
			 */
		if (ConnectInstrumentParts(SamplerInsSFX[x], "Output", 0, MyMixerSFX, "Input", x) < 0)
			printf("Error Connecting Inst to Mixer.\n");
		}

	for (x=0; x < kSFX_Count; x++)
		{
		if ((AttachedSamplesSFX[x] = CreateAttachmentVA(SamplerInsSFX[x], SampleSound[x], AF_TAG_SET_FLAGS, AF_ATTF_NOAUTOSTART, TAG_END)) < 0)
			printf("Error Attaching Inst to Sampler.\n");

		ItemTracker = CreateKnob(MyMixerSFX, "Gain", NULL);
		SetKnobPart(ItemTracker, CalcMixerGainPart(MyMixerSFXDEFINE, x, 0), 1.0);
		SetKnobPart(ItemTracker, CalcMixerGainPart(MyMixerSFXDEFINE, x, 1), 1.0);
		DeleteKnob(ItemTracker);

		StartInstrument(SamplerInsSFX[x], NULL);
		}

	StartInstrument(MyMixerSFX, NULL);

	return TRUE;
}

/*
 *
 */
int32 KillSoundFX(void)										/* Remove the sound effects routines */
{
	int32 x;

	DeleteMixerTemplate(MyMixerSFXTemplate);

	for(x=0; x < kSFX_Count; x++)
		if (UnloadSample(SampleSound[x]) < 0)		printf("Error unloading sample\n");

	for(x=0; x < MyMixerSFXCHANNELS;
x++)
		if (UnloadInstrument(SamplerInsSFX[x]) < 0)	printf("Error UnloadInstrument\n");

	if (CloseAudioFolio() != 0)								/* Clore the audio routines */
		return FALSE;

	return TRUE;
}

/******************************************************************************/
