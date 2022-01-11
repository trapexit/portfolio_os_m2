/******************************************************************************
**
**  @(#) weavestream.c 96/11/20 1.12
**
******************************************************************************/
/*****************************************************************************
**
**	File:			WeaveStream.c
**
**	Contains:		data stream merging routines
**
**	Written by:		Joe Buczek & friends
**
 ******************************************************************************/

#include <cursorctl.h>

#include <string.h>
#include <stdlib.h>

#include "weavestream.h"

#ifndef __STREAMING_DSSTREAMDEFS_H
#include <streaming/dsstreamdefs.h>
#endif

/* a chunk must have at least a header plus one long since an MPEG FRME chunk ALWAYS needs
 *  a pts
 */
#define MIN_SPLIT_SIZE					(sizeof(SubsChunkData) + sizeof(uint32))

/****************************/
/* Local routine prototypes */
/****************************/
static InStreamPtr	GetEarliestBelowNextMarker( WeaveParamsPtr pb, uint32 sizeRemaining );
static InStreamPtr	GetBestFitBelowNextMarker( WeaveParamsPtr pb, uint32 sizeRemaining );
static char*		GetIOBuffer( long bufferSize );
static bool			InputDataRemaining( WeaveParamsPtr pb );
static void			AdjustChunkTimes(SubsChunkData *chunkPtr, uint32 offset);
static bool			ReadChunkHeader( WeaveParamsPtr pb, InStreamPtr isp );
static void			SortMarkers( WeaveParamsPtr pb );

static bool			SplitChunk(WeaveParamsPtr pb, InStreamPtr isp, uint32 sizeRemaining);
static TypeSubType	*SplitChunkInfo(WeaveParamsPtr pb, SubsChunkData *header);
static bool			ChunkCanBeSplit(WeaveParamsPtr pb, InStreamPtr isp, uint32 bufferSpaceRemaining);
static bool			ChunkCanMoveEarlier(WeaveParamsPtr pb, InStreamPtr chunkIsp);
static bool			UpdateMarkerTable(WeaveParamsPtr pb);

static bool			WriteChunk( WeaveParamsPtr pb, InStreamPtr isp, uint32* bytesRemaining );
static bool			WriteMarkerTable( WeaveParamsPtr pb, uint32* bytesRemaining );
static bool			WriteHeaderChunk( WeaveParamsPtr pb, uint32* bytesRemaining );
static bool			WriteAlrmChunk( WeaveParamsPtr pb, uint32* bytesRemaining, uint32 timeValue );
static bool	 		WriteFillerChunk(WeaveParamsPtr pb, uint32* bytesRemaining);
static bool			WriteGotoChunk(WeaveParamsPtr pb, InStreamPtr isp, uint32* bytesRemaining);
static bool			WriteStopChunk(WeaveParamsPtr pb, InStreamPtr isp, uint32* bytesRemaining);
static bool			WriteHaltChunk(WeaveParamsPtr pb, InStreamPtr isp, uint32* bytesRemaining);
static bool			WriteCurrentChunk(WeaveParamsPtr pb, InStreamPtr isp, uint32 *remainingInCurrentBlock);


/*==========================================================================================
  ==========================================================================================
									Private Utility Routines
  ==========================================================================================
  ==========================================================================================*/



#if CACHE_OPEN_FILES


/*
 * OpenInputStream
 *	we always leave the most recently opened input stream open to avoid some overhead
 *	in event that we write from the same file sequentially
 */
static	bool
OpenInputStream(WeaveParamsPtr pb, InStreamPtr isp)
{
	bool	success = false;
	
	/* if the file is already open, do nothing */
	if ( isp != pb->currOpenInStream )
	{
		/* close the current file (if any).  remember the current file offset so we will 
		 *  know where to return to when we next use this file
		 */
		if ( 0 != pb->currOpenInStream )
		{
			pb->currOpenInStream->fileOffset = ftell(pb->currOpenInStream->fd);
			fclose(pb->currOpenInStream->fd);
		}

		/* open the file by name... */
		if ( NULL == (isp->fd = fopen(isp->fileName, "rb")) )
		{
			fprintf(stderr, "unable to open input data stream file: %s\n", isp->fileName );
			goto DONE;
		}
		/* reposition to the location we last read from */
		if ( 0 != fseek(isp->fd, isp->fileOffset, SEEK_SET ) )
		{
			fprintf(stderr, "Error seeking to %ld in stream file \"%s\"\n", 
						isp->fileOffset, isp->fileName);
			goto DONE;
		}

		pb->currOpenInStream = isp;
	}

	success = true;

DONE:
	return success;
}

#endif


/*******************************************************************************************
 * Routine to allocate alternate I/O buffers for the weaving process. Bigger is better!
 *******************************************************************************************/
static char		*GetIOBuffer( long bufferSize )
	{
	char	*ioBuf;

	ioBuf = (char *) malloc( bufferSize );
	if ( ioBuf == NULL )
		{
		fprintf( stderr, "GetIOBuffer() - failed. Use larger MPW memory partition!\n" );
		fprintf( stderr, "                Use Get Info... from the Finder to set it.\n" );
		}

	return ioBuf;
	}


/*******************************************************************************************
 * Routine to sort the marker records into ascending order before the weaving
 * process begins.
 *******************************************************************************************/
static void		SortMarkers( WeaveParamsPtr pb )
	{
	long		k;
	long		n;
	MarkerRec	temp;

	SpinCursor(32);

	/* Insert an 'infinity' marker so that marker comparisons
	 * will always work properly.  The table actually has "maxMarkers + 1"
	 * elements so don't worry about overflowing table here
	 */
	pb->marker[ pb->numMarkers++ ].markerTime = INFINITY_TIME;

	/* The El Cheapo bubble sort to put markers
	 * into ascending order.
	 */
	for ( k = 0; k < pb->numMarkers - 1; k++ )
		{
		for ( n = k + 1; n < pb->numMarkers; n++ )
			{
			if ( pb->marker[n].markerTime < pb->marker[k].markerTime )
				{
				temp = pb->marker[k];
				pb->marker[k] = pb->marker[n];
				pb->marker[n] = temp;
				}
			}
		}
	}


/*******************************************************************************************
 * Routine to search the input streams and determine if any data is left to output.
 *******************************************************************************************/
static bool	InputDataRemaining( WeaveParamsPtr pb )
	{
	InStreamPtr	isp = pb->inStreamList;
	bool		eof = false;

	while ( (NULL != isp) && ( (eof = isp->eof) == true ) )
		isp = isp->next;
		
	return !eof;
	}


/*******************************************************************************************
 * routine to split a chunk to fill otherwise wasted space at the end of a buffer
 *******************************************************************************************/
