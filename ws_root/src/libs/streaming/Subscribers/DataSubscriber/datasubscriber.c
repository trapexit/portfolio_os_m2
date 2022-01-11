/****************************************************************************
**
**  @(#) datasubscriber.c 96/11/27 1.9
**
*****************************************************************************/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <audio/audio.h>
#include <kernel/debug.h>
#include <kernel/mem.h>
#include <kernel/task.h>
#include <kernel/semaphore.h>


#include <streaming/dserror.h>
#include <streaming/datastreamlib.h>
#include <streaming/dsstreamdefs.h> 		/* for SUBS_CHUNK_COMMON constants */
#include <streaming/msgutils.h>
#include <streaming/mempool.h>
#include <streaming/threadhelper.h>
#include <streaming/subscribertraceutils.h>

#include "datachannels.h"
#include <streaming/datasubscriber.h>


#ifdef TRACE_DATA_SUBSCRIBER
/* Declare the actual trace buffer.  Let it be global in case other
 * modules in the subscriber want to write into it.
 */
TraceBuffer		gDataTraceBuffer;

TraceBufferPtr	gDataTraceBufPtr	= &gDataTraceBuffer;
#endif

/**************************************************************************
 * structure used to communicate with the thread at init time.  NewDataSubscriber()
 *  allocates the structure on the stack, fills in the first three fields, and
 *  passed it's address to the subscriber thread as it is created.  the subscriber
 *  is safe to fill in the return value fields because NewDataSubscriber() waits
 *  for a signal from the subscriber thread before proceeding.
 */
typedef struct _DataSubInitArgs
{
	/* arguments passed from NewDataSubscriber() to the new thread */
	Item			creatorTask;			/* who to signal when done initializing */
	uint32			creatorSignal;			/* signal used to synchronize initialization */
	DSStreamCBPtr	streamCBPtr;			/* streamer context */

	/* data returned from new subscriber thread to NewDataSubscriber() */
	PvtDataContextPtr	dataCtx;				/* data subscriber context ptr */
	int32			status;					/* result code for initialization */
}DataSubInitArgs, *DataSubInitArgsPtr;



/*============================= private interfaces ==========================================*/

/***********************************************************************
 * data subscriber utility functions
 ***********************************************************************/
static Err		VerifyDataSubCtx(PvtDataContextPtr ctx);
static void		*DefaultAllocMem(uint32 size, uint32 typeBits, uint32 chunkType,
									int32 channel, int32 time);
static void		*DefaultFreeMem(void *ptr, uint32 chunkType, int32 channel, int32 time);

/***********************************************************************
 * Routines to handle incoming messages from the stream parser thread
 ***********************************************************************/
static int32	DoNewDataArrivedDataSub(PvtDataContextPtr ctx, SubscriberMsgPtr subMsg);
static Err		DoGetChanDataSub(PvtDataContextPtr ctx, SubscriberMsgPtr subMsg);
static Err		DoSetChanDataSub(PvtDataContextPtr ctx, SubscriberMsgPtr subMsg);
static int32	DoOpenDataSub(PvtDataContextPtr ctx, SubscriberMsgPtr subMsg);
static int32	DoCloseDataSub(PvtDataContextPtr ctx, SubscriberMsgPtr subMsg);
static int32	DoStartStopDataSub(PvtDataContextPtr ctx, SubscriberMsgPtr subMsg);
static int32	DoSyncDataSub(PvtDataContextPtr ctx, SubscriberMsgPtr subMsg);
static int32	DoBranchDataSub(PvtDataContextPtr ctx, SubscriberMsgPtr subMsg);
static int32	DoAbortDataSub(PvtDataContextPtr ctx, SubscriberMsgPtr subMsg);

#ifdef DATA_SUBSCRIBER_UNUSED_CMDS
static int32	DoEOFDataSub(PvtDataContextPtr ctx, SubscriberMsgPtr subMsg);
static int32	DoControlDataSub(PvtDataContextPtr ctx, SubscriberMsgPtr subMsg);
#endif	/* DATA_SUBSCRIBER_UNUSED_CMDS */

/********************************************************
 * Main Subscriber thread and its initalization routine
 ********************************************************/
