/****************************************************************************
**
**  @(#) datachannels.c 96/11/27 1.6
**
*****************************************************************************/
#include <kernel/debug.h>
#include <kernel/semaphore.h>
#include <kernel/types.h>
#include <string.h>

#include <streaming/dserror.h>
#include <streaming/datastreamlib.h>
#include <streaming/subscribertraceutils.h>
#include <streaming/datasubscriber.h>

#include "datachannels.h"


/* private prototypes */
static void				FreeBlockAndData(PvtDataContextPtr ctx, DataHeadPtr headPtr);
static void				*DataSubAllocMem(PvtDataContextPtr ctx, int32 channel, uint32 chunkType, 
										uint32 memSize, uint32 memTypeBits);
static	void			AddToPartialCtxQueue(PvtDataContextPtr ctx, DataHeadPtr headPtr);
static	Err				DataCopyIsComplete(PvtDataContextPtr ctx, DataHeadPtr headPtr);
static	DataHeadPtr		FindBlockInPartialQueue(PvtDataContextPtr ctx, int32 channel, uint32 signature);
#ifdef DEBUG
static	uint32			ChecksumBuffer(uint32 *buffPtr, uint32 buffSize, uint32 currentCheckSum);
#endif
static	int32			StartDataCopy(PvtDataContextPtr ctx, DataChunkFirstPtr dataChunk);
static	int32			ContinueDataCopy(PvtDataContextPtr ctx, DataChunkPartPtr dataChunk);


/*
 * get the stream clock, return the time
 */
uint32 
GetStreamClock(PvtDataContextPtr ctx)
{
	DSClock			dsClock;

	DSGetPresentationClock(ctx->streamCBPtr, &dsClock);
	return dsClock.streamTime;
}


/*******************************************************************************************
 * has this channel been enabled?
 *******************************************************************************************/
Err
IsDataChanEnabled(PvtDataContextPtr ctx, int32 channelNumber)
{
	Err		err;

#if 1
	{
		uint32	word 	= channelNumber >> 5; 	/* @@ channelNumber / BITS_PER_SUBSET; */
		uint32	bit		= 1 << (channelNumber % BITS_PER_SUBSET);
		
		if ( channelNumber < kDATA_SUB_MAX_CHANNELS )
			err = ctx->chanBitMap[word] & bit;
		else
			err = kDSChanOutOfRangeErr;
	}
#else
	if ( channelNumber < kDATA_SUB_MAX_CHANNELS )
		err = ctx->chanBitMap[channelNumber >> 5] /* @@ channelNumber / BITS_PER_SUBSET */
				& (1 << (channelNumber % BITS_PER_SUBSET));
	else
		err = kDSChanOutOfRangeErr;
#endif

	return err;
}


/*******************************************************************************************
 * enable/disable a channel
 *******************************************************************************************/
Err
SetDataChanEnabled(PvtDataContextPtr ctx, int32 channelNumber, int32 enableChannel)
{
	Err		err = 0;

#if 1
	{
		uint32	word 	= channelNumber >> 5; 	/*@@ channelNumber / BITS_PER_SUBSET; */
		uint32	bit		= 1 << (channelNumber % BITS_PER_SUBSET);
		
		if ( channelNumber >= kDATA_SUB_MAX_CHANNELS )
		{
			err = kDSChanOutOfRangeErr;
			goto DONE;
		}
	
		if ( true == enableChannel )
		{
			ctx->chanBitMap[word] |= bit;
		}
		else
		{
			ctx->chanBitMap[word] &= ~bit;
		}
	}
#else
	if ( channelNumber >= kDATA_SUB_MAX_CHANNELS )
		err = kDSChanOutOfRangeErr;

	if ( true == enableChannel )
	{
		ctx->chanBitMap[channelNumber >> 5] /*@@ channelNumber / BITS_PER_SUBSET */
									|= (1 << (channelNumber % BITS_PER_SUBSET));
	}
	else
	{
		ctx->chanBitMap[channelNumber >> 5] /*@@ channelNumber / BITS_PER_SUBSET */
									&= ~(1 << (channelNumber % BITS_PER_SUBSET));
	}
#endif

DONE:
	return err;
}

