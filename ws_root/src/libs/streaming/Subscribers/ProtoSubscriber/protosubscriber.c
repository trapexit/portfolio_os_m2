/******************************************************************************
 *  @(#) protosubscriber.c 95/09/19 1.17
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <audio/audio.h>
#include <kernel/debug.h> /* for print macro: PERR */
#include <kernel/mem.h>

#include <streaming/datastreamlib.h>
#include <streaming/msgutils.h>
#include <streaming/mempool.h>
#include <streaming/threadhelper.h>
#include <streaming/protochannels.h>
#include <streaming/protosubscriber.h>

#include <streaming/subscribertraceutils.h>

#include "protostreamdefs.h"


/*****************************************************************************
 * Compile switch implementations
 *****************************************************************************/

#if PROTO_TRACE_MAIN
	/* Declare the actual trace buffer.  Let it be global in case other 
	 * modules in the subscriber want to write into it. */
	TraceBuffer		ProtoTraceBuffer;
	TraceBufferPtr	ProtoTraceBufPtr	= &ProtoTraceBuffer;
	
	#define		ADD_PROTO_TRACE_L1(bufPtr, event, chan, value, ptr)	\
						AddTrace(bufPtr, event, chan, value, ptr)

#else	/* Trace is off */
	#define		ADD_PROTO_TRACE_L1(bufPtr, event, chan, value, ptr)
#endif


/*****************************************************************************/
/* Pool from which to allocate subscriber contexts for multiple subscribers  */
/*****************************************************************************/
struct ProtoSubscriberGlobals	{
	MemPoolPtr		contextPool;		
	} ProtoGlobals;
	
/***********************************************************************/
/* Routines to handle incoming messages from the stream parser thread  */
/***********************************************************************/
static int32		DoData( ProtoContextPtr ctx, SubscriberMsgPtr subMsg );
static int32		DoGetChan( ProtoContextPtr ctx, SubscriberMsgPtr subMsg );
static int32		DoSetChan( ProtoContextPtr ctx, SubscriberMsgPtr subMsg );
static int32		DoControl( ProtoContextPtr ctx, SubscriberMsgPtr subMsg );
static int32		DoOpening( ProtoContextPtr ctx, SubscriberMsgPtr subMsg );
static int32		DoClosing( ProtoContextPtr ctx, SubscriberMsgPtr subMsg );
static int32		DoStop( ProtoContextPtr ctx, SubscriberMsgPtr subMsg );
static int32		DoSync( ProtoContextPtr ctx, SubscriberMsgPtr subMsg );
static int32		DoEOF( ProtoContextPtr ctx, SubscriberMsgPtr subMsg );
static int32		DoAbort( ProtoContextPtr ctx, SubscriberMsgPtr subMsg );

/********************************************************
 * Main Subscriber thread and its initalization routine 
 ********************************************************/
static int32		InitializeThread( ProtoContextPtr ctx );
static void		ProtoSubscriberThread( int32 notUsed, ProtoContextPtr ctx );


/*=============================================================================
  =============================================================================
							Subscriber procedural interfaces
  =============================================================================
  =============================================================================*/

/******************************************************************************
 * NOTE: This is no longer an autodoc because the Proto Subscriber is no longer
 * part of the release. In any case, this procedure is obsolete. Whatever we
 * use for a prototypical subscriber should follow the new, simpler scheme for
 * creating and deleting a subscriber.
 *
|||	  Initializes the proto subscriber memory pool.
|||
|||	  Synopsis
|||
|||	    int32 InitProtoSubscriber ( void )
|||
|||	  Description
|||
|||	    Allocates and initializes the proto subscriber memory pool
|||	    required for the subscriber context blocks.  This routine is
|||	    called once at program initialization time.  InitProtoSubscriber()
|||	    must be called before the proto subscriber thread is started
|||	    with NewProtoSubscriber().
|||
|||	  Return Value
|||
|||	    kDSNoErr
|||	        No error
|||
|||	    kDSNoMemErr
|||	        Could not allocate memory pool
|||
|||	  Implementation
|||
|||	    Streaming library call.
|||
|||	  Associated Files
|||
|||	    <streaming/protosubscriber.h>, libsubscriber.a
|||
|||	  See Also
|||
|||	    InitProtoSubscriber(), NewProtoSubscriber(), DisposeProtoSubscriber()
|||
******************************************************************************/

