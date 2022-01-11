/******************************************************************************
**
**  @(#) dataacq.c 96/11/26 1.8
**  11/19/96  Ian	Major rewrite.  We no longer use dswbblockfile.c routines
**					at all.  Client can now change CDROM speed on the fly via
**					the ResetDataAcq() API.
**  08/17/96  Ian	In DoGetData(), added logic to zero out the buffer contents
**					if EOF is going to occur within that buffer.  Since the
**					blockfile routines will read less than a full buffer of
**					data when EOF occurs within the buffer, pre-zeroing it
**					will clean out stale old data (which was leading to audio
**					blips as the stale data got parsed as if it were fresh).
**	08/16/96  Ian	In FlushPendingAcqIO(), the flush logic was changed.  
**					Instead of doing all the AbortIO() calls then letting the
**					normal IO_DONE signal handling later catch the abort 
**					completions and reply the buffers back to the streamer, 
**					we now do AbortIO/WaitIO/ReplyMsg on each outstanding 
**					buffer in order.  In other words, we abort and reply all 
**					buffers back to the streamer before we reply back the
**					request message that caused us to flush pending I/O.  This
**					makes flushing a pretty much synchronous operation, and
**					avoids certain race conditions in the streamer, such as a 
**					GoMarker followed by a Preroll.  In the old logic, the 
**					preroll would do nothing, because the buffers flushed by
**					the GoMarker call had not arrived back at the streamer yet.
**	08/01/96  Ian	New interface function ResetDataAcq(), a special API for
**					the VideoCD app.  The old DataAcq/VideoCDApp interface was
**					designed so that the app would instantiate a new DataAcq,
**					passing a track number encoded as an ascii string.  Then,
**					the app would obtain a pointer to our private blockfile 
**					structure via a backdoor API, and would directly poke new
**					values into the structure to change our behavior.  The new
**					function accomplishes basically the same thing but is 
**					cleaner in that the outside world doesn't have to know
**					about our private structures and implementation details.
**					More info about this change appears in the DoReset()
**					function comment block.
******************************************************************************/

#include <string.h>
#include <kernel/types.h>
#include <kernel/msgport.h>
#include <kernel/debug.h>
#include <kernel/mem.h>
#include <kernel/io.h>
#include <device/cdrom.h>

#include <video_cd_streaming/datastream.h>
#include <video_cd_streaming/datastreamlib.h>
#include <video_cd_streaming/dsstreamdefs.h>

#include <video_cd_streaming/msgutils.h>
#include <streaming/mempool.h>
#include <streaming/threadhelper.h>
#include <video_cd_streaming/subscriberutils.h>

/************************************
 * Local types and constants
 ************************************/

/* If RETRY_SHIFT_xxx is #define'd, we'll set the drive for 2^RETRY_SHIFT - 1 retries.
 * Even 0 retries can take a while, e.g. if the drive gets out of focus.
 * ISSUE: What are the chances that a retry will help in Form 1? In Form 2?
 * ISSUE: How much do we want to retry vs. press on?
 */

#if 1
  #define ERROR_RETRY_SHIFT_COUNT     	1       /* for Form 2 blocks, 2324 bytes/block */
  #define ERROR_RECOVERY_OPTION			CDROM_DEFAULT_RECOVERY
#else
  #define ERROR_RETRY_SHIFT_COUNT		2
  #define ERROR_RECOVERY_OPTION			CDROM_BEST_ATTEMPT_RECOVERY
#endif

/* A node used to keep track of an I/O request.
 * We keep a pool of these for fast recycling while streaming, reusing the
 * memory node and its IOReq. 
 */

typedef struct AcqIONode {
	MinNode			node;			/* for linking into the ioQueue */
	Item			ioReq;			/* the IOReq Item */
	IOReq			*ioreqItemPtr;	/* the looked-up IOReq Item data ptr */
	DataAcqMsgPtr	dataMsg;		/* get-data request msg from the streamer */
} AcqIONode;

/* Number of ioReq Items to allocate per DataAcq instance, i.e. number of
 * nodes to allocate in the pool of AcqIONodes. */
/* еее shouldn't we have one node per stream buffer?  and thus, shouldn't
 *		the client pass this value to NewDataAcq()? 
 */
		
#define	NUM_IOREQ_ITEMS	16

/* --- Acquisition context, one per open data acquisition thread --- */

typedef struct AcqContext {
	Item				cdDevice;			/* cdrom device item */
	IOInfo				cdIOInfo;			/* IOInfo for issuing cdrom reads */
	CDROMCommandOptions *cdOptions;			/* points to cdIOInfo.ioi_CmdOptions */
	int32				cdBlockSize;		/* size of blocks we read from disc */
	
	int32				cdFileBlockStart;	/* psuedo-file start block */
	int32				cdFileBlockCount;	/* psuedo-file size in blocks */
	int32				cdFileBlockOffset;	/* psuedo-file current offset in blocks */
	
	uint32				offset;				/* file block position offset, in bytes */

	bool				fEOFWasSent;		/* TRUE if we sent an EOF to the streamer */
	bool				fSOFWasSent;		/* TRUE if we sent an SOF to the streamer */
	
	Item				requestPort;		/* message port for data acquisition requests */
	uint32				requestPortSignal;	/* signal associated with requestPort */

	MemPoolPtr			ioReqPool;			/* pool of AcqIONodes to use */
	List				ioQueue;			/* queue of in-progress AcqIONodes */
	
	DataAcqMsgPtr		dataQueueHead;		/* head of requests waiting for AcqIONodes */
	DataAcqMsgPtr		dataQueueTail;		/* tail of requests waiting for AcqIONodes */

} AcqContext, *AcqContextPtr;


