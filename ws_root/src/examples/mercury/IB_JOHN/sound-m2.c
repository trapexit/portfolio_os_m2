/*
 *		[M]ega[P]lay  - sound-m2.c - Module player for the M2
 *
 *			VERSION BY:		Mark Rearick / 3DO Zoo Crew
 *
 *			PROGRAM HISTORY:
 *
 *		001		03/22/96	Began work based off of my Gus routines on the PC...
 *
 *
 *			THINGS TO DO:
 *
 *		.o.		You name it...
 *
 */


#ifdef MACINTOSH
#include <misc:event.h>										/* This is used for handling the controller */
#include <file:fileio.h>									/* It's always nice to be able to actually LOAD the file */
#include <kernel:mem.h>										/* for FreeMemTrack */
#include <kernel:types.h>
#include <kernel:debug.h>									/* for print macro: CHECK_NEG */
#include <kernel:cache.h>									/* For the cache flusher */
#include <kernel:msgport.h>									/* GetMsg */
#include <audio:audio.h>									/* Sound is what this is all about */
#include <audio:parse_aiff.h>								/* This is needed for UnloadSample */
#else
#include <misc/event.h>										/* This is used for handling the controller */
#include <file/fileio.h>									/* It's always nice to be able to actually LOAD the file */
#include <kernel/mem.h>										/* for FreeMemTrack */
#include <kernel/types.h>
#include <kernel/debug.h>									/* for print macro/ CHECK_NEG */
#include <kernel/cache.h>									/* For the cache flusher */
#include <kernel/msgport.h>									/* GetMsg */
#include <audio/audio.h>									/* Sound is what this is all about */
#include <audio/parse_aiff.h>								/* This is needed for UnloadSample */
#endif

#include <stdio.h>											/* Every good 'C' program needs this... */
#include <stdlib.h>											/* for exit() */
#include <string.h>

#include "s3m-info.h"										/* Include support for the S3M format */
#include "sound-m2.h"

/*********************************************************************/

					/*	C		C#		D		D#		E		F		F#		G		G#		A		A#		B	*/
uint16 loctave[12] = {	1712,	1616,	1524,	1440,	1356,	1280,	1208,	1140,	1076,	1016,	960,	907	};

/*********************************************************************/

instrument instrs[MAX_INSTRUMENTS];								/* Here lies the instruments of our creation */

Item MyMixerTemplate;
Item MyMixer;
Item MyMixerGains;												/* This is for panning */
Item SamplerIns[MAX_CHANNELS];									/* The mixers 'channels' */
Item SamplerGains[MAX_CHANNELS];								/* The mixers channels gains */
Item SamplerFreqs[MAX_CHANNELS];								/* Who you calling a Freq? */
Item AttachedSamples[MAX_CHANNELS];								/* Our attachment trackers */
Item DummySample[MAX_CHANNELS];

/*********************************************************************/

#define MYMIXERDEFINE			MakeMixerSpec(MAX_CHANNELS, 2, AF_F_MIXER_WITH_LINE_OUT)
#define MAGIC_FREQ				(14317056)						/* This is the magic number for tuning/sample playing */
#define PRIO_MOD				(50)							/* Priority of the module replay */

/*********************************************************************/

#define S3M_MAXPAN				(16.0)							/* The maximum panning value */
#define S3M_MAXVOLUME			(256.0)							/* The maximum volume value (it's really 64 * 4)) */

/*********************************************************************/

void M2_SetFrq(uint8 voice, uint32 period)						/* Set the frequency (sample rate) for playback */
{
	float32 realrate = (float32)(MAGIC_FREQ/period);		/* Convert S3M period into Hz... */

	realrate *= globalpitch;
	SetKnobPart(SamplerFreqs[voice], 0, realrate);
}

/*********************************************************************/

void M2_SetPan(uint8 voice, uint8 inposition)					/* Set the pan position (0x0-0xF) of each channel */
{
	float32 leftgain = globalvolume;							/* Set initial values to global volume */
	float32 rightgain = globalvolume;


	if (inposition > 7)											/* It's to the left */
		rightgain *= ((float32)inposition / S3M_MAXPAN);
	else
		if (inposition < 7)										/* It's to the right */
			leftgain *= (1.0 - ((float32)inposition / S3M_MAXPAN));


	SetKnobPart(MyMixerGains, CalcMixerGainPart(MYMIXERDEFINE, voice, 0), leftgain);
	SetKnobPart(MyMixerGains, CalcMixerGainPart(MYMIXERDEFINE, voice, 1), rightgain);
}