int32	InitProtoSubscriber( void )
	{
	ADD_PROTO_TRACE_L1( ProtoTraceBufPtr, kTraceInitSubscriber, 0, 0, 0 );

	/* Create the memory pool for allocating subscriber
	 * contexts.
	 */
	ProtoGlobals.contextPool = CreateMemPool( PR_SUBS_MAX_SUBSCRIPTIONS,
												sizeof(ProtoContext) );
	if ( ProtoGlobals.contextPool == NULL )
		return kDSNoMemErr;

	/* Return success */
	return kDSNoErr;
	}

/******************************************************************************
 * NOTE: This is no longer an autodoc because the Proto Subscriber is no longer
 * part of the release. In any case, this procedure is obsolete. Whatever we
 * use for a prototypical subscriber should follow the new, simpler scheme for
 * creating and deleting a subscriber.
 *
|||	  Frees the proto subscriber memory pool.
|||
|||	  Synopsis
|||
|||	    int32 CloseProtoSubscriber ( void )
|||
|||	  Description
|||
|||	    Deallocates resources allocated by InitProtoSubscriber().
|||
|||	  Return Value
|||
|||	    kDSNoErr
|||	        No error.
|||
|||	  Implementation
|||
|||	    Streaming library call.
|||
|||	  Associated Files
|||
|||	    <streaming/protosubscriber.h>, libsubscriber.a
|||
|||	  See Also
|||
|||	    InitProtoSubscriber()
|||
******************************************************************************/

int32	CloseProtoSubscriber( void )
	{
	ADD_PROTO_TRACE_L1( ProtoTraceBufPtr, kTraceCloseSubscriber, 0, 0, 0 );

	DeleteMemPool( ProtoGlobals.contextPool );

	ProtoGlobals.contextPool = NULL;
	
	return kDSNoErr;
	}


/******************************************************************************
 * NOTE: This is no longer an autodoc because the Proto Subscriber is no longer
 * part of the release. In any case, this procedure is out of date. Whatever we
 * use for a prototypical subscriber should follow the new, simpler scheme for
 * creating and deleting a subscriber.
 *
|||	  Creates proto subscriber thread.
|||
|||	  Synopsis
|||
|||	    int32 NewProtoSubscriber ( ProtoContextPtr *pCtx,
|||	                              DSStreamCBPtr streamCBPtr,
|||	                              int32 deltapriority )
|||
|||	  Description
|||
|||	    Creates and initializes a proto subscriber thread for the
|||	    calling task.  Returns a pointer to the proto subscriber's
|||	    context structure, which the client application uses when making
|||	    calls to the proto subscriber.  Creates the message port through
|||	    which all subsequent communications between the subscriber and
|||	    the streamer take place, as well as any other necessary per-context
|||	    resources. 
|||
|||	  Arguments
|||
|||	    pCtx
|||	        Address where pointer to proto subscriber's context
|||	        structure is returned.
|||
|||	    streamCBPtr
|||	        Pointer to control block for data stream.
|||
|||	    deltaPriority
|||	        Relative execution priority of new proto subscriber thread.
|||
|||	  Return Value
|||	    On success, NewProtoSubscriber returns the value of creatorStatus
|||	    field in context block.  On failure, it returns datastream error
|||	    codes defined in datastream.h.
|||
|||	    kDSNoErr
|||	        No error
|||
|||	    kDSNoMemErr
|||	        Could not allocate memory pool for the context block.
|||
|||	    kDSNoSignalErr
|||	        Couldn't allocate needed signal to synchronize with the
|||	        completion of the initialization.
|||
|||	    kDSSignalErr
|||	        Problem sending/receiving signal used to synchronize the
|||	        completion of the initialization.
|||
|||	    Portfolio error codes returned by NewThread().
|||
|||	  Implementation
|||
|||	    Streaming library call.
|||
|||	  Associated Files
|||
|||	    <streaming/protosubscriber.h>, libsubscriber.a
|||
|||	  See Also
|||
|||	    InitProtoSubscriber(), DisposeProtoSubscriber()
|||
******************************************************************************/

