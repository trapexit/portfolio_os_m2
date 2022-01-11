
/*****************************************************************************
**
**  @(#) mpegaudiosubscriber.c 96/11/26 1.4
**
*****************************************************************************/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <audio/audio.h>

#ifndef __KERNEL_TYPES_H
	#include <kernel/types.h>
#endif

#ifndef __KERNEL_DEBUG_H
	#include <kernel/debug.h>
#endif

#ifndef __KERNEL_MEM_H
#include <kernel/mem.h>
#endif

/* #include <streaming/datastreamlib.h> */
/* #include <streaming/dserror.h>  */ /* for DS error codes */
/* #include <streaming/mempool.h> */
#include <streaming/threadhelper.h>

#include <video_cd_streaming/mpegaudiosubscriber.h>
#include <streaming/subscribertraceutils.h>
#include "mpadecoderinterface.h"
#include "sasupport.h"
#include "sachannel.h"
#include "sasoundspoolerinterface.h"

/* This is the number of DecoderMsg structure that gets allocated
 * to be used for sending messages to the decoder. */
#define NUM_DECODER_MSGS	230

/* This is the delta priority of the decoder thread.  The idea is
 * to have the decoder run at a lower priority than the rest of
 * the streamer thread.  This may need to be adjusted. */
#define DECODER_DELTA_PRI	-7

/*****************************************************************************
 * Compile switch implementations
 *****************************************************************************/

#if SAUDIO_TRACE_MAIN

	#ifndef TRACE_LEVEL	 /* so it can be predefined via a compiler arg */
		#define TRACE_LEVEL	 1
	#endif

	/* Allow for multiple levels of tracing */
	#if (TRACE_LEVEL >= 1)
		#define	 ADD_TRACE_L1(bufPtr, event, chan, value, ptr)   \
						AddTrace(bufPtr, event, chan, value, ptr)
	#else
		#define	 ADD_TRACE_L1(bufPtr, event, chan, value, ptr)
	#endif

	#if (TRACE_LEVEL >= 2)
		#define	 ADD_TRACE_L2(bufPtr, event, chan, value, ptr)   \
						AddTrace(bufPtr, event, chan, value, ptr)
	#else
		#define	 ADD_TRACE_L2(bufPtr, event, chan, value, ptr)
	#endif

	#if (TRACE_LEVEL >= 3)
		#define	 ADD_TRACE_L3(bufPtr, event, chan, value, ptr)   \
						AddTrace(bufPtr, event, chan, value, ptr)
	#else
		#define	 ADD_TRACE_L3(bufPtr, event, chan, value, ptr)
	#endif

#else   /* Trace is off */
	#define	 ADD_TRACE_L1(bufPtr, event, chan, value, ptr)
	#define	 ADD_TRACE_L2(bufPtr, event, chan, value, ptr)
	#define	 ADD_TRACE_L3(bufPtr, event, chan, value, ptr)
#endif

/********************************************
 * Local function prototypes
 ********************************************/
static SAudioContextPtr InitializeMPEGAudioThread(
	SAudioCreationArgs *creationArgs, uint32 numberOfBuffers );
static bool AllocMsgItemsFunc( void *replyPort, void *poolEntry );
static bool DeallocMsgItemsFunc( void *replyPort, void *poolEntry );
static bool InitPoolEntryWithMsgItem( MemPoolPtr memPool, Item msgItem );
static Err HandleRepliesFromDecoder( SAudioContextPtr ctx );
static void TearDownMPEGAudioCB( SAudioContextPtr ctx,
		uint32 numberOfBuffers );

/********************************************
 * Main MPEG audio Subscriber thread routine
 ********************************************/
static void	MPEGAudioSubscriberThread( int32 notUsed,
		SAudioCreationArgs *creationArgs );


