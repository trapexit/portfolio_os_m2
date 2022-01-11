/* @(#) MPEGVideoParse.c 96/12/11 1.24 */
/* file: MPEGVideoParse.c */
/* MPEG video stream parser */
/* 10/3/94 George Mitsuoka */
/* The 3DO Company Copyright © 1994 */

#include <kernel/debug.h>
#include <kernel/types.h>
#include "MPEGQuantizerTables.h"
#include "MPEGVideoActions.h"
#include "MPEGVideoParse.h"
#include "MPEGVideoBuffers.h"
#include "MPEGStream.h"

#ifndef STANDALONE
#include "mpVideoDriver.h"
#else
#define MPVFLUSHREAD (1)
#endif

#define MPEG_MIN(a,b) ((a < b) ? a : b)

#ifdef DEBUG_PRINT_PARSE
  #define DBGPARSE(a)		PRNT(a)
#else
  #define DBGPARSE(a)		(void)0
#endif

int32 MPVideoParse(tVideoDeviceContext* theUnit, MPEGStreamInfo *context)
{
	int32 code, result = 0;

	/* force a seek to the next picture */
	if( context->parseState == gotPictHdr )
		context->parseState--;

	/* scan stream */
	while( 1 )
	{
		/* find next start code and dispatch to appropriate handler */
		code = NextStartCode(theUnit, &(context->streamID));
		switch( code )
		{
			case PICTURE_START_CODE:
				context->layer = PICTURE_START_CODE;
				context->pictureNumber++;
				if( MPCurrentPTS( &(context->streamID),
								  &(context->pts), &(context->userData) ) )	
					context->ptsValid = 1L;
				else
					context->ptsValid = 0L;
				result = MPPictureHeader(theUnit, context);
				if( result )
					return( result );
				if( context->parseState >= gotSeqHdr )
					context->parseState = gotPictHdr;
				context->flags |= MPS_FLAG_PICTHEADER_VALID;

				/* Check for play mode: All frames or I frames. */
				if (theUnit->playMode == MPEGMODEPLAY) {
					if (context->parseIFrameOnly &&
						!context->gopHeader.closedGOP) {
						DBGPARSE(("MPVideoParse: ---> START OF BRANCH\n"));
						theUnit->branchState = JUST_BRANCHED;
						ResetReferenceFrames(theUnit);
					}

					context->parseIFrameOnly = 0;
					/* If we haven't allocated reference and/or strip buffers, 
					 * allocate them now. This is the case
					 * when we were in I frame only mode and the
					 * client changes the mode without any preparation.
					 * This should not happen! New reference buffers are
					 * most probably are out of sync with MPEG stream.
					 * AllocDecodeBuffers is called to allocate the reference
					 * and/or strip buffers and prevent a driver crash.
					 */
					if (theUnit->refPicSize == 0 || theUnit->stripBuffer == NULL) {
						PERR(("MPVideoParse: WARNING too late to allocate buffers.\n"));
						if ((result = AllocDecodeBuffers(theUnit, context)))
							return result;
					}
				} else if (theUnit->playMode == MPEGMODEIFRAMESEARCH)
					context->parseIFrameOnly = 1;
				break;
			case USER_DATA_START_CODE:
				result = MPUserData(theUnit, context);
				if( result )
					return( result );
				break;
			case SEQUENCE_HEADER_CODE:
				context->layer = SEQUENCE_HEADER_CODE;
				result = MPSequenceHeader(theUnit, context);
				if( result )
					return( result );
				context->parseState = gotSeqHdr;
				context->flags |= MPS_FLAG_SEQHEADER_VALID;
				break;
			case SEQUENCE_ERROR_CODE:
				result = MPRead(theUnit, &(context->streamID),
								(char *) &code, 4L );
				if( result )
					return( result );
				break;
			case EXTENSION_START_CODE:
				result = MPExtensionData(theUnit, context);
				if( result )
					return( result );
				break;
			case SEQUENCE_END_CODE:
				result = MPRead(theUnit, &(context->streamID),
								(char *) &code, 4L);
				if( result )
					return( result );
				break;
			case GROUP_START_CODE:
				context->layer = GROUP_START_CODE;
				result = MPGroupOfPicturesHeader(theUnit, context);
				if( result )
					return( result );
				context->flags |= MPS_FLAG_GOPHEADER_VALID;
				if (context->gopHeader.closedGOP && theUnit->branchState) {
					DBGPARSE(("MPVideoParse: ---> JUMPED TO A CLOSED GOP. EOF BRANCH\n"));
					theUnit->branchState = NO_BRANCH;
				}
				break;
			case MPVFLUSHDRIVER:
			case MPVFLUSHWRITE:
				return( code );
			case ABORTED:
				DBGPARSE(("MPVideoParse: NextStartCode returns ABORTED.\n"));
				return code;
				break;
			default:
				if ((SLICE_START_CODE_MIN <= code) &&
					(code <= SLICE_START_CODE_MAX))
				{
					if(context->parseState < gotPictHdr)
						result = FlushBytes(theUnit, &context->streamID, 4);

					else if(context->parseIFrameOnly && PICT_TYPE(context) != 1)
						result = FlushBytes(theUnit, &context->streamID, 4);

					else if (theUnit->branchState) {
						switch (theUnit->branchState) {
						case JUST_BRANCHED:
							if (PICT_TYPE(context) == 1) {
								DBGPARSE(("MPVideoParse: --->JUST BRANCH: REC. ONE I FRAME\n"));
								theUnit->branchState = FIRST_REF_AFTER_BRANCH;
								result = MPSlice(theUnit, context);
							} else
								result = FlushBytes(theUnit, &context->streamID,
													4);
							break;
						case FIRST_REF_AFTER_BRANCH:
							if (PICT_TYPE(context) == 1 ||
								PICT_TYPE(context) == 2) {
								DBGPARSE(("MPVideoParse: --->REC 2ND REF. FRAME EOF BRANCH\n"));
								theUnit->branchState = NO_BRANCH;
								result = MPSlice(theUnit, context);
							} else
								result = FlushBytes(theUnit, &context->streamID,
													4);
							break;
						}
					}
					else
						result = MPSlice(theUnit, context);

					/* hardware should decode to next picture,
					   if it hasn't, skip to the next picture */
					if( context->parseState == gotPictHdr )
						context->parseState--;

					if (result)
						return result;
				}
				else if( (SYSTEM_START_CODE_MIN <= code) &&
						 (code <= SYSTEM_START_CODE_MAX) )
				{
					result = MPRead(theUnit, &(context->streamID),
									(char *) &code, 4L );
					if( result )
						return( result );
					break;
				}
				else if( code )
				{
					PERR(("NextStartCode returns %ld\n",code));
					break;
				}
				else
				{
					PERR(("MPRead returns %ld\n",code));
					result = MPRead(theUnit, &(context->streamID),
									(char *) &code, 4L );
					if( result )
						return( result );
				}
				break;
		}
	}
}

