/******************************************************************************
**
**  @(#) mpegvideosubscriber.c 96/11/26 1.10
** 08/27/96 Ian	Again reworked the logic for handling PTS values in 
**				ProcessMPEGVideoReadCompletions().  The prior attempt to re-do
**				the calculated-PTS logic wasn't working well.  What we have 
**				now is a hybrid of the original logic (used to handle normal
**				playback) and the new logic (used to handle shuttle playback).
******************************************************************************/

#include <string.h>
#include <stdio.h>
#include <limits.h>				/* INT_MIN, INT_MAX */

#include <kernel/types.h>
#include <kernel/debug.h>
#include <kernel/mem.h>
#include <audio/audio.h>

#include <video_cd_streaming/msgutils.h>
#include <streaming/mempool.h>
#include <video_cd_streaming/threadhelper.h>
#include <video_cd_streaming/datastreamlib.h>
#include <video_cd_streaming/mpegvideosubscriber.h>
#include <video_cd_streaming/mpegutils.h>
#include "fmvdriverinterface.h"
#include <streaming/subscribertraceutils.h>

#include "mpegvideochannels.h"
#include "mpegvideosupport.h"

#define MPVD_TRACE_MAIN		0	/* <HPP> DEBUGGING */

#if MPVD_TRACE_MAIN
	/* Change these trace level settings to control the level of detail. */
	#define TRACE_L1		1
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

/* 
 * Turn this on to force very-early frame arrivals to be displayed 
 * sooner than their PTS would otherwise indicate.
 */

#define AVOID_LONG_DELAYS		1

/* Turn on this compile-time switch if you want the subscriber to always print out frame
 * timing statistics that bear investigation. Otherwise it'll only print the stats when
 * compiled with MPVD_TRACE_MAIN on or DEBUG defined. */
 
#define ALWAYS_PRINT_BUM_FRAME_STATS		0


/* Number of write buffers to use in feeding the FMV Driver */
#define MPVD_FMV_WRITE_BUFFER_CNT	24

/* Number of possible actions to be pending */
#define MPVD_ACTIONS_QUEUE_CNT		16

/************************************
 * Local types and constants
 ************************************/

#define DISCARD_FRAME		-3

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
	uint32				frameWidth;			/* width of the frame */
	uint32				frameHeight;		/* height of the frame */
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
static int32		DoMPEGVideoHeaderInfo(MPEGVideoContextPtr ctx, SubscriberMsgPtr subMsg);
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

Item	NewMPEGVideoSubscriber(DSStreamCBPtr streamCBPtr, int32 deltaPriority,
			Item msgItem, uint32 bitDepth, uint32 frameWidth, uint32 frameHeight)
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
	creationArgs.frameWidth 		= frameWidth;
	creationArgs.frameHeight		= frameHeight;
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
 * Take Actions. Iterate through pending action queue. And if we are meeting our action type
 * then take the action.
 *******************************************************************************************/