/* --- DataAcqCreationArgs ---
 * This structure is used temporarily for communication between the spawning
 * (client) process and the nascent data acq thread.
 *
 * Thread-interlock is handled as follows: NewDataAcq() allocates
 * this struct on the stack, fills it in, and passes it's address as an arg to
 * the streamer thread. The streamer then owns access to it until sending a
 * signal back to the spawning thread (using the first 2 args in the struct).
 * Before sending this signal, the streamer fills in the "output" fields of
 * the struct, thus returning its creation status result code and request msg
 * port Item. After sending this signal, the streamer may no longer touch this
 * memory as NewDataAcq() will deallocate it. 
 */
 
typedef struct DataAcqCreationArgs {
	/* --- input parameters from the client to the new data acq thread --- */
	Item				creatorTask;		/* who to signal when done initializing */
	uint32				creatorSignal;		/* signal to send when done initializing */

	
	/* --- output results from spawing the new data acq thread --- */
	Err					creationStatus;		/* < 0 ==> failure */
	Item				dataAcqMsgPort;		/* data acq msg port; return it to client */
	AcqContextPtr		creationCtx;		/* our dataacq context pointer */
} DataAcqCreationArgs;

/* A fast version of CheckIO, given an IOReq's Item data ptr. */
#ifndef CheckIOPtr
#define CheckIOPtr(ioReqItemPtr)	((ioReqItemPtr)->io_Flags & IO_DONE)
#endif

/******************************************************************************
 * 
 ******************************************************************************/

static Err OpenCDDevice(AcqContextPtr ctx)
{
	memset(&ctx->cdIOInfo, 0, sizeof(ctx->cdIOInfo));
	
	ctx->cdOptions = (CDROMCommandOptions *)&ctx->cdIOInfo.ioi_CmdOptions;
	
	ctx->cdOptions->asLongword 				= 0;
	ctx->cdOptions->asFields.densityCode 	= CDROM_MODE2_XA;
	ctx->cdOptions->asFields.errorRecovery	= ERROR_RECOVERY_OPTION;
	ctx->cdOptions->asFields.retryShift	 	= ERROR_RETRY_SHIFT_COUNT;
	ctx->cdOptions->asFields.speed 			= CDROM_SINGLE_SPEED;
	ctx->cdOptions->asFields.pitch 			= CDROM_PITCH_NORMAL;
	ctx->cdOptions->asFields.blockLength	= BYTES_PER_WHITEBOOK_BLOCK;

	ctx->cdIOInfo.ioi_Command 				= CDROMCMD_READ;
	ctx->cdBlockSize						= BYTES_PER_WHITEBOOK_BLOCK;
	
	if ((ctx->cdDevice = OpenRomAppMedia()) < 0) {
		ERROR_RESULT_STATUS("OpenRomAppMedia() failed", ctx->cdDevice);
		return ctx->cdDevice;
	}

	return 0;
}

/******************************************************************************
 * This utility routine calls ReplyMsg() to return a DataAcqMsg to the thread
 * that sent it (which generally is the Data Stream Thread).
 ******************************************************************************/
 
static Err ReplyToDataAcqMsg(DataAcqMsgPtr dataMsg, Err status)
{
	Err		result;
	
	result = ReplyMsg(dataMsg->msgItem, status, dataMsg, sizeof(DataAcqMsg));
	CHECK_NEG("ReplyToDataAcqMsg", result);
	
	return result;
}

/**********************************************************************
 * Add a data message onto the tail of the queue of
 * get-data messages waiting for available IOReq Items.
 **********************************************************************/
 
static void AddDataAcqMsgToTail(AcqContextPtr ctx, DataAcqMsgPtr dataMsg)
{
	dataMsg->link = NULL;

	if (ctx->dataQueueHead != NULL) {
		ctx->dataQueueTail->link = (void *)dataMsg;
		ctx->dataQueueTail = dataMsg;
	} else {
		ctx->dataQueueHead = dataMsg;
		ctx->dataQueueTail = dataMsg;
	}
}

/**********************************************************************
 * Remove and return the data message at the head of the queue
 * of data messages waiting for available IOReq Items; NULL if none.
 **********************************************************************/
 
static DataAcqMsgPtr GetNextDataAcqMsg(AcqContextPtr ctx)
{
	DataAcqMsgPtr	dataMsg;

	if ((dataMsg = ctx->dataQueueHead) != NULL) {
		ctx->dataQueueHead = (DataAcqMsgPtr) dataMsg->link;
		if (ctx->dataQueueTail == dataMsg) {
			ctx->dataQueueTail = NULL;
		}
	}

	return dataMsg;
}