/* parse sequence header */

int32 MPSequenceHeader(tVideoDeviceContext* theUnit, MPEGStreamInfo *context )
{
	int32 status, i;
	uint8 nextByte;
	uint8 minQuantValue = 255;

	context->quantMatrixOK = 1;

	status = MPRead(theUnit, &(context->streamID),
					(char *) &(context->seqHeader),
					sizeof(sequenceHeader));
	if( status )
		return( status );
	if( context->seqHeader.loadIntraQuantizerMatrix )
	{
		context->intraQuantMatrix[ 0 ] =
			context->seqHeader.loadNonIntraQuantizerMatrix << 7;
		for( i = 0; i < 63; i++ )
		{
			status = MPRead(theUnit, &(context->streamID),
							(char *) &nextByte, 1L);
			if( status )
				return( status );
			context->intraQuantMatrix[ i ] |= nextByte >> 1;
			context->intraQuantMatrix[ i + 1 ] = nextByte << 7;
			minQuantValue = MPEG_MIN(minQuantValue, context->intraQuantMatrix[i]);
		}
		status = MPRead(theUnit, &(context->streamID), (char *) &nextByte, 1L);
		if( status )
			return( status );

		context->intraQuantMatrix[ i ] |= nextByte >> 1;
		minQuantValue = MPEG_MIN(minQuantValue, context->intraQuantMatrix[i]);
		context->seqHeader.loadNonIntraQuantizerMatrix = nextByte & 0x1;
	}
	else
		for( i = 0; i < 64; i++ )
			context->intraQuantMatrix[ i ] = defaultIntraQuantizerMatrix[ i ];
			
	if( context->seqHeader.loadNonIntraQuantizerMatrix )
	{
		for( i = 0; i < 64; i++ )
		{
			status = MPRead(theUnit, &(context->streamID), (char *) &nextByte,
							1L);
			if( status )
				return( status );

			context->nonIntraQuantMatrix[ i ] = nextByte;
			minQuantValue = MPEG_MIN(minQuantValue, context->nonIntraQuantMatrix[i]);
		}
	}
	else
		for( i = 0; i < 64; i++ )
			context->nonIntraQuantMatrix[ i ] =
				defaultNonIntraQuantizerMatrix[ i ];
			
	if (minQuantValue < 8) {
		context->quantMatrixOK = 0;
		status = InitVCDOptimize(&context->vcdInfo, context);
		if (status)
			return status;
	} else
		context->quantMatrixOK = 1;

	return AllocDecodeBuffers(theUnit, context);
}