/*********************************************************************/

void M2_SetVol(uint8 voice, uint8 involume)						/* Set the channels volume level */
{
	float32 volume = ((float32)involume / S3M_MAXVOLUME);		/* Convert S3M volume (0-63) to M2 Audio (0.0-1.0) */

	SetKnobPart(SamplerGains[voice], 0, volume);
}

/*********************************************************************/

void M2_PlayVoice(uint8 voice, uint8 instr, uint16 Offs)		/* Play back a sample */
{
	if ((instrs[instr].length == 0) ||							/* This isn't supposed to be here, but some mods do this!  LAME! */
		(instr > MAX_INSTRUMENTS))
		return;

	if (SetAudioItemInfoVA(DummySample[voice],
										AF_TAG_ADDRESS,			instrs[instr].sampledata,				/* The address of the sample */
										AF_TAG_NUMBYTES,		instrs[instr].length,					/* The size of the sample */
										AF_TAG_NUMBITS,			8,										/* The bit depth */
										AF_TAG_CHANNELS,		1,										/* Number of channels used */
										AF_TAG_SUSTAINBEGIN,	instrs[instr].loopstart,				/* Where the loop begins */
										AF_TAG_SUSTAINEND,		instrs[instr].loopend,					/* Where the loop ends */
										AF_TAG_SAMPLE_RATE_FP,	ConvertFP_TagData(instrs[instr].c2spd),	/* What the sample rate is */
										TAG_END) < 0)
		printf("Error setting up audioinfo for inst (%d) to voice (%d)\n", instr, voice);

	StartInstrument(SamplerIns[voice], NULL);

	if (instrs[instr].loop == 0)								/* There's no looping here */
		ReleaseInstrument(SamplerIns[voice], NULL);
}

/*********************************************************************/

void M2_StopVoice(uint8 voice)									/* Stop playback of a sample */
{
	StopInstrument(SamplerIns[voice], NULL);					/* Forget checking!  Just get rid of the thing... */
}

/*********************************************************************/

#define DUMMY_SAMPLE_SIZE		(16)

void M2_Init(void)												/* Initialize the M2's audio facilities */
{
	int8 *tempsample;
	int32 x;

    if ((tempsample = AllocMem((sizeof(int8) * DUMMY_SAMPLE_SIZE), MEMTYPE_NORMAL)) == NULL)		/* Set up the dummy Sample Buffer */
        Abort("Memory Allocation fail in M2_Init");

	if ((MyMixerTemplate = CreateMixerTemplate(MYMIXERDEFINE, NULL)) < 0)							/* Create the mixer Template */
		printf("CreateMixerTemplate Failed.\n");

	if ((MyMixer = CreateInstrumentVA(MyMixerTemplate, AF_TAG_PRIORITY, PRIO_MOD, TAG_END)) < 0)						/* Creates the actual mixer */
		printf("CreateInstrument Mixer Failed.\n");

	MyMixerGains = CreateKnob(MyMixer, "Gain", NULL);											/* Set up my mixer gain knob */

	for(x=0; x < MAX_CHANNELS; x++)
		{
		if ((SamplerIns[x] = LoadInstrument("sampler_8_v1.dsp", 0, PRIO_MOD)) < 0)				/* Load sampler instrument */
			printf("Error Loading DSP instrument.\n");

		if (ConnectInstrumentParts(SamplerIns[x], "Output", 0, MyMixer, "Input", x) < 0)		/* Connect the dsp instruments */
			printf("Error Connecting Inst to Mixer.\n");

		SetKnobPart(MyMixerGains, CalcMixerGainPart(MYMIXERDEFINE, x, 0), 0.0);					/* Make sure each part is inited to the max */
		SetKnobPart(MyMixerGains, CalcMixerGainPart(MYMIXERDEFINE, x, 1), 0.0);

		SamplerFreqs[x] = CreateKnob(SamplerIns[x], "SampleRate", NULL);						/* Set up the global freq knobs */
		SamplerGains[x] = CreateKnob(SamplerIns[x], "Amplitude", NULL);							/* And do the same for the gain */

		if ((DummySample[x] = CreateSampleVA(AF_TAG_ADDRESS, tempsample, AF_TAG_NUMBYTES, DUMMY_SAMPLE_SIZE, AF_TAG_NUMBITS, 8, AF_TAG_CHANNELS, 1, AF_TAG_SUSTAINBEGIN, 0, AF_TAG_SUSTAINEND, DUMMY_SAMPLE_SIZE, AF_TAG_SAMPLE_RATE_FP, ConvertFP_TagData(8423), TAG_END)) < 0)
			printf("Error creating dummy sample (%d)\n", x);

		if ((AttachedSamples[x] = CreateAttachmentVA(SamplerIns[x], DummySample[x], TAG_END)) < 0)
			printf("Error Attaching to voice (%d).\n", x);
		}

	if (StartInstrument(MyMixer, NULL) < 0)
		printf("Error StartInstrument MyMixer\n");

	FreeMem(tempsample, (sizeof(int8) * DUMMY_SAMPLE_SIZE));					/* Free up the dummy sample space */
}

