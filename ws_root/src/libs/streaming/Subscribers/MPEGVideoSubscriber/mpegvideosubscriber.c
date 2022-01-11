/******************************************************************************
**
**  @(#) mpegvideosubscriber.c 96/09/16 1.42
**
******************************************************************************/

#include <string.h>
#include <stdio.h>
#include <limits.h>				/* INT_MIN, INT_MAX */

#include <kernel/types.h>
#include <kernel/debug.h>
#include <kernel/mem.h>
#include <audio/audio.h>

#include <streaming/msgutils.h>
#include <streaming/mempool.h>
#include <streaming/threadhelper.h>
#include <streaming/datastreamlib.h>
#include <streaming/mpegvideosubscriber.h>
#include "fmvdriverinterface.h"
#include <streaming/subscribertraceutils.h>

#include "mpegvideochannels.h"
#include "mpegvideosupport.h"


#if MPVD_TRACE_MAIN
	/* Change these trace level settings to control the level of detail. */
	#define TRACE_L1		0
	#define TRACE_L2		1
	#ifndef MPVD_DUMP_COMPLETION_STATS
		#define MPVD_DUMP_COMPLETION_STATS		0
	#endif

	/* The actual trace buffer. It's extern in case other 
	 * modules of this subscriber want to write into it. */
	TraceBuffer		MPEGVideoTraceBuffer;
	TraceBufferPtr	MPEGVideoTraceBufPtr	= &MPEGVideoTraceBuffer;

#if TRACE_L1
	#define		ADD_MPVD_TRACE_L1(bufPtr, event, chan, value, ptr)	\
						AddTrace(bufPtr, event, chan, value, ptr)
#else
	#define		ADD_MPVD_TRACE_L1(bufPtr, event, chan, value, ptr)
#endif

#if TRACE_L2
	#define		ADD_MPVD_TRACE_L2(bufPtr, event, chan, value, ptr)	\
						AddTrace(bufPtr, event, chan, value, ptr)
#else
	#define		ADD_MPVD_TRACE_L2(bufPtr, event, chan, value, ptr)
#endif

#else	/* Trace is off */
	#define		ADD_MPVD_TRACE_L1(bufPtr, event, chan, value, ptr)
	#define		ADD_MPVD_TRACE_L2(bufPtr, event, chan, value, ptr)
#endif


/* Turn on this compile-time switch if you want the subscriber to always print out frame
 * timing statistics that bear investigation. Otherwise it'll only print the stats when
 * compiled with MPVD_TRACE_MAIN on and DEBUG defined. */
#define ALWAYS_PRINT_BUM_FRAME_STATS		1


/* Number of write buffers to use in feeding the FMV Driver */
#define MPVD_FMV_WRITE_BUFFER_CNT	24
	

/************************************
 * Local types and constants
 ************************************/

/* This structure is used temporarily for communication between the spawning
 * (client) process and the nascent subscriber.
 *
 * Thread-interlock is handled as follows: NewMPEGVideoSubscriber() allocates
 * this struct on the stack, fills it in, and passes it's address as an arg to
 * the subscriber thread. The subscriber then owns access to it until sending a
 * signal back to the spawning thread (using the first 2 args in the struct).
 * Before sending this signal, the subscriber fills in the "output" fields of
 * the struct, thus returning its creation status result code and request msg
 * port Item. After sending this signal, the subscriber may no longer touch this
 * memory as NewMPEGVideoSubscriber() will deallocate it. */
typedef struct MPEGVideoCreationArgs {
	/* --- input parameters from the client to the new subscriber --- */
	Item				creatorTask;		/* who to signal when done initializing */
	uint32				creatorSignal;		/* signal to send when done initializing */
	DSStreamCBPtr		streamCBPtr;		/* stream this subscriber belongs to */
	uint32				frameBufferDepth;	/* bits/pixel in decompressed frame buffers */
	
	/* --- output results from spawing the new subscriber --- */
	Err					creationStatus;		/* < 0 ==> failure */
	Item				requestPort;		/* new thread's request msg port */
	} MPEGVideoCreationArgs;


#if MPVD_TRACE_MAIN || defined(DEBUG) || ALWAYS_PRINT_BUM_FRAME_STATS
	#define DO_DEBUG_STATS			1
#else
	#define DO_DEBUG_STATS			0
#endif

typedef struct DebugStats {
	int32	minDeltaPTS;
	int32	maxDeltaPTS;
	
	int32	earliestFrame;
	int32	latestFrame;
	} DebugStats;
	
#if DO_DEBUG_STATS
	static DebugStats		debugStats;
#endif


/***********************************************************************/
/* Routines to handle incoming messages from the stream parser thread  */
/***********************************************************************/
static int32		DoMPEGVideoData(MPEGVideoContextPtr ctx, SubscriberMsgPtr subMsg);
static int32		DoMPEGVideoGetChan(MPEGVideoContextPtr ctx, SubscriberMsgPtr subMsg);
static int32		DoMPEGVideoSetChan(MPEGVideoContextPtr ctx, SubscriberMsgPtr subMsg);
static int32		DoMPEGVideoControl(MPEGVideoContextPtr ctx, SubscriberMsgPtr subMsg);
static int32		DoMPEGVideoOpening(MPEGVideoContextPtr ctx, SubscriberMsgPtr subMsg);
static int32		DoMPEGVideoClosing(MPEGVideoContextPtr ctx, SubscriberMsgPtr subMsg);
static int32		DoMPEGVideoStart(MPEGVideoContextPtr ctx, SubscriberMsgPtr subMsg);
static int32		DoMPEGVideoStop(MPEGVideoContextPtr ctx, SubscriberMsgPtr subMsg);
static int32		DoMPEGVideoSync(MPEGVideoContextPtr ctx, SubscriberMsgPtr subMsg);
static int32		DoMPEGVideoEOF(MPEGVideoContextPtr ctx, SubscriberMsgPtr subMsg);
static int32		DoMPEGVideoAbort(MPEGVideoContextPtr ctx, SubscriberMsgPtr subMsg);

/********************************************************
 * Main Subscriber thread and its initialization routine 
 ********************************************************/
static MPEGVideoContextPtr	InitializeMPEGVideoThread(
								MPEGVideoCreationArgs *creationArgs);
static void					MPEGVideoSubscriberThread(int32 unused,
								MPEGVideoCreationArgs *creationArgs);


/*==========================================================================================
							Subscriber procedural interface
  ==========================================================================================*/

