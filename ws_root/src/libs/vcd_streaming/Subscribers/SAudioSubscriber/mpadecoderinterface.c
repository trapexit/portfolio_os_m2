/******************************************************************************
**
**  @(#) mpadecoderinterface.c 96/11/26 1.3
**
******************************************************************************/

#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_MEM_H
#include <kernel/mem.h>
#endif

#ifndef __KERNEL_TASK_H
#include <kernel/task.h>
#endif

#ifndef __KERNEL_DEBUG_H
#include <kernel/debug.h>
#endif

#ifndef __MISC_MPAUDIODECODE_H
#include <video_cd_streaming/mpaudiodecode.h>
#endif

#ifndef __STREAMING_THREADHELPER_H
#include <streaming/threadhelper.h>
#endif

#include <stdlib.h>
#include "sachannel.h"
#include "mpadecoderinterface.h"

typedef struct DecoderThreadCtx {
  uint32				channel;			/* channel number the data belongs to */
  uint32				branchNumber;		/* branch number used in conjunction with the presentaionTime when setting the clock */
  MPADecoderContext *	decoderCtxPtr;		/* ptr to MPEG audio decoder context */
  Item					msgPort;			/* messages from the subscriber */
  uint32				msgPortSignal;		/* signal associated with msgPort */
  List					CompressedBfrQueue;	/* List of compressed data buffers */
  List					DecompressedBfrQueue;/* List of buffers to hold decompressed data */
  DecoderMsgPtr			curCompressedBufPtr;/* ptr to the current compressed buffer */
} DecoderThreadCtx, *DecoderThreadCtxPtr;
 
typedef struct MPADecoderCreationArgs {
	Item			creatorTask;	/* who to signal when done initializing */
	uint32			creatorSignal;	/* signal to send when done initializing */
	Err				creationStatus;	/* new thread's startup status */
	Item			requestPort;	/* new thread's request msgport */
} MPADecoderCreationArgs;

/*****************************************************************************
 * Compile switch implementations
 *****************************************************************************/

#if SAUDIO_TRACE_MPABUFFERQUEUE

	#define TRACE_LEVEL		2
	
	/* Find the actual trace buffer. It's declared in SAMain.c. */
	extern	TraceBufferPtr	SATraceBufPtr;
	
	/* Allow for multiple levels of tracing */
	#if (TRACE_LEVEL >= 1)
		#define		ADD_TRACE_L1(bufPtr, event, chan, value, ptr)	\
						AddTrace(bufPtr, event, chan, value, ptr)
	#else
		#define		ADD_TRACE_L1(bufPtr, event, chan, value, ptr)
	#endif
	
	#if (TRACE_LEVEL >= 2)
		#define		ADD_TRACE_L2(bufPtr, event, chan, value, ptr)	\
						AddTrace(bufPtr, event, chan, value, ptr)
	#else
		#define		ADD_TRACE_L2(bufPtr, event, chan, value, ptr)
	#endif
	
	#if (TRACE_LEVEL >= 3)
		#define		ADD_TRACE_L3(bufPtr, event, chan, value, ptr)	\
						AddTrace(bufPtr, event, chan, value, ptr)
	#else
		#define		ADD_TRACE_L3(bufPtr, event, chan, value, ptr)
	#endif
	
	
#else  /* Trace is off */
	#define		ADD_TRACE_L1(bufPtr, event, chan, value, ptr)
	#define		ADD_TRACE_L2(bufPtr, event, chan, value, ptr)
	#define		ADD_TRACE_L3(bufPtr, event, chan, value, ptr)
#endif /* SAUDIO_TRACE_MPABUFFERQUEUE */

/*****************************************************************************
 * Local function prototypes.
 *****************************************************************************/

static Err GetCompressedBfr(const void *unit, const uint8 **buf, int32 *len, uint32 *presentationTime, uint32 *timeIsValidFlag);
static Err CompressedBfrRead(const void *unit, uint8 *buf);

/******************************************************************************
 * ExitDecoderThread
 *
 *	Release any resources we've acquired and exit.  If we're given a decoderMsg
 *	pointer, it's the closeDecoder request from our parent.  In this case, we
 *	reply to the request only after releasing our resources, then we exit as
 *	quickly as possible after replying.  The reason for this is that we run at
 *	a lower priority than our parent, and once we've replied to the close msg
 *	we probably won't get any more cycles at all and our parent thread will 
 *	disappear out from under us.  If we didn't free up memory before replying
 *	to the close request it would never get freed at all (can you say "the memory
 *	leak is finally fixed?"  I knew you could.)
 ******************************************************************************/
 