static PvtDataContextPtr	InitializeThread(DataSubInitArgsPtr initArgs);
static void				DataSubscriberThread(void *unused, DataSubInitArgsPtr initArgs);


/*==========================================================================================
  ==========================================================================================
							Subscriber procedural interfaces
  ==========================================================================================
  ==========================================================================================*/

/******************************************************************************
|||	AUTODOC -public -class Streaming -group Startup -name NewDataSubscriber
|||	Instantiates a DATASubscriber.
|||
|||	  Synopsis
|||
|||	    Err NewDataSubscriber(DataContextPtr *ctxPtrPtr,
|||	        DSStreamCBPtr streamCBPtr, int32 deltaPriority, Item msgItem)
|||
|||	  Description
|||
|||	     Instantiates a new DATASubscriber. This creates the subscriber thread, waits for
|||	     initialization to complete, and signs it up for a subscription with the Data Stream
|||	     parser thread.  All channels are enabled.
|||
|||	  Arguments
|||
|||	    ctxPtrPtr
|||	        A pointer to a variable where the data subscriber context handle will be stored.
|||	        If initialization fails, the variable is set to NULL.
|||
|||	    streamCBPtr
|||	        The new subscriber will subscribe to this Streamer.
|||
|||	    deltaPriority
|||	        The priority (relative to the caller) for the subscriber thread.
|||
|||	    msgItem
|||	        A Message Item to use temporarily for a synchronous DSSubscribe() call.
|||
|||	  Return Value
|||
|||	    Returns zero for success, or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Streaming subscriber library call.
|||
|||	  Associated Files
|||
|||	    <streaming/datasubscriber.h>, libsubscriber.a
|||
******************************************************************************/

/*******************************************************************************************
 * Create a new subscriber. Make the subscriber thread and pass a
 * new context (allocated from the global pool) to it.  Create the message port through
 * which all subsequent communications between the subscriber and the streamer will take
 * place, as well as any other necessary per-context resources.
 *******************************************************************************************/
Err
NewDataSubscriber(DataContextPtr *ctxPtrPtr, DSStreamCBPtr streamCBPtr, int32 deltaPriority,
					Item msgItem)
{
	DataSubInitArgs		initData;
	PvtDataContextPtr	ctx = NULL;
	Err					status;
	uint32				signalBits;

	ADD_DATA_TRACE_L1(gDataTraceBufPtr, kTraceNewSubscriber, 0, 0, 0);

	/* zero the init struct in case things go badly */
	memset(&initData, 0, sizeof(DataSubInitArgs));

	/* allocate a signal to synchronize with the completion of the
	 *  subscriber thread's initialization. The subscriber thread will signal
	 *  this thread when it has finished initializing, successfully or not.
	 */
	if ( 0 == (initData.creatorSignal = AllocSignal(0)) )
	{
		status = kDSNoSignalErr;
		goto CLEANUP;
	}
	initData.creatorTask		= CURRENTTASKITEM;	/* see "task.h" for this */
	initData.streamCBPtr		= streamCBPtr;

	/* create the thread that will handle all subscriber responsibilities. */
	status =  CreateThreadVA(
				(void *)&DataSubscriberThread,					/* thread entrypoint */
				"DataSubscriber", 								/* name */
				((int32) CURRENT_TASK_PRIORITY) + deltaPriority,/* priority */
				4096, 											/* initial stack size */
				CREATETASK_TAG_ARGC,		(void *)0,			/* first arg to thread */
				CREATETASK_TAG_ARGP,		(void *)&initData,	/* second arg to thread */
				CREATETASK_TAG_DEFAULTMSGPORT,	0,				/* ask for a msg port */
				TAG_END);

	if ( status <= 0 )
		goto CLEANUP;

	/* wait here while the subscriber initializes itself.  When it's done,
	 *  look at the status returned in the context block to determine
	 *  if all went well.
	 */
	signalBits = WaitSignal(initData.creatorSignal);
	if ( signalBits != initData.creatorSignal )
	{
		status = kDSSignalErr;
		goto CLEANUP;
	}

	/* check the initialization status of the subscriber. If anything
	 *  failed, the 'initData.status' field will be set to a system result
	 *  code. If this is >= 0 then initialization was successful.
	 */
	if ( (status = initData.status) < 0 )
		goto CLEANUP;

	/* all done on our part, now ask the streamer to send us data */
	ctx = initData.dataCtx;
	status = DSSubscribe(msgItem,
						NULL,			 		/* a synchronous request */
						streamCBPtr,			/* stream context block */
						DATA_SUB_CHUNK_TYPE,	/* our data type */
						ctx->requestPort); 		/* subscriber message port */

CLEANUP:
	/* done with this signal, so give it back */
	if ( 0 != initData.creatorSignal )
		FreeSignal(initData.creatorSignal);

	/* if all went well, give the caller a copy of the context pointer */
	if ( status >= 0 )
		*ctxPtrPtr = (DataContextPtr)ctx;

	return status;				/* tell em how it went */
}

