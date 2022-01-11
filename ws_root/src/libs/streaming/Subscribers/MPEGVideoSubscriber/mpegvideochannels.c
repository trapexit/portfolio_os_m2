/******************************************************************************
**
**  @(#) mpegvideochannels.c 96/03/15 1.22
**
******************************************************************************/

#include <string.h>
#include <stdlib.h>				
#include <stdio.h>

#include <kernel/types.h>
#include <kernel/debug.h>

#include <streaming/dsstreamdefs.h>
#include <streaming/mpegvideosubscriber.h>
#include "fmvdriverinterface.h"
#include <streaming/subscribertraceutils.h>

#include "mpegvideochannels.h"
#include "mpegvideosupport.h"


/*****************************************************************************
 * Compile switches
 *****************************************************************************/

#if MPVD_TRACE_CHANNELS

/* This switch is a way to quickly change the level of tracing.
 * 	TRACE_LEVEL_1 gives you minimal trace info, 
 *	TRACE_LEVEL_2 gives you more info (includes level 1 trace),
 *	TRACE_LEVEL_3 gives you  maximal info (includes levels 1 and 2 trace). */	

#define MPVD_TRACE_LEVEL		3

/* Locate the trace buffer.  It's declared in MPEGVideoSubscriber.c. */
extern	TraceBufferPtr	MPEGVideoTraceBufPtr;

/* Allow for multiple levels of tracing */
#if (MPVD_TRACE_LEVEL >= 1)
	#define		ADD_MPVD_TRACE_L1( bufPtr, event, chan, value, ptr )	\
					AddTrace( bufPtr, event, chan, value, ptr )
#else
	#define		ADD_MPVD_TRACE_L1( bufPtr, event, chan, value, ptr )
#endif

#if (MPVD_TRACE_LEVEL >= 2)
	#define		ADD_MPVD_TRACE_L2( bufPtr, event, chan, value, ptr )	\
					AddTrace( bufPtr, event, chan, value, ptr )
#else
	#define		ADD_MPVD_TRACE_L2( bufPtr, event, chan, value, ptr )
#endif

#if (MPVD_TRACE_LEVEL >= 3)
	#define		ADD_MPVD_TRACE_L3( bufPtr, event, chan, value, ptr )	\
					AddTrace( bufPtr, event, chan, value, ptr )
#else
	#define		ADD_MPVD_TRACE_L3( bufPtr, event, chan, value, ptr )
#endif

#else /* MPVD_TRACE_CHANNELS */

/* Trace is off */
#define		ADD_MPVD_TRACE_L1( bufPtr, event, chan, value, ptr )
#define		ADD_MPVD_TRACE_L2( bufPtr, event, chan, value, ptr )	
#define		ADD_MPVD_TRACE_L3( bufPtr, event, chan, value, ptr )	

#endif /* MPVD_TRACE_CHANNELS */


/*******************************************************************************************
 * Routine to initialize a channel for a given context.  This routine is 
 * called when a new header chunk is received on a given channel.  The only
 * way a channel can become "un-initialized" is by calling CloseMPEGVideoChannel().
 * [TBD] We should also uninitialize when the streamer switches to a new file. But
 * revisit the default-channel-enabled business.
 *******************************************************************************************/