/**********************************************************************
 * Remove and return the first completed I/O request from the head of the
 * ioQueue, or return NULL if there isn't one.
 **********************************************************************/
 
static AcqIONode *GetFirstCompletedAcqIO(AcqContextPtr ctx)
{
	List		*const ioQueue = &ctx->ioQueue;
	AcqIONode	*const n = (AcqIONode *)FirstNode(ioQueue);
	
	if (IsNode(ioQueue, n) && CheckIOPtr(n->ioreqItemPtr)) {
		RemHead(ioQueue);
		return n;
	}
	
	return NULL;
}

/**********************************************************************
 * Flush pending I/O requests so we can seek or disconnect.
 * This also flushes the dataQueue messages, which might be unnecessary
 * in the seek case.
 **********************************************************************/
 
static void FlushPendingAcqIO(AcqContextPtr ctx)
{
	List				*const ioQueue = &ctx->ioQueue;
	AcqIONode			*n;
	DataAcqMsgPtr		dataMsg, nextDataMsg;
	
	/* Abort the dataQueue messages. Return them to the streamer now.
	 * Note that we must get a message's link field BEFORE returning it to
	 * the Data Stream thread to avoid a race condition. 
	 */
	for ( dataMsg = ctx->dataQueueHead; dataMsg != NULL; dataMsg = nextDataMsg) {
		nextDataMsg = (DataAcqMsgPtr)dataMsg->link;
		ReplyToDataAcqMsg(dataMsg, kDSWasFlushedErr);
	}

	ctx->dataQueueHead = ctx->dataQueueTail = NULL;
	
	/* Abort all the I/O requests in the in-progress ioQueue.
	 * Scan the queue in reverse order to save I/O time--since aborting the
	 * current request can immediately start the next request, we don't want
	 * there to be a next, stale request about to be aborted.  We wait for 
	 * the abort to complete and return the buffers to the streamer immediately.
	 * This replaces the old logic where we let the normal IO_DONE signal 
	 * processing take place for aborted buffers.  The reason is that if we
	 * reply the abort request back to the streamer first, followed by all
	 * the aborted buffers, the streamer actually has time to begin processing
	 * its next request before we can return any buffers to it.  And, if its
	 * next request is a preroll, it won't have any buffers to work with.
	 */
	 
	while ((n = (AcqIONode *)RemTail(ioQueue)) != NULL) {
		AbortIO(n->ioReq);
		WaitIO(n->ioReq);
		ReplyToDataAcqMsg(n->dataMsg, kDSWasFlushedErr);
		ReturnPoolMem(ctx->ioReqPool, n);
	}
	
}

/**********************************************************************
 * Perform any initialization at the time when a connection
 * to a stream happens. Gets called as a result of a call to DSConnect().
 **********************************************************************/
 
static Err DoConnect(AcqContextPtr ctx, DataAcqMsgPtr dataMsg)
{
	TOUCH(ctx);
	TOUCH(dataMsg);
	return kDSNoErr;
}

/***********************************************************************
 * Handle a disconnection request. Flush any pending I/O operations; we'll
 * return their get-data messages when the aborted I/O operations complete.
 * Also un-subscribe to marker tables.
 *
 * [TBD] RACE CONDITION: If a replacement DataAcq subscribes to marker tables
 * before we unsubscribe, we'll end up unsubscribe it instead of us!
 * Since the streamer sends a disconnect msg before a connect msg and DataAcqs
 * do their (un)subscribing in their (dis)connect msg handlers, we'll probably
 * be OK but the DataAcq threads are racing! If the DataAcqs run at a higher
 * priority than the DataStream thread, we'll be ok as long as this code doesn't
 * block before sending the unsubscribe message.
 * Some possible solutions:
 *   (1) Skip unsubscribing if we've received the closing message that the
 *       streamer sends to a marker table subscriber when a new one subscribes,
 *       and process such subscriber requests before data acq requests.
 *   (2) Make DSConnect() send the streamer a disconnect request and then a
 *       connect request, but that presumes the caller wanted a synchronous
 *       operation.
 *   (3) Make DoConnect() inside the streamer wait for the old DataAcq
 *       disconnect request reply before sending the new DataAcq connect reply.
 *   (4) Call a new function "DSUnsubscribe()" here which wouldn't unsubscribe
 *       anyone else.
 *   (5) Make a DataAcq able to switch files, obviating the need to switch
 *       DataAcqs.
 **********************************************************************/
 
static Err DoDisconnect(AcqContextPtr ctx)
{
	FlushPendingAcqIO(ctx);
	return kDSNoErr;
}

/**********************************************************************
 * Seek to a different point in the data stream.
 *
 * The only types of branching we support for Whitebook video cd are:
 *
 * branchOption			branchValue
 * -------------		------------------------------
 * GOMARKER_ABSOLUTE	absolute file block position
 * GOMARKER_RELATIVE	count of blocks to move (forward or back) from current position
 **********************************************************************/
 
