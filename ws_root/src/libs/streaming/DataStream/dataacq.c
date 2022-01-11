/******************************************************************************
**
**  @(#) dataacq.c 96/05/08 1.36
**
******************************************************************************/

#include <string.h>       /* for memcpy */
#include <kernel/types.h>
#include <kernel/msgport.h>
#include <kernel/debug.h> /* for print macros: PERR, ERROR_RESULT_STATUS */
#include <kernel/mem.h>

#include <streaming/datastream.h>
#include <streaming/datastreamlib.h>
#include <streaming/dsstreamdefs.h> /* for DSMarkerChunk */

#include <streaming/msgutils.h>		/* NewMsgPort, NewMsgPort */
#include <streaming/mempool.h>
#include <streaming/threadhelper.h>
#include <streaming/dsblockfile.h>
#include <streaming/subscriberutils.h>	/* ReplyToSubscriberMsg */



/************************************
 * Local types and constants
 ************************************/

/* If the following compile-time switch is true, the DataAcq signs up as a
 * subscriber (as well as a data acquisition module) to recieve Marker table
 * chunks from the stream. */
#define	TIME_BASED_BRANCHING		1


/* A node used to keep track of an I/O request.
 * We keep a pool of these for fast recycling while streaming, reusing the
 * memory node and its IOReq. */
typedef struct AcqIONode {
	MinNode			node;			/* for linking into the ioQueue */
	Item			ioReq;			/* the IOReq Item */
	IOReq			*ioreqItemPtr;	/* the looked-up IOReq Item data ptr */
	
	DataAcqMsgPtr	dataMsg;		/* get-data request msg from the streamer */
	bool			fAborted;		/* did we abort this I/O request? */
	} AcqIONode;

/* Number of ioReq Items to allocate per DataAcq instance, i.e. number of
 * nodes to allocate in the pool of AcqIONodes. */
#define	NUM_IOREQ_ITEMS	8


/* The following compile switch enables storing a circular buffer of
 * branch destinations. This should normally be set to zero. */
#define	TRACE_TIME_BASED_BRANCHING		(0 && TIME_BASED_BRANCHING && defined(DEBUG))

#if TRACE_TIME_BASED_BRANCHING
#define	DA_BRANCH_TRACE_MAX				16
uint32	gDABranchTableIndex = 0;
uint32	gDABranchTable[DA_BRANCH_TRACE_MAX];
#endif