int32	NewProtoSubscriber( ProtoContextPtr *pCtx,
							DSStreamCBPtr streamCBPtr,
							int32 deltaPriority )
	{
	int32				status;
	ProtoContextPtr		ctx;
	uint32				signalBits;

	ADD_PROTO_TRACE_L1( ProtoTraceBufPtr, kTraceNewSubscriber, 0, 0, 0 );

	/* Allocate a subscriber context */
	ctx = (ProtoContextPtr) AllocPoolMem( ProtoGlobals.contextPool );
	if ( ctx == NULL )
		return kDSNoMemErr;

	/* Allocate a signal to synchronize with the completion of the
	 * subscriber's initialization. It will signal us with this when
	 * it has finished, successfully or not, when it is done initializing
	 * itself.
	 */
	ctx->creatorTask = CURRENTTASKITEM;	/* see "kernel.h> for this */
	ctx->streamCBPtr  = streamCBPtr;

	ctx->creatorSignal = AllocSignal( 0 );
	if ( ctx->creatorSignal == 0 )
		{
		status = kDSNoSignalErr;
		goto CLEANUP;
		}

	/* Create the thread that will handle all subscriber responsibilities. */
	status = NewThread( 
		(void *)(int32)&ProtoSubscriberThread, /* thread entry point */
		4096, 							/* initial stack size */
		(int32)CURRENT_TASK_PRIORITY + deltaPriority,	/* priority */
		"ProtoSubscriber", 				/* name */
		0, 								/* first arg to the thread */
		ctx );							/* second arg to the thread */

	if ( status <= 0 )
		goto CLEANUP;
	else
		ctx->threadItem = status;

	/* Wait here while the subscriber initializes itself. When its done,
	 * look at the status returned to us in the context block to determine
	 * if it was happy.
	 */
	signalBits = WaitSignal( ctx->creatorSignal );
	if ( signalBits != ctx->creatorSignal )
		return kDSSignalErr;

	/* We're done with this signal, so give it back */
	FreeSignal( ctx->creatorSignal );

	/* Check the initialization status of the subscriber. If anything
	 * failed, the 'ctx->creatorStatus' field will be set to a system result
	 * code. If this is >= 0 then initialization was successful.
	 */
	status = ctx->creatorStatus;
	if ( status >= 0 )
		{
		*pCtx = ctx;	/* give the caller a copy of the context pointer */
		return status;	/* return 'success' */
		}

CLEANUP:
	/* Something went wrong in creating the new subscriber.
	 * Release anything we created and return the status indicating
	 * the cause of the failure. */
	ReturnPoolMem(ProtoGlobals.contextPool, ctx);

	return status;
	}


/******************************************************************************
 * NOTE: This is no longer an autodoc because the Proto Subscriber is no longer
 * part of the release. In any case, this procedure is obsolete. Whatever we
 * use for a prototypical subscriber should follow the new, simpler scheme for
 * creating and deleting a subscriber.
 *
|||	  Disposes of proto subscriber thread.
|||
|||	  Synopsis
|||
|||	    int32 DisposeProtoSubscriber( ProtoContextPtr ctx )
|||
|||	  Description
|||
|||	    Disposes of the proto subscriber thread and the thread stack
|||	    space created with newProtoSubscriber().
|||
|||	  Arguments
|||	    ctx
|||	        Pointer to proto subscriber's context structure.
|||
|||	  Return Value
|||
|||	    kDSNoErr
|||	        No error.
|||
|||	  Caveats
|||
|||	    DisposeProtoSubscriber assumes that the subscriber is in a clean
|||	    state where it has returned all resources that were passed to it,
|||	    like messages.
|||
|||	  Implementation
|||
|||	    Streaming library call.
|||
|||	  Associated Files
|||
|||	    <streaming/protosubscriber.h>, libsubscriber.a
|||
|||	  See Also
|||
|||	    NewProtoSubscriber()
|||
******************************************************************************/

