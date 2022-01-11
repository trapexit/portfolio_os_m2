/******************************************************************************
**
**  @(#) datastream.c 96/11/26 1.13
**  11/09/96  Ian	Added kDSOpDoHighResStills to allow the client to control
**					the parser in terms of whether it's the normal-res or high-res
**					stills that get parsed and delivered to the video subscriber.
**					The client must change this only when the stream is stopped! 
**					Also, when we get a new video header at the start of a stream
**					and send the header to the video subscriber via kStreamOpHeaderInfo,
**					we now send the true vertical size in the maxVideoArea field,
**					instead of sending 240 (it was hard-coded!).
**	08/19/96  Ian	In DeliverDataChunks(), after sending an audio header to the
**					audio subscriber, we now set seeking_usedinfo so that we 
**					don't keep sending the header info repeatedly.
**	08/16/96  Ian	In DoShuttle(), we now only call ReShuttle() to do the 
**					first jump if the shuttling rate is for backwards movement.
**					When the shuttle is for forward movement, there's already
**					a ton of usable data in the already-filled buffers, and
**					doing a reshuttle throws that data away and then jumps
**					even further ahead, making the first shuttling step about
**					twice the size of a normal shuttling step, so it's better
**					to not jump the DataAcq ahead (via ReShuttle) until the
**					first jump is accomplished with the data already buffered.
**					When the shuttle request is for backwards movement, we go
**					ahead and punt all the filled buffers and ask the DataAcq
**					to jump backwards right away.
**	08/16/96  Ian	Changed the logic in DoPreroll to properly honor the 
**					dontFillBuffersCount (async preroll) in the request.
**					The old logic was setting the privatePtr (sync DataAcq
**					request) field in the last few buffers it sent to the 
**					DataAcq, meaning essentially all the buffers had to come
**					back before the request was replied to the client.  (IE,
**					it was always a synchronous operation.)  Now we set the
**					privatePtr field in the first few buffers (as many as 
**					needed to honor the dontFillBufferCount value), and leave
**					the privatePtr field NULL in the last few buffers sent,
**					so that we can reply to the client as soon as the first
**					few buffers have filled, and leave the rest of the buffers
**					filling asynchonously.
**					HOWEVER:  a race condition was occurring due to logic in
**					the DataAcq.  When a stream was stop-flushed, or GoMarker'd,
**					or a DataAcq was disconnect/reconnected, the logic used in
**					the DataAcq to flush pending I/O was not fast enough.  The
**					DataAcq would reply the GoMarker/disconnect/reconnect 
**					request after calling AbortIO on all the buffers, but 
**					before returning the aborted buffers back to the streamer.
**					So, it could happen that we'd get, say, a stop-flush, a
**					GoMarker, and a semi-sync preroll, and the preroll would
**					effectively do nothing, because there weren't enough (or
**					even any) of the aborted buffers back from the DataAcq
**					yet.  Then, when the stream was started, it would stutter-
**					start due to buffer starvation.  So, a fix was also applied
**					do the DataAcq to AbortIO/WaitIO/ReplyMsg each outstanding
**					buffer BEFORE replying to the request that implied a flush.
**					That way, all aborted buffers were free and ready for a 
**					preroll before the next request from the client was processed.
**					(Man, what a bitch it is tracking this stuff down!)
**	08/15/96  Ian	Removed the backwards-shuttle accellerator logic from the
**					Reshuttle() routine.  If the app wants to accellerate 
**					shuttling, it can do so by making a series of DSShuttle()
**					calls, changing the rate each time.
******************************************************************************/

#include <stdlib.h>
#include <string.h>				/* for memset, memcpy */
#include <kernel/types.h>
#include <kernel/operror.h>
#include <audio/audio.h>
#include <kernel/debug.h>		/* for PERR, CHECK_NEG */
#include <kernel/mem.h>
#include <kernel/semaphore.h>

#include <video_cd_streaming/datastream.h>
#include <video_cd_streaming/datastreamlib.h>
#include <streaming/threadhelper.h>
#include <video_cd_streaming/msgutils.h>
#include <video_cd_streaming/mpegutils.h>

#include "wbparse.h"

/************************************
 * Local types and constants
 ************************************/

/* This flag enables the code that allows us to stop delivering a buffer's
 * chunks mid-way (e.g. at a STRM STOP or STRM GOTO chunk), put the partially-
 * delivered buffer back on the head of the filled buffer queue, and later (on
 * resume or failed GOTO) deliver the rest of its chunks. If this feature is
 * disabled, we'll ASSUME that every STRM STOP and STRM GOTO chunk is followed
 * by a FILL chunk out to the rest of that stream block. I.e. we won't deliver
 * any following chunks in that block on resuming delivery. */
#define SUPPORT_PARTIAL_STREAM_BLOCK_DELIVERY		1

/* The streamer keeps an array of these to remember the current subscribers. */
typedef struct DSSubscriber {
	DSDataType	dataType;			/* subscriber data type */
	Item		subscriberPort;		/* subscriber's input message port */
	} DSSubscriber, *DSSubscriberPtr;

/* The maximum number of subscribers allowed for a stream. */
#define	DS_MAX_SUBSCRIBERS	16

/* ProcessSTRMChunk() and DeliverDataChunks() return this value to request
 * ending the current data delivery cycle early. */
#define END_THIS_DELIVERY_CYCLE			1

#define SHUTTLE_FORWARD					1
#define SHUTTLE_BACKWARD				2

struct DSStreamCB {
	Item	clockSemaphore;			/* protects the presentation clock fields */
	uint32	branchNumber;			/* stream presentation clock branch number */
	uint32	clockOffset;			/* stream presentation clock relative to audio clock */
	uint32	stoppedBranchNumber;	/* presentation clock branchNumber when stream stopped */
	uint32	stoppedClock;			/* presentation clock value when stream stopped */
	bool	fRunning;				/* is the stream (and the clock) running?
		* We need to semaphore-lock changes to fRunning and all accesses to
		* the other clock fields. We needn't lock reads of fRunning from the
		* DataStream thread. */

	bool	fSOF;					/* hit start-of-file from data acq? */
	bool	fEOF;					/* hit end-of-file from data acq? */
	bool	fSTOP;					/* processing a STOP chunk? */
	bool	fBranchingAcq;			/* branching the DataAcq? If and when it
									 * succeeds, discard (stale) buffers */
	uint32	fTriggerPTS;			/* time of PTS value */
	bool	fShuttling;				/* shuttling forward/reverse? If true we are
									 * in shuttle mode 1: forward 2:backward*/
	uint32	fShuttlePosition;		/* original position of the shuttle command */
	int32	fShuttleRate;			/* shuttle-rate: positive for fastforward, and
									 * negative for fastreverse */
	int32	fShuttleRelativeRate;	/* relative rate with respect to original position */
	uint32	fFramesDisplayed;		/* total number of frames displayed */

	bool	fEOFDuringBranch;		/* input EOF arrived while fBranchingAcq? */
	bool	fQuitThread;			/* ready to quit the thread? */
	bool	fAborted;				/* aborted the stream? don't keep doing it */

	uint32	deliveryBranchNumber;	/* branch number of data going to subscribers */

	bool	fClkSetPending;			/* is a clock-set pending? (after go-marker or DSConnect) */
	bool	fClkSetNext;			/* set the clock on next data delivery (if we know clkSetTime)? */
	bool	fClkSetNeedDestTime;	/* need to discover clkSetTime? */
	uint32	clkSetTime;				/* new clock value if fClkSetPending && !fClkSetNeedDestTime */
	uint32	fLatestPTS;				/* this hold the latest PTS value we found */

	Item	acquirePort;			/* acquisition module message port */
	uint32	bufDataSize;			/* size of each data buffers in bytes */
	DSDataBufPtr freeBufHead;		/* pointer to list of free data buffers */

	Item	requestPort;			/* work request message port (read only) */
	uint32	requestPortSignal;		/* signal for request port */

	Item	acqReplyPort;			/* msg reply port for data acquisition */
	uint32	acqReplyPortSignal; 	/* signal for data acquisition reply port */
	MemPoolPtr dataMsgPool;			/* pool of data acq message blocks */

	Item	subsReplyPort;			/* msg reply port for subscribers */
	uint32	subsReplyPortSignal;	/* signal for subscriber reply port */
	MemPoolPtr subsMsgPool;			/* pool of subscriber message blocks */

	DSDataBufPtr filledBufHead; 	/* ptr to queue of filled data buffers */
	DSDataBufPtr filledBufTail;		/* ptr to end of queue of filled buffers */
	int32	filledBufCnt;			/* number of filled buffers in the queue */

	int32	totalBufferCount; 		/* total number of buffers this stream owns */
	int32	currentFreeBufferCount;	/* number of buffers currently available
					 				 * for filling */

	DSRequestMsgPtr	endOfStreamMsg;	/* reply to this msg at end of stream */
	DSRequestMsgPtr	startOfStreamMsg;/* reply to this msg at start of stream */
	DSRequestMsgPtr	triggerBitMsg;	/* reply to this meg at trigger-bit */

	DSRequestMsgPtr	requestMsgHead;	/* pointer to 1st queued request msg */
	DSRequestMsgPtr	requestMsgTail;	/* pointer to last queued request msg */
	Err		replyResult;			/* most significant result code from replies */
	int32	repliesPending;			/* # of replies needed to complete the
									 * current request */
	uint32	numSubscribers;			/* number of subscribers in the table */
	DSSubscriber subscriber[DS_MAX_SUBSCRIBERS];

	int32	stillImageType;			/* the type (hi/lo res) of still images we're doing */

	uint32	magicCookie;			/* to help validate DSStreamCBPtr accesses */
	};

#define DS_MAGIC_COOKIE		0x12345678


/* This structure is used temporarily for communication between the spawning
 * (client) process and the nascent streamer thread.
 *
 * Thread-interlock is handled as follows: NewDataStream() allocates
 * this struct on the stack, fills it in, and passes it's address as an arg to
 * the streamer thread. The streamer then owns access to it until sending a
 * signal back to the spawning thread (using the first 2 args in the struct).
 * Before sending this signal, the streamer fills in the "output" fields of
 * the struct, thus returning its creation status result code and request msg
 * port Item. After sending this signal, the streamer may no longer touch this
 * memory as NewDataStream() will deallocate it. */
typedef struct StreamerCreationArgs {
	/* --- input parameters from the client to the new streamer thread --- */
	Item				creatorTask;		/* who to signal when done initializing */
	uint32				creatorSignal;		/* signal to send when done initializing */
	uint32				bufDataSize;		/* size of each data buffers in bytes */
	DSDataBufPtr 		freeBufHead;		/* ptr to list of data buffers */
	int32				numSubsMsgs;		/* # subscriber messages to allocate */

	/* --- output results from spawing the new streamer thread --- */
	Err					creationStatus;		/* < 0 ==> failure */
	Item				requestMsgPort;		/* streamer's request msg port */
	DSStreamCBPtr		streamCBPtr;		/* stream context ptr */
} StreamerCreationArgs;


/* --- Macros --- */
#define	NOT_QUADBYTE_ALIGNED(x)			((uint32)(x) & 0x3)

#define ROUND_TO_LONG(s)				((uint32)(s) + 0x3 & ~0x3)

/* These macros encapsulate allocation from our MemPools, esp. the typecasts. */
#define ALLOC_ACQ_MSG_POOL_MEM(streamCBPtr)	\
		((DataAcqMsgPtr)AllocPoolMem((streamCBPtr)->dataMsgPool))

#define ALLOC_SUB_MSG_POOL_MEM(streamCBPtr)	\
		((SubscriberMsgPtr)AllocPoolMem((streamCBPtr)->subsMsgPool))

/* Buffer counts track where the buffers are for debugging purposes, e.g. to
 * debug a situation where buffers get stuck somewhere. The buffer count
 * values are all in one static struct so the debugger can easily view them.
 * They get adjusted whenever buffers come and go to DataAcq and subscribers.
 * ASSUMES: There's only one Streamer context DSStreamCB at a time. */
#if !defined(BUFFER_COUNTING) && defined(DEBUG)
	#define BUFFER_COUNTING		1
#endif
#if BUFFER_COUNTING
static struct BufferCounts { /* (FYI: these counts are in pipeline order) */
	int32 atDataAcq;		/* # buffers outstanding to Data Acq */
	int32 atStreamer;		/* # buffers in the streamer's filled & free lists */
	int32 atSubscribers;	/* # buffers outstanding to subscribers */
	int32 sum;				/* sum; should == streamCBPtr->totalBufferCount */
	} bufferCounts;

	/* Macro to recalculate bufferCounts.sum.
	 * NOTE: Be careful to call this ***after***
	 * streamCBPtr->currentFreeBufferCount, streamCBPtr->filledBufCnt, and
	 * bufferCounts.atXxx have all been updated. */
	#define SUM_BUFFER_COUNTS(streamCBPtr)	\
		{ bufferCounts.atStreamer = streamCBPtr->currentFreeBufferCount + \
		streamCBPtr->filledBufCnt;	\
		bufferCounts.sum = bufferCounts.atStreamer + bufferCounts.atDataAcq + \
		bufferCounts.atSubscribers; }
#else
	#define SUM_BUFFER_COUNTS(streamCBPtr)	{}
#endif	/* #if BUFFER_COUNTING */


/* --- Data Stream thead local utility routines --- */

static DSSubscriberPtr LookupSubscriber(DSStreamCBPtr streamCBPtr,
			DSDataType dataType);
static Err SendSubscrMsg(DSStreamCBPtr streamCBPtr,
			const SubscriberMsg *srcSubMsg, Item subscriberPort);
static Err SendSubscrMsgByType(DSStreamCBPtr streamCBPtr,
			const SubscriberMsg *srcSubMsg, DSDataType dataType);
static Err ForEachSubscriber(DSStreamCBPtr streamCBPtr,
			SubscriberMsgPtr srcSubMsg);
static Err SendAcqMsg(DSStreamCBPtr streamCBPtr, const DataAcqMsg *srcAcqMsg);

static void ReplyToEndOfStreamMsg(DSStreamCBPtr streamCBPtr, Err status);
static void AbortStream(DSStreamCBPtr streamCBPtr);
static void TearDownStreamCB(DSStreamCBPtr streamCBPtr);

/* --- DSDataBuf utility routines --- */

static void FreeDataBuf(DSStreamCBPtr streamCBPtr, DSDataBufPtr bp);
static DSDataBufPtr GetFreeDataBuf(DSStreamCBPtr streamCBPtr);
static DSDataBufPtr GetFilledDataBuf(DSStreamCBPtr streamCBPtr);
static Err FillDataBuf(DSStreamCBPtr streamCBPtr, DSDataBufPtr bp);
static void FlushAllFilledBuffers(DSStreamCBPtr streamCBPtr);
static Err ReleaseChunk(DSStreamCBPtr streamCBPtr,
				int32 subscriberStatus, DSDataBufPtr bp);

