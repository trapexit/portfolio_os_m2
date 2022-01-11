/******************************************************************************
**
**  @(#) mpaudiodecode.c 96/11/25 1.7
**
**      MPEG audio decoder top level
**	NOTE: currently we only support Layer 2
**	NOTE2: currently the hardware is only set up to play back at
**	44.1kHz sample rate.
**
*******************************************************************************/
#ifndef __STDIO_H
#include <stdio.h>
#endif

#ifndef __STDLIB_H
#include <stdlib.h>
#endif

#ifndef __STRING_H
#include <string.h>
#endif

#ifndef __KERNEL_OPERROR_H
#include <kernel/operror.h>
#endif

#ifndef __KERNEL_MEM_H
 #include <kernel/mem.h>
#endif

#ifndef __KERNEL_DEBUG_H
 #include <kernel/debug.h>
#endif

#ifndef __MISC_MPAUDIODECODE_H
 #include <misc/mpaudiodecode.h>
#endif

#ifndef __MISC_MPEG_H
 #include <misc/mpeg.h>
#endif

#include "mpegaudiotypes.h"

#include "bitstream.h"
#include "cosTable.h"
#include "mpegdebug.h"
/* #include "mpabufferqueue.h" */

#include "decode.h"
#include "windowCoeffs.h"

/*****************************/
/* local function prototypes */
/*****************************/

static Err parseFrame ( BufferInfo *bi, FrameInfo *fi, AudioDecodingBlksPtr matrices);
static void matrixFrame ( FrameInfo *fi, AudioDecodingBlksPtr matrices );
static void windowFrame ( BufferInfo *bi, FrameInfo *fi, AudioDecodingBlksPtr matrices );
void mpaMatrix(AUDIO_FLOAT *inputSamp, AUDIO_FLOAT *outputSamp, AUDIO_FLOAT *matrixCoef, AUDIO_FLOAT *scale, uint8 sblimit, uint32 nch);
void mpaWinGrp0(int32 *outputSamp, AUDIO_FLOAT *partialSums, AUDIO_FLOAT *matrixSamp, AUDIO_FLOAT *windowCoef);
void mpaWinGrp16(int32 *outputSamp, AUDIO_FLOAT *partialSums, AUDIO_FLOAT *matrixSamp, AUDIO_FLOAT *windowCoef);
void mpaWinGrp(int32 *outputSamp, AUDIO_FLOAT *partialSums, AUDIO_FLOAT *matrixSamp, AUDIO_FLOAT *windowCoef);
void mpaWinConM(int32 *output_samp);
void mpaWinConS(int32 *output_samp, int32 *windowTempArray);

/**********************************************************************
 * CreateMPAudioDecoder
 **********************************************************************/
Err CreateMPAudioDecoder( MPADecoderContext **ctx, MPACallbackFns CallbackFns )
{
MPADecoderContext *ctxPtr;

#ifdef DEBUG_AUDIO
	PRNT(("Allocating ctxPtr\n"));
#endif

	/* Allocate Decoder context structure and initialize it's
	 * contents. */
	ctxPtr = AllocMem( sizeof(*ctxPtr), MEMTYPE_FILL );
	if( !ctxPtr )
		return MPANoMemErr;

	*ctx = ctxPtr;
	/* initialize the scale factor */
	(ctxPtr)->matrices.scale = 32768.0; 
	
#ifdef DEBUG_AUDIO
	PRNT(("matrices @ 0x%x\n", &(ctxPtr)->matrices));
	PRNT(("Address of windowTempArray[]: %ld\n", (ctxPtr)->matrices.windowTempArray));
	PRNT(("Address of sampleCode[]: %ld\n", (ctxPtr)->matrices.samplecode));
	PRNT(("Address of matrixOutputSamples[]: %ld\n",
	(ctxPtr)->matrices.matrixOutputSamples));
	PRNT(("Address of matrixInputSamples[]: %ld\n",
	&(ctxPtr)->matrices.matrixInputSamples));
	PRNT(("Address of windowPartialSums[]: %ld\n",
	&(ctxPtr)->matrices.windowPartialSums[0][0]));
	PRNT(("Address of scale: %ld\n", ctxPtr->matrices.scale));
#endif

	/* Remember the callback functions */
	ctxPtr->bi.CallbackFns.CompressedBfrReadFn	= CallbackFns.CompressedBfrReadFn;
	ctxPtr->bi.CallbackFns.GetCompressedBfrFn	= CallbackFns.GetCompressedBfrFn;

	/* initialize read buffer stuff */
	ctxPtr->bi.readWord = 0L;
	ctxPtr->bi.readBits = 0L;
	ctxPtr->bi.readBuf	= NULL;
	ctxPtr->bi.readLen	= 0L;


	return (0);

} /* CreateMPAudioDecoder() */