Err InitMPEGVideoChannel(MPEGVideoContextPtr ctx,
		MPEGVideoHeaderChunkPtr headerPtr)
	{
	Err						status = kDSNoErr;
	const uint32			channelNumber = headerPtr->channel;
	MPEGVideoChannelPtr		chanPtr;
			
	ADD_MPVD_TRACE_L2( MPEGVideoTraceBufPtr, kTraceChannelInit, channelNumber, 0, 0 );

	if ( channelNumber >= MPVD_SUBS_MAX_CHANNELS )
		return kDSChanOutOfRangeErr;
		
	chanPtr = ctx->channel + channelNumber;
	
	/* Sanity check:  Don't attempt to re-initialize a channel which has already
	 * recieved a header.  This will happen frequently when streams are looped
	 * or when lots of branching is going on so it doesn't really constitute an 
	 * error. */
	if ( !IsMPEGVideoChanInitialized(chanPtr) )
		{
		/* Make sure this version of MPEG Video files are compatible with this subscriber */
		if ( headerPtr->version != MPVD_STREAM_VERSION )
			PERR(("Warning: MPEGVideoSubscriber header chunk has the wrong version #\n"));
			
		chanPtr->frameDurationPTS = headerPtr->framePeriod;
		chanPtr->frameDurationTicks =
			MPEGTimestampToAudioTicks(chanPtr->frameDurationPTS);

		/* [TBD] Use the headerPtr->maxPictureArea field: Set the decoder's minimum
		 * reference frame buffer size to this value, or maybe to the larger of this
		 * value and values we've received from previous file's headers. */
		
		/* Set the initialization flag. */
		chanPtr->status	|= MPVD_CHAN_INITIALIZED;  
	
		/* Default channel 0 to enabled.  All other channels must be explicitly
		 * enabled with a SetChan message.
		 *
		 *	[TBD] Do we want the first channel to arrive to be the one
		 *		  explicitly enabled?
		 *  [TBD] What to do about channel enabling when we re-initialize after
		 *        switching stream files? */
		if ( channelNumber == 0 )
			chanPtr->status	|= CHAN_ENABLED;
		}
		
	return status;
	}

/*******************************************************************************************
 * Routine to begin data flow for the given channel.
 *******************************************************************************************/
Err StartMPEGVideoChannel(MPEGVideoContextPtr ctx, uint32 channelNumber)
	{
	Err						status = kDSNoErr;
	MPEGVideoChannelPtr		chanPtr;
	
	ADD_MPVD_TRACE_L2( MPEGVideoTraceBufPtr, kTraceChannelStart, channelNumber, 0, 0 );

	if ( channelNumber >= MPVD_SUBS_MAX_CHANNELS )
		return kDSChanOutOfRangeErr;

	chanPtr	=	ctx->channel + channelNumber;

	/* CHAN_ACTIVE simply means that we have received the "stream started" msg */
	chanPtr->status |= CHAN_ACTIVE;

	/* If the channel is disabled, don't bother to do anything else */
	if ( IsMPEGVideoChanEnabled(chanPtr) )
		{
		/* Do whatever you need to do to get things started */
		ProcessMPEGVideoDataQueue(ctx, chanPtr);
		}

	return status;
	}

/*******************************************************************************************
 * Routine to halt data flow for the given channel.
 *******************************************************************************************/
Err StopMPEGVideoChannel(MPEGVideoContextPtr ctx, uint32 channelNumber)
	{
	Err						status = kDSNoErr;
	MPEGVideoChannelPtr		chanPtr;

	ADD_MPVD_TRACE_L2(MPEGVideoTraceBufPtr, kTraceChannelStop, channelNumber, 0, 0);

	if ( channelNumber >= MPVD_SUBS_MAX_CHANNELS )
		return kDSChanOutOfRangeErr;

	chanPtr	=	ctx->channel + channelNumber;

	/* The stream has stopped */
	chanPtr->status &= ~CHAN_ACTIVE;

	/* If the channel is disabled, don't bother to do anything else */
	if ( IsMPEGVideoChanEnabled(chanPtr) )
		{
		/* Do whatever you need to do to make things stop */

		}

	return status;
	}


/***********************************************************************************
 * Disable flush a channel and optionally deactivate further data flow for the
 * channel. Noop if the channel isn't enabled.
 ***********************************************************************************/
