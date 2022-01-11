/****************************************************************************
**
**  @(#) sfx_tools.c 95/05/09 1.3
**
****************************************************************************/
/******************************************
** Sound Effects Tools
**
** Author: Phil Burk
** Copyright (c) 1995 3DO 
** All Rights Reserved
******************************************/

#include <kernel/types.h>
#include "stdio.h"
#include <audio/audio.h>
#include "sfx_tools.h"

#define PRT(x)   { printf x; }
#define ERR(x)   PRT(x)
#define DBUG(x)  /* PRT(x) */

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		PrintError(0,"\\failure in",name,Result); \
		goto error; \
	}

/****************************************************************/
Err LoadSingleSound( Item SampleItem, char *InsName,  SingleSound **ssndPtr )
{
	SingleSound *ssnd;
	Err Result;

	ssnd = calloc(sizeof(SingleSound), 1);
	if(ssnd == NULL)
	{
		*ssndPtr = NULL;
		return -2;
	}

	ssnd->ssnd_Sample = SampleItem;
	ssnd->ssnd_OutputIns = LoadInstrument("directout.dsp", 0, 100);
	CHECKRESULT(ssnd->ssnd_OutputIns,"LoadSingleSound: LoadInstrument");

/* Load Sampler instrument */
	ssnd->ssnd_Instrument = LoadInstrument(InsName, 0, 100);
	CHECKRESULT(ssnd->ssnd_Instrument,"LoadSingleSound: LoadInstrument");

/* Connect Sampler Instrument to Output. */
	Result = ConnectInstruments ( ssnd->ssnd_Instrument, "Output",
		ssnd->ssnd_OutputIns, "InputLeft");
	CHECKRESULT(Result,"LoadSingleSound: ConnectInstruments");
	Result = ConnectInstruments ( ssnd->ssnd_Instrument, "Output",
		ssnd->ssnd_OutputIns, "InputRight");
	CHECKRESULT(Result,"LoadSingleSound: ConnectInstruments");

/* Attach the sample to the instrument for playback. */
	ssnd->ssnd_Attachment = AttachSample( ssnd->ssnd_Instrument, SampleItem, 0);
	CHECKRESULT(ssnd->ssnd_Attachment,"LoadSingleSound: AttachSample");

/* Grab knobs for continuous control. */
	ssnd->ssnd_AmplitudeKnob = GrabKnob( ssnd->ssnd_Instrument, "Amplitude" );
	CHECKRESULT(ssnd->ssnd_AmplitudeKnob, "LoadSingleSound: GrabKnob");
/* Try to grab frequency knob.  Don't panic.  Many don't have one. */
	ssnd->ssnd_FrequencyKnob = GrabKnob( ssnd->ssnd_Instrument, "Frequency" );
	if( ssnd->ssnd_FrequencyKnob < 0 ) ssnd->ssnd_FrequencyKnob = 0;

	Result = StartInstrument( ssnd->ssnd_OutputIns, NULL );
	CHECKRESULT( Result, "LoadSingleSound: StartInstrument");

/* Pass back ssnd */
	*ssndPtr = ssnd;

	return Result;

error:
	return Result;
}

/****************************************************************/
Err ControlSingleSound( SingleSound *ssnd, int32 Amplitude, int32 Frequency )
{
	int32 i;
	int32 Result;

	Result = TweakRawKnob( ssnd->ssnd_AmplitudeKnob, Amplitude );
	CHECKRESULT( Result, "FireSingleSoundBank: TweakRawKnob");
	if( ssnd->ssnd_FrequencyKnob )
	{
		Result = TweakRawKnob( ssnd->ssnd_FrequencyKnob, Frequency );
		CHECKRESULT( Result, "FireSingleSoundBank: TweakRawKnob");
	}
error:
	return Result;
}

/****************************************************************/
Err StartSingleSound( SingleSound *ssnd, int32 Amplitude, int32 Frequency )
{
	int32 i;
	int32 Result;

	Result = ControlSingleSound( ssnd, Amplitude, Frequency );
	CHECKRESULT( Result, "StartSingleSound: ControlSingleSound");

	Result = StartInstrument( ssnd->ssnd_Instrument, NULL);
	CHECKRESULT( Result, "StartSingleSound: StartInstrument");

error:
	return Result;
}

