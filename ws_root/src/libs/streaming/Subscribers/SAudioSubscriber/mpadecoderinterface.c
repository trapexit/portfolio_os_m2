/******************************************************************************
**
**  @(#) mpadecoderinterface.c 96/04/12 1.4
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
#include <misc/mpaudiodecode.h>
#endif

#ifndef __STREAMING_THREADHELPER_H
#include <streaming/threadhelper.h>
#endif

#include "sachannel.h"
#include "mpabufferqueue.h"
#include "mpadecoderinterface.h"

typedef struct MPADecoderCreationArgs {
	/* --- input parameters from the client to the new subscriber --- */
	Item			creatorTask;	/* who to signal when done initializing */
	uint32			creatorSignal;	/* signal to send when done initializing */
	SAudioContextPtr subsCtx;
	
	/* --- output results from spawing the new subscriber --- */
	Err				creationStatus;	/* < 0 ==> failure */
	Item			requestPort;	/* new thread's request msgport */
} MPADecoderCreationArgs;

/*****************************
 * Local function prototypes
 *****************************/
 DecoderMsgPtr GetNextDecompressedBfr( ListP theQueue );

/********************************************************
* Main decoder thread and its initalization routine
********************************************************/
static DecoderThreadCtxPtr InitializeDecoderThread( MPADecoderCreationArgs *creationArgs );

static void MPADecoderThread( int32 unUsed, MPADecoderCreationArgs *creationArgs );

/**********************************************************************
 * GetNextDecompressedBfrs
 * Get the next decompressed buffer to store the decoded data.
 * This function is called by DecoderThread() before calling
 * MPAudioDecode() to start decoding the next frame.
 **********************************************************************/
DecoderMsgPtr GetNextDecompressedBfr( ListP theQueue )
{
	/* LIST	theQueue		= ctx->DecompressedBfrQueue; */

	DecoderMsgPtr	decoderMsg;

	/* Note:  the list should never be empty.  Check for empty queue
	 * must be done prior to calling this routine. */

	/* Take the first node on the list */
	decoderMsg = (DecoderMsgPtr)RemHead( theQueue );

	/* Return buffer pointer */
	return ( decoderMsg );

} /* GetNextDecompressedBfr() */

/******************************************************************************
 * NewMPADecoder
 * Instantiate a new MPADecoderThread.  This creates the decoder
 * thread and waits for initialization to complete. This returns
 * the new decoder's request message port Item number or a negative
 * error code.
 ******************************************************************************/
Item NewMPADecoder( SAudioContextPtr subsCtx, int32 deltaPriority )
{

Err						status;
MPADecoderCreationArgs	creationArgs;
uint32					signalBits;

	/* Setup the creation args, including a signal to synchronize with th
	 * completion of the decoder's initialization. It will signal us when it
	 * is done initializing itself, successfully or not. */

	status = kDSNoSignalErr;
	creationArgs.creatorTask    = CURRENTTASKITEM;
	creationArgs.creatorSignal  = AllocSignal(0);
	if ( creationArgs.creatorSignal == 0 )
		 goto CLEANUP;

	creationArgs.subsCtx			= subsCtx;
	creationArgs.creationStatus     = kDSInitErr;
	creationArgs.requestPort		= -1;

	/* Create the thread that will handle all decoder responsibilities */
	status = NewThread( (void *)(int32)&MPADecoderThread,
						4096,
						(int32)CURRENT_TASK_PRIORITY + deltaPriority,
						"MPEGAudioDecoder",
						0,
						&creationArgs );

	/* Wait here until the decoder is done initializing */
	signalBits = WaitSignal( creationArgs.creatorSignal );
	if ( signalBits != creationArgs.creatorSignal )
		goto CLEANUP;

	/* We're done with this signal. So release it. */
	 FreeSignal(creationArgs.creatorSignal);
	creationArgs.creatorSignal = 0;

	/* Check the decoder's creationStatus. */
	status = creationArgs.creationStatus;
	if ( status < 0 )
		goto CLEANUP;

	return creationArgs.requestPort;        /* success! */

CLEANUP:
	/* Something went wrong.
	 * Release any resources we allocated and return the result status.
	 * The Decoder thread will clean up after itself. */
	if ( creationArgs.creatorSignal )   FreeSignal(creationArgs.creatorSignal);
	return status;

} /* NewMPADecoder() */

/******************************************************************************
 * InitializeDecoderThread
 * Do one-time initialization for the MPEG audio decoder thread:
 * allocate the decoder context structure (instance data), allocate
 * system resources, etc.
 * RETURNS: The new context pointer if successful or NULL if failed.
 * SIDE EFFECTS: To communicate with the spawning process, this fills
 * in the creationStatus and requestPort fields of the creationArgs
 * structure and then sends a signal to the spawning process.
 * NOTE: Once we signal the spawning process, the creationArgs structure
 * will go away out from under us.
 ******************************************************************************/
