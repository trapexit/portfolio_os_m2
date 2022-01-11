/******************************************************************************
**
**  @(#) mpegvideosupport.c 96/11/26 1.5
**
******************************************************************************/

#include <string.h>
#include <stdlib.h>				
#include <stdio.h>

#include <kernel/types.h>
#include <kernel/debug.h>
#include <kernel/mem.h>
#include <audio/audio.h>
#include <video_cd_streaming/mpeg.h>

#include <video_cd_streaming/datastreamlib.h>
#include <video_cd_streaming/msgutils.h>
#include <streaming/mempool.h>
#include <video_cd_streaming/threadhelper.h>
#include <video_cd_streaming/mpegvideosubscriber.h>
#include <video_cd_streaming/mpegutils.h>
#include "fmvdriverinterface.h"

#include "mpegvideochannels.h"
#include "mpegvideosupport.h"

#include <streaming/subscribertraceutils.h>

#if MPVD_TRACE_SUPPORT
	extern TraceBufferPtr	MPEGVideoTraceBufPtr;
	#define		ADD_MPVD_TRACE_L1(bufPtr, event, chan, value, ptr)	\
					AddTrace(bufPtr, event, chan, value, ptr)
#else	/* Trace is off */
	#define		ADD_MPVD_TRACE_L1(bufPtr, event, chan, value, ptr)
#endif

/*==========================================================================================
  ==========================================================================================
						    Routines for Managing MPEGBuffers  
						   typically used with ForEachPoolMember 
  ==========================================================================================
  ==========================================================================================*/

/*******************************************************************************************
 * Called by ForEachFreePoolMember to initialize each MPEGBuffer with the info needed
 * to enqueue a compressed data write to the FMV driver.
 *
 * RETURNS: TRUE if successful; FALSE if failed, in which case ForEachFreePoolMember will quit.
 *
 * ASSUMES: The MPEGBuffer is already zeroed out (by CreateMemPool).
 *******************************************************************************************/
bool	InitMPEGWriteBuffer(void *ctx, void *poolEntry)
	{
	Err						status;
	MPEGBufferPtr			bufferPtr	= (MPEGBufferPtr)poolEntry;
	MPEGVideoContextPtr		myCtx		= (MPEGVideoContextPtr)ctx;

	/* Create an IOReq Item for this buffer */
	status = FMVCreateIOReq(&myCtx->fmvDevice, myCtx->writeDoneSignal);
	if ( status < 0 )
		{
		ERROR_RESULT_STATUS("InitMPEGWriteBuffer FMVCreateIOReq", status);
		return FALSE;
		}
	bufferPtr->ioreqItem = status;
	
	/* Cache the pointer to the Item data */
	bufferPtr->ioreqItemPtr = (IOReq*)LookupItem(bufferPtr->ioreqItem);

	return TRUE;
	}


/*******************************************************************************************
 * Called by ForEachFreePoolMember to initialize each MPEGBuffer with the info needed
 * to enqueue a decompressed data read to the FMV driver.
 *
 * RETURNS: TRUE if successful; FALSE if failed, in which case ForEachFreePoolMember will quit.
 *
 * ASSUMES: The MPEGBuffer is already zeroed out (by CreateMemPool).
 * ASSUMES: ctx->initFrameNumber has been set to 0 before the first read buffer is created and
 *    initialized. This proc uses it as a sequence counter.
 *******************************************************************************************/
bool	InitMPEGReadBuffer(void *ctx, void *poolEntry)
	{
	Err							status;
	MPEGBufferPtr				bufferPtr 	= (MPEGBufferPtr)poolEntry;
	MPEGVideoContextPtr			myCtx		= (MPEGVideoContextPtr)ctx;
	
	/* Create an IOReq Item for this buffer */
	status = FMVCreateIOReq(&myCtx->fmvDevice, myCtx->readDoneSignal);
	if ( status < 0 )
		{
		ERROR_RESULT_STATUS("InitMPEGReadBuffer FMVCreateIOReq", status);
		return FALSE;
		}
	bufferPtr->ioreqItem = status;
	
	/* Cache the pointer to the Item data */
	bufferPtr->ioreqItemPtr = (IOReq*)LookupItem(bufferPtr->ioreqItem);
	
	/* allocate a message item for forwarding completed reads to the client thread */
	bufferPtr->forwardMsg = CreateMsgItem(myCtx->forwardReplyPort);
	if ( bufferPtr->forwardMsg < 0 )
		{
		ERROR_RESULT_STATUS("InitMPEGReadBuffer CreateMsgItem", status);
		return FALSE;
		}
	
	bufferPtr->userData = (void *)(myCtx->initFrameNumber++);

	return TRUE;
	}
	

/*******************************************************************************************
 * Called by ForEachFreePoolMember to de-init each MPEGBuffer.
 *
 * RETURNS: TRUE if successful; FALSE if failed, in which case ForEachFreePoolMember will quit.
 *******************************************************************************************/