/******************************************************************************
|||	AUTODOC -public -class Streaming -group DataSubscriber -name GetDataChunk
|||	Poll for new data on the specified channel.
|||
|||	  Synopsis
|||
|||	    Err GetDataChunk(DataContextPtr ctx, uint32 channelNumber,
|||	        DataChunkPtr dataChunkPtr)
|||
|||	  Description
|||
|||	     Check to see if there is a data chunk available on the specified channel,
|||	     or on every channel if channelNumber = kEVERY_DATA_CHANNEL.  If > 0 is
|||	     returned, the caller is resposible for the block of memory pointed to by
|||	     dataChunkPtr->dataPtr.  You should use the data subscriber context function
|||	     pointer ctx->freeMemFcn( ) to free the memory.
|||
|||	  Arguments
|||
|||	    ctx
|||	        The DATA subscriber context (returned by NewDataSubscriber()).
|||
|||	    channelNumber
|||	        The number of the channel to check for new data, or kEVERY_DATA_CHANNEL to
|||	        check every channel for available data.
|||
|||	    dataChunkPtr
|||	        A pointer to a DataChunk structure which will be filled in if new data
|||	        is returned.
|||
|||	  Return Value
|||
|||	    > 0 if data is returned, 0 if no data returned, negative for an error.
|||	    Possible error codes currently include:
|||
|||	    kDSSubNotFoundErr
|||	      Invalid subscriber context.
|||
|||	  Implementation
|||
|||	    Streaming subscriber library call.
|||
|||	  Associated Files
|||
|||	    <streaming/datasubscriber.h>, libsubscriber.a
|||
******************************************************************************/

/*******************************************************************************************
 * Return the next data block to the client if one is ready.
 *******************************************************************************************/
Err
GetDataChunk(DataContextPtr ctx, uint32 channelNumber, DataChunkPtr dataChunkPtr)
{
	DataHeadPtr		headPtr,
					prevPtr = NULL;
	Err				err;

	/* make sure we have a valid context (side effect is to set err to 0, the value
	 *  we return if no new data is found)
	 */
	if ( kDSNoErr != (err = VerifyDataSubCtx((PvtDataContextPtr)ctx)) )
		goto DONE;

	/* process all queued messages that have come due */
	if ( kDSNoErr != (err = ProcessMessageQueue((PvtDataContextPtr)ctx)) )
		goto DONE;

	/* now find and return the first chunk on the "completed" queue for the
	 *  requested channel
	 */
	headPtr = ((PvtDataContextPtr)ctx)->completedBlockList;
	while ( NULL != headPtr )
	{
		if ( (kEVERY_DATA_CHANNEL == channelNumber)
			|| (channelNumber == headPtr->channel) )
		{
			uint32	time = GetStreamClock((PvtDataContextPtr)ctx);

			/* copy the userdata and data ptr into the client provided block */
			dataChunkPtr->userData[0]	= headPtr->userData[0];
			dataChunkPtr->userData[1]	= headPtr->userData[1];
			dataChunkPtr->dataPtr		= headPtr->dataPtr;
			dataChunkPtr->channel		= headPtr->channel;
			dataChunkPtr->size			= headPtr->totalDataSize;

			/* unlink the block from the list & free the header, the client now owns it */
			UnlinkHeader(headPtr, prevPtr, &((PvtDataContextPtr)ctx)->completedBlockList);
			ctx->freeMemFcn(headPtr, DATA_HEADER_CHUNK_TYPE, channelNumber, time);

			/* don't look any further, we can only return one chunk per call */
			err = 1;
			break;
		}
		prevPtr = headPtr;
		headPtr = headPtr->nextHeadPtr;
	}

DONE:
	return err;
}


