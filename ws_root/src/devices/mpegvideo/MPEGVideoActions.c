/* @(#) MPEGVideoActions.c 96/12/11 1.32 */
/* file: MPEGVideoActions.c */
/* routines to execute as MPEG video stream is parsed */
/* 10/4/94 George Mitsuoka */
/* The 3DO Company Copyright © 1994 */

#include <stdio.h>
#include <stdlib.h>
#include <kernel/types.h>
#include <kernel/kernel.h>
#include <kernel/mem.h>
#include <kernel/super.h>
#include <kernel/debug.h>
#include "M2MPEGUnit.h"
#include "MPEGStream.h"
#include "MPEGVideoActions.h"
#include "MPEGVideoParse.h"

#include "mpVideoDriver.h"

#ifdef DEBUG_PRINT_ACTIONS
  #define DBGACTION(a)		PRNT(a)
#else
  #define DBGACTION(a)		(void)0
#endif

static int32 HandleInterrupts(tVideoDeviceContext* theUnit, MPEGStreamInfo *si,
							  int32 signals);

/*	this code is segmented into two sections by the following #ifdef,
	the first section is the 'real' M2 driver code and the second
	section is used for standalone testing of the parser */

#ifndef STANDALONE

static int32 HandleInterrupts(tVideoDeviceContext* theUnit, MPEGStreamInfo *si,
							  int32 signals)
{
	int32 result = 0L, len;
	uint8 *addr;

	if( signals & theUnit->signalVideoBitstreamDMA )
	{
		/* video bitstream DMA completed, get next buffer */
		if( (result = MPNextDMA(theUnit, &(si->streamID), &addr, &len )) != 0L)
			return( result );
		/* before starting the hardware, check the quant matrix. */
		if (!si->quantMatrixOK)
			DoVCDOptimize(si->vcdInfo, si, addr, len);
		M2MPGContinue( addr, len );
		result = 0L;
	}
	if( signals & theUnit->signalEverythingDone )
	{
		uint8 buf[ LOOKAHEADSIZE ], *dmaAddr;
		int32 count;

		M2MPGEndDMA( buf, &count, &dmaAddr );
/*		DBGACTION(("Calling MPEndDMA\n")); */
		MPEndDMA(theUnit, &(si->streamID),  buf, count, dmaAddr);
/*		DBGACTION(("Returned from MPEndDMA\n")); */
		result = 1L;
	}
	if( signals & theUnit->signalOutputFormatter )
		;
	if( signals & theUnit->signalEndOfPicture )
		;
	if( signals & theUnit->signalOutputDMA )
		;
	if( signals & theUnit->signalBitstreamError )
	{
/*		printf("got signalBitstreamError\n"); */
		M2MPGDumpState();
		result = MPVFLUSHREAD;
	}
	if( signals & theUnit->signalStripBufferError )
	{
/*		printf("got signalStripBufferError\n"); */
		M2MPGDumpState();
		result = 1L;
	}

	return( result );
}

#define HALFSECOND (500000L)

