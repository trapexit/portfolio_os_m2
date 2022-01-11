/******************************************************************************
**
**  @(#) datastream.c 96/08/30 1.70
**
******************************************************************************/


#include <stdlib.h>
#include <string.h>				/* for memset, memcpy */
#include <kernel/types.h>
#include <kernel/operror.h>
#include <kernel/semaphore.h>
#include <kernel/debug.h>		/* for PERR, CHECK_NEG */
#include <kernel/mem.h>
#include <audio/audio.h>

#include <streaming/datastream.h>
#include <streaming/datastreamlib.h>
#include <streaming/threadhelper.h>
#include <streaming/msgutils.h>


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

/* This flag enables the HALT feature. (A HALT chunk has an embedded
 * chunk for some subscriber. When the Data Stream thread encounters a HALT
 * chunk during playback, it sends the embedded chunk to its subscriber and
 * halts playback temporarily. When the subscriber replies to that message, the
 * data streamer exits HALT mode and resumes normal playback.) */
#define HALT_ENABLE			1		/* 0 turns it off, 1 turns it on */

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

	bool	fEOF;					/* hit end-of-file from data acq? */
	bool	fSTOP;					/* processing a STOP chunk? */
	bool	fBranchingAcq;			/* branching the DataAcq? If and when it
									 * succeeds, discard (stale) buffers */
	bool	fEOFDuringBranch;		/* input EOF arrived while fBranchingAcq? */
	bool	fQuitThread;			/* ready to quit the thread? */
	bool	fAborted;				/* aborted the stream? don't keep doing it */

	uint32	deliveryBranchNumber;	/* branch number of data going to subscribers */

	bool	fClkSetPending;			/* is a clock-set pending? (after go-marker or DSConnect) */
	bool	fClkSetNext;			/* set the clock on next data delivery (if we know clkSetTime)? */
	bool	fClkSetNeedDestTime;	/* need to discover clkSetTime? */
	uint32	clkSetTime;				/* new clock value if fClkSetPending && !fClkSetNeedDestTime */

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

	DSRequestMsgPtr	requestMsgHead;	/* pointer to 1st queued request msg */
	DSRequestMsgPtr	requestMsgTail;	/* pointer to last queued request msg */
	Err		replyResult;			/* most significant result code from replies */
	int32	repliesPending;			/* # of replies needed to complete the
									 * current request */

#if HALT_ENABLE						/* When in a halted state for sync */
	Item	haltChunkReplyPort;		/* reply port for the HALT msg */
	SubscriberMsgPtr halted_msg;	/* the HALT message */
#endif /* HALT_ENABLE */

	uint32	numSubscribers;			/* number of subscribers in the table */
	DSSubscriber subscriber[DS_MAX_SUBSCRIBERS];

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

#if HALT_ENABLE
static Err CreateHaltChunkMsg(DSStreamCBPtr streamCBPtr);
#endif


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
static Err ProcessSTRMChunk(DSStreamCBPtr streamCBPtr, DSDataBufPtr bp,
		StreamChunkPtr cp, uint8 *lastPossibleChunkInBfrAddr);
#if HALT_ENABLE
static Err DeliverHaltChunk(DSStreamCBPtr streamCBPtr,
				DSDataBufPtr bp, HaltChunk1 *scp);
#endif
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
	if ( sp == NULL )
		return kDSSubNotFoundErr;	/* NOTE: The caller may not consider this an error */

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

#if HALT_ENABLE
		FreeMem(streamCBPtr->halted_msg, sizeof(*streamCBPtr->halted_msg));
#endif

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
 * SIDE EFFECT: If something goes wrong, this calls AbortStream() and returns
 * kDSNoErr (!!!). [TBD] Reevaluate this error handling.
 *
 * CAUTION: Given PrependFilledBuffer(), this buffer could be a partially-
 * delivered buffer that's still in the filled buffer queue (at the head of the
 * queue). Don't send such a buffer back for refilling! The rest of its chunks
 * are waiting to get delivered and released.
 *******************************************************************************/