/******************************************************************************
|||	AUTODOC -public -class Streaming -group Startup -name NewMPEGVideoSubscriber
|||	Instantiates an MPEGVideoSubscriber.
|||	
|||	  Synopsis
|||	
|||	    Item NewMPEGVideoSubscriber(DSStreamCBPtr streamCBPtr,
|||	        int32 deltaPriority, Item msgItem, uint32 bitDepth)
|||	
|||	  Description
|||	
|||	    Instantiates a new MPEGVideoSubscriber. This creates the subscriber
|||	    thread, waits for initialization to complete, *and* signs it up for
|||	    a subscription with the Data Stream parser thread. This returns the new
|||	    subscriber's request message port Item number or a negative error code.
|||	    The subscriber will clean itself up and exit when it receives a
|||	    kStreamOpClosing or kStreamOpAbort message. (If DSSubscribe() fails, the
|||	    streamer will send a kStreamOpAbort message to the subscriber-wannabe.)
|||	    
|||	    This temporarily allocates and frees a signal in the caller's thread.
|||	    Currently, 4096 bytes are allocated for the subscriber's stack.
|||	    
|||	    During playback, the MPEGVideoSubscriber sends each filled frame buffer
|||	    to the client in a message that has a MPEGBufferPtr. The useful fields
|||	    in this structure are bufPtr (giving the buffer data address),
|||	    displayTime (indicating the audio time when you should display this
|||	    frame), and pts.FMVOpt_PTS (indicating the frame's MPEG PTS value, in
|||	    case you need it to compute the elapsed time).
|||	    
|||	    The client should reply to each filled-frame message when it's done
|||	    using that frame buffer, e.g. when that frame buffer is no longer
|||	    onscreen.
|||	    
|||	    After creating an MPEGVideoSubscriber, send it some Control messages for
|||	    further initialization: Send it a kMPEGVideoCtlOpSetFrameBuffers to
|||	    hand the subscriber the frame buffers to decode into. Send it a
|||	    kMPEGVideoCtlOpSetClientPort to tell the subscriber what Message port
|||	    to send filled frame buffer messages to.
|||	
|||	  Arguments
|||	
|||	    streamCBPtr
|||	        The new subscriber will subscribe to this Streamer.
|||	
|||	    deltaPriority
|||	        The priority (relative to the caller) for the subscriber thread.
|||	
|||	    msgItem
|||	        A Message Item to use temporarily for a synchronous DSSubscribe()
|||	        call.
|||	
|||	    bitDepth
|||	        The bit-depth for decoded pixels, either 16 or 24.
|||	
|||	  Return Value
|||	
|||	    A positive Item number of the new subscriber's request message
|||	    port or a negative error code indicating initialization errors, e.g.
|||	    kDSNoMemErr (couldn't allocate memory), kDSNoSignalErr (couldn't
|||	    allocate a signal), kDSSignalErr (problem sending or receiving a signal),
|||	    kDSRangeErr (parameter such as bitDepth out of range), or kDSInitErr
|||	    (initialization problem, e.g. allocating IOReq Items).
|||	
|||	  Implementation
|||	
|||	    Streaming library call.
|||	
|||	  Associated Files
|||	
|||	    <streaming/mpegvideosubscriber.h>, libsubscriber.a
|||	
******************************************************************************/
Item	NewMPEGVideoSubscriber(DSStreamCBPtr streamCBPtr, int32 deltaPriority,
			Item msgItem, uint32 bitDepth)
	{
	Err						status;
	uint32					signalBits;
	MPEGVideoCreationArgs	creationArgs;

	ADD_MPVD_TRACE_L1(MPEGVideoTraceBufPtr, kTraceNewSubscriber,
		MPVD_CHUNK_TYPE, 0, 0);

	/* Setup the creation args, including a signal to synchronize with the
	 * completion of the subscriber's initialization. It will signal us when it
	 * is done initializing itself, successfully or not. */
	status = kDSNoSignalErr;
	creationArgs.creatorTask	= CURRENTTASKITEM;	/* cf. <kernel/kernel.h>, included by <streaming/threadhelper.h> */
	creationArgs.creatorSignal	= AllocSignal(0);
	if ( creationArgs.creatorSignal == 0 )
		goto CLEANUP;
	creationArgs.streamCBPtr		= streamCBPtr;
	creationArgs.frameBufferDepth	= bitDepth;	/* MPEG video specific arg */
	creationArgs.creationStatus		= kDSInitErr;
	creationArgs.requestPort		= -1;

	/* Create the thread that will handle all subscriber responsibilities. */
	status = NewThread(
				(void *)(int32)&MPEGVideoSubscriberThread, 		/* thread entry point */
				4096, 											/* initial stack size */
				(int32)CURRENT_TASK_PRIORITY + deltaPriority,	/* priority */
				"MPEGVideoSubscriber",							/* name */
				0,												/* first arg to the thread */
				&creationArgs);									/* second arg to the thread */

	if ( status < 0 )
		goto CLEANUP;

	/* Wait here while the subscriber initializes itself. */
	status = kDSSignalErr;
	signalBits = WaitSignal(creationArgs.creatorSignal);
	if ( signalBits != creationArgs.creatorSignal )
		goto CLEANUP;

	/* We're done with this signal. So release it. */
	FreeSignal(creationArgs.creatorSignal);
	creationArgs.creatorSignal = 0;

	/* Check the subscriber's creationStatus. */
	status = creationArgs.creationStatus;
	if ( status < 0 )
		goto CLEANUP;

	status = DSSubscribe(msgItem, NULL,		/* a synchronous request */
				streamCBPtr, 				/* stream context block */
				MPVD_CHUNK_TYPE, 			/* subscriber data type */
				creationArgs.requestPort);	/* subscriber message port */
	if ( status < 0 )
		goto CLEANUP;

	return creationArgs.requestPort;		/* success! */

CLEANUP:
	/* Something went wrong.
	 * Release any resources we allocated and return the result status.
	 * The subscriber thread will clean up after itself. */
	if ( creationArgs.creatorSignal )	FreeSignal(creationArgs.creatorSignal);
	return status;
	}


/*==========================================================================================
							Internal subroutines
  ==========================================================================================*/

/*******************************************************************************************
 * Start all channels.
 *******************************************************************************************/		
static Err	StartMPEGVideoChannels(MPEGVideoContextPtr ctx, bool fFlush)
	{
	uint32	channelNumber;

	/* Start all channels for this subscription. */
	for ( channelNumber = 0; channelNumber < MPVD_SUBS_MAX_CHANNELS; channelNumber++ )
		{
		if ( fFlush )
			FlushMPEGVideoChannel(ctx, channelNumber, FALSE);

		StartMPEGVideoChannel(ctx, channelNumber);
		}

	/* Once we've activated all appropriate channels, start the read queue flowing */	
	PrimeMPEGVideoReadQueue(ctx);
		
	return kDSNoErr;
	}

		
/*******************************************************************************************
 * Close all channels.
 *******************************************************************************************/		
static Err	CloseMPEGVideoClosing(MPEGVideoContextPtr ctx)
	{
	uint32	channelNumber;

	for ( channelNumber = 0; channelNumber < MPVD_SUBS_MAX_CHANNELS; channelNumber++ )
		CloseMPEGVideoChannel(ctx, channelNumber);

	return kDSNoErr;
	}


/*******************************************************************************************
 * Process any and all decompression read I/O request completions, that is,
 * completed decompressed frame buffers.
 *******************************************************************************************/		