static bool
SplitChunk(WeaveParamsPtr pb, InStreamPtr isp, uint32 sizeRemaining)
{
	TypeSubType		*splitInfo;
	bool			success = false;

	/* Tweak the chunk in such a was as to indicate that it has been split,
	 * and save off information that will be needed to finish the job
	 */
	if ( NULL == (splitInfo = SplitChunkInfo(pb, &isp->buf)) )
	{
		fprintf (stderr, "WriteChunk() - \"SplitChunkInfo()\" returned NULL!\n\n");
		goto EXIT;
	}
	isp->preSplitSize		= isp->buf.chunkSize;
	isp->splitOffset		=
	isp->buf.chunkSize		= sizeRemaining;
	
	/* now mark the chunk as the beginning of a split chunk, remember what
	 *  we need to use as an end subtype 
	 */
	isp->buf.subChunkType	= splitInfo->splitStartSubType;
	isp->splitEndSubType	= splitInfo->splitEndSubType;
	
	success = true;

EXIT:
	return success;
}


/*******************************************************************************************
 * routine to see if a chunk can be split to fill otherwise wasted space at the 
 *  end of a buffer.
 *
 * NOTE: splitting this comparison off into a function is somewhat wasteful given
 *		 the current weaver, but it allows us to easily add more splittable types
 *******************************************************************************************/
static bool
ChunkCanBeSplit(WeaveParamsPtr pb, InStreamPtr isp, uint32 bufferSpaceRemaining)
{
	TypeSubType		*splitType = pb->splitTypes;
	int16			ndx;
	bool			canSplit = false;
	
	for ( ndx = 0; ndx < pb->splitTypeCount; ndx++, splitType++ )
	{
		if ( (isp->buf.chunkType == splitType->type)			/* chunk type registered as splittable */
			&& (isp->buf.subChunkType == splitType->subType)	/*  ditto for the subtype */
	    	&& (bufferSpaceRemaining >= MIN_SPLIT_SIZE)			/* enough buffer space for min split block */
			&& (isp->buf.chunkSize >= MIN_SPLIT_SIZE)			/* chunk big enough for min split block */
			&& ((isp->buf.chunkSize - bufferSpaceRemaining) <= pb->streamBlockSize) )
																/* chunk leftover after split is small enough 
																 *  to fit into the next whole block 
																 */
			break;
	}
	if ( ndx < pb->splitTypeCount )
		canSplit = true;

	return canSplit;
}


static TypeSubType *
SplitChunkInfo(WeaveParamsPtr pb, SubsChunkData *header)
{
	TypeSubType		*splitType = pb->splitTypes;
	int16			ndx;

	for ( ndx = 0; ndx < pb->splitTypeCount; ndx++, splitType++ )
	{
		if ( (header->chunkType == splitType->type)
			&& (header->subChunkType == splitType->subType) )
			break;
	}
	if ( ndx >= pb->splitTypeCount )
		splitType = NULL;

	return splitType;
}


/*******************************************************************************************
 * see if a chunk can be pulled back in time to fill otherwise wasted space at the
 *  end if a buffer.  we already know that the chunk can _fit_ into the space, so
 *  now make sure it meets all of the other requirements:
 *
 *    + the delta between it's dislay time and that of the most recently written chunk
 *       is not too large
 *
 *    + none of the other files has a chunk at the head of it's queue destined for 
 *       the same subscriber which has an earlier display time
 *******************************************************************************************/
static bool
ChunkCanMoveEarlier(WeaveParamsPtr pb, InStreamPtr chunkIsp)
{
	InStreamPtr	isp;
	long		delta;
	bool		canMoveChunk = false;

	/* if the chunk's display time is too far in the future, reject it */
	delta = (long)chunkIsp->buf.time - pb->lastChunkWritten.time;
	if ( (delta < 0) || (delta > pb->earlyChunkMaxTime) )
		goto DONE;

	/* now look through all of the other queued streams to see if one has a chunk
	 *  for the same subscriber which is earlier in time
	 */
	for ( isp = pb->inStreamList; isp != NULL; isp = isp->next )
	{
		if ( (false == isp->eof)
			 && (isp != chunkIsp)
			 && (isp->buf.chunkType == chunkIsp->buf.chunkType)
			 && (isp->buf.time < chunkIsp->buf.time) )
			goto DONE;
	}

	/* guess it's safe to move it... */
	canMoveChunk = true;
	
DONE:
	return canMoveChunk;
}



/*******************************************************************************************
 * Update the marker table. 
 *******************************************************************************************/
static bool UpdateMarkerTable(WeaveParamsPtr pb)
{
	bool		success = true;

	/* Reset the update flag */
	pb->fUpdateMarkerTable = false;

	/* Update the marker's file position value and time, and
	 * bump the next marker index to the next position
	 * in the marker table.
	 */
	pb->marker[pb->currMarkerIndex].markerOffset = ftell(pb->outputStreamFd);
	pb->currMarkerIndex++;

	/* make sure we haven't overflowed the marker table */
	if ( pb->currMarkerIndex >= pb->numMarkers )
	{
		fprintf( stderr, "UpdateMarkerTable() - internal error, overflowed marker table!\n" );
		success = false;
	}
	return success;
}


/*******************************************************************************************
 * Routine to search the input streams and find the stream whose chunk is temporally
 * "next" in time (least value numerically), and whose time is before the next marker
 * time in the stream.
 *******************************************************************************************/
static InStreamPtr	GetEarliestBelowNextMarker( WeaveParamsPtr pb, uint32 sizeRemaining )
	{
	InStreamPtr	winner = NULL;
	InStreamPtr	isp;
	long		timeToBeat = INFINITY_TIME;
	uint32		nextMarker = pb->marker[ pb->currMarkerIndex ].markerTime;

	/* Search the input stream buffers for a chunk whose timestamp 
	 * is "ealiest", ignoring any files whose EOF has been reached, and
	 * ignoring whether or not the block will fit in the current output block.
	 */
	for ( isp	= pb->inStreamList; isp != NULL; isp = isp->next )
	{
		if ( (false == isp->eof) && (isp->buf.time < timeToBeat) )
		{
			timeToBeat = isp->buf.time;
			winner = isp;
		}
	}

	/* Make sure we found something. It is an error if we don't
	 * because an external check is made to insure that there is at
	 * least one file which hasn't reached EOF.
	 */
	if ( winner == NULL )
		{
		fprintf( stderr, "GetEarliestBelowNextMarker() - found nothing!\n" );
		goto EXIT;
		}
		
	/* If we are not dealing with marker tables, then we
	 * don't need to update the marker table.
	 */
	if ( pb->fWriteMarkerTable ) {
	
		/* Force a marker table update if the "earliest" chunk we can find
		 * is at or after the next marker in time. It doesn't matter at this
		 * point whether or not the chunk can fit into the current output
		 * block because marker time takes precedence.
		 */
		if ( winner->buf.time >= nextMarker )
			{
			pb->fUpdateMarkerTable = true;
			winner = NULL;
			goto EXIT;
			}
		}
		
	/* If the "earliest" chunk will both fit into the space remaining
	 * and is below the next marker time, then output this chunk.
	 */
	if ( winner->buf.chunkSize <= sizeRemaining )
		goto EXIT;

	/* the "earliest" chunk is too large to fit into the block, see if we can spit it */
	if ( true == ChunkCanBeSplit(pb, winner, sizeRemaining) )
	{
		if ( false == SplitChunk(pb, winner, sizeRemaining) )
			winner = NULL;
	} 
	else
		winner = NULL;

EXIT:
	return winner;
	}