/**********************************************************************
 * DeleteMPAudioDecoder
 **********************************************************************/
Err DeleteMPAudioDecoder( MPADecoderContext *ctx )
{

	if ( ctx ) 
		FreeMem( ctx, sizeof(MPADecoderContext) );

	return (0);

}

/**********************************************************************
 * MPAFlush
 **********************************************************************/
Err MPAFlush( MPADecoderContext *ctx )
{
	memset(ctx->matrices.windowPartialSums, 0, sizeof(ctx->matrices.windowPartialSums));
	return flushRead( &(ctx->bi) );

} /* MPAFlush() */

/**********************************************************************
 * MPAudioDecode
 **********************************************************************/
Err MPAudioDecode( void *theUnit, MPADecoderContextPtr ctx, uint32 *pts,
	 uint32 *ptsIsValidFlag, uint32 *decompressedBufr, AudioHeader *header )
{
	Err			status;
	FrameInfo	fi;

	resetReadStatus(&ctx->bi);
	ctx->bi.theUnit		= theUnit;
	ctx->bi.writeBuf	= (uint8 *)decompressedBufr;
	fi.header			= header;
	
	if ((status = parseFrame( &ctx->bi, &fi, &ctx->matrices )) != 0) 
		return status;
	
#if DEBUG_AUDIO
	DEBUGprintparse( &fi, &ctx->matrices.matrixInputSamples[0][0] );
#endif

	matrixFrame( &fi, &ctx->matrices );

#if DEBUG_PRINT
	DEBUGprintmatrix2( &fi, &ctx->matrices.matrixOutputSamples[0][0] );
#endif

	windowFrame( &ctx->bi, &fi, &ctx->matrices );
	
#if DEBUG_PRINT
/*		DEBUGprintwindow();*/
#endif

#ifdef DEBUG_AUDIO
	if ( ctx->bi.readLen )
	{
		PRNT(("MPAudioDecode(): Not done with this buffer.\n"));
		PRNT(("Bytes remaining: %d, @ addr: 0x%x\n",
			ctx->bi.readLen, ctx->bi.curBytePtr ));
	}
#endif

	if( ( ctx->bi.PTSIsValid ) && ( ctx->bi.wholeFrame ) )
	{
		/* return the pts and valid flag to the client. */
		*pts			= ctx->bi.timeStamp;
		*ptsIsValidFlag	= MPAPTSVALID;

#ifdef DEBUG_AUDIO
		PRNT(("MPAudioDecode(): PTS %d is valid.\n", *pts));
#endif
		ctx->bi.PTSIsValid	= false;
	}
	else /* This frame has no valid pts. */
	{
		/* return the pts and valid flag to the client. */
		*pts			= ctx->bi.timeStamp;
		*ptsIsValidFlag	= 0;

	}

	return (0);

} /* MPAudioDecode */


/**********************************************************************
 * parseFrame
 **********************************************************************/
