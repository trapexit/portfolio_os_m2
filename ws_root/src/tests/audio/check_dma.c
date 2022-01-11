/* @(#) check_dma.c 96/07/29 1.10 */
/***************************************************************
**
** Check DMA by running a several channels a long time and
** validating DMA.
** Capture the output of a DSP instrument in a delay line.
** Look for the first non-zero sample, then checksum the next N samples.
** Compare against expected checksum and report deviations.
** Optionally display signal using character graphics.
**
** See PrintHelp() routine below for more info.
**
** By:  Phil Burk
**
** Copyright (c) 1994, 3DO Company.
** This program is proprietary and confidential.
**
***************************************************************/

#include <kernel/operror.h>
#include <audio/audio.h>
#include <stdio.h>


#define DEFAULT_INS_NAME "sampler_raw_f1.dsp"
#define DEFAULT_SAMPLE_NAME          (NULL)
#define DEFAULT_NUM_FRAMES           (2*1024)
#define DEFAULT_NUM_CHANNELS         (12)
#define MAX_CHANNELS                 (16)
#define DEFAULT_FRAMES_PREROLL       (100)
#define REPORT_PERIOD                (240*4)

#define USE_ALIGNED_BLOCKS           (1)
#define BANG_INDEX                   (0)
#define BANG_WHILE_WAITING           (1)
#define BANG_AFTER_WAITING           (0)
#define SLEEP_BEFORE_WAITING         (0)
#define LOG_BUG_EVENTS               (1)
#define USE_RANDOM_DELAY             (1)
#define RANDOM_DELAY_MASK         (0xFFFF)
#define NUM_BANGS                  (1000)
#define SLEEP_DURATION              (64)

#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */

/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		PrintError(0,"\\failure in",name,Result); \
		goto cleanup; \
	}

#include "channel_tester.h"

static uint32 gTestCount = 0;
static uint32 gScratch;  /* Write to this so compiler doesn't optimise away dummy loops. */

#if (LOG_BUG_EVENTS == 1)   /* { */

/* Log events like in a logic analyser. */
typedef struct BugEvent
{
	uint32   buge_Time;
	char    *buge_Type;
	uint32   buge_Data;
	uint32   buge_TestCount;
} BugEvent;

#define MAX_BUG_EVENTS   (1024)
static BugEvent EventLog[MAX_BUG_EVENTS];
static int32 BugEventIndex = 0;
static int32 BugEventsLogged = 0;
static void LogBugEvent( char * Type, int32 Data )
{
	BugEvent *buge = &EventLog[BugEventIndex++];
	buge->buge_Time = GetAudioTime();
	buge->buge_Type = Type;
	buge->buge_Data = Data;
	buge->buge_TestCount = gTestCount;
	if( BugEventIndex >= MAX_BUG_EVENTS ) BugEventIndex = 0;
	BugEventsLogged++;
}

static void ReportBugEvents( int32 NumEvents )
{
	int32 nr, i, bi;
	BugEvent *buge;

PRT(("ReportBugEvents\n"));
	nr = NumEvents;
	if( nr > BugEventsLogged ) nr = BugEventsLogged;
	if( nr > MAX_BUG_EVENTS ) nr = MAX_BUG_EVENTS;

	bi = BugEventIndex - nr;
	if( bi < 0 ) bi += MAX_BUG_EVENTS;
	for( i=0; i<nr; i++ )
	{
		buge = &EventLog[bi++];
		PRT(("Event: Time = %8d, T# = %d, Type = %s, Data = 0x%x\n",
			buge->buge_Time,
			buge->buge_TestCount,
			buge->buge_Type, buge->buge_Data ));
		if( bi >= MAX_BUG_EVENTS ) bi = 0;
	}
}
static char TYPE_START[]   = "Start";
static char TYPE_SIGNALS[] = "Signals";
#if SLEEP_BEFORE_WAITING == 1
static char TYPE_SLEEP[]   = "Sleep";
#endif

#else   /* } { */

#define ReportBugEvents(NumEvents)  PRT(("Bug Event Logging not compiled.\n"));
#define LogBugEvent(Type,Data) /* Never mind. */