/*******************************************************************************************
 * Disable further data flow for the specified channel.
 * Any queued data is flushed.
 *******************************************************************************************/
int32
FlushDataChannel(PvtDataContextPtr ctx, int32 channelNumber)
{
	int32				err = kDSNoErr;
	SubscriberMsgPtr	msgPtr;
	SubsQueue			tempMsgQueue;
	DataHeadPtr			headPtr;
	DataHeadPtr			prevPtr;
	DataHeadPtr			tempPtr;

	ADD_DATA_TRACE_L2(gDataTraceBufPtr, kTraceChannelFlush, channelNumber, 
						GetStreamClock(ctx), 0);

	if ( (kEVERY_DATA_CHANNEL == channelNumber) 
			|| (true == IsDataChanEnabled(ctx, channelNumber)) )
	{
		/* Acquire the context structure */
		if ( (err = LockSemaphore(ctx->ctxLock, SEM_WAIT)) < 0 )
		{
			ERROR_RESULT_STATUS("FlushDataChannel() - LockSemaphore()", err);
			goto DONE;
		}
		/* reset err as LockSemaphore() return 1 for success */
		err = kDSNoErr;

		/* Give back all queued chunks for this channel to the streamer and
		 *  reply to all the queued messages from the parser.  Because all chunks
		 *  on the queue MAY not be on this channel, push the chunks we're NOT returning
		 *  onto a temporary queue so we can reset the queue later 
		 */
		tempMsgQueue.head = NULL;
		tempMsgQueue.tail = NULL;
		while (	NULL != (msgPtr = GetNextDataMsg(&ctx->dataMsgQueue)) ) 	
		{
			/* if this chunk is on the channel we're flushing, reply to it, otherwise add
			 *  it to the tail of the temp queue 
			 */
			if ( (kEVERY_DATA_CHANNEL == channelNumber) 
				|| (channelNumber == ((SubsChunkDataPtr)msgPtr->msg.data.buffer)->channel) )
			{
				ADD_DATA_TRACE_L2(gDataTraceBufPtr, kFlushedDataMsg, channelNumber, 
									GetStreamClock(ctx) , msgPtr);
				ReplyToSubscriberMsg(msgPtr, kDSNoErr);
			}
			else
			{
				/* this chunk is NOT on the channel we're flushing, remember it on 
				 *  the temporary queue  
				 */
				AddDataMsgToTail(&tempMsgQueue, msgPtr);
			}
		}
		/* the context queue is now empty, restore whatever was left over */
		ctx->dataMsgQueue = tempMsgQueue;

		/* now, toss all partially completed blocks on the channel */
		headPtr = prevPtr = ctx->partialBlockList;
		while ( NULL != headPtr )
		{
			tempPtr = headPtr->nextHeadPtr;
			if ( (kEVERY_DATA_CHANNEL == channelNumber) || (channelNumber == headPtr->channel) )
			{
				/* unlink the block from the list */
				UnlinkHeader(headPtr, prevPtr, &ctx->partialBlockList);

				/* free the link and the block it points to */
				FreeBlockAndData(ctx, headPtr);
			}
			else
				prevPtr = headPtr;
			headPtr = tempPtr;
		}
		
		/* Release the channel context structure */
		UnlockSemaphore(ctx->ctxLock);
	}

DONE:
	return err;
}


/*******************************************************************************************
 * Free all completed but unclaimed data blocks
 *******************************************************************************************/
