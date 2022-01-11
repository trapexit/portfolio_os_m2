/******************************************************************************
**
**  @(#) datastreamlib.c 96/11/26 1.8
**
******************************************************************************/

#include <audio/audio.h>
#include <kernel/debug.h>
#include <kernel/msgport.h>

#include <video_cd_streaming/datastreamlib.h>
#include <video_cd_streaming/msgutils.h>

/* The following switch should be set to non-zero to check the formatting of
 * messages sent to the streamer *BEFORE* they are sent. A debugging aid. */
#define	VALIDATE_REQUEST_CODE	(1 && defined(DEBUG))


/*============================================================================
  ============================================================================
				Procedural Routines
  ============================================================================
  ============================================================================*/

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
	if ( (reqMsg->whatToDo < 0) || (reqMsg->whatToDo >= kDSOpTotal) )
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

Err	DSWaitStartOfStream(Item msgItem, DSRequestMsgPtr reqMsg,
		DSStreamCBPtr streamCBPtr)
{
	DSRequestMsg	localReqMsg;
	bool			fAsync;

	if ( reqMsg == NULL )
		{ reqMsg = &localReqMsg; fAsync = FALSE; }
	else
		fAsync = TRUE;

	reqMsg->whatToDo	= kDSOpWaitStartOfStream;

	return SendRequestToDSThread(msgItem, fAsync, streamCBPtr, reqMsg);
}

Err	DSWaitTriggerBit(Item msgItem, DSRequestMsgPtr reqMsg,
		DSStreamCBPtr streamCBPtr)
{
	DSRequestMsg	localReqMsg;
	bool			fAsync;

	if ( reqMsg == NULL )
		{ reqMsg = &localReqMsg; fAsync = FALSE; }
	else
		fAsync = TRUE;

	reqMsg->whatToDo	= kDSOpWaitTriggerBit;

	return SendRequestToDSThread(msgItem, fAsync, streamCBPtr, reqMsg);
}

Err	DisposeDataStream(Item msgItem, DSStreamCBPtr streamCBPtr)
	{
	DSRequestMsg	reqMsg;

	reqMsg.whatToDo = kDSOpCloseStream;

	return SendRequestToDSThread(msgItem, FALSE,	/* wait for completion */
				streamCBPtr, &reqMsg);
	}


bool	DSClockLT(uint32 branchNumber1, uint32 streamTime1,
			uint32 branchNumber2, uint32 streamTime2)
	{
	return branchNumber1 < branchNumber2 ||
		(branchNumber1 == branchNumber2 && streamTime1 < streamTime2);
	}


bool	DSClockLE(uint32 branchNumber1, uint32 streamTime1,
			uint32 branchNumber2, uint32 streamTime2)
{
	return branchNumber1 < branchNumber2 ||
		(branchNumber1 == branchNumber2 && streamTime1 <= streamTime2);
}


/*******************************************************************************************
 * Routine to format a "Shuttle" message and send it to the data stream thread.
 * This also puts the MPEG audio and video subscribers into the shuttle modes.
 * ("Skip mode" is synonymous with "Shuttle mode".)
 *******************************************************************************************/
Err DSShuttle( Item msgItem, DSRequestMsgPtr reqMsg, DSStreamCBPtr streamCBPtr,
		int32 rate )
{
	DSRequestMsg	localReqMsg;
	Boolean			fAsync;

	if ( reqMsg == NULL ) {
		reqMsg = &localReqMsg;
		fAsync = false;
	}
	else
		fAsync = true;

	/* a rate of 0 is a NoOp */
	if (rate == 0)
		return kDSNoErr;

	reqMsg->whatToDo			= kDSOpShuttle;
	reqMsg->msg.shuttle.rate	= rate;

	/* if the rate specified is 1, this puts us back in normal play mode, so remove
	** the video subscriber from I frame skip mode.  Otherwise, drop us into skip mode
	** and set the rate to the desired value.
	** Shuttle mode is implemented by:
	**   (1) asking the MPEG video subscriber to go into I-frame-only mode,
	**   (2) asking the MPEG audio subscriber to go into mute mode,
	**   (3) asking the streamer to go into shuttle mode.
	** [TBD] We should just do 3 and the streamer should in turn do 1 and 2! That's
	**   because the streamer is running at a higher priority, also because it might be
	**   able to back out if part of the operation fails.
	*/
	return SendRequestToDSThread(msgItem, fAsync, streamCBPtr, reqMsg);
}

/*******************************************************************************************
 * Routine to format a "Play" message and send it to the data stream thread.
 * This also puts the MPEG audio and video subscriber out of shuttle modes.
 *******************************************************************************************/