bool	FreeMPEGBuffer(void *notUsed, void *poolEntry)
	{
	MPEGBufferPtr			bufferPtr	= (MPEGBufferPtr)poolEntry;
	TOUCH(notUsed);			/* avert a compiler warning about unref'd parameter */

	/* Delete the IOReq item and clear its pointer */
	DeleteItem(bufferPtr->ioreqItem);

	return TRUE;
	}



/*==========================================================================================
  ==========================================================================================
						    Routines for Managing Buffer Pools  
  ==========================================================================================
  ==========================================================================================*/

/*******************************************************************************************
 * Free up one of the many MemPools used by this subscriber.
 *
 * INPUTS:
 *		*memPoolPtrPtr		-- MemPoolPtr in ctx to free up
 * OUTPUTS:
 *		*memPoolPtrPtr = NULL
 *******************************************************************************************/
void		Free1MPEGVideoMemPool(MemPoolPtr *memPoolPtrPtr)
	{
	MemPoolPtr	memPoolPtr = *memPoolPtrPtr;
	
	if ( memPoolPtr != NULL )
		{
		/* Free up all the buffers in the buffer pool. */
		ForEachFreePoolMember(memPoolPtr, FreeMPEGBuffer, 0);	/* [TBD] check for failure */
	
		/* Delete the pool */
		DeleteMemPool(memPoolPtr);
		
		*memPoolPtrPtr = NULL;
		}
	}

/*******************************************************************************************
 * Clean up and dispose a video subscriber's MemPools and any memory they allocated.
 *******************************************************************************************/
void		DisposeMPEGVideoPools(MPEGVideoContextPtr ctx)
	{
	Free1MPEGVideoMemPool(&ctx->writeQueue.bufferPool);
	Free1MPEGVideoMemPool(&ctx->readQueue.bufferPool);
	/* NOTE: There's also a ctx->forwardQueue but it has no MemPool. Nothing to free. */
	}


/*==========================================================================================
  ==========================================================================================
				    Routines for Handling Interactions with the MPEG Driver  
  ==========================================================================================
  ==========================================================================================*/

/*******************************************************************************
 * Write (enqueue) a compressed data packet or a "branch" discontinuity request
 * to the MPEG Video decoder. If the subMsg argument is NULL or a
 * kStreamOpBranch message, this will write a branch request, otherwise it must
 * be a data message. If subMsg is not NULL, this will link the request
 * structure to it so we can reply to subMsg when the flush completes.
 *
 * A branch request lets the decoder handle a discontinuity in the data. This
 * is needed to avoid bitstream errors, B frames delta-computed from the wrong
 * previous frames, leftover reference frames coming out without intervening B
 * frames, and so on.
 *
 * It's up to the caller to also abort I/O in progress if desired.
 *
 * NOTE: The I/O completion-handling code is responsible for handling subMsg
 * whether it be NULL, a branch message, or a data message.
 *******************************************************************************/
/*static int32 currVideoChunk = 0;*/
static Err WriteMPEGVideoData(MPEGVideoContextPtr ctx, SubscriberMsgPtr subMsg,
		MPEGBufferPtr writeBuffer)
{
	Err						status;
	uint32					pts = ~0, branchNumber = 0, fmvFlags;
	
	writeBuffer->fUserAbort = FALSE;	/* we'll set this to TRUE if and when
										 * we abort the I/O request */
	writeBuffer->pendingMsg = subMsg;

	/* Process a branch message differently than a data message. */
	if ( subMsg == NULL || subMsg->whatToDo == kStreamOpBranch )
		{
		writeBuffer->bufPtr  = NULL;
		writeBuffer->bufSize = 0;
		fmvFlags = FMV_FLUSH_FLAG;
		}
	else {	/* must be a kStreamOpData message. */

		writeBuffer->bufPtr = (void *)subMsg->msg.data.buffer;
		writeBuffer->bufSize = subMsg->msg.data.bufferSize;
		fmvFlags = subMsg->msg.data.ptsValid ? FMVValidPTS : 0;/* this chunk has a valid PTS value */
		branchNumber = subMsg->msg.data.branchNumber;
		pts = subMsg->msg.data.pts;
	} 
	
	status = FMVWriteBuffer(
		&ctx->fmvDevice,
		(char*)writeBuffer->bufPtr, 
		writeBuffer->bufSize,
		writeBuffer->ioreqItem,
		&writeBuffer->pts,				/* storage for FMV options */
		fmvFlags,
		pts,
		branchNumber);

	ADD_MPVD_TRACE_L1(MPEGVideoTraceBufPtr,
		(fmvFlags & FMV_FLUSH_FLAG) ? kSubmitMPEGBranch : kSubmitMPEGWriteBuffer,
		(fmvFlags == FMVValidPTS ? pts : -1), writeBuffer->ioreqItem, (void *)status);

	/* If successful, append the MPEGBuffer node to the write queue. We'll reply
	 * to the streamer request msg when the write completes. If failed, do some
	 * error recovery: Reply now and recycle the MPEGBuffer node. */
	if ( status >= 0 )
		AddMPEGBufferToTail(&ctx->writeQueue, writeBuffer);
	else
		{
		/* [TBD] Return the status to the streamer in the reply? That would make
		 * it abort the stream. [TBD] At least trace-log the error. */
		if ( subMsg != NULL )
			ReplyToSubscriberMsg(subMsg, kDSNoErr);
		ClearAndReturnBuffer(&ctx->writeQueue, writeBuffer);
		ERROR_RESULT_STATUS("FMVWriteBuffer() failed", status);
		}

	return status;
}


