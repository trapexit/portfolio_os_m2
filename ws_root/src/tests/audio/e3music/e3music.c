
/******************************************************************************
**
**  @(#) e3music.c 95/05/09 1.8
**
******************************************************************************/
/******************************************
** Sound Manager for E3 flying demo
**
** Author: Phil Burk
** Copyright (c) 1995 3DO 
** All Rights Reserved
**
** Sound file spooling based on tsp_spoolsoundfile.c by Bill Barton
**
** @@@ Warning!  This is a demo hack.
**     It does not clean up after itself very well.
******************************************/

#include <audio/audio.h>
#include <audio/musicerror.h>
#include <audio/score.h>            /* SelectSamplePlayer() */
#include <audio/soundplayer.h>
#include <kernel/kernel.h>
#include <kernel/mem.h>
#include <stdio.h>

#include "e3music.h"

/* -------------------- Macros */

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		PrintError(0,"\\failure in",name,Result); \
		goto error; \
	}

#define CHECKSIGNAL(val,name) \
	if (val <= 0) \
	{ \
		Result = val ? val : AF_ERR_NOSIGNAL; \
		PrintError (NULL,name,NULL,Result); \
		goto error; \
	}

#define CHECKPTR(val,name) \
	if (val == 0) \
	{ \
		Result = -1; \
		ERR(("Failure in %s\n", name)); \
		goto error; \
	}

/******************************************************************/
/********** Spool Soundfile ***************************************/
/******************************************************************/

/* -------------------- Parameters */

#define NUMBLOCKS (32)
#define BLOCKSIZE (2048)
#define BUFSIZE (NUMBLOCKS*BLOCKSIZE)
#define NUMBUFFS  (4)
#define MAXAMPLITUDE (0x7FFF)

/*
** Allocate enough space so that you don't get stack overflows.
** An overflow will be characterized by seemingly random crashes
** that defy all attempts at logical analysis.  You might want to
** start big then reduce the size till you crash, then double it.
*/
#define STACKSIZE (10000)
#define PRIORITY  (180)


/* -------------------- Globals for SpoolSoundFileThread */

char *gFileName;
int32 gSignal1;
Item  gMainTaskItem;
int32 gNumReps;
Item  gSpoolerThread;

Item  gSpoolerIns;
Item  gMusicAmplitudeKnob;

SingleSound *gMusicSound;
SingleSound *gEngineSound;
SingleSound *gCannonSound;

Item  gWindOutputIns;
Item  gWindIns;
Item  gWindRedNoise;
Item  gWindTimesPlus;
Item  gWindAmplitudeKnob;
Item  gWindResonanceKnob;
Item  gWindFrequencyKnob;
Item  gWindModDepthKnob;
Item  gWindModRateKnob;

#if 0
/* -------------------- Functions */

Err SpoolSoundFile (char *fileName, int32 numRepeats);
void SpoolSoundFileThread (void);

/*
    Sets up globals for background thread, starts it,
    then waits for its completion. Instead of just waiting
    for it to complete, this function could do other stuff
    after starting the thread.
*/
Err SpoolSoundFile (char *fileName, int32 numRepeats)
{
 	int32 Result=0;

        /* put parameters into globals for thread */
	gFileName = fileName;
	gNumReps = numRepeats;

	PRT(("e3music: Play file %s %d times.\n", gFileName, gNumReps));

        /* Get parent task Item so that thread can signal back. */
	gMainTaskItem = CURRENTTASK->t.n_Item;

        /* Allocate a signal for each thread to notify parent task. */
	gSignal1 = AllocSignal( 0 );
	CHECKSIGNAL(gSignal1,"AllocSignal");

        /* create the thread. execution can begin immediately */
	gSpoolerThread = CreateThread("SpoolSoundFileThread", PRIORITY,
                                 SpoolSoundFileThread, STACKSIZE);
	CHECKRESULT(gSpoolerThread,"CreateThread");

error:
	return Result;
}


/* -------------------- SpoolSoundFileThread() */

int32 PlaySoundFile (char *FileName, int32 BufSize, int32 NumReps);
Err LoopDecisionFunction (SPAction *resultAction, int32 *remainingCountP, SPSound *sound, const char *markerName);
Err SelectFixedSamplerForFile (char **resultInstrumentName, char *fileName);

void SpoolSoundFileThread( void )
{	
	int32 Result;

        /* open the audio folio for this this thread. */
	Result = OpenAudioFolio();
	CHECKRESULT (Result, "open audio folio");

        /* play the sound file passed thru the globals */
	PlaySoundFile (gFileName, BUFSIZE, gNumReps);

error:
	SendSignal( gMainTaskItem, gSignal1 );
	CloseAudioFolio();
	WaitSignal(0);
	/* Waits forever. Don't return! Thread gets deleted by parent. */
}

