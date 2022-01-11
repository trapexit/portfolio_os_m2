/******************************************************************************
**
**  @(#) mpabufferqueue.c 96/05/20 1.7
**
******************************************************************************/

#include <string.h>

#ifndef __KERNEL_MSGPORT_H
#include <kernel/msgport.h>
#endif

#ifndef __KERNEL_LIST_H
#include <kernel/list.h>
#endif

#ifndef __KERNEL_TASK_H
#include <kernel/task.h>
#endif

#ifndef __KERNEL_DEBUG_H
#include <kernel/debug.h>	/* for CHECK_NEG, PERR macro */
#endif

#ifndef __MISC_MPAUDIODECODE_H
#include <misc/mpaudiodecode.h>
#endif

#ifndef __STREAMING_SUBSCRIBERUTILITIES_H
#include <streaming/subscriberutils.h>
#endif

#ifndef __STREAMING_THREADHELPER_H
#include <streaming/threadhelper.h>
#endif

#include "mpabufferqueue.h"
/* #include "mpegaudiotypes.h" */
#include "mpadecoderinterface.h"

/*****************************************************************************
 * Compile switch implementations
 *****************************************************************************/

#if SAUDIO_TRACE_MPABUFFERQUEUE

	#define TRACE_LEVEL		2
	
	/* Find the actual trace buffer. It's declared in SAMain.c. */
	extern	TraceBufferPtr	SATraceBufPtr;
	
	/* Allow for multiple levels of tracing */
	#if (TRACE_LEVEL >= 1)
		#define		ADD_TRACE_L1( bufPtr, event, chan, value, ptr )	\
						AddTrace( bufPtr, event, chan, value, ptr )
	#else
		#define		ADD_TRACE_L1( bufPtr, event, chan, value, ptr )
	#endif
	
	#if (TRACE_LEVEL >= 2)
		#define		ADD_TRACE_L2( bufPtr, event, chan, value, ptr )	\
						AddTrace( bufPtr, event, chan, value, ptr )
	#else
		#define		ADD_TRACE_L2( bufPtr, event, chan, value, ptr )
	#endif
	
	#if (TRACE_LEVEL >= 3)
		#define		ADD_TRACE_L3( bufPtr, event, chan, value, ptr )	\
						AddTrace( bufPtr, event, chan, value, ptr )
	#else
		#define		ADD_TRACE_L3( bufPtr, event, chan, value, ptr )
	#endif
	
	
#else  /* Trace is off */
	#define		ADD_TRACE_L1( bufPtr, event, chan, value, ptr )
	#define		ADD_TRACE_L2( bufPtr, event, chan, value, ptr )
	#define		ADD_TRACE_L3( bufPtr, event, chan, value, ptr )
#endif /* SAUDIO_TRACE_MPABUFFERQUEUE */

 /*****************************
  * Local function prototypes
  *****************************/
static Err FlushCompressedBuffers( DecoderThreadCtxPtr ctx );
static Err FlushCompressedBfrQueue( ListP InputQueue );
static Err FlushDecompressedBfrQueue( ListP OutputQueue );

/**********************************************************************
 * GetCompressedBfr
 * Decoder callback function.  Dequeue a compressed buffer.
 **********************************************************************/