static Err	ProcessMPEGVideoReadCompletions(MPEGVideoContextPtr ctx)
	{
	Err						status;
	MPEGBufferPtr			doneBuffer;
	const uint32			frameDurationTicks =
		ctx->channel[ctx->activeChannel].frameDurationTicks;
	
	/* Do nothing if the active channel is currently "inactive" (paused).
	 * We'll process completed buffers when asked to resume. */
	if ( !IsMPEGVideoChanActive(&ctx->channel[ctx->activeChannel]) )
		return kDSNoErr;

	while ( (doneBuffer = DequeueDoneBuffer(&ctx->readQueue)) != NULL )
		{
		status = doneBuffer->ioreqItemPtr->io_Error;

		ADD_MPVD_TRACE_L2(MPEGVideoTraceBufPtr, kCompleteMPEGReadBuffer, 
			ctx->activeChannel, doneBuffer->ioreqItem, (void *)status);
		
		if ( status >= 0 && ctx->frameClientPort > 0 )
			{
			int32			audioClockPTS;
			uint32			ptsValue, branchNumber;
			Err				ptsStatus;
			DSClock			dsClock;
#if DO_DEBUG_STATS
			int32			deltaPTS = 0;	/* delta w.r.t. the previous frame */
			int32			numAudioTicksEarly;	/* how early/late is this frame? */

			TOUCH(deltaPTS);	/* avert compiler error when tracing is off */
#endif
			/* Read the decoded frame's PTS and the presentation clock. */
			ptsStatus = FMVGetPTS(doneBuffer->ioreqItemPtr, &ptsValue,
				&branchNumber);
			DSGetPresentationClock(ctx->streamCBPtr, &dsClock);
			if ( ptsStatus != 0 )
				{
				/* The driver didn't generate a PTS for some reason, so dead
				 * reckon one based on the last valid PTS we got.
				 * [TBD] Initialize ctx->prevPTS to, say, 3 or 4 frame periods,
				 *    and reinit it (to what?) after a branch. */
				
				branchNumber = ctx->prevBranchNumber;
				ptsValue = ctx->prevPTS +
					ctx->channel[ctx->activeChannel].frameDurationPTS;
				}

#if DO_DEBUG_STATS	/* Update delta PTS stats (within a branch #). */
			if ( branchNumber == ctx->prevBranchNumber )
				{
				deltaPTS = ptsValue - ctx->prevPTS;
				if ( deltaPTS < debugStats.minDeltaPTS )
					debugStats.minDeltaPTS = deltaPTS;
				if ( deltaPTS > debugStats.maxDeltaPTS )
					debugStats.maxDeltaPTS = deltaPTS;
				}
#endif

			/* Copy the PTS into a field of the *doneBuffer structure so the
			 * client can use it if needed. The VideoCD app can use this
			 * to compute elapsed time and overlay that onto the frame. A test
			 * program can use it to check for the right frame. */
			doneBuffer->pts.FMVOpt_PTS = ptsValue;

			/* Calculate the frame's actual display time by converting the PTS
			 * to audio time units and then calculating:
			 *   = audioClockPTS - streamTime + audioTime
			 *   = audioClockPTS - (audioTime - clockOffset) + audioTime
			 *   = audioClockPTS + clockOffset.
			 * BUT FIRST: Check the branchNumber.
			 * Set status to indicate whether to display or discard this frame.
			 * [TBD] ASSUMES: The stream clock is running.
			 * [TBD] ASSUMES: The stream clock won't be adjusted after this. */
#define DISCARD_FRAME		-3
			/* These MARKER constants are used to stand out like a sore thumb
			 * in the (decimal) trace log printouts. They substitute for normal
			 * numAudioTicksEarly values when the branch number isn't comparable. */
#define PRE_BRANCH_FRAME_MARKER		4000000001L
#define POST_BRANCH_FRAME_MARKER	4000000002L
			status = kDSNoErr;
			audioClockPTS = MPEGTimestampToAudioTicks(ptsValue);
			if ( branchNumber < dsClock.branchNumber )
				{
				/* Discard this pre-branch frame. */
				status = DISCARD_FRAME;
				doneBuffer->displayTime = 0;
#if DO_DEBUG_STATS
				numAudioTicksEarly = PRE_BRANCH_FRAME_MARKER;
				TOUCH(numAudioTicksEarly);	/* avert compiler warning if tracing is off */
#endif
				ADD_MPVD_TRACE_L1(MPEGVideoTraceBufPtr,
					kTracePreBranchTime + dsClock.running,
					dsClock.branchNumber, dsClock.audioTime,
					(void*)dsClock.clockOffset);
				}
			else if ( branchNumber > dsClock.branchNumber )
				{
				/* Delay this post-branch frame until the clock branches.
				 * [TBD] For now, just delay it by 1 frame time. */
				doneBuffer->displayTime = dsClock.audioTime + frameDurationTicks;
#if DO_DEBUG_STATS
				numAudioTicksEarly = POST_BRANCH_FRAME_MARKER;
				TOUCH(numAudioTicksEarly);	/* avert compiler warning if tracing is off */
#endif
				ADD_MPVD_TRACE_L1(MPEGVideoTraceBufPtr,
					kTracePostBranchTime + dsClock.running,
					dsClock.branchNumber, dsClock.audioTime,
					(void*)dsClock.clockOffset);
				}
			else
				{
				/* The normal case. Schedule this frame. */
				doneBuffer->displayTime = audioClockPTS + dsClock.clockOffset;
				ADD_MPVD_TRACE_L1(MPEGVideoTraceBufPtr,
					kTraceCurrentBranchTime + dsClock.running,
					dsClock.branchNumber, dsClock.audioTime,
					(void*)dsClock.clockOffset);

#if DO_DEBUG_STATS	/* Update numAudioTicksEarly debugging stats. */
				numAudioTicksEarly =
					(int32)doneBuffer->displayTime - (int32)dsClock.audioTime;
				if ( numAudioTicksEarly > debugStats.earliestFrame )
					debugStats.earliestFrame = numAudioTicksEarly;
				if ( numAudioTicksEarly < debugStats.latestFrame )
					debugStats.latestFrame = numAudioTicksEarly;
#endif
				}

			ADD_MPVD_TRACE_L2(MPEGVideoTraceBufPtr, kMPEGReadPTS,
				branchNumber, ( ptsStatus != 0 ) ? -1 : ptsValue,
				(void *)audioClockPTS);
			ADD_MPVD_TRACE_L2(MPEGVideoTraceBufPtr, kMPEGReadDeltaPTS,
				deltaPTS,			/* delta w.r.t. the previous frame, 90 kHz */
				numAudioTicksEarly,	/* how early/late is this frame? */
				(void *)doneBuffer->displayTime);
			
			/* [TBD] We can minimize the effects of an absurdly early PTS:
			 * adjust it to merely 4 frames early so we'll just play slowly. */
#define AVOID_LONG_DELAYS		1
#if AVOID_LONG_DELAYS
			if ( status >= 0 && branchNumber == dsClock.branchNumber )
				{
				const int32	fourFramesAhead = (int32)dsClock.audioTime +
						4 * frameDurationTicks;

				if ( doneBuffer->displayTime > fourFramesAhead )
					doneBuffer->displayTime = fourFramesAhead;
				}
#endif

			/* Update the previous-frame PTS for dead-reckoning PTSs. */
			ctx->prevBranchNumber = branchNumber;
			ctx->prevPTS = ptsValue;
			
			/* Send a completed frame off to the client thread, then add the
			 * MPEGBuffer to the forward queue for processing when the application
			 * sends the message back via a reply */
			if ( status >= 0 )
				{
				ADD_MPVD_TRACE_L2(MPEGVideoTraceBufPtr, kTraceDecodeBufferSent,
					doneBuffer->forwardMsg, doneBuffer->ioreqItem,
					(void *)doneBuffer->displayTime);
				status = SendMsg(ctx->frameClientPort, doneBuffer->forwardMsg,
					doneBuffer, sizeof(MPEGBuffer));
				}
			
			if ( status < 0 )
				/* [TBD] Report the error unless it's DISCARD_FRAME. */
				ClearAndReturnBuffer(&ctx->readQueue, doneBuffer);
			else
				AddMPEGBufferToTail(&ctx->forwardQueue, doneBuffer);
			}
		else
			{
			/* The I/O failed or there's no frameClient. Just return the read
			 * request to the pool. */
			ClearAndReturnBuffer(&ctx->readQueue, doneBuffer);
			}
			
		}
	return PrimeMPEGVideoReadQueue(ctx);
	}