int32	DisposeProtoSubscriber(ProtoContextPtr ctx)
	{
	if ( ctx != NULL )
		{
		ADD_PROTO_TRACE_L1(ProtoTraceBufPtr, kTraceDisposeSubscriber,
							0, 0, 0);

		DisposeThread(ctx->threadItem);

		ReturnPoolMem(ProtoGlobals.contextPool, ctx);
		}

	return kDSNoErr;
	}



/*============================================================================
  ============================================================================
					High level interfaces used by the main thread to process
									incoming messages. 
  ============================================================================
  ============================================================================*/
/*****************************************************************************
 * Routine to process arriving data chunks. 
 *
 * NOTE:	Fields 'streamChunkPtr->streamChunkType' and 'streamChunkPtr->
 *			streamChunkSize' contain the 4 character stream data type and size,
 *			in BYTES, of the actual chunk data.
 ******************************************************************************/
static int32	DoData( ProtoContextPtr ctx, SubscriberMsgPtr subMsg )
{
	int32					status;
	StreamChunkPtr			streamChunkPtr;
	ProtoHeaderChunkPtr 	protoHeader;
	ProtoDataChunkPtr		protoData;
	ProtoHaltSubsChunkPtr		protoHalt;
	int32					channelNumber;
	Item 					aTimerCue;
	
	streamChunkPtr	= (StreamChunkPtr) subMsg->msg.data.buffer;
	protoData 		= (ProtoDataChunkPtr) streamChunkPtr;
	protoHeader		= (ProtoHeaderChunkPtr) streamChunkPtr;
	protoHalt		= (ProtoHaltSubsChunkPtr) streamChunkPtr;
	channelNumber	= protoData->channel;

	/* The incoming message could be data or a header chunk. Figure out
	 * which;  For headers, initalize the channel that the header arrived
	 * on.  For data, call the new data handling routine.
	 */
	switch (protoData->subChunkType)
		{
		case PDAT_CHUNK_SUBTYPE:	/* Data msg arrived */

			status = ProcessNewProtoDataChunk( ctx, subMsg );
			break;
		
		case PHDR_CHUNK_SUBTYPE:	/* Header msg arrived */

			status = InitProtoChannel( ctx, protoHeader );
			if (status < 0)  
				{
				PERR(( "Initializaion for proto channel %ld failed; error = %ld\n", channelNumber, status ));
				CloseProtoChannel( ctx, channelNumber );
				}

			/* Because the subscriber considers headers and data to  both
			 * be "Data" messages we have to reply to the header chunk
			 * message here.  Data msgs will be replied to when the
			 * subscriber is done using the data.
			 */
			status = ReplyMsg( subMsg->msgItem, status, subMsg,
								sizeof(SubscriberMsg) ); 
			if ( status < 0 )
				ERROR_RESULT_STATUS( "Reply to Proto header msg", status );
			break;

		case PHLT_CHUNK_SUBTYPE:	/* HALT chunk arrived */
			ADD_PROTO_TRACE_L1( ProtoTraceBufPtr, kHaltChunkArrived, 
								protoHalt->channel, 
								protoHalt->haltDuration, 0 );

			/* Make an Audio Timer Cue */
			aTimerCue = CreateCue( NULL );

			/* Sleep for the duration specified in the HALT chunk */
			status = SleepUntilTime( aTimerCue, GetAudioTime() +
									 protoHalt->haltDuration );

			DeleteCue( aTimerCue );

			/* As with header chunks, we have to reply to the header
			 * chunk message here.
			 */
			status = ReplyMsg( subMsg->msgItem, status, subMsg,
								sizeof(SubscriberMsg) ); 
			if ( status < 0 )
				ERROR_RESULT_STATUS( "Reply to Proto header msg", status );

			ADD_PROTO_TRACE_L1( ProtoTraceBufPtr, kRepliedToHaltChunk, 
								protoHalt->channel, 
								protoHalt->haltDuration, 0 );
			break;

		default:
			/* For unrecognized or hosed chunk types */
			status = ReplyMsg( subMsg->msgItem, kDSNoErr, subMsg,
								sizeof(SubscriberMsg) ); 
			if ( status < 0 )
				ERROR_RESULT_STATUS( "Reply to unrecognized proto chunk type", status );
			break;
		
		}	/* switch */
		
	return status;
	}

		