int32 PlaySoundFile (char *FileName, int32 BufSize, int32 NumReps)
{
	int32 Result;
	char *samplername;
	Item outputins=0;
	SPPlayer *player=NULL;
	SPSound *sound=NULL;

	PRT(("e3music thread: PlaySoundFile() using Advanced Sound Player. NumReps=%ld\n", NumReps));

        /* pick appropriate sample player */
	Result = SelectFixedSamplerForFile (&samplername, FileName);
	CHECKRESULT (Result, "select sample player");

	    /* load and connect instruments */
	PRT(("e3music thread: using '%s'\n", samplername));
	gSpoolerIns = LoadInstrument (samplername, 0, 100);
	CHECKRESULT (gSpoolerIns, "load sampler instrument");

	outputins = LoadInstrument ("directout.dsp", 0, 100);
	CHECKRESULT (outputins, "load output instrument");

/* Grab knob to allow control over music amplitude. */
	gMusicAmplitudeKnob = GrabKnob( gSpoolerIns, "Amplitude" );
	CHECKRESULT(gMusicAmplitudeKnob, "GrabKnob");

        /* try mono connection */
	ConnectInstruments (gSpoolerIns, "Output", outputins, "InputLeft");
	Result = ConnectInstruments (gSpoolerIns, "Output", outputins, "InputRight");

	    /* if that failed, try stereo connection */
	if (Result < 0)
	{
		Result = ConnectInstruments (gSpoolerIns, "LeftOutput",
                                     outputins, "InputLeft");
		CHECKRESULT (Result, "connect left");
		Result = ConnectInstruments (gSpoolerIns, "RightOutput",
                                     outputins, "InputRight");
		CHECKRESULT (Result, "connect right");
	}

        /* start output instrument */
	Result = StartInstrument (outputins, NULL);
	CHECKRESULT (Result, "start output");

        /* create player */
	Result = spCreatePlayer (&player, gSpoolerIns, NUMBUFFS, BufSize, NULL);
	CHECKRESULT (Result, "create player");

	Result = spAddSoundFile (&sound, player, FileName);
	if (Result < 0)
	{
		PrintError (NULL, "add sound", FileName, Result);
		goto error;
	}

        /* set up default action to loop sound */
	Result = spLoopSound (sound);
	CHECKRESULT (Result, "loop sound");

        /* start playing */
	Result = spStartReading (sound, SP_MARKER_NAME_BEGIN);
	CHECKRESULT (Result, "start reading");
#if 0
	Result = spStartPlayingVA (player, AF_TAG_AMPLITUDE, MAXAMPLITUDE,
                    TAG_END);
#endif
	CHECKRESULT (Result, "start playing");

	{
		const int32 playersigs = spGetPlayerSignalMask (player);

		while (spGetPlayerStatus(player) & SP_STATUS_F_BUFFER_ACTIVE)
		{
			const int32 sigs = WaitSignal (playersigs);

			Result = spService (player, sigs);
			CHECKRESULT (Result, "Service");
		}
	}

	spStop (player);
	PRT(("e3music thread: done.\n"));

error:
	spDeletePlayer (player);
	UnloadInstrument (gSpoolerIns);
	UnloadInstrument (outputins);
	return Result;
}

/****************************************************************/
Err SelectFixedSamplerForFile (char **resultSamplerName, char *fileName)
{
    char *InstrumentName = NULL;
    Item TempSample;
    int32 Result = 0;

        /* validate resultSamplerName */
    if (!resultSamplerName) return ML_ERR_BADPTR;

        /* scan AIFF file to get an item for SelectSamplePlayer() */
	TempSample = ScanSample (fileName, 0);
	CHECKRESULT (TempSample,"ScanSample");

	    /* find a suitable instrument */
    InstrumentName = SelectSamplePlayer (TempSample, FALSE);
    if (InstrumentName == NULL)
    {
        ERR(("No instrument to play that sample.\n"));
        Result = ML_ERR_UNSUPPORTED_SAMPLE;
        goto error;
    }

error:
    UnloadSample (TempSample);
    *resultSamplerName = InstrumentName;
    return Result;
}

/****************************************************************/
Err e3InitMusic( void )
{
	int32 Result;
/* Initialize audio, return if error. */
	Result = OpenAudioFolio();
	CHECKRESULT (Result, "open audio folio");
error:
	return Result;
}
/****************************************************************/
Err e3TermMusic( void )
{	int32 Result;
/* Close audio, return if error. */
	Result = CloseAudioFolio();
	CHECKRESULT (Result, "close audio folio");
error:
	return Result;
}