static Err DoGoMarker(AcqContextPtr ctx, DataAcqMsgPtr dataMsg)
{
	int32			newOffset;
	uint32			options 		= dataMsg->msg.marker.options;
	uint32			branchOption	= options & GOMARKER_BRANCH_TYPE_MASK;
	int32			branchValue		= (int32)dataMsg->msg.marker.value;

	/* validate branch type and calc new location in psuedo-file. */

	if (branchOption == GOMARKER_ABSOLUTE) {
		newOffset = branchValue;
	} else if (branchOption == GOMARKER_RELATIVE) {
		newOffset =  ctx->cdFileBlockOffset + branchValue;
	} else {
		return kDSUnImplemented;
	}
	
#if 0
	PRNT(("branchOption 0x%X branchValue %d cdFileBlockCount(#%d) new cdFileBlockOffset %d\n",
		  branchOption,
		  branchValue,
		  ctx->cdFileBlockCount,
		  newOffset
		));
#endif

	/* Clip to the file limits. */
	
	if (newOffset < 0) {
		ctx->cdFileBlockOffset = 0;
		ctx->fSOFWasSent = TRUE;
		return kDSSeekPastSOFErr;
	} else if (newOffset >= ctx->cdFileBlockCount) {
		ctx->cdFileBlockOffset = ctx->cdFileBlockCount;
		ctx->fEOFWasSent = TRUE;
		return kDSSeekPastEOFErr;
	}
	
	ctx->cdFileBlockOffset	= newOffset;
	ctx->fSOFWasSent		= FALSE;
	ctx->fEOFWasSent		= FALSE;

	FlushPendingAcqIO(ctx);
	
	return kDSNoErr;
}

/**********************************************************************
 * Reset the psuedo-file access parameters.
 *
 *	Because the streamer is somewhat file-oriented, and we're dealing
 *	with VideoCDs that aren't exactly file-oriented, we implement the
 *	concept of a psuedo-file.  A psuedo-file is just a contiguous set
 *	of disc blocks described by the starting block number and a count
 *	of blocks.  After we've read from disc as many blocks as the count 
 *	indicates, we signal EOF back to the streamer.
 *
 *	The VideoCD app knows a lot more about the layout of the data on
 *	on a VideoCD disc than we can practically accomplish for ourselves.
 *	The Reset API allows the app to direct our management of psuedo-
 *	files.  When the app wants to play a stream from a new area of the
 *	disc (be it a track, an entry, a segment play item, or whatever),
 *	it will disconnect the DataAcq from the streamer, call ResetDataAcq()
 *	passing a block start and count describing the new psuedo-file, then
 *	re-connect the DataAcq to the streamer again.  (The dis/reconnect
 *	sequence resets the streamer parser state, as will as g'teeing that
 *	we won't try to reset the psuedo-file parms while I/O is active.)
 *
 *	Because all MPEG A/V data on a VideoCD is XA Mode2/Form2 sectors,
 *	we can safely open the disc device just once at init time, 
 *	then we can access different MPEG A/V data items just by block 
 *	start and count info.  If we ever needed to support CDDA data as
 *	well as MPEG A/V data, this routine would have to be more complex.
 *
 *	Added 11/15/96 (Ian): The app's desired CDROM speed also comes in
 *	the reset request message now.  If the speed is passed in as zero,
 *	no speed change is made.  If the blockStart/blockCount values are
 *	passed in as zero, they are not changed.  (So speed, file extents,
 *	or both, can be changed with a ResetDataAcq() call.)  File extents
 *	should only be changed while the DataAcq is disconnected from the
 *	streamer, but speed can be changed at any time, even when the stream
 *	is actively running, and takes effect immediately for all subsequent
 *	I/O requests.  Changing the speed on the fly might be useful for
 *	things like ramping down the speed as a strategy for coping with
 *	an excessive error rate, or processing scan/shuttle mode at a 
 *	different speed than normal playback mode.
 **********************************************************************/

static Err DoReset(AcqContextPtr ctx, DataAcqMsgPtr dataMsg)
{
	/* Map out the new psuedo-file */

	if (dataMsg->msg.reset.blockStart > 0 && dataMsg->msg.reset.blockCount > 0) {
		FlushPendingAcqIO(ctx);
		ctx->fSOFWasSent 		= FALSE;
		ctx->fEOFWasSent 		= FALSE;
		ctx->cdFileBlockStart	= dataMsg->msg.reset.blockStart;
		ctx->cdFileBlockCount	= dataMsg->msg.reset.blockCount;
		ctx->cdFileBlockOffset	= 0;
	}
	
	/* change the cdrom speed setting.  if the new speed value passed in isn't	*/
	/* valid, the current speed doesn't get changed.							*/
	
	switch (dataMsg->msg.reset.cdromSpeed) {
	  case 1:
		ctx->cdOptions->asFields.speed = CDROM_SINGLE_SPEED;
		break;
	  case 2:
		ctx->cdOptions->asFields.speed = CDROM_DOUBLE_SPEED;
		break;
	  case 4:
		ctx->cdOptions->asFields.speed = CDROM_4X_SPEED;
		break;
	  default:
	  	/* client doesn't want a speed change */
	  	break;
	}

	return 0;
}

