/******************************************************************************
**
**  @(#) datastreamlib.c 96/10/07 1.40
**
******************************************************************************/

#include <audio/audio.h>
#include <kernel/debug.h>
#include <kernel/msgport.h>

#include <streaming/datastreamlib.h>
#include <streaming/msgutils.h>

/* The following switch should be set to non-zero to check the formatting of
 * messages sent to the streamer *BEFORE* they are sent. A debugging aid. */
#define	VALIDATE_REQUEST_CODE	(1 && defined(DEBUG))


/*============================================================================
  ============================================================================
				Procedural Routines
  ============================================================================
  ============================================================================*/

/******************************************************************************
|||	AUTODOC -public -class Streaming -group Startup -name FillPoolWithMsgItems
|||	Initialize a MemPool of GenericMsg entries.
|||	
|||	  Synopsis
|||	
|||	    bool FillPoolWithMsgItems(MemPoolPtr memPool, Item replyPort)
|||	
|||	  Description
|||	
|||	    Given a MemPool of GenericMsg entries, initialize each entry by creating
|||	    a Message Item and storing that in the entry's msgItem field. If it
|||	    fails to initialize ALL the entries, this will clean up after
|||	    itself and return FALSE.
|||	
|||	  Arguments
|||	
|||	    memPool
|||	        The MemPool of GenericMsg entries to initialize.
|||	
|||	    replyPort
|||	        Reply port for each entry's Message Item.
|||	
|||	  Return Value
|||	
|||	    TRUE if successful, FALSE if not.
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
|||	    <streaming/mempool.h>, ForEachFreePoolMember()
|||	
*******************************************************************************/

/* A ForEachFreePoolMember callback routine to initialize a MemPool entry:
 * Create a message Item and store it in the MemPool entry.
 * RETURNS: TRUE if successful, FALSE if anything goes wrong. */
static bool	AllocMsgItemsFunc(void *replyPort, void *poolEntry)
	{
	Item			msgItem;

	((GenericMsgPtr)poolEntry)->msgItem = msgItem =
		CreateMsgItem((Item)replyPort);

	return msgItem >= 0;
	}

/* A ForEachFreePoolMember callback routine to release resources in a
 * MemPool entry, used to clean up when pool initialization failed. */
static bool	DeallocMsgItemsFunc(void *replyPort, void *poolEntry)
	{
	TOUCH(replyPort);
	DeleteItem(((GenericMsgPtr)poolEntry)->msgItem);
	return TRUE;
	}

bool	FillPoolWithMsgItems(MemPoolPtr memPool, Item replyPort)
	{
	bool	success =
		ForEachFreePoolMember(memPool, AllocMsgItemsFunc, (void*)replyPort);
	
	if ( !success )
		ForEachFreePoolMember(memPool, DeallocMsgItemsFunc, (void*)replyPort);
	
	return success;
	}


/*=============================================================================
  =============================================================================
		Routines to send messages to the Data Stream Thread
  =============================================================================
  ============================================================================*/


/******************************************************************************
 * Send a request message to the Data Stream thread and optionally wait for
 * the reply. When this does wait for the reply (a synchronous request), and
 * when there were no Kernel errors with the message send and reply itself, this
 * returns the Data Stream error code from the message reply.
 *******************************************************************************/
static Err	SendRequestToDSThread(Item msgItem, bool fAsync,
		DSStreamCBPtr streamCBPtr, DSRequestMsgPtr reqMsg)
	{
	Err				status;
	Message			*msgPtr;
	Item			replyPortItem;

#if VALIDATE_REQUEST_CODE
	/* Do a range check on the whatToDo field to avoid queuing bogus messages. */
	if ( (reqMsg->whatToDo < 0) || (reqMsg->whatToDo > kDSOpConnect) )
		return kDSInvalidDSRequest;
#endif

	/* Get a pointer to the Message structure. */
	msgPtr = MESSAGE(reqMsg->msgItem = msgItem);
	if ( msgPtr == NULL )
		return kDSBadItemErr;

	/* Get the reply port Item. */
	replyPortItem = msgPtr->msg_ReplyPort;
	if ( replyPortItem == 0 )
		return kDSNoReplyPortErr;

	/* Send the request message to the Data Stream thread */
	status = SendMsg(GetDataStreamMsgPort(streamCBPtr), msgItem, reqMsg,
						sizeof(DSRequestMsg));
	if ( status < 0 )
		return status;

	/* If the caller specifies synchronous operation (!fAsync), then
	 * wait right here for the result of the streamer request. 
	 * NOTE: THERE IS A POTENTIAL FOR DEADLOCK HERE IF A SUBSCRIBER THREAD
	 *	 CALLS ANY STREAMER FUNCTION SYNCHRONOUSLY!!! */
	if ( !fAsync )
		{
		/* Wait for a specific reply message to the one we just sent */
		status = WaitPort(replyPortItem, msgItem);
		if ( status >= 0 )
			status = msgPtr->msg_Result;
		}

	return status;
	}