/****************************************************************/
/* Start music which will play continuously. */
Err e3StartMusic( char *FileName )
{
/* do the demo */
	return SpoolSoundFile ( FileName, 1000000 );
}

Err e3SetMusicAmplitude( int32 Amplitude )
{
	int32 Result;
	return TweakRawKnob( gMusicAmplitudeKnob, Amplitude );
}

/****************************************************************/
Err e3StopMusic( void )
{
/* !!! Should send signal to tell thread to stop playing. But just DEMO! */
	DeleteThread( gSpoolerThread );
	return 0;
}
#else
/****************************************************************/
Err e3InitMusic( void )
{
	int32 Result;
/* Initialize audio, return if error. */
	Result = OpenAudioFolio();
	CHECKRESULT (Result, "open audio folio");
error:
	return Result;
}
/****************************************************************/
Err e3TermMusic( void )
{	int32 Result;
/* Close audio, return if error. */
	Result = CloseAudioFolio();
	CHECKRESULT (Result, "close audio folio");
error:
	return Result;
}

/****************************************************************/
/* Control sound of ultralight engine. */
Err e3LoadMusic( char *MusicName )
{
	Item Attachment;
	int32 Result;
	char *InstrumentName;
	Item SampleItem;

/* Load digital audio AIFF Sample file from disk. */
	SampleItem = LoadSample(MusicName);
	CHECKRESULT(SampleItem,"LoadSample");

/* find a suitable instrument */
	InstrumentName = SelectSamplePlayer (SampleItem, FALSE);
	if (InstrumentName == NULL)
	{
		ERR(("No instrument to play that sample.\n"));
		Result = ML_ERR_UNSUPPORTED_SAMPLE;
		goto error;
	}
	Result = LoadSingleSound( SampleItem, InstrumentName, &gMusicSound );
	CHECKRESULT(Result,"e3LoadMusic: LoadSingleSound");

error:
	return Result;
}
/****************************************************************/
/* Start music which will play continuously. */
Err e3StartMusic( int32 Amplitude )
{
	return StartSingleSound( gMusicSound, Amplitude, 0 );
}

Err e3SetMusicAmplitude( int32 Amplitude )
{
	return ControlSingleSound( gMusicSound, Amplitude, 0 );
}
Err e3StopMusic( void )
{
	return StopSingleSound( gMusicSound );
}
#endif

/****************************************************************/
/* Control sound of ultralight engine. */
Err e3LoadEngineSound( char *SampleName )
{
	Item Attachment;
	int32 Result;
	char *InstrumentName;
	Item SampleItem;

/* Load digital audio AIFF Sample file from disk. */
	SampleItem = LoadSample(SampleName);
	CHECKRESULT(SampleItem,"LoadSample");

	Result = LoadSingleSound( SampleItem, "sampler.dsp", &gEngineSound );
	CHECKRESULT(Result,"e3LoadEngineSound: LoadSingleSound");

	return Result;

error:
	e3StopEngineSound( );
	return Result;
}