/**********************************************************************
 * Initiate an asynchronous read of the next data block in the stream, or,
 * if there isn't an I/O node available in the ioReqPool, move the given
 * request to the end of the pending data message queue.
 *
 * The dataMsg arg is a get-data request message from the Data Stream thread,
 * either one just now received or one retrieved from the pending data queue
 * when an I/O node becomes available.
 *
 * ASSUMES: If dataMsg was retrieved from the pending data queue, there must
 * be an I/O node available. (Or else dataMsg will get moved to the end of the
 * data queue!)
 *
 * SIDE EFFECT: This procedure takes over responsibility from the caller for
 * replying to dataMsg. If certain errors arise, it replies immediately and
 * returns a negative Err code. Otherwise it queues dataMsg in the I/O queue or
 * the pending data queue, where we'll later get a chance to process it further
 * and eventually reply to it.
 *
 * SIDE EFFECT: This moves the file position forwards.
 **********************************************************************/
 
static Err DoGetData(AcqContextPtr ctx, DataAcqMsgPtr dataMsg)
{
	Err				status;
	AcqIONode *		ioNode;
	uint32			readBlockCount;
	DSDataBufPtr	dsBufPtr = dataMsg->msg.data.dsBufPtr;
	uint32			bufferSize = dataMsg->msg.data.bufferSize;

	/* Check for read past end of file. The first time we hit EOF, return the
	 * EOF Err code to the streamer. After that, return read requests with the
	 * buffer-was-flushed Err code. This simplifies the streamer's EOF handling.
	 *
	 * NOTE: This returns EOF buffers immediately, which can be out of FIFO order
	 * w.r.t. buffers that we're still filling. The DataStreamer is ok with this. */
	 	 
	if (ctx->cdFileBlockOffset >= ctx->cdFileBlockCount) {
		status = (ctx->fEOFWasSent) ? kDSWasFlushedErr : kDSEndOfFileErr;
		ctx->fEOFWasSent = TRUE;
		ReplyToDataAcqMsg(dataMsg, status);
		return status;
	}

	/* Allocate an AcqIONode (with an ioReqItem) for this data buffer from
	 * the pool. If the pool is empty, move the request to the data queue. 
	 */
	 
	if ((ioNode = AllocPoolMem(ctx->ioReqPool)) == NULL) {
		AddDataAcqMsgToTail(ctx, dataMsg);
		return kDSNoErr;
	}

	ioNode->dataMsg		= dataMsg;

	/* If the EOF point is in this buffer, we have to read only as many sectors
	 * as remain in the psuedo-file, and zero out the rest of the sectors in 
	 * the buffer.  If we didn't zero out the unread sectors, the streamer would
	 * try to act on the stale data from a prior read, leading to audio blips.
	 * To keep things simple, we zero the whole buffer then do the partial read.
	 */

	readBlockCount = bufferSize / ctx->cdBlockSize;

	if ((ctx->cdFileBlockOffset + readBlockCount) > ctx->cdFileBlockCount) {
		readBlockCount = ctx->cdFileBlockCount - ctx->cdFileBlockOffset;
		memset(dsBufPtr->streamData, 0, bufferSize);
	}

	/* Initiate the I/O request. */
	
	ctx->cdIOInfo.ioi_Offset			= ctx->cdFileBlockStart + ctx->cdFileBlockOffset;
	ctx->cdIOInfo.ioi_Recv.iob_Buffer	= dsBufPtr->streamData;
	ctx->cdIOInfo.ioi_Recv.iob_Len		= readBlockCount * ctx->cdBlockSize;
	
	status = SendIO(ioNode->ioReq, &ctx->cdIOInfo);

	/* If the read was successfully queued, append the node to
	 * the in-progress ioQueue and update the file position offset.
	 *
	 * Else do error recovery. We won't get an I/O completion signal for this
	 * I/O request so we must reply to the streamer's request message now (or
	 * the request message would be lost). 
	 */
	 
	if (status >= 0) {
		AddTail(&ctx->ioQueue, (Node *)ioNode);
		ctx->cdFileBlockOffset += readBlockCount;
		dsBufPtr->curDataPtr = dsBufPtr->streamData;
	} else {
		ReturnPoolMem(ctx->ioReqPool, ioNode);
		ReplyToDataAcqMsg(dataMsg, status);
	}

	return status;
}

/***********************************************************************
 * This proc is called by CreateItemPool to initialize each AcqIONode in
 * a MemPool of them. It creates an ioReqItem for the pool entry and looks
 * up its IOReq Item structure.
 ***********************************************************************/
 
static bool InitAcqIONode(void *userData, void *poolEntry)
{
	AcqContext *	ctx		= (AcqContext *)userData;
	AcqIONode * 	ioNode	= (AcqIONode *)poolEntry;
	
	ioNode->ioReq = CreateIOReq(NULL, 0, ctx->cdDevice, 0);
	ioNode->ioreqItemPtr = IOREQ(ioNode->ioReq);
	
	return (ioNode->ioReq >= 0);
}


/***********************************************************************
 * Clean up memory acquired by the thread before exiting.
 **********************************************************************/