/******************************************************************************
 * Routine to set the status bits of a given channel.
 ******************************************************************************/		
static int32	DoSetChan( ProtoContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	int32				status			= kDSNoErr;
	int32				channelNumber;
	ProtoChannelPtr		channelPtr;
	int32				wasEnabled;
	
	channelNumber	= subMsg->msg.channel.number;
	channelPtr		= ctx->channel + channelNumber;
	
	if ( channelNumber < PR_SUBS_MAX_CHANNELS )
		{
		/* Allow only bits that are Read/Write to be set by this call.
		 *
		 * NOTE: 	Any special actions that might need to be taken as as
		 *			result of bits being set or cleared should be taken
		 *			now. If the only purpose served by status bits is to 
		 *			enable or disable features, or some other communication,
		 *			then the following is all that is necessary.
		 */
		wasEnabled = IsProtoChanEnabled( channelPtr );

		/* Mask off bits reserved by the system or by the subscriber */
		channelPtr->status |= ( (~PROTO_CHAN_SUBSBITS | ~CHAN_SYSBITS) & 
									subMsg->msg.channel.status );

		/* If the channel is becoming disabled, flush data; if it is
		 * becoming enabled, start it up.
		 */
		if ( wasEnabled && (!IsProtoChanEnabled(channelPtr)) )
			status = FlushProtoChannel( ctx, channelNumber );
		else if ( !wasEnabled && (IsProtoChanEnabled(channelPtr)) )
			status = StartProtoChannel( ctx, channelNumber );
		}

	return status;
	}

		
/******************************************************************************
 * Routine to return the status bits of a given channel.
 ******************************************************************************/
static int32	DoGetChan( ProtoContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	int32		status;
	int32		channelNumber;

	channelNumber = subMsg->msg.channel.number;

	if ( channelNumber < PR_SUBS_MAX_CHANNELS )
		status = ctx->channel[ channelNumber ].status;
	else
		status = 0;

	return status;
	}

		
/******************************************************************************
 * Routine to perform an arbitrary subscriber defined action. 
 ******************************************************************************/		
static int32 DoControl( ProtoContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	int32				status			= kDSNoErr;
	int32				userWhatToDo;
	ProtoCtlBlockPtr	ctlBlockPtr;

	TOUCH(ctx);

	userWhatToDo = subMsg->msg.control.controlArg1;
	ctlBlockPtr = (ProtoCtlBlockPtr) subMsg->msg.control.controlArg2;

	switch ( userWhatToDo )
		{
		case kProtoCtlOpTest:
			if ( ctlBlockPtr->test.channelNumber < PR_SUBS_MAX_CHANNELS )
				{
				/* status = DoSomethingUseful( ctx,
				 * ctlBlockPtr->test.channelNumber,
				 * ctlBlockPtr->test.aValue ); */
				}
			else
				status = kDSChanOutOfRangeErr;
				break;

		default:
			status = 0;		/* ignore unknown control messages for now... */
			break;
		}
	
	return status;
	}

		
