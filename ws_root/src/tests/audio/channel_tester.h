/* @(#) channel_tester.h 96/07/29 1.10 */
/***************************************************************
**
** Check read and write DMA.
**
** By:  Phil Burk
**
** Copyright (c) 1995, 3DO Company.
** This program is proprietary and confidential.
**
***************************************************************/

#include <audio/audio.h>
#include <kernel/operror.h>
#include <kernel/types.h>
#include <stdio.h>
#include <stdlib.h>     /* malloc() */
#include <string.h>     /* memset() */
#include <hardware/PPCasm.h>   /* _dcbf */

#include "capture_tool.h"

#ifndef PRT
	#define	PRT(x)	{ printf x; }
#endif
#ifndef ERR
	#define	ERR(x)	PRT(x)
#endif
#ifndef DBUG
	#define	DBUG(x)	/* PRT(x) */
#endif

#define MAX_ERRORS                  (32)
#define ALLOC_DELAY_FIRST           (0)
#define USE_RANDOM_OFFSET           (0)

#ifndef CHECKRESULT
/* Macro to simplify error checking. */
#define CHECKRESULT(val,name) \
	if (val < 0) \
	{ \
		Result = val; \
		PrintError(0,"\\failure in",name,Result); \
		goto cleanup; \
	}
#endif


typedef struct ChannelTester
{
	AudioCapture *chnt_acap;
	uint16 *chnt_Data;
	uint16  chnt_Flags;
	Item    chnt_SampleItem;
	Item    chnt_SampleAtt;
	Item    chnt_TestIns;
	int32   chnt_TestIndex;
	int32   chnt_NumTests;   /* How many times have we completed a test. */
	int32   chnt_NumFrames;
	int32   chnt_Offset;
	int32   chnt_NumFramesPreroll;
	int32   chnt_Signal;
} ChannelTester;

#define CHNT_F_DID_ALLOC   (0x0001)

Err CreateChannelTester( int32 numFrames, int32 numPreroll, int32 testIndex,
	uint16 *SourceData, ChannelTester **chntPtr );
Err StartChannelTester( ChannelTester *chnt );
Err CheckChannelTester( ChannelTester *chnt );
void DeleteChannelTester( ChannelTester *chnt );

static uint32   TriggerArray[256];   /* Use array so we can isolate cache writeback. */
static uint32  *TriggerVarPtr;  /* Writes to this trigger scope. */

static uint16 *gAllocData = NULL;

#define MakeSourceData(ti,si) ( ACAP_VALID_DATA_MARKER | ( ( ((ti)+1) & 0xF ) << 8 ) | ((si) & 0xFF) )
#define MakeDestData(ti,si) ( ACAP_VALID_DATA_MARKER  | ( ( ((ti)+1) & 0xF ) << 8 ) | ((si) & 0xFF) )

#define CACHE_LINE_SIZE  (32)
/*********************************************************************/
static MyCacheFlush( char *StartAddr, int32 NumBytes )
{
	char *CacheAddr;
	int32 i, NumLoops;

	NumLoops = (NumBytes / CACHE_LINE_SIZE) + 2;
	CacheAddr = (char *) (((uint32)StartAddr) & (~(CACHE_LINE_SIZE-1)));
DBUG(("StartAddr = 0x%x, CacheAddr = 0x%x\n", StartAddr, CacheAddr ));
	for( i=0; i<NumLoops; i++ )
	{
		_dcbf( CacheAddr );
		CacheAddr += CACHE_LINE_SIZE;
	}
	_sync();
}