/*==========================================================================================
					High level interfaces used by the main thread to process
									incoming messages. 
  ==========================================================================================*/

/*******************************************************************************************
 * Process an arriving data chunk.
 *******************************************************************************************/
static int32	DoMPEGVideoData(MPEGVideoContextPtr ctx, SubscriberMsgPtr subMsg)
	{
	Err							status;
	const SubsChunkDataPtr		subsChunkPtr =
		(SubsChunkDataPtr)subMsg->msg.data.buffer;
	const uint32				chunkSubtype =
		( subsChunkPtr->chunkSize >= sizeof(SubsChunkData) ) ?
			subsChunkPtr->subChunkType : 0;

	/* The incoming message could be data or a header chunk. Figure out
	 * which;  For headers, initialize the channel that the header arrived
	 * on.  For data, call the new data handling routine. */
	switch ( chunkSubtype )
		{
		/* A data message can arrive in one part or split into two parts */
		case MVDAT_CHUNK_SUBTYPE:
		case MVDAT_SPLIT_START_CHUNK_SUBTYPE:
		case MVDAT_SPLIT_END_CHUNK_SUBTYPE:
			status = ProcessNewMPEGVideoDataChunk(ctx, subMsg);
			break;
		
		case MVHDR_CHUNK_SUBTYPE:	/* Header msg arrived */
			status = InitMPEGVideoChannel(ctx,
				(MPEGVideoHeaderChunkPtr)subsChunkPtr);
			if ( status < 0 )  
				{
				ERROR_RESULT_STATUS("InitMPEGVideoChannel", status);
				CloseMPEGVideoChannel(ctx, subsChunkPtr->channel);
				}

			/* Because the subscriber considers headers and data to both be "Data" 
			 * messages we have to reply to the header chunk message here.
			 * Data msgs will be replied to when the subscriber is done
			 * using the data. */
			status = ReplyToSubscriberMsg(subMsg, status);
			break;

#if 0		/* [TBD] This is not a useful implementation of the halt chunk. Typically, the
			 * halt chunk would wait for a set of initializations to complete. */
		case MVHLT_CHUNK_SUBTYPE:	/* HALT chunk arrived */
			{
			Item 					aTimerCue;
			MPEGVideoHaltChunkPtr	mpegVideoHalt = (MPEGVideoHaltChunkPtr)subsChunkPtr;

			ADD_MPVD_TRACE_L1(MPEGVideoTraceBufPtr, kHaltChunkArrived, 
				mpegVideoHalt->channel, mpegVideoHalt->haltDuration, 0);
			
			/* Make an Audio Timer Cue */
			aTimerCue = CreateCue(NULL);

			/* Sleep for the duration specified in the HALT chunk */
			status = SleepUntilTime(aTimerCue, GetAudioTime() + mpegVideoHalt->haltDuration);

			DeleteCue(aTimerCue);

			/* Reply to the chunk message now. */
			status = ReplyToSubscriberMsg(subMsg, status, subMsg); 

			ADD_MPVD_TRACE_L1(MPEGVideoTraceBufPtr, kRepliedToHaltChunk, mpegVideoHalt->channel, 
				mpegVideoHalt->haltDuration, 0);
			break;
			}
#endif

		default:	/* an unrecognized chunk sub-type */
			status = ReplyToSubscriberMsg(subMsg, kDSNoErr);
			break;
		
		}	/* switch */
	
	return status;
	}

		
/*******************************************************************************************
 * Routine to set the status bits of a given channel.
 *******************************************************************************************/		
static int32	DoMPEGVideoSetChan(MPEGVideoContextPtr ctx, SubscriberMsgPtr subMsg)
	{
	Err						status = kDSChanOutOfRangeErr;
	const uint32			channelNumber = subMsg->msg.channel.number;
	MPEGVideoChannelPtr		channelPtr;
	int32					wasEnabled;
	uint32					mask;
	
	if ( channelNumber < MPVD_SUBS_MAX_CHANNELS )
		{
		channelPtr		= ctx->channel + channelNumber;

		/* Allow only bits that are Read/Write to be set by this call.
		 *
		 * NOTE: 	Any special actions that might need to be taken as as
		 *			result of bits being set or cleared should be taken
		 *			now. If the only purpose served by status bits is to 
		 *			enable or disable features, or some other communication,
		 *			then the following is all that is necessary. */
		wasEnabled = IsMPEGVideoChanEnabled(channelPtr);

		/* Mask off bits reserved by the system or by the subscriber */
        mask = subMsg->msg.channel.mask & ~(CHAN_SYSBITS | MPVD_CHAN_SUBSBITS);
        channelPtr->status = subMsg->msg.channel.status & mask |
			channelPtr->status & ~mask;

		/* If the channel became disabled, flush data & disable flow. If it
		 * became enabled, deactivate any other channel and start the new one. */
		if ( wasEnabled && !IsMPEGVideoChanEnabled(channelPtr) )
			{
			channelPtr->status |= CHAN_ENABLED;
			status = FlushMPEGVideoChannel(ctx, channelNumber, TRUE);
			channelPtr->status &= ~CHAN_ENABLED;
			}
		else if ( !wasEnabled && IsMPEGVideoChanEnabled(channelPtr) )
			{
			uint32					prevChannelNumber = ctx->activeChannel;
			MPEGVideoChannelPtr		prevEnabledChanPtr =
				ctx->channel + prevChannelNumber;

			if ( prevChannelNumber != channelNumber &&
					IsMPEGVideoChanEnabled(prevEnabledChanPtr) )
				{
				FlushMPEGVideoChannel(ctx, prevChannelNumber, TRUE);
				prevEnabledChanPtr->status &= ~CHAN_ENABLED;
				}

			status = StartMPEGVideoChannel(ctx, channelNumber);
			ctx->activeChannel = channelNumber;
			}

		}	/* if ( channelNumber < MPVD_SUBS_MAX_CHANNELS ) */

	return status;
	}

		