static Err	TakeActions(MPEGVideoContextPtr ctx, uint32 ptsValue )
{
	ActionPtr	actionptr, refactionptr;
	Err			status;

	refactionptr = NULL;
	actionptr = ctx->actionQueue.head;

	while( actionptr && (actionptr != refactionptr) ) {
		if( actionptr = GetNextAction(&ctx->actionQueue) ) {
			if( ptsValue >= actionptr->actionPTS ) {
				status = (*actionptr->actionProc)(actionptr->actionParam); /* TAKE THE ACTION */
				TOUCH(status); /* [TBD] do something with this? */
				ClearAndReturnAction(&ctx->actionQueue, actionptr);
			} else {/* if we didn't match our requirements add it back to the queue */
				AddActionToTail(&ctx->actionQueue, actionptr);
				refactionptr = actionptr;
			}
		}
	}
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

	/* Do nothing if the active channel is currently "inactive" (paused).
	 * We'll process completed buffers when asked to resume. */
	 
	if ( !IsMPEGVideoChanActive(&ctx->channel[ctx->activeChannel]) ) {
		return kDSNoErr;
	}
	
	while ( (doneBuffer = DequeueDoneBuffer(&ctx->readQueue)) != NULL )	{
		status = doneBuffer->ioreqItemPtr->io_Error;
		
		ADD_MPVD_TRACE_L2(MPEGVideoTraceBufPtr, kCompleteMPEGReadBuffer,
			ctx->activeChannel, doneBuffer->ioreqItem, (void *)status);

		if ( status >= 0 && ctx->frameClientPort > 0 ) {
			MPEGVideoChannelPtr	chanPtr	= ctx->channel + ctx->activeChannel;
			uint32				frameDurationPTS;
			uint32				frameDurationTicks;
			int32				audioClockPTS;
			uint32				ptsValue;
			uint32				branchNumber;
			Err					ptsStatus;
			DSClock				dsClock;
			int32				deltaPTS;	/* delta w.r.t. the previous frame */
#if DO_DEBUG_STATS
			int32				numAudioTicksEarly;	/* how early/late is this frame? */
#endif

			/*
			 * We always have to return an accurate PTS value in the doneBuffer we
			 * hand back to the client. (The VideoCD app displays elapsed time based
			 * on PTS values, and can't cope with missing or bad values).  
			 *
			 * If we're doing normal playback (not shuttling), we use the prior frame's
			 * PTS plus the default frame duration for the stream (which came from the
			 * video sequence header). 
			 *
			 * If we're shuttling we don't have a fixed rate, but the previous delta
			 * will be a darned close guess to the shuttling rate.  A shuttle delta
			 * PTS can be negative.
			 */

			frameDurationPTS	= chanPtr->frameDurationPTS;
			frameDurationTicks	= chanPtr->frameDurationTicks;
	
			ptsStatus = FMVGetPTS(doneBuffer->ioreqItemPtr, &ptsValue, &branchNumber);
			if (ptsStatus != 0) {
				branchNumber = ctx->prevBranchNumber; /* prevent spurious frame discard */
				if (ctx->fShuttling) {
					ptsValue = ctx->prevPTS + ctx->prevShuttleDeltaPTS;
				} else {
					ptsValue = ctx->prevPTS + frameDurationPTS;
				}
			}
			
			deltaPTS = ptsValue - ctx->prevPTS;
			
			doneBuffer->pts.FMVOpt_PTS = ptsValue;

			/* 
			 * Now calculate the display time (in AudioClock ticks).  When shuttling, 
			 * we want the frames displayed as quickly as possible, so we set the 
			 * displayTime to zero.  For normal frames, we calc the displayTime from
			 * the PTS value.
			 */

			if( ctx->fShuttling ) {
				doneBuffer->displayTime  = 0;
				ctx->prevShuttleDeltaPTS = deltaPTS;
			} else {
				/* Read the presentation clock, and figure out the current frame 
				 * duration (in audio ticks) from the previous delta value.
				 */
				DSGetPresentationClock(ctx->streamCBPtr, &dsClock);

#if DO_DEBUG_STATS	
				/* Update delta PTS stats (within a branch #). */
				if ( branchNumber == ctx->prevBranchNumber ) {
					if ( deltaPTS < debugStats.minDeltaPTS )
						debugStats.minDeltaPTS = deltaPTS;
					if ( deltaPTS > debugStats.maxDeltaPTS )
						debugStats.maxDeltaPTS = deltaPTS;
					}
#endif

				/* Calculate the frame's actual display time by converting the PTS
				 * to audio time units and then calculating:
				 *   = audioClockPTS - streamTime + audioTime
				 *   = audioClockPTS - (audioTime - clockOffset) + audioTime
				 *   = audioClockPTS + clockOffset.
				 * BUT FIRST: Check the branchNumber.
				 * Set status to indicate whether to display or discard this frame.
				 * [TBD] ASSUMES: The stream clock is running.
				 * [TBD] ASSUMES: The stream clock won't be adjusted after this. */

				status = kDSNoErr;
				audioClockPTS = MPEGTimestampToAudioTicks(ptsValue);
				
				if ( branchNumber < dsClock.branchNumber ) {
				
					/* Discard this pre-branch frame. */
					status = DISCARD_FRAME;
					doneBuffer->displayTime = 0;
	#if DO_DEBUG_STATS
					numAudioTicksEarly = 0x8000000;
					TOUCH(numAudioTicksEarly);	/* avert compiler warning if tracing is off */
	#endif
					ADD_MPVD_TRACE_L1(MPEGVideoTraceBufPtr,
						kTracePreBranchTime + dsClock.running,
						dsClock.branchNumber, dsClock.audioTime,
						(void*)dsClock.clockOffset);
						
				} else if ( branchNumber > dsClock.branchNumber ) {
				
					/* Delay this post-branch frame until the clock branches.
					 * [TBD] For now, just delay it by 1 frame time. 
					 */
					doneBuffer->displayTime = dsClock.audioTime + frameDurationTicks;
#if DO_DEBUG_STATS
					numAudioTicksEarly = 0x7FFFFFFF;
					TOUCH(numAudioTicksEarly);	/* avert compiler warning if tracing is off */
#endif
					ADD_MPVD_TRACE_L1(MPEGVideoTraceBufPtr,
						kTracePostBranchTime + dsClock.running,
						dsClock.branchNumber, dsClock.audioTime,
						(void*)dsClock.clockOffset);
						
				} else /* branchNumber == dsClock.branchNumber */ {
					int32	fourFramesAhead = (int32)dsClock.audioTime + (4 * frameDurationTicks);
				
					/* The normal case. Schedule this frame. */
					doneBuffer->displayTime = audioClockPTS + dsClock.clockOffset;
					
					/* A hack to fix some clock glitches...
					 *	If a frame has arrived very early in relation to its PTS,
					 *	we may have a problem.  This doesn't happen very often; 
					 *	some of the reasons it can happen are:
					 *	- A badly-authored stream.
					 *	- A glitch reading a PTS (it looked valid but wasn't).
					 *	- The audio subscriber jerks the clock around based on
					 *	  audio PTSes, and that can glitch the normal video PTS
					 *	  time calcs.
					 *	- The initial stream clock is set based on the first SCR
					 *	  that comes through, but then gets reset by the audio
					 *	  subscriber when a valid audio PTS come through; this
					 *	  effectively 'jerks' the clock forward or backward a bit.
					 *	In general, if we're doing moving video, we want to keep it
					 *	moving.  (Slide shows -- IFRAME mode but not shuttling -- are
					 *	a whole different story: we have to honor the PTSes exactly
					 *	no matter how far in the future they seem to be.)  If something
					 *	goes wrong with the clocks or PTSes, we still want to keep things
					 *	moving through the discontinuity; everything will get re-synced 
					 *	over a short period of time anyway (usually well under a second).
					 *	In practical terms, this logic only gets used on the first frame
					 *	or two at the start of a stream, unless some kind of major disc
					 *	glitch is present (scratches, badly authored, etc).
				 	 */
					 
#if AVOID_LONG_DELAYS
					if ( ctx->mode == FMV_MODE && doneBuffer->displayTime > fourFramesAhead ) {
						doneBuffer->displayTime = fourFramesAhead; 
					}
#endif

					ADD_MPVD_TRACE_L1(MPEGVideoTraceBufPtr,
						kTraceCurrentBranchTime + dsClock.running,
						dsClock.branchNumber, dsClock.audioTime,
						(void*)dsClock.clockOffset);


#if DO_DEBUG_STATS	/* Update numAudioTicksEarly debugging stats. */
					numAudioTicksEarly = (int32)doneBuffer->displayTime - (int32)dsClock.audioTime;
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

 			}

			/* Update the previous-frame values used for dead-reckoning PTSs. */
			
			ctx->prevBranchNumber	= branchNumber;
			ctx->prevPTS			= ptsValue;			

			/* Send a completed frame off to the client thread, then add the
			 * MPEGBuffer to the forward queue for processing when the application
			 * sends the message back via a reply */
			if ( status >= 0 ) {
				ADD_MPVD_TRACE_L2(MPEGVideoTraceBufPtr, kTraceDecodeBufferSent,
					doneBuffer->forwardMsg, doneBuffer->ioreqItem,
					(void *)doneBuffer->displayTime);
				status = SendMsg(ctx->frameClientPort, doneBuffer->forwardMsg, doneBuffer, sizeof(MPEGBuffer));
			}
			if ( status < 0 ) {
				/* [TBD] Report the error unless it's DISCARD_FRAME. */
				ClearAndReturnBuffer(&ctx->readQueue, doneBuffer);
			} else  {
				AddMPEGBufferToTail(&ctx->forwardQueue, doneBuffer);
				TakeActions(ctx, ptsValue);
			}
		} else {
			ERROR_RESULT_STATUS("FMV returned status in read buffer", status)
			/* The I/O failed or there's no frameClient. Just return the read request to the pool. */
			ClearAndReturnBuffer(&ctx->readQueue, doneBuffer);
		}
	}
	
	return PrimeMPEGVideoReadQueue(ctx);
}

/*******************************************************************************************
 * Set a new playback mode (FMV or IFRAME) along with the shuttling flag.
 *
 * When we change from IFRAME mode to FMV mode, we have to look at the current h/v sizes we
 * last set in the driver.  If we were doing high-res stills, the sizes will be high-res
 * sizes, and if we change the driver to FMV mode while the sizes are still that big, it will
 * try to allocate a reference buffer for motion video decoding based on those huge sizes.
 * We'll never have motion video that big, so we must never let the driver allocate such a 
 * big reference buffer (it's likely to fail anyway).  So, on a return to FMV mode, we also
 * reset the driver's sizes back to normal FMV sizes.
 *******************************************************************************************/
 
static int32	SetMPEGPlayMode(MPEGVideoContextPtr ctx, int32 frameMode, Boolean shuttle)
{
	Err		status = kDSNoErr;
	
	if (frameMode == FMV_MODE) {
		if (ctx->curHSize > FMV_MAX_HSIZE || ctx->curVSize > FMV_MAX_VSIZE) {
			ctx->curHSize = FMV_MAX_HSIZE;
			ctx->curVSize = FMV_MAX_VSIZE;
			status = FMVSetVideoSize(&ctx->fmvDevice, ctx->newframeBuffers.bitDepth, ctx->curHSize, ctx->curVSize, kCODEC_SQUARE_RESAMPLE);
			CHECK_NEG("FMVSetVideoSize: ", status);
		}
		if (ctx->mode != FMV_MODE) {
			ctx->mode = FMV_MODE;
			status = FMVSetVideoMode(&ctx->fmvDevice, VID_CODEC_TAG_PLAY, 0);
			CHECK_NEG("FMVSetVideoMode: ", status);
		}
		ctx->fShuttling = FALSE;
	} else {
		if (ctx->mode != IFRAME_MODE) {
			ctx->mode = IFRAME_MODE;
			status = FMVSetVideoMode(&ctx->fmvDevice, VID_CODEC_TAG_KEYFRAMES, 0);
			CHECK_NEG("FMVSetVideoMode: ", status);
		}
		ctx->fShuttling = shuttle;
	}

	return status;
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
	return ProcessNewMPEGVideoDataChunk(ctx, subMsg);
}

/*******************************************************************************************
 * Process an arriving header chunk info
 *******************************************************************************************/
static int32	DoMPEGVideoHeaderInfo(MPEGVideoContextPtr ctx, SubscriberMsgPtr subMsg)
	{
	Err								status;
	const MPEGVideoHeaderChunkPtr	hdrChunkPtr = (MPEGVideoHeaderChunkPtr)subMsg->msg.data.buffer;

	status = InitMPEGVideoChannel(ctx, (MPEGVideoHeaderChunkPtr)hdrChunkPtr);
	if ( status < 0 ) {
		ERROR_RESULT_STATUS("InitMPEGVideoChannel", status);
		CloseMPEGVideoChannel(ctx, 0 /* <HPP> 0 for WhiteBook subsChunkPtr->channel*/);
	}

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
				if ( IsMPEGVideoChanActive(ctx->channel + chanIndex) ) {
					status = kDSSubscriberBusyErr;
					break;
				}

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
			 * to pause the stream until it completes.)  "Cogito ergo punt"
			 */
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
			if ( ctx->readQueue.bufferPool == NULL ) {
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

		case kMPEGVideoCtlOpPlayIFrameOnly:
			status = SetMPEGPlayMode(ctx, IFRAME_MODE, FALSE);
			break;

		case kMPEGVideoCtlOpPlayAllFrames:
			status = SetMPEGPlayMode(ctx, FMV_MODE, FALSE);
			break;

#if 0
		case kMPEGVideoCtlOpReOpenFMV:
			/* reopen the video subscriber */
			if ( ctx->fmvDevice )
				FMVCloseDevice( &ctx->fmvDevice );
			status = FMVOpenVideo(&ctx->fmvDevice,
								  ctx->frameBufferDepth,	/* Bits per pixel */
							      ctx->frameWidth,
								  ctx->frameHeight,
								  kCODEC_SQUARE_RESAMPLE);
#endif
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
	StartMPEGVideoChannels(ctx, (subMsg->msg.start.options & SOPT_FLUSH) != 0);

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

	/* once a stream is stopped, revert to non-shuttle FMV mode. */
	/* (Ian says: Is this is a good idea?  It makes us flip modes 
	 * back and forth a lot on PBC discs.  I think we should turn
	 * off shuttle mode but not change FMV/IFRAME mode at all.)
	 */

	SetMPEGPlayMode(ctx, FMV_MODE, FALSE);

	/* Stop all channels for this subscription. */
	for ( channelNumber = 0; channelNumber < MPVD_SUBS_MAX_CHANNELS; channelNumber++ )
		{
		if ( fFlush )
			FlushMPEGVideoChannel(ctx, channelNumber, TRUE);	/* ==> Stop */
		else
			StopMPEGVideoChannel(ctx, channelNumber);
		}

	ClearAndReturnAllActions(&ctx->actionQueue);	/* make sure we don't have any pending actions */

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
		/*
		 * When starting a new stream, init the values used to dead-reckon a
		 * PTS when the genuine PTS is missing.  The delta value is set to
		 * one rather than zero, so that there'll be some small delta even 
		 * for the first frame.  
		 */
		if (subMsg->msg.branch.options & SOPT_NEW_STREAM)
			{
			ctx->prevPTS			 = 0;
			ctx->prevShuttleDeltaPTS = 1;
			}
		status = FlushMPEGVideoChannel(ctx, ctx->activeChannel, FALSE);
		TOUCH(status);
		status = ReplyToSubscriberMsg(subMsg, status);
		}
	else
		status = EnqueueBranchMPEGVideoChannel(ctx, ctx->activeChannel, subMsg);

	return status;
}