static DecoderThreadCtxPtr InitializeDecoderThread( MPADecoderCreationArgs *creationArgs )
{
Err					status;
MPACallbackFns		CallbackFns;
DecoderThreadCtxPtr	decoderThreadCtx;

	status = kDSNoMemErr;
	/* Allocate the decoder thread context structure. */
	decoderThreadCtx = AllocMem( sizeof(*decoderThreadCtx), MEMTYPE_FILL );
	if( !decoderThreadCtx )
		goto BAILOUT;

	/* Initialize the compressed buffers and decompressed buffers list. */
	PrepList( &decoderThreadCtx->CompressedBfrQueue );
	PrepList( &decoderThreadCtx->DecompressedBfrQueue );

	/* Set callback functions. */
	CallbackFns.CompressedBfrReadFn = (MPACompressedBfrReadFn)CompressedBfrRead;
	CallbackFns.GetCompressedBfrFn = (MPAGetCompressedBfrFn)GetCompressedBfr;

	/* Allocate the decoder context structure. */
	status = CreateMPAudioDecoder( &decoderThreadCtx->decoderCtxPtr, CallbackFns );
	if ( status < 0 )
		goto BAILOUT;
	
	/* Create the message port where this decoder will accept
	 * messages from the subscriber. */
	status = NewMsgPort( &decoderThreadCtx->msgPortSignal );
	if ( status < 0 )
		goto BAILOUT;
	creationArgs->requestPort = decoderThreadCtx->msgPort = status;


BAILOUT:

	creationArgs->creationStatus = status;

 	status = SendSignal(creationArgs->creatorTask, creationArgs->creatorSignal);
 	creationArgs = NULL;    /* can't access this memory after sending the signal */
	TOUCH(creationArgs);    /* avoid a compiler warning */
	if ( status < 0 )
	{
		if ( decoderThreadCtx->decoderCtxPtr )
		{
			status = DeleteMPAudioDecoder( decoderThreadCtx->decoderCtxPtr );
			CHECK_NEG( "DeleteMPAudioDecoder", status );
		}

		FreeMem( decoderThreadCtx, sizeof(*decoderThreadCtx) );
		decoderThreadCtx = NULL;
	}

	return decoderThreadCtx;

} /* initializeDecoderThread() */

/******************************************************************************
 * MPADecoderThread
 * This thread reads the decoder message port for requests and
 * performs appropriate actions.
 ******************************************************************************/
static void MPADecoderThread( int32 unUsed,
	MPADecoderCreationArgs *creationArgs )
{

Err					status;
DecoderThreadCtxPtr	decoderThreadCtx;
uint32				anySignal;
uint32				signalBits;
ListP				/* compressedBfrQueue, */
					decompressedBfrQueue;
DecoderMsgPtr		decoderMsgPtr;

	TOUCH(unUsed);    /* avoid a compiler warning */

	/*  Call a subroutine to perform all startup initialization. */
	decoderThreadCtx = InitializeDecoderThread( creationArgs );
	creationArgs = NULL;    /* can't access that memory anymore */
	TOUCH(creationArgs);    /* avoid a compiler warning */
	if ( decoderThreadCtx == NULL )
		goto FAILED;

/*
	compressedBfrQueue		= &decoderThreadCtx->CompressedBfrQueue;
*/
	decompressedBfrQueue	= &decoderThreadCtx->DecompressedBfrQueue;

	anySignal = decoderThreadCtx->msgPortSignal;

	/* main loop */
	while ( true )
	{
		signalBits = WaitSignal( anySignal );

		/* process arrival of new compressed and decompressed data buffers */
		if( signalBits & decoderThreadCtx->msgPortSignal )
		{
			status = ProcessNewMsgs( decoderThreadCtx );
			if( status == kDSEndOfFileErr )
				break;

		} /* process new data */

		/* process all new data */
		while( !IsEmptyList(decompressedBfrQueue) )
		{
			/* Get next available buffer to hold one decompressed frame */
			decoderMsgPtr = GetNextDecompressedBfr( decompressedBfrQueue);

			status = MPAudioDecode( (void *)decoderThreadCtx,
						decoderThreadCtx->decoderCtxPtr,
						&decoderMsgPtr->presentationTime,
						&decoderMsgPtr->timeIsValidFlag,
						decoderMsgPtr->buffer,
						&decoderMsgPtr->header );
			CHECK_NEG("mpAudioDecode", status );

			/* Set the channel and branch number the decompressed
			 * data belongs to. */
			decoderMsgPtr->channel = decoderThreadCtx->channel;
			decoderMsgPtr->branchNumber = decoderThreadCtx->branchNumber;
			status = DecompressedBfrComplete( decoderMsgPtr, status );
			CHECK_NEG("DecompressedBfrComplete", status );

		} /* while () */

	} /* while( true ) */

FAILED:
	/* Dispose all memory we allocated and clean up any static state.
	 * The OS will automatically reclaim the Items we allocated. */
	if( decoderThreadCtx )
	{
		/* Delete the decoder and its resources. */
		status = DeleteMPAudioDecoder( decoderThreadCtx->decoderCtxPtr );
		CHECK_NEG( "DeleteMPAudioDecoder", status );

		/* The decoder is deleted when the CloseDecoderMsg
		 * is processed in ProcessNewMsgs() */
		FreeMem( decoderThreadCtx, sizeof(*decoderThreadCtx) );
	}

} /* DecoderThread() */