/******************************************************************************
|||	AUTODOC -public -class Streaming -group DataSubscriber -name SetDataMemoryFcns
|||	Set memory allocation/free functions for a DataSubscriber.
|||
|||	  Synopsis
|||
|||	    Err SetDataMemoryFcns(DataContextPtr ctx, DataAllocMemFcn allocMemFcn,
|||	        DataFreeMemFcn freeMemFcn)
|||
|||	  Description
|||
|||	    Override the functions used by the DATA subscriber to allocate and free
|||	    memory (by default, the subscriber calls AllocMemTrackWithOptions()
|||	    and FreeMemTrack()).  The subscriber makes two allocations for each
|||	    complete DATA chunk it recieves from the stream: one for a header used
|||	    to track the chunk as it's data is accumulated from the stream (chunk
|||	    type 'DAHD'), and one for the actual data to be copied from the stream
|||	    (chunk type 'BLOK').
|||
|||	  Arguments
|||
|||	    ctx
|||	        The DATA subscriber context (returned by NewDataSubscriber()).
|||
|||	    allocMemFcn
|||	        A pointer to the function to call to allocate memory.
|||
|||	    freeMemFcn
|||	        A pointer to the function to call to free memory.
|||
|||	  Return Value
|||
|||	    Returns zero or a negative error code for failure. Possible error codes currently
|||	    include:
|||
|||	    kDSSubNotFoundErr
|||	      Invalid subscriber context.
|||
|||	    kDataErrMemsBeenAllocated
|||	      The subscriber has already allocated memory (upon receipt of data from the
|||	      streamer).  This function must be called BEFORE the subscriber receives
|||	      any stream data.
|||
|||	  Implementation
|||
|||	    Streaming subscriber library call.
|||
|||	  Associated Files
|||
|||	    <streaming/datasubscriber.h>, libsubscriber.a
|||
|||	  See Also
|||
|||	    NewDataSubscriber()
|||
******************************************************************************/

/*******************************************************************************************
 * SetDataMemoryFcns
 *  Set the functions we'll use to allocate and free memory.  By default we call
 *   "AllocMemTrackWithOptions()" and "FreeMemTrack()".
 *******************************************************************************************/
Err
SetDataMemoryFcns(DataContextPtr ctx, DataAllocMemFcn allocMemFcn, DataFreeMemFcn freeMemFcn)
{
	Err		err;

	if ( kDSNoErr == (err = VerifyDataSubCtx((PvtDataContextPtr)ctx)) )
	{
		/* see if we've already allocated memory (already pulled a data chunk out of the
		 *  stream buffer).  if so, can't change the functions now
		 */
		if ( FlagIsSet(((PvtDataContextPtr)ctx)->flags, DATA_MEM_ALLOCED) )
			err = kDataErrMemsBeenAllocated;
		else
		{
			ctx->allocMemFcn	= allocMemFcn;
			ctx->freeMemFcn		= freeMemFcn;
			err = kDSNoErr;
		}
	}

	return err;
}




/*==========================================================================================
  ==========================================================================================
							Private, utility functions
  ==========================================================================================
  ==========================================================================================*/

/*******************************************************************************************
 * VerifyDataSubCtx
 *	Verify the datasubscriber context.
 *******************************************************************************************/
static Err
VerifyDataSubCtx(PvtDataContextPtr ctx)
{
	if ( (NULL != ctx) && (ctx->cookie == (uint32)ctx) )
		return kDSNoErr;
	else
		return kDSSubNotFoundErr;
}


/*******************************************************************************************
 * DefaultAllocMem
 *	Default procedure called when allocating memory.  Simply call "AllocMemTrack()"
 *******************************************************************************************/
static void *
DefaultAllocMem(uint32 size, uint32 typeBits, uint32 chunkType, int32 channel, int32 time)
{
	TOUCH(chunkType);
	TOUCH(channel);
	TOUCH(time);
	return AllocMemTrackWithOptions(size, typeBits);
}


/*******************************************************************************************
 * DefaultFreeMem
 *	Default procedure called when freeing memory.  Simply call "FreeMemTrack()"
 *******************************************************************************************/