/******************************************************************************
|||	AUTODOC -public -class Streaming -group Streaming -name DSSubscribe
|||	Add/replace/remove a data stream subscriber.
|||	
|||	  Synopsis
|||	 
|||	    Err DSSubscribe(Item msgItem, DSRequestMsgPtr reqMsg,
|||	        DSStreamCBPtr streamCBPtr, DSDataType dataType,
|||	        Item subscriberPort)
|||	 
|||	  Description
|||	 
|||	    Sends a request to the Data Stream thread to add/replace/remove a
|||	    subscriber. The Data Streamer will start sending data chunks of type
|||	    dataType to subscriberPort (or discarding them, if subscriberPort == 0).
|||	    
|||	    If there's currently a subscriber for dataType, the Data Streamer will
|||	    unsubscribe it and send it a kStreamOpClosing message so it'll know to
|||	    close down. If the new subscriberPort is not 0, the Data Streamer will
|||	    subscribe it and send it a kStreamOpOpening message, unless it can't
|||	    add the subscriber, in which case it'll send it a kStreamOpAbort
|||	    message (to close down) and return an error code.
|||	    
|||	    DSSubscribe() sends a synchronous request (if reqMsg == NULL) or an
|||	    asynchronous request (if reqMsg != NULL) to the Data Streamer to perform
|||	    the operation.
|||	    ("Synchronous" meaning the procedure waits for the Streamer's reply.)
|||	    It returns any error code arising from the message send. For a
|||	    synchronous request with no messaging errors, this returns the Streamer's
|||	    error code (from the message's msg_Result field).
|||	 
|||	  Arguments
|||	 
|||	    msgItem
|||	        Item to use for the request message to the Data Stream thread.
|||	 
|||	    reqMsg
|||	        Pointer to the DSRequestMsg message struct to fill in and send as an
|||	        asynchronous request to the DataStreamer. This struct must remain
|||	        allocated and available to the data stream thread until it replies to
|||	        the message. A value of NULL means use a stack-allocated DSRequestMsg
|||	        and do a synchronous operation.
|||	 
|||	    streamCBPtr
|||	        Pointer to the stream context block.
|||	 
|||	    dataType
|||	        Elementary stream data type to (un)subscribe to.
|||	 
|||	    subscriberPort
|||	        Subscriber's Message port to receive data and control messages.
|||	
|||	  Return Value
|||	
|||	    kDSSubMaxErr
|||	        This is returned by the streamer (in this call if a synchronous
|||	        request; in the message if an asynchronous request) if the streamer
|||	        has no room in its subscriber table for another subscriber.
|||	    
|||	    kDSBadPtrErr
|||	        streamCBPtr is NULL. (The caller doesn't have to check.)
|||	
|||	    Or other Portfolio error code.
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
|||	    <streaming/datastream.h>
|||	
*******************************************************************************/
Err	DSSubscribe(Item msgItem, DSRequestMsgPtr reqMsg, DSStreamCBPtr streamCBPtr, 
		DSDataType dataType, Item subscriberPort)
	{
	DSRequestMsg	localReqMsg;
	bool			fAsync;
	
	if ( reqMsg == NULL )
		{ reqMsg = &localReqMsg; fAsync = FALSE; }
	else
		fAsync = TRUE;

	reqMsg->whatToDo						= kDSOpSubscribe;
	reqMsg->msg.subscribe.dataType			= dataType;
	reqMsg->msg.subscribe.subscriberPort	= subscriberPort;

	return SendRequestToDSThread(msgItem, fAsync, streamCBPtr, reqMsg);
	}


/******************************************************************************
|||	AUTODOC -public -class Streaming -group Streaming -name DSPreRollStream
|||	Start prefilling empty buffers with data.
|||	
|||	  Synopsis
|||	 
|||	    Err DSPreRollStream(Item msgItem, DSRequestMsgPtr reqMsg,
|||	        DSStreamCBPtr streamCBPtr, uint32 asyncBufferCnt)
|||	 
|||	  Description
|||	 
|||	    Asks the Data Streamer to start filling its buffers from the input device.
|||	    You call this before starting stream playback.
|||	    
|||	    The Data Streamer will wait for all but asyncBufferCnt of the buffers to
|||	    return from DataAcq before replying to this request msg. So preroll is
|||	    more than a hint. This is esp. important with MPEG.
|||	    
|||	    We give the client control over the desired preroll buffer level because
|||	    waiting for ALL the buffers to come back before starting playback could
|||	    easily miss a disc rev. Tune it empirically. Start with something like 2.
|||	    
|||	    DSPreRollStream() sends a synchronous request (if reqMsg == NULL) or an
|||	    asynchronous request (if reqMsg != NULL) to the Data Streamer to perform
|||	    the operation.
|||	    ("Synchronous" meaning the procedure waits for the Streamer's reply.)
|||	    It returns any error code arising from the message send. For a
|||	    synchronous request with no messaging errors, this returns the Streamer's
|||	    error code (from the message's msg_Result field).
|||	 
|||	  Arguments
|||	 
|||	    msgItem
|||	        Item to use for the request message to the Data Stream thread.
|||	
|||	    reqMsg
|||	        Pointer to the DSRequestMsg message struct to fill in and send as an
|||	        asynchronous request to the DataStreamer. This struct must remain
|||	        allocated and available to the data stream thread until it replies to
|||	        the message. A value of NULL means use a stack-allocated DSRequestMsg
|||	        and do a synchronous operation.
|||	 
|||	    streamCBPtr
|||	        Pointer to the stream context block.
|||	 
|||	    asyncBufferCnt
|||	        This many buffers may be filled asynchronously, that is, after the
|||	        Streamer replies to the request message. If you start playback after
|||	        all but a couple buffers are filled, there'll be a smaller chance of
|||	        missing a disc rev.
|||	 
|||	  Return Value
|||	    
|||	    kDSBadPtrErr
|||	        streamCBPtr is NULL. (The caller doesn't have to check.)
|||	 
|||	    Or other Portfolio error code.
|||	
|||	  Caveats
|||	
|||	    This procedure expects to be called when the stream is not "running"
|||	    and not at EOF, but it doesn't check.
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
|||	    <streaming/datastream.h>, DSStartStream()
|||	
*******************************************************************************/
Err	DSPreRollStream(Item msgItem, DSRequestMsgPtr reqMsg,
		DSStreamCBPtr streamCBPtr, uint32 asyncBufferCnt)
	{
	DSRequestMsg	localReqMsg;
	bool			fAsync;

	if ( reqMsg == NULL )
		{ reqMsg = &localReqMsg; fAsync = FALSE; }
	else
		fAsync = TRUE;

	reqMsg->whatToDo = kDSOpPreRollStream;
	reqMsg->msg.preroll.asyncBufferCnt = asyncBufferCnt;

	return SendRequestToDSThread(msgItem, fAsync, streamCBPtr, reqMsg);
	}


