/******************************************************************************
**
**  @(#) EZFlixSubscriber.c 96/07/18 1.18
**
******************************************************************************/

#include <stdio.h>
#include <strings.h>			/* for the memcpy() routine */

#include <kernel/debug.h>
#include <kernel/nodes.h>
#include <kernel/kernelnodes.h>
#include <kernel/list.h>
#include <kernel/task.h>
#include <kernel/mem.h>
#include <kernel/semaphore.h>
#include <kernel/io.h>
#include <kernel/time.h>
#include <kernel/cache.h>		/* WriteBackDCache */
#include <audio/audio.h>
#include <file/filefunctions.h>

#include <streaming/datastreamlib.h>
#include <streaming/msgutils.h>
#include <streaming/threadhelper.h>
#include <streaming/subscriberutils.h>
#include <streaming/ezflixsubscriber.h>
#include <streaming/dsstreamdefs.h>

#include "EZFlixDecoder.h"


/*****************************************************************************
 * Compile switches
 *****************************************************************************/

/* Decompressor requires that width of destination buffer be same
 * as a screen. */
/* #define EZFLIX_CELBUFFER_WIDTH	320 */	/* [TBD] Remove this? */

/* This definition sets the implementation upper bound for numChannels, the
 * number of logical channels the subscriber will listen to.
 **** NOTE: If you change it, update the AUTODOC. ****/
#define	EZFLIX_MAX_CHANNELS					8


/****************************************************************/
/* Subscriber channel structure, one per channel per subscriber */
/****************************************************************/

typedef struct EZFlixChannel {
	uint32			status;			/* state bits (see below) */
	SubsQueue		dataQueue;		/* queue of waiting data chunks */
	Item			dataQueueSem;	/* semaphore to manage access to data list */
	bool			fFlushOnSync;	/* TRUE => flush all chunks from channel on sync */
	EZFlixImageDesc	imageData;		/* describes the output frame buffers */
	} EZFlixChannel, *EZFlixChannelPtr;


/******************************************/
/* Subscriber context, one per subscriber */
/******************************************/

struct EZFlixContext {
	Item			requestPort;		/* message port for incoming subscriber requests */
	uint32			requestPortSignal;	/* signal to detect request port messages */

	uint32			numChannels;
	EZFlixChannel	channel[EZFLIX_MAX_CHANNELS];	/* an array of channels */
	};


/************************************/
/* EZFlix channel context structure */
/************************************/

struct EZFlixRec {
	DSStreamCBPtr	streamCBPtr;
	EZFlixHeader	header;				/* copy of the Header chunk for this stream channel */
	uint32			channelNum;			/* the stream channel # to use with this record */
	bool			curFrameAvail;		/* is there a current frame? */
	uint32			curBranchNumber;	/* current frame's branchNumber */
	uint32			curFrameTime;		/* the current frame's presentation time */
	uint32			curFrameDuration;	/* the current frame's duration */
	};


/*****************************************************************
 * This structure is used temporarily for communication between the spawning
 * (client) process and the nascent subscriber.
 *
 * Thread-interlock is handled as follows: NewEZFlixSubscriber() allocates
 * this struct on the stack, fills it in, and passes it's address as an arg to
 * the subscriber thread. The subscriber then owns access to it until sending a
 * signal back to the spawning thread (using the first 2 args in the struct).
 * Before sending this signal, the subscriber fills in the "output" fields of
 * the struct, thus returning its creation status result code and request msg
 * port Item. After sending this signal, the subscriber may no longer touch this
 * memory as NewEZFlixSubscriber() will deallocate it. */

typedef struct EZFlixCreationArgs {
	/* --- input parameters from the client to the new subscriber --- */
	Item				creatorTask;		/* who to signal when done initializing */
	uint32				creatorSignal;		/* signal to send when done initializing */
	DSStreamCBPtr		streamCBPtr;		/* stream this subscriber belongs to */
	uint32				numChannels;		/* number of flix channels */

	/* --- output results from spawing the new subscriber --- */
	Err					creationStatus;		/* < 0 ==> failure */
	Item				requestPort;		/* new thread's request msg port */
	EZFlixContextPtr	contextPtr;			/* EZFlix subscriber context ptr */
	} EZFlixCreationArgs;


/****************************/
/* Local routine prototypes */
/****************************/