Err GetCompressedBfr( const void *ctx, const uint8 **buf, int32 *len,
	uint32 *presentationTime, uint32 *timeIsValidFlag )
{

	uint32	msgPortSignal = ((DecoderThreadCtxPtr)ctx)->msgPortSignal;

	ListP	theQueue = &((DecoderThreadCtxPtr)ctx)->CompressedBfrQueue;
	uint32	*channel =  &((DecoderThreadCtxPtr)ctx)->channel;

	DecoderMsgPtr	decoderMsg;
	Err				status = kDSNoErr;

	/* Set length to zero */
	*len	= 0;
	*buf	= NULL;

	/* Set curCompressedBufPtr to NULL */
	((DecoderThreadCtxPtr)ctx)->curCompressedBufPtr = NULL;

	/* Check if list is empty */
	while( IsEmptyList( theQueue ) )
	{
		/* Wait here for new data */
		WaitSignal( msgPortSignal );
		status = ProcessNewMsgs( (DecoderThreadCtxPtr)ctx );
		if( status == kDSEndOfFileErr )
			return status;
		CHECK_NEG( "ProcessNewMsgs", status );
	}

	/* Take the first node on the list */
	decoderMsg = (DecoderMsgPtr)RemHead( theQueue );

	/* Remember the channel and branchnumber */
	*channel = decoderMsg->channel;
	((DecoderThreadCtxPtr)ctx)->branchNumber = decoderMsg->branchNumber;

	/* Remember this buffer. */
	((DecoderThreadCtxPtr)ctx)->curCompressedBufPtr = decoderMsg;

	/* Set length */
	*len = (int32)decoderMsg->size;

	/* Set buff */
	*buf = (uint8 *)decoderMsg->buffer;

	/* Set presentation time and timeIsValidFlag */
	*presentationTime	= decoderMsg->presentationTime;
	*timeIsValidFlag	= decoderMsg->timeIsValidFlag;

	return status;

} /* GetCompressedBfr() */

/**********************************************************************
 * CompressedBfrRead
 * Decoder callback function.  Recycle the completed compressed buffer
 * to the Streamer.
 **********************************************************************/
Err CompressedBfrRead( const void *ctx, uint8 *buf )
{
	DecoderMsgPtr	completedBuf =
		((DecoderThreadCtxPtr)ctx)->curCompressedBufPtr;
	Err				status = kDSNoErr;

	TOUCH(buf);

	if( completedBuf != NULL )
	{
		/* If buf is null, then it must of have been flushed by the
		 * decoder. */
		if( (buf != NULL) && (completedBuf->buffer != (uint32*)buf) )
			PERR(("The completed input stream buffer is not the current buffer\n"));

		/* Reply to the subscriber message */
		status = ReplyMsg( completedBuf->msgItem, kDSNoErr,
			completedBuf, sizeof(*completedBuf) );
	
		/* Invalidate the curCompressedBfrPtr. */
		((DecoderThreadCtxPtr)ctx)->curCompressedBufPtr = NULL;
	}

	return status;

} /* CompressedBfrRead() */


/**********************************************************************
 * DecompressedBfrComplete
 * This function is called by the MPADecoderThread() when the decoder is
 * done with the decompressed buffer, to send it back to the subscriber.
 **********************************************************************/
Err DecompressedBfrComplete( DecoderMsgPtr msgPtr, Err status )
{
	/* Hand this decoded buffer back to the subscriber. */
	return( ReplyMsg( msgPtr->msgItem, status,
			 msgPtr, sizeof(*msgPtr) ) );
} /* DecompressedBfrComplete */

/**********************************************************************
 * FlushCompressedBuffers
 * Flush out the compressed buffer queue and the decoder.
 **********************************************************************/
static Err FlushCompressedBuffers( DecoderThreadCtxPtr ctx )
{
	ListP	InputQueue	= &ctx->CompressedBfrQueue;
	Err		status;

	status = FlushCompressedBfrQueue( InputQueue );
	CHECK_NEG( "FlushCompressedBfrQueue", status );

	/* Flush the decoder. The decoder will call the
	 * CompressedBfrRead() which will reply to the
	 * subscriber the curCompressedBufPtr. */
	status = MPAFlush( ctx->decoderCtxPtr );
	CHECK_NEG( "MPAFlush", status );

	return status;

} /* FlushCompressedBuffers() */

/**********************************************************************
 * FlushCompressedBfrQueue
 * Flush out the compressed buffer queue.
 **********************************************************************/
static Err FlushCompressedBfrQueue( ListP InputQueue )
{
	Err					status = 0;
	DecoderMsgPtr		decoderMsg;

	while( !IsEmptyList( InputQueue ) )
	{
		decoderMsg = (DecoderMsgPtr)RemHead( InputQueue );

		/* Reply to the subscriber message */
		status = ReplyMsg( decoderMsg->msgItem, kDSNoErr,
			decoderMsg, sizeof(*decoderMsg) );
	} /* while () */

	return( status );

} /* FlushCompressedBfrQueue() */