/******************************************************************************
 * Handle replies from the decoder.
******************************************************************************/
static Err HandleRepliesFromDecoder( SAudioContextPtr ctx )
{
	Err				status = kDSNoErr;
	Item			msgItem;
	DecoderMsgPtr	decoderMsg;
	Message			*messagePtr;

	/* Queue the decompressed buffers to the soundspooler. */
	while( (msgItem = GetMsg(ctx->datatype.MPEGAudio.replyPort)) > 0 )
	{
		messagePtr		= MESSAGE(msgItem);
		decoderMsg		= messagePtr->msg_DataPtr;

		ADD_TRACE_L1( SATraceBufPtr, kGotDecoderReply,
			decoderMsg->channel, decoderMsg->messageType, decoderMsg );
		ctx->datatype.MPEGAudio.pendingReplies--;
#ifdef DEBUG_PRINT
		PRNT(("Got reply for msg# %d, msg addr @ 0x%x\n",
				decoderMsg->messageType, decoderMsg ));
#endif
		switch( decoderMsg->messageType )
			{
			case compressedBfrMsg:
				ADD_TRACE_L1( SATraceBufPtr,
					kTraceCompressedBfrMsg,
					decoderMsg->messageType,
					(decoderMsg->timeIsValidFlag ? decoderMsg->presentationTime : -1), decoderMsg );

				/* Return this buffer to the streamer. */
#ifdef DEBUG_PRINT
				PRNT(( "ReplyToSubscriberMsg, subMmsgPtr addr @ 0x%x\n",
					decoderMsg->subMsgPtr ));
#endif
				status = ReplyToSubscriberMsg( decoderMsg->subMsgPtr, 0 );
				CHECK_NEG( "ReplyToSubscriberMsg", status );

				break;

			case decompressedBfrMsg:
				ADD_TRACE_L1( SATraceBufPtr,
					kTraceReceivedDecompressedBfrMsg,
					decoderMsg->channel,
					(decoderMsg->timeIsValidFlag ? decoderMsg->presentationTime : -1), decoderMsg );

				/* Queue it for playback. */
				if( messagePtr->msg_Result == kDSNoErr )
				{
					/* Queue decompressed buffers */
					status = QueueDecompressedBfrsToSoundSpooler
								( ctx, decoderMsg );
					CHECK_NEG( "QueueDecompressedBfrsToSoundSpooler", status );
					BeginSAudioPlaybackIfAppropriate( ctx,
						decoderMsg->channel );
 				}
				else
					/* This buffer may have been flushed or something.
					 * Recycle this buffer back to the decompressed bfr
					 * pool. */
					ReturnPoolMem( ctx->datatype.MPEGAudio.decompressedBfrMsgPool,
									decoderMsg );
 				break;

			case flushReadMsg:
				ADD_TRACE_L1( SATraceBufPtr,
					kTraceFlushReadMsg,
					decoderMsg->channel,
					decoderMsg->messageType, decoderMsg );
				break;

			case flushWriteMsg:
				ADD_TRACE_L1( SATraceBufPtr,
					kTraceFlushWriteMsg,
					decoderMsg->channel,
					decoderMsg->messageType, decoderMsg );
				break;

			case flushReadWriteMsg:
				ADD_TRACE_L1( SATraceBufPtr,
					kTraceFlushReadWriteMsg,
					decoderMsg->channel,
					decoderMsg->messageType, decoderMsg );
				break;

			case closeDecoderMsg:
				ADD_TRACE_L1( SATraceBufPtr,
					kTraceCloseDecoderMsg,
					decoderMsg->channel,
					decoderMsg->messageType, decoderMsg );
				break;

			} /* switch */

			/* Return the decoder message to the pool as long as
			 * it's not a decompressedBfrMsg. */
			if( decoderMsg->messageType != decompressedBfrMsg )
				ReturnPoolMem( ctx->datatype.MPEGAudio.decoderMsgPool,
					decoderMsg );

		} /* while */
		CHECK_NEG( "GetMsg", msgItem );

	return status;

} /* HandleRepliesFromDecoder() */

/******************************************************************************
 * Tear down a MPEG audio control block.  Dispose of all non-system
 * resources that were create. But first wait until all replies from
 * the decoder have been received.
******************************************************************************/
static void TearDownMPEGAudioCB( SAudioContextPtr ctx,
		uint32 numberOfBuffers )
{

	Err		status;

	while( ctx->datatype.MPEGAudio.pendingReplies > 0 )
	{
		WaitSignal( ctx->datatype.MPEGAudio.replyPortSignal );
		status = HandleRepliesFromDecoder( ctx );
		CHECK_NEG( "HandleRepliesFromDecoder", status );
	}

	/* Dispose all memory we allocated and clean up any static state.
	 * The OS will automatically reclaim the Items we allocated. */
	if ( ctx != NULL )
		{
		/* Get rid of memory pool. */
		DeleteMemPool( ctx->datatype.MPEGAudio.decoderMsgPool );
		DeleteMemPool( ctx->datatype.MPEGAudio.decompressedBfrMsgPool );

		/* Get rid of audiobfrs */
		FreeMem( ctx->datatype.MPEGAudio.audioBfrs,
			numberOfBuffers * SIZE_OF_DECOMPRESSED_BUFFER );

		FreeMem(ctx, sizeof(*ctx));
		} /* if( ctx != NULL ) */

} /* TearDownMPEGAudioCB() */