int32
FreeCompletedDataChannel(PvtDataContextPtr ctx, int32 channelNumber)
{
	int32				err = kDSNoErr;
	DataHeadPtr			headPtr;
	DataHeadPtr			prevPtr;
	DataHeadPtr			tempPtr;

	ADD_DATA_TRACE_L2(gDataTraceBufPtr, kTraceChannelFlush, channelNumber, 
						GetStreamClock(ctx), 0);

	if ( (kEVERY_DATA_CHANNEL == channelNumber) 
			|| (true == IsDataChanEnabled(ctx, channelNumber)) )
	{
		/* Acquire the context structure */
		if ( (err = LockSemaphore(ctx->ctxLock, SEM_WAIT)) < 0 )
		{
			ERROR_RESULT_STATUS("FreeCompletedDataChannel() - LockSemaphore()", err);
			goto DONE;
		}
		/* reset err as LockSemaphore() return 1 for success */
		err = kDSNoErr;

		/* free all complete but undelivered blocks on the channel */
		headPtr = prevPtr = ctx->completedBlockList;
		while ( NULL != headPtr )
		{
			tempPtr = headPtr->nextHeadPtr;
			if ( (kEVERY_DATA_CHANNEL == channelNumber) || (channelNumber == headPtr->channel) )
			{
				/* unlink the block from the list */
				UnlinkHeader(headPtr, prevPtr, &ctx->completedBlockList);

				/* free the link and the block it points to */
				FreeBlockAndData(ctx, headPtr);
			}
			else
				prevPtr = headPtr;
			headPtr = tempPtr;
		}
		
		/* Release the channel context structure */
		UnlockSemaphore(ctx->ctxLock);
	}

DONE:
	return err;
}


/*******************************************************************************************
 * Stop and Flush the specified channel; Then free up all it's resources.
 * Leave the channel in pre-initalized state.
 *******************************************************************************************/
int32		
CloseDataChannel(PvtDataContextPtr ctx, int32 channelNumber)
{
	int32	err = 0;
	
	ADD_DATA_TRACE_L2(gDataTraceBufPtr, kTraceChannelClose, channelNumber, 
						GetStreamClock(ctx), 0);

	/* If channel was never enabled and initalized, don't bother to flush */
	if ( (kEVERY_DATA_CHANNEL == channelNumber) 
			|| (true == IsDataChanEnabled(ctx, channelNumber)) )
	{
		/* Stop any activity and flush pending buffers */
		err = FlushDataChannel(ctx, channelNumber);
	}
		
	return err;
}



/*******************************************************************************************
 * Handle a data chunk that has arrived.
 *******************************************************************************************/
int32
ProcessNewDataChunk(PvtDataContextPtr ctx, SubscriberMsgPtr subMsg)
{
	int32				err;
	SubsChunkDataPtr	dataChunk;

	dataChunk 		= (SubsChunkDataPtr) subMsg->msg.data.buffer;
	if ( dataChunk->subChunkType == DATA_FIRST_CHUNK_TYPE )
		ADD_DATA_TRACE_L3(gDataTraceBufPtr, kDataChunkPiece1Arrived, dataChunk->channel, 
							GetStreamClock(ctx), dataChunk->time);
	else
		ADD_DATA_TRACE_L3(gDataTraceBufPtr, kDataChunkPieceNArrived, dataChunk->channel, 
							GetStreamClock(ctx), dataChunk->time);

	/* Acquire the channel context structure */
	if ( (err = LockSemaphore(ctx->ctxLock, SEM_WAIT)) < 0 )
	{
		ERROR_RESULT_STATUS("ProcessNewChunk() - LockSemaphore()", err);
		goto DONE;
	}

	/* Both header and data chunks are queued in order of arrival */
	AddDataMsgToTail(&ctx->dataMsgQueue, subMsg);

	UnlockSemaphore(ctx->ctxLock);

	/* run the data queue in case it has data on disabled channels */
	err = ProcessMessageQueue(ctx);

DONE:
	return err;
}



/* ------------------------ private subscriber functions ----------------------------------- */


/*******************************************************************************************
 * unlink a header from it's list.
 *******************************************************************************************/
void
UnlinkHeader(DataHeadPtr headPtr, DataHeadPtr prevHeadPtr, DataHeadPtr *listFirst)
{
	if ( headPtr == *listFirst )
		*listFirst = headPtr->nextHeadPtr;
	else
		prevHeadPtr->nextHeadPtr = headPtr->nextHeadPtr;
}


/*******************************************************************************************
 * free the memory allocated for a header and the data block it points to.
 *******************************************************************************************/
static void
FreeBlockAndData(PvtDataContextPtr ctx, DataHeadPtr headPtr)
{
	if ( NULL != headPtr )
	{
		uint32	time = GetStreamClock(ctx);

		if ( NULL != headPtr->dataPtr )
			ctx->freeMemFcn(headPtr->dataPtr, DATA_HEADER_CHUNK_TYPE, headPtr->channel, time);
		ctx->freeMemFcn(headPtr, DATA_BLOCK_CHUNK_TYPE, headPtr->channel, time);
	}
}