void M2_Remove(void)										/* Clear out everything we used */
{
	int32 x;

	DeleteMixerTemplate(MyMixerTemplate);
	DeleteKnob(MyMixerGains);

	for(x=0; x < MAX_CHANNELS; x++)
		{
		DeleteAttachment(AttachedSamples[x]);

		if (UnloadInstrument(SamplerIns[x]) < 0)
			printf("Error UnloadInstrument\n");

		DeleteKnob(SamplerGains[x]);
		DeleteKnob(SamplerFreqs[x]);
		DeleteItem(DummySample[x]);
		}

	for(x=0; x < MAX_INSTRUMENTS; x++)
		if (instrs[x].c2spd > 0)
			{
			if (instrs[x].sampledata > 0)
				FreeMem(instrs[x].sampledata, TRACKED_SIZE);
			}

	if (CloseAudioFolio() != 0)										/* Close the audio routines */
		printf("Couldn't close audio folio in M2_Remove()\n");
}

/*********************************************************************/

void M2_SetupInst(uint8 instr, RawFile *s3m, s3minstr *CurInst)		/* Initialize and Convert S3M sample to M2 sample */
{
	uint32 x;


    if ((instrs[instr].sampledata = AllocMem((sizeof(int8) * CurInst->Length), MEMTYPE_TRACKSIZE)) == NULL)		/* Set up the Sample Buffer */
        Abort("Memory Allocation fail in M2_SetupInst");
	memset(instrs[instr].sampledata, 0, (sizeof(int8) * CurInst->Length));			/* Zero out the buffer */

	ReadRawFile(s3m, instrs[instr].sampledata, (sizeof(int8) * CurInst->Length));	/* Read in the sample data */

	for(x=0; x < (sizeof(uint8) * CurInst->Length); x++)							/* This fixes the samples from PC to M2 */
		instrs[instr].sampledata[x] ^= 0x80;										/* This handles the sign efficiently */

	instrs[instr].c2spd  =  CurInst->C2spd;								/* What's the sampling rate */
	instrs[instr].volume =  CurInst->Vol;								/* What's the default volume */
	instrs[instr].length = CurInst->Length;								/* This is obviously where the loop starts!  duh! */

	strcpy(instrs[instr].dosname, CurInst->DOSName);					/* Copy some of the name stuff */
	strcpy(instrs[instr].name, CurInst->Sname);

	instrs[instr].loop   =  CurInst->Flags;								/* Does it loop? */

	if (instrs[instr].loop == 0)
		{
		instrs[instr].loopstart = 0;									/* There's no loop here! */
		instrs[instr].loopend = CurInst->Length;						/* There's ain't no loopin' going on! */
		}
	else
		{
		instrs[instr].loopstart = CurInst->LoopBg;						/* This is obviously where the loop starts!  duh! */
		instrs[instr].loopend = CurInst->LoopNd;						/* There's a loop, and it ends HERE! */
		}

	for(x=0; x<12;x++)													/* Precalculate tuning */
		instrs[instr].tune[x] = ((8363 * 16 * loctave[x]) / instrs[instr].c2spd);
}

/*********************************************************************/