/******************************************************************************
|||	AUTODOC -public -class Streaming -group Streaming -name DSStartStream
|||	Starts a stream playing.
|||	
|||	  Synopsis
|||	 
|||	    Err DSStartStream(Item msgItem, DSRequestMsgPtr reqMsg,
|||	        DSStreamCBPtr streamCBPtr, uint32 startOptions)
|||	 
|||	  Description
|||	 
|||	    Asks the DataStreamer to start playback and start the presentation clock.
|||	    
|||	    You can pause the stream by calling DSStopStream(..., SOPT_NOFLUSH) and
|||	    resume it by calling DSStartStream(..., SOPT_NOFLUSH). In that case, the
|||	    streamer will pause and resume the presentation clock. If you start the
|||	    stream after flushing it, the streamer will automatically set the
|||	    presentation clock.
|||	    
|||	    DSStartStream() sends a synchronous request (if reqMsg == NULL) or an
|||	    asynchronous request (if reqMsg != NULL) to the Data Streamer to perform
|||	    the operation.
|||	    ("Synchronous" meaning the procedure waits for the Streamer's reply.)
|||	    It returns any error code arising from the message send. For a
|||	    synchronous request with no messaging errors, this returns the Streamer's
|||	    error code (from the message's msg_Result field).
|||	 
|||	  Arguments
|||	 
|||	    msgItem
|||	        Item to use for the request message to the Data Stream thread.
|||	 
|||	    reqMsg
|||	        Pointer to the DSRequestMsg message struct to fill in and send as an
|||	        asynchronous request to the DataStreamer. This struct must remain
|||	        allocated and available to the data stream thread until it replies to
|||	        the message. A value of NULL means use a stack-allocated DSRequestMsg
|||	        and do a synchronous operation.
|||	 
|||	    streamCBPtr
|||	        Pointer to the stream context block.
|||	 
|||	    startOptions
|||	        SOPT_FLUSH to flush the stream buffers before starting, otherwise
|||	        SOPT_NOFLUSH.
|||	
|||	  Return Value
|||	
|||	    kDSWasRunningErr
|||	        The stream was already playing.
|||	    
|||	    kDSBadPtrErr
|||	        streamCBPtr is NULL. (The caller doesn't have to check.)
|||	
|||	    Or other Portfolio error code.
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
|||	    <streaming/datastream.h>, DSPreRollStream(), DSStopStream()
|||	
*******************************************************************************/
Err	DSStartStream(Item msgItem, DSRequestMsgPtr reqMsg,
						DSStreamCBPtr streamCBPtr, 
						uint32 startOptions)
	{
	DSRequestMsg	localReqMsg;
	bool			fAsync;

	if ( reqMsg == NULL )
		{ reqMsg = &localReqMsg; fAsync = FALSE; }
	else
		fAsync = TRUE;

	reqMsg->whatToDo = kDSOpStartStream;
	reqMsg->msg.start.options = startOptions;

	return SendRequestToDSThread(msgItem, fAsync, streamCBPtr, reqMsg);
	}


/******************************************************************************
|||	AUTODOC -public -class Streaming -group Streaming -name DSStopStream
|||	Stops stream playback.
|||	
|||	  Synopsis
|||	 
|||	    Err DSStopStream(Item msgItem, DSRequestMsgPtr reqMsg, DSStreamCBPtr
|||	        streamCBPtr, uint32 stopOptions)
|||	 
|||	  Description
|||	 
|||	    Asks the DataStreamer to stop playback and stop the presentation clock.
|||	    
|||	    You can pause the stream by calling DSStopStream(..., SOPT_NOFLUSH) and
|||	    resume it by calling DSStartStream(..., SOPT_NOFLUSH). In that case, the
|||	    streamer will pause and resume the presentation clock. If you asked to
|||	    flush the stream via these calls, the streamer will automatically reset
|||	    the presentation clock when it starts stream playback.
|||	    
|||	    After pausing, you might want to call DSPreRollStream() to refill all
|||	    stream buffers. But it might not matter much because few buffers will
|||	    return from subscribers while the stream is paused.
|||	    
|||	    If you stop the stream with the SOPT_FLUSH option, the streamer will
|||	    relinquish its use of the stream data buffers (but not the DSDataBuf
|||	    structures that point to the data buffers) so you can reuse the memory.
|||	    
|||	    DSStopStream() sends a synchronous request (if reqMsg == NULL) or an
|||	    asynchronous request (if reqMsg != NULL) to the Data Streamer to perform
|||	    the operation.
|||	    ("Synchronous" meaning the procedure waits for the Streamer's reply.)
|||	    It returns any error code arising from the message send. For a
|||	    synchronous request with no messaging errors, this returns the Streamer's
|||	    error code (from the message's msg_Result field).
|||	 
|||	  Arguments
|||	 
|||	    msgItem
|||	        Item to use for the request message to the Data Stream thread.
|||	 
|||	    reqMsg
|||	        Pointer to the DSRequestMsg message struct to fill in and send as an
|||	        asynchronous request to the DataStreamer. This struct must remain
|||	        allocated and available to the data stream thread until it replies to
|||	        the message. A value of NULL means use a stack-allocated DSRequestMsg
|||	        and do a synchronous operation.
|||	 
|||	    streamCBPtr
|||	        Pointer to the stream context block.
|||	 
|||	    stopOptions
|||	        SOPT_FLUSH to flush the stream buffers before starting, otherwise
|||	        SOPT_NOFLUSH.
|||	
|||	  Return Value
|||	    
|||	    kDSBadPtrErr
|||	        streamCBPtr is NULL. (The caller doesn't have to check.)
|||	
|||	    Or other Portfolio error code.
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
|||	    <streaming/datastream.h>,  DSStartStream()
|||	
*******************************************************************************/
Err	DSStopStream(Item msgItem, DSRequestMsgPtr reqMsg, DSStreamCBPtr streamCBPtr, 
		uint32 stopOptions)
	{
	DSRequestMsg	localReqMsg;
	bool			fAsync;

	if ( reqMsg == NULL )
		{ reqMsg = &localReqMsg; fAsync = FALSE; }
	else
		fAsync = TRUE;

	reqMsg->whatToDo = kDSOpStopStream;
	reqMsg->msg.stop.options = stopOptions;

	return SendRequestToDSThread(msgItem, fAsync, streamCBPtr, reqMsg);
	}