/*******************************************************************************************
 * Routine to find the next block in the stream which is both below the next marker
 * temporally, and is a best fit for the 'size' given.
 *******************************************************************************************/
static InStreamPtr	GetBestFitBelowNextMarker( WeaveParamsPtr pb, uint32 sizeRemaining )
{
	InStreamPtr	winner = NULL;
	InStreamPtr	isp;
	uint32		timeToBeat = INFINITY_TIME;
	uint32		nextMarker = pb->marker[ pb->currMarkerIndex ].markerTime;

	/* if we are need to write a marker, don't bother to check for a chunk */
	if ( (pb->fWriteMarkerTable) && (pb->fUpdateMarkerTable) )
		goto EXIT;

	/* if we have a defined chunk type to split and the chunk size remaining is at least
	 *  as least MIN_SPLIT_SIZE, search the current header list and return a partial chunk.
	 */
	if ( (pb->splitTypeCount > 0) && (sizeRemaining >= MIN_SPLIT_SIZE) )
	{
		for ( isp = pb->inStreamList; isp != NULL ; isp = isp->next )
		{
			/* see if we are allowed to split the chunk type, and if so make sure  
			 *  that it won't screw up the next marker
			 */
			if ( (false == isp->eof)
				 && (true == ChunkCanBeSplit(pb, isp, sizeRemaining))
				 && (((isp->buf.time < nextMarker) && (nextMarker > 0)) || (nextMarker == 0)) )
			{
				if ( true == SplitChunk(pb, isp, sizeRemaining) )
				{
					winner = isp;
					goto EXIT;
				}
			}
		}
	}

	/* Search the input stream buffers for the chunk whose
	 * timestamp is lowest. Ignore any files whose EOF has been reached.
	 */
	for ( isp = pb->inStreamList; isp != NULL; isp = isp->next )
	{
		/* If we've found a potential candidate, make sure that it won't cause a
		 *  conflict with an upcoming marker chunk, and that it is a better fit than
		 *  whatever chunk we have already chosen
		 */
		if ( (false == isp->eof) 
			 && ((isp->buf.time < nextMarker) || (nextMarker == 0))
			 && (isp->buf.chunkSize <= sizeRemaining)
			 && (isp->buf.time < timeToBeat) )
		{
			/* we know it will fit, see if it is legal to move the chunk earlier */
			if ( ChunkCanMoveEarlier(pb, isp) )
			{
				timeToBeat = isp->buf.time;
				winner = isp;
			}
		}
	} /* end for loop */



EXIT:
	/* Return "best fit" or NULL */
	return winner;
}


/*******************************************************************************************
 * modify a chunk's timestamp(s) by a constant
 *******************************************************************************************/
static void
AdjustChunkTimes(SubsChunkData *chunkPtr, uint32 offset)
{
	chunkPtr->time += offset;
	
	/* MPEG chunks also need to have their PTS adjusted */
	if ( (MPVD_CHUNK_TYPE == chunkPtr->chunkType) && (MVDAT_CHUNK_SUBTYPE == chunkPtr->subChunkType) )
	{
		/* pts is expressed in 90kHz units, convert from 3DO ticks */
		((MPEGVideoFrameChunk *)chunkPtr)->pts += (offset * (90000. / AUDIO_TICKS_PER_SEC));
	}
}


/*******************************************************************************************
 * Routine to read the next chunk header for a given input stream. Sets any necessary
 * flags and outputs diagnostic messages if there's any error.
 *******************************************************************************************/
static bool	ReadChunkHeader(WeaveParamsPtr pb, InStreamPtr isp)
{
	uint32	fillChunkSize;
	bool	success = false;

#if CACHE_OPEN_FILES
	/* !!!!!
	   this routine is NEVER called when 'isp' is not the currently open file.  
		IF YOU CHANGE THIS BEHAVIOR, MAKE SURE THE FILE IS OPENED FIRST!!
	   !!!!! 
	*/
#endif

	/* Load the next chunk header from the file we just wrote
	 * data from. If we reach EOF, set the EOF flag in the input
	 * stream control block. Skip any FILLER chunks we find.
	 */
	while ( 1 == fread( &isp->buf, sizeof(SubsChunkData), 1, isp->fd ) )
	{
		/* We don't want to weave fillers into the output
		 * stream, so ignore any we encounter.
		 */
		if ( isp->buf.chunkType == FILL_CHUNK_TYPE )
		{
			/* =============== UNSPEAKABLE HACK !!! =================
			 * The following is done because it is possible to have written
			 * out a _partial_ FILL chunk due to the fixed buffer sizes we use.
			 * I.e., there may not have been enough space to write an entire
			 * FILL chunk to the output stream. We have three cases to consider:
			 *		1.	a well formed FILL chunk
			 *		2.	a FILL chunk with the 'FILL' and size intact
			 *		3.	a FILL chunk with only the 'FILL' intact
			 *
			 * Here's what we do: if the chunk size "looks like" it is bad
			 * (there are bits set in the high 8 bits of a 32-bit count), 
			 * we assume that we have case #3, above, and assume the chunk
			 * size is 4 bytes (just the 'FILL' header). Otherwise, we use
			 * the size as-is, even if it is smaller than sizeof(SubsChunkData)
			 * because the fseek() will take a negative value if the calculation
			 * to get us to the start of the next block requires us to "back up".
			 */
			if ( (isp->buf.chunkSize & 0xff000000) != 0 )
				/* Assume only the 'FILL' is there */
				fillChunkSize = 4;
			else
				/* May or may not be a 12 byte basic chunk, but assume size is good */
				fillChunkSize = isp->buf.chunkSize;

			/* Seek past the filler chunk so we don't try to weave it
			 * into the output stream.
			 */
			if ( 0 == fseek( isp->fd, fillChunkSize - sizeof(SubsChunkData), SEEK_CUR ) )
				continue;
				
			fprintf( stderr, "ReadChunkHeader() - error seeking past filler in file: %s\n",
						isp->fileName );
				
			goto ERROR_EXIT;
		} /* chunkType == FILL_CHUNK_TYPE */

		/* Check that the chunk size is <= the stream block size. If not,
		 * output an error message and bail out.
		 */
		if ( (isp->buf.chunkSize > pb->streamBlockSize) 
			&& (false == ChunkCanBeSplit(pb, isp, pb->streamBlockSize)) )
		{
			fprintf( stderr, "ReadChunkHeader() - chunk size (%ld) > streamblocksize (%ld) in file: %s\n",
						isp->buf.chunkSize, pb->streamBlockSize, isp->fileName );
			goto ERROR_EXIT;
		}

		/* Add the starting time constant into the stream time
		 * for the newly read chunk. This causes it to sort into the
		 * output stream at the specified starting time. This also
		 * assumes that input streams are zero-relative.
		 */
		AdjustChunkTimes(&isp->buf, isp->startTime);
		
		goto DONE;
	} /* end while */

	/* Check for an EOF return. Set the stream's eof flag if we
	 * detect one, otherwise we've gotten a read error of some kind.
	 */
	if ( feof( isp->fd ) )
	{
		isp->eof = true;
		fclose ( isp->fd );
	}
	else
	{
		fprintf( stderr, "ReadChunkHeader() - error reading next chunk header from: %s\n",
						isp->fileName );
		goto ERROR_EXIT;
	}


DONE:
	success = true;

ERROR_EXIT:

	return success;
}