/*******************************************************************************
 * Send a "flush write" request to the MPEG Video decoder without aborting I/O
 * in progress so it can handle discontinuous data. This queues a "plug" so the
 * decoder will reset when it finishes with previous write requests.
 *
 * If the subMsg argument is not NULL, this procedure will link the request
 * structure to it so we can reply to that subMsg when the flush completes.
 *
 * ASSUMES: There is an available node in the writeQueue. That will be assured
 * if this is called after aborting write requests and returing their nodes to
 * the writeQueue.
 *******************************************************************************/
Err BranchMPEGVideoDecoder(MPEGVideoContextPtr ctx, SubscriberMsgPtr subMsg)
{
	Err						status = kDSNoMsgErr;
	MPEGBufferPtr			flushBuffer;

	flushBuffer = (MPEGBufferPtr)AllocPoolMem(ctx->writeQueue.bufferPool);

	if ( flushBuffer != NULL )
		status = WriteMPEGVideoData(ctx, subMsg, flushBuffer);

	return status;
}


/*******************************************************************************
 * Send all backlogged compressed data and "branch" messages to the MPEG Driver.
 *
 * [TBD] The subscriber or its active channel should remember whether it last
 * sent the decoder:
 *   a complete frame and a terminating start code,
 *   a complete frame without a terminating start code,
 *   a partial frame.
 * In various branching and EOS cases, the subscriber needs to use this info.
 * E.g. at EOS it needs to be sure the last frame was followed by a terminating
 * start code. (If not, send a Video Sequence Header. Or maybe set a
 * "end-of-frame" flag on each write request that's an entire frame or the last
 * part of a frame.
 *******************************************************************************/
Err ProcessMPEGVideoDataQueue(MPEGVideoContextPtr ctx,
		MPEGVideoChannelPtr chanPtr)
	{
	Err						status = kDSNoErr;
	MPEGBufferPtr			writeBuffer;
	SubscriberMsgPtr		subMsg;
	
	/* Do nothing if the channel is currently inactive (paused). */
	if ( !IsMPEGVideoChanActive(chanPtr) )
		return kDSNoErr;

	/* Send each queued data/branch message that we can pair with a write buffer. */
	while ( chanPtr->dataMsgQueue.head != NULL && 
			(writeBuffer =
			 (MPEGBufferPtr)AllocPoolMem(ctx->writeQueue.bufferPool)) != NULL )
		{
		subMsg = GetNextDataMsg(&chanPtr->dataMsgQueue);

		status = WriteMPEGVideoData(ctx, subMsg, writeBuffer);
		}
		
	return status;	/* NOTE: This is just the last write status. */
	}


/*******************************************************************************************
 * Send all available read requests to the MPEG Driver.
 *******************************************************************************************/
Err PrimeMPEGVideoReadQueue(MPEGVideoContextPtr ctx)
	{
	Err					status;
	MPEGBufferPtr		readBuffer;
	
	/* Do nothing if the active channel is currently "inactive" (paused).
	 * We'll process buffers when asked to resume. */
	if ( !IsMPEGVideoChanActive(&ctx->channel[ctx->activeChannel]) )
		return kDSNoErr;

	while ( ((readBuffer = (MPEGBufferPtr)AllocPoolMem(ctx->readQueue.bufferPool)) != NULL) )
		{
		int32		screenIndex = (int32)readBuffer->userData;
		
		readBuffer->bufPtr		= ctx->newframeBuffers.frameBuffer[screenIndex];
		readBuffer->bufSize		= ctx->newframeBuffers.frameBufferSize;
		readBuffer->fUserAbort	= FALSE;

		status = FMVReadVideoBuffer(
			readBuffer->bufPtr,
			readBuffer->bufSize,
			readBuffer->ioreqItem);
			
		ADD_MPVD_TRACE_L1(MPEGVideoTraceBufPtr, kSubmitMPEGReadBuffer, 
			/* ctx->activeChannel */ screenIndex, readBuffer->ioreqItem,
			(void *)status);

		if ( status < 0 )
			{
			ERROR_RESULT_STATUS("FMVReadVideoBuffer() failed", status);
			ClearAndReturnBuffer(&ctx->readQueue, readBuffer);
			return status;
			}
			
		/* Append the MPEGBuffer node to the reads-pending queue. */
		AddMPEGBufferToTail(&ctx->readQueue, readBuffer);
		}
		
	return kDSNoErr;
	}