static void TearDownAcqCtx(AcqContextPtr ctx)
{
	if (ctx != NULL) {
		DeleteMemPool(ctx->ioReqPool);
		FreeMem(ctx, sizeof(*ctx));
	}
}

/***********************************************************************
 * Do one-time initialization for the new data acq thread: Allocate its
 * context structure (instance data), allocate system resources, etc.
 *
 * RETURNS: The new context pointer if successful, or NULL if failed.
 * SIDE EFFECTS: To communicate with the spawning process, this fills in the
 *    output fields of the creationArgs structure and then signals the spawning
 *    process.
 * NOTE: Once we signal the spawning process, the creationArgs structure will
 *    go away out from under us.
 **********************************************************************/
 
static AcqContextPtr InitializeDataAcqThread(DataAcqCreationArgs *creationArgs)
{
	AcqContextPtr	ctx;
	Err				status;

	/* Allocate the zeroed-out data acq context structure. */
	
	if ((ctx = AllocMem(sizeof(*ctx), MEMTYPE_FILL)) == NULL) {
		status = kDSNoMemErr;
		goto BAILOUT;
	}
	
	/* Create the message port where we will accept request messages. */
	
	if ((status = NewMsgPort(&ctx->requestPortSignal)) < 0) {
		goto BAILOUT;
	}
	
	creationArgs->dataAcqMsgPort = ctx->requestPort = status;
	creationArgs->creationCtx	 = ctx;	/* point to our ctx */

	/* open the CD device driver. */
	
	if ((status = OpenCDDevice(ctx)) < 0) {
		goto BAILOUT;
	}
	
	/* Create a MemPool of nodes to track I/O operations and init each member. */
	
	if ((ctx->ioReqPool = CreateMemPool(NUM_IOREQ_ITEMS, sizeof(AcqIONode))) == NULL) {
		status = kDSNoMemErr; 
		goto BAILOUT; 
	}
	
	if (!ForEachFreePoolMember(ctx->ioReqPool, InitAcqIONode, ctx)) {
		status = kDSInitErr;
		goto BAILOUT;
	}
	
	/* Initialize the ioQueue List */
	
	PrepList(&ctx->ioQueue);

	/* Success! */
	
	status = kDSNoErr;

BAILOUT:	/* ASSUMES: status indicates creation status at this point. */

	/* Inform our creator that we've finished with initialization.
	 *
	 * If initialization failed, clean up resources we allocated, letting the
	 * system release system resources. We just need to free up memory we allocated.
	 */
	
	creationArgs->creationStatus = status;	/* return info to the creator task */
	
	SendSignal(creationArgs->creatorTask, creationArgs->creatorSignal);
	
	if (status < 0) {
		TearDownAcqCtx(ctx);
		ctx = NULL;
	}

	return ctx;
}


/**********************************************************************
 *	Data Acquisition
 *	----------------
 *  This thread performs data acquisition services for the data stream thread
 *  (parser). The parser expects to be supplied with a stream of data
 *  buffers in response to its request messages. Data acquisition
 *  is responsible for issuing asynchronous reads to obtain the data,
 *  and returning the buffers back to the stream parser when the I/O completes.
 *
 *	Request message opcodes:
 *	------------------------
 *  kAcqOpConnect
 *	  This call is make as a result of calling DSConnect(). The
 *	  stream parser calls the data acquisition proc to inform it that
 *    the stream is being connected so that it may perform any necessary
 *    initializations. The result code is reported back to the caller
 *    of DSConnect(), so the data acquisition thread can prevent the
 *    stream from connecting should sufficient resources not exist to
 *    actually deliver data to the stream.
 *
 *  kAcqOpDisconnect
 *    This call is made as a result of calling DSHStreamClose() and is
 *    the data acquisition proc's opportunity to release any resources
 *    that may have been allocated by it, such as those allocated at
 *    the time the kAcqOpConnect call was handled.
 *
 *  kAcqOpGetData
 *    This is the data delivery call. The stream parser is requesting
 *    that the specified data buffer be filled with data from the next
 *    available position (sequentially) in the stream. THIS CALL SHOULD
 *    NOT BLOCK!!! The parser calls this whenever a buffer becomes free
 *    in order to keep the data flowing. Later, when the data becomes
 *    available, the data acquisition proc is reponsible for reporting
 *    the availability of the data to the parser.
 *
 *  kAcqOpGoMarker
 *    No assumption is made by the parser about the addressability of
 *    data in a stream (hence, there is no "seek()" interface to this
 *    code). Instead, positions in streams are addressed with an abstract
 *    token called a "marker". The presence of markers and their data
 *    dependent implementation is defined by the data acquisition proc
 *    (i.e., markers may not be implemented in some applications, such
 *    as streamed data from a cable TV network).
 *
 *  kAcqOpExit
 *    Tells the thread to clean up and exit. Sent as a result of a call
 *    to DisposeDataAcq().
 *
 *	kAcqOpReset
 *	  This call resets the disc access parameters to access a new 
 *	  psuedo-file.  A psuedo-file is a section of a Whitebook disc
 *    described by a starting block number and a block count.
 **********************************************************************/
 