static void *
DefaultFreeMem(void *ptr, uint32 chunkType, int32 channel, int32 time)
{
	TOUCH(chunkType);
	TOUCH(channel);
	TOUCH(time);
	FreeMemTrack(ptr);
	return NULL;
}



/*==========================================================================================
  ==========================================================================================
					High level interfaces used by the main thread to process
									incoming messages.
  ==========================================================================================
  ==========================================================================================*/

/*******************************************************************************************
 * Routine to process arriving data chunks.
 *
 * NOTE:	Fields 'streamChunkPtr->streamChunkType' and 'streamChunkPtr->streamChunkSize'
 *			contain the 4 character stream data type and size, in BYTES, of the actual
 *			chunk data.
 *******************************************************************************************/
static int32
DoNewDataArrivedDataSub(PvtDataContextPtr ctx, SubscriberMsgPtr subMsg)
{
	int32				status;
	DataChunkFirstPtr	dataHeader;

	dataHeader		= (DataChunkFirstPtr)subMsg->msg.data.buffer;

	/* The incoming message could be data or a header chunk. Figure out
	 * which; for headers, initalize the channel on which the header arrived.
	 */
	switch ( dataHeader->subChunkType )
	{
		case DATA_FIRST_CHUNK_TYPE:		/* the first part of a data block */
		case DATA_NTH_CHUNK_TYPE:		/* a partial data block */
			status = ProcessNewDataChunk(ctx, subMsg);
			break;

		default:
			/* For unrecognized or thrashed chunk types */
			status = ReplyToSubscriberMsg(subMsg, kDSNoErr);
			PERR(("Data subscriber: recieved unknown chunk type: \"%.4s\"", dataHeader->subChunkType));
			break;

	}

	return status;
}


/*******************************************************************************************
 * Set the status bits of the specified channel.
 *******************************************************************************************/
static Err
DoSetChanDataSub(PvtDataContextPtr ctx, SubscriberMsgPtr subMsg)
{
	Err			status;
	uint32		mask;
	int32		channelNumber = subMsg->msg.channel.number;
	int32		wasEnabled;

	if ( channelNumber < kDATA_SUB_MAX_CHANNELS )
	{
		/* Allow only bits that are Read/Write to be set by this call. */
		wasEnabled = IsDataChanEnabled(ctx, channelNumber);

		/* this subscriber ONLY reccognizes "enabled" bit as we only use one bit per
		 *  channel to store state
		 */
		mask = subMsg->msg.channel.mask & ~CHAN_ENABLED;
		status = SetDataChanEnabled(ctx, channelNumber, (subMsg->msg.channel.status & mask));
		if ( 0 != status )
		{
			PERR(("Data Subscriber: error in set channel\n", status));
		}
		else
		{
			/* If the channel has become disabled, flush ALL data */
			if ( wasEnabled && ( false == IsDataChanEnabled(ctx, channelNumber) ) )
			{
				FlushDataChannel(ctx, channelNumber);
				FreeCompletedDataChannel(ctx, channelNumber);
			}
		}
	}
	else
	{
		PERR(("Data Subscriber: channel number (%ld) out of bounds (0 - %ld allowed).\n",
							channelNumber, kDATA_SUB_MAX_CHANNELS - 1));
		status = kDSChanOutOfRangeErr;
	}

	return status;
}


/*******************************************************************************************
 * Return the status bits of the specified channel.
 *******************************************************************************************/
static Err
DoGetChanDataSub(PvtDataContextPtr ctx, SubscriberMsgPtr subMsg)
{
	Err			status;
	int32		channelNumber;

	channelNumber = subMsg->msg.channel.number;

	if ( channelNumber < kDATA_SUB_MAX_CHANNELS )
		status = (int32)IsDataChanEnabled(ctx, channelNumber);
	else
	{
		PERR(("Data Subscriber: channel number (%ld) out of bounds (0 - %ld allowed).\n",
					channelNumber, channelNumber - 1));
		status = kDSChanOutOfRangeErr;
	}

	return status;
}


#ifdef DATA_SUBSCRIBER_UNUSED_CMDS
/*******************************************************************************************
 * Perform an arbitrary subscriber-specific operation.
 *******************************************************************************************/