/* --- Data Stream thread procedures to handle request messages --- */

static Err DoGetChannel(DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg);
static Err DoSetChannel(DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg);
static Err DoControl(DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg);
static Err DoCloseStream(DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg);
static Err DoSubscribe(DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg);
static Err DoPreRollStream(DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg);

static Err DoStartStream(DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg);
static Err DoStopStream(DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg);
static Err DoGoMarker(DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg);
static Err DoShuttle(DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg);
static Err ReShuttle(DSStreamCBPtr steramCBPtr);
static Err DoConnect(DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg);
static Err DoRegisterEndOfStream(DSStreamCBPtr streamCBPtr,
					DSRequestMsgPtr reqMsg);

/* --- Data Stream thread high-level utilities --- */

static Err	InternalStopStream(DSStreamCBPtr streamCBPtr,
		DSRequestMsgPtr reqMsg, uint32 options);

static void	HandleSubscriberReply(DSStreamCBPtr streamCBPtr,
				SubscriberMsgPtr subMsg, Message *messagePtr);

static void	HandleDataAcqReply(DSStreamCBPtr streamCBPtr,
				DataAcqMsgPtr acqMsg, Message *messagePtr);

static Err DeliverData(DSStreamCBPtr streamCBPtr);
static Err DeliverDataChunks(DSStreamCBPtr streamCBPtr, DSDataBufPtr bp);
static void HandleDSRequest(DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg);

static DSStreamCBPtr InitializeDSThread(StreamerCreationArgs *creationArgs);
static void	 DataStreamThread(int32 notUsed, StreamerCreationArgs *creationArgs);


/*============================================================================
  ============================================================================
			Miscellaneous Utility Routines
			used by the Data Streamer thread
  ============================================================================
  ============================================================================*/


/******************************************************************************
 * This utility routine calls ReplyMsg() to return a DSRequestMsg to the thread
 * that sent it (which generally is the client application).
|******************************************************************************/
static Err ReplyToReqMsg(DSRequestMsgPtr reqMsg, Err status)
	{
	Err		result;

	result = ReplyMsg(reqMsg->msgItem, status, reqMsg, sizeof(DSRequestMsg));
	CHECK_NEG("ReplyToReqMsg", result);
	return result;
	}


/****************************************************************************
 * NOTE: Some client request messages are synchronous, that is, the streamer
 * must finish processing it before it starts processing the next request.
 *
 * In order to process one of these at a time (the request at the head of the
 * request queue, streamCBPtr->requestMsgHead), streamCBPtr->repliesPending
 * counts the number of sub-requests sent out to the data acq and subscribers
 * that must come back to complete processing of this streamer request.
 *
 * Cases:
 *
 * (1) Sub-messages in synchronous service of client request messages:
 *     msg->whatToDo != kStreamOpData (if a subscriber msg).
 *     msg->privatePtr == pointer to the client request message.
 *
 * (2) Data flow messages to subscribers (these are not in direct service of a
 *       specific client request message):
 *     msg->whatToDo == kStreamOpData.
 *     msg->privatePtr == the buffer pointer [WARNING: reuse of this field].
 *
 * (3) Messages for asynchronous requests and other messages not in synchronous
 *       service of a client request message:
 *     msg->privatePtr == NULL.
 *
 * The procedures SendSubscrMsg() and SendAcqMsg() automatically increment
 * streamCBPtr->repliesPending in case (1).
 *****************************************************************************/


/*******************************************************************************
 * Return the subscriber descriptor for a given data type, or NULL if there is
 * no current subscriber for that data type.
 *******************************************************************************/
static DSSubscriberPtr LookupSubscriber(DSStreamCBPtr streamCBPtr,
		DSDataType dataType)
	{
	uint32			index;
	DSSubscriberPtr	sp = &streamCBPtr->subscriber[0];

	for ( index = streamCBPtr->numSubscribers; index > 0; --index )
		{
		if ( sp->dataType == dataType )
			return sp;
		++sp;
		}

	return NULL;
	}


/*******************************************************************************
 * Send a copy of the given message struct to a subscriber and, if appropriate
 * (case (1), above) increment repliesPending. This procedure allocates a
 * subscriber message from the pool, copies in the fields from *srcSubMsg
 * (except the msgItem), and sends that to the subscriber. It handles error
 * cases, e.g. empty pool and SendMsg() errors.
 *
 * The message receiver does not have to be a current stream subscriber. E.g.
 * we can send an "abort" message to a subscriber-wannabe.
 *
 * SIDE EFFECT: Allocates a message from the subscriber-message pool.
 *
 * SIDE EFFECT: Increments streamCBPtr->repliesPending if we successfully sent
 *    out a sub-message in service of a synchronous streamer request (case (1)).
 *
 * IMPLEMENTATION NOTE: This is a commonly called subroutine so it makes sense
 * to keep the error-handling cases in the "else" clauses because the PowerPC
 * predicts that forward branches won't be taken. But the source code would read
 * shorter the other way around.
 *******************************************************************************/
static Err SendSubscrMsg(DSStreamCBPtr streamCBPtr,
		const SubscriberMsg *srcSubMsg, Item subscriberPort)
	{
	Err					status;
	Item				msgItem;
	SubscriberMsgPtr	copySubMsg;

	copySubMsg = ALLOC_SUB_MSG_POOL_MEM(streamCBPtr);
	if ( copySubMsg != NULL )
		{
		/* Copy the source message to the one we just allocated from the pool.
		 * But preserve the msgItem in the allocated message! */
		msgItem = copySubMsg->msgItem;
		*copySubMsg = *srcSubMsg;
		copySubMsg->msgItem = msgItem;

		status = SendMsg(subscriberPort, msgItem, copySubMsg,
			sizeof(SubscriberMsg));
		if ( status >= 0 )
			{
			if ( copySubMsg->privatePtr != NULL
					&& copySubMsg->whatToDo != kStreamOpData )
				streamCBPtr->repliesPending++;
			}
		else
			{
			ERROR_RESULT_STATUS("SendSubscrMsg SendMsg", status);
#ifdef DEBUG
			if ( LookupItem(subscriberPort) == NULL )
				PERR(("  Bad subscriberPort Item\n"));
			if ( LookupItem(msgItem) == NULL )
				PERR(("  Bad msgItem Item\n"));
#endif

			/* Partial error recovery: return the message node to its pool */
			ReturnPoolMem(streamCBPtr->subsMsgPool, copySubMsg);
			}
		}
	else
		status = kDSNoMsgErr;

	return status;
	}


/*******************************************************************************
 * Like SendSubscrMsg(), but also looks up the receiver subscriber by data type.
 * If there is no subscriber of this data type, this procedure returns
 * kDSSubNotFoundErr but doesn't print any debugging error messages or anything.
 * So the caller can handle that any way it wants to.
 *******************************************************************************/
static Err	SendSubscrMsgByType(DSStreamCBPtr streamCBPtr,
		const SubscriberMsg *srcSubMsg, DSDataType dataType)
	{
	DSSubscriberPtr		sp;

	/* Find the subscriber descriptor given the data type */
	sp = LookupSubscriber(streamCBPtr, dataType);
	if ( sp == NULL ) {
		return kDSSubNotFoundErr;	/* NOTE: The caller may not consider this an error */
	}

	/* Send it the msg. */
	return SendSubscrMsg(streamCBPtr, srcSubMsg, sp->subscriberPort);
}


/*******************************************************************************
 * Send a copy of the same message each current subscriber. If an error occurs,
 * some subscribers may not receive the message.
 *
 * SIDE EFFECT: Increments streamCBPtr->repliesPending for each message
 * successfully sent, if appropriate. See the rules, above.
 *******************************************************************************/
static Err ForEachSubscriber(DSStreamCBPtr streamCBPtr,
		SubscriberMsgPtr srcSubMsg)
	{
	Err					status = kDSNoErr;
	uint32				index;
	DSSubscriberPtr		sp;

	/* If there aren't enough messages in the pool to send one to every
	 * subscriber, then give up now. */
	if ( streamCBPtr->subsMsgPool->numFreeInPool < streamCBPtr->numSubscribers )
		{
		PERR(("ForEachSubscriber: not enough subscriber messages\n"));
		return kDSNoMsgErr;
		}

	/* Send each subscriber a copy of the specified message.
	 * [TBD] Bail out at the first SendSubscrMsg() error? */
	sp = &streamCBPtr->subscriber[0];
	for ( index = streamCBPtr->numSubscribers; index > 0; --index )
		{
		status = SendSubscrMsg(streamCBPtr, srcSubMsg, sp->subscriberPort);
		++sp;
		}

	return status;
	}


/*******************************************************************************
 * Send a copy of the given message struct to the current data acq and, if
 * appropriate (case (1), above), increment repliesPending. This procedure
 * allocates a data acq message from the pool, copies in the fields from
 * *srcAcqMsg (except the msgItem), fills in the message's acquirePort field,
 * and sends that to the data acq. It handles error cases, e.g. empty pool and
 * SendMsg() errors.
 *
 * SIDE EFFECT: Allocates a message from the data-acq-message pool.
 *
 * SIDE EFFECT: Increments streamCBPtr->repliesPending if we successfully sent
 *    out a sub-message in service of a synchronous streamer request (case (1)).
 *******************************************************************************/
static Err SendAcqMsg(DSStreamCBPtr streamCBPtr, const DataAcqMsg *srcAcqMsg)
	{
	Err					status;
	Item				msgItem;
	DataAcqMsgPtr		copyAcqMsg;

	copyAcqMsg = ALLOC_ACQ_MSG_POOL_MEM(streamCBPtr);
	if ( copyAcqMsg != NULL )
		{
		/* Copy the source message to the one we just allocated from the pool.
		 * But preserve the msgItem in the allocated message! */
		msgItem = copyAcqMsg->msgItem;
		*copyAcqMsg = *srcAcqMsg;
		copyAcqMsg->msgItem = msgItem;

		/* Fill in the message's dataAcqPort field */
		copyAcqMsg->dataAcqPort = streamCBPtr->acquirePort;

		status = SendMsg(streamCBPtr->acquirePort, msgItem, copyAcqMsg,
			sizeof(DataAcqMsg));
		if ( status >= 0 )
			{
			if ( copyAcqMsg->privatePtr != NULL )
				streamCBPtr->repliesPending++;
			}
		else
			{
			ERROR_RESULT_STATUS("SendAcqMsg SendMsg", status);

			/* Partial error recovery: return the message node to its pool */
			ReturnPoolMem(streamCBPtr->dataMsgPool, copyAcqMsg);
			}
		}
	else
		status = kDSNoMsgErr;

	return status;
	}


/*******************************************************************************************
 * Reply to the client's registered end-of-stream message, if any.
 * Notify the client that we reached some kind of end condition.
 *
 * We can only reply once to a message, so the client must re-register each time.
 *
 * --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---
 * [TBD] Add the following feature and some of these EOS result codes from the
 *       VideoCD streamer:
 *
 * If this indicates a persistent stream condition, remember it so
 * DoRegisterEndOfStream can (re-)reply with the same error code.
 *
 * INPUTS: result, which sets the message's msg_Result, indicating why the stream stopped:
 *		kDSNoErr						=> normal EOF/EOS
 *		kDSStreamStopped				=> the stream was stopped by a kDSOpStopStream
 *		kDSNoDisc						=> the drive door is open or there is no disc present
 *		kDSUnrecoverableIOErr			=> unrecoverable disc I/O error
 *		kDSTooManyRecoverableDataErr	=> too many recoverable disc read or parse errors
 *		kDSAbortErr						=> internal streamer error, e.g. in sending a msg to
 *											a subscriber
 *		kDSEOSRegistrationErr			=> The client tried to register more than one EOS
 *											notification message.
 *		kDSShuttleBackToStart			=> shuttled backwards to start of stream
 *******************************************************************************************/
static void ReplyToEndOfStreamMsg(DSStreamCBPtr streamCBPtr, Err result)
{
#define PERSISTENT_EOS_CODE		0	/* [TBD] add this feature from the VideoCD
			* streamer. It requires "#if PERSISTENT_EOS_CODE" code and more. */
#if PERSISTENT_EOS_CODE
	if ( result != kDSEOSRegistrationErr &&
		 result != kDSStreamStopped )
		streamCBPtr->endOfStreamCode = result;	/* remember this persistent condition */
#endif

	if ( streamCBPtr->endOfStreamMsg != NULL )
		{
		ReplyToReqMsg(streamCBPtr->endOfStreamMsg, result);

		streamCBPtr->endOfStreamMsg = NULL;
		}

	PRNT(("  DS EOS\n"));	/* End Of Stream playback */
}

/*******************************************************************************************
 * Reply to the client's registered start-of-stream message, if any.
 * Notify the client that we reached some kind of end condition.
 *
 * We can only reply once to a message, so the client must re-register each time.
 *
 * --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---
 * [TBD] Add the following feature and some of these EOS result codes from the
 *       VideoCD streamer:
 *
 * If this indicates a persistent stream condition, remember it so
 * DoRegisterStartOfStream can (re-)reply with the same error code.
 *
 * INPUTS: result, which sets the message's msg_Result, indicating why the stream stopped:
 *		kDSNoErr						=> normal EOF/EOS
 *		kDSStreamStopped				=> the stream was stopped by a kDSOpStopStream
 *		kDSNoDisc						=> the drive door is open or there is no disc present
 *		kDSUnrecoverableIOErr			=> unrecoverable disc I/O error
 *		kDSTooManyRecoverableDataErr	=> too many recoverable disc read or parse errors
 *		kDSAbortErr						=> internal streamer error, e.g. in sending a msg to
 *											a subscriber
 *		kDSSOSRegistrationErr			=> The client tried to register more than one EOS
 *											notification message.
 *		kDSShuttleBackToStart			=> shuttled backwards to start of stream
 *******************************************************************************************/
static void ReplyToStartOfStreamMsg(DSStreamCBPtr streamCBPtr, Err result)
{
#define PERSISTENT_EOS_CODE		0	/* [TBD] add this feature from the VideoCD
			* streamer. It requires "#if PERSISTENT_EOS_CODE" code and more. */
#if PERSISTENT_EOS_CODE
	if ( result != kDSSOSRegistrationErr &&
		 result != kDSStreamStopped )
		streamCBPtr->startOfStreamCode = result;/* remember this persistent condition */
#endif

	if ( streamCBPtr->startOfStreamMsg != NULL )
		{
		ReplyToReqMsg(streamCBPtr->startOfStreamMsg, result);

		streamCBPtr->startOfStreamMsg = NULL;
		}

	PRNT(("  DS SOS\n"));	/* Start Of Stream playback */
}