static Err ReleaseChunk(DSStreamCBPtr streamCBPtr, int32 subscriberStatus,
		DSDataBufPtr bp)
	{
	Err			status	= kDSNoErr;

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
	if ( status >= 0 )
		{
		SubscriberMsg	subMsg;

		/* Flush all filled buffers so we don't parse and deliver stale data. */
		FlushAllFilledBuffers(streamCBPtr);

		/* Clear any EOF associated with the previous DataAcq/position and data
		 * so we will read at the new DataAcq/position. */
		streamCBPtr->fEOF = FALSE;

		/* Increment the deliveryBranchNumber and inform the subscribers. */
		subMsg.whatToDo					= kStreamOpBranch;
		subMsg.privatePtr				= NULL;
		subMsg.msg.branch.options		= options;
		subMsg.msg.branch.branchNumber	= ++streamCBPtr->deliveryBranchNumber;

		ForEachSubscriber(streamCBPtr, &subMsg);
		}
	else	/* The Reposition failed. Act as if we never tried. */
		{
		if ( streamCBPtr->fEOFDuringBranch )
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
		acqMsg.privatePtr			=
			(streamCBPtr->currentFreeBufferCount < buffersToNotWaitFor) ? NULL : reqMsg;
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


/*============================================================================
  ============================================================================
		Procedural API for other threads to access the Data Stream Thread
  ============================================================================
  ============================================================================*/

/******************************************************************************
|||	AUTODOC -public -class Streaming -group Startup -name NewDataStream
|||	Instantiates a new data stream thread.
|||
|||	  Synopsis
|||
|||	    Item NewDataStream(DSStreamCBPtr *pCtx, void *bufferListPtr,
|||	        uint32 bufferSize, int32 deltaPriority, int32 numSubsMsgs)
|||
|||	  Description
|||
|||	    Instantiates a new data stream (server) thread. This creates the thread,
|||	    allocates all its resources, and waits for initialization to complete.
|||	    Call DisposeDataStream() to ask the stremer thread to clean up and exit.
|||
|||	    This currently returns the streamer's context-block (instance variables)
|||	    pointer via *pCtx. The caller should not peek or poke through this
|||	    pointer. But currently, there are many procedures which require a
|||	    DSStreamCBPtr argument.
|||
|||	    This temporarily allocates and frees a signal in the caller's thread.
|||	    Currently, 4096 bytes are allocated for the streamer thread's stack.
|||
|||	  Arguments
|||
|||	    pCtx
|||	        Address to store the new stream context pointer.
|||
|||	    bufferListPtr
|||	        Pointer to a list of input buffers for the streamer to use.
|||
|||	        Currently, the streamer uses the first part of each of these
|||	        structures as a header to track the buffer, and the rest as
|||	        an I/O buffer. In the future, we'll separate these two, so the
|||	        buffers themselves can be reused while streaming is quiescent.
|||
|||	    bufferSize
|||	        Size of each buffer in the list. This MUST be a whole number of
|||	        disc blocks and it MUST agree with the woven stream block size.
|||
|||	    deltaPriority
|||	        Task priority for the streamer thread, relative to the caller.
|||
|||	    numSubsMsgs
|||	        Number of subscriber messages that the streamer should allocate.
|||
|||	  Return Value
|||
|||	    The Data Stream thread's request message port, or a negative Portfolio
|||	    error code such as:
|||
|||	    kDSNoMemErr
|||	        Could not allocate enough memory.
|||
|||	    kDSBadBufAlignErr
|||	        One or more of the buffers passed in is not quadbyte aligned.
|||
|||	    kDSRangeErr
|||	        Not enough buffers passed in, or other input-parameter range error.
|||
|||	    kDSSignalErr
|||	        Problem sending/receiving a signal.
|||
|||	    kDSInitErr
|||	        Other initialization error.
|||
|||	    kDSNoSignalErr
|||	        Couldn't allocate a Signal to synchronize with the spawning thread.
|||
|||	    kDSNoMsgErr
|||	        Couldn't allocate a needed Message Item.
|||
|||	    This procedure also returns a pointer to the new streamer context in
|||	    *pCtx.
|||
|||	  Implementation
|||
|||	    Streaming library call.
|||
|||	  Associated Files
|||
|||	    <streaming/datastream.h>, libds.a
|||
|||	  See Also
|||
|||	    DisposeDataStream()
|||
 ******************************************************************************/
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


/******************************************************************************
|||	AUTODOC -private -class Streaming -group Startup -name GetDataStreamMsgPort
|||	Returns the Data Stream thread's request message port.
|||
|||	  Synopsis
|||
|||	    Item GetDataStreamMsgPort(DSStreamCBPtr streamCBPtr)
|||
|||	  Description
|||
|||	    Returns the Data Stream thread's request message port. Use this message
|||	    port to send it messages.
|||
|||	  Arguments
|||
|||	    streamCBPtr
|||	        The Data Stream thread's context pointer.
|||
|||	  Return Value
|||
|||	    The Item number of the Data Stream thread's request message port, or a
|||	    Portfolio error code such as kDSBadPtrErr.
|||
|||	  Assumes
|||
|||	    This procedure peeks into the Data Stream thread's data structure,
|||	    which means it has to assume that thread hasn't exited. So it's not
|||	    robust about the Data Stram thread exiting. Its result--a message
|||	    port Item number--is robust because it will become a "bad Item" when
|||	    the Data Stream thread exits. So it's better to keep a copy of the
|||	    message port Item number (from NewDataStream() or
|||	    GetDataStreamMsgPort()) than a copy of the DSStreamCBPtr.
|||
|||	  Implementation
|||
|||	    Streaming library call.
|||
|||	  Associated Files
|||
|||	    <streaming/datastream.h>, libds.a
|||
|||	  See Also
|||
|||	    NewDataStream(), DisposeDataStream()
|||
 ******************************************************************************/

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


/******************************************************************************
|||	AUTODOC -public -class Streaming -group Clock -name DSGetPresentationClock
|||	Returns the Data Stream presentation clock and associated values.
|||
|||	  Synopsis
|||
|||	    void DSGetPresentationClock(DSStreamCBPtr streamCBPtr,
|||	      DSClock *dsClock)
|||
|||	  Description
|||
|||	    Returns (in *dsClock) the current value of the streamer's presentation
|||	    clock. The current clock time is calculated as the difference between
|||	    the current Portfolio audio clock and the streamer's presentation clock
|||	    offset (or just copied from the streamer's snapshotted value if the
|||	    stream is stopped).
|||
|||	    Presentation time in the DataStreamer is measured in standard Portfolio
|||	    audio clock ticks. The DataStreamer does not change the rate of the
|||	    audio clock. Since the DataStreamer does not own the audio clock,
|||	    however, external tasks can affect the DataStreamer clock by altering
|||	    the underlying audio clock.
|||
|||	    The DSClock structure contains several values along with the streamTime
|||	    presentation time itself. There's a bool 'running' indicating if the
|||	    stream (and thus the clock) is currently running or stopped. There's a
|||	    branchNumber which allows us to serialize presentation time even when
|||	    taking a branch backwards in the stream or switching to another stream.
|||	    <branchNumber, streamTime> (like <trackNumber, time> on a CD) is a
|||	    monotonically increasing pair. I.e. it never goes backwards. And
|||	    there's the current audioTime and clockOffset values which (if the
|||	    stream is running) were used to calculate the streamTime.
|||
|||	    This procedure works by accessing the data stream thread's data
|||	    structures with a semaphore inter-thread lock.
|||
|||	  Arguments
|||
|||	    streamCBPtr
|||	        Pointer to the stream context block.
|||
|||	    dsClock
|||	        Pointer to the DSClock struct to store the results in.
|||
|||	  Implementation
|||
|||	    Streaming library call.
|||
|||	  Associated Files
|||
|||	    <streaming/datastreamlib.h>, libds.a
|||
|||	  See Also
|||
|||	    <streaming/datastream.h>, DSSetPresentationClock(), DSStartStream(),
|||	    DSStopStream(), DSClockLT(), DSClockLE().
|||
 ******************************************************************************/
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


/******************************************************************************
|||	AUTODOC -public -class Streaming -group Clock -name DSSetPresentationClock
|||	Sets the Data Stream presentation clock and branch number.
|||
|||	  Synopsis
|||
|||	    void DSSetPresentationClock(DSStreamCBPtr streamCBPtr,
|||	        uint32 branchNumber, uint32 streamTime)
|||
|||	  Description
|||
|||	    Sets the Data Stream presentation clock <branchNumber, time>, whether
|||	    the clock is running or stopped. This is meant to be called by a
|||	    subscriber that's currently responsible for driving the clock.
|||
|||	    *** WARNING: The client application should never call this! ***
|||
|||	    Presentation time in the DataStreamer is measured in standard Portfolio
|||	    audio clock ticks. The DataStreamer does not change the rate of the
|||	    audio clock. Since the DataStreamer does not own the audio clock,
|||	    however, external tasks can affect the DataStreamer clock by altering
|||	    the state of the audio clock.
|||
|||	    The branchNumber allows us to serialize presentation time even
|||	    when taking a branch backwards in the stream or switching to another
|||	    stream. <branchNumber, streamTime> (like <trackNumber, time> on a CD)
|||	    is a monotonically increasing pair. I.e. it never goes backwards.
|||
|||	    The streamer increments a delivery-branch-number each time it branches
|||	    in the stream or switches streams. The streamer includes this number in
|||	    each data delivery message to a subscriber. The subscriber that's
|||	    currently driving the clock should use the data's branch number and
|||	    presentation time to set the presentation clock.
|||
|||	    This procedure won't adjust the clock by only 1 or 2 audio ticks, to
|||	    avoid jittering the playback.
|||
|||	    This procedure works by accessing the data stream thread's data
|||	    structures with a semaphore inter-thread lock.
|||
|||	  Caveats
|||
|||	    The client application should never call this!
|||
|||	    This should ONLY be called by the Data Stream thread and by subscriber
|||	    threads that adjust the stream clock using their subscription time
|||	    stamps.
|||
|||	    [In the old days, client applications would call DSSetClock( ) as a
|||	    hack workaround way to initialize the clock in case there wasn't an
|||	    audio subscriber or data on an enabled audio channel. Don't do it
|||	    anymore! The streamer now sets the clock in all cases except no-flush
|||	    branch, where the audio subscriber must do it.]
|||
|||	  Arguments
|||
|||	    streamCBPtr
|||	        Pointer to the stream context block.
|||
|||	    branchNumber
|||	        The new presentation branch number (or "sequence number").
|||
|||	    streamTime
|||	        The new presentation time.
|||
|||	  Implementation
|||
|||	    Streaming library call.
|||
|||	  Associated Files
|||
|||	    <streaming/datastreamlib.h>, libds.a
|||
|||	  See Also
|||
|||	    <streaming/datastream.h>, DSGetPresentationClock(), DSStartStream(),
|||	    DSStopStream(), DSClockLT(), DSClockLE().
|||
 ******************************************************************************/
void	DSSetPresentationClock(DSStreamCBPtr streamCBPtr, uint32 branchNumber,
			uint32 streamTime)
	{
	Err				status;
	uint32			newClockOffset;
	int32			delta;
#define THRESHOLD	2

	if ( streamCBPtr == NULL || streamCBPtr->magicCookie != DS_MAGIC_COOKIE )
		{
		APERR(("DSSetPresentationClock: invalid streamCBPtr\n"));
		return;
		}

	status = LockSemaphore(streamCBPtr->clockSemaphore, SEM_WAIT);
	CHECK_NEG("Write-lock the presentation clock semaphore", status);

		if ( streamCBPtr->fRunning )
			{
			newClockOffset = GetAudioTime() - streamTime;
			delta = newClockOffset - streamCBPtr->clockOffset;
			if ( streamCBPtr->branchNumber != branchNumber ||
					delta > THRESHOLD || delta < -THRESHOLD )
				streamCBPtr->clockOffset = newClockOffset;
			streamCBPtr->branchNumber = branchNumber;
			}
		else
			{
			streamCBPtr->stoppedBranchNumber = branchNumber;
			streamCBPtr->stoppedClock = streamTime;
			}

	status = UnlockSemaphore(streamCBPtr->clockSemaphore);
	CHECK_NEG("Unlock the presentation clock semaphore", status);
	}


/******************************************************************************
|||	AUTODOC -public -class Streaming -group Streaming -name DSIsRunning
|||	Returns TRUE if the Data Stream's presentation clock is currently running.
|||
|||	  Synopsis
|||
|||	    bool DSIsRunning(DSStreamCBPtr streamCBPtr)
|||
|||	  Description
|||
|||	    Returns TRUE if the Data Stream's presentation clock is currently
|||	    running, i.e. if the Data Stream thread is currently delivering data
|||	    to subscribers.
|||
|||	  Arguments
|||
|||	    streamCBPtr
|||	        Pointer to the stream context block.
|||
|||	  Return Value
|||
|||	        TRUE if the Data Stream's presentation clock is currently running.
|||	        FALSE if not.
|||
|||	  Implementation
|||
|||	    Streaming library call.
|||
|||	  Associated Files
|||
|||	    <streaming/datastreamlib.h>, libds.a
|||
|||	  See Also
|||
|||	    DSStartStream(), DSStopStream(), DSGetPresentationClock().
|||
 ******************************************************************************/
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
static void	HandleSubscriberReply(DSStreamCBPtr streamCBPtr,
									SubscriberMsgPtr subMsg,
									Message *messagePtr)
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
		{
		ReleaseChunk(streamCBPtr, messagePtr->msg_Result,
						(DSDataBufPtr)subMsg->privatePtr);
		}

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
	 *    [TBD] Maybe someday also kStreamOpEOF.
	 * Any other reply we received is an error. */
	else if ( subMsg->whatToDo != kStreamOpStop &&
				subMsg->whatToDo != kStreamOpBranch )
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
static void		HandleDataAcqReply(DSStreamCBPtr streamCBPtr,
		DataAcqMsgPtr acqMsg, Message *messagePtr)
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
			streamCBPtr->fClkSetNeedDestTime =
				(options & GOMARKER_NEED_TIME_FLAG) != 0;
			streamCBPtr->clkSetTime = acqMsg->msg.marker.markerTime;
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
static Err DeliverDataChunks(DSStreamCBPtr streamCBPtr, DSDataBufPtr bp)
	{
	Err					status = kDSNoErr;
	StreamChunkPtr		cp = (StreamChunkPtr)bp->curDataPtr;
	uint8				*const endOfBfrPtr =
							(uint8 *)bp->streamData + streamCBPtr->bufDataSize;
	uint8				*const lastPossibleChunkInBfrAddr =
							endOfBfrPtr - STREAMCHUNK_HEADER_SIZE;
	SubscriberMsg		subMsg;

	/* Loop over all remaining chunks in the buffer, but stop if status becomes
	 * positive (stop delivery early) or negative (an error). */
	for ( cp = (StreamChunkPtr)ROUND_TO_LONG((uint8 *)cp);
			(uint8 *)cp <= lastPossibleChunkInBfrAddr && status == kDSNoErr;
			cp = (StreamChunkPtr)ROUND_TO_LONG((uint8 *)cp + cp->streamChunkSize) )
		{
#if defined(DEBUG) && 0
		static uint32		chunkCount;

		++chunkCount;
	#if 0	/* <-- Turn on this switch to print every chunk <type, size>. */
		PRNT(("Chunk #%d: %.4s %lx\n",
			chunkCount, &cp->streamChunkType, cp->streamChunkSize));
	#endif
#endif

		/* Validate the chunk's size field.
		 * NOTE: The odd-looking test for streamChunkSize too big is designed to cope
		 * with wraparound (which happened on a corrupted stream file) and check:
		 *  (uint8 *)cp + cp->streamChunkSize > endOfBfrPtr  */
		if ( cp->streamChunkSize < STREAMCHUNK_HEADER_SIZE
				|| cp->streamChunkSize > endOfBfrPtr - (uint8 *)cp )
			status = kDSBadChunkSizeErr;

		/* If it's a STRM chunk, process it in this thread. */
		else if ( cp->streamChunkType == STREAM_CHUNK_TYPE )
			status = ProcessSTRMChunk(streamCBPtr, bp, cp,
				lastPossibleChunkInBfrAddr);

		/* Else try to deliver the chunk. */
		else
			{
			/* If we need to discover the clkSetTime, use the first deliverable
			 * chunk's delivery time. If we're ready to set the presentation
			 * clock after a branch, do it just before delivering the next chunk. */
			/* [TBD] Should we do this for a chunk that has no subscriber? */
			if ( streamCBPtr->fClkSetPending )
				{
				if ( streamCBPtr->fClkSetNeedDestTime &&
						cp->streamChunkSize >= sizeof(SubsChunkData) &&
						LookupSubscriber(streamCBPtr, cp->streamChunkType)
							!= NULL )
					{
					streamCBPtr->clkSetTime = ((SubsChunkDataPtr)cp)->time;
					streamCBPtr->fClkSetNeedDestTime = FALSE;
					}

				if ( streamCBPtr->fClkSetNext &&
						!streamCBPtr->fClkSetNeedDestTime )
					{
					DSSetPresentationClock(streamCBPtr,
						streamCBPtr->deliveryBranchNumber,
						streamCBPtr->clkSetTime);
						/* [TBD] Subtract a fudge factor to allow for delivery
						 * and decode time? (But don't let it go negative.) */
					streamCBPtr->fClkSetPending = streamCBPtr->fClkSetNext =
						FALSE;
					}
				}

			/* If there is a subscriber for this data type, send it a pointer
			 * to the data then increment the buffer's use count
			 * (decremented when the subscriber returns the chunk). */
			subMsg.whatToDo 				= kStreamOpData;
			subMsg.privatePtr				= (void *)bp;
			subMsg.msg.data.buffer			= (void *)cp;
			subMsg.msg.data.branchNumber	= streamCBPtr->deliveryBranchNumber;

			status = SendSubscrMsgByType(streamCBPtr, &subMsg,
				cp->streamChunkType);

			if ( status == kDSNoErr )
				bp->useCount++;
			else if ( status == kDSSubNotFoundErr )
				status = kDSNoErr;
			} /* not STREAM_CHUNK_TYPE */

		} /* while the buffer has more chunks && status == 0 */

	if ( status < kDSNoErr )
		AbortStream(streamCBPtr);

	return status;
	} /* DeliverDataChunks */


/*******************************************************************************
 * Process a STRM chunk encountered by DeliverDataChunks().
 *
 * RETURNS: A negative Err code, or 0 for no error, or END_THIS_DELIVERY_CYCLE
 * to ask the caller to end this data delivery cycle early.
 *
 * SIDE EFFECTS: For END_THIS_DELIVERY_CYCLE, this sets bp->curDataPtr to point
 * at the next chunk to process in this buffer (asking the caller to put it back
 * at the head of the queue) or to NULL (asking the caller to free this buffer).
 * Also other side effects depending on the chunk subtype, e.g. stopping data
 * delivery.
 *******************************************************************************/
static Err ProcessSTRMChunk(DSStreamCBPtr streamCBPtr, DSDataBufPtr bp,
		StreamChunkPtr cp, uint8 *lastPossibleChunkInBfrAddr)
	{
	Err				status = kDSNoErr;
	HaltChunk1		*const scp = (HaltChunk1 *)cp;

	switch ( scp->subChunkType )
		{
#if HALT_ENABLE
		case HALT_CHUNK_SUBTYPE:
			status = DeliverHaltChunk(streamCBPtr, bp, scp);
			break;
#endif

		case GOTO_CHUNK_SUBTYPE:
			{
			StreamGoToChunk		*const gcp = (StreamGoToChunkPtr)scp;
			uint32				options =
									GOMARKER_NUMBER | GOMARKER_NEED_TIME_FLAG;
			uint32				markerValue = gcp->dest1;

			switch ( gcp->options & GOTO_OPTIONS_BRANCH_TYPE_MASK )
				{
				case GOTO_OPTIONS_ABSOLUTE:
					options = GOMARKER_ABSOLUTE;
					break;

				case GOTO_OPTIONS_MARKER:
					break;

#if 0			/* [TBD] Implement programmed branches. */
				case GOTO_OPTIONS_PROGRAMMED:
					markerValue = programmedBranchTable[markerValue];
					break;
#endif

				default:
					PERR(("Unknown branch type in GOTO chunk.\n"));
					return kDSNoErr;	/* Ignore future branch types. */
				}

			if ( !(gcp->options & GOTO_OPTIONS_FLUSH) )
				options |= GOMARKER_NO_FLUSH_FLAG;

			/* [TBD] If (gcp->options & GOTO_OPTIONS_WAIT) then stop the stream
			 * when the successful go-marker reply arrives from DataAcq. */

			status = InternalGoMarker(streamCBPtr, NULL, markerValue, options,
				gcp->dest2);
			if ( status >= 0 )
				status = END_THIS_DELIVERY_CYCLE;

			break;
			}

		case STOP_CHUNK_SUBTYPE:
			/* NOTE: The STOP chunk contains an options field but no uses for
			 * for it are yet defined. */

#if 0		/* Stop (pause) immediately, without finishing queued data. */
			status = InternalStopStream(streamCBPtr, NULL, SOPT_NOFLUSH);
			if ( status >= 0 )
				status = END_THIS_DELIVERY_CYCLE;

#else		/* Stop here, similar to EOF: Turn on fSTOP, stopping data delivery but
			 * playing out queued data, and stop the stream when all the buffers
			 * return to the filled and free buffer queues.
			 * NOTE: Since the present buffer isn't in the filled or free queues,
			 * we know it's not yet time to stop the stream. */
			streamCBPtr->fSTOP = TRUE;
			status = END_THIS_DELIVERY_CYCLE;
#endif

			break;

#if 0	/* [TBD] Implement the TIME chunk or better yet make the Weaver smarter. */
		case TIME_CHUNK_SUBTYPE:
			status = xxx;
			break;
#endif
		}

	/* If we processed a chunk that requires stopping data delivery, status will
	 * be END_THIS_DELIVERY_CYCLE to ask the caller to end this delivery cycle
	 * and a flag like fBranchingAcq or fSTOP will be set so the main loop won't
	 * deliver data again until further action.
	 *
	 * We also need to set bp->curDataPtr to point at the next chunk to process
	 * in the current buffer (thus asking the caller to put it back at the head
	 * of the queue) or to NULL (thus asking the caller to free this buffer.) */
	if ( status == END_THIS_DELIVERY_CYCLE )
		{
#if SUPPORT_PARTIAL_STREAM_BLOCK_DELIVERY
		cp = (StreamChunkPtr)ROUND_TO_LONG((uint8 *)cp + cp->streamChunkSize);

		/* Skip over a FILL_CHUNK_TYPE chunk. */
		if ( (uint8 *)cp <= lastPossibleChunkInBfrAddr &&
				cp->streamChunkType == FILL_CHUNK_TYPE )
			cp = (StreamChunkPtr)ROUND_TO_LONG((uint8 *)cp +
				cp->streamChunkSize);

		if ( (uint8 *)cp > lastPossibleChunkInBfrAddr )
			cp = NULL;	/* ==> no more useful chunks in this buffer */

		bp->curDataPtr = (uint8 *)cp;
#else
		bp->curDataPtr = NULL;	/* ASSUMES no more useful chunks in this buffer */
		TOUCH(lastPossibleChunkInBfrAddr);	/* avert a compiler warning */
#endif
		}

	return status;
	}


/*******************************************************************************************
 * Deliver the chunk nested within a HALT chunk, and wait for a reply from the
 * subscriber before resuming data delivery.
 *
 * [TBD] We may want to revisit this design later. Halting DataStreamer for a
 * long time may cause resources (i.e. buffers) to not be freed.
 *******************************************************************************************/
#if HALT_ENABLE
static Err DeliverHaltChunk(DSStreamCBPtr streamCBPtr, DSDataBufPtr bp,
		HaltChunk1 *scp)
	{
	SubscriberMsgPtr	subMsg		= streamCBPtr->halted_msg;
	Err					status		= kDSNoErr;
	StreamChunkPtr		cp			= &scp->subscriberChunk;
	DSSubscriberPtr		sp;
	Item				msgItem;
	Message				*replyMsgPtr;

	/* Validate the chunk's size field. */
	if ( cp->streamChunkSize < STREAMCHUNK_HEADER_SIZE
			|| (uint8*)cp + cp->streamChunkSize > (uint8*)scp + scp->chunkSize )
		return kDSBadChunkSizeErr;

	/* If there is a subscriber for this data, pass it a pointer
	 * to the data. Otherwise, no-op. */
	sp = LookupSubscriber(streamCBPtr, cp->streamChunkType);
	if ( sp != NULL )
		{
		if ( subMsg != NULL )
			{
			/* subMsg->whatToDo is kStreamOpData, init'd in CreateHaltChunkMsg */
			subMsg->privatePtr				= (void *)bp;
			subMsg->msg.data.buffer			= (void *)cp;
			subMsg->msg.data.branchNumber	= streamCBPtr->deliveryBranchNumber;

			/* Send the message to a subscriber */
			status = SendMsg(sp->subscriberPort, subMsg->msgItem, subMsg,
							sizeof(SubscriberMsg));

			if ( status == kDSNoErr )
				{
				/* Wait for completion reply */
				msgItem = WaitPort(streamCBPtr->haltChunkReplyPort,
									subMsg->msgItem);

				if ( msgItem == subMsg->msgItem )
					{
					/* Check the result code in the reply message */
					replyMsgPtr = (Message*)LookupItem(msgItem);
					if ( replyMsgPtr == NULL )
						status = kDSAbortErr;
					else
						status = replyMsgPtr->msg_Result;
					}
				else
					status = kDSAbortErr;

				} /* status == kDSNoErr */

			} /* subMsg != NULL */
		else
			status = kDSNoMsgErr;
		} /* chunk's subscriber found */

	return status;
	} /* DeliverHaltChunk */
#endif


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

		default:
			status = kDSInvalidDSRequest;
		}

	/* If the request was completely processed (repliesPending == 0) already,
	 * remove it from the request queue and reply to it now UNLESS it's a
	 * DoRegisterEndOfStream request, to which we'll reply when EOS occurs. */
	if ( streamCBPtr->repliesPending == 0 )
		{
		reqMsg = GetNextDSRequest(streamCBPtr);

		if ( reqMsg->whatToDo != kDSOpWaitEndOfStream )
			status = ReplyToReqMsg(reqMsg, status);
		}

	if ( status < 0 )
		{
		PERR(("HandleDSRequest whatToDo = %ld", reqMsg->whatToDo));
		ERROR_RESULT_STATUS("", status);
		}
	}