static void	EZFlixSubscriberThread( int32 notUsed,
				EZFlixCreationArgs* creationArgs );

static uint32 GetEZFlixDstStuff( EZFlixImageDesc *pImageData, Bitmap *bitmap );


/*=============================================================================
  =============================================================================
		Subscriber procedural interface (called by the client thread)

		NOTE: It's critical to distinguish the client-thread procedures from
		the subscriber-thread procedures, and to ensure that they use
		semaphore locking on shared data. The EZFlixContext and EZFlixChannel
		structures are owned by the subscriber thread, while the EZFlixRec
		structures are owned by the client thread.

		[TBD] Some context and channel fields are not semaphore-protected.
		Are they ok by virtue of not changing or some other special case?
  =============================================================================
  =============================================================================*/


/******************************************************************************
 * Get the next available chunk for a channel.
 ******************************************************************************/
static SubscriberMsgPtr	GetEZFlixChunk(EZFlixContextPtr ctx,
		EZFlixRecPtr recPtr)
	{
	SubscriberMsgPtr	subMsg;
	EZFlixChannelPtr	chanPtr = ctx->channel + recPtr->channelNum;

	LockSemaphore(chanPtr->dataQueueSem, 1);

	subMsg = GetNextDataMsg(&chanPtr->dataQueue);

	UnlockSemaphore(chanPtr->dataQueueSem);

	return subMsg;
	}


/******************************************************************************
 * Poll for the next data msg (kStreamOpData) in this channel. If there is one,
 * return TRUE, pass the chunk back in *pChunkDataPtr, and pass the subscriber
 * message back in *subMsg. If not, return FALSE.
 ******************************************************************************/
static bool PollEZFlixChunk(EZFlixContextPtr	ctx,
							EZFlixRecPtr		recPtr,
							SubsChunkDataPtr	*pChunkDataPtr,
							SubscriberMsgPtr	*subMsg )
	{
	if ( (*subMsg = GetEZFlixChunk(ctx, recPtr)) == NULL )
		return FALSE;

	*pChunkDataPtr = (SubsChunkDataPtr)(*subMsg)->msg.data.buffer;
	return TRUE;
	}


/*******************************************************************************************
 * Routine to free a chunk back to the streamer.
 * NOTE: It's now ok if subMsg == NULL.
 *******************************************************************************************/
static Err FreeEZFlixChunk(EZFlixContextPtr ctx, SubscriberMsgPtr subMsg)
	{
	TOUCH(ctx);		/* avert a compiler warning */

	if ( subMsg != NULL )
		return ReplyToSubscriberMsg(subMsg, kDSNoErr);

	return kDSNoErr;
	}


/*******************************************************************************************
 * Allocate and initialize a streamed EZFlix state record. DrawEZFlixToBuffer will return NULL
 * until the Header and first frame have been received. When the First frame comes in, the
 * current audio time will be read and saved to determine which frame to display each call.
 *
 * TBD - CLEANUP this routine. Don't return pointers to the client...
 *
 *******************************************************************************************/
Err	InitEZFlixStreamState(DSStreamCBPtr		streamCBPtr,
						EZFlixContextPtr	ctx,
						EZFlixRecPtr		*pRecPtr,
						uint32				channel,
						bool				flushOnSync)
	{
	EZFlixRecPtr		recPtr;
	EZFlixChannelPtr	chanPtr;

	recPtr = AllocMem(sizeof(*recPtr), MEMTYPE_FILL);
	if ( recPtr == NULL )
		return kDSNoMemErr;

	recPtr->channelNum	= channel;
	recPtr->streamCBPtr	= streamCBPtr;
	*pRecPtr			= recPtr;

	chanPtr = ctx->channel + channel;
	chanPtr->fFlushOnSync	= flushOnSync;

	chanPtr->imageData.xPos = chanPtr->imageData.yPos = 0;

	return kDSNoErr;
	} /* InitEZFlixStreamState */


/*******************************************************************************************
 *	Return all EZFlix msgs to the EZFlixSubscriber back end. The EZFlix subscriber will return one
 *	subscriber chunk msg to the Data Streamer for each of these msgs returned.
 *  This is safe to call if ctx == NULL.
 *******************************************************************************************/