static int32
DoControlDataSub(PvtDataContextPtr ctx, SubscriberMsgPtr subMsg)
{
	int32				status;
	int32				userWhatToDo;
	DataControlBlockPtr	controlBlockPtr;

	userWhatToDo = subMsg->msg.control.controlArg1;
	controlBlockPtr = (DataControlBlockPtr) subMsg->msg.control.controlArg2;

	PERR(("Data Subscriber: unknown control message: %ld.\n", userWhatToDo));
	status = kDSNoErr;			/* ignore unknown control messages for now... */

	return status;
}
#endif	/* DATA_SUBSCRIBER_UNUSED_CMDS */

/*******************************************************************************************
 * Open a subscription.
 *******************************************************************************************/
static int32
DoOpenDataSub(PvtDataContextPtr ctx, SubscriberMsgPtr subMsg)
{
	TOUCH(ctx);
	TOUCH(subMsg);
	return kDSNoErr;
}


/*******************************************************************************************
 * Close down an open subscription.
 *******************************************************************************************/
static int32
DoCloseDataSub(PvtDataContextPtr ctx, SubscriberMsgPtr subMsg)
{
	TOUCH(subMsg);
	/* Close all logical channels */
	CloseDataChannel(ctx, kEVERY_DATA_CHANNEL);

	return kDSNoErr;
}


/*******************************************************************************************
 * Start/Stop all logical channels.  for this subscriber is simply means flushing old,
 *  INCOMPLETE data.  Assume that the user will still want to be able to get the
 *  completed but unclaimed blocks.
 *******************************************************************************************/
static int32
DoStartStopDataSub(PvtDataContextPtr ctx, SubscriberMsgPtr subMsg)
{
	/* flush all uncompleted blocks */
	if ( subMsg->msg.start.options & (SOPT_NOFLUSH | SOPT_FLUSH | SOPT_NEW_STREAM) )
		FlushDataChannel(ctx, kEVERY_DATA_CHANNEL);

	return kDSNoErr;
}


/*******************************************************************************************
 * Flush all data waiting and/or queued under the assumption that we have
 * just arrived at a branch point and should be ready to deal with all new data
 * from an entirely different part of the stream.
 *******************************************************************************************/
static int32
DoSyncDataSub(PvtDataContextPtr ctx, SubscriberMsgPtr subMsg)
{
	DoStartStopDataSub(ctx, subMsg);
	return kDSNoErr;
}


/*******************************************************************************************
 * Process a stream-branched message: assume that the user wants us to do the same things
 *  we do for a start/stop/sync, free all paritial and unprocessed data but hang onto
 *  completed but unclaimed data blocks.
 *******************************************************************************************/
static int32
DoBranchDataSub(PvtDataContextPtr ctx, SubscriberMsgPtr subMsg)
{
	DoStartStopDataSub(ctx, subMsg);
	return kDSNoErr;
}


#ifdef DATA_SUBSCRIBER_UNUSED_CMDS
/*******************************************************************************************
 * Handle end-of-file on the stream.
 *******************************************************************************************/
static int32
DoEOFDataSub(PvtDataContextPtr ctx, SubscriberMsgPtr subMsg)
{
	TOUCH(ctx);
	TOUCH(subMsg);
	return kDSNoErr;
}
#endif


/*******************************************************************************************
 * Kill all output, return all queued buffers, and generally stop everything.
 * Return all channels to pre-initalized state.
 *******************************************************************************************/
static int32
DoAbortDataSub(PvtDataContextPtr ctx, SubscriberMsgPtr subMsg)
{
	TOUCH(subMsg);

	/* Toss all undeliverd and partial chunks */
	FlushDataChannel(ctx, kEVERY_DATA_CHANNEL);
	FreeCompletedDataChannel(ctx, kEVERY_DATA_CHANNEL);

	return kDSNoErr;
}



/*==========================================================================================
  ==========================================================================================
									The subscriber thread
  ==========================================================================================
  ==========================================================================================*/