static void DataAcqThread(int32 notUsed, DataAcqCreationArgs *creationArgs)
{
	Err				status;
	AcqContextPtr	ctx;
	uint32			signalBits;
	uint32			anyPort;
	bool			fWantToExit = FALSE;
	DataAcqMsgPtr	exitRequestMsg = NULL;

	TOUCH(notUsed);

	/* Call a subroutine to perform all startup initialization. */
	/* We can't touch the creationArgs data after this call! 	*/
	
	if ((ctx = InitializeDataAcqThread(creationArgs)) == NULL) {
		goto THREAD_EXIT;
	}
	
	/* All resources are now allocated and ready to use. Our creator has
	 * been informed that we are ready to accept requests for work. All
	 * that's left to do is dispatch work request messages when they arrive. 
	 */
	
	anyPort = ctx->requestPortSignal | SIGF_IODONE;

	while (TRUE) {
	
		/* Wait for a request message or an I/O completion. */
	
		signalBits = WaitSignal(anyPort);


		/********************************************************
		 * Process I/O completions first as this may result in buffers
		 * freeing up so that additional I/O can be queued (keep the
		 * data flowing).
		 ********************************************************/
		 
		if (signalBits & SIGF_IODONE) {
			AcqIONode		*ioNode;
			DataAcqMsgPtr	dataMsg;
			
			while ((ioNode = GetFirstCompletedAcqIO(ctx)) != NULL) {
				/* Reply to the completed get-data request message. */
				status = ioNode->ioreqItemPtr->io_Error;
				ReplyToDataAcqMsg(ioNode->dataMsg, status);
				
				/* Return the I/O node to the ioReqPool. */
				ReturnPoolMem(ctx->ioReqPool, ioNode);

				/* Since we just freed one IOReqItem node, check to see if
				 * there's a pending data request that we can use it for. */
				if ((dataMsg = GetNextDataAcqMsg(ctx)) != NULL) {
					DoGetData(ctx, dataMsg);
				}
			}
		}


		/********************************************************
		 * Process any new requests for service in the request queue.
		 ********************************************************/
		 
		if (signalBits & ctx->requestPortSignal) {
			Item			msgItem;
			DataAcqMsgPtr	dataMsg;
			
			while ((msgItem = GetMsg(ctx->requestPort)) > 0) {
				dataMsg = (DataAcqMsgPtr)MESSAGE(msgItem)->msg_DataPtr;
				switch (dataMsg->whatToDo) {
				  case kAcqOpConnect:
					status = DoConnect(ctx, dataMsg);
					break;

				  case kAcqOpDisconnect:
					status = DoDisconnect(ctx);
					break;

				  case kAcqOpGoMarker:
					status = DoGoMarker(ctx, dataMsg);
					break;
					
				  case kAcqOpGetData:
					status = DoGetData(ctx, dataMsg);
					break;

				  case kAcqOpExit:
					fWantToExit = TRUE;	/* We'll exit when all pending work is done. */
					status = kDSNoErr;

					/* Make sure we don't get more than one exit request
					 * because this logic is not fancy enough to handle that.
					 * Only one to a customer... */
					if (exitRequestMsg == NULL) {
						exitRequestMsg = dataMsg;
					} else {
						PERR(("DataAcqThread got multiple kAcqOpExit msgs!\n"));
					}
					break;
					
				  case kAcqOpReset:
					status = DoReset(ctx, dataMsg);
					break;
						
				  default:
					status = kDSInvalidDSRequest;
					break;
				}

				/* Reply to the request message unless it's a kind of message
				 * that we handle asynchronously. 
				 */
				 
				if (dataMsg->whatToDo != kAcqOpGetData && dataMsg->whatToDo != kAcqOpExit) {
					ReplyToDataAcqMsg(dataMsg, status);
				}
			}
		}

		/********************************************************
		 * If fWantToExit, see if we're ready to exit, that is, if there are
		 * no pending I/O requests and no outstanding DS request messages.
		 ********************************************************/
		 
		if (fWantToExit && IsListEmpty(&ctx->ioQueue)) {
			ReplyToDataAcqMsg(exitRequestMsg, kDSNoErr);
			goto THREAD_EXIT;
		}
	}

THREAD_EXIT:

	/* Dispose all memory we allocated and clean up any static state.
	 * The OS will automatically reclaim the Items we allocated. */
	
	TearDownAcqCtx(ctx);
}

/*----------------------------------------------------------------------------
 * The following public procedures are called from the client app's context. 
 *--------------------------------------------------------------------------*/

/**********************************************************************
 * Start up the DataAcq thread.
 **********************************************************************/