void	FlushEZFlixChannel( EZFlixContextPtr ctx, EZFlixRecPtr recPtr )
	{
	SubsChunkDataPtr	pChunkData;
	SubscriberMsgPtr	subMsg;

	if ( ctx == NULL )
		return;

	while ( PollEZFlixChunk(ctx, recPtr, &pChunkData, &subMsg) )
		FreeEZFlixChunk(ctx, subMsg);

	recPtr->curFrameAvail = FALSE;
	}


/*******************************************************************************************
 * Flush an EZFlix channel and deallocate a corresponding EZFlixRec structure.
 * This is safe to call if ctx == NULL and/or if recPtr == NULL.
 *******************************************************************************************/
Err	DestroyEZFlixStreamState( EZFlixContextPtr ctx, EZFlixRecPtr recPtr )
	{
	FlushEZFlixChannel( ctx, recPtr );
	FreeMem( recPtr, sizeof(*recPtr) );

	return kDSNoErr;
	}


/*******************************************************************************************
 * Return the duration of the film (in 240ths), or -1 if the film frame hasn't been read
 * yet.  NOTE: this function returns the time of the BEGINING of the last frame
 *******************************************************************************************/
int32	EZFlixDuration (EZFlixRecPtr recPtr)
	{
	int32 frameRateAudioTime;

	if ( !recPtr->curFrameAvail )
		return -1;

	frameRateAudioTime = 240 / recPtr->header.scale;
	return (recPtr->header.count * frameRateAudioTime) - frameRateAudioTime;
	}


/*******************************************************************************************
 * Return the current time of the film (in 240ths), or -1 of the film frame hasn't been
 *  read yet.
 *******************************************************************************************/
int32	EZFlixCurrTime (EZFlixRecPtr recPtr)
	{
	/* If no frames read yet then can't tell what time it is */
	if ( !recPtr->curFrameAvail )
		return -1;

	return recPtr->curFrameTime;
	}


/******************************************************************************
 * Return TRUE if it's time to dequeue and decode a new frame, i.e. if we don't
 * have one in hand or if the current one has expired (or will expire within
 * an estimated decode time).
 *******************************************************************************/
bool	IsTimeForNextFrame (EZFlixRecPtr recPtr)
	{
	DSClock				dsClock;
	static const uint32	kCompressionTimeEst = 5;	/* approx. 21 msec */
	/* kCompressionTimeEst is an estimate of the frame decompession delay.
	 * It's used to get closer to presenting
	 * frames on screen at their actual presentation time.  Since the
	 * decompression process takes significant computation time, frames
	 * are presented on screen some time after they have been pulled
	 * from the queue for processing.  If frames are taken from the queue
	 * at their presentation time, as the CinePak subscriber does, they
	 * will display on screen late.  This constant is a small attempt
	 * at getting the old CinePak subscriber to have frames ready to
	 * display nearer their presentation times. [TBD] What is really needed is an
	 * architectural change to allow the subscriber to decompress frames
	 * independent of, and in advance of, presentation to the client.
	 * The constant is selected to be less than the expected lower bound
	 * for frame decompression times.  Setting this value to zero makes the
	 * logic of this routine equivalent to the same routine found in the
	 * CinePak subscriber. */

	/* Read the stream presentation clock. */
	DSGetPresentationClock(recPtr->streamCBPtr, &dsClock);

	/* This test in the CinePak subscriber is intended to prevent presenting
	 * the first frame of the stream too early. No frames are presented until
	 * the presentation time of the first stream header is past.  This test
	 * does not work properly for streams containing multiple headers
	 * (video segments) when those streams are rewound or looped.  (The
	 * header being tested is the current header, maybe the third header in
	 * the stream, which will have a higher time value than the current time,
	 * which has looped back, until the new header at the loop-back destination
	 * time is handled.)
	 * For this release of EZFlix this test is disabled, so multi-segment streams
	 * can be rewound but the first frame of a stream is not presented at it's
	 * correct time.
	 *
	 * [Question to whoever wrote the above: Why doesn't the frame's presentation
	 * time keep it from being presented too early? It should.] */
	/*if ( recPtr->header.time > dsClock.streamTime ) */
	/*	return FALSE; */

	/* If no frame available then we definitely need to read one. */
	if ( !recPtr->curFrameAvail )
		return TRUE;

	/* If the stream clock branched since this frame was received, this frame
	 * and possibly others still in the msg queue are stale. So yes, we need to
	 * get a new frame. */
	if ( dsClock.branchNumber > recPtr->curBranchNumber )
		return TRUE;

	/* If we received new data that's a branch ahead of the presentation clock,
	 * then we should wait for the clock to catch up. */
	if ( dsClock.branchNumber < recPtr->curBranchNumber )
		return FALSE;

	/* If the current frame should still be displayed, return FALSE.
	 * Otherwise (the current frame's starting time plus its duration is less
	 * than the current time) we need a new frame, so return TRUE. */
	return dsClock.streamTime + kCompressionTimeEst >=
		recPtr->curFrameTime + recPtr->curFrameDuration;
	}