/**********************************************************************
** Create structure to track DMA on one channel.
**********************************************************************/
Err CreateChannelTester( int32 numFrames, int32 numPreroll, int32 testIndex,
	uint16 *SourceData, ChannelTester **chntPtr )
{
/* Declare local variables */
	int32   Result;
	int32   i;
	int32   delayFrames;
	ChannelTester *chnt;

	chnt = (ChannelTester *) calloc( sizeof(ChannelTester), 1 );
	if( chnt == NULL )
	{
		Result = -1;
		goto cleanup;
	}

#if( ALLOC_DELAY_FIRST == 1 )
	delayFrames = numFrames + numPreroll;  /* Give time for instrument to fire up. */
	Result = acapCreate( delayFrames, 1, &chnt->chnt_acap );
	CHECKRESULT(Result,"acapCreate");
#endif

/* Load instrument to be tested. */
	chnt->chnt_TestIns = LoadInstrument( DEFAULT_INS_NAME,  0, 100);
	CHECKRESULT(chnt->chnt_TestIns,"LoadInstrument");

/* Create audio AIFF Sample. */
	if( SourceData == NULL )
	{
		chnt->chnt_Data = (uint16 *) malloc( numFrames*2 );
		if( chnt->chnt_Data == NULL )
		{
			Result = -1;
			goto cleanup;
		}
		chnt->chnt_Flags |= CHNT_F_DID_ALLOC;
	}
	else
	{
		chnt->chnt_Data = SourceData;
	}

/* Fill in data with recognizable pattern. */
	for( i=0; i<numFrames; i++ )
	{
		chnt->chnt_Data[i] = MakeSourceData(testIndex,i);
	}
	chnt->chnt_SampleItem = CreateSampleVA(
		AF_TAG_ADDRESS, (TagData) chnt->chnt_Data,
		AF_TAG_FRAMES, (TagData) numFrames,
		TAG_END );
	CHECKRESULT(chnt->chnt_SampleItem,"CreateSample");

	chnt->chnt_NumFrames = numFrames;
	chnt->chnt_NumFramesPreroll = numPreroll;
	chnt->chnt_TestIndex = testIndex;

/* Attach the sample to the instrument for playback. */
	chnt->chnt_SampleAtt = CreateAttachment(chnt->chnt_TestIns, chnt->chnt_SampleItem, NULL);
	CHECKRESULT(chnt->chnt_SampleAtt,"CreateAttachment");

#if( ALLOC_DELAY_FIRST == 0 )
	delayFrames = numFrames + numPreroll;  /* Give time for instrument to fire up. */
	Result = acapCreate( delayFrames, 1, &chnt->chnt_acap );
	CHECKRESULT(Result,"acapCreate");
#endif

	Result = acapConnect( chnt->chnt_acap, chnt->chnt_TestIns );
	CHECKRESULT(Result,"acapConnect");

	chnt->chnt_Signal = chnt->chnt_acap->acap_CueSignal;

	PRT(("Test# %d, 0x%x frames\n   Original Data at 0x%x\n", testIndex, numFrames, chnt->chnt_Data));
	PRT(("   Captured Data at 0x%x\n", chnt->chnt_acap->acap_DelayData));

	MyCacheFlush( (char *) chnt->chnt_Data, 2 * chnt->chnt_NumFrames );
	MyCacheFlush( (char *) chnt->chnt_acap->acap_DelayData, 2 * (chnt->chnt_NumFrames + chnt->chnt_NumFramesPreroll) );


	TriggerVarPtr = (vuint32 *) ( ((uint32) &TriggerArray[128]) & ~(CACHE_LINE_SIZE-1) );

	*chntPtr = chnt;
	return Result;

cleanup:
	if(chnt) DeleteChannelTester( chnt );
	return Result;
}


/**********************************************************************
** Delete a set of Channel testers that use aligned DMA blocks.
**********************************************************************/
void DeleteChannelTesters( int32 numSets, ChannelTester **chntPtr )
{
	int32  i;
/* Tear them down. */
	for( i=0; i<numSets; i++ )
	{
		DeleteChannelTester( chntPtr[i] );
	}
	if( gAllocData )
	{
		free( gAllocData );
		gAllocData = NULL;
	}
}
/**********************************************************************
** Create a set of Channel testers that use aligned DMA blocks.
**********************************************************************/
Err CreateChannelTesters( int32 numFrames, int32 numPreroll, int32 numSets, ChannelTester *chntPtr[] )
{
	int32  i, n, Result = 0;
	int32  numToAlloc, numOnes, numBytes;
	uint16 *alignedData;

/* Check to make sure numFrames is a power of 2 by counting 1's bits. */
	n = numFrames;
	numOnes = 0;
	do
	{
		if( (n&1) == 1 ) numOnes++;
		n = n>>1;
	} while ( n > 0 );
	if( numOnes != 1 )
	{
		ERR(("CreateChannelTesters: numFrames must be power of 2, not 0x%x\n", numFrames ));
		return -1;
	}

/* Figure out how many bytes to allocate. */
	numBytes = numFrames * 2;
	numToAlloc = numBytes * (numSets + 1);
	gAllocData = (uint16 *) malloc( numToAlloc );
	if( gAllocData == NULL )
	{
		Result = -1;
		goto cleanup;
	}
/* Get aligned address. */
	alignedData = (uint16 *) ( (((uint32)gAllocData) + (numBytes-1)) & ~(numBytes-1));
PRT(("CreateChannelTesters: numFrames = %d = 0x%x\n", numFrames, numFrames ));
PRT(("CreateChannelTesters: numBytes = 0x%x, gAllocData = 0x%x, alignedData = 0x%x\n",
	numBytes, gAllocData, alignedData ));

/* Create individual testers using aligned blocks. */
	for( i=0; i<numSets; i++ )
	{

		Result = CreateChannelTester( numFrames, numPreroll, i,
			alignedData + (i*numFrames), &chntPtr[i] );
		CHECKRESULT(Result,"CreateChannelTester");
	}

	return Result;
cleanup:
	DeleteChannelTesters( numSets, chntPtr );
	return Result;
}