Err e3StartEngineSound( int32 Amplitude, int32 Frequency )
{
	return StartSingleSound( gEngineSound, Amplitude, Frequency );
}
Err e3ControlEngineSound( int32 Amplitude, int32 Frequency )
{
	return ControlSingleSound( gEngineSound, Amplitude, Frequency );
}
Err e3StopEngineSound( void )
{
	return StopSingleSound( gEngineSound );
}
/****************************************************************/
/* Control sound of ultralight engine. */
Err e3StartWindSound( int32 Amplitude, int32 Frequency )
{
	int32 Result;

/* Load a directout instrument to send the sound to the DAC. */
	gWindOutputIns = LoadInstrument("directout.dsp", 0, 100);
	CHECKRESULT(gWindOutputIns,"LoadInstrument");
	StartInstrument( gWindOutputIns, NULL );

/* Load Wind instrument */
	gWindIns = LoadInstrument("filterednoise.dsp",  0, 100);
	CHECKRESULT(gWindIns,"LoadInstrument");

/* Load Red Noise generator for natural feel. */
	gWindRedNoise = LoadInstrument("rednoise.dsp",  0, 100);
	CHECKRESULT(gWindIns,"LoadInstrument");
	gWindTimesPlus = LoadInstrument("timesplus.dsp",  0, 100);
	CHECKRESULT(gWindIns,"LoadInstrument");

/* Add Red Noise to Wind Frequency using TimesPlus */
	Result = ConnectInstruments (gWindRedNoise, "Output", gWindTimesPlus, "InputA");
	CHECKRESULT(Result,"ConnectInstruments");
	gWindModDepthKnob = GrabKnob( gWindTimesPlus, "InputB" );
	CHECKRESULT(gWindModDepthKnob, "GrabKnob");
	gWindFrequencyKnob = GrabKnob( gWindTimesPlus, "InputC" );
	CHECKRESULT(gWindFrequencyKnob, "GrabKnob");
	Result = ConnectInstruments (gWindTimesPlus, "Output", gWindIns, "Frequency");
	CHECKRESULT(Result,"ConnectInstruments");


/* Grab knobs for continuous control. */
	gWindAmplitudeKnob = GrabKnob( gWindIns, "Amplitude" );
	CHECKRESULT(gWindAmplitudeKnob, "GrabKnob");
	gWindResonanceKnob = GrabKnob( gWindIns, "Resonance" );
	CHECKRESULT(gWindResonanceKnob, "GrabKnob");
	gWindModRateKnob = GrabKnob( gWindRedNoise, "Frequency" );
	CHECKRESULT(gWindModRateKnob, "GrabKnob");

/* Connect Sampler Instrument to DirectOut. Works for mono or stereo. */
	Result = ConnectInstruments (gWindIns, "Output", gWindOutputIns, "InputLeft");
	CHECKRESULT(Result,"ConnectInstruments");
	Result = ConnectInstruments (gWindIns, "Output", gWindOutputIns, "InputRight");
	CHECKRESULT(Result,"ConnectInstruments");

	Result = TweakRawKnob( gWindResonanceKnob, 0x200 );
	CHECKRESULT(Result,"TweakRawKnob");

	Result = StartInstrument (gWindRedNoise, NULL);
	CHECKRESULT(Result,"StartInstrument");
	Result = StartInstrument (gWindTimesPlus, NULL);
	CHECKRESULT(Result,"StartInstrument");

	Result = StartInstrumentVA (gWindIns,
	                            AF_TAG_AMPLITUDE, Amplitude,
	                            AF_TAG_FREQUENCY, Frequency,
	                            TAG_END);
	CHECKRESULT(Result,"StartInstrumentVA");


	return Result;

error:
	e3StopWindSound( );
	return Result;
}


/****************************************************************/
Err e3ControlWindSound( int32 Amplitude, int32 Frequency )
{
	int32 Result;

	Result = TweakRawKnob( gWindAmplitudeKnob, Amplitude );
	CHECKRESULT(Result,"TweakRawKnob");
DBUG(("Wind Frequency = 0x%x\n", Frequency ));
	Result = TweakRawKnob( gWindFrequencyKnob, Frequency );
	CHECKRESULT(Result,"TweakRawKnob");
	Result = TweakRawKnob( gWindModDepthKnob, Frequency/4 );
	CHECKRESULT(Result,"TweakRawKnob");
	Result = TweakRawKnob( gWindModRateKnob, Frequency/16 );
	CHECKRESULT(Result,"TweakRawKnob");

#if 0
	{
		int32 Val;
		Item myProbe = CreateProbe( gWindTimesPlus, "Output", NULL );
		Result = ReadProbe( myProbe, &Val );
		CHECKRESULT(Result,"ReadProbe");
		PRT(("Probe = =x%x\n", Val ));
		DeleteProbe( myProbe );
	}
#endif

error:
	return Result;
}

/****************************************************************/
Err e3StopWindSound( void )
{
	StopInstrument( gWindIns, NULL );
	UnloadInstrument( gWindOutputIns );
	return 0;
}

/****************************************************************/
	
/****************************************************************/
/* Control sound of ultralight engine. */
Err e3LoadCannonSound( char *SampleName )
{
	Item Attachment;
	int32 Result;
	char *InstrumentName;
	Item SampleItem;

/* Load digital audio AIFF Sample file from disk. */
	SampleItem = LoadSample(SampleName);
	CHECKRESULT(SampleItem,"LoadSample");

	Result = LoadSingleSound( SampleItem, "sampler.dsp", &gCannonSound );
	CHECKRESULT(Result,"e3LoadCannonSound: LoadSingleSound");

	return Result;

error:
	e3StopCannonSound( );
	return Result;
}

Err e3StartCannonSound( int32 Amplitude, int32 Frequency )
{
	return StartSingleSound( gCannonSound, Amplitude, Frequency );
}
Err e3ControlCannonSound( int32 Amplitude, int32 Frequency )
{
	return ControlSingleSound( gCannonSound, Amplitude, Frequency );
}
Err e3StopCannonSound( void )
{
	return StopSingleSound( gCannonSound );
}