Err parseFrame ( BufferInfo *bi, FrameInfo *fi, AudioDecodingBlksPtr matrices )
{
	int32		gr, p, ch;
	Err			status;
	tableToUse	tablesPicked;
				
	status = findSync (bi);				/* find the sync word */
	if( status != 0 )
	{
		/* Compressed data buffer was flushed or EOF encountered. */
		return status;
	}

	/* on error in parsing header, exit and reparse */
	if ((status = parseHeader(bi, fi, &tablesPicked)) != 0 )
	{
		return status;
	}

	/* for each triplet of sample codes */
	for (gr=0; gr<12; gr++)
	{

		/* parse the sample codes */
		if ((status = parseSubbands(bi, fi, &tablesPicked, &matrices->samplecode[0][0][0])) != 0) 
		{
			return status;
		}

		for (p=0; p<3; p++)
		{
			for (ch=0; ch<fi->nch; ch++)
			{

				/* requantize the values */
				Requantize( fi,
					 		&tablesPicked,
						 	&matrices->samplecode[ch][0][p],
							&matrices->matrixInputSamples[ch][(gr*3 + p)<<5],
						 	&fi->allocation[ch], 
						 	&fi->scalefactor_index[(gr>>2)+3*ch]);
			} /* for (ch=0; ch<fi->nch; ch++) */
		} /* for (p=0; p<3; p++) */
	} /* for (gr=0; gr<12; gr++) */

	return (0);
} /* parseFrame	*/			


/**********************************************************************
 * matrixFrame
 **********************************************************************/
void matrixFrame ( FrameInfo *fi, AudioDecodingBlksPtr matrices )
{

	mpaMatrix(&matrices->matrixInputSamples[0][0],
			&matrices->matrixOutputSamples[0][0],
			cosTable,
			&matrices->scale,
			fi->sblimit,
			fi->nch);

  return;
}


/**********************************************************************
 * windowFrame
 **********************************************************************/
void windowFrame ( BufferInfo *bi, FrameInfo *fi, AudioDecodingBlksPtr matrices )
{
	uint32 i;
	int32 *outputSamp;
	AUDIO_FLOAT *matrixSamp;


	outputSamp = (int32 *)(bi->writeBuf);
	matrixSamp = (AUDIO_FLOAT *)&matrices->matrixOutputSamples[0][0];

	/* process group 0 samples, first channel */
	mpaWinGrp0(outputSamp,
			 &matrices->windowPartialSums[0][0],
			 matrixSamp, 
			 window); 

	/* process group 16 samples, first channel */
	mpaWinGrp16(outputSamp + 1, 
				&matrices->windowPartialSums[0][16],
				matrixSamp, 
				window); 

	/* do remaining group pair samples, first channel */
	for (i=1; i<16; i++)
	{
		mpaWinGrp(outputSamp + i*2, 
				&(matrices->windowPartialSums[0][i*32]),
				matrixSamp + i*72,
				window + i*16);
	}

	if (fi->nch == 2)
	{	/* stereo or dual channel */
		matrixSamp = (AUDIO_FLOAT *)&(matrices->matrixOutputSamples[1][0]);

		/* process group 0 samples, second channel */
		mpaWinGrp0(matrices->windowTempArray,
				 &(matrices->windowPartialSums[1][0]),
				 matrixSamp, 
				 window); 

		/* process group 16 samples, second channel */
		mpaWinGrp16(matrices->windowTempArray + 1,
					&(matrices->windowPartialSums[1][16]),
					matrixSamp, 
					window); 

		/* do remaining group pair samples, second channel */
		for (i=1; i<16; i++)
		{
			mpaWinGrp(matrices->windowTempArray + i*2,
						&(matrices->windowPartialSums[1][i*32]),
						matrixSamp + i*72,
						window + i*16);
		}

		/* convert to 16 bit integers, clip, and pack into array */
		/* do for each sample in group */
		mpaWinConS(outputSamp, matrices->windowTempArray);
	}
	else
	{	/* mono */
		/* convert to 16 bit integers, clip, and pack into array */
		/* do for each sample in group */
		mpaWinConM(outputSamp);
	}

	return;
} /* windowFrame() */