/******************************************************************************
 * Routine to do whatever is necessary when a subscriber is added to a stream,
 * typically just after the stream is opened.
 ******************************************************************************/		
static int32	DoOpening( ProtoContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	TOUCH(ctx);
	TOUCH(subMsg);

	return kDSNoErr;
	}

		
/******************************************************************************
 * Routine to close down an open subscription.
 ******************************************************************************/		
static int32	DoClosing( ProtoContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	int32	channelNumber;

	TOUCH(subMsg);

	for ( channelNumber = 0; channelNumber < PR_SUBS_MAX_CHANNELS;
			channelNumber++ )
		CloseProtoChannel( ctx, channelNumber );

	return kDSNoErr;
	}

		
/******************************************************************************
 * Routine to start all channels for this subscription.
 ******************************************************************************/		
static int32	DoStart( ProtoContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	int32	channelNumber;

	/* Start all channels for this subscription.
	 */
	for ( channelNumber = 0; channelNumber < PR_SUBS_MAX_CHANNELS;
			channelNumber++ )
		{
		if ( subMsg->msg.start.options & SOPT_FLUSH )
			FlushProtoChannel( ctx, channelNumber );

		StartProtoChannel( ctx, channelNumber );
		}
		
	return kDSNoErr;
	}

		
/******************************************************************************
 * Routine to stop all channels for this subscription.
 ******************************************************************************/		
static int32	DoStop( ProtoContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	int32	channelNumber;

	/* Stop all channels for this subscription.
	 */
	for ( channelNumber = 0; channelNumber < PR_SUBS_MAX_CHANNELS;
			channelNumber++ )
		{
		/* FlushProtoChannel() has an implicit Stop */
		if ( subMsg->msg.stop.options & SOPT_FLUSH )
			FlushProtoChannel( ctx, channelNumber );
		else
			StopProtoChannel( ctx, channelNumber );		
		}	

	return kDSNoErr;
	}

		
/******************************************************************************
 * Routine flush all data waiting and/or queued under the assumption that we have 
 * just arrived at a branch point and should be ready to deal with all new data 
 * from an entirely different part of the stream.
 ******************************************************************************/		
static int32	DoSync( ProtoContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	int32	channelNumber;

	/* Halt and flush all channels for this subscription, then restart.
	 */
	for ( channelNumber = 0; channelNumber < PR_SUBS_MAX_CHANNELS;
			channelNumber++ )
		FlushProtoChannel( ctx, channelNumber );
		
	DoStart( ctx, subMsg );
	return kDSNoErr;
	}

		
/******************************************************************************
 * Routine to take action on end of file condition.
 ******************************************************************************/		
static int32	DoEOF( ProtoContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	TOUCH(ctx);
	TOUCH(subMsg);

	return kDSNoErr;
	}

		
/******************************************************************************
 * Routine to kill all output, return all queued buffers, and generally stop everything.
 * Should return all channels to pre-initalized state.
 ******************************************************************************/		
static int32	DoAbort( ProtoContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	int32	channelNumber;

	TOUCH(subMsg);

	/* Halt, flush, and close all channels for this subscription.
	 */
	for ( channelNumber = 0; channelNumber < PR_SUBS_MAX_CHANNELS;
			channelNumber++ )
		CloseProtoChannel( ctx, channelNumber );

	return kDSNoErr;
	}

/*=============================================================================
  =============================================================================
									The subscriber thread
  =============================================================================
  =============================================================================*/