static void ExitDecoderThread(DecoderThreadCtxPtr ctx, DecoderMsgPtr decoderMsg)
{
	if (ctx) {
		if (ctx->decoderCtxPtr) {
			DeleteMPAudioDecoder(ctx->decoderCtxPtr);
		}
		FreeMem(ctx, sizeof(*ctx));
	}
	
	if (decoderMsg) {
		ReplyMsg(decoderMsg->msgItem, 0, decoderMsg, sizeof(*decoderMsg));
	}
	
	exit(0);
}
 
/******************************************************************************
 * InitializeDecoderThread
 * Do one-time initialization for the MPEG audio decoder thread:
 * allocate the decoder context structure (instance data), allocate
 * system resources, etc.
 * RETURNS: The new context pointer if successful or NULL if failed.
 ******************************************************************************/
 
static DecoderThreadCtxPtr InitializeDecoderThread(MPADecoderCreationArgs *creationArgs)
{
	Err					status;
	MPACallbackFns		CallbackFns;
	DecoderThreadCtxPtr	ctx;

	ctx = AllocMem(sizeof(*ctx), MEMTYPE_FILL);
	if (!ctx) {
		status = kDSNoMemErr;
		goto BAILOUT;
	}
	
	PrepList(&ctx->CompressedBfrQueue);
	PrepList(&ctx->DecompressedBfrQueue);

	CallbackFns.CompressedBfrReadFn = (MPACompressedBfrReadFn)CompressedBfrRead;
	CallbackFns.GetCompressedBfrFn  = (MPAGetCompressedBfrFn)GetCompressedBfr;

	status = CreateMPAudioDecoder(&ctx->decoderCtxPtr, CallbackFns);
	if (status < 0) {
		goto BAILOUT;
	}
	
	ctx->msgPort = status = NewMsgPort(&ctx->msgPortSignal);
	if (status < 0) {
		goto BAILOUT;
	}
	
	creationArgs->requestPort = ctx->msgPort;

BAILOUT:

	if (status < 0) {
		if (ctx->decoderCtxPtr) {
			DeleteMPAudioDecoder(ctx->decoderCtxPtr);
		}
		FreeMem(ctx, sizeof(*ctx));
		ctx = NULL;
	}

	creationArgs->creationStatus = status;

	SendSignal(creationArgs->creatorTask, creationArgs->creatorSignal);
	
	return ctx;

}


/**********************************************************************
 * FlushCompressedBuffers
 * Flush out the compressed buffer queue and the decoder. 
 * The decoder will call the CompressedBfrRead() which will reply to the
 * subscriber the curCompressedBufPtr.
 **********************************************************************/
 
static Err FlushCompressedBuffers(DecoderThreadCtxPtr ctx)
{
	Err				status;
	DecoderMsgPtr	decoderMsg;
	ListP			InputQueue	= &ctx->CompressedBfrQueue;

	while (!IsEmptyList(InputQueue)) {
		decoderMsg = (DecoderMsgPtr)RemHead(InputQueue);
		ReplyMsg(decoderMsg->msgItem, kDSNoErr, decoderMsg, sizeof(*decoderMsg));
	}

	status = MPAFlush(ctx->decoderCtxPtr);
	CHECK_NEG("MPAFlush", status);

	return 0;

}
/**********************************************************************
 * FlushDecompressedBuffers
 * Flush out the decompressed buffer queue.
 **********************************************************************/
 
static Err FlushDecompressedBuffers(DecoderThreadCtxPtr ctx)
{
	DecoderMsgPtr	decoderMsg;
	ListP 			OutputQueue	= &ctx->DecompressedBfrQueue;
	
	while (!IsEmptyList(OutputQueue)) {
		decoderMsg = (DecoderMsgPtr)RemHead(OutputQueue);
		ReplyMsg(decoderMsg->msgItem, kDSWasFlushedErr, decoderMsg, sizeof(*decoderMsg));
	}

	return 0;
}

/**********************************************************************
 * ProcessOneMsg
 *
 * Decode and handle one message from the subscriber.
 *
 *	If we're in the decoder (IE, in the callback to get more data), we
 *	want to process buffer messages, but defer processing of other types
 *	of messages until we leave the decoder.  The way we defer processing
 *	is to return the message item number, which then gets returned from the
 *	callback to the decoder. The decoder returns the value to the thread's 
 *	main loop, at which point the message item gets fed back into this
 *	routine with the inDecoder flag false, and we can finally act on
 *	the message.  The reason for all this complexity is that you can't
 *	flush the decoder from within the get-more-data callback; you have
 *	to exit the decoder before the buffers it's holding onto are 
 *	available for flushing back to the subscriber.
 *
 *	When we get the close message, we call a routine that releases all
 *	resources we've acquired, then replies to the close request msg, 
 *	then calls exit().  
 **********************************************************************/
 