static PvtDataContextPtr
InitializeThread(DataSubInitArgsPtr initArgs)
{
	PvtDataContextPtr	ctx;
	Err					err;

	/* allocate a subscriber context, fill in what we know */
	err = kDSNoMemErr;
	ctx = (PvtDataContextPtr)AllocMem(sizeof(PvtDataContext),
		MEMTYPE_NORMAL|(MEMTYPE_FILL|0x0));
	if ( NULL == ctx )
		goto BAILOUT;

	/* Initialize fields in the context record */
	ctx->cookie				= (uint32)ctx;
	ctx->threadItem			= CURRENTTASKITEM;
	ctx->requestPort		= 0;
	ctx->flags				= 0;
	ctx->completedBlockList	= NULL;
	ctx->partialBlockList	= NULL;
	ctx->streamCBPtr		= initArgs->streamCBPtr;

	/* turn on all channels by initing the channel bitmap to -1 */
	memset(ctx->chanBitMap, 0xFFFFFFFF, sizeof(uint32) * LONGS_IN_BITMAP);

	/* use NewPtr/FreePtr as default memory functions */
	ctx->allocMemFcn		= (DataAllocMemFcn)DefaultAllocMem;
	ctx->freeMemFcn			= (DataFreeMemFcn)DefaultFreeMem;

	/* create the context semaphore */
	if ( (err = ctx->ctxLock = CreateSemaphore("DataSubLock", 0)) < 0 )
		goto BAILOUT;

	/* Create the message port where this subscriber thread will accept
	 *  request messages.
	 */
	ctx->requestPort		= CURRENTTASK->t_DefaultMsgPort;
	ctx->requestPortSignal	= MSGPORT(ctx->requestPort)->mp_Signal;

BAILOUT:
	/* Inform the creator of the subscriber thread that initialization has
 	 *  completed - ready and able to accept requests for work, or failed to initialize.
	 *  If the latter, deallocate all memory that we allocated.
	 *  All other resources we created will be cleaned up for us by the system.
	 */
	if ( (initArgs->status = err) < 0 )
		{
		FreeMem(ctx, sizeof(*ctx));
		ctx = NULL;
		}

	initArgs->dataCtx = ctx;
	(void)SendSignal(initArgs->creatorTask, initArgs->creatorSignal);

	return ctx;
}


/*******************************************************************************************
 * This thread is started by a call to NewDataSubscriber(). It waits on the subscriber
 * message port for work requests.
 *******************************************************************************************/