/*******************************************************************************************
 * allocate a chunk of memory using the user supplied function.  remember that we've
 *  used the function so we don't let the user reset it.
 *******************************************************************************************/
static void *
DataSubAllocMem(PvtDataContextPtr ctx, int32 channel, uint32 chunkType, uint32 memSize, 
				uint32 memTypeBits)
{
	void	*ptr;
	
	/* if we get the memory we ask for, remember that we've used the function
	 *   so the allocator fcn ptr can't be reset later
	 */
	if ( NULL != (ptr = ((ctx->allocMemFcn)(memSize, memTypeBits, chunkType, 
							channel, GetStreamClock(ctx)))) )
		SetFlag(ctx->flags, DATA_MEM_ALLOCED);

	return ptr;
}


/*******************************************************************************************
 * link a partial data block into the "partial" queue
 *******************************************************************************************/
static void
AddToPartialCtxQueue(PvtDataContextPtr ctx, DataHeadPtr headPtr)
{
	/* link it into the partial context list */
	headPtr->nextHeadPtr = ctx->partialBlockList;
	ctx->partialBlockList = headPtr;
}


/*******************************************************************************************
 * find a partial data block in the "partial" queue, remove it
 *******************************************************************************************/
static DataHeadPtr
FindBlockInPartialQueue(PvtDataContextPtr ctx, int32 channel, uint32 signature)
{
	DataHeadPtr headPtr;
	DataHeadPtr prevPtr;
	
	headPtr = prevPtr = ctx->partialBlockList;
	while ( NULL != headPtr )
	{
		if ( (channel == headPtr->channel) && (signature == headPtr->signature) )
		{
			/* unlink the block from the list */
			UnlinkHeader(headPtr, prevPtr, &ctx->partialBlockList);
			break;
		}

		prevPtr = headPtr;
		headPtr = headPtr->nextHeadPtr;
	}
	
	return headPtr;
}



/*******************************************************************************************
 * all data for a block has been copied out of the stream.  finish things up so the client
 *  can have the data
 *******************************************************************************************/
static Err
DataCopyIsComplete(PvtDataContextPtr ctx, DataHeadPtr headPtr)
{
	Err			err = 0;
	DataHeadPtr	*lastBlockPtrPtr;

#ifdef DEBUG
	/* in debug mode, make sure the checksum is the same as we expect */
	if ( headPtr->checkSum != headPtr->signature )
	{
		err = kDataChunkInvalidChecksum;
		ERROR_RESULT_STATUS("Data Subscriber:DataCopyIsComplete()", err);
	}
#endif


	/* if the block was compressed, delete the decompressor */
	if ( NULL != headPtr->decompressor )
	{
#ifdef DEAL_WITH_COMPFOLIO_ERROR
		/* see note in OutputDecompressedWord() to explain this */
		headPtr->badWriteCount = 0;
#endif
		if ( ((err = DeleteDecompressor(headPtr->decompressor)) < 0)
			|| ((err = headPtr->decompErr) < 0) )
		{
			ERROR_RESULT_STATUS("Data Subscriber:DataCopyIsComplete()", err);
		}
	}
	
	/* link a partial data block into the "complete" queue.  stick it onto the end
	 *  of the queue in case it isn't collected before something placed there earlier
	 */
	lastBlockPtrPtr = &ctx->completedBlockList;
	while ( NULL != *lastBlockPtrPtr )
		lastBlockPtrPtr = &((*lastBlockPtrPtr)->nextHeadPtr);
	headPtr->nextHeadPtr = *lastBlockPtrPtr;
	*lastBlockPtrPtr = headPtr;
	
	return err;
}


/*******************************************************************************************
 * write a word (or less) of decompressed data into the buffer
 *******************************************************************************************/