/******************************************************************************
|||	AUTODOC -public -class Streaming -group Streaming -name DSGoMarker
|||	Asks the streamer to branch within the stream.
|||	 
|||	  Synopsis
|||	 
|||	    Err DSGoMarker(Item msgItem, DSRequestMsgPtr reqMsg,
|||	        DSStreamCBPtr streamCBPtr, uint32 markerValue, uint32 options)
|||	 
|||	  Description
|||	 
|||	    Asks the streamer to branch within the stream. Stream positions are
|||	    normally defined by a Marker Table embedded in the stream, but you
|||	    can also branch to an absolute position in the stream.
|||	    
|||	    The options argument specifies the branch type, i.e. how to use the
|||	    markerValue argument to find the branch destination. All branch types
|||	    except GOMARKER_ABSOLUTE require the DataAcq to have received a Marker
|||	    Table chunk from the stream.
|||	    
|||	    When you use the Weaver, you can tell it where in the stream you want
|||	    marker points, in units of Audio ticks of stream time. The Weaver will
|||	    generate a marker table sorted by stream time. Each marker table entry
|||	    gives the stream byte position and stream presentation time of one marker
|||	    point.
|||	    
|||	    With branching, there are issues to consider such as flushing part or all
|||	    of the data flow pipeline and covering CD seek times.
|||	    
|||	    DSGoMarker() normally performs a "flush branch", which means it flushes
|||	    data that's queued at the subscribers in order to reach the destination
|||	    ASAP. But you can bitor GOMARKER_NO_FLUSH_FLAG into the options argument to
|||	    ask the streamer to instead perform a "butt-joint branch" where the
|||	    subscribers play out their queued data before playing the post-branch data.
|||	    A flush branch produces an immediate reaction but does no seek covering.
|||	    A non-flush branch has a delayed response time but does some seek covering.
|||	    
|||	    WARNING: DSGoMarker(GOMARKER_NO_FLUSH_FLAG) is not well defined since
|||	    the request is asynchronous to the stream but it asks the streamer to
|||	    finish playing whatever data has been delivered to subscribers. Since the
|||	    streamer doesn't know when the old data will finish, it can't adjust the
|||	    presentation clock over this discontinuity. If an audio subscriber is
|||	    playing data on its "clock channel", it'll set the clock. Otherwise the
|||	    stream will appear frozen while all subscribers wait for the clock to get
|||	    past the discontinuity.
|||	    
|||	    The Data Stream thread will react to a GOTO chunk in the stream much like
|||	    it reacts to a DSGoMarker() call. However a GOTO chunk is at a well-defined
|||	    place in the stream and it has its own presentation time stamp, so the
|||	    streamer could adjust the clock after the branch. (NOTE: The streamer does
|||	    not yet adjust the clock after a non-flush branch.) A GOTO chunk normally
|||	    does a non-flush branch in order, i.e. plays out the data that preceeds it
|||	    in the stream.
|||	    
|||	    DSGoMarker() sends a synchronous request (if reqMsg == NULL) or an
|||	    asynchronous request (if reqMsg != NULL) to the Data Streamer to perform
|||	    the operation. ("Synchronous" means the procedure waits for the
|||	    Streamer's reply.)
|||	    It returns any error code arising from the message send. For a
|||	    synchronous request with no messaging errors, this returns the Streamer's
|||	    error code (from the message's msg_Result field).
|||	 
|||	  Arguments
|||	 
|||	    msgItem
|||	        Item to use for the request message to the Data Stream thread.
|||	 
|||	    reqMsg
|||	        Pointer to the DSRequestMsg message struct to fill in and send as an
|||	        asynchronous request to the DataStreamer. This struct must remain
|||	        allocated and available to the data stream thread until it replies to
|||	        the message. A value of NULL means use a stack-allocated DSRequestMsg
|||	        and do a synchronous operation.
|||	 
|||	    streamCBPtr
|||	        Pointer to the Stream context block.
|||	 
|||	    markerValue
|||	        This argument supplies a branch amount. Depending on the options
|||	        argument, markerValue could be an absolute or relative count of
|||	        presentation clock ticks, marker numbers, or file bytes.
|||	 
|||	    options
|||	        This argument specifies the branch type (how to interpret the
|||	        markerValue argument) and whether to take a flush branch or a
|||	        butt-joint branch. See "Options", below.
|||	 
|||	  Options -preformatted
|||	
|||	    options choice      use of markerValue
|||	    -----------------   --------------------------------------------------
|||	    GOMARKER_ABSOLUTE   branch to the absolute file byte position markerValue
|||	                        (No marker table needed.) (The destination position
|||	                        no longer needs to be the start of a stream block.)
|||	    
|||	    GOMARKER_FORWARD    find the first marker at or after the current
|||	                        presentation time, count forward markerValue markers
|||	                        in the marker table, and branch to that marker
|||	    
|||	    GOMARKER_BACKWARD   find the first marker at or after the current
|||	                        presentation time, count backward markerValue markers
|||	                        in the marker table, and branch to that marker
|||	    
|||	    GOMARKER_ABS_TIME   branch to the first marker at or after markerValue, in
|||	                        audio ticks
|||	    
|||	    GOMARKER_FORW_TIME  branch to the first marker at or after the current
|||	                        presentation time plus markerValue, in audio ticks
|||	    
|||	    GOMARKER_BACK_TIME  branch to the first marker at or after the current
|||	                        presentation time minus markerValue, in audio ticks
|||	    
|||	    GOMARKER_NAMED      char* name of destination marker [UNIMPLEMENTED]
|||	    
|||	    GOMARKER_NUMBER     branch to marker number markerValue, an index into the
|||	                        marker table ranging from 0 to the number of markers - 1
|||	    
|||	    options flag              meaning
|||	    -----------------         --------------------------------------------
|||	    GOMARKER_NO_FLUSH_FLAG    Don't flush data queued at the subscribers.
|||	                              [See the WARNING under Description, above.]
|||	
|||	  Return Value
|||	    
|||	    kDSBadPtrErr
|||	        streamCBPtr is NULL. (The caller doesn't have to check.)
|||	    
|||	    kDSUnImplemented
|||	        Unimplemented branch type (GOMARKER_NAMED).
|||	    
|||	    kDSEndOfFileErr
|||	        Tried to branch beyond the end of the file via GOMARKER_ABSOLUTE or a
|||	        bum marker.
|||	    
|||	    kDSBranchNotDefined
|||	        Undefined branch destination, in particular, can't branch to a
|||	        marker if the DataAcq hasn't received a marker table from the stream.
|||	    
|||	    kDSRangeErr
|||	        Parameter out of range: undefined options choice or GOMARKER_NUMBER,
|||	        GOMARKER_FORWARD, or GOMARKER_BACKWARD tried to index beyond the marker
|||	        table's bounds. (The first marker is marker number 0.)
|||	
|||	    Or other Portfolio error code.
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
|||	    <streaming/datastream.h>
|||	
*******************************************************************************/
Err	DSGoMarker(Item msgItem, DSRequestMsgPtr reqMsg, DSStreamCBPtr streamCBPtr, 
		uint32 markerValue, uint32 options)
	{
	DSRequestMsg	localReqMsg;
	bool			fAsync;

	if ( reqMsg == NULL )
		{ reqMsg = &localReqMsg; fAsync = FALSE; }
	else
		fAsync = TRUE;

	reqMsg->whatToDo						= kDSOpGoMarker;

	reqMsg->msg.goMarker.markerValue		= markerValue;
	reqMsg->msg.goMarker.options			= options;

	return SendRequestToDSThread(msgItem, fAsync, streamCBPtr, reqMsg);
	}