Err FlushMPEGVideoChannel(MPEGVideoContextPtr ctx, uint32 channelNumber,
		bool deactivate)
	{
	Err						status = kDSNoErr;
	MPEGVideoChannelPtr		chanPtr;
	SubscriberMsgPtr		msgPtr;
	MPEGBufferPtr			abortedBuffer;

	ADD_MPVD_TRACE_L2(MPEGVideoTraceBufPtr, kTraceChannelFlush, channelNumber, 0, 0);

	if ( channelNumber >= MPVD_SUBS_MAX_CHANNELS )
		return kDSChanOutOfRangeErr;

	chanPtr	=	ctx->channel + channelNumber;

	/* If the channel is disabled, don't bother to do anything else */
	if ( IsMPEGVideoChanEnabled(chanPtr) )
		{
		if ( deactivate )
			{ /* Halt whatever activity is associated with the channel */
			status = StopMPEGVideoChannel( ctx, channelNumber );
			FAIL_NEG("FlushMPEGVideoChannel StopMPEGVideoChannel", status);
			}

		/* Give back all queued chunks for this channel to the
		 * stream parser. We do this by replying to all the
		 * "chunk arrived" messages that we have queued. */
		while (	( msgPtr = GetNextDataMsg(&chanPtr->dataMsgQueue) ) != NULL ) 	
			{
			ADD_MPVD_TRACE_L2(MPEGVideoTraceBufPtr, kFlushedDataMsg,
				channelNumber, -1 /* "dataMsgQueue" */, msgPtr);
			status = ReplyToSubscriberMsg(msgPtr, kDSNoErr);
			if ( status < 0 )
				goto FAILED;
			}

		/* Abort all pending write requests and recycle all their resources. */
		AbortQueue(&ctx->writeQueue);
		while ( (abortedBuffer = GetNextMPEGBuffer(&ctx->writeQueue)) != NULL )
			{
			msgPtr = abortedBuffer->pendingMsg;
			ADD_MPVD_TRACE_L2(MPEGVideoTraceBufPtr, kFlushedDataMsg,
				channelNumber, abortedBuffer->ioreqItem, msgPtr);
			if ( msgPtr != NULL )	/* Some branch requests have no subMsgs */
				status = ReplyToSubscriberMsg(msgPtr, kDSNoErr);
			abortedBuffer->pendingMsg = NULL;
			ClearAndReturnBuffer(&ctx->writeQueue, abortedBuffer);
			if ( status < 0 )
				goto FAILED;
			}

		/* Abort all pending reads and return resources */
		AbortQueue(&ctx->readQueue);
		while ( (abortedBuffer = GetNextMPEGBuffer(&ctx->readQueue)) != NULL )
			{
			ADD_MPVD_TRACE_L2(MPEGVideoTraceBufPtr, kFlushedBuffer,
				channelNumber, abortedBuffer->ioreqItem, msgPtr);
			ClearAndReturnBuffer(&ctx->readQueue, abortedBuffer);
			}

		/* Now tell the decoder we're branching. */
		BranchMPEGVideoDecoder(ctx, NULL);
		}

FAILED:	/* [TBD] Recover? */
	return status;
	}


/*******************************************************************************************
 * Stop and Flush a channel; Then free up all it's resources.  Should leave a channel
 * in pre-initialized state.
 *******************************************************************************************/
Err CloseMPEGVideoChannel(MPEGVideoContextPtr ctx, uint32 channelNumber)
	{
	Err						status = kDSNoErr;
	MPEGVideoChannelPtr		chanPtr;
	
	ADD_MPVD_TRACE_L2(MPEGVideoTraceBufPtr, kTraceChannelClose, channelNumber, 0, 0);

	if ( channelNumber >= MPVD_SUBS_MAX_CHANNELS )
		return kDSChanOutOfRangeErr;

	chanPtr	=	ctx->channel + channelNumber;

	/* If channel was never enabled and initialized, don't bother */
	if ( IsMPEGVideoChanInitialized(chanPtr) && IsMPEGVideoChanEnabled(chanPtr) )
		{
		/* Stop any activity and flush pending buffers if any */	
		status = FlushMPEGVideoChannel(ctx, channelNumber, TRUE);
			
		/* Reset all of the channel's variables */
		chanPtr->status				= 0;
		}
		
	return status;
	}