/* parse group of pictures header */

int32 MPGroupOfPicturesHeader(tVideoDeviceContext* theUnit,
							  MPEGStreamInfo *context)
{
	int32 status;
	
	
	status = MPRead(theUnit, &(context->streamID),
					(char *) &(context->gopHeader), sizeof(GOPHeader));
	return( status );
}

/* parse picture header */

int32 MPPictureHeader(tVideoDeviceContext* theUnit, MPEGStreamInfo *context)
{
	int32 status;
	uint8 nextByte;
	
	status = MPRead(theUnit, &(context->streamID),
					(char *) &(context->pictHeader), sizeof(pictureHeader));
	if( status )
		return( status );

	if( (PICT_TYPE(context) == 2) || (PICT_TYPE(context) == 3) )
	{
		status = MPRead(theUnit, &(context->streamID), (char *) &nextByte, 1L);
		if( status )
			return( status );

		context->fullPelForwardVector = context->pictHeader.extraBits >> 2;
		context->forwardFCode = (context->pictHeader.extraBits << 1) & 0x7;
		context->forwardFCode |= nextByte >> 7;

		if( PICT_TYPE(context) == 3 )
		{
			context->fullPelBackwardVector = (unsigned) (nextByte >> 6) & 0x1;
			context->backwardFCode = (unsigned) (nextByte >> 3) & 0x7;
		}
	}
	else
	{
		context->fullPelForwardVector = 0;
		context->forwardFCode = 0;
		context->fullPelBackwardVector = 0;
		context->backwardFCode = 0;
	}
	/* we ignore any extra_information_picture bytes for now */
			
	return( 0L );
}

/* handle slice */

int32 MPSlice(tVideoDeviceContext* theUnit, MPEGStreamInfo *context)
{
	return DoSlice(theUnit, context);
}

/* parse extension data */

int32 MPExtensionData(tVideoDeviceContext* theUnit, MPEGStreamInfo *context)
{
	int32 status, code;
	uint8 nextByte;
	
	status = MPRead(theUnit, &(context->streamID), (char *) &code, 4L);
	if( status )
		return( status );
	while( 1 )
	{
		status = MPLook( &(context->streamID), (char *) &code, 4L );
		if( status )
			return( status );

		if( (code & 0xffffff00) == START_CODE_PREFIX )
			break;
		status = MPRead(theUnit, &(context->streamID), (char *) &nextByte, 1L);
		if( status )
			return( status );
		DoMPVideoExtensionData( context, nextByte );
	}
	return( 0L );
}

/* parse user data */

int32 MPUserData(tVideoDeviceContext* theUnit, MPEGStreamInfo *context)
{
	int32 status, code;
	uint8 nextByte;

	status = MPRead(theUnit, &(context->streamID), (char *) &code, 4L);
	if( status )
		return( status );
	while( 1 )
	{
		status = MPLook( &(context->streamID), (char *) &code, 4L );
		if( status )
			return( status );

		if( (code & 0xffffff00) == START_CODE_PREFIX )
			break;
		status = MPRead(theUnit, &(context->streamID), (char *) &nextByte, 1L);
		if( status )
			return( status );
		DoMPVideoUserData( context, nextByte );
	}
	return( 0L );
}