/*******************************************************************************************
 * Routine to return the status bits of a given channel.
 *******************************************************************************************/
static int32	DoMPEGVideoGetChan(MPEGVideoContextPtr ctx, SubscriberMsgPtr subMsg)
	{
	Err						status = kDSChanOutOfRangeErr;
	const uint32			channelNumber = subMsg->msg.channel.number;

	if ( channelNumber < MPVD_SUBS_MAX_CHANNELS )
		status = ctx->channel[channelNumber].status;

	return status;
	}

		
/*******************************************************************************************
 * Routine to perform an arbitrary subscriber defined action. 
 *******************************************************************************************/		
static int32	DoMPEGVideoControl(MPEGVideoContextPtr ctx, SubscriberMsgPtr subMsg)
	{
	Err						status = kDSNoErr;
	int32					userWhatToDo = subMsg->msg.control.controlArg1;
	MPEGVideoCtlBlockPtr	ctlBlockPtr =
		(MPEGVideoCtlBlockPtr)subMsg->msg.control.controlArg2;
	uint32					chanIndex;

	switch ( userWhatToDo )
		{
		case kMPEGVideoCtlOpFlushClient:
			/* check to make sure that all channels are inactive before
			 * attempting this operation */
			for ( chanIndex = 0; chanIndex < MPVD_SUBS_MAX_CHANNELS; chanIndex++ )
				if ( IsMPEGVideoChanActive(ctx->channel + chanIndex) )
					return kDSSubscriberBusyErr;

			/* Clear out the Frame Client message port referecnce */
			ctx->frameClientPort = 0;
			
			/* Release local references to the frame buffer */
			/* [TBD] Release ctx's pointers to frame buffers? The client could use
			 * kMPEGVideoCtlOpSetFrameBuffers to do that. */
			
			/* release the existing read queue, if one exists */
			if ( ctx->readQueue.bufferPool != NULL )
				Free1MPEGVideoMemPool(&ctx->readQueue.bufferPool);

			break;
		
				
		case kMPEGVideoCtlOpSetClientPort:
			/* Update the Frame Client message port */
			ctx->frameClientPort = ctlBlockPtr->SetClientPort.clientPort;
			break;
		
		
		case kMPEGVideoCtlOpSetFrameBuffers:
			/* Check that the frame buffer we've been handed is the same depth as
			 * the current session of the MPEGVideoSubscriber.  If not, complain and
			 * return an error.
			 * [TBD] Should we just change the FMV driver's state? (On CL-450 based
			 * systems, that takes a LONG time reloading microcode, which means we'd have
			 * to pause the stream until it completes.)  "Cogito ergo punt" */
			if (ctx->frameBufferDepth != ctlBlockPtr->SetFrameBuffers.frameBuffers.bitDepth)
				{
				status = kDSBadFrameBufferDepthErr;
				break;
				}
			
			/* cache a copy of the frame buffer descriptor */
			memcpy(&ctx->newframeBuffers, &ctlBlockPtr->SetFrameBuffers.frameBuffers,
				sizeof(FrameBufferDesc));
			if ( ctx->newframeBuffers.numFrameBuffers > MPVD_SUBS_MAX_FRAMEBUFFERS )
				ctx->newframeBuffers.numFrameBuffers = MPVD_SUBS_MAX_FRAMEBUFFERS;

			/* release the existing read queue, if one exists */
			if ( ctx->readQueue.bufferPool != NULL )
				Free1MPEGVideoMemPool(&ctx->readQueue.bufferPool);

		
			/* set up the FMV Read Queue */
			ctx->readQueue.bufferPool =
				CreateMemPool(ctx->newframeBuffers.numFrameBuffers, sizeof(MPEGBuffer));
			if ( ctx->readQueue.bufferPool == NULL )
				{
				Free1MPEGVideoMemPool(&ctx->readQueue.bufferPool);
				return kDSNoMemErr;
				}
				
			/* Initialize every pool member. */
			ctx->initFrameNumber = 0;	/* used by InitMPEGReadBuffer */
			if ( !ForEachFreePoolMember(ctx->readQueue.bufferPool, InitMPEGReadBuffer, (void*)ctx) )
				{
				Free1MPEGVideoMemPool(&ctx->readQueue.bufferPool);
				return kDSInitErr;
				}
			break;


		default:
			/* ignore unknown control messages for now... */
			break;
		}
	
	return status;
	}

		
/*******************************************************************************************
 * Routine to do whatever is necessary when a subscriber is added to a stream, typically
 * just after the stream is opened.
 *******************************************************************************************/		
static int32	DoMPEGVideoOpening(MPEGVideoContextPtr ctx, SubscriberMsgPtr subMsg)
	{
	TOUCH(ctx);				/* avert a compiler warning about unref'd parameter */
	TOUCH(subMsg);			/* avert a compiler warning about unref'd parameter */

	return kDSNoErr;
	}


/*******************************************************************************************
 * Routine to close down an open subscription.
 *******************************************************************************************/		
static int32	DoMPEGVideoClosing(MPEGVideoContextPtr ctx, SubscriberMsgPtr subMsg)
	{
	TOUCH(subMsg);			/* avert a compiler warning about unref'd parameter */
	
	return CloseMPEGVideoClosing(ctx);
	}


/*******************************************************************************************
 * Process a request msg to start all channels for this subscription.
 *******************************************************************************************/		
static int32	DoMPEGVideoStart(MPEGVideoContextPtr ctx, SubscriberMsgPtr subMsg)
	{
	StartMPEGVideoChannels(ctx,
		(subMsg->msg.start.options & SOPT_FLUSH) != 0);
	
	/* Now that data can flow again, process any completed read requests. */
	return ProcessMPEGVideoReadCompletions(ctx);
	}

		
/*******************************************************************************************
 * Routine to stop all channels for this subscription.
 *******************************************************************************************/		
static int32	DoMPEGVideoStop(MPEGVideoContextPtr ctx, SubscriberMsgPtr subMsg)
	{
	uint32	channelNumber;
	bool	fFlush = (subMsg->msg.stop.options & SOPT_FLUSH) != 0;

	/* Stop all channels for this subscription. */
	for ( channelNumber = 0; channelNumber < MPVD_SUBS_MAX_CHANNELS; channelNumber++ )
		{
		if ( fFlush )
			FlushMPEGVideoChannel(ctx, channelNumber, TRUE);	/* ==> Stop */
		else
			StopMPEGVideoChannel(ctx, channelNumber);		
		}	

	return kDSNoErr;
	}

		