/*******************************************************************************************
 * Routine to write out the chunk specified by 'isp' to the stream specified by
 * 'pb'. Update the bytes remaining count by the number of bytes written. Also,
 * read the next chunk from the same file, setting the file's EOF flag if
 * appropriate.
 *******************************************************************************************/
static bool	WriteChunk( WeaveParamsPtr pb, InStreamPtr isp, uint32* bytesRemaining )
	{
	uint32	bytesToRead = 0;
	uint32	bytesToPad = 0;
	bool	success = false;

#if CACHE_OPEN_FILES
	/* make sure the file we're about to read from is open... */
	if ( false == OpenInputStream(pb, isp) )
		goto ERROR_EXIT;
#endif

	/* Make sure there's space in the current block for the chunk
	 * we are about to write. This should never happen because the 
	 * chunk selection process is supposed to prevent this.
	 */
	if ( isp->buf.chunkSize > (*bytesRemaining) )
		{
		fprintf( stderr, "WriteChunk() - chunk size too big!\n" );
		goto ERROR_EXIT;
		}

	/* remember the UNADJUSTED timestamp of this chunk because we do all comparisons with
	 *  the chunk time as it comes off of disk (ie. before being adjusted for stream bias
	 */
	pb->lastChunkWritten = isp->buf;

	/* Write out the chunk header from the buffer in the input stream
	 * descriptor. Add the output stream base time to the time in the
	 * chunk to create the desired time bias in the output stream.
	 */
	isp->buf.time += pb->outputBaseTime;
	if (1 != fwrite(&isp->buf, sizeof(SubsChunkData), 1, pb->outputStreamFd)) 
	{
		fprintf(stderr, "WriteChunk() - error writing marker table\n");
		goto ERROR_EXIT;
	}

	/* Read the actual chunk data from the selected input file. */
	bytesToRead = isp->buf.chunkSize - sizeof(SubsChunkData);

	/* 4 byte alignment. */
	bytesToPad = 0;
	if (bytesToRead % 4) {
		bytesToPad = 4 - (bytesToRead % 4);
		bytesToRead += bytesToPad;
	}
	
	if (bytesToRead < 0 || isp->buf.chunkSize < sizeof(SubsChunkData)) {
		fprintf (stderr, "WriteChunk() - chunkSize < sizeof SubsChunkData\n\n");
		goto ERROR_EXIT;
	}
	
	if (1 != fread(pb->scratchBuf, bytesToRead, 1, isp->fd)) {
		fprintf(stderr, "WriteChunk() - error reading file: %s\n", isp->fileName);
		goto ERROR_EXIT;
	}
	
	/* Check for padding (bytes not accounted for in the chunk's length word but used to pad it's
	 *  actual length out to a quad multiple). All pad bytes MUST be zero
	 */
	if (bytesToPad) {
		int i = 0;
		for(i = 0; i < bytesToPad; i++) {
			if (pb->scratchBuf[bytesToRead - 1 - i] != 0) {
				fprintf(stderr, "WriteChunk() - Non zero padding detected (all chunk pad bytes must be zero)\n");
				goto ERROR_EXIT;
			}
		}
	}
	
	/* Write out the chunk data we just read */
	if (1 != fwrite(pb->scratchBuf, bytesToRead, 1, pb->outputStreamFd)) 
	{
		fprintf(stderr, "WriteChunk() - error writing chunk data\n");
		goto ERROR_EXIT;
	}

	/* Update remaining free bytes in the theoretical output buffer.
	 * NOTE: the 'remaining' count is maintained only to know when a block
	 *			boundary is approaching, not that any buffer is actually being
	 *			consumed. This allow the algorithm to write out blocks that
	 *			will match the stream reader's input buffers.
	 */
	(*bytesRemaining) -= (sizeof(SubsChunkData) + bytesToRead);
	if ( (*bytesRemaining) == 0 )
		(*bytesRemaining) = pb->streamBlockSize;

	/* Load the next chunk header from the file we just wrote
	 * data from. If we reach EOF, set the EOF flag in the input
	 * stream control block.  If the chunk we just processed was 
	 * the beginning of a split chunk, don't read a new header just yet.
	 */
	if ( 0 == isp->splitOffset )
		success = ReadChunkHeader( pb, isp );
	else 
		{
		/* We've just finished processing the first half of a split
		 * chunk. Now fix up the header for the second half and 
		 * do some other minor housekeeping
		 */
		isp->buf.subChunkType = isp->splitEndSubType;
		isp->buf.chunkSize = (isp->preSplitSize - isp->splitOffset) + sizeof (SubsChunkData);
		 
		/* subtract off the outputBaseTime so we don't have "Blown Chunks"
		 * when we write the second half of the chunk
		 */
		isp->buf.time -= pb->outputBaseTime;
		isp->preSplitSize = isp->splitOffset = 0;
		 
		/* recurse to write the last half of the chunk */
		success = WriteChunk(pb, isp, bytesRemaining);
		}

ERROR_EXIT:

	return success;
	}



/*******************************************************************************************
 * Routine to write out the stream header chunk. 
 *******************************************************************************************/
static bool		WriteHeaderChunk( WeaveParamsPtr pb, uint32* bytesRemaining )
	{
	uint32			index, bytesToWrite = 0;
	DSHeaderChunk	header;

	/* If the flag that says we should write a header out is false,
	 * then we don't write anything.
	 */
	if ( ! pb->fWriteStreamHeader )
		return true;

	/* Make sure there's space in the current block for the chunk
	 * we are about to write.
	 */
	if ( sizeof(DSHeaderChunk) > (*bytesRemaining) )
		{
		fprintf( stderr, "WriteHeaderChunk() - not enough space!\n" );
		return false;
		}

	/* Format the header chunk for output using parameters passed
	 * to us initially.
	 */
	header.chunkType		= HEADER_CHUNK_TYPE;
	header.subChunkType		= 0;
	header.chunkSize		= sizeof(DSHeaderChunk);
	header.channel			= 0;
	header.time				= 0;
	header.headerVersion	= DS_STREAM_VERSION;
	header.streamBlockSize	= pb->streamBlockSize;
	header.streamBuffers	= pb->streamBuffers;
	header.audioClockChan	= pb->audioClockChan;
	header.enableAudioChan	= pb->enableAudioChan;
	header.streamerDeltaPri	= pb->streamerDeltaPri;	
	header.dataAcqDeltaPri	= pb->dataAcqDeltaPri;
	header.numSubsMsgs		= pb->numSubsMsgs;

	/* Copy the instrument preload table */
	for ( index = 0; index < sizeof(pb->preloadInstList)
							/ sizeof(pb->preloadInstList[0]); index++ )
		header.preloadInstList[ index ] = pb->preloadInstList[ index ];
	
	/* Copy the subscriber tag table */
	for ( index = 0; index < sizeof(pb->subscriberList)
							/ sizeof(pb->subscriberList[0]); index++ )
		header.subscriberList[ index ] = pb->subscriberList[ index ];

	/* Write the header chunk to the output file */
	bytesToWrite = sizeof(DSHeaderChunk);
	if ( 1 != fwrite( &header, bytesToWrite, 1, pb->outputStreamFd ) )
		{
		fprintf( stderr, "WriteHeaderChunk() - error writing stream header\n" );
		return false;
		}

	/* Update remaining free bytes in the theoretical output buffer.
	 */
	(*bytesRemaining) -= sizeof(DSHeaderChunk);
	if ( (*bytesRemaining) == 0 )
		(*bytesRemaining) = pb->streamBlockSize;

	return true;
	}