static void
DataSubscriberThread(void *unused, DataSubInitArgsPtr initArgs)
{
	int32				status;
	SubscriberMsgPtr	subMsg;
	uint32				signalBits;
	uint32				anySignal;
	PvtDataContextPtr		ctx;
	bool				continueThread;

	TOUCH(unused);

	/* Startup initialization. kill the thread if we get an error */
	if ( (ctx = InitializeThread(initArgs)) == NULL )
		goto ERROR_EXIT;

	/* All resources are now allocated and ready to use. Our creator has been informed
	 * that we are ready to accept requests for work. All that's left to do is
	 * wait for work request messages to arrive.
	 */
	continueThread = true;
	while ( true == continueThread )
	{
		ADD_DATA_TRACE_L1(gDataTraceBufPtr, kTraceWaitingOnSignal,
					-1, ctx->requestPortSignal , 0);

		anySignal = ctx->requestPortSignal;
		signalBits = WaitSignal(anySignal);

		ADD_DATA_TRACE_L1(gDataTraceBufPtr, kTraceGotSignal, -1, signalBits, 0);

		/* Check for and process any incoming request messages */
		if ( signalBits & ctx->requestPortSignal )
		{
			/* Process any new requests for service as determined by the incoming
			 * message data.
			 */
			while( true == PollForMsg(ctx->requestPort, NULL, NULL, (void **) &subMsg, &status) )
			{
				switch ( subMsg->whatToDo )
				{
					case kStreamOpData:				/* new stream data has arrived */
						ADD_DATA_TRACE_L1(gDataTraceBufPtr, kTraceDataSubmitted, -1, 0, subMsg);
						status = DoNewDataArrivedDataSub(ctx, subMsg);
						break;

					case kStreamOpGetChan:			/* get logical channel status */
						ADD_DATA_TRACE_L1(gDataTraceBufPtr,
									kTraceGetChanMsg, subMsg->msg.channel.number,
									0, subMsg);
						status = DoGetChanDataSub(ctx, subMsg);
						break;

					case kStreamOpSetChan:			/* set logical channel status */
						ADD_DATA_TRACE_L1(gDataTraceBufPtr,
									kTraceSetChanMsg, subMsg->msg.channel.number,
									subMsg->msg.channel.status, subMsg);
						status = DoSetChanDataSub(ctx, subMsg);
						break;

#ifdef DATA_SUBSCRIBER_UNUSED_CMDS
					case kStreamOpControl:			/* perform subscriber-defined function */
						ADD_DATA_TRACE_L1(gDataTraceBufPtr, kTraceControlMsg, -1, 0, subMsg);
						status = DoControlDataSub(ctx, subMsg);
						break;
#endif	/* DATA_SUBSCRIBER_UNUSED_CMDS */

					case kStreamOpSync:				/* clock changed */
						ADD_DATA_TRACE_L1(gDataTraceBufPtr, kTraceSyncMsg, -1, 0, subMsg);
						status = DoSyncDataSub(ctx, subMsg);
						break;

					case kStreamOpOpening:			/* one time initialization call from DSH */
						ADD_DATA_TRACE_L1(gDataTraceBufPtr, kTraceOpeningMsg, -1, 0, subMsg);
						status = DoOpenDataSub(ctx, subMsg);
						break;

					case kStreamOpClosing:			/* stream is being closed */
						ADD_DATA_TRACE_L1(gDataTraceBufPtr, kTraceClosingMsg, -1, 0, subMsg);
						status = DoCloseDataSub(ctx, subMsg);
						continueThread = false;
#ifdef	TRACE_DATA_SUBSCRIBER
						DumpRawTraceBuffer(gDataTraceBufPtr, TRACE_FILENAME);
#endif
						break;

					case kStreamOpStop:				/* stream is being stopped */
						ADD_DATA_TRACE_L1(gDataTraceBufPtr, kTraceStopMsg, -1, 0, subMsg);
						status = DoStartStopDataSub(ctx, subMsg);
						break;

					case kStreamOpStart:			/* stream is being started */
						ADD_DATA_TRACE_L1(gDataTraceBufPtr, kTraceStartMsg, -1, 0, subMsg);
						status = DoStartStopDataSub(ctx, subMsg);
						break;

#ifdef DATA_SUBSCRIBER_UNUSED_CMDS
					case kStreamOpEOF:				/* physical EOF on data, no more to come */
						ADD_DATA_TRACE_L1(gDataTraceBufPtr, kTraceEOFMsg, -1, 0, subMsg);
						status = DoEOFDataSub(ctx, subMsg);
						break;
#endif	/* DATA_SUBSCRIBER_UNUSED_CMDS */

					case kStreamOpAbort:			/* somebody gave up, stream is aborted */
						ADD_DATA_TRACE_L1(gDataTraceBufPtr, kTraceAbortMsg, -1, 0, subMsg);
						status = DoAbortDataSub(ctx, subMsg);
						continueThread = false;
#ifdef	TRACE_DATA_SUBSCRIBER
						DumpRawTraceBuffer(gDataTraceBufPtr, TRACE_FILENAME);
#endif
						break;

					case kStreamOpBranch:			/* data discontinuity */
						ADD_DATA_TRACE_L1(gDataTraceBufPtr, kTraceBranchMsg, -1, 0, subMsg);
						status = DoBranchDataSub(ctx, subMsg);
						break;

					default:
						;
				}

				/* Reply to the request we just handled unless this is a "data arrived"
				 * message. These are replied to asynchronously when the data is actually
				 * consumed and must not be replied to here.
				 */
				if ( subMsg->whatToDo != kStreamOpData )
				{
					if ( (status = ReplyToSubscriberMsg(subMsg, status)) < 0 )
					{
						ERROR_RESULT_STATUS(("DataSubscriberThread() - error %ld in ReplyToSubscriberMsg()"), status);
					}
				}
			} /* while PollForMsg */
		} /* if RequestPortSignal */
	} /* while continueThread */


	/* Halt, flush, and close all channels for this subscription. */
	FlushDataChannel(ctx, kEVERY_DATA_CHANNEL);
	FreeCompletedDataChannel(ctx, kEVERY_DATA_CHANNEL);

ERROR_EXIT:
	/* Deallocate memory then return. The system will automatically free the
	 * thread's Items. */
	FreeMem(ctx, sizeof(*ctx));
}


