/* $Id: ta_halfrate.c,v 1.1 1994/10/25 00:11:36 phil Exp phil $ */
/***************************************************************
**
** Test Half Rate DSP execution.
** 
** Play AIFF Sample in memory using Control Pad
** By:  Phil Burk
**
** Link with the music.lib to get SelectSamplePlayer()
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
***************************************************************/

#include "types.h"
#include "filefunctions.h"
#include "debug.h"
#include "operror.h"
#include "stdio.h"
#include "stdlib.h"
#include "event.h"

/* Include this when using the Audio Folio */
#include "audio.h"
#include "music.h"

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		PrintError(0,"\\failure in",name,val); \
		goto cleanup; \
	}

Err TestHalfRateSample( char *SampleName, int32 Pitch, int32 Divider );
Err TestHalfRateSynth( int32 Pitch, int32 Divider );

int main(int argc, char *argv[])
{
	int32 Result = -1;
	int32 Pitch = 55;
	char *SampleName = "$samples/PitchedL/Cello/Cello.C3LM44k.aiff";
	
	PRT(("Usage: %s <samplefile> <Pitch>\n", argv[0]));
	
	if (argc > 1) SampleName = argv[1];  /* from command line? */
	if (argc > 2)
	{
		Pitch=atoi(argv[2]);
	}
	
	PRT(("Button Menu:\n"));
	PRT(("   A = Start\n"));
	PRT(("   B = Release\n"));
	PRT(("   C = Stop\n"));
	PRT(("   X = Next\n"));
	
/* Initialize audio, return if error. */ 
	Result = OpenAudioFolio();
	if (Result < 0)
	{
		PrintError(0,"Audio Folio could not be opened.",0,Result);
		return(-1);
	}

/* Initialize the EventBroker. */
#ifndef LC_ISFOCUSED
#define LC_ISFOCUSED 1
#endif
	Result = InitEventUtility(1, 0, TRUE);
	if (Result < 0)
	{
		PrintError(0,"InitEventUtility",0,Result);
		goto cleanup;
	}

	PRT(("Normal rate sample. =======================================\n"));
	Result = TestHalfRateSample( SampleName, Pitch, 1 );
	CHECKRESULT(Result,"TestHalfRateSample");
	PRT(("And now at HALF RATE  sample!!! ============================\n"));
	Result = TestHalfRateSample( SampleName, Pitch, 2 );
	CHECKRESULT(Result,"TestHalfRateSample");
	
	PRT(("Normal rate synth. =======================================\n"));
	Result = TestHalfRateSynth( Pitch, 1 );
	CHECKRESULT(Result,"TestHalfRateSynth");
	PRT(("And now at HALF RATE synth!!! ============================\n"));
	Result = TestHalfRateSynth( Pitch, 2 );
	CHECKRESULT(Result,"TestHalfRateSynth");
	
cleanup:
/* Cleanup the EventBroker. */
	Result = KillEventUtility();
	if (Result < 0)
	{
		PrintError(0,"KillEventUtility",0,Result);
	}
	CloseAudioFolio();
	PRT(( "%s finished.\n", argv[0] ));
	return((int) Result);
}

	
Err TestHalfRateSample( char *SampleName, int32 Pitch, int32 Divider )
{
	Item OutputIns = 0;
	Item SamplerTmp = 0, SamplerIns = 0;
	Item SampleItem = 0;
	Item Attachment = 0;
	int32 Result = -1;
	int32 DoIt = TRUE;
	int32 IfVariable = FALSE;
	char *InstrumentName;
	uint32 Buttons;
	ControlPadEventData cped;
	TagArg SamplerTags[3];
	
	
	if( Pitch != 60 ) IfVariable = TRUE;
	
/* Use directout instead of mixer. */
	OutputIns = LoadInstrument("directout.dsp",  0, 100);
	CHECKRESULT(OutputIns,"LoadInstrument");
	StartInstrument( OutputIns, NULL );

/* Load digital audio Sample from disk. */
	SampleItem = LoadSample(SampleName);
	CHECKRESULT(SampleItem,"LoadSample");
	
/* Look at sample information. */
//	DebugSample(SampleItem);
	
/* Load Sampler instrument */
	InstrumentName = SelectSamplePlayer( SampleItem , IfVariable );
	if (InstrumentName == NULL)
	{
		ERR(("No instrument to play that sample.\n"));
		goto cleanup;
	}
	PRT(("Use instrument: %s\n", InstrumentName));
	SamplerTmp = LoadInsTemplate(InstrumentName,  0);
	
	{
		TagArg Tags[4];
	
		Tags[0].ta_Tag = AF_TAG_TEMPLATE;
		Tags[0].ta_Arg = (void *) SamplerTmp;
		Tags[1].ta_Tag = AF_TAG_PRIORITY;
		Tags[1].ta_Arg = (void *) 100;
		Tags[2].ta_Tag = AF_TAG_CALCRATE_DIVIDE;
		Tags[2].ta_Arg = (void *) Divider;
		Tags[3].ta_Tag = TAG_END;
	
		SamplerIns = CreateItem( MKNODEID(AUDIONODE,AUDIO_INSTRUMENT_NODE), &Tags[0] );
	}
	CHECKRESULT(SamplerIns,"LoadInstrument");

/* Connect Sampler Instrument to DirectOut. Works for mono or stereo. */
	Result = ConnectInstruments (SamplerIns, "Output", OutputIns, "InputLeft");
	if( Result >= 0 )
	{
		Result = ConnectInstruments (SamplerIns, "Output", OutputIns, "InputRight");
		CHECKRESULT(Result,"ConnectInstruments");
	}
	else
	{
		Result = ConnectInstruments (SamplerIns, "LeftOutput", OutputIns, "InputLeft");
		CHECKRESULT(Result,"ConnectInstruments");
		Result = ConnectInstruments (SamplerIns, "RightOutput", OutputIns, "InputRight");
		CHECKRESULT(Result,"ConnectInstruments");
	}
	
/* Attach the sample to the instrument. */
	Attachment = AttachSample(SamplerIns, SampleItem, 0);
	CHECKRESULT(Attachment,"AttachSample");

	SamplerTags[0].ta_Tag = AF_TAG_PITCH;
	SamplerTags[0].ta_Arg = (void *) Pitch;
	SamplerTags[1].ta_Tag = TAG_END;
	Result = StartInstrument( SamplerIns, SamplerTags );
	
	while(DoIt)
	{
		
/* Get User input. */
		Result = GetControlPad (1, TRUE, &cped);
		if (Result < 0) {
			PrintError(0,"read control pad in","PlaySoundFile",Result);
		}
		Buttons = cped.cped_ButtonBits;
	
/* Process buttons pressed. */
		if(Buttons & ControlX) /* EXIT */
		{
			DoIt = FALSE;
		}
		if(Buttons & ControlA) /* START */
		{
			Result = StartInstrument( SamplerIns, SamplerTags );
			CHECKRESULT(Result,"StartInstrument");
		}
		if(Buttons & ControlB) /* RELEASE */
		{
			Result = ReleaseInstrument( SamplerIns, NULL );
			CHECKRESULT(Result,"ReleaseInstrument");
		}
		if(Buttons & ControlC) /* STOP */
		{
			Result = StopInstrument( SamplerIns, NULL );
			CHECKRESULT(Result,"StopInstrument");
		}
	}

	Result = StopInstrument( SamplerIns, NULL );
	CHECKRESULT(Result,"StopInstrument");
	
cleanup:

	DetachSample( Attachment );
	UnloadSample( SampleItem );
	UnloadInstrument( SamplerIns );
	UnloadInstrument( OutputIns );
	return Result;
}