/*******************************************************************************************
 *	Although the frame buffer displays is 320 by 240 (384x288 in PAL) , only about 288 by
 *	216 pixels are in the visible area. (348x260 in PAL)  All display calculations are
 *	therefore based on a rectangle that is roughly centered in the frame buffer:
 *	VisibleRect(TLBR)= [12,18,12+216,18+288];
 *******************************************************************************************/
static uint32	GetEZFlixDstStuff(EZFlixImageDesc *pImageData, Bitmap *bitmap)
	{
	int32	hOff;
	int32	vOff;
	int32	bytesPerPixel = 4;

	if (bitmap->bm_Type == BMTYPE_16)
		bytesPerPixel = 2;
	hOff = ((bitmap->bm_Width - (pImageData->width+pImageData->xPos))/2) * bytesPerPixel;
	vOff = (((bitmap->bm_Height - (pImageData->height+pImageData->yPos))/2) & 0xFFFFFFFC) *
		(bitmap->bm_Width * bytesPerPixel);

	pImageData->baseAddr = (int32)bitmap->bm_Buffer + hOff + vOff;
	pImageData->rowBytes = bitmap->bm_Width * bytesPerPixel;

	return bytesPerPixel;
	}


/*******************************************************************************************
 * Get the next compressed frame data in the streamed EZFlix film.
 * Returns TRUE if a frame was drawn, and FALSE otherwise.
 *******************************************************************************************/
bool DrawEZFlixToBuffer( EZFlixContextPtr ctx, EZFlixRecPtr recPtr, Bitmap *bitmap )
	{
	EZFlixChannelPtr	pChannel = &ctx->channel[recPtr->channelNum];
	EZFlixImageDesc		*pImageData = &pChannel->imageData;
	SubscriberMsgPtr	subMsg;					/* subscriber msg received */
	SubscriberMsgPtr	curFrameSubMsg = NULL;	/* current subs. frame msg */
	EZFlixFramePtr		curFramePtr = NULL;		/* current frame ptr to decode */
	SubsChunkDataPtr	pChunkData;				/* subs. msg data contents */
	bool				result = FALSE;
	EZFlixDecoderParams	params;

	/* If it's time to display a new frame or the stream hasn't supplied one yet,
	 * poll for one in the queue. Repeat until we've got the most timely frame
	 * available. Process all "data" messages we encounter. */
	while ( IsTimeForNextFrame(recPtr) &&
			PollEZFlixChunk(ctx, recPtr, &pChunkData, &subMsg) )
		{
		switch ( pChunkData->subChunkType )
			{
			case EZFLIX_HDR_CHUNK_SUBTYPE:	/* EZFlix Header Chunk */
				/* Copy the header so we can release its chunk. */
				memcpy(&recPtr->header, pChunkData, sizeof(EZFlixHeader));

				/* These are used by GetEZFlixDstStuff to calc the offset into
				 * the Frame Buffer */
				pImageData->width	= recPtr->header.width;
				pImageData->height	= recPtr->header.height;

				/* Set the state of the decompressor object */
				params.width = recPtr->header.width;
				params.height = recPtr->header.height;
				params.codecVersion = recPtr->header.codecVersion;
				params.EZFlixVersion = recPtr->header.version;
				SetEZFlixDecoderParams(&params);

				/* We're starting a new sequence with new parameters, so
				 * release any previous frame. Also release the hdr chunk. */
				FreeEZFlixChunk(ctx, curFrameSubMsg);
				curFrameSubMsg = NULL;
				curFramePtr = NULL;
				recPtr->curFrameAvail = FALSE;
				FreeEZFlixChunk(ctx, subMsg);
				break;

			case EZFLIX_FRME_CHUNK_SUBTYPE:	/* EZFlix Frame Chunk */
				/* We received a(nother) frame, so release any previous one
				 * and latch onto the new one. */
				FreeEZFlixChunk(ctx, curFrameSubMsg);

				curFrameSubMsg = subMsg;
				curFramePtr = (EZFlixFramePtr)pChunkData;

				recPtr->curFrameAvail = TRUE;
				recPtr->curBranchNumber = subMsg->msg.data.branchNumber;
				recPtr->curFrameTime = curFramePtr->time;
				recPtr->curFrameDuration = curFramePtr->duration;
				break;

			default:
				PERR(("DrawEZFlixToBuffer: Unknown chunk type: '%.4s'",
					&pChunkData->subChunkType));
				break;
			}	/* switch */
		}	/* while */

	/* Decompress the last of the new frames we received, if any, and
	 * then release its subMsg. We need to release it for the streamer's
	 * end-of-stream detection to work, and the sooner the better. */
	if ( curFramePtr != NULL )
		{
		uint32		bytesPerPixel;

		bytesPerPixel = GetEZFlixDstStuff(pImageData, bitmap);
		DecompressEZFlix((char *)curFramePtr->frameData,
			curFramePtr->frameSize,
			(char *)pImageData->baseAddr,
			pImageData->rowBytes,
			pImageData->width,
			pImageData->height,
			bytesPerPixel,
			&result);

		/* Write the frame data out of the D cache to RAM, so the video display
		 * generator can read it. */
		FreeEZFlixChunk(ctx, curFrameSubMsg);
		WriteBackDCache(0, (const void *)pImageData->baseAddr,
			pImageData->rowBytes * pImageData->height);
		}

	return result;
	}