Item NewDataAcq(char *fileName, AcqContextPtr *dataAcqCtxHdl, int32 deltaPriority)
{
	Err						status;
	uint32					signalBits;
	DataAcqCreationArgs		creationArgs;

	TOUCH(fileName);	/* not used anymore */

	/* Setup creationArgs, including a signal to synchronize with the
	 * completion of the thread's initialization. It will signal us when it
	 * is done initializing itself, successfully or not. */
	status = kDSNoSignalErr;
	creationArgs.creatorTask	= CURRENTTASKITEM;	/* cf. <kernel/kernel.h>, included by <streaming/threadhelper.h> */
	creationArgs.creatorSignal	= AllocSignal(0);
	if (creationArgs.creatorSignal == 0)
		goto CLEANUP;

	creationArgs.creationStatus	= kDSInitErr;
	creationArgs.dataAcqMsgPort	= 0;
	creationArgs.creationCtx	= NULL;
	/* Create the data acquisition thread. */
	status = NewThread(
		(void *)(int32)&DataAcqThread,					/* thread entry point */
		4096, 											/* stack size */
		(int32)CURRENT_TASK_PRIORITY + deltaPriority,	/* priority */
		"DataAcq", 										/* name */
		0, 												/* 1st arg to the thread */
		&creationArgs);									/* 2nd arg to the thread */
	if (status < 0)
		goto CLEANUP;

	/* Wait here while the thread initializes itself. */
	status = kDSSignalErr;
	signalBits = WaitSignal(creationArgs.creatorSignal);
	if (signalBits != creationArgs.creatorSignal)
		goto CLEANUP;

	/* We're done with this signal. So release it. */
	FreeSignal(creationArgs.creatorSignal);
	creationArgs.creatorSignal = 0;

	/* Check the thread's creationStatus. */
	status = creationArgs.creationStatus;
	if (status < 0)
		goto CLEANUP;

	/* now its ok to return the pointer to our acqcontext */
	*dataAcqCtxHdl = creationArgs.creationCtx;			/* return our new private acq context */

	/* Success! Pass the data acq's msg port back to the caller. */
	return creationArgs.dataAcqMsgPort;

CLEANUP:

	/* Something went wrong.
	 * Release any resources we allocated and return the result status.
	 * The new data acq thread will clean up after itself. */
	if (creationArgs.creatorSignal)
		FreeSignal(creationArgs.creatorSignal);
	return status;
}

/**********************************************************************
 * Send a shutdown request message to the DataAcq thread.
 **********************************************************************/
 
Err DisposeDataAcq(Item dataAcqMsgPort)
{
	Err			status;
	DataAcqMsg	acqMsg;
	Item		acqMsgReplyPort;

	if (dataAcqMsgPort < 0)
		return kDSBadItemErr;
	
	/* Create a temporary message Item and a reply port for
	 * communicating with the data acquisition thread. */
	acqMsg.msgItem = -1;	/* for the cleanup code in case NewMsgPort fails */
	status = acqMsgReplyPort = NewMsgPort(NULL);
	FAIL_NEG("DisposeDataAcq NewMsgPort", status);

	status = acqMsg.msgItem = CreateMsgItem(acqMsgReplyPort);
	FAIL_NEG("DisposeDataAcq CreateMsgItem", status);

	/* Send an "exit" request to the data acquisition thread and await a reply. */
	acqMsg.whatToDo = kAcqOpExit;
	status = SendMsg(dataAcqMsgPort, acqMsg.msgItem, &acqMsg,
				sizeof(DataAcqMsg));
	FAIL_NEG("DisposeDataAcq SendMsg", status);
	
	status = WaitPort(acqMsgReplyPort, acqMsg.msgItem);
	FAIL_NEG("DisposeDataAcq WaitPort", status);

FAILED:
	/* Clean up the temporary Message Item and its reply port */
	DeleteMsg(acqMsg.msgItem);
	DeleteMsgPort(acqMsgReplyPort);
	
	return status;
}

/**********************************************************************
 * Send a reset request message to the DataAcq thread.
 **********************************************************************/
 
Err	ResetDataAcq(Item dataAcqMsgPort, uint32 blockStart, uint32 blockCount, uint32 cdromSpeed)
{
	Err			status;
	DataAcqMsg	acqMsg;
	Item		acqMsgReplyPort;

	if (dataAcqMsgPort < 0)
		return kDSBadItemErr;
	
	/* Create a temporary message Item and a reply port for
	 * communicating with the data acquisition thread. */
	acqMsg.msgItem = -1;	/* for the cleanup code in case NewMsgPort fails */
	status = acqMsgReplyPort = NewMsgPort(NULL);
	FAIL_NEG("DisposeDataAcq NewMsgPort", status);

	status = acqMsg.msgItem = CreateMsgItem(acqMsgReplyPort);
	FAIL_NEG("DisposeDataAcq CreateMsgItem", status);

	/* Send an "exit" request to the data acquisition thread and await a reply. */
	acqMsg.whatToDo = kAcqOpReset;
	acqMsg.msg.reset.blockStart = blockStart;
	acqMsg.msg.reset.blockCount = blockCount;
	acqMsg.msg.reset.cdromSpeed = cdromSpeed;
	status = SendMsg(dataAcqMsgPort, acqMsg.msgItem, &acqMsg,
				sizeof(DataAcqMsg));
	FAIL_NEG("DisposeDataAcq SendMsg", status);
	
	status = WaitPort(acqMsgReplyPort, acqMsg.msgItem);
	FAIL_NEG("DisposeDataAcq WaitPort", status);

FAILED:
	/* Clean up the temporary Message Item and its reply port */
	DeleteMsg(acqMsg.msgItem);
	DeleteMsgPort(acqMsgReplyPort);
	
	return status;
}