static Err ProcessOneMsg(DecoderThreadCtxPtr ctx, Boolean inDecoder, Item msgItem)
{
	Message	*		msgPtr; 
	DecoderMsgPtr	decoderMsg;
	Err				status = 0;
	
	if ((msgPtr = MESSAGE(msgItem)) != NULL) {
		decoderMsg = msgPtr->msg_DataPtr;
		switch(decoderMsg->messageType) 	{
		  case compressedBfrMsg:
			AddTail(&ctx->CompressedBfrQueue, (Node *)decoderMsg);
			break;

		  case decompressedBfrMsg:
			AddTail(&ctx->DecompressedBfrQueue, (Node *)decoderMsg);
			break;
			
		  default:
		  	if (inDecoder) {
				status = msgItem;
			} else {
				switch(decoderMsg->messageType) 	{
				  case flushReadMsg:
					FlushCompressedBuffers(ctx);
					break;
		
				  case flushWriteMsg:
					FlushDecompressedBuffers(ctx);
					break;
		
				  case flushReadWriteMsg:
					FlushCompressedBuffers(ctx);
					FlushDecompressedBuffers(ctx);
					break;
	
				  case closeDecoderMsg:
					FlushCompressedBuffers(ctx);
					FlushDecompressedBuffers(ctx);
					ExitDecoderThread(ctx, decoderMsg); /* does not return (calls exit(0)) */
					break;
					
				  default:
					PERR(("Unimplemented message type\n"));
					break;
				}
				ReplyMsg(msgItem, kDSNoErr, decoderMsg, sizeof(*decoderMsg));
			}
			break;
		} 
	}
	
	return status;
}

/**********************************************************************
 * ProcessNewMsgs
 * Handle incoming messages from the subscriber.
 *	
 *	Keep processing messages until there are none left, or until we get
 *	a non-zero status back from processing one of them. 
 **********************************************************************/
 
static Err ProcessNewMsgs(DecoderThreadCtxPtr ctx, Boolean inDecoder)
{
	Item			msgItem;
	Err				status = 0;

	while (status == 0 && (msgItem = GetMsg(ctx->msgPort)) != 0) {
		status = ProcessOneMsg(ctx, inDecoder, msgItem);
	} 

	return status;
}

/**********************************************************************
 * GetCompressedBfr
 * Decoder callback function.  
 *	
 *	We always check for new messages first, because if we have a flush
 *	request or shutdown request queued up, we want to get them 
 *	processed before pumping any more data through the decoder.  If 
 *	there is a flush or shutdown request, we'll get a non-zero status
 *	from ProcessNewMessages(), and we just want to feed that status
 *	back to the decoder, which will return it to the thread's main
 *	loop, where it will get acted on.
 *
 *	If there is no pending flush or shutdown request, we check to see
 *	if we have more compressed data ready to feed to the decoder.  If
 *	not, we wait for more data to come in via a subscriber message.
 **********************************************************************/

static Err GetCompressedBfr(const void *unit, const uint8 **buf, int32 *len,
	uint32 *presentationTime, uint32 *timeIsValidFlag)
{
	Err					status;
	DecoderMsgPtr		decoderMsg;
	DecoderThreadCtxPtr	ctx = (DecoderThreadCtxPtr)unit;

	for (;;) {
		if ((status = ProcessNewMsgs(ctx, TRUE)) != 0) {
			return status;
		}
		if (!IsEmptyList(&ctx->CompressedBfrQueue)) {
			decoderMsg 				 = (DecoderMsgPtr)RemHead(&ctx->CompressedBfrQueue);
			ctx->channel			 = decoderMsg->channel;
			ctx->branchNumber 		 = decoderMsg->branchNumber;
			ctx->curCompressedBufPtr = decoderMsg;
			*len 					 = (int32)decoderMsg->size;
			*buf 					 = (uint8 *)decoderMsg->buffer;
			*presentationTime		 = decoderMsg->presentationTime;
			*timeIsValidFlag		 = decoderMsg->timeIsValidFlag;
			return 0;
		}
		WaitSignal(ctx->msgPortSignal);
	}
}

/**********************************************************************
 * CompressedBfrRead
 * Decoder callback function.  Recycle the completed compressed buffer
 * to the Streamer.
 **********************************************************************/
 
static Err CompressedBfrRead(const void *unit, uint8 *buf)
{
	DecoderThreadCtxPtr	ctx 			= (DecoderThreadCtxPtr)unit;
	DecoderMsgPtr		completedBuf 	= ctx->curCompressedBufPtr;

	if (completedBuf != NULL) {
		/* If buf is null, then it must of have been flushed by the decoder. */
		if ((buf != NULL) && (completedBuf->buffer != (uint32*)buf)) {
			PERR(("The completed input stream buffer is not the current buffer\n"));
		}
		
		/* Reply to the subscriber message */
		ReplyMsg(completedBuf->msgItem, kDSNoErr, completedBuf, sizeof(*completedBuf));
	
		/* Invalidate the curCompressedBfrPtr. */
		ctx->curCompressedBufPtr = NULL;
	}

	return 0;
}