/*******************************************************************************************
 * Routine flush all data waiting and/or queued under the assumption that we have 
 * just arrived at a branch point and should be ready to deal with all new data 
 * from an entirely different part of the stream.
 *******************************************************************************************/		
static int32	DoMPEGVideoSync(MPEGVideoContextPtr ctx, SubscriberMsgPtr subMsg)
	{
	TOUCH(subMsg);
	return StartMPEGVideoChannels(ctx, TRUE);
	}

		
/*******************************************************************************************
 * Routine to take action on end of file condition.
 *******************************************************************************************/		
static int32	DoMPEGVideoEOF(MPEGVideoContextPtr ctx, SubscriberMsgPtr subMsg)
	{
	TOUCH(ctx);				/* avert a compiler warning about unref'd parameter */
	TOUCH(subMsg);			/* avert a compiler warning about unref'd parameter */
	
	/* [TBD] Tell the MPEG Decoder that we reached EOS so it can emit the
	 * final frame. Once that last frame gets displayed, reply to this EOF
	 * request and flush the decoder. (Currently, the EOF request gets replied
	 * as soon as this procedure returns.) */

	return kDSNoErr;
	}

		
/*******************************************************************************************
 * Routine to kill all output, return all queued buffers, and generally stop everything.
 * Should return all channels to pre-initialized state.
 *******************************************************************************************/		
static int32	DoMPEGVideoAbort(MPEGVideoContextPtr ctx, SubscriberMsgPtr subMsg)
	{
	TOUCH(subMsg);			/* avert a compiler warning about unref'd parameter */
	
	return CloseMPEGVideoClosing(ctx);
	}


/*******************************************************************************
 * Process a stream-branched message: If it's a flush-branch, then flush data
 * and reply to the subMsg now. Else enqueue a "branch" request "plug" on the
 * active channel, and reply to the subMsg when the branch request completes.
 *
 * ASSUMES: There's only one active channel. To generalize the no-flush case to
 * n channels, we'd need to enqueue a "branch plug" in each active channel's
 * data queue so a branch requeust can be queued on the decoder channel at the
 * right point in the stream. Putting the branch subMsg in the dataQueue along
 * with data subMsgs is an easy approach to the one-channel situation.
 *******************************************************************************/
static Err	DoMPEGVideoBranch(MPEGVideoContextPtr ctx, SubscriberMsgPtr subMsg)
	{
	Err		status;

	if ( subMsg->msg.branch.options & SOPT_FLUSH )
		{
		status = FlushMPEGVideoChannel(ctx, ctx->activeChannel, FALSE);
		TOUCH(status);
		status = ReplyToSubscriberMsg(subMsg, status);
		}
	else
		status = EnqueueBranchMPEGVideoChannel(ctx, ctx->activeChannel, subMsg);

	return status;
	}


/*==========================================================================================
  ==========================================================================================
									The subscriber thread
  ==========================================================================================
  ==========================================================================================*/

/* Do one-time initialization for the new subscriber thread: Allocate its
 * context structure (instance data), allocate system resources, etc.
 *
 * RETURNS: The new context pointer if successful, or NULL if failed.
 * SIDE EFFECTS: To communicate with the spawning process, this fills in the
 *    output fields of the creationArgs structure and then signals the spawning
 *    process.
 * NOTE: Once we signal the spawning process, the creationArgs structure will
 *    go away out from under us. */
static MPEGVideoContextPtr	InitializeMPEGVideoThread(
		MPEGVideoCreationArgs *creationArgs)
	{
	MPEGVideoContextPtr	ctx;
	Err					status;

	/* Allocate the subscriber context structure (instance data) zeroed
	 * out, and start initializing fields. */
	status = kDSNoMemErr;
	ctx = AllocMem(sizeof(*ctx), MEMTYPE_FILL);
	if ( ctx == NULL )
		goto BAILOUT;
	ctx->streamCBPtr		= creationArgs->streamCBPtr;
	ctx->frameBufferDepth	= creationArgs->frameBufferDepth;

	/* Create the message port where this subscriber will accept
	 * request messages from the Streamer and client threads. */
	status = NewMsgPort(&ctx->requestPortSignal);
	if ( status < 0 )
		goto BAILOUT;
	creationArgs->requestPort = ctx->requestPort = status;

	/* Open the Audio Folio for this thread */
	if ( (status = OpenAudioFolio()) < 0 )
		goto BAILOUT;

	
	/* ***  ***  ***  ***  ***  ***  ***  ***  
	**	MPEG Video specific initializations
	** ***  ***  ***  ***  ***  ***  ***  ***/
	
	/* Set up the frame size based on the frame buffer depth */
	if (ctx->frameBufferDepth == 16)
		ctx->frameSize = NTSC_FRAME_WIDTH * NTSC_FRAME_HEIGHT * 2;
	else if (ctx->frameBufferDepth == 24)
		ctx->frameSize = NTSC_FRAME_WIDTH * NTSC_FRAME_HEIGHT * 4;
	else
		{
		status = kDSRangeErr;
		goto BAILOUT;
		}
	
	/* Initialize the FMV Driver */
	status = FMVOpenVideo(
		&ctx->fmvDevice,
		ctx->frameBufferDepth,	/* Bits per pixel */
		NTSC_FRAME_WIDTH,
		NTSC_FRAME_HEIGHT,
		/* ctx->frameBufferDepth == 16 ? kCODEC_NTSC_RESAMPLE : */ /* <- this is only useful on Opera */
			kCODEC_SQUARE_RESAMPLE);
	if ( status < 0 )
		goto BAILOUT;


	/* Allocate I/O completion signal masks, one for write requests (for
	 * sending compressed data to the MPEG decoder) and one for read requests
	 * (for receiving decompressed data from the MPEG decoder). */
	status = kDSNoSignalErr;
	if ( (ctx->writeDoneSignal = AllocSignal(0)) == 0 )
		goto BAILOUT;
	if ( (ctx->readDoneSignal = AllocSignal(0)) == 0 )
		goto BAILOUT;

	
	/* Initialize the MPEG write queue and queue elements.
	 * [TBD] There is a possibility that applications may wish to reduce the
	 *		memory footprint of the subscriber while it's inactive; it may
	 *		make sense to have the write queue initialized at "Start playing"
	 *		time and freed apon a detach-resource control call */
	status = kDSNoMemErr;
	InitMPEGBufferQueue(&ctx->writeQueue);
	ctx->writeQueue.bufferPool = CreateMemPool(MPVD_FMV_WRITE_BUFFER_CNT,
		sizeof(MPEGBuffer));
	if ( ctx->writeQueue.bufferPool == NULL )
		goto BAILOUT;
	
	status = kDSInitErr;
	if ( !ForEachFreePoolMember(ctx->writeQueue.bufferPool, InitMPEGWriteBuffer, ctx) )
		goto BAILOUT;


	/* Initialize the MPEG read queue. We'll initialize its elements later,
	 * in DoMPEGVideoControl(), so they can be tied to display buffers. */
	InitMPEGBufferQueue(&ctx->readQueue);


	/* Create the message port where the new subscriber will 
	 * recieve replies to frame messages sent to the client thread */
	status = NewMsgPort(&ctx->forwardReplyPortSignal);
	if ( status < 0 )
		goto BAILOUT;
	ctx->forwardReplyPort = status;


	/* Initialize the forward queue. This is where we hold read completions
	 * after they've been sent to the frame client thread. Once we get a reply
	 * from the client we return the MPEGBuffer to the read pool.
	 *
	 * NOTE: This queue doesn't have a pool associated with it since the
	 *    MPEGBufffers originate from the ReadQueue's pool */
	InitMPEGBufferQueue(&ctx->forwardQueue);

	/* Since we got here, creation must've been successful! */
	status = kDSNoErr;

BAILOUT:	/* ASSUMES: status indicates creation status at this point. */

	/* Inform our creator that we've finished with initialization.
	 *
	 * If initialization failed, clean up resources we allocated, letting the
	 * system release system resources. We need to free up memory we
	 * allocated and restore static state. */
	creationArgs->creationStatus = status;	/* return info to the creator task */
	
	SendSignal(creationArgs->creatorTask, creationArgs->creatorSignal);
	creationArgs = NULL;	/* can't access this memory after sending the signal */
	TOUCH(creationArgs);	/* avoid a compiler warning */
	
	if ( status < 0 )
		{
#if 0	/* This generates a bunch of warnings about not first deleting the
		 * IOReqs. So let the OS clean up the device and its IOReqs. */
		FMVCloseDevice(&ctx->fmvDevice);
#endif
		DisposeMPEGVideoPools(ctx);
		FreeMem(ctx, sizeof(*ctx));
		ctx = NULL;
		}

	return ctx;
	}