/**********************************************************************
 * FlushDecompressedBfrQueue
 * Flush out the decompressed buffer queue.
 **********************************************************************/
static Err FlushDecompressedBfrQueue( ListP OutputQueue )
{
	Err				status = 0;
	DecoderMsgPtr	decoderMsg;
	
	while( !IsEmptyList( OutputQueue ) )
	{
		decoderMsg = (DecoderMsgPtr)RemHead( OutputQueue );

		/* Hand this buffer back to the subscriber. */
		status = ReplyMsg( decoderMsg->msgItem, kDSWasFlushedErr,
 			(void *)decoderMsg, sizeof(*decoderMsg) );
		CHECK_NEG( "ReplyMsg", status );
	}

	return( status );

} /* FlushDecompressedBfrQueue() */

/**********************************************************************
 * ProcessNewMsgs
 * Handle incoming messages from the subscriber.
 **********************************************************************/
Err ProcessNewMsgs( DecoderThreadCtxPtr ctx )
{

	Item	msgItem;
	Message	*msgPtr; 
	Err		status = kDSNoErr;
	ListP	InputQueue	= &ctx->CompressedBfrQueue;
	ListP	OutputQueue	= &ctx->DecompressedBfrQueue;

	DecoderMsgPtr	decoderMsgPtr;

	while ( msgItem = GetMsg( ctx->msgPort ) )
	{
		msgPtr = MESSAGE( msgItem );
		decoderMsgPtr = msgPtr->msg_DataPtr;

		/* check the flush command to see if we need
		 * to flush the new data and write (decoded) buffers. */
		switch( decoderMsgPtr->messageType )
		{
			case compressedBfrMsg:
				/* Add the new data buffer to the list */
				AddTail( InputQueue, (Node *)decoderMsgPtr );
				break;

			case decompressedBfrMsg:
				/* Add the next buffer to store the decompressed
				 * data to the queue */
				AddTail( OutputQueue, (Node *)decoderMsgPtr );
				break;

			case flushReadMsg:
				status = FlushCompressedBuffers( ctx );
				CHECK_NEG( "FlushCompressedBuffers", status );

				break;

			case flushWriteMsg:
				status = FlushDecompressedBfrQueue( OutputQueue );
				CHECK_NEG( "FlushDecompressedBfrQueue", status );
				break;

			case flushReadWriteMsg:
				/* Flush the compressed buffer queue and the decoder. */
				status = FlushCompressedBuffers( ctx );
				CHECK_NEG( "FlushCompressedBuffers", status );
				status = FlushDecompressedBfrQueue( OutputQueue );
				CHECK_NEG( "FlushDecompressedBfrQueue", status );

				break;

			case closeDecoderMsg:
				/* Flush both the compressed and decompressed buffer
				 * queue. */
				status = FlushCompressedBuffers( ctx );
				CHECK_NEG( "FlushCompressedBuffers", status );

				/* Flush the decompressed buffer queue. */
				status = FlushDecompressedBfrQueue( OutputQueue );
				CHECK_NEG( "FlushDecompressedBfrQueue", status );

				/* Reply to this message */
				status = ReplyMsg( msgItem, kDSNoErr,
					decoderMsgPtr, sizeof(*decoderMsgPtr) );
				CHECK_NEG( "ReplyMsg", status );

				return kDSEndOfFileErr;

			default:
				PERR(( "Unimplemented message type\n" ));
				break;

		} /* switch( decoderMsgPtr->messageType ) */

		/* Reply to flush messages. compressedBfrMsg and
		 * decompressedBfrMsg will get replied to when the decoder is
		 * done processing the buffers. */
		if( ( decoderMsgPtr->messageType != compressedBfrMsg ) &&
			( decoderMsgPtr->messageType != decompressedBfrMsg ) )
		{
			status = ReplyMsg( msgItem, kDSNoErr,
				decoderMsgPtr, sizeof(*decoderMsgPtr) );
			CHECK_NEG( "ReplyMsg", status );
		}

	} /* while () */

	return status;
} /* ProcessNewMsgs() */