/*******************************************************************************************
 * Reply to the client's registered trigger-bit message, if any.
 * Notify the client that we reached some kind of end condition.
 *
 * We can only reply once to a message, so the client must re-register each time.
 *
 * --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---
 * [TBD] Add the following feature and some of these EOS result codes from the
 *       VideoCD streamer:
 *
 * If this indicates a persistent stream condition, remember it so
 * DoRegisterTriggerBit can (re-)reply with the same error code.
 *
 * INPUTS: result, which sets the message's msg_Result, indicating why the stream stopped:
 *		kDSNoErr						=> normal EOF/EOS
 *		kDSStreamStopped				=> the stream was stopped by a kDSOpStopStream
 *		kDSNoDisc						=> the drive door is open or there is no disc present
 *		kDSUnrecoverableIOErr			=> unrecoverable disc I/O error
 *		kDSTooManyRecoverableDataErr	=> too many recoverable disc read or parse errors
 *		kDSAbortErr						=> internal streamer error, e.g. in sending a msg to
 *											a subscriber
 *		kDSSOSRegistrationErr			=> The client tried to register more than one EOS
 *											notification message.
 *		kDSShuttleBackToStart			=> shuttled backwards to start of stream
 *******************************************************************************************/
static void ReplyToTriggerBitMsg(DSStreamCBPtr streamCBPtr, Err result)
{
#define PERSISTENT_TB_CODE		0	/* [TBD] add this feature from the VideoCD
			* streamer. It requires "#if PERSISTENT_EOS_CODE" code and more. */
#if PERSISTENT_TB_CODE
	if ( result != kDSSOSRegistrationErr &&
		 result != kDSStreamStopped )
		streamCBPtr->startOfStreamCode = result;/* remember this persistent condition */
#endif

	if ( streamCBPtr->triggerBitMsg != NULL ) {
		ReplyToReqMsg(streamCBPtr->triggerBitMsg, result);
		streamCBPtr->triggerBitMsg = NULL;
	}

	PRNT(("  DS TB\n"));	/* trigger-bit */
}


/*******************************************************************************
 * Abort a stream. This halts stream playback and returns the given Err code to
 * the client application in the EOS reply.
 *******************************************************************************/
static void	AbortStream1(DSStreamCBPtr streamCBPtr, Err status)
	{
	if ( !streamCBPtr->fAborted )
		{
		streamCBPtr->fAborted = TRUE;	/* don't abort again */
		PERR(("AbortStream\n"));
		InternalStopStream(streamCBPtr, NULL, SOPT_FLUSH);
		ReplyToEndOfStreamMsg(streamCBPtr, status);
		}
	}

/******************************************************************************
 * AbortStream() aborts a stream, returning kDSAbortErr in the EOS
 * reply to the client application.
 ******************************************************************************/
static void	AbortStream(DSStreamCBPtr streamCBPtr)
	{
	AbortStream1(streamCBPtr, kDSAbortErr);
	}


/*******************************************************************************
 * Utility routine to tear down a stream control block. Here, we only dispose of
 * the non-system resources that were created for the stream, that is, memory
 * allocated and static state. The system will dispose all Items we created.
 *******************************************************************************/
static void TearDownStreamCB(DSStreamCBPtr streamCBPtr)
	{
	if ( streamCBPtr != NULL )
		{
		streamCBPtr->magicCookie = 0;

		DeleteMemPool(streamCBPtr->dataMsgPool);
		DeleteMemPool(streamCBPtr->subsMsgPool);
		FreeMem(streamCBPtr, sizeof(*streamCBPtr));
		}
	}


/*******************************************************************************
 * Enqueue a buffer on the free buffer queue (the holding bin for empty buffers
 * waiting to be refilled).
 *
 * If we are the end of file with data acquisition and we have all of our buffers
 * back in hand (in the free buffer queue), then we've arrived at the "end of
 * stream" condition. So reply to the registered EOS message.
 *
 * [TBD] We should really wait until one or maybe all subscribers emit their
 * last data chunks before firing off the EOS message. When the buffers come
 * home, at least the SAudioSubscriber will be done emitting its data. An
 * MPEGAudioSubscriber, on the other hand, may still be playing its last few
 * decoded buffers.
 *
 * If we're in the fSTOP state (after hitting a STOP chunk) and we have all of our
 * buffers back in hand (in the filled and free buffer queues), then stop the
 * stream and reply to the registered EOS message.
 *******************************************************************************/
static void		FreeDataBuf(DSStreamCBPtr streamCBPtr, DSDataBufPtr bp)
	{
	bp->next = streamCBPtr->freeBufHead;
	streamCBPtr->freeBufHead = bp;

	streamCBPtr->currentFreeBufferCount++;

	if ( streamCBPtr->fEOF && streamCBPtr->totalBufferCount ==
			streamCBPtr->currentFreeBufferCount )
		ReplyToEndOfStreamMsg(streamCBPtr, kDSNoErr);

	else if ( streamCBPtr->fSTOP && streamCBPtr->totalBufferCount ==
			streamCBPtr->currentFreeBufferCount + streamCBPtr->filledBufCnt )
		{
		streamCBPtr->fSTOP = FALSE;
		InternalStopStream(streamCBPtr, NULL, SOPT_NOFLUSH);
		ReplyToEndOfStreamMsg(streamCBPtr, kDSSTOPChunk);
		}
	}


/*******************************************************************************
 * Dequeue a free (i.e. empty) data buffer. Called by the buffer
 * filling code that will schedule disk reads into these buffers.
 *******************************************************************************/
static DSDataBufPtr GetFreeDataBuf(DSStreamCBPtr streamCBPtr)
	{
	DSDataBufPtr		bp;

	/* Take the first free buffer in the queue */
	bp = streamCBPtr->freeBufHead;

	/* If one exists, remove it from the free list and reset the use count. */
	if ( bp != NULL )
		{
		streamCBPtr->freeBufHead	= bp->next;		/* forward link or NULL */
		bp->useCount	 			= 0;

		/* Decrement the number of free buffers we posess */
		streamCBPtr->currentFreeBufferCount--;
		}

	return bp;
	}


/*******************************************************************************
 * Enqueue a newly-filled buffer on the tail of the filled buffer queue, and
 * clear the buffer's useCount.
 *******************************************************************************/
static void AppendFilledBuffer(DSStreamCBPtr streamCBPtr, DSDataBufPtr bp)
	{
	bp->useCount = 0;

	streamCBPtr->filledBufCnt++;

	bp->next = NULL;
	if ( streamCBPtr->filledBufHead == NULL )
		streamCBPtr->filledBufHead = bp;
	else
		streamCBPtr->filledBufTail->next = bp;
	streamCBPtr->filledBufTail = bp;

	if ( streamCBPtr->fSTOP && streamCBPtr->totalBufferCount ==
			streamCBPtr->currentFreeBufferCount + streamCBPtr->filledBufCnt )
		{
		streamCBPtr->fSTOP = FALSE;
		InternalStopStream(streamCBPtr, NULL, SOPT_NOFLUSH);
		ReplyToEndOfStreamMsg(streamCBPtr, kDSSTOPChunk);
		}
	}


/*******************************************************************************
 * Enqueue a partly-filled buffer on the ***head** of the filled buffer queue so
 * it will be the first up for delivery. This is used to put a buffer back in
 * waiting when a chunk in it (e.g. a STOP chunk) causes us to pause delivery
 * mid-buffer. This does NOT clear the buffer's useCount because we may've
 * already delivered some chunks in it and may yet deliver some more.
 *
 * CAUTION: This means the buffer accessed by ReleaseChunk() could be in the
 * filled buffer queue, and the buffer returned by GetFilledDataBuf() could have
 * useCount > 0, with chunks out to subscribers.
 *******************************************************************************/
#if SUPPORT_PARTIAL_STREAM_BLOCK_DELIVERY
static void PrependFilledBuffer(DSStreamCBPtr streamCBPtr, DSDataBufPtr bp)
	{
	streamCBPtr->filledBufCnt++;

	if ( streamCBPtr->filledBufHead == NULL )
		{
		bp->next = NULL;
		streamCBPtr->filledBufTail = bp;
		}
	else
		bp->next = streamCBPtr->filledBufHead;
	streamCBPtr->filledBufHead = bp;

	if ( streamCBPtr->fSTOP && streamCBPtr->totalBufferCount ==
			streamCBPtr->currentFreeBufferCount + streamCBPtr->filledBufCnt )
		{
		streamCBPtr->fSTOP = FALSE;
		InternalStopStream(streamCBPtr, NULL, SOPT_NOFLUSH);
		ReplyToEndOfStreamMsg(streamCBPtr, kDSSTOPChunk);
		}
	}
#endif


/*******************************************************************************
 * Dequeue and return the first filled data buffer. Called by the buffer
 * delivery code that will dispatch chunks from these buffers. Returns NULL if
 * there are no more filled data buffers in the list.
 *
 * CAUTION: Given PrependFilledBuffer(), GetFilledDataBuf() may return a buffer
 * with some chunks already delivered to subscribers. (If any of its chunks are
 * still at subscribers, it'll have useCount > 0.)
 *******************************************************************************/
static DSDataBufPtr	GetFilledDataBuf(DSStreamCBPtr streamCBPtr)
	{
	DSDataBufPtr		bp;

	if ( (bp = streamCBPtr->filledBufHead) != NULL )
		{
		streamCBPtr->filledBufCnt--;

		/* Update the head of the filled buffer list to point to the
		 * next available buffer in the list, if any. */
		streamCBPtr->filledBufHead = bp->next;

		/* If we just removed the tail buffer from the list, make sure
		 * to set the tail pointer to NULL so further queuing will do
		 * the right thing. */
		if ( streamCBPtr->filledBufTail == bp )
			streamCBPtr->filledBufTail = NULL;
		}

	return bp;
	}


/*******************************************************************************
 * Send an empty data buffer to DataAcq to get it filled with data, or--if
 * the stream is stopping, at EOF, or has had some other problem--queue the
 * empty buffer onto the stream's free buffer list.
 *
 * [TBD] Keep reading (preroll) at a STOP chunk? We'd have to watch for end of
 * of playback when filled buffers come home, and handle the case where we hit
 * EOF before then, and ... At least, the client can ask to preroll after getting
 * the stop-chunk EOS message reply.
 ******************************************************************************/
static Err	FillDataBuf(DSStreamCBPtr streamCBPtr, DSDataBufPtr bp)
	{
	DataAcqMsg		acqMsg;
	Err				status;

	if ( (streamCBPtr->acquirePort != 0)
			&& streamCBPtr->fRunning
			&& !streamCBPtr->fEOF
			&& !streamCBPtr->fSTOP )
		{
		/* Ask data acquisition to fill a data buffer. */
		acqMsg.whatToDo				= kAcqOpGetData;
		acqMsg.privatePtr			= NULL;
		acqMsg.msg.data.dsBufPtr	= bp;
		acqMsg.msg.data.bufferSize	= streamCBPtr->bufDataSize;

		status = SendAcqMsg(streamCBPtr, &acqMsg);
		if ( status < 0 )
			return status;

#if BUFFER_COUNTING
		++bufferCounts.atDataAcq;
		SUM_BUFFER_COUNTS(streamCBPtr);
#endif
		}
	else
		/* Add the buffer to the free list */
		FreeDataBuf(streamCBPtr, bp);

	return kDSNoErr;
	}


/*********************************************
 * Flush all filled buffers we currently own to prevent their parsing and
 * delivery--simply move them to the free queue.
 * HOWEVER if first buffer has a useCount > 0, it's had some chunks delivered
 * to subscribers and was put back to the head of the queue. We just want to avoid
 * delivering the rest of its contents, so treat it as a fully-delivered buffer.
 *
 * NOTE: When we move buffers from one internal queue to another, that doesn't
 * change SUM_BUFFER_COUNTS() buffer counts.
 *********************************************/
static void		FlushAllFilledBuffers(DSStreamCBPtr streamCBPtr)
	{
	DSDataBufPtr	bp;

	while ( (bp = GetFilledDataBuf(streamCBPtr)) != NULL )
		{
#if SUPPORT_PARTIAL_STREAM_BLOCK_DELIVERY
		if ( bp->useCount == 0 )
			FreeDataBuf(streamCBPtr, bp);
	#if BUFFER_COUNTING
		else	/* Subscribers are now in charge of this buffer. */
			{
			++bufferCounts.atSubscribers;
			SUM_BUFFER_COUNTS(streamCBPtr);
			}
	#endif	/* #if BUFFER_COUNTING */
#else
		FreeDataBuf(streamCBPtr, bp);
#endif
		}
	}


/*******************************************************************************
 * Utility routine to record the end of use of a chunk in a buffer. This routine
 * is called by HandleSubscriberReply() when subscribers reply to their data
 * messages. Decrement the buffer's useCount. When it reaches zero, send the
 * block back to data acquisition to be refilled.
 *
 * <HPP> Changed the Release chunk to parameter to also get the DSWBChunk infor-
 * mation.  This way we can say the chunk isn't being used.
 *
 * SIDE EFFECT: If something goes wrong, this calls AbortStream() and returns
 * kDSNoErr (!!!). [TBD] Reevaluate this error handling.
 *
 * CAUTION: Given PrependFilledBuffer(), this buffer could be a partially-
 * delivered buffer that's still in the filled buffer queue (at the head of the
 * queue). Don't send such a buffer back for refilling! The rest of its chunks
 * are waiting to get delivered and released.
 *******************************************************************************/
static Err ReleaseChunk(DSStreamCBPtr streamCBPtr, int32 subscriberStatus,
						DSDataBufPtr bp )
	{
	Err				status	= kDSNoErr;

	/* Decrement the buffer's useCount, and refill it or put it back into the
	 * free list if it has no remaining delivered chunks (useCount == 0) AND
	 * if it's not a partially-delivered buffer at the head of the filled queue. */
	if ( --bp->useCount == 0 )
		{
#if SUPPORT_PARTIAL_STREAM_BLOCK_DELIVERY
		if ( bp != streamCBPtr->filledBufHead )
#endif
			{
#if BUFFER_COUNTING
			--bufferCounts.atSubscribers;
			SUM_BUFFER_COUNTS(streamCBPtr);
#endif

			/* If the subscriber is not aborting us and data delivery is
			 * still enabled (stream is running), queue the buffer for another
			 * read and keep the data coming... */
			if ( subscriberStatus == kDSNoErr )
				status = FillDataBuf(streamCBPtr, bp);
			else
				FreeDataBuf(streamCBPtr, bp);
			}
		}

	/* Make sure we haven't seen more replies for this buffer
	 * than there were data chunk messages sent out for it. */
	else if ( bp->useCount < 0 )
		{
		PERR(("ReleaseChunk: negative useCount!\n"));
		status = kDSAbortErr;
		}

	/* Since the subscriber may be informing us of an error condition, pass
	 * the subscriber's input status to all other subscribers if something
	 * other than 'success' is indicated. */
	if ( (subscriberStatus != kDSNoErr) || (status != kDSNoErr) )
		AbortStream(streamCBPtr);

	return kDSNoErr;
	}


/*******************************************************************************
 * Utility routine to add a streamer request message onto the tail of the
 * streamer's request queue. It's ok if reqMsg == NULL.
 *******************************************************************************/