/******************************************************************************
 * MPADecoderThread
 * This thread reads the decoder message port for requests and
 * performs appropriate actions.
 ******************************************************************************/
 
static void MPADecoderThread(int32 unUsed, MPADecoderCreationArgs *creationArgs)
{
	Err					status;
	DecoderThreadCtxPtr	ctx;
	DecoderMsgPtr		decoderMsg;
	Item				pendingMsgItem;
	
	TOUCH(unUsed);    /* avoid a compiler warning */

	/*  Call a subroutine to perform all startup initialization. */

	ctx = InitializeDecoderThread(creationArgs);
	creationArgs = NULL;    /* can't access that memory anymore */
	TOUCH(creationArgs);    /* avoid a compiler warning */
	if (ctx == NULL) {
		goto FAILED;
	}
	
	for (;;) {

		status = ProcessNewMsgs(ctx, FALSE);
		CHECK_NEG("ProcessNewMsgs", status);
		
		if (!IsEmptyList(&ctx->DecompressedBfrQueue)) {
			decoderMsg = (DecoderMsgPtr)RemHead(&ctx->DecompressedBfrQueue);

			status = MPAudioDecode((void *)ctx,
						ctx->decoderCtxPtr,
						&decoderMsg->presentationTime,
						&decoderMsg->timeIsValidFlag,
						decoderMsg->buffer,
						&decoderMsg->header);
			
			if (status == 0) {			
				decoderMsg->channel = ctx->channel;
				decoderMsg->branchNumber = ctx->branchNumber;
				ReplyMsg(decoderMsg->msgItem, status, decoderMsg, sizeof(*decoderMsg));
			} else {
				/* buffer doesn't contain valid data; put it back, we'll re-use it next time */
				AddHead(&ctx->DecompressedBfrQueue, (Node *)decoderMsg);
				if (status < 0) {
					CHECK_NEG("mpAudioDecode", status);
				} else {
					pendingMsgItem = status;
					status = ProcessOneMsg(ctx, FALSE, pendingMsgItem);
					if (status == kDSEndOfFileErr) {
						break;
					} else {
						CHECK_NEG("ProcessOneMsg", status);
					}
				}
			}
		} else {
			WaitSignal(ctx->msgPortSignal);
		}
	}

FAILED:

	ExitDecoderThread(ctx, NULL);
}

/******************************************************************************
 * NewMPADecoder
 * Instantiate a new MPADecoderThread.  This creates the decoder
 * thread and waits for initialization to complete. This returns
 * the new decoder's request message port Item number or a negative
 * error code.
 ******************************************************************************/
 
Item NewMPADecoder(SAudioContextPtr subsCtx, int32 deltaPriority)
{
	Err						status;
	MPADecoderCreationArgs	creationArgs;
	uint32					signalBits;

	TOUCH(subsCtx);	/* no idea why this gets passed, we don't need it. */

	/* Setup the creation args, including a signal to synchronize with th
	 * completion of the decoder's initialization. It will signal us when it
	 * is done initializing itself, successfully or not. */

	status = kDSNoSignalErr;
	creationArgs.creatorTask    = CURRENTTASKITEM;
	creationArgs.creatorSignal  = AllocSignal(0);
	if (creationArgs.creatorSignal == 0) {
		 goto CLEANUP;
	}
	
	creationArgs.creationStatus     = kDSInitErr;
	creationArgs.requestPort		= -1;

	/* Create the thread that will handle all decoder responsibilities */
	status = NewThread((void *)(int32)&MPADecoderThread,
						4096,
						(int32)CURRENT_TASK_PRIORITY + deltaPriority,
						"MPEGAudioDecoder",
						0,
						&creationArgs);

	/* AHEM! status check here? */

	/* Wait here until the decoder is done initializing */
	signalBits = WaitSignal(creationArgs.creatorSignal);
	if (signalBits != creationArgs.creatorSignal) {
		goto CLEANUP;
	}
	
	/* We're done with this signal. So release it. */
	 FreeSignal(creationArgs.creatorSignal);
	creationArgs.creatorSignal = 0;

	/* Check the decoder's creationStatus. */
	status = creationArgs.creationStatus;
	if (status < 0) {
		goto CLEANUP;
	}
	
	return creationArgs.requestPort;        /* success! */

CLEANUP:

	/* Something went wrong.
	 * Release any resources we allocated and return the result status.
	 * The Decoder thread will clean up after itself. */
	if (creationArgs.creatorSignal) {
		FreeSignal(creationArgs.creatorSignal);
	}
	
	return status;

}