#endif  /* } */

/*********************************************************************/
void PrintHelp( void )
{
	printf("Usage: capture_ins {options}\n");
	printf("   -nNumFrames     = Number of Sample Frames to Analyse (default = %d)\n", DEFAULT_NUM_FRAMES);
	printf("   -cNumChannels   = Number of Channels (default = %d)\n", DEFAULT_NUM_CHANNELS);
	printf("   -pPreRoll       = Number of Frames to PreRoll (default = %d)\n", DEFAULT_FRAMES_PREROLL);
}

#if 0
/***********************************************************************/
Err WriteDataToMac( char *DataPtr, int32 numBytes, char *FileName )
{
	FILE *fid;
	int32 Result = 0;
	int32 NumWritten;

	fid = fopen( FileName, "w" );
	if (fid == NULL)
	{
		printf("Could not open %s\n", FileName);
		return -1;
	}

	NumWritten = fwrite( DataPtr, 1, numBytes, fid );
	if(NumWritten != numBytes)
	{
		ERR(("Error writing file.\n"));
		Result = -2;
		goto cleanup;
	}

cleanup:
	if (fid) fclose( fid );
	return Result;
}
#endif

/**********************************************************************
** Delete structure.
**********************************************************************/
Err CheckDMA( int32 numFrames, int32 numChannels, int32 numPreroll )
{
	int32 Result;
	int32 i;
	ChannelTester *Testers[MAX_CHANNELS];
	int32 SignalMask = 0, Signals;
	int32 errorCount = 0;
	AudioTime  lastTime;
	Item SleepCue;

	SleepCue = CreateCue(NULL);
	CHECKRESULT(SleepCue, "CreateCue");


/* Create Testers. */
	if( numChannels > MAX_CHANNELS )
	{
		ERR(("CheckDMA: can't have more than %d channels!\n", MAX_CHANNELS ));
		return -1;
	}
#if (USE_ALIGNED_BLOCKS == 1)
	Result =  CreateChannelTesters( numFrames, numPreroll, numChannels, &Testers[0] );
	CHECKRESULT(Result, "CreateChannelTesters");
#else
	for( i=0; i<numChannels; i++ )
	{
		Result =  CreateChannelTester( numFrames, numPreroll, i, NULL, &Testers[i] );
		CHECKRESULT(Result, "CreateChannelTester");
	}
#endif
/* Fire them off with staggered timing. */
	for( i=0; i<numChannels; i++ )
	{
		if( i != BANG_INDEX )
		{
			Result =  StartChannelTester( Testers[i] );
			CHECKRESULT(Result, "StartChannelTester");
			SignalMask |= Testers[i]->chnt_Signal;
		}
	}

/* Loop on signals returned. */
	lastTime = GetAudioTime();
PRT(("Wait for first signals to come back = 0x%x\n", SignalMask));
	while(1)
	{
#if (BANG_WHILE_WAITING == 1)
	#if 1
		Signals = WaitSignal( SignalMask );  /* Wait for first channel to finish. */
		LogBugEvent(TYPE_SIGNALS,Signals);
		LogBugEvent(TYPE_START,BANG_INDEX);
		for( i=0; i<NUM_BANGS; i++ )
		{
/* Hammer FDMA stack. */
			Result =  StartChannelTester( Testers[BANG_INDEX] );
			CHECKRESULT(Result, "StartChannelTester");
		}
		LogBugEvent(TYPE_START,BANG_INDEX);
		if( (SignalMask & ~Signals) != 0 ) Signals |= WaitSignal( SignalMask );
		LogBugEvent(TYPE_SIGNALS,Signals);

	#else
		do
		{
			LogBugEvent(TYPE_START,BANG_INDEX);
			for( i=0; i<NUM_BANGS; i++ )
			{
/* Hammer FDMA stack. */
				Result =  StartChannelTester( Testers[BANG_INDEX] );
				CHECKRESULT(Result, "StartChannelTester");
			}
			LogBugEvent(TYPE_START,BANG_INDEX);
		}while ((Signals = GetCurrentSignals() & SignalMask) == 0);
		WaitSignal( Signals );  /* Clear before next time. */
		LogBugEvent(TYPE_SIGNALS,Signals);
	#endif
#elif (SLEEP_BEFORE_WAITING == 1)

		LogBugEvent(TYPE_SLEEP, SLEEP_DURATION );
		Result = SleepUntilTime( SleepCue, GetAudioTime() + SLEEP_DURATION );
		CHECKRESULT(Result, "SleepUntilTime");

		Signals = WaitSignal( SignalMask );
		LogBugEvent(TYPE_SIGNALS,Signals);

		LogBugEvent(TYPE_START,BANG_INDEX);
		Result =  StartChannelTester( Testers[BANG_INDEX] );
		CHECKRESULT(Result, "StartChannelTester");

#else
		Signals = WaitSignal( SignalMask );
		LogBugEvent(TYPE_SIGNALS,Signals);
#endif


#if (BANG_AFTER_WAITING == 1)
		LogBugEvent(TYPE_START,BANG_INDEX);
		for( i=0; i<1000; i++ )
		{
			Result =  StartChannelTester( Testers[BANG_INDEX] );
			CHECKRESULT(Result, "StartChannelTester");
		}
		LogBugEvent(TYPE_START,BANG_INDEX);
#endif

DBUG(("Got signals back = 0x%x\n", Signals));

		for( i=0; i<numChannels; i++ )
		{
			if( (i != BANG_INDEX) && (Signals & Testers[i]->chnt_Signal) )
			{
				if(CheckChannelTester(Testers[i]) < 0)
				{
					errorCount++;
					ReportBugEvents(20);
				}
				gTestCount++;
				Result =  StartChannelTester( Testers[i] );
				CHECKRESULT(Result, "StartChannelTester");
				LogBugEvent(TYPE_START,i);
			}
#if (USE_RANDOM_DELAY == 1)
			{
				int32 li, nloops, sum=0;
				nloops = rand() & RANDOM_DELAY_MASK;
				for( li=0; li<nloops; li++ ) sum+= li;
				gScratch = sum;
			}
#endif
		}
		if( AudioTimeLaterThan(GetAudioTime(),(lastTime + REPORT_PERIOD)) )
		{
			PRT(("CheckDMA: #tests = %d, #errors = %d\n", gTestCount, errorCount ));
			lastTime = GetAudioTime();
		}
	}

cleanup:
/* Tear them down. */
	for( i=0; i<numChannels; i++ )
	{
		DeleteChannelTester( Testers[i] );
	}
	return Result;
}
/***********************************************************************/
int main(int argc, char *argv[])
{
	int32 numFrames;
	int32 numChannels;
	int32 numPreroll;
	char *s, c;
	int i;
	int32 Result;

	numFrames = DEFAULT_NUM_FRAMES;
	numPreroll = DEFAULT_FRAMES_PREROLL;
	numChannels = DEFAULT_NUM_CHANNELS;

/* Get input parameters. */
	for( i=1; i<argc; i++ )
	{
		s = argv[i];

		if( *s++ == '-' )
		{
			c = *s++;
			switch(c)
			{
			case 'n':
				numFrames = atoi(s);
				break;
			case 'c':
				numChannels = atoi(s);
				break;
			case 'p':
				numPreroll = atoi(s);
				break;
			case '?':
			default:
				PrintHelp();
				exit(1);
				break;
			}
		}
		else
		{
			PrintHelp();
			exit(1);
		}
	}


/* Initialize audio, return if error. */
	PRT(("OPEN AUDIO!\n"));
	if ((Result = OpenAudioFolio()) < 0)
	{
		ERR(("Audio Folio could not be opened!\n"));
		return(Result);
	}

	DBUG(("Check_Dma: numFrames = %d, numChannels = %d, NumPreroll = %d\n",
		numFrames, numChannels, numPreroll ));
	Result = CheckDMA( numFrames, numChannels, numPreroll );

	CloseAudioFolio();
	return (int) Result;
}