static void		AddDSRequestToTail(DSStreamCBPtr streamCBPtr,
		DSRequestMsgPtr reqMsg)
	{
	if ( reqMsg != NULL )
		{
		reqMsg->link = NULL;

		if ( streamCBPtr->requestMsgHead != NULL )
			{
			/* Add the new message onto the end of the
			 * existing list of queued messages. */
			streamCBPtr->requestMsgTail->link = (void *)reqMsg;
			streamCBPtr->requestMsgTail = reqMsg;
			}
		else
			{
			/* Add the message to an empty queue  */
			streamCBPtr->requestMsgHead = reqMsg;
			streamCBPtr->requestMsgTail = reqMsg;
			}
		}
	}


/*******************************************************************************
 * Utility routine to remove the first entry from the streamer request queue and
 * return a point to it. If the queue is empty, returns NULL.
 *******************************************************************************/
static DSRequestMsgPtr	GetNextDSRequest(DSStreamCBPtr streamCBPtr)
	{
	DSRequestMsgPtr	reqMsg;

	if ( (reqMsg = streamCBPtr->requestMsgHead) != NULL )
		{
		/* Advance the head pointer to point to the next entry in the list. */
		streamCBPtr->requestMsgHead = (DSRequestMsgPtr)reqMsg->link;

		/* If we are removing the tail entry from the list, set the
		 * tail pointer to NULL. */
		if ( streamCBPtr->requestMsgTail == reqMsg )
			streamCBPtr->requestMsgTail = NULL;
		}

	return reqMsg;
	}


/******************************************************************************
 * Call this when the input data from DataAcq reaches EOF.
 * It sets the input EOF flag to stop the buffer-filling process.
 * If the stream buffers are all back from subscribers already, this procedure
 * also replies to the EOF notification msg. If some buffers are out (the
 * normal case), we'll reply when they come home.
 *
 * [TBD] Send an EOS msg to the Subscribers so they can be sure to play out and
 * return the last buffers.
 *
 * NOTE: This does not stop the clock nor clear fRunning. We expect the client
 * to call DSStopStream().
 ******************************************************************************/
static void	InputEOF(DSStreamCBPtr streamCBPtr)
{
	PRNT(("  DS EOF\n"));	/* End Of input File */
	streamCBPtr->fEOF = TRUE;

	if ( streamCBPtr->currentFreeBufferCount
			== streamCBPtr->totalBufferCount )
		ReplyToEndOfStreamMsg(streamCBPtr, kDSNoErr);
}

/******************************************************************************
 * Call this when the input data from DataAcq reaches SOF.
 * It sets the input SOF flag to stop the buffer-filling process.
 * If the stream buffers are all back from subscribers already, this procedure
 * also replies to the SOF notification msg. If some buffers are out (the
 * normal case), we'll reply when they come home.
 *
 * [TBD] Send an SOS msg to the Subscribers so they can be sure to play out and
 * return the last buffers.
 *
 * NOTE: This does not stop the clock nor clear fRunning. We expect the client
 * to call DSStopStream().
 ******************************************************************************/
static void	InputSOF(DSStreamCBPtr streamCBPtr)
{
	PRNT(("  DS SOF\n"));	/* Start Of input File */
	streamCBPtr->fSOF = TRUE;

	ReplyToStartOfStreamMsg(streamCBPtr, kDSNoErr);
}

/******************************************************************************
 * Call this when the input data from parser reaches trigger-bit.
 * It sets the input triggerbit flag to stop the buffer-filling process.
 * If the stream buffers are all back from subscribers already, this procedure
 * also replies to the Trigger-bit notification msg. If some buffers are out (the
 * normal case), we'll reply when they come home.
 *
 * [TBD] Send an trigger-bit msg to the Subscribers so they can be sure to play out and
 * return the last buffers.
 *
 * NOTE: This does not stop the clock nor clear fRunning. We expect the client
 * to call DSStopStream().
 ******************************************************************************/
static void	InputTriggerBit(DSStreamCBPtr streamCBPtr)
{
	PRNT(("  DS TB\n"));			/* trigger-bit occured */
	ReplyToTriggerBitMsg(streamCBPtr, kDSNoErr);
}


/******************************************************************************
 * Call this when we finish switching or repositioning the DataAcq to complete
 * the branch-in-progress work: Reset fBranchingAcq and if the branch was
 * successful: clear EOF, flush stale data buffers, increment the delivery
 * branch number, and inform the subscribers (optionally flushing them).
 * When switching DataAcqs, this hands off responsibility for not delivering
 * incoming stale data from the fBranchingAcq test to the not-current-DataAcq
 * test.
 * The options arg is the branch message options for the subscribers, e.g.
 * SOPT_NOFLUSH, SOPT_FLUSH, SOPT_NEW_STREAM.
 *
 * NOTE: If status >= 0, the caller should probably also set the streamCB fields
 * fClkSetPending, fClkSetNext, fClkSetNeedDestTime, and (if
 * !fClkSetNeedDestTime) clkSetTime.
 ******************************************************************************/
static void	SwitchedOrRepositionedDataAcq(DSStreamCBPtr streamCBPtr, Err status,
		uint32 options)
{
	if ( status >= 0 ) {
		SubscriberMsg	subMsg;

		/* Flush all filled buffers so we don't parse and deliver stale data. */
		FlushAllFilledBuffers(streamCBPtr);

		/* Clear any EOF associated with the previous DataAcq/position and data
		 * so we will read at the new DataAcq/position. */
		streamCBPtr->fEOF = FALSE;
		streamCBPtr->fSOF = FALSE;
		/* Increment the deliveryBranchNumber and inform the subscribers. */
		subMsg.whatToDo					= kStreamOpBranch;
		subMsg.privatePtr				= NULL;
		subMsg.msg.branch.options		= options;
		subMsg.msg.branch.branchNumber	= ++streamCBPtr->deliveryBranchNumber;

		ForEachSubscriber(streamCBPtr, &subMsg);
	}
	else {	/* The Reposition failed. Act as if we never tried. */

		if ( status == kDSSeekPastSOFErr )
			InputSOF(streamCBPtr);
		else
		if ( status == kDSSeekPastEOFErr || streamCBPtr->fEOFDuringBranch )
			InputEOF(streamCBPtr);	/* Do delayed EOF work */
	}

	streamCBPtr->fBranchingAcq = streamCBPtr->fEOFDuringBranch = FALSE;
}


/*==============================================================================
  ==============================================================================
						Message handlers for the Data Stream Thread
  ==============================================================================
  =============================================================================*/


/*******************************************************************************
 * Get channel status bits by sending a kStreamOpGetChan message to the
 * subscriber.
 *******************************************************************************/
static Err	DoGetChannel(DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg)
	{
	SubscriberMsg		subMsg;

	subMsg.whatToDo 			= kStreamOpGetChan;
	subMsg.privatePtr			= reqMsg;
	subMsg.msg.channel.number	= reqMsg->msg.getChannel.channelNumber;
	subMsg.msg.channel.status	= 0;	/* unused for 'get channel' */
	subMsg.msg.channel.mask		= 0;	/* unused for 'get channel' */

	return SendSubscrMsgByType(streamCBPtr, &subMsg,
		reqMsg->msg.getChannel.streamType);
	}


/*******************************************************************************
 * Ask a subscriber for its logical channel status bits by sending it a
 * kStreamOpSetChan message.
 *******************************************************************************/
static Err DoSetChannel(DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg)
	{
	SubscriberMsg		subMsg;

	subMsg.whatToDo 			= kStreamOpSetChan;
	subMsg.privatePtr			= reqMsg;
	subMsg.msg.channel.number	= reqMsg->msg.setChannel.channelNumber;
	subMsg.msg.channel.status	= reqMsg->msg.setChannel.channelStatus;
	subMsg.msg.channel.mask		= reqMsg->msg.setChannel.mask;

	return SendSubscrMsgByType(streamCBPtr, &subMsg,
		reqMsg->msg.setChannel.streamType);
	}


/******************************************************************************
 * Ask subscriber to perform a subscriber-defined control function by sending
 * it a kStreamOpControl message.
 ******************************************************************************/
static Err	DoControl(DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg)
	{
	SubscriberMsg		subMsg;

	subMsg.whatToDo 				= kStreamOpControl;
	subMsg.privatePtr				= reqMsg;
	subMsg.msg.control.controlArg1	= reqMsg->msg.control.userDefinedOpcode;
	subMsg.msg.control.controlArg2	= reqMsg->msg.control.userDefinedArgPtr;

	return SendSubscrMsgByType(streamCBPtr, &subMsg,
		reqMsg->msg.control.streamType);
	}


/******************************************************************************
 * Close an open stream. Sends a "closing" message to all subscribers.
 * (We expect that the stream is already stopped.)
 *
 * [TBD] Is there anything else we should do at this time? Send a
 * kAcqOpDisconnect message to DataAcq?
 ******************************************************************************/
static Err	DoCloseStream(DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg)
	{
	SubscriberMsg	subMsg;
	Err				status;

	/* Ensure we don't deliver or refill any more data.
	 * Don't bother to properly stop the clock or lock its semaphore. */
	streamCBPtr->fRunning = FALSE;

	/* Bug 3002: 28 byte memory leak.  Removed code that sends kAcqOpDisconnect
	 * message to data acquisition module.  Since disconnection is done
	 * asynchronously, the request/reply for kAcqOpDisconnect message never gets
	 * completed because one of the two threads in question will have already
	 * exited. Data acquisition module will do any post clean-up of its
	 * resources when DisposeDataAcq is called. */

	/* Tell each subscriber that the stream is being closed
	 * Note that DataAcq might be doubling as a subscriber (a dubious design). */
	subMsg.whatToDo		= kStreamOpClosing;
	subMsg.privatePtr	= reqMsg;

	status = ForEachSubscriber(streamCBPtr, &subMsg);

	if ( streamCBPtr->repliesPending <= 0 )
		streamCBPtr->fQuitThread = TRUE;
 	/* else HandleSubscriberReply will set fQuitThread when the last reply arrives */

	/* Empty the subscriber table. */
	streamCBPtr->numSubscribers = 0;

	return status;
	}


/******************************************************************************
 * Add, replace, or remove a data subscriber to the subscriber list.
 * The reqMsg contains a subscriber data type code and a subscriber message port
 * Item number. Port 0 means "unsubscribe". Another port number means "subscribe",
 * replacing any previous subscriber for that data type.
 *
 * SIDE EFFECT: Sends a kStreamOpClosing message to a subscriber being
 *   unsubscribed, and a kStreamOpAbort message to any subscriber that fails to
 *   subscribe, so it'll know to shut down.
 ******************************************************************************/
static Err	DoSubscribe(DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg)
	{
	Err					status = kDSNoErr;
	DSSubscriberPtr		sp;
	SubscriberMsg		subMsg;

	/* If there's a current subscriber of this data type, unsubscribe it. */
	sp = LookupSubscriber(streamCBPtr, reqMsg->msg.subscribe.dataType);
	if ( sp != NULL )
		{
		/* Check for an error case: Trying to replace a subscriber with itself. */
		if ( reqMsg->msg.subscribe.subscriberPort == sp->subscriberPort )
			return kDSSubDuplicateErr;

		/* Send a kStreamOpClosing msg to the outgoing subscriber. */
		subMsg.whatToDo		= kStreamOpClosing;
		subMsg.privatePtr	= reqMsg;
		status = SendSubscrMsg(streamCBPtr, &subMsg, sp->subscriberPort);

		/* Remove the outgoing subscriber from the table:
		 * Decrement the count of entries in the table and slide the
		 * remaining table entries (beyond sp) down one entry, atop *sp. The
		 * number of bytes to move equals the number of bytes from *sp to the
		 * start of last still-useful entry in the table. */
		memcpy(sp, sp + 1,
			(char *)&streamCBPtr->subscriber[--streamCBPtr->numSubscribers]
				- (char *)sp);
		}

	/* If the request was simply to unsubscribe, then we're done. */
	if ( reqMsg->msg.subscribe.subscriberPort == 0 )
		return status;

	/* Error if there isn't room in the subscriber table for one more entry. */
	if ( streamCBPtr->numSubscribers >= DS_MAX_SUBSCRIBERS )
		{
		/* Send a kStreamOpAbort msg to the subscriber-wannabe. */
		subMsg.whatToDo		= kStreamOpAbort;
		subMsg.privatePtr	= reqMsg;
		status = SendSubscrMsg(streamCBPtr, &subMsg,
			reqMsg->msg.subscribe.subscriberPort);
		TOUCH(status);

		/* And return the "subscriber table full" error code. */
		return kDSSubMaxErr;
		}

	/* Add the new subscriber to the subscriber table.
	 *
	 * [TBD] We could insertion-sort into the subscriber table and take
	 * advantage of that in LookupSubscriber(). */
	sp = streamCBPtr->subscriber + streamCBPtr->numSubscribers++;
	sp->dataType		= reqMsg->msg.subscribe.dataType;
	sp->subscriberPort	= reqMsg->msg.subscribe.subscriberPort;

	/* Send the new subscriber an initialization messge. */
	subMsg.whatToDo 	= kStreamOpOpening;
	subMsg.privatePtr	= reqMsg;
	status = SendSubscrMsg(streamCBPtr, &subMsg, sp->subscriberPort);

	return status;
	}


/*******************************************************************************
 * Start prefilling empty buffers with data before starting a stream.
 *
 * NOTE: This is now a MOSTLY-synchronous operation: It waits for all but
 * asyncBufferCnt of the empty buffers to return from DataAcq before replying to
 * the client's request msg, where asyncBufferCnt is a parameter in the request
 * msg. This makes preroll more than a hint. It's esp. important with MPEG.
 *
 * We give the client control over the desired preroll buffer level because
 * waiting for ALL the buffers to come back before starting playback could
 * easily miss a disc rev. Tune it empirically, starting with something like 2.
 *******************************************************************************/
static Err	DoPreRollStream(DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg)
{
	Err				status = kDSNoErr;
	DSDataBufPtr	bp;
	DataAcqMsg		acqMsg;
	uint32			buffersToNotWaitFor;

	TOUCH(reqMsg);

	if ( streamCBPtr->fRunning )
		return kDSWasRunningErr;
	if ( streamCBPtr->acquirePort == 0 || streamCBPtr->fEOF )
		return kDSNoErr;

	/* Send all the free buffers to data acq to get filled */
	buffersToNotWaitFor = reqMsg->msg.preroll.asyncBufferCnt;
	while ( (bp = GetFreeDataBuf(streamCBPtr)) != NULL )
		{
		acqMsg.whatToDo				= kAcqOpGetData;
		acqMsg.privatePtr = (streamCBPtr->currentFreeBufferCount < buffersToNotWaitFor) ? NULL : reqMsg;
		acqMsg.msg.data.dsBufPtr	= bp;
		acqMsg.msg.data.bufferSize	= streamCBPtr->bufDataSize;

		status = SendAcqMsg(streamCBPtr, &acqMsg);
		if ( status < 0 )
			break;

#if BUFFER_COUNTING
		++bufferCounts.atDataAcq;
		SUM_BUFFER_COUNTS(streamCBPtr);
#endif
		}

	return status;
}