/******************************************************************************
|||	AUTODOC -public -class Streaming -group Startup -name NewEZFlixSubscriber
|||	Instantiates an EZFlixSubscriber.
|||
|||	  Synopsis
|||
|||	    Item NewEZFlixSubscriber(DSStreamCBPtr streamCBPtr,
|||	        int32 deltaPriority, Item msgItem, uint32 numChannels,
|||	        EZFlixContextPtr *pContextPtr)
|||
|||	  Description
|||
|||	    Instantiates a new EZFlixSubscriber. This creates the subscriber
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
|||	    numChannels
|||	        The number of channels to support. This puts an upper bound on the
|||	        channel numbers that the EZFlix Subscriber will listen to. There's
|||	        also an implemention max, currently set to 8.
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
|||	    <streaming/ezflixsubscriber.h>, libsubscriber.a
|||
******************************************************************************/
Item	NewEZFlixSubscriber(DSStreamCBPtr streamCBPtr, int32 deltaPriority,
			Item msgItem, uint32 numChannels, EZFlixContextPtr *pContextPtr)
	{
	Err						status;
	uint32					signalBits;
	EZFlixCreationArgs		creationArgs;

	/* Construct the decompressor. TBD - what happens if multiple subscribers
	 * are created? <- Creating another subscriber will cause the old one to
	 * shutdown. Can the decoder handle the overlap? Is there decoder cleanup
	 * that needs to get done after the last subscriber closes down? */
	status = CreateEZFlixDecoder();
	if ( status != 0 )
		goto CLEANUP;

	/* Setup the creation args, including a signal to synchronize with the
	 * completion of the subscriber's initialization. It will signal us when it
	 * is done initializing itself, successfully or not. */
	status = kDSNoSignalErr;
	creationArgs.creatorTask	= CURRENTTASKITEM;	/* cf. <kernel/kernel.h>, included by <streaming/threadhelper.h> */
	creationArgs.creatorSignal	= AllocSignal(0);
	if ( creationArgs.creatorSignal == 0 )
		goto CLEANUP;
	creationArgs.streamCBPtr		= streamCBPtr;
	creationArgs.numChannels		= numChannels;
	creationArgs.creationStatus		= kDSInitErr;
	creationArgs.requestPort		= -1;
	creationArgs.contextPtr			= NULL;

	/* Create the thread that will handle all subscriber responsibilities. */
	status = NewThread(
				(void *)(int32)&EZFlixSubscriberThread, 		/* thread entry point */
				4096, 											/* stack size */
				(int32)CURRENT_TASK_PRIORITY + deltaPriority,	/* priority */
				"EZFlixSubscriber",								/* name */
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
				EZFLIX_CHUNK_TYPE, 			/* subscriber data type */
				creationArgs.requestPort);	/* subscriber message port */
	if ( status < 0 )
		goto CLEANUP;

	*pContextPtr = creationArgs.contextPtr;	/* return the created context */
	return creationArgs.requestPort;		/* success! */

CLEANUP:
	/* Something went wrong.
	 * Release any resources we allocated and return the result status.
	 * The subscriber thread will clean up after itself. */
	if ( creationArgs.creatorSignal )	FreeSignal(creationArgs.creatorSignal);
	return status;
	}