/*******************************************************************************
	cause the video to go into an I-Frame
 *******************************************************************************/
static Err	DoMPEGVideoShuttle(MPEGVideoContextPtr ctx, SubscriberMsgPtr subMsg)
{
	TOUCH(subMsg);
	return SetMPEGPlayMode(ctx, IFRAME_MODE, TRUE);
}

/*******************************************************************************
	cause the video to go into regular playback mode and display all frames
 *******************************************************************************/
static Err	DoMPEGVideoPlay(MPEGVideoContextPtr ctx, SubscriberMsgPtr subMsg)
{
	TOUCH(subMsg);
	return SetMPEGPlayMode(ctx, FMV_MODE, FALSE);	
}

/*******************************************************************************
	cause the video to take an action when a certain PTS value is reached
 *******************************************************************************/
static Err	DoMPEGSetAction(MPEGVideoContextPtr ctx, SubscriberMsgPtr subMsg)
{
	ActionPtr	actionptr;
	Err			status = kDSNoMemErr;

	actionptr = (ActionPtr)AllocPoolMem(ctx->actionQueue.actionPool);

	if( actionptr != NULL ) {
		actionptr->link = NULL;
		actionptr->actionType = subMsg->msg.action.type;
		actionptr->actionPTS = subMsg->msg.action.pts;
		actionptr->actionProc = (ActionProc)subMsg->msg.action.proc;
		actionptr->actionParam = subMsg->msg.action.param;
		AddActionToTail(&ctx->actionQueue, actionptr);
		status = kDSNoErr;
	}

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
static MPEGVideoContextPtr	InitializeMPEGVideoThread(MPEGVideoCreationArgs *creationArgs)
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
	ctx->frameWidth			= creationArgs->frameWidth;
	ctx->frameHeight		= creationArgs->frameHeight;

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
		ctx->frameSize = ctx->frameWidth * ctx->frameHeight * 2;
	else if (ctx->frameBufferDepth == 24)
		ctx->frameSize = ctx->frameWidth * ctx->frameHeight * 4;
	else
		{
		status = kDSRangeErr;
		goto BAILOUT;
		}

	/* Initialize the FMV Driver */
	status = FMVOpenVideo(
		&ctx->fmvDevice,
		ctx->frameBufferDepth,	/* Bits per pixel */
		ctx->frameWidth,
		ctx->frameHeight,
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

	/* Initialize the action queue. This is where we actions to take place when
	 * we reach the desired PTS value(within range). Once we do reach the action PTS
	 * we will call the actionProc. */

	InitActionQueue(&ctx->actionQueue);
	ctx->actionQueue.actionPool = CreateMemPool(MPVD_ACTIONS_QUEUE_CNT, sizeof(Action));
	if ( ctx->actionQueue.actionPool == NULL )
		goto BAILOUT;

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
		DeleteMemPool(ctx->actionQueue.actionPool);
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

		APRNT((" MPEG Video PTS deltas ranged [%d to %d] "
			"with frames ready [%d to %d] audio ticks early.\n"
			" [A PTS delta range spanning more than %d is dubious, as are frames ready\n"
			" > 1 frame early (> 10 ticks @ 24 frames/sec) or at all late (< 0 ticks).]\n",
			debugStats.minDeltaPTS, debugStats.maxDeltaPTS,
			debugStats.earliestFrame, debugStats.latestFrame,
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
				
				if (completionStatus < 0) {
					ERROR_RESULT_STATUS("FMV returned status in write buffer", completionStatus);
				}

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
					case kStreamOpHeaderInfo:		/* video header information */
						status = DoMPEGVideoHeaderInfo(ctx, subMsg);
						break;
					case kStreamOpShuttle:			/* video shuttle */
						status = DoMPEGVideoShuttle(ctx, subMsg);
						break;
					case kStreamOpPlay:				/* video play */
						status = DoMPEGVideoPlay(ctx, subMsg);
						break;
					case kStreamOpSetAction:
						status = DoMPEGSetAction(ctx, subMsg);
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
		DeleteMemPool(ctx->actionQueue.actionPool);
		DisposeMPEGVideoPools(ctx);
		FreeMem(ctx, sizeof(*ctx));
		}
	}