/**********************************************************************
** Start capture.
**********************************************************************/
Err StartChannelTester( ChannelTester *chnt )
{
	int32 Result;
#if (USE_RANDOM_OFFSET == 1)
/* Select a random offset into the data. */
	chnt->chnt_Offset = ((rand() & 0xFFFF) * (chnt->chnt_NumFrames/4)) / 0x10000;
DBUG(("Offset = %d\n", chnt->chnt_Offset));

/* Make Attachment start at that offset. */
	Result = SetAudioItemInfoVA( chnt->chnt_SampleAtt, AF_TAG_START_AT, chnt->chnt_Offset, TAG_END );
	CHECKRESULT(Result,"SetAudioItemInfoVA");
#endif

/* Start capturing sound. */
	Result = acapStart( chnt->chnt_acap );
	CHECKRESULT(Result,"acapStart");

/* Immediately start playing sample. */
	Result = StartInstrument( chnt->chnt_TestIns, NULL );
	CHECKRESULT(Result,"StartInstrument");
cleanup:
	return Result;
}
/**********************************************************************
** Analyse result.
**********************************************************************/
Err CheckChannelTester( ChannelTester *chnt )
{
	int32     firstNonZero;
	int32     Result=0;
	int32     i, si;
	int32     errorCount = 0;
	int32     errorIndex = 0;
	register uint32    oldData, calcSrcData, calcDstData, newData;
	int32     ifReportedYet = FALSE;

/* FlushDCache() before reading from captured data */
	acapFlushDCache (chnt->chnt_acap);

/* Scan for first non-zero data. */
	firstNonZero = acapFirstNonZero( chnt->chnt_acap );
DBUG(("CheckChannelTester: firstNonZero = %d\n", firstNonZero ));

	if( firstNonZero < 0 )
	{
		ERR(("CheckChannelTester: all data zero!\n"));
		Result = -1;
	}
	else if( firstNonZero > chnt->chnt_NumFramesPreroll )
	{
		ERR(("CheckChannelTester: did not capture full signal! Insufficient pre-roll.\n"));
		Result = -2;
	}
	else if( firstNonZero == 0 )
	{
		ERR(("CheckChannelTester: warning, first sample is non-zero.\n"));
		ERR(("            May have missed some data.\n"));
		Result = -2;
	} else
	{
/* Compare original data to captured data. */
		for( i=0; i<(chnt->chnt_NumFrames - chnt->chnt_Offset); i++ )
		{
			si = chnt->chnt_Offset + i;
			oldData = chnt->chnt_Data[si];
			calcSrcData = MakeSourceData(chnt->chnt_TestIndex, (si) );
			calcDstData = MakeDestData(chnt->chnt_TestIndex, (si) );
			newData = chnt->chnt_acap->acap_DelayData[firstNonZero + i];
DBUG(("Original = 0x%x, Captured = 0x%x\n", oldData, newData ));
			if( (newData != calcDstData) ||
			    (oldData != calcSrcData) )
			{
				if( !ifReportedYet )
				{
/* Trigger scope. */
					*TriggerVarPtr = 0xABCD1234;
					MyCacheFlush( (char *) TriggerVarPtr, 4 );

					ERR(("ERROR test# %d, Offset = 0x%x, FirstnonZero = 0x%x\n",
						chnt->chnt_TestIndex, chnt->chnt_Offset, firstNonZero ));
					ifReportedYet = TRUE;
				}
				ERR(("ERROR     samp# = 0x%04x, Original = 0x%x, Calculated = 0x%x, Captured = 0x%x",
					i, oldData, calcSrcData, newData ));
				ERR((" at  0x%x\n", &chnt->chnt_acap->acap_DelayData[firstNonZero + i] ));

				Result = -3;
				if( errorCount++ > MAX_ERRORS )
				{
					ERR(("Maximum num errors exceeded.\n"));
					break;
				}
				if( errorCount == 1 )
				{
					errorIndex = i;
				}
			}
		}
	}
	chnt->chnt_NumTests++;

	if( errorCount > 0 )
	{
		PRT(("    TriggerVarPtr = 0x%x, This channel test %d times.\n", TriggerVarPtr, chnt->chnt_NumTests ));
		PRT(("    Original  Data at 0x%x\n", chnt->chnt_Data));
		PRT(("    Captured  Data at 0x%x\n", chnt->chnt_acap->acap_DelayData));
		PRT(("    NonZero   Data at 0x%x\n", &chnt->chnt_acap->acap_DelayData[firstNonZero] ));
		PRT(("    First Bad Data at 0x%x\n", &chnt->chnt_acap->acap_DelayData[firstNonZero + errorIndex]));
		/* PRT(("HANG for debug!\n")); while(1); */
		/* PRT(("BAIL OUT!!\n")); exit(1); */
	}
	else
	{
		DBUG(("CheckChannelTester #%d succeeded!\n", chnt->chnt_TestIndex));
	}

	DBUG(("CheckChannelTester calls MyCacheFlush\n"));
	MyCacheFlush( (char *) chnt->chnt_acap->acap_DelayData, 2 * (chnt->chnt_NumFrames + chnt->chnt_NumFramesPreroll) );


	return Result;
}
/**********************************************************************
** Delete structure.
**********************************************************************/
void DeleteChannelTester( ChannelTester *chnt )
{
	if( chnt == NULL ) return;
	if(chnt->chnt_Data && (chnt->chnt_Flags & CHNT_F_DID_ALLOC))
	{
		free(chnt->chnt_Data);
	}
	DeleteAttachment( chnt->chnt_SampleAtt );
	UnloadInstrument( chnt->chnt_TestIns );
	DeleteSample( chnt->chnt_SampleItem );
	acapDelete( chnt->chnt_acap );
	free( chnt );
}