/*******************************************************************************************
 * Output debugging stats.
 *******************************************************************************************/
#if DO_DEBUG_STATS
	#define DELTA_PTS_RANGE_THRESHOLD			4
	#define EARLIEST_FRAME_THRESHOLD			10
	#define LATEST_FRAME_TRESHOLD				0

	static void OutputDebugStats(void)
		{
		#if MPVD_TRACE_MAIN
			DumpRawTraceBuffer(MPEGVideoTraceBufPtr, "MPEGVideoTraceRawDump.txt");
			#if MPVD_DUMP_COMPLETION_STATS
				DumpTraceCompletionStats(MPEGVideoTraceBufPtr, kSubmitMPEGWriteBuffer,
					kCompleteMPEGWriteBuffer, "MPEGVideoWriteTraceStatsDump.txt");
				DumpTraceCompletionStats(MPEGVideoTraceBufPtr, kSubmitMPEGReadBuffer,
					kCompleteMPEGReadBuffer, "MPEGVideoReadTraceStatsDump.txt");
			#endif
		#endif
	
		APRNT(("\nMPEG Video playback stats: PTS deltas ranged [%d to %d] with\n"
			" frames ready [%d to %d] audio ticks early. [A PTS delta range\n"
			" wider than %d is questionable, as are frames ready > 1 frame\n"
			" early (10 ticks @ 24 frames/sec) or at all late (< 0 ticks).]\n\n",
			debugStats.minDeltaPTS, debugStats.maxDeltaPTS,
			debugStats.latestFrame, debugStats.earliestFrame,
			DELTA_PTS_RANGE_THRESHOLD));
		}

#else
	#define OutputDebugStats()		((void)0)
#endif


/*******************************************************************************************
 * This thread is started by a call to InitMPEGVideoSubscriber(). It reads the subscriber message
 * port for work requests and performs appropriate actions. The subscriber message
 * definitions are located in "DataStream.h".
 *******************************************************************************************/