/******************************************************************************
 * Enable the flow of buffered data to registered subscribers.
 ******************************************************************************/
static Err	DoStartStream(DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg)
{
	SubscriberMsg	subMsg;
	Err				status;

	if ( streamCBPtr->fRunning )
		return kDSWasRunningErr;

	/* Resume playback: Allow data delivery and resume the clock. */
	status = LockSemaphore(streamCBPtr->clockSemaphore, SEM_WAIT);
	CHECK_NEG("Write-lock the presentation clock semaphore", status);

		streamCBPtr->fRunning = TRUE;
		streamCBPtr->branchNumber = streamCBPtr->stoppedBranchNumber;
		streamCBPtr->clockOffset = GetAudioTime() - streamCBPtr->stoppedClock;

	status = UnlockSemaphore(streamCBPtr->clockSemaphore);
	CHECK_NEG("Unlock the presentation clock semaphore", status);

	/* Tell each subscriber that the stream is being started */
	subMsg.whatToDo				= kStreamOpStart;
	subMsg.privatePtr			= reqMsg;
	subMsg.msg.start.options	= reqMsg->msg.start.options;

	if ( subMsg.msg.start.options & SOPT_FLUSH )
		streamCBPtr->fClkSetPending = streamCBPtr->fClkSetNext =
			streamCBPtr->fClkSetNeedDestTime = TRUE;

	return ForEachSubscriber(streamCBPtr, &subMsg);

	/* [TBD] Also flush DataAcq (kAcqOpFlush) if SOPT_FLUSH? */
}


/******************************************************************************
 * Internal procedure to stop streaming data buffers. This stops the stream
 * clock, sets stream flags to prevent buffer flow, sends a "stop" message to
 * all subscribers, and optionally flushes the stream.
 *
 * It's ok to stop a stream that's already stopped. This can be useful, e.g. to
 * stop-flush a stream that was stopped without a flush. It also avoids race
 * conditions.
 *
 * CAUTION: reqMsg may be NULL.
 ******************************************************************************/
static Err	InternalStopStream(DSStreamCBPtr streamCBPtr,
		DSRequestMsgPtr reqMsg, uint32 options)
{
	SubscriberMsg	subMsg;
	Err				status;

	/* Prevent additional data queuing and pause the stream clock.
	 * NOTE: If the client is just pausing, it might want to follow this request
	 *   with a preroll request so buffers will continue refilling. Perhaps it
	 *   doesn't matter much because few buffers will come back from
	 *   subscribers now. */
	if ( streamCBPtr->fRunning )
		{
		status = LockSemaphore(streamCBPtr->clockSemaphore, SEM_WAIT);
		CHECK_NEG("Read-lock the presentation clock semaphore", status);

			streamCBPtr->fRunning = FALSE;
			streamCBPtr->stoppedBranchNumber = streamCBPtr->branchNumber;
			streamCBPtr->stoppedClock = GetAudioTime() -
				streamCBPtr->clockOffset;

		status = UnlockSemaphore(streamCBPtr->clockSemaphore);
		CHECK_NEG("Unlock the presentation clock semaphore", status);
		}

	if ( options & SOPT_FLUSH )
		{
		FlushAllFilledBuffers(streamCBPtr);
		streamCBPtr->fClkSetPending = streamCBPtr->fClkSetNext =
			streamCBPtr->fClkSetNeedDestTime = TRUE;
		}

	/* Tell each subscriber that the stream is now stopped */
	subMsg.whatToDo			= kStreamOpStop;
	subMsg.privatePtr		= reqMsg;
	subMsg.msg.stop.options = options;

	return ForEachSubscriber(streamCBPtr, &subMsg);

	/* [TBD] Also flush DataAcq (kAcqOpFlush) if SOPT_FLUSH? */
	/* [TBD] NOTE: Should we ReplyToEndOfStreamMsg(..., kDSStreamStopped)
	 * to inform the client? If we did that, we'd have to be careful to send the
	 * AbortStream1() code instead if that's why we're stopping. */
}


/******************************************************************************
 * Stop streaming data buffers in response to a client request message.
 *
 * It's ok to stop a stream that's already stopped. This can be useful, e.g. to
 * stop-flush a stream that was stopped without a flush. It also avoids race
 * conditions.
 ******************************************************************************/
static Err	DoStopStream(DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg)
{
	streamCBPtr->fShuttling = FALSE;	/* always turn shuttling off */
	return InternalStopStream(streamCBPtr, reqMsg, reqMsg->msg.stop.options);
}


/*******************************************************************************
 * Internal procedure to branch the stream: Flush any queued, completed data
 * buffers and send a positioning message to data acquisition.
 *
 * CAUTION: reqMsg may be NULL.
 *******************************************************************************/
static Err	InternalGoMarker(DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg,
		uint32 markerValue, uint32 options, uint32 markerTime)
{
	Err				status;
	DataAcqMsg		acqMsg;

	/* Ask the data acquisition client to re-position the stream. */
	acqMsg.whatToDo					= kAcqOpGoMarker;
	acqMsg.privatePtr				= reqMsg;
	acqMsg.msg.marker.value			= markerValue;
	acqMsg.msg.marker.options		= options;
	acqMsg.msg.marker.markerTime	= markerTime;

	status = SendAcqMsg(streamCBPtr, &acqMsg);

	/* Remember that we're currently branching the DataAcq. If and when it
	 * succeeds, we'll flush all pre-branch buffers, clear fEOF, etc. */
	if ( status >= 0 )
		streamCBPtr->fBranchingAcq = TRUE;

	/* NOTE: If the data acq request succeeds, it'll return the branch
	 * destination's presentation time (unless it's a GOMARKER_ABSOLUTE) in
	 * msg.marker.markerTime . We'll use it to adjust the clock. */

	return status;
}


/*******************************************************************************
 * Branch the stream in response to a client request message.
 * This asks DataAcq to try to return the branch destination time.
 *******************************************************************************/
static Err	DoGoMarker(DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg)
{
	return InternalGoMarker(streamCBPtr, reqMsg,
		reqMsg->msg.goMarker.markerValue,
		reqMsg->msg.goMarker.options | GOMARKER_NEED_TIME_FLAG,
		0);
}

/*******************************************************************************
 * Shuttle forward or backward in response to a client request message.
 *******************************************************************************/
static Err	DoShuttle(DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg)
{
	SubscriberMsg	subMsg;
	int32			rate;
	Err				status;

	rate = reqMsg->msg.shuttle.rate;

	/* inform all the subscriber to start shuttling. This means put the video
	 * into a I-Frame mode, and mute the audio */

	streamCBPtr->fShuttling = (rate >= 0 ? SHUTTLE_FORWARD : SHUTTLE_BACKWARD);
	streamCBPtr->fShuttleRate = rate;
	streamCBPtr->fShuttleRelativeRate = rate;

	subMsg.whatToDo			= kStreamOpShuttle;
	subMsg.privatePtr		= NULL;

	PRNT(("SHUTTLING %s and rate = %d\n", (rate >= 0 ? "FORWARD" : "REVERSE"), rate));

	status = ForEachSubscriber(streamCBPtr, &subMsg);

	/*
	 * Don't do a reshuttle here if the shuttle request is for forward movement, 
	 * because it causes the first shuttling step to be roughly twice 
	 * as long as it should be (EG, you'll jump from 2 seconds to 22 
	 * seconds if the shuttle rate is 10 seconds).  The reason seems to be that
	 * there's already enough data in the currently-filled data buffers, and doing
	 * a reshuttle will flush all that data plus jump much further ahead, giving 
	 * you effectively a double-sized jump to start off with.  When the request is
	 * to shuttle backwards, it's okay to flush what's already in the buffers and
	 * start moving backwards immediately.
	 *
	 * [TBD] For what it's worth:  the really right way to do this shuttling stuff
	 *	would be to know what the block number is of the buffer at the head of the
	 *	filled buffer queue, and then at reshuttle time, use a GOMARKER_ABSOLUTE
	 *	to that block number plus the shuttle rate.  That would isolate the app
	 *	from shuttle rates having different effects depending on the buffer count,
	 *	how far ahead the DataAcq is keeping, and other such GOMARKER_RELATIVE
	 *	vagaries that we're living with now.  But, this isn't an easy change, since
	 *	we currently have no way of knowing what the disc block number is for any
	 *	given filled data buffer.
	 */
	 
	if ( status >= 0 && rate < 0) {
		status = ReShuttle(streamCBPtr);
	}
	
	return status;
}

/*******************************************************************************
 * ReShuttle - while in shuttle mode, we need to call this routine cause the
 * dataacq continue jumping to different location.
 *******************************************************************************/
static Err	ReShuttle(DSStreamCBPtr streamCBPtr)
{
	Err				status;

	if ( streamCBPtr->fShuttling )	{

		/* Ask data acquisition to fill a data buffer. */
		status = InternalGoMarker(
					streamCBPtr,
					NULL,
					(uint32)streamCBPtr->fShuttleRelativeRate,
					GOMARKER_RELATIVE,
					0 );

		if ( status < 0 )
			PRNT(("Go Marker error(#%d) (SOF=%d) and (EOF=%d)\n", status, kDSSeekPastSOFErr, kDSSeekPastEOFErr));

#if 0 /* what's the deal here?  did somebody decide that shuttling
  	   * backwards should automatically accellerate?  that should 
	   * be up to the app, not the streamer!
	   */
		if ( streamCBPtr->fShuttleRate < 0 )/* if we are doing reverse */
			streamCBPtr->fShuttleRelativeRate += streamCBPtr->fShuttleRate;
#endif

		streamCBPtr->fFramesDisplayed = 0;	/* <HPP> reset the frame count */
	}

	return kDSNoErr;
}

/*******************************************************************************
 * restore the streamer in regular play state
 *******************************************************************************/

static Err	DoPlay(DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg)
{
	SubscriberMsg	subMsg;
	Err				status;

	TOUCH(reqMsg);

	streamCBPtr->fShuttling = FALSE;

	/* Ask data acquisition to fill a data buffer and start normal continue */
	status = InternalGoMarker(
					streamCBPtr,
					NULL,
					(uint32)0,
					GOMARKER_RELATIVE,
					0 );
	TOUCH(status);
	/* ignore any errors for now */

	/* inform all the subscriber to start regular play mode */

	subMsg.whatToDo			= kStreamOpPlay;
	subMsg.privatePtr		= NULL;

	status = ForEachSubscriber(streamCBPtr, &subMsg);

	return status;
}

/******************************************************************************
 * Connect/replace/disconnect a data acquisition client to the stream. First,
 * if we are currently connected, send a disconnect message to the current data
 * acquisition client. Then, (if acquirePort != 0) switch to the new data
 * acquisition client and send it a connect message.
 *
 * The disconnect message should cause the first data supplier to cancel any
 * outstanding I/O and return the buffers to us as "flushed" so we can ignore
 * their contents and reuse the buffers. Here we flush any data we currently
 * have queued.
 ******************************************************************************/
static Err	DoConnect(DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg)
	{
	Err				status = kDSNoErr;
	DataAcqMsg		acqMsg;

	streamCBPtr->fAborted = FALSE;	/* Experiment: reset this flag here. */

	/* No-op if we're connecting to the same data acq again.
	 * This works even if there's no current data acq (acquirePort == 0). */
	if ( streamCBPtr->acquirePort == reqMsg->msg.connect.acquirePort )
		return kDSNoErr;

	/* If we're currently connected, notify the current data acq. */
	/* [TBD] Make the reqMsg wait for the disconnect operation to complete as
	 * well as the connect operation? Does the client need assurance that it's
	 * safe to do things to the old data acq? We could do that just by setting
	 * acqMsg.privatePtr = reqMsg. */
	if ( streamCBPtr->acquirePort != 0 )
		{
		acqMsg.whatToDo		= kAcqOpDisconnect;
		acqMsg.privatePtr	= NULL;

		status = SendAcqMsg(streamCBPtr, &acqMsg);

		/* If something went wrong, bail out now. */
		if ( status < 0 )
			return status;
		}

	/* Switching DataAcqs is like goto a new file. So flush buffers, reset EOF,
	 * and get ready to set the presentation clock. It's like go-absolute, so
	 * we'll have to discover the destination time when we get there. If asked
	 * to disconnect and later to connect, we don't need to do all this twice.
	 * So set clock on connect or reconnect; flush subscribers on disconnect
	 * ([TBD] Change that to "connect"?) or reconnect.
	 * [TBD] Maybe offer a no-flush option as for go-marker, esp. since changing
	 * streams is sometimes used in development to simulate branching within
	 * segments that will later be concatenated into a long stream. */
	if ( reqMsg->msg.connect.acquirePort != 0 )
		streamCBPtr->fClkSetPending = streamCBPtr->fClkSetNext =
			streamCBPtr->fClkSetNeedDestTime = TRUE;
	SwitchedOrRepositionedDataAcq(streamCBPtr, kDSNoErr,
		(streamCBPtr->acquirePort != 0) ? (SOPT_FLUSH | SOPT_NEW_STREAM) :
			SOPT_NOFLUSH);

	/* NOTE: With a parser for MPEG Systems Streams for VideoCD, we should reset
	 * any parser state here. */

	WBParse_Initialize(streamCBPtr->stillImageType);					/* <HPP> initialize the parser state */

	/* Switch the streamer's data acquisition port to the new port specified. */
	streamCBPtr->acquirePort = reqMsg->msg.connect.acquirePort;

	/* Next, connect to the new port, if any. If we are given a 0 port as
	 * input, then we are done (and all we've done is disconnect the existing
	 * connection, if any). */
	if ( streamCBPtr->acquirePort != 0 )
		{
		/* Send a connect message to the new data acquisition client. */
		acqMsg.whatToDo					= kAcqOpConnect;
		acqMsg.privatePtr				= reqMsg;
		acqMsg.msg.connect.streamCBPtr	= streamCBPtr;
		acqMsg.msg.connect.bufferSize	= streamCBPtr->bufDataSize;

		status = SendAcqMsg(streamCBPtr, &acqMsg);
		}

	return status;
	}


/******************************************************************************
 * Handle registration for end of stream notification.
 * NOTE: This streamer request returns when EOS is reached. Meanwhile, it
 * doesn't prevent other requests from being processed.
 ******************************************************************************/