/******************************************************************************
|||	AUTODOC -public -class Streaming -group Streaming -name DSGetChannel
|||	Read the status of a subscriber's logical channel.
|||	 
|||	  Synopsis
|||	 
|||	    Err DSGetChannel(Item msgItem, DSRequestMsgPtr reqMsg, DSStreamCBPtr
|||	        streamCBPtr, DSDataType streamType, uint32 channelNumber,
|||	        uint32 *channelStatusPtr)
|||	 
|||	  Description
|||	 
|||	    Read the status of a subscriber's logical channel into *channelStatusPtr.
|||	 
|||	    DSGetChannel() sends a synchronous request (if reqMsg == NULL) or an
|||	    asynchronous request (if reqMsg != NULL) to the Data Streamer to perform
|||	    the operation.
|||	    ("Synchronous" meaning the procedure waits for the Streamer's reply.)
|||	    It returns any error code arising from the message send. For a
|||	    synchronous request with no messaging errors, this returns the Streamer's
|||	    error code (from the message's msg_Result field).
|||	 
|||	  Arguments
|||	 
|||	    msgItem
|||	        Item to use for the request message to the Data Stream thread.
|||	 
|||	    reqMsg
|||	        Pointer to the DSRequestMsg message struct to fill in and send as an
|||	        asynchronous request to the DataStreamer. This struct must remain
|||	        allocated and available to the data stream thread until it replies to
|||	        the message. A value of NULL means use a stack-allocated DSRequestMsg
|||	        and do a synchronous operation.
|||	 
|||	    streamCBPtr
|||	        Pointer to the stream context block.
|||	 
|||	    streamType
|||	        Elementary stream data type (subscriber type) to get status for.
|||	 
|||	    channelNumber
|||	        Number of the logical channel to read status for.
|||	 
|||	    channelStatusPtr
|||	        Address to store the channel status bits.
|||	 
|||	  Return Value
|||	
|||	    kDSSubNotFoundErr
|||	        The stream has no subscriber for this data type.
|||	    
|||	    kDSBadPtrErr
|||	        streamCBPtr is NULL. (The caller doesn't have to check.)
|||	
|||	    Or other Portfolio error code.
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
|||	    <streaming/datastream.h>, DSSetChannel().
|||	
*******************************************************************************/
Err	DSGetChannel(Item msgItem, DSRequestMsgPtr reqMsg, DSStreamCBPtr streamCBPtr, 
		DSDataType streamType, uint32 channelNumber, uint32 *channelStatusPtr)
	{
	DSRequestMsg	localReqMsg;
	bool			fAsync;

	if ( reqMsg == NULL )
		{ reqMsg = &localReqMsg; fAsync = FALSE; }
	else
		fAsync = TRUE;

	reqMsg->whatToDo						= kDSOpGetChannel;
	reqMsg->msg.getChannel.streamType		= streamType;
	reqMsg->msg.getChannel.channelNumber	= channelNumber;
	reqMsg->msg.getChannel.channelStatusPtr	= channelStatusPtr;

	return SendRequestToDSThread(msgItem, fAsync, streamCBPtr, reqMsg);
	}


