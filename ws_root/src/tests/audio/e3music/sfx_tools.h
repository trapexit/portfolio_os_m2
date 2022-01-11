#ifndef __SFX_TOOLS_H
#define __SFX_TOOLSSFX_TOOLS_H


/****************************************************************************
**
**  @(#) sfx_tools.h 95/05/08 1.2
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
#include <audio/audio.h>

typedef struct SingleSound
{
	Item    ssnd_Sample;
	Item    ssnd_OutputIns;
	Item	ssnd_Attachment;
	Item	ssnd_Instrument;
	Item	ssnd_AmplitudeKnob;
	Item	ssnd_FrequencyKnob;
} SingleSound;

#define SSB_MAX_VOICES  (8)
typedef struct SingleSoundBank
{
	Item    ssb_Sample;
	Item    ssb_MixerIns;
	Item    ssb_NumVoices;
	Item    ssb_InsTemplate;
	Item	ssb_Attachments[SSB_MAX_VOICES];
	Item	ssb_Instruments[SSB_MAX_VOICES];
	Item	ssb_LeftGains[SSB_MAX_VOICES];
	Item	ssb_RightGains[SSB_MAX_VOICES];
	int32   ssb_NextAvailable;   /* Round robin scheduler. */
} SingleSoundBank;

Err LoadSingleSound( Item SampleItem, char *InsName,  SingleSound **ssndPtr );
Err StartSingleSound( SingleSound *ssnd, int32 Amplitude, int32 Frequency );
Err ControlSingleSound( SingleSound *ssnd, int32 Amplitude, int32 Frequency );
Err StopSingleSound( SingleSound *ssnd );

Err LoadSingleSoundBank( Item SampleItem, Item InsTemplate, Item MixerIns, int32 NumVoices, SingleSoundBank **ssbPtr );
Err FireSingleSoundBank( SingleSoundBank *ssb, int32 Amplitude, int32 Frequency );

#endif /* __SFX_TOOLS_H */