/*******************************************************************************************
 * Routine to write out the marker table chunk. 
 *******************************************************************************************/
static bool		WriteMarkerTable( WeaveParamsPtr pb, uint32* bytesRemaining )
	{
	uint32			sizeOfMarkerData = 0;
	DSMarkerChunk	markerChunk;

	/* If the flag that says we should write a marker table is false,
	 * then we don't write anything.
	 */
	if ( ! pb->fWriteMarkerTable )
		return true;

	/* Calculate the size of the marker table data.
	 *
	 * NOTE: 	We intentionally omit the last marker in the
	 *			table because it is an infinity placeholder
	 *			that we create at the start of the weaving 
	 *			process. This is done to keep the marker
	 *			comparison logic simple.
	 */
	sizeOfMarkerData = (pb->numMarkers - 1) * sizeof(MarkerRec);

	/* Format the marker table chunk for output using parameters passed
	 * to us initially.
	 */
	markerChunk.chunkType		= DATAACQ_CHUNK_TYPE;
	markerChunk.subChunkType	= MARKER_TABLE_SUBTYPE;
	markerChunk.chunkSize		= sizeof(DSMarkerChunk) + sizeOfMarkerData;
	markerChunk.channel			= 0;
	markerChunk.time			= 0;


	/* Make sure there's space in the current block for the chunk
	 * we are about to write.
	 */
	if ( markerChunk.chunkSize > (*bytesRemaining) )
		{
		fprintf( stderr, "WriteMarkerTable() - marker table will not into first stream block\n" );
		return false;
		}

	/* Write the marker table chunk header to the output file */
 	if ( 1 != fwrite( &markerChunk, sizeof(DSMarkerChunk), 1, pb->outputStreamFd ) )
	{
		fprintf(stderr, "WriteMarkerTable() - error writing marker table\n");
		return false;
	}

	/* Write the marker data to the output file */
	if ( 1 != fwrite( pb->marker, sizeOfMarkerData, 1, pb->outputStreamFd ) )
	{
		fprintf(stderr, "WriteMarkerTable() - error writing marker table\n");
		return false;
	}

	/* Update remaining free bytes in the theoretical output buffer.
	 */
	(*bytesRemaining) -= markerChunk.chunkSize;
	if ( (*bytesRemaining) == 0 )
		(*bytesRemaining) = pb->streamBlockSize;

	return true;
	}


/*******************************************************************************************
 * Routine to write out a alrm chunk for the Control Subscriber. This is done after the 
 * a goto chunk.
 *******************************************************************************************/
static bool	WriteAlrmChunk( WeaveParamsPtr pb, uint32* bytesRemaining, uint32 timeValue )
	{
	ControlChunk	Alrm_Chunk;

	/* Make sure there's space in the current block for the alrm chunk
	 * we are about to write. This should never happen because the
	 * previous chunk processing should have prevented this.
	 */
	if ( sizeof(ControlChunk) > (*bytesRemaining) )					
		{
		fprintf( stderr, "WriteAlrmChunk() - not enough space!\n" );
		return false;
		}

	/* Format the Control Subscriber 'ALRM' chunk for output. 
	 * Time is the absolute time. This time should already
	 * include the output stream base time to create the desired
	 * time bias in the output stream.
	 */
	Alrm_Chunk.chunkType		= CTRL_CHUNK_TYPE;
	Alrm_Chunk.subChunkType		= ALRM_CHUNK_SUBTYPE;
	Alrm_Chunk.chunkSize		= sizeof(ControlChunk);
	Alrm_Chunk.channel			= 0;
	Alrm_Chunk.time				= timeValue; 
	Alrm_Chunk.u.alarm.options	= 0;
	Alrm_Chunk.u.alarm.newTime	= 0;

	/* Write the sync chunk to the output file */
 	if ( 1 != fwrite( &Alrm_Chunk, sizeof(ControlChunk), 1, pb->outputStreamFd ) )
	{
		fprintf(stderr, "WriteAlrmChunk() - error writing ALRM chunk\n");
		return false;
	}

	/* Update remaining free bytes in the theoretical output buffer.
	 */
	(*bytesRemaining) -= sizeof(ControlChunk);
	if ( (*bytesRemaining) == 0 )
		(*bytesRemaining) = pb->streamBlockSize;

	return true;

	} /* end WriteAlrmChunk */
	
/*******************************************************************************************
 * Routine to write out the filler chunk.
 *******************************************************************************************/
static bool WriteFillerChunk( WeaveParamsPtr pb, uint32* bytesRemaining )

	{
	/* Write a filler block at the end of the current block. */
	pb->fillerChunk->chunkSize	= *bytesRemaining;

	if ( 1 != fwrite( pb->fillerChunk, *bytesRemaining, 1, pb->outputStreamFd ) )
	{
		fprintf(stderr, "WriteFillerChunk() - error writing Filler chunk\n");
		return false;
	}

	/* Reset free buffer space variable */
	*bytesRemaining = pb->streamBlockSize;
	
	return true;

	}
				

/*******************************************************************************************
 * Routine to write out a goto chunk.
 *******************************************************************************************/