static void
OutputDecompressedWord(DataHeadPtr headPtr, uint32 word)
{
	if ( headPtr->dataWriteAddr <= (headPtr->dataPtr + headPtr->totalDataSize))
	{
   		uint32	bytesToWrite = (headPtr->dataPtr + headPtr->totalDataSize) - headPtr->dataWriteAddr;
    	if ( bytesToWrite >= 4 )
			*(uint32 *)headPtr->dataWriteAddr = word;
    	else
    	{
	    	/* if this is the last longword of uncompressed data, make sure we don't write
	    	 *  off the end of our buffer
	    	 */
			uint8	*writeAddr		= headPtr->dataWriteAddr;
			uint8	*dataToWrite	= (uint8 *)&word;

   			/* note that these case fall through (no "break;") on purpose */
    		switch ( bytesToWrite )
    		{
    			case 3:	*writeAddr++	= *dataToWrite++;
    			case 2:	*writeAddr++	= *dataToWrite++;
    			case 1:	*writeAddr		= *dataToWrite;
    		}
    	}
		
		headPtr->dataWriteAddr	+= sizeof(uint32);
	}
    else
    {
#ifdef DEAL_WITH_COMPFOLIO_ERROR
		/* the compession folio is broken so that it SOMETIMES calls this function
		 *  with one extra word of bogus data when we delete the decompressor, triggering
		 *  a bogus error message as we have already written to the end of our allocated
		 *  buffer.  unfortunately, we can't just ignore attempted writes past the end
		 *  of the buffer because if we feed it a bad compressed buffer (corrupt data,  
		 *  off in the bitstream, etc) the folio may also attempt to write too much data,
		 *  and in this case we will need to alert the user.  
		 * SO, until the folio is fixed, keep we have an extra word in the data header 
		 *  which we will use to count the number of attempted writes past the end of the
		 *  buffer, we'll assume an actual data error if this function is called and the count
		 *  is greater than 0.  because ANY attempted write outside of the buffer is an error
		 *  during normal decompression, we set it to 1 before we begin data decompression
		 *  so that ANY attempted bogus writes will flag an error.  Just before calling 
		 *  DeleteCompressor() we set it to 0, and allow exactly ONE attempted write past the  
		 *  end of the buffer without flagging an error condition
		 */
		if ( ++headPtr->badWriteCount > 1 )
			headPtr->decompErr = COMP_ERR_OVERFLOW;
#else
		headPtr->decompErr = COMP_ERR_OVERFLOW;
#endif
    }

}


/*******************************************************************************************
 * create a decompessor engine for a block of data
 *******************************************************************************************/
static Err
CreateDataDecompressor(DataHeadPtr headPtr)
{
	TagArg		tags[2];
	Err			err;

	/* create & init a decompressor... */
	headPtr->decompErr = 0;
	tags[0].ta_Tag = COMP_TAG_USERDATA;
	tags[0].ta_Arg = (void *)headPtr;
	tags[1].ta_Tag = TAG_END;
	if ( (err = CreateDecompressor(&headPtr->decompressor, (CompFunc)OutputDecompressedWord, tags)) < 0 )
	{
		ERROR_RESULT_STATUS("Data Subscriber:CreateDataDecompressor() CreateDecompressor() failed", err);
	}

	return err;
}


#ifdef DEBUG
/*******************************************************************************************
 * generate an XOR checksum for a buffer
 *******************************************************************************************/
static uint32
ChecksumBuffer(uint32 *buffPtr, uint32 buffSize, uint32 currentCheckSum)
{
	uint32	numLongs = buffSize / sizeof(uint32);

	/* Calculate an XOR checksum of LONG words */
	while ( numLongs-- > 0 )
		currentCheckSum ^= *buffPtr++;
	return currentCheckSum;
}
#endif


/*******************************************************************************************
 * allocate memory for a block and copy the first piece from the stream
 *******************************************************************************************/