Err DSPlay( Item msgItem, DSRequestMsgPtr reqMsg, DSStreamCBPtr streamCBPtr )
{
	DSRequestMsg	localReqMsg;
	Boolean			fAsync;

	if ( reqMsg == NULL ) {
		reqMsg = &localReqMsg;
		fAsync = false;
	}
	else
		fAsync = true;

	reqMsg->whatToDo	= kDSOpPlay;

	return SendRequestToDSThread(msgItem, fAsync, streamCBPtr, reqMsg);
}

/*******************************************************************************************
 * Routine to inform the streamer that a single buffer has been displayed.
 *******************************************************************************************/
Err DSFrameDisplayed(Item msgItem, DSRequestMsgPtr reqMsg, DSStreamCBPtr streamCBPtr)
{
	DSRequestMsg	localReqMsg;
	Boolean			fAsync;

	if ( reqMsg == NULL ) {
		reqMsg = &localReqMsg;
		fAsync = false;
	}
	else
		fAsync = true;

	reqMsg->whatToDo	= kDSOpFrameDisplayed;

	return SendRequestToDSThread(msgItem, fAsync, streamCBPtr, reqMsg);
}

/*******************************************************************************************
 * Routine to request either normal-res or high-res stills.
 *******************************************************************************************/
Err DSDoHighResStills(Item msgItem, DSRequestMsgPtr reqMsg, DSStreamCBPtr streamCBPtr, Boolean doHighRes)
{
	DSRequestMsg	localReqMsg;
	Boolean			fAsync;

	if ( reqMsg == NULL ) {
		reqMsg = &localReqMsg;
		fAsync = false;
	}
	else
		fAsync = true;

	reqMsg->whatToDo					= kDSOpDoHighResStills;
	reqMsg->msg.doHighResStills.option	= doHighRes;
	return SendRequestToDSThread(msgItem, fAsync, streamCBPtr, reqMsg);
}

/*******************************************************************************************
 * Routine to format a "GetMPEGTime" message and send it to the data stream thread.
 * By asking the stream thread, we get a reliable answer. The streamer can atomically test
 * if there's a current video PTS and a reference SCR, and so on.
 *
 * OUTPUTS:
 *	*timePtr			-- The stream time, as an MPEG timestamp. (A NULL ptr is not ok).
 *	*fromVideoPTSPtr	-- Indicates if the result value derives from the current, running
 *							video PTS vs. a substitute such as a branch-in-progress
 *							destination. (A NULL ptr is not ok.)
 *******************************************************************************************/
int32 DSGetElapsedMPEGTime(Item msgItem, DSRequestMsgPtr reqMsg, DSStreamCBPtr streamCBPtr,
		uint32 *timePtr, Boolean *fromVideoPTSPtr)
{
	DSRequestMsg	localReqMsg;
	Boolean			fAsync;

	if ( timePtr == NULL || fromVideoPTSPtr == NULL )
		return kDSRangeErr;

	if ( reqMsg == NULL )
		{
		reqMsg = &localReqMsg;
		fAsync = false;
		}
	else
		fAsync = true;

	reqMsg->whatToDo						= kDSOpGetMPEGTime;
	reqMsg->msg.getMPEGTime.timePtr			= timePtr;
	reqMsg->msg.getMPEGTime.fromVideoPTSPtr	= fromVideoPTSPtr;

	return SendRequestToDSThread(msgItem, fAsync, streamCBPtr, reqMsg);
}


/*******************************************************************************************
 * Return current elapsed time into the stream, in seconds. This is based on the latest video
 * PTS or a substitute (e.g. branch-in-progress destination) if the video doesn't currently
 * have a valid PTS. Returns a negative error code if we can't read the elapsed time.
 *
 * OUTPUTS:
 *	*secsPtr			-- The stream time, in seconds. (A NULL ptr is not ok).
 *	*fromVideoPTSPtr	-- Indicates if the result value derives from the current, running
 *							video PTS vs. a substitute such as a branch-in-progress
 *							destination. (A NULL ptr is ok for this argument.)
 *
 * [TBD] POSSIBLE OPTIMIZATION: This could memoize the last result to avoid the multiply
 * some of the time.
 *******************************************************************************************/
int32	DSGetElapsedSecs(Item msgItem, DSRequestMsgPtr reqMsg, DSStreamCBPtr streamCBPtr,
			int32 *secsPtr, Boolean *fromVideoPTSPtr)
	{
	uint32			mpegTime;
	int32			result;
	Boolean			localBoolean;

	TOUCH(secsPtr);

	if ( fromVideoPTSPtr == NULL )
		fromVideoPTSPtr = &localBoolean;

	/* Get the current stream time into mpegTime. */
	result = DSGetElapsedMPEGTime(msgItem, reqMsg, streamCBPtr, &mpegTime, fromVideoPTSPtr);
	if ( result < 0 )
		return result;

	/* 	*secsPtr = DSMPEGTimestampToSeconds(mpegTime); */

	return kDSNoErr;
	}