/*******************************************************************************************
 * Routine to set the CLUT required by EZFlix on the specified screen.
 *******************************************************************************************/


/*============================================================================
  ============================================================================
			Subscriber thread routines to handle subscriber messages
  ============================================================================
  ============================================================================*/


/*******************************************************************************************
 * Utility routine to disable further data flow for the given channel, and to cause
 * any associated physical processes associated with the channel to stop.
 *******************************************************************************************/
static void DoFlushEZFlixChannel( EZFlixContextPtr ctx, uint32 channelNumber )
	{
	EZFlixChannelPtr	chanPtr;
	SubscriberMsgPtr	msgPtr;
	SubscriberMsgPtr	next;

	if ( channelNumber < ctx->numChannels )
		{
		chanPtr = ctx->channel + channelNumber;

		/* Make sure we are the only thread using this queue. */
		LockSemaphore (chanPtr->dataQueueSem, 1);

		/* Give back all queued chunks for this channel to the
		 * stream parser. We do this by replying to all the
		 * "chunk arrived" messages that we have queued. */
		msgPtr = chanPtr->dataQueue.head;
		while ( msgPtr != NULL )
			{
			/* Get the pointer to the next message in the queue */
			next = (SubscriberMsgPtr) msgPtr->link;

			/* Reply to this chunk so that the stream parser
			 * can eventually reuse the buffer space. */
			ReplyToSubscriberMsg(msgPtr, kDSNoErr);

			/* Continue with the next message in the queue */
			msgPtr = next;
			}

		chanPtr->dataQueue.head = NULL;
		chanPtr->dataQueue.tail = NULL;
		UnlockSemaphore (chanPtr->dataQueueSem);
		}
	}


/*******************************************************************************************
 * Flush all channels in the subscriber.
 *******************************************************************************************/
static void	DoFlushAllEZFlixChannels(EZFlixContextPtr ctx)
	{
	uint32			channelNumber;

	for ( channelNumber = 0; channelNumber < ctx->numChannels; channelNumber++ )
		DoFlushEZFlixChannel(ctx, channelNumber);
	}


/*******************************************************************************************
 * Routine to queue arriving data chunks. All we do here is place the new data into
 * the msg queue for its respective channel. Something else dequeues the data and uses
 * ReplyToSubscriberMsg() to send the empty buffer back to the stream parser.
 *******************************************************************************************/
static Err	DoEZFlixData( EZFlixContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	int32				status = kDSNoErr;
	SubsChunkDataPtr	chunkData = (SubsChunkDataPtr)subMsg->msg.data.buffer;
	EZFlixChannelPtr	chanPtr;
	uint32				channelNumber = chunkData->channel;

	if ( channelNumber < ctx->numChannels )
		{
		chanPtr = ctx->channel + channelNumber;
		AddDataMsgToTail(&chanPtr->dataQueue, subMsg);
		}
	else
		{
		ReplyToSubscriberMsg(subMsg, kDSNoErr);
		status = kDSChanOutOfRangeErr;
		}

	return status;
	}


/*******************************************************************************************
 * Routine to return the status bits of a given channel.
 *******************************************************************************************/
static Err	DoEZFlixGetChan( EZFlixContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	Err				status = kDSChanOutOfRangeErr;
	uint32			channelNumber = subMsg->msg.channel.number;

	if ( channelNumber < ctx->numChannels )
		status = ctx->channel[channelNumber].status;

	return status;
	}


/*******************************************************************************************
 * Routine to set the channel status bits of a given channel.
 *******************************************************************************************/