/*******************************************************************************************
|||	AUTODOC -public -class Streaming -group Streaming -name DSSetChannel
|||	Sets the status of a subscriber's logical stream channel.
|||	 
|||	  Synopsis
|||	 
|||	    Err DSSetChannel(Item msgItem, DSRequestMsgPtr reqMsg,
|||	        DSStreamCBPtr streamCBPtr, DSDataType streamType,
|||	        uint32 channelNumber, uint32 channelStatus, uint32 mask)
|||	 
|||	  Description
|||	 
|||	    Sets the status of a subscriber's logical stream channel.
|||	    
|||	    DSSetChannel() sends a synchronous request (if reqMsg == NULL) or an
|||	    asynchronous request (if reqMsg != NULL) to the Data Streamer to perform
|||	    the operation.
|||	    ("Synchronous" meaning the procedure waits for the Streamer's reply.)
|||	    It returns any error code arising from the message send. For a
|||	    synchronous request with no messaging errors, this returns the Streamer's
|||	    error code (from the message's msg_Result field).
|||	 
|||	  Arguments
|||	 
|||	    msgItem
|||	        Item to use for the request message to the Data Stream thread.
|||	 
|||	    reqMsg
|||	        Pointer to the DSRequestMsg message struct to fill in and send as an
|||	        asynchronous request to the DataStreamer. This struct must remain
|||	        allocated and available to the data stream thread until it replies to
|||	        the message. A value of NULL means use a stack-allocated DSRequestMsg
|||	        and do a synchronous operation.
|||	 
|||	    streamCBPtr
|||	        Pointer to the stream context block.
|||	 
|||	    streamType
|||	        Elementary stream data type (subscriber type) to set status for.
|||	 
|||	    channelNumber
|||	        Logical channel number to set status for.
|||	 
|||	    channelStatus
|||	        New status bits for the channel.
|||	 
|||	    mask
|||	        Determines which status bits to set.
|||	 
|||	  Return Value
|||	
|||	    kDSSubNotFoundErr
|||	        The stream has no subscriber for this data type.
|||	    
|||	    kDSBadPtrErr
|||	        streamCBPtr is NULL. (The caller doesn't have to check.)
|||	
|||	    Or other Portfolio error code.
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
|||	    <streaming/datastream.h>, DSGetChannel().
|||	
*******************************************************************************/
Err	DSSetChannel(Item msgItem, DSRequestMsgPtr reqMsg, DSStreamCBPtr streamCBPtr, 
		DSDataType streamType, uint32 channelNumber, uint32 channelStatus,
		uint32 mask)
	{
	DSRequestMsg	localReqMsg;
	bool			fAsync;

	if ( reqMsg == NULL )
		{ reqMsg = &localReqMsg; fAsync = FALSE; }
	else
		fAsync = TRUE;

	reqMsg->whatToDo						= kDSOpSetChannel;
	reqMsg->msg.setChannel.streamType		= streamType;
	reqMsg->msg.setChannel.channelNumber	= channelNumber;
	reqMsg->msg.setChannel.channelStatus	= channelStatus;
	reqMsg->msg.setChannel.mask				= mask;

	return SendRequestToDSThread(msgItem, fAsync, streamCBPtr, reqMsg);
	}


/******************************************************************************
|||	AUTODOC -public -class Streaming -group Streaming -name DSControl
|||	Sends a control message to a data stream subscriber.
|||	
|||	  Synopsis
|||	 
|||	    Err DSControl(Item msgItem, DSRequestMsgPtr reqMsg,
|||	        DSStreamCBPtr streamCBPtr, DSDataType streamType,
|||	        int32 userDefinedOpcode, void *userDefinedArgPtr)
|||	 
|||	  Description
|||	 
|||	    Sends a control message to a data stream subscriber.
|||	 
|||	    DSControl() sends a synchronous request (if reqMsg == NULL) or an
|||	    asynchronous request (if reqMsg != NULL) to the Data Streamer to perform
|||	    the operation.
|||	    ("Synchronous" meaning the procedure waits for the Streamer's reply.)
|||	    It returns any error code arising from the message send. For a
|||	    synchronous request with no messaging errors, this returns the Streamer's
|||	    error code (from the message's msg_Result field).
|||	 
|||	  Arguments
|||	 
|||	    msgItem
|||	        Item to use for the request message to the Data Stream thread.
|||	 
|||	    reqMsg
|||	        Pointer to the DSRequestMsg message struct to fill in and send as an
|||	        asynchronous request to the DataStreamer. This struct must remain
|||	        allocated and available to the data stream thread until it replies to
|||	        the message. A value of NULL means use a stack-allocated DSRequestMsg
|||	        and do a synchronous operation.
|||	 
|||	    streamCBPtr
|||	        Pointer to the stream context block.
|||	 
|||	    streamType
|||	        Elementary stream data type (subscriber type) to control.
|||	 
|||	    userDefinedOpcode
|||	        Defined by the subscriber.
|||	 
|||	    userDefinedArgPtr
|||	        Defined by the subscriber; depends on userDefinedOpcode.
|||	 
|||	  Return Value
|||	
|||	    kDSSubNotFoundErr
|||	        The stream has no subscriber for this data type.
|||	    
|||	    kDSBadPtrErr
|||	        streamCBPtr is NULL. (The caller doesn't have to check.)
|||	
|||	    Or other Portfolio error code.
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
|||	    <streaming/datastream.h>
|||	
 *******************************************************************************************/
Err	DSControl(Item msgItem, DSRequestMsgPtr reqMsg, DSStreamCBPtr streamCBPtr, 
		DSDataType streamType, int32 userDefinedOpcode, void *userDefinedArgPtr)
	{
	DSRequestMsg	localReqMsg;
	bool			fAsync;

	if ( reqMsg == NULL )
		{ reqMsg = &localReqMsg; fAsync = FALSE; }
	else
		fAsync = TRUE;

	reqMsg->whatToDo						= kDSOpControl;
	reqMsg->msg.control.streamType			= streamType;
	reqMsg->msg.control.userDefinedOpcode	= userDefinedOpcode;
	reqMsg->msg.control.userDefinedArgPtr	= userDefinedArgPtr;

	return SendRequestToDSThread(msgItem, fAsync, streamCBPtr, reqMsg);
	}
	