/* --- Acquisition context, one per open data acquisition thread --- */
typedef struct AcqContext {
	char			*fileName;			/* input stream file name string */
	uint32			offset;				/* file block position offset, in bytes */
	uint32   		remainder;			/* remainder offset into next block */
	BlockFile 		blockFile;			/* block file object for disc I/O */
	bool			fEOFWasSent;		/* TRUE if we sent an EOF to the streamer */
	uint32			blockFileSize;		/* size of the file, in bytes */
	uint32			streamBlockSize;	/* stream block size, in bytes (must be
										 * a multiple of the file's block size) */
	
	Item			requestPort;		/* message port for data acquisition requests */
	uint32			requestPortSignal;	/* signal associated with requestPort */

	MemPoolPtr		ioReqPool;			/* pool of AcqIONodes to use */
	List			ioQueue;			/* queue of in-progress AcqIONodes */

	DataAcqMsgPtr	dataQueueHead;		/* head of requests waiting for AcqIONodes */
	DataAcqMsgPtr	dataQueueTail;		/* tail of requests waiting for AcqIONodes */

#if TIME_BASED_BRANCHING
	Item			dsReqReplyPort;		/* reply port for requests to streamer */
	uint32			dsReqReplyPortSignal; /* signal for replies to streamer requests */
	int32			dsRepliesPending;		/* count of DS request replies pending */

	uint32			subscriberPortSignal; /* signal for receipt of subscriber messages */
	Item			subscriberPort;		/* message port for our data type */

	DSStreamCBPtr	streamCBPtr;		/* stream control block of stream we are connected to */

	DSMarkerChunkPtr markerChunk;		/* pointer to copy of most recent tranlation table */
#endif
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
 * memory as NewDataAcq() will deallocate it. */
typedef struct DataAcqCreationArgs {
	/* --- input parameters from the client to the new data acq thread --- */
	Item				creatorTask;		/* who to signal when done initializing */
	uint32				creatorSignal;		/* signal to send when done initializing */
	char				*fileName;			/* initial stream file name string */
	
	/* --- output results from spawing the new data acq thread --- */
	Err					creationStatus;		/* < 0 ==> failure */
	Item				dataAcqMsgPort;		/* data acq msg port; return it to client */
	} DataAcqCreationArgs;


/* A fast version of CheckIO, given an IOReq's Item data ptr. */
#ifndef CheckIOPtr
#define CheckIOPtr(ioReqItemPtr)	((ioReqItemPtr)->io_Flags & IO_DONE)
#endif


/****************************/
/* Local routine prototypes */
/****************************/

/* --- Utility routines for queuing pending data requests */
static void	AddDataAcqMsgToTail(AcqContextPtr ctx, DataAcqMsgPtr dataMsg);
static DataAcqMsgPtr GetNextDataAcqMsg(AcqContextPtr ctx);

/* --- Utility routines for managing queued I/O */
static AcqIONode *GetFirstCompletedAcqIO(AcqContextPtr ctx);
static void	FlushPendingAcqIO(AcqContextPtr ctx);

/* --- Routine for mapping markers to file positions */
static Err MapMarkerToOffset(AcqContextPtr ctx, DataAcqMsgPtr dataMsg,
	uint32 *pOffset);

/* --- Routine for opening a new stream file */
static Err	OpenStreamFile(AcqContextPtr ctx);

/* --- Request message handlers --- */
static Err DoConnect(AcqContextPtr ctx, DataAcqMsgPtr dataMsg);
static Err DoDisconnect(AcqContextPtr ctx);
static Err DoGoMarker(AcqContextPtr ctx, DataAcqMsgPtr dataMsg);
static Err DoGetData(AcqContextPtr ctx, DataAcqMsgPtr dataMsg);

/* --- Thread-related routines */
static AcqContextPtr InitializeDataAcqThread(DataAcqCreationArgs *creationArgs);
static void	DataAcqThread(int32 notUsed, DataAcqCreationArgs *creationArgs);


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

	if ( ctx->dataQueueHead != NULL )
		{
		/* Add the new message onto the end of the
		 * existing list of queued messages. */
		ctx->dataQueueTail->link = (void *) dataMsg;
		ctx->dataQueueTail = dataMsg;
		}
	else
		{
		/* Add the message to an empty queue */
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

	if ( (dataMsg = ctx->dataQueueHead) != NULL )
		{
		/* Advance the head pointer to point to the next entry
		 * in the list. */
		ctx->dataQueueHead = (DataAcqMsgPtr) dataMsg->link;

		/* If we are removing the tail entry from the list, set the
		 * tail pointer to NULL. */
		if ( ctx->dataQueueTail == dataMsg )
			ctx->dataQueueTail = NULL;
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
	
	if ( IsNode(ioQueue, n) && CheckIOPtr(n->ioreqItemPtr) )
		{
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
	
	/* Abort all the I/O requests in the in-progress ioQueue.
	 * Scan the queue in reverse order to save I/O time--since aborting the
	 * current request can immediately start the next request, we don't want
	 * there to be a next, stale request about to be aborted. */
	ScanListB(ioQueue, n, AcqIONode)
		{
		AbortIO(n->ioReq);
		n->fAborted = TRUE;
		}
	
	/* Abort the dataQueue messages, too. Return them to the streamer now.
	 * Note that we must get a message's link field BEFORE returning it to
	 * the Data Stream thread to avoid a race condition. */
	for ( dataMsg = ctx->dataQueueHead; dataMsg != NULL; dataMsg = nextDataMsg )
		{
		nextDataMsg = (DataAcqMsgPtr)dataMsg->link;
		ReplyToDataAcqMsg(dataMsg, kDSWasFlushedErr);
		}

	ctx->dataQueueHead = ctx->dataQueueTail = NULL;
	}


/* ----------------------------------------------------------------------------
 * NOTE: The following public procedure is called from the client application's
 * thread. It spawns a DataAcq thread. */

/******************************************************************************
|||	AUTODOC -public -class Streaming -group Startup -name NewDataAcq
|||	Instantiates a new data acquistion thread.
|||	
|||	  Synopsis
|||	
|||	    Item NewDataAcq(char *fileName, int32 deltaPriority)
|||	
|||	  Description
|||	
|||	    Instantiates a new data acquistion thread. This creates the thread,
|||	    allocates all its resources, and waits for initialization to complete.
|||	    Call DisposeDataAcq() to ask the data acq thread to clean up and exit.
|||	    
|||	    This temporarily allocates and frees a signal in the caller's thread.
|||	    Currently, 4096 bytes are allocated for the streamer thread's stack.
|||	
|||	  Arguments
|||	
|||	    fileName
|||	        Name of the stream file to open.
|||	
|||	    deltaPriority
|||	        Task priority for the streamer thread, relative to the caller.
|||	
|||	  Return Value
|||	
|||	    The new DataAcq thread's request message port, or a Portfolio Err code such as:
|||	
|||	    kDSNoMemErr
|||	        Could not allocate enough memory.
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
|||	    DisposeDataAcq()
|||	
 ******************************************************************************/
Item NewDataAcq(char *fileName, int32 deltaPriority)
	{
	Err						status;
	uint32					signalBits;
	DataAcqCreationArgs		creationArgs;

	/* Setup creationArgs, including a signal to synchronize with the
	 * completion of the thread's initialization. It will signal us when it
	 * is done initializing itself, successfully or not. */
	status = kDSNoSignalErr;
	creationArgs.creatorTask	= CURRENTTASKITEM;	/* cf. <kernel/kernel.h>, included by <streaming/threadhelper.h> */
	creationArgs.creatorSignal	= AllocSignal(0);
	if ( creationArgs.creatorSignal == 0 )
		goto CLEANUP;
	creationArgs.fileName		= fileName;
	creationArgs.creationStatus	= kDSInitErr;
	creationArgs.dataAcqMsgPort	= 0;

	/* Create the data acquisition thread. */
	status = NewThread(
		(void *)(int32)&DataAcqThread,					/* thread entry point */
		4096, 											/* stack size */
		(int32)CURRENT_TASK_PRIORITY + deltaPriority,	/* priority */
		"DataAcq", 										/* name */
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

	/* Success! Pass the data acq's msg port back to the caller. */
	return creationArgs.dataAcqMsgPort;

CLEANUP:
	/* Something went wrong.
	 * Release any resources we allocated and return the result status.
	 * The new data acq thread will clean up after itself. */
	if ( creationArgs.creatorSignal )	FreeSignal(creationArgs.creatorSignal);
	return status;
	}


/* ----------------------------------------------------------------------------
 * NOTE: The following public procedure is called from the client application's
 * thread. It sends a message to a DataAcq thread, asking it to exit, and waits
 * for the reply. */

/******************************************************************************
|||	AUTODOC -public -class Streaming -group Shutdown -name DisposeDataAcq
|||	Shuts down a data acquisition thread.
|||	
|||	  Synopsis
|||	
|||	    Err DisposeDataAcq(Item dataAcqMsgPort)
|||	
|||	  Description
|||	
|||	    Asks the data acquisition thread to clean up and shut down.
|||	    
|||	    You should disconnect a data acquisition thread (via DSConnect())
|||	    before disposing it. After disconnecting, the data acquisition thread
|||	    will continue to return the flushed buffers to the streamer as those
|||	    aborted I/O operations complete.
|||	    
|||	    DisposeDataAcq() is a synchronous operation, that is, it waits until
|||	    the data acquisition thread is done communicating with the data stream
|||	    thread and done shutting down.
|||	    
|||	    The proper way to shut down the streamer is to disconnect its data acq
|||	    (this begins decoupling the two threads), then dispose the data acq
|||	    (this waits for the data acq to finish communicating with the streamer),
|||	    then dispose the data streamer.
|||	
|||	  Arguments
|||	
|||	    dataAcqMsgPort
|||	        The data acquisition thread's request message port.
|||	
|||	  Return Value
|||	
|||	    A Portfolio Err code.
|||	
|||	  Assumes
|||	
|||	    The data acq thread has been disconnected from the streamer thread.
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
|||	    NewDataAcq(), DSConnect()
|||	
 ******************************************************************************/
Err DisposeDataAcq(Item dataAcqMsgPort)
	{
	Err			status;
	DataAcqMsg	acqMsg;
	Item		acqMsgReplyPort;

	if ( dataAcqMsgPort < 0 )
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
 * Perform any initialization at the time when a connection
 * to a stream happens. Gets called as a result of a call to DSConnect().
 *
 * NOTE: This allocates a message buffer and message Item to make an
 * asynchronous subscription request to the streamer. These resources are
 * disposedof when the streamer responds to the request.
 * [TBD] Allocate these resources once per DataAcq instance.
 **********************************************************************/
static Err DoConnect(AcqContextPtr ctx, DataAcqMsgPtr dataMsg)
	{
#if TIME_BASED_BRANCHING
	int32				status;
	Item				messageItem;
	DSRequestMsgPtr		dsReqMsgPtr;

	/* If this data acquirer currently owns a marker table, then
	 * dispose of it now. Note that this should not happen because
	 * the connection protocol should guarantee a disconnect message
	 * if we get disconnected. */
	FreeMemTrack(ctx->markerChunk);
	ctx->markerChunk = NULL;

	/* Copy the stream control block pointer into our
	 * context block so that we can communicate with the
	 * appropriate stream. */
	ctx->streamCBPtr = dataMsg->msg.connect.streamCBPtr;
	
	/* Use the stream buffer size as the stream block size.
	 * Check that it's an integral multiple of the file block size. */
	ctx->streamBlockSize = dataMsg->msg.connect.bufferSize;
	if ( ctx->streamBlockSize % GetBlockFileBlockSize(&ctx->blockFile) != 0 )
		return kDSRangeErr;

	/* Create a message buffer for asynch subscription request. */
	dsReqMsgPtr = (DSRequestMsgPtr)AllocMemTrack(sizeof(DSRequestMsg));
	if ( dsReqMsgPtr == NULL )
		return kDSNoMemErr;

	/* Create a message item to send the subscription request. */
	messageItem = CreateMsgItem(ctx->dsReqReplyPort);
	if ( messageItem < 0 )
		{
		FreeMemTrack(dsReqMsgPtr);
		return kDSNoMsgErr;
		}

	/* Subscribe to the Marker table data type.
	 * Unfortunately, we have to make an asynchronous call because the streamer
	 * is obviously busy with a DSConnect() call so a recursive synchronous call
	 * would deadlock. */
	status = DSSubscribe(
			messageItem,					/* msg item */
			dsReqMsgPtr,					/* asynchronous call */
			ctx->streamCBPtr, 				/* stream context block */
			(DSDataType)DATAACQ_CHUNK_TYPE,	/* subscriber data type */
			ctx->subscriberPort);			/* subscriber message port */

	if ( status >= 0 )
		++ctx->dsRepliesPending;
	else
		{
		FreeMemTrack(dsReqMsgPtr);
		DeleteMsg(messageItem);
		}

	return status;

#else
	TOUCH(ctx); TOUCH(dataMsg);
	return kDSNoErr;
#endif
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
#if TIME_BASED_BRANCHING
	int32				status		= kDSNoErr;
	Item				messageItem	= 0;
	DSRequestMsgPtr		dsReqMsgPtr	= NULL;


	FlushPendingAcqIO(ctx);

	/* Un-subscribe to marker tables, unless we're already disconnected. */
	if ( ctx->streamCBPtr == NULL )
		goto BAILOUT;

	/* Create a message buffer for sending an asynchronous un-subscribe msg. */
	dsReqMsgPtr = (DSRequestMsgPtr)AllocMemTrack(sizeof(DSRequestMsg));
	if ( dsReqMsgPtr == NULL )
		{
		status = kDSNoMemErr;
		goto BAILOUT;
		}

	/* Create a message item to send the un-subscribe request */
	messageItem = CreateMsgItem(ctx->dsReqReplyPort);
	if ( messageItem < 0 )
		{
		status = kDSNoMsgErr;
		goto BAILOUT;
		}

	/* Un-subscribe to the Marker table data type. */
	status = DSSubscribe(
			messageItem,					/* msg item */
			dsReqMsgPtr,					/* asynchronous call */
			ctx->streamCBPtr, 				/* stream context block */
			(DSDataType)DATAACQ_CHUNK_TYPE,	/* subscriber data type */
			0);								/* 0 message port ==> unsubscribe */

	if ( status >= 0 )
		++ctx->dsRepliesPending;

BAILOUT:
	/* If no errors, dispose any marker table we've allocated and dissassociate
	 * from the stream. If not, we'll do that in TearDownAcqCtx(). */
	if ( status >= 0 )
		{
		FreeMemTrack(ctx->markerChunk);
		ctx->markerChunk = NULL;
		ctx->streamCBPtr = NULL;

		return status;
		}

	/* Error exit: Clean up any resources that may have been left dangling. */
	FreeMemTrack(dsReqMsgPtr);
	DeleteMsg(messageItem);

	return status;

#else

	FlushPendingAcqIO(ctx);
	return kDSNoErr;
#endif
	}


/***********************************************************************
 * Convert a <branchOption, branchValue> pair into a file position for
 * subsequent read operations.
 *
 * (Hopefully the stream will contain data at the destination suitable for
 * branching, e.g. all data to be presented after a target time T for all
 * elementary streams.)
 *
 * INPUTS: The kAcqOpGoMarker DataAcqMsg and a pointer to memory to store
 * the computed file offset.
 *
 * OUTPUTS: Returns an Err code. In *pOffset, the file offset in bytes.
 *
 * SIDE EFFECTS: If the destination time is known from the marker (i.e. if it's
 * not an absolute branch) and the streamer asked for it (by setting the options
 * flag GOMARKER_NEED_TIME_FLAG), this passes back the destination time in
 * dataMsg->msg.marker.markerTime and clears the GOMARKER_NEED_TIME_FLAG.
 *
 * branchOption			branchValue
 * -------------		------------------------------
 * GOMARKER_ABSOLUTE	absolute destination file byte position [no marker table needed]
 * GOMARKER_FORWARD		count of markers to advance from current time's marker
 * GOMARKER_BACKWARD	count of markers to regress from current time's marker
 * GOMARKER_ABS_TIME	absolute stream time of destination marker
 * GOMARKER_FORW_TIME	stream time to add to current time for destination marker
 * GOMARKER_BACK_TIME	stream time to subtract from current time for destination marker
 * GOMARKER_NAMED		char* name of destination marker [UNIMPLEMENTED]
 * GOMARKER_NUMBER		destination marker number
 *
 * [TBD] Why subtract 1 from GOMARKER_FORWARD's branchValue?
 **********************************************************************/
static Err MapMarkerToOffset(AcqContextPtr ctx, DataAcqMsgPtr dataMsg,
		uint32 *pOffset)
	{
	uint32			options = dataMsg->msg.marker.options;
	uint32			branchOption = options & GOMARKER_BRANCH_TYPE_MASK;
	uint32			branchValue = dataMsg->msg.marker.value;
	const int32		fileSize = ctx->blockFileSize;
#if TIME_BASED_BRANCHING
	int32			streamClockTarget;
	MarkerRecPtr	markerTable, endOfMarkerTable, markerPtr;
#endif

	/* GOMARKER_ABSOLUTE just specifies a destination position (in bytes). It
	 * doesn't need a marker table. But check for seek past EOF. */
	if ( branchOption == GOMARKER_ABSOLUTE )
		{
		if ( branchValue >= fileSize )
			return kDSEndOfFileErr;

		*pOffset = branchValue;
		return kDSNoErr;
		}

#if TIME_BASED_BRANCHING
	/* For any other branchOption make sure we have a marker table, find the
	 * desired marker in the table, and return its contents. */
	if ( (ctx->streamCBPtr == NULL) || (ctx->markerChunk == NULL) )
		return kDSBranchNotDefined;
	markerTable = (MarkerRecPtr)((char *)ctx->markerChunk +
		sizeof(DSMarkerChunk));
	endOfMarkerTable = (MarkerRecPtr)((char *)ctx->markerChunk +
		ctx->markerChunk->chunkSize);

	/* For GOMARKER_NUMBER, just look up a marker in the marker table. */
	if ( branchOption == GOMARKER_NUMBER )
		markerPtr = &markerTable[branchValue];
	else
		{
		/* [TBD] Implement GOMARKER_NAMED. */
	
		/* Range check branchOption. */
		if ( branchOption > GOMARKER_BACK_TIME )
			return kDSRangeErr;
	
		/* GOMARKER_ABS_TIME specifies a target presentation clock time.
		 * GOMARKER_FORW_TIME, GOMARKER_BACK_TIME, GOMARKER_FORWARD, and
		 * GOMARKER_BACKWARD all operate on the current time.
		 * Either way, get a "target" stream clock time value. */
		if ( branchOption == GOMARKER_ABS_TIME )
			streamClockTarget = branchValue;
		else
			{
			DSClock		dsClock;
			
			DSGetPresentationClock(ctx->streamCBPtr, &dsClock);
			streamClockTarget = dsClock.streamTime;
	
			/* For relative-time options, adjust the current stream time
			 * by the amount specified, either forward or backward. */
			if ( branchOption == GOMARKER_FORW_TIME )
				streamClockTarget += branchValue;
	
			else if ( branchOption == GOMARKER_BACK_TIME )
				streamClockTarget -= branchValue;
			}
	
		/* Search the marker table for the first marker at or after
		 * the target time streamClockTarget.
		 * [TBD] OPTIMIZATION: Could do a binary search, or make room after the
		 * marker table for a sentinal value to avoid one test per loop. */
		markerPtr = markerTable;
		while ( (markerPtr < endOfMarkerTable) &&
				(markerPtr->markerTime < streamClockTarget) )
			markerPtr++;

		/* NOTE: It's ok if markerPtr == endOfMarkerTable (just past the end of
		 * the table) because we'll range-check it after adjusting it for
		 * GOMARKER_FORWARD or GOMARKER_BACKWARD. */

		/* Adjust markerPtr for GOMARKER_FORWARD or GOMARKER_BACKWARD.
		 * Adjusting by 0 may branch back to the found marker. */
		if ( branchOption == GOMARKER_FORWARD )
			markerPtr += (int32)branchValue - 1;	/* [TBD] Why subtract 1?!!! */
		else if ( branchOption == GOMARKER_BACKWARD )
			markerPtr -= (int32)branchValue;
		}

	/* Now we have an adjusted markerPtr. Make sure it's still within the
	 * marker table. */
	if ( (markerPtr < markerTable) ||
		 (markerPtr >= endOfMarkerTable) )
		return kDSRangeErr;

	/* Lastly, check that the marker specifies a valid position in the file. */
	if ( markerPtr->markerOffset >= fileSize )
		return kDSEndOfFileErr;

	/* Pass back the marker's file position. */
	*pOffset = markerPtr->markerOffset;

	/* Pass back the marker's stream time, if requested. */
	if ( options & GOMARKER_NEED_TIME_FLAG )
		{
		dataMsg->msg.marker.markerTime = markerPtr->markerTime;
		dataMsg->msg.marker.options = options & ~GOMARKER_NEED_TIME_FLAG;
		}

	return kDSNoErr;
#else

	return kDSRangeErr;	/* other branchOptions require TIME_BASED_BRANCHING */
#endif	/* TIME_BASED_BRANCHING */
	}


/**********************************************************************
 * Seek to a different point in the data stream.
 * NOTE: This functionality is OPTIONAL for data acquisition. For some
 * implementations, such as cable TV streams, seeking may make no sense.
 **********************************************************************/
static Err DoGoMarker(AcqContextPtr ctx, DataAcqMsgPtr dataMsg)
	{
	Err			status;
	uint32		position	= 0;

#if TIME_BASED_BRANCHING
	/* Short-circuit name based branching requests here for now... */
	if ( dataMsg->msg.marker.options == GOMARKER_NAMED )
		return kDSUnImplemented;
#endif

	/* Attempt to map the marker value to a file position and fill in
	 * msg.marker.markerTime. If we get an error, return it immediately. */
	status = MapMarkerToOffset(ctx, dataMsg, &position);
	if ( status != 0 )
		return status;

#if TRACE_TIME_BASED_BRANCHING
	/* Save the current branch destination in the circular buffer
	 * for debugging purposes. */
	gDABranchTable[ gDABranchTableIndex++ ] = position;

	if ( gDABranchTableIndex >= DA_BRANCH_TRACE_MAX )
		gDABranchTableIndex = 0;
#endif

	/* Flush any pending I/O operations so we don't return any stale data. */
	FlushPendingAcqIO(ctx);

	/* Move our file offset to the marker position's block start, and remember
	 * the remainder offset into the first buffer. */
	ctx->offset = position / ctx->streamBlockSize * ctx->streamBlockSize;
	ctx->remainder = position - ctx->offset;

	/* Clear the "we sent an EOF" flag so that the next time an attempt
	 * is made to read past the end, we will actually report an EOF error. */
	ctx->fEOFWasSent = FALSE;

	return kDSNoErr;
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
	Err					status;
	AcqIONode			*ioNode;
	const DSDataBufPtr	dsBufPtr = dataMsg->msg.data.dsBufPtr;
	const uint32		bufferSize = dataMsg->msg.data.bufferSize;

	/* Check for read past end of file. The first time we hit EOF, return the
	 * EOF Err code to the streamer. After that, return read requests with the
	 * buffer-was-flushed Err code. This simplifies the streamer's EOF handling.
	 *
	 * NOTE: This returns EOF buffers immediately, which can be out of FIFO order
	 * w.r.t. buffers that we're still filling. The DataStreamer is ok with this. */
	if ( ctx->offset >= ctx->blockFileSize )
		{
		status = ( ctx->fEOFWasSent ) ? kDSWasFlushedErr : kDSEndOfFileErr;
		ctx->fEOFWasSent = TRUE;
		
		ReplyToDataAcqMsg(dataMsg, status);
		return status;
		}

	/* Allocate an AcqIONode (with an ioReqItem) for this data buffer from
	 * the pool. If the pool is empty, move the request to the data queue. */
	ioNode = AllocPoolMem(ctx->ioReqPool);
	if ( ioNode == NULL )
		{
		AddDataAcqMsgToTail(ctx, dataMsg);
		return kDSNoErr;
		}

	/* Initiate the I/O request. */
	ioNode->dataMsg		= dataMsg;
	ioNode->fAborted	= FALSE;
	status = AsynchReadBlockFile(
		&ctx->blockFile, 			/* stream's block file */
		ioNode->ioReq, 				/* ioReq Item */
		dsBufPtr->streamData,		/* read buffer address */
		bufferSize,					/* number of bytes to read */
		ctx->offset);				/* file position offset */

	/* If the read was successfully queued, append the node to
	 * the in-progress ioQueue and update the file position offset and transfer
	 * the sub-block file position remainder to the DSDataBuf struct.
	 *
	 * Else do error recovery. We won't get an I/O completion signal for this
	 * I/O request so we must reply to the streamer's request message now (or
	 * the request message would be lost). */
	if ( status >= 0 )
		{
		AddTail(&ctx->ioQueue, (Node *)ioNode);
		ctx->offset += bufferSize;
		dsBufPtr->curDataPtr = dsBufPtr->streamData + ctx->remainder;
		ctx->remainder = 0;
		}
	else
		{
		ReturnPoolMem(ctx->ioReqPool, ioNode);
		ReplyToDataAcqMsg(dataMsg, status);
		}

	return status;
	}



/*====================================================================
  ====================================================================
						Data Acquisition thread
  ====================================================================
  ====================================================================*/


/* Subroutine to clean up the DataAcqThread.
 * This lets the system dispose the system Items. */
static void TearDownAcqCtx(AcqContextPtr ctx)
	{
	if ( ctx != NULL )
		{
#if TIME_BASED_BRANCHING
		FreeMemTrack(ctx->markerChunk);
#endif

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

	/* Allocate the data acq context structure (instance data) zeroed
	 * out, and start initializing fields. */
	status = kDSNoMemErr;
	ctx = AllocMem(sizeof(*ctx), MEMTYPE_FILL);
	if ( ctx == NULL )
		goto BAILOUT;
	ctx->fileName		= creationArgs->fileName;

#if TRACE_TIME_BASED_BRANCHING
	memset(&gDABranchTable, ~0, sizeof(gDABranchTable));
#endif

	/* Create the message port where we will accept request messages. */
	status = NewMsgPort(&ctx->requestPortSignal);
	if ( status < 0 )
		goto BAILOUT;
	creationArgs->dataAcqMsgPort = ctx->requestPort = status;

	/* Create a MemPool of nodes to track I/O operations. */
	ctx->ioReqPool = CreateMemPool(NUM_IOREQ_ITEMS, sizeof(AcqIONode));
	if ( ctx->ioReqPool == NULL )
		{ status = kDSNoMemErr; goto BAILOUT; }
	
	/* Open the specified file for reading, and initialize the ioReqPool
	 * with IOReq Items for this file. */
	status = OpenStreamFile(ctx);
	if ( status < 0 )
		goto BAILOUT;
	
	/* Initialize the ioQueue List */
	PrepList(&ctx->ioQueue);

#if TIME_BASED_BRANCHING
	/* Create a message port to receive subscriber messages. While this
	 * may seem weird, it's not exceedingly so. We subscribe to our own
	 * data type (marker tables) and the streamer delivers this stuff to us (even though
	 * we are supplying the raw data). We are not a usual subscriber,
	 * however, and don't have any application-oriented API to implement. */
	status = NewMsgPort(&ctx->subscriberPortSignal);
	if ( status < 0 )
		goto BAILOUT;
	ctx->subscriberPort = status;

	/* Create a message port and signal to receive replies to
	 * asynchronous requests we send to the streamer. */
	status = NewMsgPort(&ctx->dsReqReplyPortSignal);
	if ( status < 0 )
		goto BAILOUT;
	ctx->dsReqReplyPort = status;
#endif

	/* Success! */
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
		TearDownAcqCtx(ctx);
		ctx = NULL;
		}

	return ctx;
	}


/***********************************************************************
 * This proc is called by CreateItemPool to initialize each AcqIONode in
 * a MemPool of them. It creates an ioReqItem for the pool entry and looks
 * up its IOReq Item structure.
 ***********************************************************************/
static bool InitAcqIONode(void *ctx, void *poolEntry)
	{
	Item			ioReq;
	
	((AcqIONode *)poolEntry)->ioReq = ioReq =
		CreateBlockFileIOReq(((AcqContextPtr)ctx)->blockFile.fDevice, 0);
	((AcqIONode *)poolEntry)->ioreqItemPtr = IOREQ(ioReq);
	return ioReq >= 0;
	}


/**********************************************************************
 * Open a file for streaming and initialize a pool of I/O request Item
 * nodes for it.
 *
 * ASSUMES: The context and its ioReqPool have been created.
 * NOTE: This can be used to open a new file if old state has been cleaned
 *    up: flushing outstanding I/O requests, deleting the previous IOReq
 *    Items, closing the previous file, etc.
 **********************************************************************/
static Err OpenStreamFile(AcqContextPtr ctx)
	{
	Err				status;

	/* Clear out any state in the DSBlockFile "object". */
	ctx->blockFile.fDevice	= 0;

	/* Open the specified file for reading. */
	status = OpenBlockFile(ctx->fileName, &ctx->blockFile);
	if ( status < 0 )
		return status;
	
	ctx->offset = ctx->remainder = 0;
	ctx->blockFileSize = GetBlockFileSize(&ctx->blockFile);

	/* Init a pool of I/O request Item nodes for this file. */
	if ( !ForEachFreePoolMember(ctx->ioReqPool, InitAcqIONode, ctx) )
		return kDSInitErr;

	return kDSNoErr;		/* success */
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
	ctx = InitializeDataAcqThread(creationArgs);
	creationArgs = NULL;	/* can't access that memory anymore */
	TOUCH(creationArgs);	/* avoid a compiler warning about unused assignment */
	if ( ctx == NULL )
		goto THREAD_EXIT;

	/* All resources are now allocated and ready to use. Our creator has
	 * been informed that we are ready to accept requests for work. All
	 * that's left to do is dispatch work request messages when they arrive. */
	anyPort = ctx->requestPortSignal | SIGF_IODONE
#if TIME_BASED_BRANCHING
				| ctx->subscriberPortSignal | ctx->dsReqReplyPortSignal
#endif
		;

	while ( TRUE )
		{
		/* Wait for a request message or an I/O completion. */
		signalBits = WaitSignal(anyPort);


		/********************************************************
		 * Process I/O completions first as this may result in buffers
		 * freeing up so that additional I/O can be queued (keep the
		 * data flowing).
		 ********************************************************/
		if ( signalBits & SIGF_IODONE )
			{
			AcqIONode		*ioNode;
			DataAcqMsgPtr	dataMsg;
			
			while ( (ioNode = GetFirstCompletedAcqIO(ctx)) != NULL )
				{
				/* Reply to the completed get-data request message. */
				status = ( ioNode->fAborted ) ?
					kDSWasFlushedErr : ioNode->ioreqItemPtr->io_Error;
				ReplyToDataAcqMsg(ioNode->dataMsg, status);
				
				/* Return the I/O node to the ioReqPool. */
				ReturnPoolMem(ctx->ioReqPool, ioNode);

				/* Since we just freed one IOReqItem node, check to see if
				 * there's a pending data request that we can use it for. */
				if ( (dataMsg = GetNextDataAcqMsg(ctx)) != NULL )
					{
					status = DoGetData(ctx, dataMsg);
					CHECK_NEG("DataAcqThread DoGetData", status);	/* [TBD] error handling */
					}
				}
			}


		/********************************************************
		 * Process any new requests for service in the request queue.
		 ********************************************************/
		if ( signalBits & ctx->requestPortSignal )
			{
			Item			msgItem;
			DataAcqMsgPtr	dataMsg;
			
			while ( (msgItem = GetMsg(ctx->requestPort)) > 0 )
				{
				dataMsg = (DataAcqMsgPtr)MESSAGE(msgItem)->msg_DataPtr;
				switch ( dataMsg->whatToDo )
					{
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
						/* We'll exit when all pending work is done. */
						fWantToExit = TRUE;
						status = kDSNoErr;

						/* Make sure we don't get more than one exit request
						 * because this logic is not fancy enough to handle that.
						 * Only one to a customer... */
						if ( exitRequestMsg == NULL )
							exitRequestMsg = dataMsg;
						else
							PERR(("DataAcqThread got multiple kAcqOpExit msgs!\n"));
						break;
					
					default:
						status = kDSInvalidDSRequest;
					}

				/* Reply to the request message unless it's a kind of message
				 * that we handle asynchronously. */
				if ( dataMsg->whatToDo != kAcqOpGetData
						&& dataMsg->whatToDo != kAcqOpExit )
					ReplyToDataAcqMsg(dataMsg, status);
				}
			}


#if TIME_BASED_BRANCHING
		/********************************************************
		 * Process any replies to our asynchronous requests to the streamer.
		 ********************************************************/
		if ( signalBits & ctx->dsReqReplyPortSignal )
			{
			Item				msgItem;
			DSRequestMsgPtr		dsReqMsgPtr;
			
			while ( (msgItem = GetMsg(ctx->dsReqReplyPort)) > 0 )
				{
				dsReqMsgPtr = (DSRequestMsgPtr)MESSAGE(msgItem)->msg_DataPtr;
				
				/* [TBD] We don't track or pool the buffers or message Items
				 * used to submit asynchronous streamer requests. Because they
				 * happen very infrequently, we just dynamically allocate and
				 * free them. If this proves to cause fragmentation or other
				 * problems, we could use a pool. */
				DeleteMsg(msgItem);
				FreeMemTrack(dsReqMsgPtr);
				--ctx->dsRepliesPending;
				}
			}


		/********************************************************
		 * Process any subscriber request messages from the streamer.
		 ********************************************************/
		if ( signalBits & ctx->subscriberPortSignal )
			{
			Item				subMsgItem;
			SubscriberMsgPtr	subMsg;
			DSMarkerChunkPtr	markerChunkPtr;
			
			while ( (subMsgItem = GetMsg(ctx->subscriberPort)) > 0 )
				{
				subMsg = (SubscriberMsgPtr)MESSAGE(subMsgItem)->msg_DataPtr;
				if ( subMsg->whatToDo == kStreamOpData )
					{
					/* Get a pointer to our data */
					markerChunkPtr = (DSMarkerChunkPtr)subMsg->msg.data.buffer;

					/* For now, handle only the marker table data subtype */
					if ( markerChunkPtr->subChunkType == MARKER_TABLE_SUBTYPE )
						{
						/* If a marker table is currently defined, then
						 * dispose of it to make way for the new one. */
						if ( ctx->markerChunk != NULL )
							FreeMemTrack(ctx->markerChunk);

						/* Allocate a buffer to hold a copy of the new marker table */
						ctx->markerChunk = (DSMarkerChunkPtr)
							AllocMemTrack(markerChunkPtr->chunkSize);

						/* Copy the marker table data from the stream
						 * buffer so as not to constipate the stream
						 * buffering. */
						if ( ctx->markerChunk != NULL )
								memcpy(ctx->markerChunk, markerChunkPtr,
									markerChunkPtr->chunkSize);
						}
					}

				/* Reply to the request we just handled. */
				status = ReplyToSubscriberMsg(subMsg, kDSNoErr);
				CHECK_NEG("DataAcqThread ReplyToSubscriberMsg", status);	/* [TBD] error handling */
				}	/* while ( GetMsg() ) */
			}
#endif

		/********************************************************
		 * If fWantToExit, see if we're ready to exit, that is, if there are
		 * no pending I/O requests and no outstanding DS request messages.
		 ********************************************************/
		if ( fWantToExit && IsListEmpty(&ctx->ioQueue)
#if TIME_BASED_BRANCHING
				&& ctx->dsRepliesPending == 0
#endif
				)
			{
			ReplyToDataAcqMsg(exitRequestMsg, kDSNoErr);
			goto THREAD_EXIT;
			}
		}

THREAD_EXIT:

	/* Dispose all memory we allocated and clean up any static state.
	 * The OS will automatically reclaim the Items we allocated. */
	TearDownAcqCtx(ctx);
	}