Err TestHalfRateSynth( int32 Pitch, int32 Divider )
{
	Item OutputIns = 0;
	Item SamplerTmp = 0, SamplerIns = 0;
	int32 Result = -1;
	int32 DoIt = TRUE;
	int32 IfVariable = FALSE;
	uint32 Buttons;
	ControlPadEventData cped;
	TagArg SamplerTags[3];
	
	
	if( Pitch != 60 ) IfVariable = TRUE;
	
/* Use directout instead of mixer. */
	OutputIns = LoadInstrument("directout.dsp",  0, 100);
	CHECKRESULT(OutputIns,"LoadInstrument");
	StartInstrument( OutputIns, NULL );

	SamplerTmp = LoadInsTemplate("sawtooth.dsp",  0);
	
	{
		TagArg Tags[4];
	
		Tags[0].ta_Tag = AF_TAG_TEMPLATE;
		Tags[0].ta_Arg = (void *) SamplerTmp;
		Tags[1].ta_Tag = AF_TAG_PRIORITY;
		Tags[1].ta_Arg = (void *) 100;
		Tags[2].ta_Tag = AF_TAG_CALCRATE_DIVIDE;
		Tags[2].ta_Arg = (void *) Divider;
		Tags[3].ta_Tag = TAG_END;
	
		SamplerIns = CreateItem( MKNODEID(AUDIONODE,AUDIO_INSTRUMENT_NODE), &Tags[0] );
	}
	CHECKRESULT(SamplerIns,"LoadInstrument");

/* Connect Sampler Instrument to DirectOut. Works for mono or stereo. */
	Result = ConnectInstruments (SamplerIns, "Output", OutputIns, "InputLeft");
	if( Result >= 0 )
	{
		Result = ConnectInstruments (SamplerIns, "Output", OutputIns, "InputRight");
		CHECKRESULT(Result,"ConnectInstruments");
	}
	else
	{
		Result = ConnectInstruments (SamplerIns, "LeftOutput", OutputIns, "InputLeft");
		CHECKRESULT(Result,"ConnectInstruments");
		Result = ConnectInstruments (SamplerIns, "RightOutput", OutputIns, "InputRight");
		CHECKRESULT(Result,"ConnectInstruments");
	}
	

	SamplerTags[0].ta_Tag = AF_TAG_PITCH;
	SamplerTags[0].ta_Arg = (void *) Pitch;
	SamplerTags[1].ta_Tag = TAG_END;
	Result = StartInstrument( SamplerIns, SamplerTags );
	
	while(DoIt)
	{
		
/* Get User input. */
		Result = GetControlPad (1, TRUE, &cped);
		if (Result < 0) {
			PrintError(0,"read control pad in","PlaySoundFile",Result);
		}
		Buttons = cped.cped_ButtonBits;
	
/* Process buttons pressed. */
		if(Buttons & ControlX) /* EXIT */
		{
			DoIt = FALSE;
		}
		if(Buttons & ControlA) /* START */
		{
			Result = StartInstrument( SamplerIns, SamplerTags );
			CHECKRESULT(Result,"StartInstrument");
		}
		if(Buttons & ControlB) /* RELEASE */
		{
			Result = ReleaseInstrument( SamplerIns, NULL );
			CHECKRESULT(Result,"ReleaseInstrument");
		}
		if(Buttons & ControlC) /* STOP */
		{
			Result = StopInstrument( SamplerIns, NULL );
			CHECKRESULT(Result,"StopInstrument");
		}
	}

	Result = StopInstrument( SamplerIns, NULL );
	CHECKRESULT(Result,"StopInstrument");
	
cleanup:
	UnloadInstrument( SamplerIns );
	UnloadInstrument( OutputIns );
	return Result;
}