static Err DoRegisterEndOfStream(DSStreamCBPtr streamCBPtr,
		DSRequestMsgPtr reqMsg)
	{
	/* Reply to previous registrant to notify of losing its registration for
	 * the end of stream condition. The new registrant wil now get the
	 * notification when the condition happens.
	 *
	 * NOTE: This test is redundant with ReplyToEndOfStreamMsg() but it makes it
	 * possible during debugging to break on any EOF reply situation--whether
	 * or not there's a registered EOF msg--and avoid the case where we're
	 * checking for kDSEOSRegistrationErr and there is no problem. */
	if ( streamCBPtr->endOfStreamMsg != NULL )
		ReplyToEndOfStreamMsg(streamCBPtr, kDSEOSRegistrationErr);

	/* Remember the message to reply to later, upon EOS. */
	streamCBPtr->endOfStreamMsg = reqMsg;

#if PERSISTENT_EOS_CODE	/* [TBD] add this feature from the VideoCD streamer */
	/* If the stream is already at EOS, reply right now. This fixes race
	 * conditions where the client would miss EOS notification. E.g. if we
	 * reached bona fide EOS while the client was processing kDSStreamStopped
	 * pause notification. E.g. if the client called DSGoMarker, soaked up the
	 * EOS notification in case it occurred before the seek, but doesn't want to
	 * miss the post-seek EOS. */
	if ( streamCBPtr->endOfStreamCode != kDSNoStopCodeYet )
		ReplyToEndOfStreamMsg(streamCBPtr, streamCBPtr->endOfStreamCode);
#endif

	return kDSNoErr;
	}

/******************************************************************************
 * Handle registration for start of stream notification.
 * NOTE: This streamer request returns when SOS is reached. Meanwhile, it
 * doesn't prevent other requests from being processed.
 ******************************************************************************/
static Err DoRegisterStartOfStream(DSStreamCBPtr streamCBPtr,
		DSRequestMsgPtr reqMsg)
{
	/* Reply to previous registrant to notify of losing its registration for
	 * the start of stream condition. The new registrant wil now get the
	 * notification when the condition happens.
	 *
	 * NOTE: This test is redundant with ReplyToStartOfStreamMsg() but it makes it
	 * possible during debugging to break on any SOF reply situation--whether
	 * or not there's a registered SOF msg--and avoid the case where we're
	 * checking for kDSSOSRegistrationErr and there is no problem. */
	if ( streamCBPtr->startOfStreamMsg != NULL )
		ReplyToStartOfStreamMsg(streamCBPtr, kDSSOSRegistrationErr);

	/* Remember the message to reply to later, upon SOS. */
	streamCBPtr->startOfStreamMsg = reqMsg;

	return kDSNoErr;
}

/******************************************************************************
 * Handle registration for trigger-bit notification.
 * NOTE: This streamer request returns when trigger-bit is reached. Meanwhile, it
 * doesn't prevent other requests from being processed.
 ******************************************************************************/
static Err DoRegisterTriggerBit(DSStreamCBPtr streamCBPtr,
		DSRequestMsgPtr reqMsg)
{
	/* Reply to previous registrant to notify of losing its registration for
	 * the trigger-bit condition. The new registrant wil now get the
	 * notification when the condition happens.
	 *
	 * NOTE: This test is redundant with ReplyToTriggerBitMsg() but it makes it
	 * possible during debugging to break on any trigger-bit reply situation--whether
	 * or not there's a registered trigger-bit msg--and avoid the case where we're
	 * checking for kDSSOSRegistrationErr and there is no problem. */
	if ( streamCBPtr->triggerBitMsg != NULL )
		ReplyToTriggerBitMsg(streamCBPtr, kDSSOSRegistrationErr);

	/* Remember the message to reply to later, upon SOS. */
	streamCBPtr->triggerBitMsg = reqMsg;

	return kDSNoErr;
}

/*============================================================================
  ============================================================================
		Procedural API for other threads to access the Data Stream Thread
  ============================================================================
  ============================================================================*/

Item NewDataStream(DSStreamCBPtr *pCtx, void *bufferListPtr, uint32 bufferSize,
		int32 deltaPriority, int32 numSubsMsgs)
	{
	Err						status;
	uint32					signalBits;
	StreamerCreationArgs	creationArgs;

	*pCtx = NULL;

	status = kDSRangeErr;
	if ( bufferSize < 2048 )	/* sanity check; 32K to 96K is typical */
		goto CLEANUP;

	/* Setup creationArgs, including a signal to synchronize with the
	 * completion of the thread's initialization. It will signal us when it
	 * is done initializing itself, successfully or not. */
	status = kDSNoSignalErr;
	creationArgs.creatorTask	= CURRENTTASKITEM;	/* cf. <kernel/kernel.h>, included by <streaming/threadhelper.h> */
	creationArgs.creatorSignal	= AllocSignal(0);
	if ( creationArgs.creatorSignal == 0 )
		goto CLEANUP;
	creationArgs.bufDataSize		= bufferSize;
	creationArgs.freeBufHead		= (DSDataBufPtr)bufferListPtr;
	creationArgs.numSubsMsgs		= numSubsMsgs;
	creationArgs.creationStatus		= kDSInitErr;
	creationArgs.requestMsgPort		= -1;
	creationArgs.streamCBPtr		= NULL;

	/* Create the data stream execution thread. */
	status = NewThread(
		(void *)(int32)&DataStreamThread,				/* thread entry point */
		4096, 											/* stack size */
		(int32)CURRENT_TASK_PRIORITY + deltaPriority,	/* priority */
		"DataStream",		 							/* name */
		0, 												/* 1st arg to the thread */
		&creationArgs);									/* 2nd arg to the thread */
	if ( status < 0 )
		goto CLEANUP;

	/* Wait here while the thread initializes itself. */
	status = kDSSignalErr;
	signalBits = WaitSignal(creationArgs.creatorSignal);
	if ( signalBits != creationArgs.creatorSignal )
		goto CLEANUP;

	/* We're done with this signal. So release it. */
	FreeSignal(creationArgs.creatorSignal);
	creationArgs.creatorSignal = 0;

	/* Check the thread's creationStatus. */
	status = creationArgs.creationStatus;
	if ( status < 0 )
		goto CLEANUP;

	/* Success! Pass the request msg port and context ptr back to the caller. */
	*pCtx = creationArgs.streamCBPtr;
	return creationArgs.requestMsgPort;

CLEANUP:
	/* Something went wrong.
	 * Release any resources we allocated and return the result status.
	 * The new streamer thread will clean up after itself. */
	if ( creationArgs.creatorSignal )	FreeSignal(creationArgs.creatorSignal);
	return status;
	}



/* IMPLEMENTATION NOTE: This procedure is called by other threads. But it does
 * not need a semaphore because the stream's requestPort field is read-only once
 * the data stream thread is finished initializing, which is before any other
 * thread could get a pointer to the DSStreamCB.
 * But this procedure is not robust about the streamer exiting. It uses a "magic
 * cookie" value to reduce the odds of getting fooled. */

Item	GetDataStreamMsgPort(DSStreamCBPtr streamCBPtr)
	{
	return ( streamCBPtr == NULL || streamCBPtr->magicCookie != DS_MAGIC_COOKIE )
		? kDSBadPtrErr : streamCBPtr->requestPort;
	}


void	DSGetPresentationClock(DSStreamCBPtr streamCBPtr, DSClock *dsClock)
	{
	uint32		stoppedBranchNumber, stoppedClock;
	Err			status;
	uint32		audioTime;

	if ( streamCBPtr == NULL || streamCBPtr->magicCookie != DS_MAGIC_COOKIE )
		{
		APERR(("DSGetPresentationClock: invalid streamCBPtr\n"));
		return;
		}

	status = LockSemaphore(streamCBPtr->clockSemaphore, SEM_SHAREDREAD);
	CHECK_NEG("Read-lock the presentation clock semaphore", status);

		dsClock->running = streamCBPtr->fRunning;
		dsClock->branchNumber = streamCBPtr->branchNumber;
		dsClock->clockOffset = streamCBPtr->clockOffset;
		stoppedBranchNumber = streamCBPtr->stoppedBranchNumber;
		stoppedClock = streamCBPtr->stoppedClock;

	status = UnlockSemaphore(streamCBPtr->clockSemaphore);
	CHECK_NEG("Unlock the presentation clock semaphore", status);

	dsClock->audioTime = audioTime = GetAudioTime();
	if ( dsClock->running )
		dsClock->streamTime = audioTime - dsClock->clockOffset;
	else
		{
		dsClock->branchNumber = stoppedBranchNumber;
		dsClock->streamTime = stoppedClock;
		}
	}


void	DSSetPresentationClock(DSStreamCBPtr streamCBPtr, uint32 branchNumber,
			uint32 streamTime)
{
	Err				status;

	if ( streamCBPtr == NULL || streamCBPtr->magicCookie != DS_MAGIC_COOKIE )
		{
		APERR(("DSSetPresentationClock: invalid streamCBPtr\n"));
		return;
		}

	status = LockSemaphore(streamCBPtr->clockSemaphore, SEM_WAIT);
	CHECK_NEG("Write-lock the presentation clock semaphore", status);

		if ( streamCBPtr->fRunning )
			{
			streamCBPtr->branchNumber = branchNumber;
			streamCBPtr->clockOffset = GetAudioTime() - streamTime;
			}
		else
			{
			streamCBPtr->stoppedBranchNumber = branchNumber;
			streamCBPtr->stoppedClock = streamTime;
			}

	status = UnlockSemaphore(streamCBPtr->clockSemaphore);
	CHECK_NEG("Unlock the presentation clock semaphore", status);
	
}


/* IMPLEMENTATION NOTE: There's no race condition inherent here because reading
 * a word from the stream context is atomic. But the information could be out of
 * date by the time the caller reacts to it. */
bool	DSIsRunning(DSStreamCBPtr streamCBPtr)
	{
	if ( streamCBPtr == NULL || streamCBPtr->magicCookie != DS_MAGIC_COOKIE )
		{
		APERR(("DSIsRunning: invalid streamCBPtr\n"));
		return FALSE;
		}

	return streamCBPtr->fRunning;
	}


/*=============================================================================
  =============================================================================
							Data Stream thread
  =============================================================================
  =============================================================================*/


/*******************************************************************************************
 * This routine is called repeatedly with the replies of requests we have sent to the
 * subscribers. Some replies signify the end of use of a data chunk that we have sent
 * to subscribers, while others are replies to requests we have posted to them.
 *******************************************************************************************/
static void	HandleSubscriberReply(DSStreamCBPtr streamCBPtr, SubscriberMsgPtr subMsg, Message *messagePtr)
{
	DSRequestMsgPtr	reqMsg;

	if ( subMsg == NULL )
		{
		ERROR_RESULT_STATUS("HandleSubscriberReply NULL subMsg reply",
			messagePtr->msg_Result);	/* KILLED (in operror.h) ==> the
				* subscriber task exited before replying to this message so the
				* OS returned the message with NULL contents. */

		/* [TBD] Error recovery? (We'd have to scan the subsMsgPool's allocation
		 * array for the subMsg with the matching msgItem, then decide how to
		 * handle that reply, and return it to subsMsgPool.) */

		AbortStream(streamCBPtr);
		return;
		}

	/* If this is a data reply, the subscriber is finished using a data chunk we
	 * sent it. Release the chunk, perhaps freeing the stream buffer for reuse.
	 * Data messages are initiated by the streamer, with privatePtr pointing to
	 * the stream buffer, so there's no app request to reply to. */
	if ( subMsg->whatToDo == kStreamOpData )
		ReleaseChunk(streamCBPtr, messagePtr->msg_Result, (DSDataBufPtr)subMsg->privatePtr);

	/* If the reply's privatePtr points to the message at the head of the
	 * streamer request queue (the current request), then decrement the count of
	 * repliesPending needed to finish that request. If the counter reaches 0,
	 * reply to that streamer request. (The next request can then be started.)
	 * Return the last negative err code from ack/subscriber replies, if any,
	 * or else the last non-negative code. */
	else if ( (subMsg->privatePtr != NULL) &&
			  (subMsg->privatePtr == streamCBPtr->requestMsgHead) )
		{
		if ( messagePtr->msg_Result < 0 || streamCBPtr->replyResult >= 0 )
			streamCBPtr->replyResult = messagePtr->msg_Result;
		if ( --streamCBPtr->repliesPending <= 0 )
			{
			/* Remove the completed streamer request message from the head of
			 * the request queue. */
			reqMsg = GetNextDSRequest(streamCBPtr);

			/* We have a little extra work to do for some requests.
			 * Get-channel-status: Copy the result to the caller's buffer.
			 * Close-stream: Note that the thread can now quit. */
			switch ( reqMsg->whatToDo )
				{
				case kDSOpGetChannel:
					*reqMsg->msg.getChannel.channelStatusPtr =
						messagePtr->msg_Result;
					break;
				case kDSOpCloseStream:
					streamCBPtr->fQuitThread = TRUE;
					break;
				}

			/* Reply to the current request to indicate that we are
			 * finished processing it. */
			ReplyToReqMsg(reqMsg, streamCBPtr->replyResult);
			streamCBPtr->replyResult = 0;
			}
		}

	/* Some other messages are originated by the streamer with
	 * subMsg->privatePtr == NULL. They require no processing work here:
	 *    kStreamOpStop is sent by AbortStream() and STOP chunk processing.
	 *    kStreamOpBranch is sent by SwitchedOrRepositionedDataAcq().
	 *	  kStreamOpHeaderInfo is sent by DeliverDataChunk to initialize the video header
	 *    kStreamOpShuttle is sent do put the subscribers in shuttle mode
	 *	  kStreamOpPlay is sent to put the subscribers in regular play mode
	 *    [TBD] Maybe someday also kStreamOpEOF.
	 * Any other reply we received is an error. */
	else if ( subMsg->whatToDo != kStreamOpStop &&
			  subMsg->whatToDo != kStreamOpBranch &&
			  subMsg->whatToDo != kStreamOpHeaderInfo &&
			  subMsg->whatToDo != kStreamOpShuttle &&
			  subMsg->whatToDo != kStreamOpPlay &&
			  subMsg->whatToDo != kStreamOpSetAction )
		{
		PERR(("HandleSubscriberReply unexpected reply, whatToDo = %ld\n",
			subMsg->whatToDo));
		}

	/* Return the message node to its pool */
	ReturnPoolMem(streamCBPtr->subsMsgPool, subMsg);
}

/******************************************************************************
 * Process a message reply from the data acquisition client. Some of these
 * messages are simply requests to fill data buffers while others implement
 * stream positioning, etc. Some replies finish app -> streamer requests.
 ******************************************************************************/