static void		MPEGVideoSubscriberThread(int32 unused,
					MPEGVideoCreationArgs *creationArgs)
	{
	MPEGVideoContextPtr		ctx;
	Err						status;
	uint32					signalBits;
	uint32					anySignal;
	bool					fKeepRunning;
	
	TOUCH(unused);			/* avert a compiler warning about unref'd parameter */


	/* Call a subroutine to perform all startup initialization. */
	ctx = InitializeMPEGVideoThread(creationArgs);
	creationArgs = NULL;	/* can't access that memory anymore */
	TOUCH(creationArgs);	/* avoid a compiler warning */
	if ( ctx == NULL )
		goto FAILED;
	
#if DO_DEBUG_STATS
	debugStats.minDeltaPTS = debugStats.latestFrame = INT_MAX;
	debugStats.maxDeltaPTS = debugStats.earliestFrame = INT_MIN;
#endif

	/* All resources are now allocated and ready to use. Our creator has been informed
	 * that we are ready to accept requests for work. All that's left to do is
	 * process work request messages when they arrive. */
	anySignal = ctx->requestPortSignal		| 
				ctx->writeDoneSignal		|
				ctx->readDoneSignal			|
				ctx->forwardReplyPortSignal;

	fKeepRunning = TRUE;
	while ( fKeepRunning )
		{
		ADD_MPVD_TRACE_L1(MPEGVideoTraceBufPtr, kTraceWaitingOnSignal, -1, anySignal, 0);

		signalBits = WaitSignal(anySignal);

		ADD_MPVD_TRACE_L1(MPEGVideoTraceBufPtr, kTraceGotSignal, -1, signalBits, 0);

		/**************************************************************		
		 * Check for and process any writes returning from the driver.
		 * Process all completed requests at the head of the queue in the order
		 * submitted. The subMsg can be a data or branch messages, or NULL
		 * from an internally-generated branch after a flush.
		 **************************************************************/
		if ( signalBits & ctx->writeDoneSignal)
			{
			SubscriberMsgPtr		subMsg;
			MPEGVideoChannelPtr		chanPtr;
			Err						completionStatus;
			MPEGBufferPtr			doneBuffer;
			
			while ((doneBuffer = DequeueDoneBuffer(&ctx->writeQueue)) != NULL)
				{
				/* Note the status of the completing write */
				completionStatus = doneBuffer->ioreqItemPtr->io_Error;
				
				/* Extract a pointer to the subscriber message associated with this
				 * write completion and reply to it */
				subMsg = doneBuffer->pendingMsg;
				
				ADD_MPVD_TRACE_L2(MPEGVideoTraceBufPtr, kCompleteMPEGWriteBuffer,
					ctx->activeChannel, doneBuffer->ioreqItem, (void *)completionStatus);
			
				if ( subMsg != NULL )	/* a flush-branch request has a NULL subMsg */
					{
					status = ReplyToSubscriberMsg(subMsg, completionStatus);
					if ( status < 0 )
						goto FAILED;
					}

				/* Disassociate buffer from a subscriber msg */
				doneBuffer->pendingMsg = NULL;
		
				/* Free the buffer back into the buffer pool */	
				ClearAndReturnBuffer(&ctx->writeQueue, doneBuffer);
				}
				
			/* dispatch any pending data mesages on the current channel with the
			 * resources freed from the write reply
			 * [TBD] can we assume it's the right channel for this? */
			chanPtr = ctx->channel + ctx->activeChannel;
			ProcessMPEGVideoDataQueue(ctx, chanPtr);
			}


		/**************************************************************		
		 * Process any completing frame reads from the FMV Driver
		 * Process all completed requests at the head of the queue, in the order submitted.
		 **************************************************************/
		if ( signalBits & ctx->readDoneSignal)
			{
			ProcessMPEGVideoReadCompletions(ctx);
			}


		/********************************************************
		 * Check for and process and frame client replies.
		 * Recycle the buffers.
		 ********************************************************/
		if ( signalBits & ctx->forwardReplyPortSignal )
			{
			MPEGBufferPtr			doneBuffer;
			
			while ( (status = GetMsg(ctx->forwardReplyPort)) > 0 )
				{
				doneBuffer = GetNextMPEGBuffer(&ctx->forwardQueue);
				if ( status != doneBuffer->forwardMsg )
					APERR(("MVID client returned a buffer out of FIFO order!\n"));
				ADD_MPVD_TRACE_L2(MPEGVideoTraceBufPtr, kTraceDecodeBufferRcvd,
					doneBuffer->forwardMsg, doneBuffer->ioreqItem,
					(void *)doneBuffer->displayTime);
				
				if ( doneBuffer )
			 		ClearAndReturnBuffer(&ctx->readQueue, doneBuffer);
				}
			
			/* fire off any read requests that are now ready to go */
			PrimeMPEGVideoReadQueue(ctx);
			}


		/********************************************************/
		/* Check for and process and incoming request messages. */
		/********************************************************/
		if ( signalBits & ctx->requestPortSignal )
			{
			SubscriberMsgPtr		subMsg;
			
			/* Process any new requests for service as determined by the incoming
			 * message data. */
			while ( PollForMsg(ctx->requestPort, NULL, NULL, (void **)&subMsg,
					&status) )
				{	
				switch ( subMsg->whatToDo )
					{
					case kStreamOpData:				/* new data has arrived */	
						ADD_MPVD_TRACE_L1(MPEGVideoTraceBufPtr, kTraceDataMsg, -1, 0, subMsg);
						status = DoMPEGVideoData(ctx, subMsg);
						break;

					case kStreamOpGetChan:			/* get logical channel status */
						ADD_MPVD_TRACE_L1(MPEGVideoTraceBufPtr,  kTraceGetChanMsg,
							subMsg->msg.channel.number, 0, subMsg);
						status = DoMPEGVideoGetChan(ctx, subMsg);
						break;

					case kStreamOpSetChan:			/* set logical channel status */
						ADD_MPVD_TRACE_L1(MPEGVideoTraceBufPtr, kTraceSetChanMsg,
							subMsg->msg.channel.number, subMsg->msg.channel.status, subMsg);
						status = DoMPEGVideoSetChan(ctx, subMsg);
						break;

					case kStreamOpControl:			/* perform subscriber defined function */
						ADD_MPVD_TRACE_L1(MPEGVideoTraceBufPtr, kTraceControlMsg, -1, 0, subMsg);
						status = DoMPEGVideoControl(ctx, subMsg);
						break;

					case kStreamOpSync:				/* clock stream resynched the clock */
						ADD_MPVD_TRACE_L1(MPEGVideoTraceBufPtr, kTraceSyncMsg, -1, 0, subMsg);
						status = DoMPEGVideoSync(ctx, subMsg);
						break;

					case kStreamOpOpening:			/* one time initialization call from DSH */
						ADD_MPVD_TRACE_L1(MPEGVideoTraceBufPtr, kTraceOpeningMsg, -1, 0, subMsg);
						status = DoMPEGVideoOpening(ctx, subMsg);
						break;

					case kStreamOpClosing:			/* stream is being closed */
						ADD_MPVD_TRACE_L1(MPEGVideoTraceBufPtr, kTraceClosingMsg, -1, 0, subMsg);
						status = DoMPEGVideoClosing(ctx, subMsg);
						fKeepRunning = FALSE;
						OutputDebugStats();
						break;

					case kStreamOpStop:				/* stream is being stopped */
						ADD_MPVD_TRACE_L1(MPEGVideoTraceBufPtr, kTraceStopMsg, -1, 0, subMsg);
						status = DoMPEGVideoStop(ctx, subMsg);
						break;

					case kStreamOpStart:			/* stream is being started */
						ADD_MPVD_TRACE_L1(MPEGVideoTraceBufPtr, kTraceStartMsg, -1, 0, subMsg);
						status = DoMPEGVideoStart(ctx, subMsg);
						break;

					case kStreamOpEOF:				/* physical EOF on data, no more to come */
						ADD_MPVD_TRACE_L1(MPEGVideoTraceBufPtr, kTraceEOFMsg, -1, 0, subMsg);
						status = DoMPEGVideoEOF(ctx, subMsg);
						break;

					case kStreamOpAbort:			/* somebody gave up, stream is aborted */
						ADD_MPVD_TRACE_L1(MPEGVideoTraceBufPtr, kTraceAbortMsg, -1, 0, subMsg);
						status = DoMPEGVideoAbort(ctx, subMsg);
						fKeepRunning = FALSE;
						OutputDebugStats();
						break;

					case kStreamOpBranch:			/* data discontinuity */
						ADD_MPVD_TRACE_L1(MPEGVideoTraceBufPtr, kTraceBranchMsg, -1, 0, subMsg);
						status = DoMPEGVideoBranch(ctx, subMsg);
						break;

					default:
						;
					} /* switch whattodo */
		
				/* Reply to the request we just handled unless this is a "data
				 * arrived" message or a "branch here" message. These are
				 * replied to asynchronously when the message is actually done
				 * and must not be replied to here. */
				if ( subMsg->whatToDo != kStreamOpData &&
						subMsg->whatToDo != kStreamOpBranch )
					{
					status = ReplyToSubscriberMsg(subMsg, status);
					if ( status < 0 )
						goto FAILED;
					}

				} /* while PollForMsg */
				
			} /* if RequestPortSignal */

	} /* while KeepRunning */


FAILED:	/* [TBD] Recover after FAIL_NEG()? */

	/* Dispose all memory we allocated and clean up any static state.
	 * The OS will automatically reclaim the Items we allocated. */
	if ( ctx != NULL )
		{
#if 0	/* This generates a bunch of warnings about not first deleting the
		 * IOReqs. So let the OS clean up the device and its IOReqs. */
		FMVCloseDevice(&ctx->fmvDevice);
#endif
		DisposeMPEGVideoPools(ctx);
		FreeMem(ctx, sizeof(*ctx));
		}
	}