/****************************************************************/
Err StopSingleSound( SingleSound *ssnd )
{
	int32 i;
	int32 Result;

	Result = ReleaseInstrument( ssnd->ssnd_Instrument, NULL);
	CHECKRESULT( Result, "StartSingleSound: ReleaseInstrument");

error:
	return Result;
}
/****************************************************************/
Err LoadSingleSoundBank( Item SampleItem, Item InsTemplate, Item MixerIns, int32 NumVoices, SingleSoundBank **ssbPtr )
{
	SingleSoundBank *ssb;
	int32 i;
	char ScratchName[32];
	Err Result;

	if( NumVoices > SSB_MAX_VOICES)
	{
		ERR(("sfxLoadSingleSoundBank: too many voices %d > %d\n", NumVoices, SSB_MAX_VOICES));
		return -1;
	}

	ssb = calloc(sizeof(SingleSoundBank), 1);
	if(ssb == NULL)
	{
		*ssbPtr = NULL;
		return -2;
	}

	ssb->ssb_Sample = SampleItem;
	ssb->ssb_MixerIns = MixerIns;
	ssb->ssb_InsTemplate = InsTemplate;
	ssb->ssb_NumVoices = NumVoices;

	for( i=0; i<NumVoices; i++ )
	{
/* Load Sampler instrument */
		ssb->ssb_Instruments[i] = AllocInstrument(InsTemplate, 100);
		CHECKRESULT(ssb->ssb_Instruments[i],"LoadSingleSoundBank: AllocInstrument");

/* Connect Sampler Instrument to Mixer. Works for mono or stereo. */
		sprintf( ScratchName, "Input%d", i );
		Result = ConnectInstruments ( ssb->ssb_Instruments[i], "Output", MixerIns, ScratchName);
		CHECKRESULT(Result,"LoadSingleSoundBank: ConnectInstruments");

/* Attach the sample to the instrument for playback. */
		ssb->ssb_Attachments[i] = AttachSample( ssb->ssb_Instruments[i], SampleItem, 0);
		CHECKRESULT(ssb->ssb_Attachments[i],"LoadSingleSoundBank: AttachSample");

/* Grab knobs for continuous control. */
		sprintf( ScratchName, "LeftGain%d", i );
		PRT(("ScratchName = %s\n", ScratchName ));
		ssb->ssb_LeftGains[i] = GrabKnob( MixerIns, ScratchName );
		CHECKRESULT(ssb->ssb_LeftGains[i], "LoadSingleSoundBank: GrabKnob");
		sprintf( ScratchName, "RightGain%d", i );
		PRT(("ScratchName = %s\n", ScratchName ));
		ssb->ssb_RightGains[i] = GrabKnob( MixerIns, ScratchName );
		CHECKRESULT(ssb->ssb_RightGains[i], "LoadSingleSoundBank: GrabKnob");
	}

	Result = StartInstrument( MixerIns, NULL );
	CHECKRESULT( Result, "LoadSingleSoundBank: StartInstrument");

/* Pass back ssb */
	*ssbPtr = ssb;

	return Result;

error:
	return Result;
}

/****************************************************************/
Err FireSingleSoundBank( SingleSoundBank *ssb, int32 Amplitude, int32 Frequency )
{
	int32 i;
	int32 Result;
	
	i = ssb->ssb_NextAvailable++;
	if( ssb->ssb_NextAvailable >= ssb->ssb_NumVoices ) ssb->ssb_NextAvailable = 0;


	Result = TweakRawKnob( ssb->ssb_LeftGains[i], Amplitude );
	CHECKRESULT( Result, "FireSingleSoundBank: TweakRawKnob");
	Result = TweakRawKnob( ssb->ssb_RightGains[i], Amplitude );
	CHECKRESULT( Result, "FireSingleSoundBank: TweakRawKnob");

	Result = StartInstrument( ssb->ssb_Instruments[i], NULL);
	CHECKRESULT( Result, "FireSingleSoundBank: StartInstrument");

error:
	return Result;
}