static Err	DoEZFlixSetChan( EZFlixContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	uint32					channelNumber = subMsg->msg.channel.number;
	EZFlixChannelPtr		chanPtr;
	uint32					mask;

	if ( channelNumber < ctx->numChannels )
		{
		/* Allow only bits that are Read/Write to be set by this call.
		 *
		 * NOTE: 	Any special actions that might need to be taken as as
		 *			result of bits being set or cleared should be taken
		 *			now. If the only purpose served by status bits is to
		 *			enable or disable features, or some other communication,
		 *			then the following is all that is necessary. */
		chanPtr = &ctx->channel[channelNumber];
        mask = subMsg->msg.channel.mask & ~CHAN_SYSBITS;
        chanPtr->status = subMsg->msg.channel.status & mask |
			chanPtr->status & ~mask;
		return kDSNoErr;
		}

	return kDSChanOutOfRangeErr;
	}


/*******************************************************************************************
 * Routine to stop all channels for this subscription.
 *******************************************************************************************/
static Err	DoEZFlixStop( EZFlixContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	/* Stop all logical channels.
	 * No real work is necessary to stop an EZFlix channel, but the
	 * semantics of the call require that queued data be flushed
	 * as requested. */
	if ( subMsg->msg.stop.options & SOPT_FLUSH )
		DoFlushAllEZFlixChannels(ctx);

	return kDSNoErr;
	}


/*******************************************************************************************
 * Routine to
 *******************************************************************************************/
static Err	DoEZFlixSync( EZFlixContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	uint32				channelNumber;
	EZFlixChannelPtr	chanPtr;

	TOUCH(subMsg);	/* prevent compiler warning */

	/* Reply all chunks for each channel back to the DSL */
	/* [TBD] Eliminate fFlushOnSync and just call DoFlushAllEZFlixChannels()? */
	for ( channelNumber = 0; channelNumber < ctx->numChannels; channelNumber++ )
		{
		chanPtr = ctx->channel + channelNumber;

		if ( chanPtr->fFlushOnSync == TRUE )
			DoFlushEZFlixChannel( ctx, channelNumber );
		}

	return kDSNoErr;
	}


/*******************************************************************************************
 * Routine to kill all output, return all queued buffers, and generally stop everything.
 *******************************************************************************************/
static Err	DoEZFlixAbort( EZFlixContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	TOUCH(subMsg); /* prevent compiler warning */

	/* Halt and flush all channels for this subscription. */
	DoFlushAllEZFlixChannels(ctx);

	return kDSNoErr;
	}


/*******************************************************************************
 * Process a stream-branched message: If it's a flush-branch, then flush all
 * data. Also handle a data discontinuity, altho that doesn't affect the EZFlix
 * subscriber since EZFlix has no inter-frame info.
 *******************************************************************************/
static Err	DoEZFlixBranch(EZFlixContextPtr ctx, SubscriberMsgPtr subMsg)
	{
	if ( subMsg->msg.branch.options & SOPT_FLUSH )
		DoFlushAllEZFlixChannels(ctx);;

	return kDSNoErr;
	}


/*=============================================================================
  =============================================================================
					The subscriber thread main routines
  =============================================================================
  =============================================================================*/

static EZFlixContextPtr
InitializeEZFlixThread( EZFlixCreationArgs* creationArgs )
	{
	EZFlixContextPtr	ctx;
	Err					status;
	uint32				channelNumber;
	EZFlixChannelPtr	chanPtr;
	char				semName[8];

	/* Allocate the subscriber context structure (instance data) zeroed
	 * out, and start initializing fields. */
	status = kDSNoMemErr;
	ctx = AllocMem(sizeof(*ctx), MEMTYPE_FILL);
	if ( ctx == NULL )
		goto BAILOUT;
	/* ctx->streamCBPtr		= creationArgs->streamCBPtr; */
	ctx->numChannels		= creationArgs->numChannels;
	if ( ctx->numChannels > EZFLIX_MAX_CHANNELS )
		ctx->numChannels = EZFLIX_MAX_CHANNELS;
	creationArgs->contextPtr = ctx;

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
	**	EZFlix specific initializations
	** ***  ***  ***  ***  ***  ***  ***  ***/

	/* Initialize channel structures */
	chanPtr = ctx->channel;
	for ( channelNumber = 0; channelNumber < ctx->numChannels;
			channelNumber++, chanPtr++ )
		{
		chanPtr->status	= CHAN_ENABLED;

		/* Create a semaphore to manage access to the dataQueue list for this
		 * channel */
		semName[0] = 'c';
		semName[1] = 'p';
		semName[2] = (char)('0' + channelNumber);		/* {"0","1",...} */
		semName[3] = 0;
		status = chanPtr->dataQueueSem = CreateSemaphore(semName, 100);
		if ( status < 0 )
			goto BAILOUT;
		}

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
		FreeMem(ctx, sizeof(*ctx));
		ctx = NULL;
		}

	return ctx;
	}