int32 DoSlice(tVideoDeviceContext *theUnit, MPEGStreamInfo *si)
{
	uint8 *addr;
	int32 status, len, abort = 0L;

#ifdef TEST_ONLY
	printf("sz %3d %3d pr %2d br %5d bs %5d cgop %1d pic %3d pt %1d tr %2d\n",
		   SEQ_WIDTH(si),SEQ_HEIGHT(si),
		   si->seqHeader.pictureRate,si->seqHeader.bitRate,
		   si->seqHeader.vbvBufferSize,
		   si->gopHeader.closedGOP,
		   si->pictureNumber,
		   si->pictHeader.pictureCodingType,
		   si->pictHeader.temporalReference);
#else
	/* get a picture destination buffer */
	status = MPVNextWriteBuffer(theUnit, &(theUnit->destBuffer),
								&(theUnit->destBufferSize));
	switch( status )
	{
		case MPVFLUSHREAD:
			return( status );
		case ABORTED:
			/* ack! */
			DBGACTION(("mpegvideo DoSlice: Abort getting NextWriteBuffer\n"));
			return( status );
		default:
			break;
	}
	/* get next bitstream buffer to dma */
	if( (status = MPNextDMA(theUnit, &(si->streamID), &addr, &len)) != 0L)
		return( status );

	/* clear any leftover signals (is there a better way?) */
	ClearCurrentSignals( theUnit->signalMask );

	/* before starting the hardware, check the quant matrix. */
	if (!si->quantMatrixOK)
		DoVCDOptimize(si->vcdInfo, si, addr, len);

	/* start hardware slice decoder */
	M2MPGStartSlice(theUnit, si, addr, len);

	/* wait for a bitstream or picture completion interrupt */
	while( 1 )
	{
		int32 signals;

		/* turn on Metronome in case of timeout */
		if( theUnit->timerIOReqItem )
			StartMetronome( theUnit->timerIOReqItem,
							0L, HALFSECOND, theUnit->signalTimeout );

		/* wait for abort, interrupts, or timeout */
		signals = WaitSignal(theUnit->signalMask |
							 theUnit->signalReadAbort |
							 theUnit->signalWriteAbort |
							 theUnit->signalFlush |
							 theUnit->signalQuit);

		/* turn off Metronome */
		if( theUnit->timerIOReqItem )
			StopMetronome( theUnit->timerIOReqItem );

		if( signals & theUnit->signalTimeout )
		{
			DBGACTION(("mpegvideo DoSlice: timeout!\n"));
			return( MPVFLUSHWRITE );
		}	
		if ((signals & SIGF_ABORT) || (signals == ABORTED) ||
			(signals & theUnit->signalQuit))
		{
			/* how should we handle this? */
			DBGACTION(("mpegvideo DoSlice: Got ABORTED.\n"));
			M2MPGEndSlice();
			return( ABORTED );
		}
		if( signals & theUnit->signalFlush )
		{
			DBGACTION(("mpegvideo DoSlice: got signalFlush\n"));
			abort = MPVFLUSHDRIVER;
		}
		if( signals & theUnit->signalReadAbort )
		{
			DBGACTION(("mpegvideo DoSlice: got signalReadAbort\n"));
			abort = MPVFLUSHREAD;
		}
		if( signals & theUnit->signalWriteAbort )
		{
			DBGACTION(("mpegvideo DoSlice: got signalWriteAbort\n"));
			abort = MPVFLUSHWRITE;
		}
		/* if we got an abort and the decoder needs attention, can */
		/* safely return because the hardware is in an idle state */
		if( abort && ((signals & theUnit->signalEverythingDone) ||
					  (signals & theUnit->signalVideoBitstreamDMA) ||
					  (signals & theUnit->signalBitstreamError) ||
					  (signals & theUnit->signalStripBufferError)) )
		{
			M2MPGEndSlice();
			return( abort );
		}

		if( (status = HandleInterrupts(theUnit, si, signals)) == 1 )
			break;
		else if( status )
		{
			M2MPGEndSlice();
			return( status );
		}
	}
	M2MPGEndSlice();

	/* current pts is valid only for B pictures or if the unit is
	 * I frame only mode.
	 */
	if (PICT_TYPE(si) == 3 || si->parseIFrameOnly)
		MPVCompleteWrite(theUnit, 0, si->ptsValid, si->pts, si->userData);
	else
	{
		/* only output a reference picture if it has been decoded */
        if( theUnit->refPicFlags[ theUnit->nextRef ] & REFPICDECODED )
            MPVCompleteWrite(theUnit, 0, si->refPTSValid, si->refPTS, si->refUserData );
        theUnit->refPicFlags[ theUnit->nextRef ^ 1] |= REFPICDECODED;
        si->refPTSValid = si->ptsValid;
        si->refPTS = si->pts;
        si->refUserData = si->userData;
    }
#endif
	return( 0L );
}

int32 DoMPVideoExtensionData( MPEGStreamInfo *context, int32 data )
{
	TOUCH( context );
	TOUCH( data );

	return( 0L );
}

int32 DoMPVideoUserData( MPEGStreamInfo *context, int32 data )
{
	TOUCH( context );
	TOUCH( data );

	return( 0L );
}

#else	/* standalone test code */

/* the following test code is for debugging the mpeg parsers using
   the standard C libraries */

#define ERROR 0xffffffffL

static DumpQuantTable( MPEGStreamInfo *context, int32 offset, uint8 *table )
{
	int32 i;

	for( i = 0; i < 64; i++ )
	{
		fprintf( context->streamID.commandsFile,
				 "\tctlWrite (`MPG_REG_QTABLE_BASE + %d, 32'h%08lx);\n",
				 offset,(int32) table[ i ] );
		offset += 4;
	}
	fprintf( context->streamID.commandsFile,"\n" );
}

static DumpIntraQuantTable( MPEGStreamInfo *context )
{
	fprintf( context->streamID.commandsFile,
			 "\t/* intra quantization table */\n");

	DumpQuantTable( context, 0L, context->intraQuantMatrix );
}

static DumpNonIntraQuantTable(  MPEGStreamInfo *context )
{
	fprintf( context->streamID.commandsFile,
			 "\t/* non intra quantization table */\n");

	DumpQuantTable( context, 256L, context->nonIntraQuantMatrix );
}

#define REF0ADDRESS 0x20000L
#define REF1ADDRESS 0x40000L
#define BADDRESS	0x60000L

char pictureType[] = "xIPB";
char outFilePrefix[256];
uint32 pictureNumber = 0L;