static int32
StartDataCopy(PvtDataContextPtr ctx, DataChunkFirstPtr dataChunk)
{
	int32			err = kDSNoErr;
	DataHeadPtr		headPtr;

	ADD_DATA_TRACE_L3(gDataTraceBufPtr, kDataTraceStartDataCopy, dataChunk->channel, 
						GetStreamClock(ctx), dataChunk->time);

#ifdef	DEBUG
	/* sanity check the chunk size */
	if ( dataChunk->totalDataSize <= 0 )
	{
		PERR(("Data Subscriber:StartDataCopy() DATA header has impossible size field = %ld\n", dataChunk->totalDataSize));
		err = kDataChunkInvalidSize;
		headPtr = NULL;
		goto ERROR_EXIT;
	}
#endif
	
	/* allocate memory for the header */
	headPtr = (DataHeadPtr)DataSubAllocMem(ctx, dataChunk->channel, DATA_HEADER_CHUNK_TYPE,
										sizeof(DataHead), MEMTYPE_ANY);
	if ( NULL == headPtr )
	{
		PERR(("Data Subscriber:StartDataCopy() can't allocatge %ld bytes for data header\n", dataChunk->totalDataSize));
		err = kDSNoMemErr;
		goto ERROR_EXIT;
	}
	
	/* allocate memory large enough for the whole data chunk */
	headPtr->dataPtr = (uint8 *)DataSubAllocMem(ctx, dataChunk->channel, DATA_BLOCK_CHUNK_TYPE, 
								dataChunk->totalDataSize, dataChunk->memTypeBits);
	if ( NULL == headPtr->dataPtr )
	{
		PERR(("Data Subscriber:StartDataCopy() can't allocatge %ld bytes for data chunk\n", 
					dataChunk->totalDataSize));
		goto ERROR_EXIT;
	}

	/* setup the rest of the header ptr... */
	headPtr->signature		= dataChunk->signature;
	headPtr->channel		= dataChunk->channel;
	headPtr->pieceCount		= dataChunk->pieceCount;
	headPtr->userData[0]	= dataChunk->userData[0];
	headPtr->userData[1]	= dataChunk->userData[1];
	headPtr->totalDataSize	= dataChunk->totalDataSize;
	headPtr->dataWriteAddr	= headPtr->dataPtr;
	headPtr->decompressor	= NULL;

#ifdef DEAL_WITH_COMPFOLIO_ERROR
	/* see note in function OutputDecompressedWord to explain this */
	headPtr->badWriteCount	= 1;
#endif

#ifdef DEBUG
	/* in debug mode, checksum the buffer */
	headPtr->checkSum 		= ChecksumBuffer(
				(uint32 *)((uint8 *)dataChunk + sizeof(DataChunkFirst)),	/* start of buffer */
				dataChunk->dataSize, 										/* size of this chunk */
				0);															/* current checksum */
#endif
	
	/* copy the data chunk into the memory we just allocated, either directly or via
	 *  a decompressor 
	 */
	 if ( DATA_NO_COMPRESSOR == dataChunk->compressor )
	 {
		memcpy(headPtr->dataPtr, 							/* where to copy it */
				(uint8 *)dataChunk + sizeof(DataChunkFirst),	/* start of data to copy */ 
				dataChunk->dataSize);						/* amount to copy */
		headPtr->dataWriteAddr += dataChunk->dataSize;
	 }
	 else
	 {
		/* create the compressor... */
		if ( (err = CreateDataDecompressor(headPtr)) < 0 )
			goto DONE;

		/* and call the decompressor. the first word of a file written by comp3DO is the length of
		 *  the compressed data, so don't bother showing that to the folio
		 */
		err = FeedDecompressor(headPtr->decompressor, 
								(uint8 *)dataChunk + sizeof(DataChunkFirst),
								dataChunk->dataSize / sizeof(uint32));
		if ( err > 0 )
			err = headPtr->decompErr;
		if ( err < 0 )
		{
			ERROR_RESULT_STATUS("Data Subscriber:CreateDataDecompressor() error in FeedDecompressor()\n", err);
			goto DONE;
		}
	 }
	
	/* if the chunk fit into the header, we're done */
	if ( 1 == dataChunk->pieceCount )
	{
		/* move it onto the list of data to deliver */
		err = DataCopyIsComplete(ctx, headPtr);
	}
	else
	{
		/* add the block to the partial list, it will remain there until it is done */
		headPtr->nextChunkNum = 2;
		AddToPartialCtxQueue(ctx, headPtr);
	}
	
DONE:
	return err;

ERROR_EXIT:

	FreeBlockAndData(ctx, headPtr);
	goto DONE;
}


/*******************************************************************************************
 * Copy another piece of data from the stream into channel local memory
 *******************************************************************************************/