static int32	InitializeThread( ProtoContextPtr ctx )
	{
	int32		status;
	int32		k;

	/* Initialize fields in the context record */

	ctx->creatorStatus	= 0;	/* assume initialization will succeed */
	ctx->requestPort	= 0;

	/* Initialize once-only channel related things. 
	 */
	for ( k = 0; k < PR_SUBS_MAX_CHANNELS; k++ )
		{
		ctx->channel[k].status					= 0;
		ctx->channel[k].dataMsgQueue.head		= NULL;
		ctx->channel[k].dataMsgQueue.tail		= NULL;
		}

	/* Create the message port where the new subscriber will accept
	 * request messages.
	 */
	status = NewMsgPort( &ctx->requestPortSignal );
	if ( status <= 0 )
		goto BAILOUT;
	else
		ctx->requestPort = status;

	/* Open the Audio Folio for this thread */
	if ( (status = OpenAudioFolio() ) < 0 )
		{
		ctx->creatorStatus = status;
		goto BAILOUT;
		}

BAILOUT:
	/* Inform our creator that we've finished with initialization. We are
	 * either ready and able to accept requests for work, or we failed to
	 * initialize ourselves. If the latter, we simply exit after informing
	 * our creator.  All resources we created are thankfully cleaned up
	 * for us by the system.
	 */
	status = SendSignal( ctx->creatorTask, ctx->creatorSignal );
	if ( (ctx->creatorStatus < 0) || (status < 0) )
		return -1;
	else
		return kDSNoErr;
	}