static void HandleDataAcqReply(DSStreamCBPtr streamCBPtr, DataAcqMsgPtr acqMsg, Message *messagePtr)
{
	DSRequestMsgPtr	reqMsg;
	DSDataBufPtr	bp;
	Err				msg_Result = messagePtr->msg_Result;

	/* When the current DataAcq replies to a go-marker request, set the
	 * fClkSetPending, fClkSetNext, fClkSetNeedDestTime, and clkSetTime fields,
	 * and finish the fBranchingAcq work: If successful, flush the pipeline
	 * (and optionally the subscribers) and increment the branch number. */
	/* NOTE: If !fFlush, we depend on a subscriber or a timer to set the clock. */
	/* [TBD] Set the clock after a no-flush GOTO-chunk branch via a timer set
	 * for the GOTO chunk's time stamp. */
	if ( acqMsg->whatToDo == kAcqOpGoMarker
			&& acqMsg->dataAcqPort == streamCBPtr->acquirePort )
		{
		const uint32		options = acqMsg->msg.marker.options;
		const bool			fFlush = !(options & GOMARKER_NO_FLUSH_FLAG);

		if ( msg_Result >= 0 )
			{
			streamCBPtr->fClkSetPending = TRUE;
			streamCBPtr->fClkSetNext = fFlush;
			streamCBPtr->fClkSetNeedDestTime = (options & GOMARKER_NEED_TIME_FLAG) != 0;
			streamCBPtr->clkSetTime = acqMsg->msg.marker.markerTime;
			WBParse_SetSCRSeekState(seeking_info);
			}

		SwitchedOrRepositionedDataAcq(streamCBPtr, msg_Result,
			fFlush ? SOPT_FLUSH : SOPT_NOFLUSH);
		}

	/* If the reply's privatePtr points to the message at the head of the
	 * request queue (the current request) then decrement the count of
	 * repliesPending needed to finish that request. If the counter reaches 0,
	 * reply to that streamer request. (The next request can then be started.)
	 * Return the last negative err code from ack/subscriber replies (excepting
	 * EOF or Flushed INFO codes from kDSOpPreRollStream requests), if any, or
	 * else the last non-negative code. */
	if ( (acqMsg->privatePtr != NULL) &&
		 (acqMsg->privatePtr == streamCBPtr->requestMsgHead) )
		{
		if ( (msg_Result < 0 || streamCBPtr->replyResult >= 0) &&
				(acqMsg->whatToDo != kAcqOpGetData ||
				msg_Result != kDSEndOfFileErr && msg_Result != kDSWasFlushedErr) )
			streamCBPtr->replyResult = msg_Result;

		if ( --streamCBPtr->repliesPending <= 0 )
			{
			/* Remove the completed  streamer request message from the head of
			 * the request queue. */
			reqMsg = GetNextDSRequest(streamCBPtr);

			/* Reply to the current request to indicate that we are
			 * finished processing it. */
			ReplyToReqMsg(reqMsg, streamCBPtr->replyResult);
			streamCBPtr->replyResult = 0;
			}
		/* NOTE: The streamer now allows DataAcq repliesPending to be > 1.
		 * This can happen when counting up kAcqOpGetData replies for a client's
		 * kDSOpPreRollStream request, or if we (in the future) count both the
		 * kAcqOpDisconnect and kAcqOpConnect replies for a client's kDSOpConnect
		 * request. */
		}

	/* Some other messages are originated by the streamer itself, not in service
	 * of a message in the request queue. privatePtr should be NULL.
	 * Any other message reply from DataAcq is inappropriate. */
	else if (	acqMsg->whatToDo != kAcqOpGetData
					&& acqMsg->whatToDo != kAcqOpDisconnect
					&& acqMsg->whatToDo != kAcqOpGoMarker
				|| acqMsg->privatePtr != NULL )
		PERR(("HandleDataAcqReply unexpected reply, whatToDo = %ld\n",
			acqMsg->whatToDo));


	/* Handle a reply to a kAcqOpGetData request to fill a data buffer:
	 *    (1) move the buffer to the free buffer list if stale (pre-branch data
	 *        or data from an old DataAcq) or there was an error, else move it
	 *        to the filled buffer list, and
	 *    (2) process its message result Err code. */
	if ( acqMsg->whatToDo == kAcqOpGetData )
		{
		bp = acqMsg->msg.data.dsBufPtr;

		/* Move the data to the free or filled buffer list. If the data is
		 * stale, discount its msg_Result, too. */
		if ( acqMsg->dataAcqPort != streamCBPtr->acquirePort )
			{
			FreeDataBuf(streamCBPtr, bp);
			msg_Result = kDSNoErr;
			}
		else if ( msg_Result != 0 )
			FreeDataBuf(streamCBPtr, bp);
		else
			AppendFilledBuffer(streamCBPtr, bp);

#if BUFFER_COUNTING
		--bufferCounts.atDataAcq;
		SUM_BUFFER_COUNTS(streamCBPtr);
#endif	/* #if BUFFER_COUNTING */

		/* Process the message result code, which could indicate an error or
		 * some kind of special completion like EOF. */
		switch ( msg_Result )
			{
			case kDSNoErr:
				break;

			case kDSWasFlushedErr:
				/* This happens to queued reads when a seek happens, when
				 * switching streams with DSConnect(), and other cases.
				 * This is really not an error, just a special case condition.
				 * There's nothing more to do; we already moved the buffer to
				 * the free buffer list. */
				break;

			case kDSEndOfFileErr:	/* do EOF work later or now */
				if ( streamCBPtr->fBranchingAcq )
					streamCBPtr->fEOFDuringBranch = TRUE;
				else
					InputEOF(streamCBPtr);
				break;

			default:
				/* Halt the stream and inform the client of the abnormal EOS. */
				ERROR_RESULT_STATUS("HandleDataAcqReply kAcqOpGetData",
					msg_Result);
				AbortStream1(streamCBPtr, msg_Result);
				break;
			} /* switch */

		} /* if ( acqMsg->whatToDo == kAcqOpGetData ) */

	/* Return the message node to its pool */
	ReturnPoolMem(streamCBPtr->dataMsgPool, acqMsg);
} /* HandleDataAcqReply */


/*******************************************************************************************
 * Deliver all data buffers currently awaiting delivery, unless we're stopped or
 * temporarily not delivering data.
 * RETURNS: A negative Err code, or 0 for no error, or END_THIS_DELIVERY_CYCLE if
 * we ended this data delivery cycle early for a STRM chunk like STOP or GOTO.
 *******************************************************************************************/
static Err	DeliverData(DSStreamCBPtr streamCBPtr)
{
	DSDataBufPtr		bp;
	Err					status	= kDSNoErr;

	/* If the stream is not running currently, then do not attempt to send
	 * the data to subscribers. */
	if ( !streamCBPtr->fRunning || streamCBPtr->fBranchingAcq ||
			streamCBPtr->fSTOP )
		return kDSNoErr;

	/* Deliver all data buffers currently in the filled buffer list.
	 * For each buffer, scan all the chunks in the buffer, look up each chunk's
	 * subscriber, and message it with a pointer to the chunk data. */
	while ( (bp = GetFilledDataBuf(streamCBPtr)) != NULL )
		{
		status = DeliverDataChunks(streamCBPtr, bp);
		if ( status < 0 )
			break;

		/* Buffer handoff: requeue it, refill it, or subscribers have it.
		 * If we're stopping data delivery and more chunks remain in this
		 * buffer, then put it back at the head of the queue. We'll dequeue it
		 * and hand it off later.
		 * Else if no subscriber claimed any of the chunks in the buffer, then
		 * fill the buffer again with data and keep going. Note that the
		 * following routine will simply queue the buffer back to the free
		 * list if the stream is stopped or at end of file. */
#if SUPPORT_PARTIAL_STREAM_BLOCK_DELIVERY
		if ( status == END_THIS_DELIVERY_CYCLE && bp->curDataPtr != NULL )
			PrependFilledBuffer(streamCBPtr, bp);
		else
#endif
		if ( bp->useCount == 0 )
			status = FillDataBuf(streamCBPtr, bp);
#if BUFFER_COUNTING
		else
			{	/* Subscribers are now in charge of this buffer. */
			++bufferCounts.atSubscribers;
			SUM_BUFFER_COUNTS(streamCBPtr);
			}
#endif	/* #if BUFFER_COUNTING */

		if ( status != 0 )
			break;	/* END_THIS_DELIVERY_CYCLE or negative error */
		}

	return status;
}

/*******************************************************************************************
 * Parse the given stream buffer and deliver all remaining data chunks in it
 * to the appropriate subscribers. This is the guts of DeliverData.
 * Also set the presentation clock if a clock-branch is pending.
 *
 * We set the presentation clock for a flush-branch.
 * [TBD] For a non-flush branch, set a timer and do it when the timer expires.
 * If we don't know when to do it, punt and hope a subscriber sets the clock.
 *
 * NOTE: This now handles chunks that aren't quadbyte-sized. It ASSUMES there's
 * padding in the stream so that the next chunk will still be quadbyte-aligned.
 * This is important for MPEG Audio and maybe MPEG Video.
 *
 * RETURNS: A negative Err code, or 0 for no error, or END_THIS_DELIVERY_CYCLE
 * to ask the caller to end this data delivery cycle early.
 *
 * SIDE EFFECTS: See the "SIDE EFFECTS" note on ProcessSTRMChunk().
 *******************************************************************************************/

DSWBChunk				wbChunks[MAX_WB_CHUNKS];
MPEGVideoHeaderChunk	wbVideoHeader;
static 	int32			prevendcode = -1;

static Err DeliverDataChunks(DSStreamCBPtr streamCBPtr, DSDataBufPtr bp)
{
	Err					status = kDSNoErr;
	SubscriberMsg		subMsg;
	int32				result;
	uint32				i, numOfWBChunks = 0;

	result = WBParse_Data((const char *)bp->curDataPtr,
					      streamCBPtr->bufDataSize,
					      wbChunks,
					      &numOfWBChunks);
	if( result < 0 )					/* error occured lets stop this now */
		return result;

	if( prevendcode != WBParse_GetMPEGEndCodes() ) {
		PRNT(("EndCode = %d\n", WBParse_GetMPEGEndCodes()));
		prevendcode = WBParse_GetMPEGEndCodes();
	}

	if (WBParse_GetAudioSeekState()==seeking_foundInfo) {
		WBParse_SetAudioSeekState(seeking_usedInfo);
	  subMsg.whatToDo = kStreamOpHeaderInfo;
	  subMsg.msg.audioParam.audioModes = WBParse_GetAudioMode();
	  subMsg.msg.audioParam.audioEmphasis = WBParse_GetAudioEmphasis();
	  subMsg.privatePtr = NULL;
	  status = SendSubscrMsgByType(streamCBPtr, &subMsg, MPAU_CHUNK_TYPE);
	  if (status == kDSSubNotFoundErr)
	    status = kDSNoErr;
	  CHECK_NEG("Send kStreamOpHeaderInfo", status);
	}

	if( WBParse_GetVideoSeekState()==seeking_foundInfo ) {
		WBParse_SetVideoSeekState(seeking_usedInfo);
		PRNT(("VideoFrameRate = %d VideoVerticalSize = %d\n", WBParse_GetVideoFrameRate(), WBParse_GetVideoVerticalSize()));
		wbVideoHeader.version = MPVD_STREAM_VERSION;
		wbVideoHeader.maxPictureArea = WBParse_GetVideoVerticalSize();
		wbVideoHeader.framePeriod = VideoSequenceHeader_GetFrameRate(WBParse_GetVideoSequenceHeader());
		subMsg.whatToDo					= kStreamOpHeaderInfo;
		subMsg.privatePtr				= NULL;
		subMsg.msg.data.buffer			= (void *)&wbVideoHeader;
		subMsg.msg.data.branchNumber	= streamCBPtr->deliveryBranchNumber;
		status = SendSubscrMsgByType(streamCBPtr, &subMsg, MPVD_CHUNK_TYPE);
		if ( status == kDSSubNotFoundErr )
			status = kDSNoErr;
		CHECK_NEG("Send kStreamOpHeaderInfo", status);
	}

	/* If we need to discover the clkSetTime, use the first deliverable
	 * chunk's delivery time. If we're ready to set the presentation
	 * clock after a branch, do it just before delivering the next chunk. */
	/* [TBD] Should we do this for a chunk that has no subscriber? */
	if ( streamCBPtr->fClkSetPending ) {
		if ( streamCBPtr->fClkSetNeedDestTime ) {
			if( WBParse_GetSCRSeekState() == seeking_foundInfo ) {
				streamCBPtr->clkSetTime = MPEGTimestampToAudioTicks(WBParse_GetFirstSCR());
				WBParse_SetSCRSeekState(seeking_usedInfo);
				streamCBPtr->fClkSetNeedDestTime = FALSE;
			}
		}

		if ( streamCBPtr->fClkSetNext && !streamCBPtr->fClkSetNeedDestTime ) {
			DSSetPresentationClock(streamCBPtr,
				streamCBPtr->deliveryBranchNumber,
				streamCBPtr->clkSetTime);
				/* [TBD] Subtract a fudge factor to allow for delivery
				 * and decode time? (But don't let it go negative.) */
			streamCBPtr->fClkSetPending = streamCBPtr->fClkSetNext =
				FALSE;
		}
	}

	for(i=0; i<numOfWBChunks;i++) {

		/* keep track of our latest PTS value */
		if( wbChunks[i].ptsValid )
			streamCBPtr->fLatestPTS = wbChunks[i].pts;

		/* <HPP> videocd 2.0 feature waits to find a trigger bit */
		if( wbChunks[i].packetStart & 0x00000010 /*&& streamCBPtr->triggerBitMsg */) {
			subMsg.whatToDo = kStreamOpSetAction;
	  		subMsg.privatePtr = NULL;
			subMsg.msg.action.type = 0;
			subMsg.msg.action.pts = streamCBPtr->fLatestPTS;
			subMsg.msg.action.proc = (void *)InputTriggerBit;
			subMsg.msg.action.param = (void *)streamCBPtr;
			status = SendSubscrMsgByType(streamCBPtr, &subMsg, MPVD_CHUNK_TYPE);
			if (status == kDSSubNotFoundErr)
	    		status = kDSNoErr;
	  		CHECK_NEG("Send kStreamOpSetAction", status);
		}

		/* If there is a subscriber for this data type, send it a pointer
		 * to the data then increment the buffer's use count
		 * (decremented when the subscriber returns the chunk). */
		subMsg.whatToDo 				= kStreamOpData;
		subMsg.privatePtr				= (void *)bp;
		subMsg.msg.data.buffer			= (void *)wbChunks[i].buffer;
		subMsg.msg.data.branchNumber	= streamCBPtr->deliveryBranchNumber;
		subMsg.msg.data.bufferSize 		= wbChunks[i].bufferSize;
		subMsg.msg.data.pts 			= wbChunks[i].pts;
		subMsg.msg.data.ptsValid 		= wbChunks[i].ptsValid;
		subMsg.msg.data.packetStart 	= wbChunks[i].packetStart;

	 	status = SendSubscrMsgByType(streamCBPtr, &subMsg, wbChunks[i].chunkType);
		if ( status == kDSNoErr )
			bp->useCount++;
		else if ( status == kDSSubNotFoundErr )
			status = kDSNoErr;
		else
			CHECK_NEG("SendSubscrMsgByType", status);
	}

	if ( status < kDSNoErr ) {
		PRNT(("<TESTING> AbortStream in DeliverDataChunks\n"));
		AbortStream(streamCBPtr);
	}

	/* <HPP> this makes our routine to shuttle continuously */
	if ((streamCBPtr->fShuttling && (streamCBPtr->fFramesDisplayed > 0)) ||
		(streamCBPtr->fShuttling == SHUTTLE_BACKWARD && streamCBPtr->fEOF))
		ReShuttle(streamCBPtr);

	return status;
} /* DeliverDataChunks */