static int32
ContinueDataCopy(PvtDataContextPtr ctx, DataChunkPartPtr dataChunk)
{
	int32			err = kDSNoErr;
	DataHeadPtr		headPtr;

	ADD_DATA_TRACE_L3(gDataTraceBufPtr, kDataTraceContinueCopy, dataChunk->channel, 
						GetStreamClock(ctx), dataChunk->time);
	
	/* get the first part of this piece */
	if ( NULL == (headPtr = FindBlockInPartialQueue(ctx, dataChunk->channel, dataChunk->signature)) )
	{
		PERR(("Data Subscriber:ContinueDataCopy() DATn piece %ld recieved out of order on channel %ld\n",
					dataChunk->pieceNum, dataChunk->channel));
		err = kDataChunkNotFound;
		goto DONE;
	}
	
	/* sanity check the piece number, it must be one greater than the last piece we copied.  [we 
	 *  COULD allow data to be delivered out order by including the actual byte offset within the
	 *  stream data, but I'm assuming that the weaver isn't smart enough to be able to rearrange
	 *  blocks without direction from the operator, so it isn't necessary]
	 */
	if ( dataChunk->pieceNum != headPtr->nextChunkNum )
	{
		PERR(("Data Subscriber:ContinueDataCopy() DATn chunk #%ld recieved, expecting chunk #%ld\n", 
					dataChunk->pieceNum, headPtr->nextChunkNum));
		err = kDataChunkRecievedOutOfOrder;
		goto DONE;
	}
	
#ifdef DEBUG
	/* in debug mode, checksum the buffer */
	headPtr->checkSum = ChecksumBuffer(
				(uint32 *)((uint8 *)dataChunk + sizeof(DataChunkPart)), 	/* start of buffer */
				dataChunk->dataSize, 
				headPtr->checkSum);											/* current checksum */
#endif
	
	/* bounds checks are OK, copy it into our block */
	if ( NULL == headPtr->decompressor )
	{
#ifdef	DEBUG
		/* Check to see if we are about to write off the end of the chunk of memory 
		 *  we allocated
		 */
		if ( (headPtr->dataWriteAddr + dataChunk->dataSize) > 
				(headPtr->dataPtr + headPtr->totalDataSize))
		{
			PERR(("Data Subscriber:ContinueDataCopy() DATn chunk copy would overflow buffer.  Channel %ld, chunk %ld/%ld.",
						dataChunk->channel, dataChunk->pieceNum, headPtr->pieceCount));
			err = kDataChunkOverflow;
			goto DONE;
		}
#endif	/* DEBUG */

		memcpy(headPtr->dataWriteAddr,				/* dest (after current data) */
				(uint8 *)dataChunk + sizeof(DataChunkPart),	/* source (after chunk header)  */
				dataChunk->dataSize);
		headPtr->dataWriteAddr += dataChunk->dataSize;
	}
	else
	{
		err = FeedDecompressor(headPtr->decompressor,				/* decompression engine */
						(uint8 *)dataChunk + sizeof(DataChunkPart),	/* start of compressed data */
						dataChunk->dataSize / sizeof(uint32));		/* how much to decompress */
		if ( err >= 0 )
			err = headPtr->decompErr;
		if ( err < 0 )
		{
			ERROR_RESULT_STATUS("Data Subscriber:ContinueDataCopy() error in FeedDecompressor()\n", err);
			goto DONE;
		}
	}

	/* if we have the whole chunk move it to the completed queue, otherwise remember 
	 *  what to do next
	 */
	if ( headPtr->pieceCount == dataChunk->pieceNum )
	{
		err = DataCopyIsComplete(ctx, headPtr);
	}
	else
	{
		/* add the block back into the partial list, we're not done building it */
		AddToPartialCtxQueue(ctx, headPtr);
		++headPtr->nextChunkNum;
	}

DONE:
	if ( 0 != err )
		FreeBlockAndData(ctx, headPtr);
	return err;
}


/*******************************************************************************************
 * Process messages from the data queue whose start times are LESS THAN the current 
 *  stream time.
 *******************************************************************************************/