/******************************************************************************
|||	AUTODOC -public -class Streaming -group Streaming -name DSConnect
|||	Connects/replaces/disconnects a data acquisition client to the stream.
|||	
|||	  Synopsis
|||	 
|||	    Err DSConnect(Item msgItem, DSRequestMsgPtr reqMsg,
|||	        DSStreamCBPtr streamCBPtr, Item acquirePort)
|||	 
|||	  Description
|||	 
|||	    Asks the streamer to connect/replace/disconnect a data acquisition client.
|||	    If ithe streamer is currently connected, it will send a disconnect message
|||	    to the current data acquisition client. Then, (if acquirePort != 0) it will
|||	    switch to the new data aquisition client and sends it a connect message.
|||	    
|||	    On connect, the streamer automatically sets the presentation clock. On
|||	    disconnect, the streamer flushes all data queued at the old data acq and at
|||	    subscribers.
|||	    
|||	    The streamer replies to the connect request (which allows a "synchronous"
|||	    DSConnect() call to return), when the changeover of data acqs is effective,
|||	    but it doesn't wait for the old data acq to finish returning flushed
|||	    buffers. The disconnected data acq will return the flushed buffers to the
|||	    streamer as their aborted I/O operations complete.
|||	    
|||	    The proper way to shut down the streamer is to disconnect its data acq
|||	    (this begins decoupling the two threads), then dispose the data acq
|||	    (this waits for the data acq to finish communicating with the streamer),
|||	    then dispose the data streamer.
|||	 
|||	    DSConnect() sends a synchronous request (if reqMsg == NULL) or an
|||	    asynchronous request (if reqMsg != NULL) to the Data Streamer to perform
|||	    the operation. ("Synchronous" means the procedure waits for the Streamer's
|||	    reply.)
|||	    It returns any error code arising from the message send. For a
|||	    synchronous request with no messaging errors, this returns the Streamer's
|||	    error code (from the message's msg_Result field).
|||	 
|||	  Arguments
|||	 
|||	    msgItem
|||	        Item to use for the request message to the Data Stream thread.
|||	 
|||	    reqMsg
|||	        Pointer to the DSRequestMsg message struct to fill in and send as an
|||	        asynchronous request to the DataStreamer. This struct must remain
|||	        allocated and available to the data stream thread until it replies to
|||	        the message. A value of NULL means use a stack-allocated DSRequestMsg
|||	        and do a synchronous operation.
|||	 
|||	    streamCBPtr
|||	        Pointer to the stream context block.
|||	 
|||	    acquirePort
|||	        Message port of the DataAcq to connect to, or 0 to disconnect.
|||	
|||	  Return Value
|||	    
|||	    kDSBadPtrErr
|||	        streamCBPtr is NULL. (The caller doesn't have to check.)
|||	
|||	    Or other Portfolio error code.
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
|||	    <streaming/datastream.h>, NewDataAcq(), DisposeDataStream(),
|||	    DisposeDataAcq()
|||	
*******************************************************************************/
Err	DSConnect(Item msgItem, DSRequestMsgPtr reqMsg, DSStreamCBPtr streamCBPtr, 
		Item acquirePort)
	{
	DSRequestMsg	localReqMsg;
	bool			fAsync;

	if ( reqMsg == NULL )
		{ reqMsg = &localReqMsg; fAsync = FALSE; }
	else
		fAsync = TRUE;

	reqMsg->whatToDo				= kDSOpConnect;
	reqMsg->msg.connect.acquirePort	= acquirePort;

	return SendRequestToDSThread(msgItem, fAsync, streamCBPtr, reqMsg);
	}

/******************************************************************************
|||	AUTODOC -public -class Streaming -group Streaming -name DSWaitEndOfStream
|||	Register for end-of-stream notification.
|||	
|||	  Synopsis
|||	 
|||	    Err DSWaitEndOfStream(Item msgItem, DSRequestMsgPtr reqMsg,
|||	        DSStreamCBPtr streamCBPtr)
|||	 
|||	  Description
|||	 
|||	    This asks the Data Streamer for end-of-stream notification. It works by
|||	    sending a message; the Streamer will reply to this message when it
|||	    reaches EOS. When you get the message reply, check the msg_Result
|||	    field. Afterwards, call DSWaitEndOfStream() again if you want to
|||	    re-register for end-of-stream notification.
|||	 
|||	    A Data Streamer only supports one EOS registrant at a time. So
|||	    DSWaitEndOfStream() will replace the previous registrant, replying to
|||	    the previous registrant with kDSEOSRegistrationErr.
|||	 
|||	    This sends a synchronous request (if reqMsg == NULL) or an asynchronous
|||	    request (if reqMsg != NULL) to the Data Streamer to perform the operation.
|||	    ("Synchronous" means the procedure waits for the Streamer's reply--which
|||	    in this case means it really will wait for the End Of Stream!) Unlike all
|||	    other streamer request messages, a pending end-of-stream notification
|||	    request message doesn't lock out other streamer request messages.
|||	    
|||	    This returns any error code arising from the message send. For a
|||	    synchronous request with no messaging errors, this returns the Streamer's
|||	    error code (from the message's msg_Result field).
|||	 
|||	  Arguments
|||	 
|||	    msgItem
|||	        Item to use for the request message to the Data Stream thread.
|||	 
|||	    reqMsg
|||	        Pointer to the DSRequestMsg message struct to fill in and send as an
|||	        asynchronous request to the DataStreamer. This struct must remain
|||	        allocated and available to the data stream thread until it replies to
|||	        the message. A value of NULL means use a stack-allocated DSRequestMsg
|||	        and do a synchronous operation.
|||	 
|||	    streamCBPtr
|||	        Pointer to the stream context block
|||	 
|||	  Return Value
|||	
|||	    kDSBadPtrErr
|||	        streamCBPtr is NULL. (The caller doesn't have to check.)
|||	
|||	    Or other Portfolio error code.
|||	 
|||	  EOS message reply's msg_Result
|||	
|||	    kDSNoErr == 0
|||	        Normal end of stream playback. This means the streamer has delivered
|||	        the last of the stream's data to subscribers and the subscribers
|||	        have processed it enough to return the chunks to the streamer. The
|||	        subscribers might still be playing out the last decoded data. The
|||	        stream clock is still running. The client might next call
|||	        DSStopStream() to finish stopping or DSGoMarker() to continue
|||	        playback at a different point.
|||	
|||	    kDSSTOPChunk
|||	        The stream stopped at a STOP chunk. You might want to call
|||	        DSPreRollStream(). You can resume playback by calling
|||	        DSStartStream(..., SOPT_NOFLUSH), presumably after re-registering
|||	        for EOS notification.
|||	
|||	    kDSEOSRegistrationErr
|||	        This EOS message registrant was just replaced by a new registrant.
|||	
|||	    kDSAbortErr
|||	        Stream playback was aborted due to an unrecoverable problem.
|||	
|||	    other Portfolio error code
|||	        Most likely an I/O error code from reading the stream file.
|||	
|||	  Caveats
|||	 
|||	    This procedure should have be called DSRegisterEndOfStream( ).
|||	    
|||	    If the stream is already at EOS, the message will wait for the next EOS.
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
|||	    <streaming/datastream.h>, DSStartStream(), DSStopStream()
|||	
*******************************************************************************/
Err	DSWaitEndOfStream(Item msgItem, DSRequestMsgPtr reqMsg,
		DSStreamCBPtr streamCBPtr)
	{
	DSRequestMsg	localReqMsg;
	bool			fAsync;

	if ( reqMsg == NULL )
		{ reqMsg = &localReqMsg; fAsync = FALSE; }
	else
		fAsync = TRUE;

	reqMsg->whatToDo	= kDSOpWaitEndOfStream;

	return SendRequestToDSThread(msgItem, fAsync, streamCBPtr, reqMsg);
	}