/*******************************************************************************************
 * A valid data chunk has arrived.  Do whatever needs to happen.
 * Chances are that you will want to pass this chunk to some routine that interface
 * with the part of the system that this subscriber drives.  Having been down this
 * path before let me take a a minute recommend that you implement a separate module 
 * which contains a set of interfaces to the part of the system that you want to use.  
 * The SAudioSubscriber was converted to do this way too late in the game 
 * and so is more unclean than necessary.
 *******************************************************************************************/
Err	ProcessNewMPEGVideoDataChunk(MPEGVideoContextPtr ctx, SubscriberMsgPtr subMsg)
	{
	Err						status = kDSNoErr;
	MPEGVideoFrameChunkPtr	mpegVideoData =
		(MPEGVideoFrameChunkPtr)subMsg->msg.data.buffer;
	uint32					channelNumber = mpegVideoData->channel;
	MPEGVideoChannelPtr		chanPtr;

	ADD_MPVD_TRACE_L3(MPEGVideoTraceBufPtr, kTraceChannelNewDataArrived,
		channelNumber, subMsg->msg.data.branchNumber, subMsg);

	if ( channelNumber >= MPVD_SUBS_MAX_CHANNELS )
		return kDSChanOutOfRangeErr;

	chanPtr = ctx->channel + channelNumber;

	/* Enqueue the data message if this channel is enabled, else reply to the
	 * subscriber message now.  Channels can be enabled with a SetChan call. Channel
	 * 0 is enabled by default. */
	if ( IsMPEGVideoChanEnabled(chanPtr) )
		AddDataMsgToTail(&chanPtr->dataMsgQueue, subMsg);		
	 else
	 	{
		/* Somehow we got a data chunk but never got a header chunk to init 
		 * the channel.  This is bad news and should not happen.  Perhaps
		 * someone caused the streamer to branch into the middle of a
		 * stream?  Or somone disabled a channel inapropriately? */  
		PERR(("ProcessNewMPEGVideoDataChunk got a data msg for a disabled channel!\n"));
		status = ReplyToSubscriberMsg(subMsg, kDSNoErr);
		}
	
	/* Now that we've enqueued a data message, fire off pending messages in the
	 * ***active channel's*** write Queue. */
	chanPtr = ctx->channel + ctx->activeChannel;
	ProcessMPEGVideoDataQueue(ctx, chanPtr);
	
	return status;
	}


/************************************************************************************
 * Enqueue a "branch" message in a channel's dataQueue. When we get to it in the
 * queue, we'll enqueue a branch request to the decoder. When the decoder gets to it
 * in its queue, it'll reset its state for a data discontinuity.
 ************************************************************************************/
Err EnqueueBranchMPEGVideoChannel(MPEGVideoContextPtr ctx, uint32 channelNumber,
		SubscriberMsgPtr subMsg)
	{
	Err						status = kDSNoErr;
	MPEGVideoChannelPtr		chanPtr;

	ADD_MPVD_TRACE_L3(MPEGVideoTraceBufPtr, kTraceChannelBranchMsg,
		channelNumber, subMsg->msg.branch.branchNumber, subMsg);

	if ( channelNumber >= MPVD_SUBS_MAX_CHANNELS )
		return kDSChanOutOfRangeErr;

	chanPtr = ctx->channel + channelNumber;

	/* Enqueue the branch message if this channel is enabled, else reply to the
	 * subscriber message now. */
	if ( IsMPEGVideoChanEnabled(chanPtr) )
		AddDataMsgToTail(&chanPtr->dataMsgQueue, subMsg);		
	 else
		status = ReplyToSubscriberMsg(subMsg, kDSNoErr);
	
	/* Now that we've enqueued a branch message, fire off pending messages in the
	 * ***active channel's*** write Queue. */
	chanPtr = ctx->channel + ctx->activeChannel;
	ProcessMPEGVideoDataQueue(ctx, chanPtr);
	
	return status;
	}