/******************************************************************************
 * The proto subscriber's main thread procedure.
******************************************************************************/
static void		ProtoSubscriberThread( int32 notUsed, ProtoContextPtr ctx )
	{
	int32				status			= kDSNoErr;
	SubscriberMsgPtr	subMsg			= NULL;
	uint32				signalBits;
	uint32				anySignal;
	int32				channelNumber;
	Boolean				fKeepRunning	= true;

	TOUCH(notUsed);

	/* Call a utility routine to perform all startup initialization.
	 */
	if ( (status = InitializeThread( ctx )) != 0 )
		exit(0);

	anySignal = ctx->requestPortSignal;

	/* All resources are now allocated and ready to use. Our creator has
	 * been informed that we are ready to accept requests for work. All
	 * that's left to do is wait for work request messages to arrive.
	 */
	while ( fKeepRunning )
		{
		ADD_PROTO_TRACE_L1( ProtoTraceBufPtr, kTraceWaitingOnSignal, 
					-1, ctx->requestPortSignal , 0 );

		signalBits = WaitSignal( anySignal );

		ADD_PROTO_TRACE_L1( ProtoTraceBufPtr, kTraceGotSignal, -1,
							signalBits, 0 );

		/********************************************************/
		/* Check for and process and incoming request messages. */
		/********************************************************/
		if ( signalBits & ctx->requestPortSignal )
			{
			/* Process any new requests for service as determined by the incoming
			 * message data.
			 */
			while( PollForMsg( ctx->requestPort, NULL, NULL,
					(void **) &subMsg, &status ) )
				{	
				switch ( subMsg->whatToDo )
					{
					case kStreamOpData:		/* new data has arrived */	
						ADD_PROTO_TRACE_L1( ProtoTraceBufPtr, 
									kTraceDataMsg, -1, 0, subMsg );
						status = DoData( ctx, subMsg );		
						break;
		
		
					case kStreamOpGetChan:	/* get logical channel status */
						ADD_PROTO_TRACE_L1( ProtoTraceBufPtr, 
								kTraceGetChanMsg, subMsg->msg.channel.number, 
								0, subMsg );
						status = DoGetChan( ctx, subMsg );		
						break;
		
		
					case kStreamOpSetChan:	/* set logical channel status */
						ADD_PROTO_TRACE_L1( ProtoTraceBufPtr, 
								kTraceSetChanMsg, subMsg->msg.channel.number, 
								subMsg->msg.channel.status, subMsg );
						status = DoSetChan( ctx, subMsg );		
						break;
		
		
					case kStreamOpControl:	/* perform subscriber
											 * defined function */
						ADD_PROTO_TRACE_L1( ProtoTraceBufPtr,
							kTraceControlMsg, -1, 0, subMsg );
						status = DoControl( ctx, subMsg );		
						break;
		

					case kStreamOpSync:		/* clock stream resynched the
											 * clock */
						ADD_PROTO_TRACE_L1( ProtoTraceBufPtr,
							kTraceSyncMsg, -1, 0, subMsg );
						status = DoSync( ctx, subMsg );		
						break;
		
		
				/**************************************************************
				 * NOTE:	The following msgs have no extended message
				 * 			arguments and only may use the whatToDo and
				 * 			context fields in the message.
				 **************************************************************/


					case kStreamOpOpening:	/* one time initialization
											 * call from DSH */
						ADD_PROTO_TRACE_L1( ProtoTraceBufPtr,
							kTraceOpeningMsg, -1, 0, subMsg );
						status = DoOpening( ctx, subMsg );		
						break;
		
		
					case kStreamOpClosing:	/* stream is being closed */
						ADD_PROTO_TRACE_L1( ProtoTraceBufPtr,
							kTraceClosingMsg, -1, 0, subMsg );
						status = DoClosing( ctx, subMsg );		
						fKeepRunning = false;

#if PROTO_DUMP_TRACE_ON_STREAM_CLOSE
						DumpRawTraceBuffer( ProtoTraceBufPtr,
											"ProtoTraceRawDump.txt" );
						DumpTraceCompletionStats( ProtoTraceBufPtr,
												  kTraceDataSubmitted, 
												  kTraceDataCompleted,
												 "ProtoTraceStatsDump.txt" );
#endif
						break;
		
		
					case kStreamOpStop:		/* stream is being stopped */
						ADD_PROTO_TRACE_L1( ProtoTraceBufPtr,
							kTraceStopMsg, -1, 0, subMsg );
						status = DoStop( ctx, subMsg );
						break;
		
		
					case kStreamOpStart:	/* stream is being started */
						ADD_PROTO_TRACE_L1( ProtoTraceBufPtr,
							kTraceStartMsg, -1, 0, subMsg );
						status = DoStart( ctx, subMsg );
						break;
		
		
					case kStreamOpEOF:		/* physical EOF on data, no
											 * more to come */
						ADD_PROTO_TRACE_L1( ProtoTraceBufPtr,
							kTraceEOFMsg, -1, 0, subMsg );
						status = DoEOF( ctx, subMsg );		
						break;
		
		
					case kStreamOpAbort:	/* somebody gave up, stream
											 * is aborted */
						ADD_PROTO_TRACE_L1( ProtoTraceBufPtr,
							kTraceAbortMsg, -1, 0, subMsg );
						status = DoAbort( ctx, subMsg );		
						fKeepRunning = false;

#if PROTO_DUMP_TRACE_ON_STREAM_ABORT
						DumpRawTraceBuffer( ProtoTraceBufPtr,
							"ProtoTraceRawDump.txt" );
#endif
						break;
		
					default:
						;
					} /* switch whattodo */
		
				/* Reply to the request we just handled unless this is
				 * a "data arrived" message. These are replied to
				 * asynchronously as the data is actually
				 * consumed and must not be replied to here.
				 */
				if ( subMsg->whatToDo != kStreamOpData )
					{
					status = ReplyMsg( subMsg->msgItem, status, subMsg,
						sizeof(SubscriberMsg) );
					if ( status < 0 )
						ERROR_RESULT_STATUS( "saudiosubscriber.hread() - ReplyMsg()", status );
					}

				} /* while PollForMsg */
				
			} /* if RequestPortSignal */

	} /* while KeepRunning */


	/* Halt, flush, and close all channels for this subscription.
	 */
	for ( channelNumber = 0; channelNumber < PR_SUBS_MAX_CHANNELS;
			channelNumber++ )
		CloseProtoChannel( ctx, channelNumber );

	/* Exiting should cause all resources we allocated to be
	 * returned to the system.
	 */
	ctx->threadItem = 0;
	exit(0);
	}