static bool WriteGotoChunk
	( WeaveParamsPtr pb, InStreamPtr isp, uint32* bytesRemaining )
	{

	uint32				bytesToWrite = 0;	
	StrmChunkNodePtr	tempPtr;

	/* Write out any last goto chunk or write out the goto chunk if
	 * the "earliest" chunk found is after the goto chunk time.
	 */
	 if ( ( isp->buf.time < pb->gotoChunkList->chunk.gotoChk.time ) &&
	 	 ( !isp->eof ) )
	 	 goto DONE;

	/* Make sure there's space in the current block for the 2 chunks
	 * we are about to write.	 */
	if ( sizeof(StreamGoToChunk) > (*bytesRemaining) )
	{
		/* Write a filler block at the end of the current block. */
		if ( ! WriteFillerChunk( pb, bytesRemaining ) )
			return false;
	}

	/* Add the output stream base time to the time in the chunk
	 * to create the desired time bias in the output stream.
	 */
	 pb->gotoChunkList->chunk.gotoChk.time += pb->outputBaseTime;

	/* Write out the goto chunk */
	bytesToWrite = sizeof(StreamGoToChunk);		
	if ( 1 != fwrite( &pb->gotoChunkList->chunk.gotoChk, bytesToWrite, 1, pb->outputStreamFd ) )
	{
		fprintf(stderr, "WriteGotoChunk() - error writing GOTO chunk\n");
		return false;
	}

	/* Update remaining free bytes in the theoretical output buffer. */
	(*bytesRemaining) -= bytesToWrite;
	if ( ( *bytesRemaining) == 0 )
		(*bytesRemaining) = pb->streamBlockSize;

	/* Write a filler block at the end of the current block. */
	else if ( ! WriteFillerChunk( pb, bytesRemaining ) )
			return false;

	/* Delete the current Goto Chunk object that was just written */
	tempPtr					= pb->gotoChunkList->next;
	free ( pb->gotoChunkList );
	pb->gotoChunkList		= tempPtr;
	if ( NULL == pb->gotoChunkList )
		pb->fWriteGotoChunk = false;
	
DONE:	
	return true;

	}
	
/*******************************************************************************************
 * Routine to write out a stop chunk.
 *******************************************************************************************/
static bool WriteStopChunk
	( WeaveParamsPtr pb, InStreamPtr isp, uint32* bytesRemaining )
	{

	uint32			bytesToWrite;	
	StrmChunkNodePtr	tempPtr;

	/* Write out any last stop chunk or write out the stop chunk if
	 * the "earliest" chunk found is after the stop chunk time.
	 */
	 if ( ( isp->buf.time < pb->stopChunkList->chunk.stopChk.time ) &&
		 ( !isp->eof ) )
		 goto DONE;

	/* Make sure there's space in the current block for the chunk
	 * we are about to write.	 */
	if ( sizeof(StreamStopChunk) > (*bytesRemaining) )

		/* Write a filler block at the end of the current block. */
		if ( ! WriteFillerChunk( pb, bytesRemaining ) )
			return false;
		
	/* Add the output stream base time to the time in the chunk
	 * to create the desired time bias in the output stream.
	 */
	 pb->stopChunkList->chunk.stopChk.time += pb->outputBaseTime;

	/* Write out the stop chunk */
	bytesToWrite = sizeof(StreamStopChunk);		

	if ( 1 != fwrite( &pb->stopChunkList->chunk.stopChk, bytesToWrite, 1, pb->outputStreamFd ) )
	{
		fprintf(stderr, "WriteStopChunk() - error writing stop chunk\n");
		return false;
	}

	/* Update remaining free bytes in the theoretical output buffer. */
	(*bytesRemaining) -= bytesToWrite;
	if ( ( *bytesRemaining) == 0 )
		(*bytesRemaining) = pb->streamBlockSize;

	/* Write a filler block at the end of the current block. */
	else if ( ! WriteFillerChunk( pb, bytesRemaining ) )
			return false;


	/* Delete the current Stop Chunk object that was just written */
	tempPtr					= pb->stopChunkList->next;
	free ( pb->stopChunkList );
	pb->stopChunkList		= tempPtr;
	if ( NULL == pb->stopChunkList )
		pb->fWriteStopChunk = false;

DONE:
	return true;

	}


/*******************************************************************************************
 * Routine to write out a halt chunk.
 *
 * NOTE: This routine presently disabled.
 *
 *******************************************************************************************/
static bool WriteHaltChunk( WeaveParamsPtr pb, InStreamPtr isp, uint32* bytesRemaining )
	{

	uint32			bytesToWrite;	
	HaltChunksPtr	tempPtr;

	/* Write out any last Halt chunk or write out the Halt chunk if
	 * the "earliest" chunk found is after the Halt chunk time.
	 */
	 if ( ( isp->buf.time >= pb->haltChunkList->dataChunk.streamerChunk.time ) ||
		 ( isp->eof ) )
		{

		/* Make sure there's space in the current block for the chunk
		 * we are about to write.	 */
		if ( sizeof(HaltChunk) > (*bytesRemaining) )

			/* Write a filler block at the end of the current block. */
			if ( ! WriteFillerChunk( pb, bytesRemaining ) )
				return false;
			
		/* Add the output stream base time to the time in the chunk
		 * to create the desired time bias in the output stream.
		 */
		 pb->haltChunkList->dataChunk.streamerChunk.time += pb->outputBaseTime;

		/* Write out the Halt chunk */
		bytesToWrite = sizeof(HaltChunk);		

		if ( 1 != fwrite( &pb->haltChunkList->dataChunk, bytesToWrite, 1, pb->outputStreamFd ) )
		{
			fprintf(stderr, "WriteHaltChunk() - error writing Halt chunk\n");
			return false;
		}

		/* Update remaining free bytes in the theoretical output buffer. */
		(*bytesRemaining) -= bytesToWrite;
		if ( ( *bytesRemaining) == 0 )
			(*bytesRemaining) = pb->streamBlockSize;

		/* Delete the current Halt Chunk object that was just written */
		tempPtr					= pb->haltChunkList->next;
		free ( pb->haltChunkList );
		pb->haltChunkList		= tempPtr;
		if ( NULL == pb->haltChunkList )
			pb->fWriteHaltChunk = false;

	} /* end if write Halt chunk */
	
	return true;

	} /* End WriteHaltChunk */



static bool	
WriteCurrentChunk( WeaveParamsPtr pb, InStreamPtr isp, uint32 *remainingInCurrentBlock )
{
	bool		status = true;	/* assume success */

	/* Write out a goto control chunk, if the user has specified that one
	 * should be output.
	 */ 
	if ( pb->fWriteGotoChunk )
		{
		if ( ! WriteGotoChunk( pb, isp, remainingInCurrentBlock ) )
			{
			fprintf( stderr, "WriteCurrentChunk() - Error writing goto chunk!\n" );
			status = false;
			goto BAILOUT;
			}
		
		} /*if fWriteGotoChunk */
	 
	/* Write out a stop control chunk, if the user has specified that one
	 * should be output.
	 */ 
	if ( pb->fWriteStopChunk )
		{
		if ( ! WriteStopChunk(pb, isp, remainingInCurrentBlock) )
			{
			fprintf( stderr, "WriteCurrentChunk() - Error writing stop chunk!\n" );
			status = false;
			goto BAILOUT;
			}
		} /*if fWriteStopChunk */ 

	/* Write out a halt chunk, if the user has specified that one
	 * should be output.
	 */ 
	if ( pb->fWriteHaltChunk )
		{
		if ( ! WriteHaltChunk( pb, isp, remainingInCurrentBlock ) )
			{
			fprintf( stderr, "WriteCurrentChunk() - Error writing halt chunk!\n" );
			status = false;
			goto BAILOUT;
			}
		} /*if fWriteHaltChunk */ 

	if ( ! WriteChunk( pb, isp, remainingInCurrentBlock ) )
		{
		status = false;
		fprintf( stderr, "WriteCurrentChunk() - Error writing data chunk!\n" );
		goto BAILOUT;
		}

BAILOUT:
	return status;
}