/******************************************************************************
|||	AUTODOC -public -class Streaming -group Shutdown -name DisposeDataStream
|||	Shuts down a data stream thread.
|||	
|||	  Synopsis
|||	
|||	    Err DisposeDataStream(Item msgItem, DSStreamCBPtr streamCBPtr)
|||	
|||	  Description
|||	
|||	    Asks the streamer to shut down. The streamer will ask each subscriber to
|||	    shut down and will then shut itself down. Currently, the client is
|||	    responsible for shutting down the Data Acq module.
|||	 
|||	    This sends a synchronous request to the Data Streamer to perform the
|||	    operation. It returns an Err code. If there are no Kernel errors with the
|||	    message send and reply itself, this returns the Streamer error code from
|||	    the message reply.
|||	
|||	  Arguments
|||	
|||	    msgItem
|||	        Item to use for the request message to the Data Stream thread.
|||	
|||	    streamCBPtr
|||	        Pointer to the stream context block.
|||	
|||	  Return Value
|||	    
|||	    kDSBadPtrErr
|||	        streamCBPtr is NULL. (The caller doesn't have to check.)
|||	
|||	    Or other Portfolio error code.
|||	
|||	  Assumes
|||	
|||	    The stream is stopped and the DataStream thread is disconnected from data
|||	    acquisition threads (see DSConnect()) and in a clean state where it has
|||	    returned messages and other resources that were passed to it.
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
|||	    NewDataStream(), DSConnect(), DisposeDataAcq()
|||	
 ******************************************************************************/
Err	DisposeDataStream(Item msgItem, DSStreamCBPtr streamCBPtr)
	{
	DSRequestMsg	reqMsg;

	reqMsg.whatToDo = kDSOpCloseStream;

	return SendRequestToDSThread(msgItem, FALSE,	/* wait for completion */
				streamCBPtr, &reqMsg);
	}


/******************************************************************************
|||	AUTODOC -public -class Streaming -group Clock -name DSClockLT
|||	Returns TRUE if (branchNumber1, streamTime1) < (branchNumber2, streamTime2).
|||	
|||	  Synopsis
|||	 
|||	    bool DSClockLT(uint32 branchNumber1, uint32 streamTime1,
|||	        uint32 branchNumber2, uint32 streamTime2)
|||	 
|||	  Description
|||	 
|||	    Compares two DS Presentation Clock (branchNumber, streamTime) pairs
|||	    and returns TRUE if the first is less than the second.
|||	 
|||	    The streamer uses a (branchNumber, streamTime) pair to serialize time
|||	    even when branching backwards or branching to a different stream file.
|||	
|||	  Arguments
|||	
|||	    branchNumber1
|||	        The first stream presentation clock branch number.
|||	
|||	    streamTime1
|||	        The first stream presentation clock time value.
|||	
|||	    branchNumber2
|||	        The second stream presentation clock branch number.
|||	
|||	    streamTime2
|||	        The second stream presentation clock time value.
|||	
|||	  Return Value
|||	
|||	        (branchNumber1, streamTime1) < (branchNumber2, streamTime2)
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
|||	    <streaming/datastream.h>, DSClockLE(), DSGetPresentationClock(),
|||	    DSSetPresentationClock(), DSStartStream(), DSStopStream().
|||	
 ******************************************************************************/
bool	DSClockLT(uint32 branchNumber1, uint32 streamTime1,
			uint32 branchNumber2, uint32 streamTime2)
	{
	return branchNumber1 < branchNumber2 ||
		(branchNumber1 == branchNumber2 && streamTime1 < streamTime2);
	}


/******************************************************************************
|||	AUTODOC -public -class Streaming -group Clock -name DSClockLE
|||	Returns TRUE if (branchNumber1, streamTime1) <= (branchNumber2, streamTime2).
|||	
|||	  Synopsis
|||	 
|||	    bool DSClockLE(uint32 branchNumber1, uint32 streamTime1,
|||	        uint32 branchNumber2, uint32 streamTime2)
|||	 
|||	  Description
|||	 
|||	    Compares two DS Presentation Clock (branchNumber, streamTime) pairs
|||	    and returns TRUE if the first is less than or equal to the second.
|||	 
|||	    The streamer uses a (branchNumber, streamTime) pair to serialize time
|||	    even when branching backwards or branching to a different stream file.
|||	
|||	  Arguments
|||	
|||	    branchNumber1
|||	        The first stream presentation clock branch number.
|||	
|||	    streamTime1
|||	        The first stream presentation clock time value.
|||	
|||	    branchNumber2
|||	        The second stream presentation clock branch number.
|||	
|||	    streamTime2
|||	        The second stream presentation clock time value.
|||	
|||	  Return Value
|||	
|||	        (branchNumber1, streamTime1) <= (branchNumber2, streamTime2)
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
|||	    <streaming/datastream.h>, DSClockLT(), DSGetPresentationClock(),
|||	    DSSetPresentationClock(), DSStartStream(), DSStopStream().
|||	
 ******************************************************************************/
bool	DSClockLE(uint32 branchNumber1, uint32 streamTime1,
			uint32 branchNumber2, uint32 streamTime2)
	{
	return branchNumber1 < branchNumber2 ||
		(branchNumber1 == branchNumber2 && streamTime1 <= streamTime2);
	}