/******************************************************************************
 * Allocate message item for an entry in the memory pool.
******************************************************************************/
static bool AllocMsgItemsFunc( void *replyPort, void *poolEntry )
{
	Item	msgItem;
	((DecoderMsgPtr)poolEntry)->msgItem = msgItem =
		 CreateMsgItem((Item)replyPort);

	return (msgItem >= 0);

} /* AccocMsgItemsFunc() */

/******************************************************************************
 * Deallocate message item for an entry in the memory pool.
******************************************************************************/
static bool DeallocMsgItemsFunc( void *replyPort, void *poolEntry )
{
	TOUCH(replyPort);
	DeleteItem( ((DecoderMsgPtr)poolEntry)->msgItem );
	return true;

} /* DeallocMsgItemsFunc() */

/******************************************************************************
 * Initialize each pool entry with its system resources.
******************************************************************************/
static bool InitPoolEntryWithMsgItem( MemPoolPtr memPool, Item msgItem )
{
	bool	success;

	if ( memPool == NULL )
		return FALSE;

	/* Initialize each pool entry with its system resources. */
	success = ForEachFreePoolMember( memPool, AllocMsgItemsFunc,
					(void *)msgItem );

	if( !success )
	{
		ForEachFreePoolMember( memPool, DeallocMsgItemsFunc, (void *)msgItem );
	}

#ifdef DEBUG_PRINT
{
	PRNT(("Addr of memPool 0x%x\n", memPool));
	PRNT(("sizeof(DecoderMsg) %d\n", sizeof(DecoderMsg)));
	PRNT(("memPool->totalPoolSize %d\n",
	memPool->totalPoolSize));
	PRNT(("memPool->numItemsInPool %d\n",
	memPool->numItemsInPool));
	PRNT(("memPool->numFreeInPool %d\n",
	memPool->numFreeInPool));
	PRNT(("Addr of memPool->MemPoolEntry %d\n",
	memPool->data));
}
#endif

	return success;

} /* InitPoolEntryWithMsgItem() */

/*=============================================================================*/
/* Do one-time initialization for the MPEG audio subscriber thread: Allocate
 * is context structure (instance data), allocate system resources, etc.
 *
 * RETURNS: The new context pointer if successful or NULL if failed.
 * SIDE EFFECTS: To communicate with the spawning process, this fills in the
 *    creationStatus and requestPort fields of the creationArgs structure and
 *    then sends a signal to the spawning process.
 * NOTE: Once we signal the spawning process, the creationArgs structure will
 *    go away out from under us. */