/*******************************************************************************************
 * Enqueue a new DS request message (if not NULL) and dispatch the request at
 * at the head of the queue (if we're not still working on another request).
 * Each message is a single request, and it must be replied to once processed so
 * the sender can tell that the request has been processed. This lets the sender
 * operate either synchronously or asynchronously with the DataStream thread.
 *******************************************************************************************/
static void HandleDSRequest(DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg)
	{
	Err		status;

	/* Add the new streamer request message (if any) to the tail of the request
	 * message queue. This procedure doesn't mind a NULL reqMsg. */
	AddDSRequestToTail(streamCBPtr, reqMsg);

	/* If we're still processing a previous request or a GOTO-chunk branch, then
	 * just return. */
	if ( streamCBPtr->repliesPending > 0 || streamCBPtr->fBranchingAcq )
		return;

	/* Else dispatch the request at the head of the request queue, if any.
	 * Zero the count of replies still needed to complete the new request. */
	streamCBPtr->repliesPending = streamCBPtr->replyResult = 0;
	if ( (reqMsg = streamCBPtr->requestMsgHead) == NULL )
		return;

	switch ( reqMsg->whatToDo )
		{
		case kDSOpPreRollStream:
			status = DoPreRollStream(streamCBPtr, reqMsg);
			break;

		case kDSOpCloseStream:
			status = DoCloseStream(streamCBPtr, reqMsg);
			break;

		case kDSOpWaitEndOfStream:
			status = DoRegisterEndOfStream(streamCBPtr, reqMsg);
			break;

		case kDSOpWaitStartOfStream:
			status = DoRegisterStartOfStream(streamCBPtr, reqMsg);
			break;

		case kDSOpWaitTriggerBit:
			status = DoRegisterTriggerBit(streamCBPtr, reqMsg);
			break;

		case kDSOpStartStream:
			status = DoStartStream(streamCBPtr, reqMsg);
			break;

		case kDSOpStopStream:
			status = DoStopStream(streamCBPtr, reqMsg);
			break;

		case kDSOpSubscribe:
			status = DoSubscribe(streamCBPtr, reqMsg);
			break;

		case kDSOpGoMarker:
			status = DoGoMarker(streamCBPtr, reqMsg);
			break;

		case kDSOpGetChannel:
			status = DoGetChannel(streamCBPtr, reqMsg);
			break;

		case kDSOpSetChannel:
			status = DoSetChannel(streamCBPtr, reqMsg);
			break;

		case kDSOpControl:
			status = DoControl(streamCBPtr, reqMsg);
			break;

		case kDSOpConnect:
			status = DoConnect(streamCBPtr, reqMsg);
			break;

		case kDSOpShuttle:
			PRNT(("kDSOpShuttle...\n"));
			status = DoShuttle(streamCBPtr, reqMsg);
			break;

		case kDSOpPlay:
			PRNT(("kDSOpPlay...\n"));
			status = DoPlay(streamCBPtr, reqMsg);
			break;

		case kDSOpFrameDisplayed:
			streamCBPtr->fFramesDisplayed++;
			break;

		case kDSOpDoHighResStills:
			PRNT(("kDSOpDoHighResStills...\n"));
			streamCBPtr->stillImageType = reqMsg->msg.doHighResStills.option ? 
				STREAMID_VIDEO_HIGH_STILL : STREAMID_VIDEO_NORMAL_STILL;
			status = 0;
			break;

		default:
			status = kDSInvalidDSRequest;
		}

	/* If the request was completely processed (repliesPending == 0) already,
	 * remove it from the request queue and reply to it now UNLESS it's a
	 * DoRegisterEndOfStream, DoRegisterStartOfStream or DoRegisterTriggerBit request,
	 * to which we'll reply when EOS , SOS, or TB occurs. */
	if ( streamCBPtr->repliesPending == 0 ) {
		reqMsg = GetNextDSRequest(streamCBPtr);
		if ( reqMsg->whatToDo != kDSOpWaitEndOfStream	&&
			 reqMsg->whatToDo != kDSOpWaitStartOfStream	&&
			 reqMsg->whatToDo != kDSOpWaitTriggerBit )
			status = ReplyToReqMsg(reqMsg, status);
	}

	if ( status < 0 ) {
		PERR(("HandleDSRequest whatToDo = %ld", reqMsg->whatToDo));
		ERROR_RESULT_STATUS("", status);
	}
}

/*******************************************************************************************
 * Do one-time initialization for the new streamer thread: Allocate its
 * context structure (instance data), allocate system resources, etc.
 *
 * RETURNS: The new context pointer if successful, or NULL if failed.
 * SIDE EFFECTS: To communicate with the spawning process, this fills in the
 *    output fields of the creationArgs structure and then signals the spawning
 *    process.
 * NOTE: Once we signal the spawning process, the creationArgs structure will
 *    go away out from under us.
 *******************************************************************************************/
static DSStreamCBPtr InitializeDSThread(StreamerCreationArgs *creationArgs)
	{
	DSStreamCBPtr		streamCBPtr;
	Err					status;
	DSDataBufPtr		dp;
	int32				bufferCount;

	/* Allocate the streamer context structure (instance data) zeroed
	 * out, and start initializing fields. */
	status = kDSNoMemErr;
	streamCBPtr = AllocMem(sizeof(*streamCBPtr), MEMTYPE_FILL);
	if ( streamCBPtr == NULL )
		goto BAILOUT;
	streamCBPtr->bufDataSize	= creationArgs->bufDataSize;
	streamCBPtr->freeBufHead	= creationArgs->freeBufHead;


	/* Make sure that the buffer list passed in contains only QUADBYTE
	 * aligned buffers. Also, count the buffers. */
	status = kDSBadBufAlignErr;
	bufferCount = 0;
	for ( dp = streamCBPtr->freeBufHead; dp != NULL; dp = dp->next )
		{
		if ( NOT_QUADBYTE_ALIGNED(dp) )
			goto BAILOUT;
		bufferCount++;
		}
	if ( bufferCount < 2 )
		{ status = kDSRangeErr; goto BAILOUT; }

	/* Initialize buffer counts. We keep track of the total number of buffers
	 * and the number currently in the free list. We use this to know when we
	 * reach end of stream: a combination of reaching EOF in the input stream
	 * *and* knowing when all buffers have been drained by the subscribers. */
	streamCBPtr->totalBufferCount		= bufferCount;
	streamCBPtr->currentFreeBufferCount	= bufferCount;
#if BUFFER_COUNTING
	memset(&bufferCounts, 0, sizeof(bufferCounts));
#endif

	/* Open the Audio Folio for this thread */
	if ( (status = OpenAudioFolio()) < 0 )
		goto BAILOUT;

	/* Create the message port for incomming requests. */
	status = NewMsgPort(&streamCBPtr->requestPortSignal);
	if ( status < 0 )
		goto BAILOUT;
	creationArgs->requestMsgPort = streamCBPtr->requestPort = status;

	/* Create the message port for replies from the data acquisition module. */
	status = NewMsgPort(&streamCBPtr->acqReplyPortSignal);
	if ( status < 0 )
		goto BAILOUT;
	streamCBPtr->acqReplyPort = status;

	/* Create the message port for subscriber replies. */
	status = NewMsgPort(&streamCBPtr->subsReplyPortSignal);
	if ( status < 0 )
		goto BAILOUT;
	streamCBPtr->subsReplyPort = status;

	/* Create the semaphore to protect the presentation clock.
	 * [TBD] Optimization: Use the PowerPC reservation register instead. */
	status = streamCBPtr->clockSemaphore = CreateSemaphore(NULL, 0);
	if ( status < 0 )
		goto BAILOUT;

	/* Create a pool of messages for sending data & commands to subscribers. */
	status = kDSNoMemErr;
	streamCBPtr->subsMsgPool =
		CreateMemPool(creationArgs->numSubsMsgs, sizeof(SubscriberMsg));
	if ( streamCBPtr->subsMsgPool == NULL )
		goto BAILOUT;

	/* Create a pool of messages for sending data & commands to data acquisition.
	 * We need at least one per data buffer plus one for each async request we
	 * may need to send. For now, these are: kAcqOpGoMarker, kAcqOpConnect,
	 * and kAcqOpDisconnect. Pad it on the high side for safety. */
#define	NUMBER_OF_ADDITIONAL_DATA_MSGS	(3 * 2)
	streamCBPtr->dataMsgPool = CreateMemPool(
		bufferCount + NUMBER_OF_ADDITIONAL_DATA_MSGS, sizeof(DataAcqMsg));
	if ( streamCBPtr->dataMsgPool == NULL )
		goto BAILOUT;

	/* Initialize each pool entry with its system resources. */
	status = kDSNoMsgErr;
	if ( !FillPoolWithMsgItems(streamCBPtr->subsMsgPool,
			streamCBPtr->subsReplyPort) )
		goto BAILOUT;
	if ( !FillPoolWithMsgItems(streamCBPtr->dataMsgPool,
			streamCBPtr->acqReplyPort) )
		goto BAILOUT;

	/* Success! */
	streamCBPtr->magicCookie = DS_MAGIC_COOKIE;
	creationArgs->streamCBPtr = streamCBPtr;
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
		TearDownStreamCB(streamCBPtr);
		streamCBPtr = NULL;
		}

	return streamCBPtr;
	}


/*******************************************************************************************
 * Server thread for running a data stream. It waits for service requests, data
 * completion messages, and subscriber completion messages; parses received data
 * into chunks and delivers them to subscribers; and repeats until it receives a
 * "close" request or a catestrophic error occurrs.
 * This thread is spawned by NewDataStream().
 *******************************************************************************************/
static void		DataStreamThread(int32 notUsed,
		StreamerCreationArgs *creationArgs)
	{
	Err					status;
	DSStreamCBPtr		streamCBPtr;
	uint32				signalBits;
	uint32				anySignal;


	TOUCH(notUsed);

	/* Call a subroutine to perform all startup initialization. */
	streamCBPtr = InitializeDSThread(creationArgs);
	creationArgs = NULL;	/* can't access that memory anymore */
	TOUCH(creationArgs);	/* avoid a compiler warning about unused assignment */
	if ( streamCBPtr == NULL )
		goto FAILED;

	/* All resources are now allocated and ready to use. Our creator has been
	 * informed that we are ready to accept requests for work. All that's left
	 * to do is wait for work request messages to arrive. */
	anySignal = streamCBPtr->requestPortSignal
				| streamCBPtr->acqReplyPortSignal
				| streamCBPtr->subsReplyPortSignal;

	/***********************************************
	 * Run the main thread loop until told to quit
	 ***********************************************/
	while ( !streamCBPtr->fQuitThread )
		{
		signalBits = WaitSignal(anySignal);

		/*********************
		 * Handle all work request messages. We do this first because someone
		 * may be trying to shut off the stream or close it, and we want to
		 * handle those requests before continuing to spew data.
		 *********************/
		if ( signalBits & streamCBPtr->requestPortSignal )
			{
			Item				msgItem;
			DSRequestMsgPtr		reqMsg;

			while ( (msgItem = GetMsg(streamCBPtr->requestPort)) > 0 )
				{
				reqMsg = MESSAGE(msgItem)->msg_DataPtr;
				HandleDSRequest(streamCBPtr, reqMsg);
				}
			CHECK_NEG("Streamer request GetMsg", msgItem);
			}


		/*********************
		 * Handle replies from subscribers (freeing data chunks, etc.)
		 *********************/
		if ( signalBits & streamCBPtr->subsReplyPortSignal )
			{
			Item				msgItem;
			Message				*messagePtr;
			SubscriberMsgPtr	subMsg;

			while ( (msgItem = GetMsg(streamCBPtr->subsReplyPort)) > 0 )
				{
				messagePtr = MESSAGE(msgItem);
				subMsg = messagePtr->msg_DataPtr;
				HandleSubscriberReply(streamCBPtr, subMsg, messagePtr);
				}
			CHECK_NEG("Subscriber reply GetMsg", msgItem);
			}


		/*********************
		 * Handle replies from data acquisition (arriving data, etc.)
		 *********************/
		if ( signalBits & streamCBPtr->acqReplyPortSignal ) {
			Item				msgItem;
			Message				*messagePtr;
			DataAcqMsgPtr		acqMsg;

			while ( (msgItem = GetMsg(streamCBPtr->acqReplyPort)) > 0 ) {
				messagePtr = MESSAGE(msgItem);
				acqMsg = messagePtr->msg_DataPtr;
				HandleDataAcqReply(streamCBPtr, acqMsg, messagePtr);
			}
			CHECK_NEG("DataAcq reply GetMsg", msgItem);
		}


		/*********************
		 * If there are no replies pending, then we may have just completed
		 * a client request. While no request is in progress, requests are
		 * queued, and no GOTO-chunk branch is in progress, dispatch queued
		 * requests. Again, do this before moving buffers around because the
		 * client may be trying to stop the stream.
		 *********************/
		while ( streamCBPtr->repliesPending <= 0 &&
				streamCBPtr->requestMsgHead != NULL &&
				!streamCBPtr->fBranchingAcq )
			{
			HandleDSRequest(streamCBPtr, NULL);
			}


		/*********************
		 * Send out any data we have accumulated
		 *********************/
		status = DeliverData(streamCBPtr);
		CHECK_NEG("DeliverData", status);


		/*********************
		 * Try to fill any unfilled buffers that we are holding if and only if:
		 * (1) there is currently a data acquisition client connected
		 * (2) the stream is "running" [TBD or "prerolling"]
		 * (3) the stream is not currently at EOF
		 * (4) the streamer is not processing a STOP chunk
		 *
		 * Note: If these factors aren't true, FillDataBuf() will just move the
		 * buffer back to the free list, which would case an infinite loop here.
		 *
		 * [TBD] Keep reading (prerolling) at a STOP chunk?
		 *********************/
		if ( (streamCBPtr->acquirePort != 0)
				&& streamCBPtr->fRunning
				&& !streamCBPtr->fEOF
				&& !streamCBPtr->fSTOP )
			{
			DSDataBufPtr		bp;

			while ( (bp = GetFreeDataBuf(streamCBPtr)) != NULL )
				{
				status = FillDataBuf(streamCBPtr, bp);
				if ( status < kDSNoErr )
					{
					AbortStream(streamCBPtr);
					break;
					}
				}
			}
		}

FAILED:

	/* Time to clean up. */
	TearDownStreamCB(streamCBPtr);

	return;
	}