/*==========================================================================================
  ==========================================================================================
										Public Routines
  ==========================================================================================
  ==========================================================================================*/


/*******************************************************************************************
 * Routine to open all the stream files. The file name pointers have been
 * set up before we get here.
 *******************************************************************************************/
bool		OpenDataStreams( WeaveParamsPtr pb )
	{
	InStreamPtr	isp = pb->inStreamList;
	bool		success = false;
	
	/* Sort the marker array in ascending time order.
	 */
	if ( pb->numMarkers > 1 )
		SortMarkers( pb );

	/* Open the output stream file
	 */
	pb->outputStreamFd = fopen( pb->outputStreamName, "wb+" );
	if ( NULL == pb->outputStreamFd )
		{
		fprintf (stderr, "unable to open output data stream file: %s\n", 
					pb->outputStreamName );
		goto DONE;
		}
		
#if USE_BIGGER_BUFFERS
	if ( setvbuf( pb->outputStreamFd, GetIOBuffer( pb->ioBufferSize ), _IOFBF, pb->ioBufferSize ) )
		{
		fprintf (stderr, "setvbuf() failed for file: %s\n", pb->outputStreamName );
		goto DONE;
		}
#endif

	/* Open all of the input stream files
	 */
	for ( ; NULL != isp; isp = isp->next )
		{

#if CACHE_OPEN_FILES
	/* make sure the file we're about to read from is open... */
	if ( false == OpenInputStream(pb, isp) )
		return false;
#else
		isp->fd = fopen( isp->fileName, "rb" );
		if ( NULL == isp->fd )
			{
			fprintf (stderr, "unable to open input data stream file: %s\n", 
								isp->fileName );
			goto DONE;
			}
#endif

#if USE_BIGGER_BUFFERS
		if ( setvbuf( isp->fd, GetIOBuffer( pb->ioBufferSize ), _IOFBF, pb->ioBufferSize ) )
			{
			fprintf (stderr, "setvbuf() failed for file: %s\n", isp->fileName );
			goto DONE;
			}
#endif

#if CACHE_OPEN_FILES
			/* in the case where we don't keep all files open, read the header first chunk header */ 
			/*  and close it */
			if ( ! ReadChunkHeader( pb, isp ) )
				goto DONE;
#endif	/* #if CACHE_OPEN_FILES */

		} /*end for loop */
		
	success = true;
DONE:

	return success;
	}


/*******************************************************************************************
 * Routine to close all the open stream files.
 *******************************************************************************************/
void		CloseDataStreams( WeaveParamsPtr pb )
	{
	InStreamPtr	isp = pb->inStreamList;

	/* Close the output file */
	if ( pb->outputStreamFd )
		fclose( pb->outputStreamFd );

	/* All of the input stream files are
	 * closed already.
	 */

	/* Free the scratch buffer memory */
	if ( pb->scratchBuf )
		free( pb->scratchBuf );

	} /* end CloseDataStreams */


/*******************************************************************************************
 * Routine to merge all input data streams into a single output data stream. The
 * parameter block contains all the info we need to do this already set up for us.
 *
 *	Rules for output chunk selection:
 *	---------------------------------
 *	1.	write the lowest TIME chunk that will fit into the current block
 *		whose time is below the next MARKER
 *
 *	2.	write any chunk that will fit into the current block
 *		whose time is below the next MARKER
 *
 *	3.	write a FILLER chunk
 *
 *	while there's any data remaining in the input collection
 *		get next chunk as selected by lowest TIME value
 *		if the selected chunk will fit into the current block
 *				&& the chunk's TIME is below the next MARKER value
 *					write the selected chunk
 *		else if there's another chunk in the input collection that will fit into the current block
 *				&& the chunk's TIME is below the next MARKER value
 *					write that block
 *		else
 *			write a FILLER block
 *
 *******************************************************************************************/
/*
 * NOTE: Writing of control SYNC chunks has been disabled by the
 *		 #if 0 comment.  Code concerning SYNC chunks should all be
 *		 removed, now that CTRL subscriber is history.
 */