Err 
ProcessMessageQueue(PvtDataContextPtr ctx)
{
	DSClock				dsClock;
	SubscriberMsgPtr	subMsg;
	DataChunkPartPtr	dataChunk;
	Err					err;
	Err					subsErr = 0;

	/* lock the channel state, bail if we catch an error */
	if ( (err = LockSemaphore(ctx->ctxLock, SEM_WAIT)) < 0 ) 
	{
		ERROR_RESULT_STATUS("Data Subscriber: ProcessMessageQueue() - error %ld in LockSemaphore()\n", err);
		goto DONE;
	}
	/* reset err as LockSemaphore() return 1 for success */
	err = kDSNoErr;

	/* grab the current stream time, we'll process everything with this time assuming that
	 *  the clock won't change significantly while we're doing it
	 */
	DSGetPresentationClock(ctx->streamCBPtr, &dsClock);

	/* log that we're entering */
	ADD_DATA_TRACE_L3(gDataTraceBufPtr, kDataEnterProcessMessageQueue, 0, 
						dsClock.streamTime, 0);

	/* dequeue and handle messages whose start times are LESS
	 * THAN or the SAME AS the current stream time.  Loop terminates
	 * when the head message has a start time greater than the current
	 * stream time, or when the queue is empty.
	 */
	for ( ; ; )
	{
		/* look at the next chunk on the queue, we're done if there is nothing left. */
		if ( NULL == (subMsg = ctx->dataMsgQueue.head) )
			goto UNLOCKANDRETURN;

		/* if the stream time (branch number and presentation time) is LESS THAN 
		 *  the data chunk time, the new data is NOT ready to be consumed yet.  
		 *
		 * !!!!! NOTE: we are assuming that ALL data which is still on our internal queues
		 *  at this point should be consumed.  this presupposes that all "unnecessary" data
		 *  will have been flushed at the appropriate time (our flush routines remove all
		 *  data from the queues
		 *
		 */
		dataChunk = (DataChunkPartPtr)subMsg->msg.data.buffer;
		if ( true == DSClockLT(dsClock.branchNumber, dsClock.streamTime,
								subMsg->msg.data.branchNumber, dataChunk->time) )
			goto UNLOCKANDRETURN;
		
		/* if the channel is not enabled, simply return the message to the streamer 
		 *  without pulling the data from the stream buffer.  we hang onto data 
		 *  without regard to the state of it's channel, ie. chunks are
		 *  kept in an internal queue until their presentation time, and then are returned
		 *  to the streamer if the channel is still disabled.  this strategy may plug up 
		 *  stream buffers somewhat, but it means that you don't have to take the size of
		 *  your stream buffers into account when enabling/disabling channels.
		 */
		if ( true == IsDataChanEnabled(ctx, dataChunk->channel) )
		{
			switch ( dataChunk->subChunkType )
			{
				case DATA_FIRST_CHUNK_TYPE:
					subsErr = StartDataCopy(ctx, (DataChunkFirstPtr)dataChunk);
					break;
				case DATA_NTH_CHUNK_TYPE:
					subsErr = ContinueDataCopy(ctx, dataChunk);
					break;
				
				/* we don't need a default case as the chunk types are checked when
				 *  they are queued at the lower level 
				 */
			}
		}

		/* the chunk has been handled, or we won't be processing the data.  dequeue and
		 *  return it's message to the streamer 
		 */
		(void)GetNextDataMsg(&ctx->dataMsgQueue);
		if ( 0 != (err = ReplyToSubscriberMsg(subMsg, kDSNoErr)) )
		{
			ERROR_RESULT_STATUS("ProcessMessageQueue() - ReplyToSubscriberMsg()", err);
			goto DONE;
		}

		/* Record the return of the chunk to the data streamer */
		ADD_DATA_TRACE_L3(gDataTraceBufPtr, kTraceDataCompleted, dataChunk->channel, 
							dsClock.streamTime, dataChunk->time);
		
		/* if we had an error above, break out now */
		if ( kDSNoErr != subsErr )
		{
			err = subsErr;
			break;
		}
	}

UNLOCKANDRETURN:

	/* log that we're leaving, release the channel context and we're done */
	ADD_DATA_TRACE_L3(gDataTraceBufPtr, kDataLeaveProcessMessageQueue, 0, 
						dsClock.streamTime, 0);
	UnlockSemaphore(ctx->ctxLock);

DONE:
	return err;
}