static SAudioContextPtr InitializeMPEGAudioThread(
	SAudioCreationArgs *creationArgs, uint32 numberOfBuffers )
{
 	SAudioContextPtr    ctx;
 	Err                 status;
	int32				index;

	/* Allocate the subscriber context structure (instance data), zeroed
	 * out, and start initializing fields.
	 * Default clock channel to 0 */

	status = kDSNoMemErr;
	ctx = AllocMem(sizeof(*ctx), MEMTYPE_FILL);
	if ( ctx == NULL )
	    goto FAILED;
	ctx->streamCBPtr        = creationArgs->streamCBPtr;

	/* Create the message port for MPEG audio decoder replies. */
	status = kDSNoPortErr;
	ctx->datatype.MPEGAudio.replyPort =
		NewMsgPort( &ctx->datatype.MPEGAudio.replyPortSignal );
	if( ctx->datatype.MPEGAudio.replyPort < 0 )
		goto FAILED;

	/* Create a pool of messages for sending decompressed buffers to be
	 * filled with data, to MPEG audio decoder. The number of decoder
	 * messages should be is the number of decompressed audio buffers.
	 * This is kept separate from the decoderMsgPool because these
	 * messages will be filled and reused for this purpose only. */
	status = kDSNoMemErr;
	ctx->datatype.MPEGAudio.decompressedBfrMsgPool =
		CreateMemPool(creationArgs->numberOfBuffers, sizeof(DecoderMsg));

	if( !InitPoolEntryWithMsgItem( ctx->datatype.MPEGAudio.decompressedBfrMsgPool,
		ctx->datatype.MPEGAudio.replyPort ) )
		goto FAILED;

	/* Create a pool of messages for sending data and commands to
	 * MPEG audio decoder. The number of decoder messages should be
	 * at least the number of decompressed audio buffers. */
	status = kDSNoMemErr;
	ctx->datatype.MPEGAudio.decoderMsgPool =
		CreateMemPool(NUM_DECODER_MSGS, sizeof(DecoderMsg));
	if( !InitPoolEntryWithMsgItem( ctx->datatype.MPEGAudio.decoderMsgPool,
		ctx->datatype.MPEGAudio.replyPort ) )
		goto FAILED;

	/* Initialize numBuffers for all channels.  This is the number
	 * of buffers allocated for queuing to the soundspooler (for
	 * each channel). */
	for ( index = 0; index < SA_SUBS_MAX_CHANNELS; index++ )
		ctx->channel[index].numBuffers = numberOfBuffers;

	/* Open the Audio Folio for this thread */
	if ( (status = OpenAudioFolio() ) < 0 )
		goto FAILED;

	/* Load the envelope instrument template. This gets used every time
	 * a channel is opened to create an envelope instrument for the channel
	 * so we can ramp amplitudes on start and stop. */
	status = ctx->envelopeTemplateItem =
		LoadInsTemplate( "envelope.dsp", 0 );
	if ( status < 0 )
		goto FAILED;

	/* Do the rest of initialization. */
	status = InitializeSAudioThread( ctx );
	FAIL_NEG( "InitializeSAudioThread", status );

	creationArgs->requestPort = ctx->requestPort;

FAILED:
	/* Inform our creator that we've finished with initialization.
	 *
	 * If initialization failed, clean up resources we allocated, letting the
	 * system release system resources. We need to free up memory we
	 * allocated and restore static state. */
	creationArgs->creationStatus = status;  /* return info to the creator task */

	SendSignal( creationArgs->creatorTask, creationArgs->creatorSignal );
    creationArgs = NULL;    /* can't access this memory after sending the signal */
    TOUCH(creationArgs);    /* avoid a compiler warning */

	if ( status < 0 )
		{
		FreeMem(ctx, sizeof(*ctx));

		/* Get rid of memory pool. */
		DeleteMemPool( ctx->datatype.MPEGAudio.decoderMsgPool );
		DeleteMemPool( ctx->datatype.MPEGAudio.decompressedBfrMsgPool );
		ctx = NULL;
		}

    return ctx;

} /* InitializeMPEGAudioThread() */

Item NewMPEGAudioSubscriber( DSStreamCBPtr streamCBPtr,
							int32 deltaPriority,
							Item msgItem,
							uint32 numberOfBuffers )
	{
	Err						status;
    SAudioCreationArgs  	creationArgs;
	uint32					signalBits;


	ADD_TRACE_L1( SATraceBufPtr, kTraceNewSubscriber, 0, 0, 0 );

	/* Setup the creation args, including a signal to synchronize with the
	 * completion of the subscriber's initialization. It will signal us when it
	 * is done initializing itself, successfully or not. */
	status = kDSNoSignalErr;
	creationArgs.creatorTask    = CURRENTTASKITEM;  /* cf. <kernel/kernel.h>, included by <streaming /threadhelper.h> */
	creationArgs.creatorSignal  = AllocSignal(0);
	if ( creationArgs.creatorSignal == 0 )
		goto CLEANUP;
	creationArgs.streamCBPtr        = streamCBPtr;
	creationArgs.creationStatus     = -1;
	creationArgs.requestPort        = -1;
	creationArgs.numberOfBuffers    = numberOfBuffers;


	/* Create the thread that will handle all subscriber responsibilities. */
	status = NewThread(
		(void *)(int32)&MPEGAudioSubscriberThread,		/* thread entry point */
		4096, 											/* stack size */
		(int32)CURRENT_TASK_PRIORITY + deltaPriority,	/* priority */
		"MPEGAudioSubscriber", 							/* name */
		0, 												/* first arg to the thread */
		&creationArgs);									/* second arg to the thread */

	if ( status < 0 )
		goto CLEANUP;

	/* Wait here while the subscriber initializes itself. */
	status = kDSSignalErr;
	signalBits = WaitSignal( creationArgs.creatorSignal );
	if ( signalBits != creationArgs.creatorSignal )
		goto CLEANUP;

	/* We're done with this signal, so give it back */
	FreeSignal( creationArgs.creatorSignal );
	creationArgs.creatorSignal = 0;

	/* Check the initialization status of the subscriber. If anything
	 * failed, the 'ctx->creatorStatus' field will be set to a system
	 * result code. If this is >= 0 then initialization was successful. */
	status = creationArgs.creationStatus;
	if ( status < 0 )
		goto CLEANUP;

	status = DSSubscribe(msgItem, NULL,     /* a synchronous request */
				streamCBPtr,                /* stream context block */
				MPAU_CHUNK_TYPE,            /* subscriber data type */
				creationArgs.requestPort);  /* subscriber message port */
	if ( status < 0 )
		goto CLEANUP;

	return creationArgs.requestPort;        /* success! */

CLEANUP:
	/* Something went wrong.
	 * Release any resources we allocated and return the result status.
	 * The subscriber thread will clean up after itself. */
	if ( creationArgs.creatorSignal )	FreeSignal(creationArgs.creatorSignal);
	return status;

} /* NewMPEGAudioSubscriber() */