/*******************************************************************************************
 * This thread is started by a call to InitEZFlixSubscriber(). It reads the subscriber message
 * port for work requests and performs appropriate actions. The subscriber message
 * definitions are located in "DataStreamLib.h".
 *******************************************************************************************/
static void	EZFlixSubscriberThread(int32 notUsed,
		EZFlixCreationArgs *creationArgs)
	{
	EZFlixContextPtr 	ctx;
	Err					status = kDSNoErr;
	uint32				signalBits;
	uint32				anySignal;
	bool				fKeepRunning = TRUE;

	TOUCH(notUsed);		/* prevent compiler warning */

	/* Call a utility routine to perform all startup initialization. */
	ctx = InitializeEZFlixThread(creationArgs);
	creationArgs = NULL;	/* can't access that memory anymore */
	TOUCH(creationArgs);	/* avoid a compiler warning */
	if (ctx == NULL)
		return;

	/* All resources are now allocated and ready to use. Our creator has been informed
	 * that we are ready to accept requests for work. All that's left to do is
	 * wait for work request messages to arrive. */
	anySignal = ctx->requestPortSignal;

	while ( fKeepRunning )
		{
		signalBits = WaitSignal( anySignal );

		/********************************************************/
		/* Check for and process and incoming request messages. */
		/********************************************************/
		if ( signalBits & ctx->requestPortSignal )
			{
			SubscriberMsgPtr	subMsg;

			/* Process any new requests for service as determined by the incoming
			 * message data. */
			while( PollForMsg(ctx->requestPort, NULL, NULL, (void **)&subMsg,
					&status) )
				{

				switch ( subMsg->whatToDo )
					{
					case kStreamOpData:				/* new data has arrived */
						status = DoEZFlixData( ctx, subMsg );
						break;

					case kStreamOpGetChan:			/* get logical channel status */
						status = DoEZFlixGetChan( ctx, subMsg );
						break;

					case kStreamOpSetChan:			/* set logical channel status */
						status = DoEZFlixSetChan( ctx, subMsg );
						break;

					case kStreamOpControl:			/* perform subscriber defined function */
						status = kDSNoErr;
						break;

					case kStreamOpSync:				/* clock stream resynched the clock */
						status = DoEZFlixSync( ctx, subMsg );
						break;

					case kStreamOpOpening:			/* one time initialization call from DSH */
						status = kDSNoErr;
						break;

					case kStreamOpClosing:			/* stream is being closed */
						status = kDSNoErr;
						fKeepRunning = FALSE;
						break;

					case kStreamOpStop:				/* stream is being stopped */
						status = DoEZFlixStop( ctx, subMsg );
						break;

					case kStreamOpEOF:				/* physical EOF on data, no more to come */
						status = kDSNoErr;
						break;

					case kStreamOpAbort:			/* somebody gave up, stream is aborted */
						status = DoEZFlixAbort( ctx, subMsg );
						break;

					case kStreamOpBranch:			/* data discontinuity */
						status = DoEZFlixBranch(ctx, subMsg);
						break;

					default:
						;
					}

				/* Reply to the request we just handled unless this is a "data arrived"
				 * message. Those are replied to asynchronously as the data is actually
				 * consumed and must not be replied to here. */
				if ( subMsg->whatToDo != kStreamOpData )
					ReplyToSubscriberMsg(subMsg, status);
				}
			}
		}

	/* Halt and flush all channels for this subscription. */
	DoFlushAllEZFlixChannels(ctx);

	/* Dispose all memory we allocated and clean up any static state.
	 * The OS will automatically reclaim the Items we allocated.
	 * Free all memory we allocated. */
	FreeMem(ctx, sizeof(*ctx));
	}