static DumpCommands( MPEGStreamInfo *context )
{
	TImageSizeReg sizeReg;
	TParserConfigReg configReg;
	uint32 tempReg, size;
	uint32 outAddr, backAddr, forwardAddr;
	static uint32 nextRef = 0;

	fprintf( context->streamID.commandsFile,
			 "\n\t/* picture #%ld (%c) */\n",
			 pictureNumber,
			 pictureType[PICT_TYPE(context)] );

	tempReg = 0x00000001L;
	fprintf( context->streamID.commandsFile,
			 "\tctlWrite (`MPG_REG_CONFIG, 32'h%08lx);\n\n",
			 tempReg);

	DumpIntraQuantTable( context );
	DumpNonIntraQuantTable( context );

	*((uint32 *) &sizeReg) = 0L;
	sizeReg.mbHeight = (SEQ_HEIGHT(context) + 15) >> 4;
	sizeReg.mbWidth = (SEQ_WIDTH(context) + 15) >> 4;
	tempReg = *((uint32 *) &sizeReg);

	fprintf( context->streamID.commandsFile,
			 "\tctlWrite (`MPG_REG_IMAGESIZE, 32'h%08lx);\n",
			 tempReg);

	switch (PICT_TYPE(context))
	{
		case 1:	/* I picture */
		case 2:	/* P picture */
			if( !nextRef )
			{
				outAddr = REF0ADDRESS;
				forwardAddr = REF1ADDRESS;
			}
			else
			{
				outAddr = REF1ADDRESS;
				forwardAddr = REF0ADDRESS;
			}
			nextRef ^= 1L;
			break;
		case 3:	/* B picture */
			outAddr = BADDRESS;
			if( !nextRef )
			{
				forwardAddr = REF0ADDRESS;
				backAddr = REF1ADDRESS;
			}
			else
			{
				forwardAddr = REF1ADDRESS;
				backAddr = REF0ADDRESS;
			}
			break;
		default:
			fprintf( stderr,"BAD PICTURE TYPE\n");
			break;
	}
	switch (PICT_TYPE(context))
	{
		case 3: /* B */
			fprintf( context->streamID.commandsFile,
					 "\tctlWrite (`MPG_REG_REV_MOT, 32'h%08lx);\n",
					 backAddr );
		case 2: /* P */
			fprintf( context->streamID.commandsFile,
					 "\tctlWrite (`MPG_REG_FWD_MOT, 32'h%08lx);\n",
					 forwardAddr );
		case 1: /* I */
			fprintf( context->streamID.commandsFile,
					 "\tctlWrite (`MPG_REG_VOD_CNTL, 32'h%08lx);\n",
					 outAddr );
	}
	*((uint32 *) &configReg) = 0L;
	configReg.fullPelBackwardVector = context->fullPelBackwardVector;
	configReg.backwardRSize = context->backwardFCode - 1;
	configReg.fullPelForwardVector = context->fullPelForwardVector;
	configReg.forwardRSize = context->forwardFCode - 1;
	configReg.pictureCodingType = PICT_TYPE(context);
	tempReg = *((uint32 *) &configReg);

	fprintf( context->streamID.commandsFile,
			 "\tctlWrite (`MPG_REG_PARSER_CONFIG, 32'h%08lx);\n",
			 tempReg);

	tempReg = 0xFFFFFFFFL;
	fprintf( context->streamID.commandsFile,
			 "\tctlWrite (`MPG_REG_CONFIG, 32'h%08lx);\n",
			 tempReg);

	fprintf( context->streamID.commandsFile,
			 "\tWaitForPicture;\n");

	size = (SEQ_WIDTH(context) * SEQ_HEIGHT(context) * 3) / 2;

	fprintf( context->streamID.commandsFile,
			 "\t$csim (\"bsave 0x%08lx %ld %s/%s/%s.hw%d.yuv\");\n",
			 0x40000000L+outAddr,
			 size,
			 "/db/bdwork/mpeg/streams",
			 outFilePrefix,
			 outFilePrefix,
			 pictureNumber++ );
}

extern int32 gStartPictureNumber, gPictureCount;

int32 DoSlice(tVideoDeviceContext *theUnit, MPEGStreamInfo *si)
{
	if( pictureNumber < gStartPictureNumber )
	{
		pictureNumber++;
		return( 0L );
	}
	if( pictureNumber > (gStartPictureNumber + gPictureCount) )
	{
		exit( 0 );
	}
	DumpCommands( context );
	return( MPDumpSlices( &(context->streamID) ) );
}

int32 DoMPVideoExtensionData( MPEGStreamInfo *context, int32 data )
{
	return( 0L );
}

int32 DoMPVideoUserData( MPEGStreamInfo *context, int32 data )
{
	return( 0L );
}

int32 DoOpenOutputFiles( MPEGStreamInfo *context, char *filePrefix )
{
	char fileName[256];

	sprintf( outFilePrefix,"%s",filePrefix );

	sprintf( fileName,"%s.slices",filePrefix );
	if( (context->streamID.slicesFile = fopen( fileName, "wb" )) ==
		(FILE *) NULL )
	{
		fprintf(stderr,"couldn't open %s\n",fileName);
		goto abort;
	}
	printf("writing slices file: %s\n",fileName);

	sprintf( fileName,"%s.commands",filePrefix );
	if( (context->streamID.commandsFile = fopen( fileName, "w" )) ==
		(FILE *) NULL )
	{
		fprintf(stderr,"couldn't open %s\n",fileName);
		goto abortCloseSlices;
	}
	printf("writing commands file: %s\n",fileName);

	return( 0 );

abortCloseSlices:
	fclose( context->streamID.slicesFile );
abort:
	return( ERROR );
}

#endif