long		WeaveStreams( WeaveParamsPtr pb )
	{
	InStreamPtr			isp;
	uint32 				remainingInCurrentBlock;
	long				status;
	long				markerChunkFPOS;

	/* Start marker processing at the first entry */
	pb->currMarkerIndex = 0;

	/* Allocate the output stream's scratch buffer. This buffer
	 * is used to copy data from input streams to the output stream.
	 */
	pb->scratchBuf = (char 	*)malloc(pb->streamBlockSize);
	if ( NULL == pb->scratchBuf )
		{
		status = -1;
		goto BAILOUT;
		}

	/* Allocate the "filler chunk" buffer. This is allocated to accomodate
	 * the largest possible filler chunk. As we need to write out fillers,
	 * we stuff the *actual* filler chunk size into the header of this buffer
	 * and write out only what we need. 
	 */
	remainingInCurrentBlock = pb->streamBlockSize;
	pb->fillerChunk = (SubsChunkDataPtr) malloc( pb->streamBlockSize );
	if ( NULL == pb->fillerChunk )
		{
		fprintf( stderr, "WeaveStreams() - Error allocating a filler chunk!\n" );
		status = -1;
		goto BAILOUT;
		}

	/* Initialize the filler buffer */
	memset( pb->fillerChunk, 0, pb->streamBlockSize );
	pb->fillerChunk->chunkType	= FILL_CHUNK_TYPE;
	pb->fillerChunk->chunkSize	= 0;
	pb->fillerChunk->time		= 0;

#if CACHE_OPEN_FILES
	/* in the case where we don't keep all files open, we pre-cache all headers */
	/*  when the files are first opened */
#else
	/* Pre-load chunk headers from all input streams */
	for ( isp = pb->inStreamList ; NULL != isp; isp = isp->next )
		if ( ! ReadChunkHeader( pb, isp ) )
			{
			fprintf( stderr, "WeaveStreams() - Error pre-loading chunk headers!\n" );
			status = -1;	
			goto BAILOUT;
			}
#endif


#if !FORCE_FIRST_DATA_ONTO_STREAMBLOCK_BOUNDARY
	/* we're NOT going to keep userdata out of the first stream block, but if we will
	 *  be writing a marker table we need to point the first marker to the begining
	 *  of the file
	 */
	if ( pb->fWriteMarkerTable )
	{
		if ( false == UpdateMarkerTable(pb) )
			{
			status = -10;
			goto BAILOUT;
			}
	}
#endif

	/* Write out a stream header chunk if the user has specified that one
	 * should be output.
	 */
	if ( ! WriteHeaderChunk( pb, &remainingInCurrentBlock ) )
		{
		fprintf( stderr, "WeaveStreams() - Error writing streaming header chunk!\n" );
		status = -1;
		goto BAILOUT;
		}		

	/* Write out the marker table, this time as a place holder since none of
	 * its entries will have been filled in yet.
	 */
	markerChunkFPOS = pb->streamBlockSize - remainingInCurrentBlock;

	if ( ! WriteMarkerTable( pb, &remainingInCurrentBlock ) )
		{
		fprintf( stderr, "WeaveStreams() - Error placing a place holder for marker table!\n" );
		status = -1;
		goto BAILOUT;
		}

#if FORCE_FIRST_DATA_ONTO_STREAMBLOCK_BOUNDARY
	/* Force input stream data onto the next stream block boundary so that branching
	 * to time zero can be done meaningfully. If options dictate that neither a header
	 * nor a marker table have been written to the output stream, then skip writing
	 * a filler chunk.
	 */
	if ( pb->fWriteStreamHeader || pb->fWriteMarkerTable )
		{

#if 1
		if ( ! WriteFillerChunk(pb, &remainingInCurrentBlock) )
#else
		pb->fillerChunk->chunkSize	= remainingInCurrentBlock;
		if ( 1 != fwrite( pb->fillerChunk, remainingInCurrentBlock, 1, pb->outputStreamFd ) )
#endif
			{
			fprintf( stderr, "WeaveStreams() - Error writing filler chunk!\n" );
			status = -1;
			goto BAILOUT;
			}
		remainingInCurrentBlock = pb->streamBlockSize;
		}
#endif

	/*************************************************************************/
	/* Main loop to merge all input streams into a single output stream file */
	/*************************************************************************/
	while ( InputDataRemaining( pb ) )
		{
		SpinCursor(32);
		
		if ( NULL != (isp = GetEarliestBelowNextMarker( pb, remainingInCurrentBlock )) )
			{
			/* Write out all control chunks who's time has come up (goto, stop,
			 *  halt, fill) and then the current data chunk
			 */ 
			if ( ! WriteCurrentChunk( pb, isp, &remainingInCurrentBlock ) )
				{
				status = -1;
				goto BAILOUT;
				}
			} /* else-if GetEarliestBelowNextMarker != NULL */

		else if ( NULL != (isp = GetBestFitBelowNextMarker( pb, remainingInCurrentBlock )) )
			{
			/* Write out all control chunks who's time has come up (goto, stop,
			 *  halt, fill) and then the current data chunk
			 */ 
			if ( ! WriteCurrentChunk( pb, isp, &remainingInCurrentBlock ) )
				{
				status = -1;
				goto BAILOUT;
				}
			} /* else-if GetBestFitBelowNextMarker != NULL */

		else
			/* Can't find anything to fit into the current block. This can be
			 * because there isn't enough space to write the next chunk, or
			 * because we're hitting a marker and need to get all blocks in the
			 * output stream to align in time to the next marker. Hence, we
			 * write a filler block at the end of the current block.
			 */
			{
#if 1
			if ( ! WriteFillerChunk(pb, &remainingInCurrentBlock) )
#else
			pb->fillerChunk->chunkSize	= remainingInCurrentBlock;
			if ( 1 != fwrite( pb->fillerChunk, remainingInCurrentBlock, 1, pb->outputStreamFd ) )
#endif
				{
				fprintf( stderr, "WeaveStreams() - Error writing filler chunk!\n" );
				status = -1;
				goto BAILOUT;
				}

			/* Reset free buffer space variable */
			remainingInCurrentBlock = pb->streamBlockSize;
			}

		/* Update the marker table if one of the search processes 
		 * determined that it was necessary. This must be done after the
		 * optional writing of a filler chunk so that the file position
		 * reported by ftell() will be correct.
		 */
		if ( (pb->fWriteMarkerTable) && (pb->fUpdateMarkerTable) )
			{
			if ( false == UpdateMarkerTable(pb) )
				{
				status = -10;
				goto BAILOUT;
				}
			} /* if fUpdateMarkerTable */
		} /* end while */

		/* Write out any last stop or goto chunks.  There should only be one
		 * these chunk left since we are at the end.
		 */
		if ( pb->fWriteGotoChunk )
			{
			if ( ! WriteGotoChunk( pb, isp, &remainingInCurrentBlock ) )
				{
				fprintf( stderr, "WeaveStreams() - Error writing goto chunk!\n" );
				status = -1;
				goto BAILOUT;
				}
			} /*if fWriteGotoChunk */
	
		else if ( pb->fWriteStopChunk )
			{
			if ( ! WriteStopChunk( pb, isp, &remainingInCurrentBlock ) )
				{
				fprintf( stderr, "WeaveStreams() - Error writing stop chunk!\n" );
				status = -1;
				goto BAILOUT;
				}
			} /*if fWriteStopChunk */ 

		/* Write out a halt chunk, if the user has specified that one
		 * should be output.
		 */ 
		else if ( pb->fWriteHaltChunk )
			{
			if ( ! WriteHaltChunk( pb, isp, &remainingInCurrentBlock ) )
				{
				fprintf( stderr, "WeaveStreams() - Error writing halt chunk!\n" );
				status = -1;
				goto BAILOUT;
				}
			} /*if fWriteHaltChunk */ 

		if ( remainingInCurrentBlock != pb->streamBlockSize )
			{
			/* Write a filler chunk to fill out the end of the last buffer 
			 * we will write to the output stream if we currently have 
			 * an incomplete block.
			 */
#if 1
			if ( ! WriteFillerChunk(pb, &remainingInCurrentBlock) )
#else
			pb->fillerChunk->chunkSize	= remainingInCurrentBlock;

			if ( 1 != fwrite( pb->fillerChunk, remainingInCurrentBlock, 1, pb->outputStreamFd ) )
#endif
				{
				fprintf( stderr, "WeaveStreams() - Error writing filler chunk!\n" );
				status = -1;
				goto BAILOUT;
				}
			}

	/* Write out the marker table, this time the table will have all its entries
	 * filled in. First, seek back to the place where it was first written, and
	 * then OVERWRITE it in place.
	 */
		if ( 0 != fseek( pb->outputStreamFd, markerChunkFPOS, SEEK_SET ) )
			{
			fprintf( stderr, "WeaveStreams() - Error overwriting the marker table!\n" );
			status = -1;
			goto BAILOUT;
			}
	
		/* Fixed the following to pass in the correct bytesremaining in the first block.
		 *
		*/
		remainingInCurrentBlock = pb->streamBlockSize - markerChunkFPOS;
		if ( ! WriteMarkerTable( pb, &remainingInCurrentBlock ) )
			{
			fprintf( stderr, "WeaveStreams() - Error writing the marker table!\n" );
			status = -1;
			goto BAILOUT;
			}

	status = 0;

BAILOUT:
	if ( pb->fillerChunk )
		free(pb->fillerChunk);

	return status;
	
	} /* end WeaveStreams */