/******************************************************************************
 * Utility routine to create a halt message to synchronize the completion of
 * halt chunk. Called by InitializeDSThread(), to do the halt-chunk init work.
 * If this fails, resources it allocated will be released by TearDownStreamCB().
 ******************************************************************************/
#if HALT_ENABLE
static Err	CreateHaltChunkMsg(DSStreamCBPtr streamCBPtr)
	{
	/* Create the message port which is used to synchronize the
	 * completion of halt chunk. */
	streamCBPtr->haltChunkReplyPort = NewMsgPort(NULL);
	if ( streamCBPtr->haltChunkReplyPort < 0 )
		return kDSNoPortErr;

	/* Allocate and initialize a subscriber-message node. */
	streamCBPtr->halted_msg =
		(SubscriberMsgPtr)AllocMem(sizeof(*streamCBPtr->halted_msg),
			MEMTYPE_NORMAL);
	if ( streamCBPtr->halted_msg == NULL )
		return kDSNoMemErr;

	streamCBPtr->halted_msg->whatToDo 	= kStreamOpData;

	/* Create the Message Item. */
	streamCBPtr->halted_msg->msgItem =
		CreateMsgItem(streamCBPtr->haltChunkReplyPort);
	if ( streamCBPtr->halted_msg->msgItem < 0 )
		return kDSNoMsgErr;

	return kDSNoErr;
	} /* CreateHaltChunkMsg */
#endif


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

#if HALT_ENABLE
	if ( CreateHaltChunkMsg(streamCBPtr) < 0 )
		goto BAILOUT;
#endif

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
		if ( signalBits & streamCBPtr->acqReplyPortSignal )
			{
			Item				msgItem;
			Message				*messagePtr;
			DataAcqMsgPtr		acqMsg;

			while ( (msgItem = GetMsg(streamCBPtr->acqReplyPort)) > 0 )
				{
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