/******************************************************************************
 * MPEGAudioSubscriberThread.
 * This thread reads the subscriber message port for work requests and performs
 * appropriate actions.
 ******************************************************************************/
static void	MPEGAudioSubscriberThread( int32 notUsed, SAudioCreationArgs *creationArgs )
	{
	SAudioContextPtr    ctx;
	Err					status;
	bool				fKeepRunning = true;
	int32				signalBits;
	uint32				anySignal;
	uint32				numberOfBuffers = creationArgs->numberOfBuffers;

	TOUCH(notUsed);			/* avoid a compiler warning */

	/* Call a subroutine to perform all startup initialization. */
	ctx = InitializeMPEGAudioThread(creationArgs, numberOfBuffers );

	/* Create MPEG audio decoder thread */
	status = NewMPADecoder( ctx, DECODER_DELTA_PRI );
	FAIL_NEG("NewMPADecoder", status );
	ctx->datatype.MPEGAudio.decoderPort = status;

	/* Set the lastTimeIsValid to true */
	ctx->datatype.MPEGAudio.lastTimeIsValid = true;

	/* Allocate memory pool for decompressed audio buffers . */
	ctx->datatype.MPEGAudio.audioBfrs =
		AllocMem( numberOfBuffers * SIZE_OF_DECOMPRESSED_BUFFER,
			MEMTYPE_FILL );

	if( ctx->datatype.MPEGAudio.audioBfrs == NULL )
		goto FAILED;

	creationArgs = NULL;    /* can't access that memory anymore */
	TOUCH(creationArgs);	/* avoid a compiler warning */

    if ( ctx == NULL )
        goto FAILED;

	anySignal = ctx->requestPortSignal |
				ctx->datatype.MPEGAudio.replyPortSignal;

	/* All resources are now allocated and ready to use. Our creator has
	 * been informed that we are ready to accept requests for work. All
	 * that's left to do is wait for work request messages to arrive.
	 */
	while ( fKeepRunning )
		{

		/* Note: This must be within the main loop because the signals are
		 * assigned when a soundspooler (one for each channel) is created in
		 * InitMPEGAudioChannel() .  InitMPEGAudioChannel() is called whenever a new
		 * audio header chunk is received which may be anywhere within the stream.
		 */
		anySignal |= ctx->SpoolersSignalMask;

		ADD_TRACE_L1( SATraceBufPtr, kTraceWaitingOnSignal, -1,
			(ctx->requestPortSignal |
			ctx->datatype.MPEGAudio.replyPortSignal |
			ctx->SpoolersSignalMask), 0 );

		signalBits = WaitSignal( anySignal );
		CHECK_NEG( "WaitSignal", signalBits );

		ADD_TRACE_L1( SATraceBufPtr, kTraceGotSignal, -1, signalBits, 0 );

		/*******************************************************/
		/* Check for and process any sample buffer completions */
		/*******************************************************/
		if ( signalBits & ctx->SpoolersSignalMask )
		{
			HandleCompletedMPABuffers( ctx, signalBits );
		}


		/********************************************************/
		/* Check for and process replies from the decoder.      */
		/* We get the following types of replies:               */
		/*  1) closeDecoderMsg - exit.                          */
		/*  2) decompressedBfrMsg - data is decompressed and    */
		/*     ready for playback.                              */
		/*	3) all other replies, we recycle the message    */
		/********************************************************/
		if ( signalBits & ctx->datatype.MPEGAudio.replyPortSignal )
			{
			status = HandleRepliesFromDecoder( ctx );
			CHECK_NEG( "HandleRepliesFromDecoder", status );
			} /* if ( signalBits & ctx->replyPortSignal ) */


		/********************************************************/
		/* Check for and process incoming request messages.     */
		/********************************************************/
		if ( signalBits & ctx->requestPortSignal )
			{
			status = ProcessRequestMsgs( ctx, &fKeepRunning );
			CHECK_NEG( "ProcessRequestMsgs", status );
			}


		} /* while (fKeepRunning) */


FAILED:
	TearDownMPEGAudioCB( ctx, numberOfBuffers );

	} /* MPEGAudioSubscriberThread() */
