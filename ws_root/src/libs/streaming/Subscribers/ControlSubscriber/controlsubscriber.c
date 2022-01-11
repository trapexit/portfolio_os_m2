/******************************************************************************
**
**  @(#) controlsubscriber.c 96/02/21 1.20
**
******************************************************************************/

#include <audio/audio.h>
#include <stdlib.h>				/* for the exit() routine */
#include <kernel/debug.h>		/* for print macro: CHECK_NEG */
#include <kernel/mem.h>

#include <streaming/controlsubscriber.h>
#include <streaming/datastreamlib.h>
#include <streaming/dsstreamdefs.h> 		/* for SUBS_CHUNK_COMMON constants */
#include <streaming/mempool.h>
#include <streaming/msgutils.h>
#include <streaming/threadhelper.h>
#include <streaming/subscriberutils.h>


/* max # of logical channels per subscription */
#define	CTRL_MAX_CHANNELS		1

/* max # of msgs to allocate for broadcasting */
#define	CTRL_MAX_SUBS_MESSAGES	16

/* number of msgs for async streamer requests */
#define	CTRL_NUM_DS_REQS_MSGS	8


/**************************************/
/* Subscriber context, one per stream */
/**************************************/

typedef struct CtrlContext {
	DSStreamCBPtr streamCBPtr;		/* Ptr to the stream's context block */

	Item		requestPort;		/* message port item for subscriber requests */
	uint32		requestPortSignal;	/* signal to detect request port messages */

	Item		dsReqReplyPort;		/* reply port for requests to streamer */
	uint32		dsReqReplyPortSignal;
									/* signal for replies to streamer requests */
	MemPoolPtr	dsReqMsgPool;		/* pool of message blocks for requests to
									 * streamer */

	Item		subsReplyPort;		/* reply port for subscriber broadcasts */
	uint32		subsReplyPortSignal; /* signal for subscriber reply port */
	MemPoolPtr	subsMsgPool;		/* pool of subscriber message blocks */

	Item		cueItem;			/* audio cue item for scheduling output */
	uint32		cueSignal;			/* signal associated with cueItem */
	Boolean		fTimerRunning;		/* flag: timer currently running */
	uint32		timerOwner;			/* subchunk processing that is using the
									 * timer */

	uint32		newClockTime;		/* set stream clock to this when we wake
									 * from timer */

	SubsChannel	channel[CTRL_MAX_CHANNELS];	/* an array of channels */
	} CtrlContext, *CtrlContextPtr;


/************************************
 * Local types and constants
 ************************************/

/* This structure is used temporarily for communication between the spawning
 * (client) process and the nascent subscriber.
 *
 * Thread-interlock is handled as follows: NewCtrlSubscriber() allocates
 * this struct on the stack, fills it in, and passes it's address as an arg to
 * the subscriber thread. The subscriber then owns access to it until sending a
 * signal back to the spawning thread (using the first 2 args in the struct).
 * Before sending this signal, the subscriber fills in the "output" fields of
 * the struct, thus returning its creation status result code and request msg
 * port Item. After sending this signal, the subscriber may no longer touch this
 * memory as NewCtrlSubscriber() will deallocate it. */
typedef struct CtrlCreationArgs {
    /* --- input parameters from the client to the new subscriber --- */
    Item                creatorTask;        /* who to signal when done initializing */
    uint32              creatorSignal;      /* signal to send when done initializing */
    DSStreamCBPtr       streamCBPtr;        /* stream this subscriber belongs to */

    /* --- output results from spawning the new subscriber --- */
    Err                 creationStatus;     /* < 0 ==> failure */
    Item                requestPort;        /* new thread's request msg port */
    } CtrlCreationArgs;


/***********************************
 * Local utility routine prototypes
 ***********************************/

static void		StopCtrlChannel( CtrlContextPtr ctx, uint32 channelNumber );
static void		FlushCtrlChannel( CtrlContextPtr ctx, uint32 channelNumber );
static void 	TearDownCtrlSubscriberCB( CtrlContextPtr ctx );


/***********************************
 * Subchunk implementation routines
 ***********************************/

static void		DoCtlAlarm( CtrlContextPtr ctx, uint32 time, uint32 newTime,
						 uint32 options );
static void		DoCtlPause( CtrlContextPtr ctx, uint32 time, uint32 amount,
						 uint32 options );
static void		DoCtlTimerExpired( CtrlContextPtr ctx );


/********************************************************
 * Main Subscriber thread and its initalization routine
 ********************************************************/

static CtrlContextPtr	InitializeCtrlThread( CtrlCreationArgs *creationArgs );
static void				CtrlSubscriberThread( int32 notUsed, CtrlCreationArgs *creationrgs );


/*============================================================================
  ============================================================================
							Subscriber procedural interfaces
  ============================================================================
  ============================================================================*/


/******************************************************************************
|||	AUTOxxxDOC -public -class Streaming -group Startup -name NewCtrlSubscriber
|||	Creates a control subscriber.
|||	
|||	  Synopsis
|||	
|||	    Item NewCtrlSubscriber(DSStreamCBPtr streamCBPtr, int32 priority,
|||	        Item msgItem)
|||	
|||	  Description
|||	
|||	    Instantiates a new CtrlSubscriber. This creates the subscriber
|||	    thread, waits for initialization to complete, *and* signs it up for
|||	    a subscription with the Data Stream parser thread. This returns the new
|||	    subscriber's request message port Item number or a negative error code.
|||	    The subscriber will clean itself up and exit when it receives a
|||	    kStreamOpClosing or kStreamOpAbort message.
|||	    
|||	    This procedure temporarily allocates and frees a signal bit in the
|||	    caller's thread.
|||	    The stack size for the CtrlSubscriber thread is 4096.
|||	
|||	  Arguments
|||	
|||	    streamCBPtr
|||	        Pointer to control block for data stream. The new subscriber will
|||	        subscribe to this stream.
|||	
|||	    priority
|||	        Execution priority of new control subscriber thread relative to the
|||	        caller.
|||	
|||	    msgItem
|||	        A Message Item to use temporarily for a synchronous DSSubscribe()
|||	        call.
|||	
|||	  Return Value
|||	    
|||	    A non-negative Item number of the new subscriber's request message
|||	    port or a negative error code indicating initialization errors, e.g.:
|||	
|||	    kDSNoErr
|||	        No error
|||	
|||	    kDSNoMemErr
|||	        Could not allocate enough memory for the new subscriber.
|||	
|||	    kDSNoSignalErr
|||	        Couldn't allocate a signal to synchronize with the
|||	        subscriber's initialization.
|||	
|||	    kDSSignalErr
|||	        Problem sending/receiving a signal.
|||	
|||	  Implementation
|||	
|||	    Streaming library call.
|||	
|||	  Associated Files
|||	
|||	    <streaming/controlsubscriber.h>, libsubscriber.a
|||	
******************************************************************************/
Item NewCtrlSubscriber( DSStreamCBPtr streamCBPtr, int32 priority, Item msgItem )
	{
	Err					status;
	CtrlCreationArgs	creationArgs;
	uint32				signalBits;

	/* Setup the creation args, including a signal to synchronize with the
	 * completion of the subscriber's initialization. It will signal us when it
	 * is done initializing itself, successfully or not. */
	creationArgs.creatorTask	= CURRENTTASKITEM;  /* cf. <kernel/kernel.h>, included by <streaming/threadhelper.h> */
	creationArgs.creatorSignal  = AllocSignal(0);
	if ( creationArgs.creatorSignal == 0 )
		{
		status = kDSNoSignalErr;
		goto CLEANUP;
		}
	creationArgs.streamCBPtr	= streamCBPtr;
	creationArgs.creationStatus	= kDSInitErr;
	creationArgs.requestPort	= -1;


	/* Create the thread that will handle all subscriber
	 * responsibilities. */
	status = NewThread(
		(void *)(int32)&CtrlSubscriberThread, 		/* thread entrypoint */
		4096, 										/* stack size */
		(int32)CURRENT_TASK_PRIORITY + priority,	/* priority */
		"CtrlSubscriber", 							/* name */
		0, 											/* first arg */
		&creationArgs);								/* second arg */

	if ( status < 0 )
		goto CLEANUP;

	/* Wait here while the subscriber initializes itself. */
	status = kDSSignalErr;
	signalBits = WaitSignal( creationArgs.creatorSignal );
	if ( signalBits != creationArgs.creatorSignal )
		goto CLEANUP;

	/* We're done with this signal, so give it back */
	FreeSignal( creationArgs.creatorSignal );
	creationArgs.creatorSignal = 0;

	/* Check the initialization status of the subscriber. If anything
	 * failed, creationArgs.creationStatus field will be set to a system Err
	 * code. If this is >= 0 then initialization was successful. */
	status = creationArgs.creationStatus;
	if ( status < 0 )
		goto CLEANUP;

	status = DSSubscribe(msgItem, NULL,	 	/* a synchronous request */
				streamCBPtr,				/* stream context block */
				CTRL_CHUNK_TYPE,			/* subscriber data type */
				creationArgs.requestPort);  /* subscriber message port */
	if ( status < 0 )
		goto CLEANUP;

	return creationArgs.requestPort;		/* success! */


CLEANUP:

	/* Something went wrong in creating the new subscriber.
	 * Release any resources we allocated and return the result status.
	 * The subscriber thread will clean up after itself. */
	if ( creationArgs.creatorSignal )	FreeSignal(creationArgs.creatorSignal);
	return status;
	}


/*============================================================================
  ============================================================================
			Routines to handle the various data message subtypes
  ============================================================================
  ============================================================================*/

/*****************************************************************************
 * Cancel the Control Subscriber's timer if it's running, else noop.
 *****************************************************************************/
static void CancelCtlTimer(CtrlContextPtr ctx)
	{
	if ( ctx->fTimerRunning )
		{
		AbortTimerCue(ctx->cueItem);
		ClearCurrentSignals(ctx->cueSignal);
		ctx->fTimerRunning = FALSE;
		}
	}


/******************************************************************************
 * This routine schedules an action that will wait for the stream clock to advance to
 * whenTime, and then set the stream clock to newTime. For now, options are ignored.
 ******************************************************************************/
static void	DoCtlAlarm( CtrlContextPtr ctx, uint32 whenTime, uint32 newTime, uint32 options )
	{
	int32		status;
	DSClock		dsClock;
	int32		amountToDelay;
	AudioTime	whenToWakeUp;

	TOUCH(options);

	/* If we're already waiting for a timer, cancel it. */
	CancelCtlTimer(ctx);

	/* Get the current stream time. */
	DSGetPresentationClock(ctx->streamCBPtr, &dsClock);

	/* Remember the value that will be used to set the stream time when
	 * the delay timer expires. */
	ctx->timerOwner = ALRM_CHUNK_SUBTYPE;
	ctx->newClockTime = newTime;

	/* Calculate the amount of time we need to delay before performing
	 * the requested action. */
	amountToDelay = whenTime - dsClock.streamTime;

	if ( amountToDelay > 0 )
		{
		/* Calculate the absolute audio time when we wish to be
		 * signalled to perform the requested action. */
		whenToWakeUp = dsClock.audioTime + amountToDelay;

		status = SignalAtTime( ctx->cueItem, whenToWakeUp );
		if ( status < 0 )
			return;

		/* Assert that we are waiting for the timer */
		ctx->fTimerRunning = TRUE;
		}
	else
		/* We're already too late. Do the deed immediately */
		DoCtlTimerExpired( ctx );
	}


/******************************************************************************
 * Routine to pause the stream the specified amount, then restart it running.
 * For now, the options are ignored.
 ******************************************************************************/
static void	DoCtlPause( CtrlContextPtr ctx, uint32 time, uint32 amount,
					 uint32 options )
	{
	Err				status;
	DSClock			dsClock;
	AudioTime		whenToWakeUp;
	DSRequestMsgPtr reqMsg;

	TOUCH(time);
	TOUCH(options);

	/* If we're already waiting for a timer, cancel it. */
	CancelCtlTimer(ctx);

	/* Get the current stream time. */
	DSGetPresentationClock(ctx->streamCBPtr, &dsClock);

	ctx->timerOwner = PAUS_CHUNK_SUBTYPE;

	/* Calculate the audio time when we need to wake up in order to set the
	 * stream clock to the desired value, and start our timer running. */
	whenToWakeUp = dsClock.audioTime + (dsClock.streamTime + amount);
	status = SignalAtTime( ctx->cueItem, whenToWakeUp );
	if ( status < 0 )
		return;

	/* Assert that we are waiting for the timer */
	ctx->fTimerRunning = TRUE;

	/* Allocate a request message and send it */
	reqMsg = (DSRequestMsgPtr) AllocPoolMem( ctx->dsReqMsgPool );
	if ( reqMsg != NULL )
		{
		status = DSStopStream( reqMsg->msgItem, reqMsg, ctx->streamCBPtr, 0 );
		CHECK_NEG("ControlSubscriber DoCtlPause DSStopStream", status);
		/* [TBD] use FAIL_NEG() to handle the error */
		}
	}


/******************************************************************************
 * Routine to handle the timer going off. This may be set by either an ALRM subchunk's
 * processing or a PAUS subchunk's processing.
 ******************************************************************************/
static void DoCtlTimerExpired( CtrlContextPtr ctx )
	{
	DSRequestMsgPtr reqMsg;
	DSClock			dsClock;

	switch ( ctx->timerOwner )
		{
		case ALRM_CHUNK_SUBTYPE:
			/* Set the stream clock to the new time without changing the
			 * branchNumber. [TBD] Set it to the control chunk's branchNumber? */
			DSGetPresentationClock(ctx->streamCBPtr, &dsClock);
			DSSetPresentationClock(ctx->streamCBPtr, dsClock.branchNumber,
				ctx->newClockTime);
			break;

		case PAUS_CHUNK_SUBTYPE:
			reqMsg = (DSRequestMsgPtr) AllocPoolMem( ctx->dsReqMsgPool );
			if ( reqMsg != NULL )
				{
				DSStartStream( reqMsg->msgItem, reqMsg, ctx->streamCBPtr, 0 );
				}
			break;

		default:
			;
		}

	/* Assert that the timer is free for its next use */
	ctx->fTimerRunning = FALSE;
	}


/*============================================================================
  ============================================================================
						Routines to handle subscriber messages
  ============================================================================
  ============================================================================*/


/******************************************************************************
 * Utility routine to disable further data flow for the given channel, and to halt
 * any associated physical processes associated with the channel (i.e., stop sound
 * playing, etc.)
 ******************************************************************************/
static void		StopCtrlChannel( CtrlContextPtr ctx, uint32 channelNumber )
	{
	SubsChannelPtr	chanPtr;

	if ( channelNumber < CTRL_MAX_CHANNELS )
		{
		chanPtr = (SubsChannel *) ctx->channel + channelNumber;

		/* Prevent further data queuing */
		chanPtr->status &= ~CHAN_ENABLED;

		/* Stop any physical processes associated with the channel */
		}
	}


/*******************************************************************************
 * Utility routine to tear down a control subscriber control block.
 * We only have to deallocate memory and clean up any static state, since the
 * OS will dispose of all the Items belonging to the thread when it exits.
 *******************************************************************************/
static void TearDownCtrlSubscriberCB(CtrlContextPtr ctx)
	{
	if ( ctx != NULL )
		{
		DeleteMemPool(ctx->dsReqMsgPool);
		DeleteMemPool(ctx->subsMsgPool);
		FreeMem(ctx, sizeof(*ctx));
		}
	}


/******************************************************************************
 * Utility routine to disable further data flow for the given channel, and to cause
 * any associated physical processes associated with the channel to stop.
 ******************************************************************************/
static void		FlushCtrlChannel( CtrlContextPtr ctx, uint32 channelNumber )
	{
	if ( channelNumber < CTRL_MAX_CHANNELS )
		{
		/* Halt whatever activity is associated with the channel */
		StopCtrlChannel( ctx, channelNumber );

#if 0	/* (The Control Subscriber never uses its msgQueues.) */
		{
		SubsChannelPtr		chanPtr;
		SubscriberMsgPtr	msgPtr;
		SubscriberMsgPtr	next;

		chanPtr = ctx->channel + channelNumber;

		/* Give back all queued chunks for this channel to the
		 * stream parser. We do this by replying to all the
		 * "chunk arrived" messages that we have queued. */
		for ( msgPtr = chanPtr->msgQueue.head; msgPtr != NULL; msgPtr = next )
			{
			/* Get the pointer to the next message in the queue */
			next = (SubscriberMsgPtr) msgPtr->link;

			/* Reply to this chunk so that the stream parser
			 * can eventually reuse the buffer space. */
			ReplyToSubscriberMsg(msgPtr, kDSNoErr);
			}

		chanPtr->msgQueue.head = chanPtr->msgQueue.tail = NULL;
		}
#endif
		}
	}


/******************************************************************************
 * Routine to process arriving data chunks (possibly queuing them). We determine
 * the type of control message and call back into the streamer to handle it.
 *
 * NOTE: This arrangement implies race conditions. Instead of calling into the
 * streamer thread, we should either send it messages, or just move the useful
 * functions there and eliminate the control subscriber.
 ******************************************************************************/
static Err	DoCtlData( SubscriberMsgPtr subMsg, CtrlContextPtr ctx )
	{
	Err				status = kDSNoErr;
	ControlChunkPtr	ccp;
	DSRequestMsgPtr reqMsg;

	ccp = (ControlChunkPtr) subMsg->msg.data.buffer;

	/* Switch on the control chunk's subtype. */
	switch ( ccp->subChunkType )
		{
		case GOTO_CHUNK_SUBTYPE:
			/* Implement an embedded branch to another place in the
			 * current stream. */
			reqMsg = (DSRequestMsgPtr) AllocPoolMem( ctx->dsReqMsgPool );
			if ( reqMsg != NULL )
				{
				status = DSGoMarker( reqMsg->msgItem, reqMsg, ctx->streamCBPtr,
					ccp->u.marker.value,
					GOMARKER_ABSOLUTE | GOMARKER_NO_FLUSH_FLAG );
				CHECK_NEG("ControlSubscriber DSGoMarker", status);
				}
			else
				{
				status = kDSNoMsgErr;
				CHECK_NEG("ControlSubscriber GOTO", status);
				}
			break;

		case SYNC_CHUNK_SUBTYPE:
#define ENABLE_SYNC_CHUNK		0	/* dubious feature no longer supported */
#if ENABLE_SYNC_CHUNK
			{
			Boolean			fAtMarkerPosition	= FALSE;

			/* [TBD] When we remove this code, there's other code below that
			 * can be removed. Search for "DSClockSync". */
			/* Implement a conditional call to DSClockSync() on whether we are
			 * arriving at a "marker" due to our playing through the marker, or
			 * due to having executed a branch to the marker. It is assumed that
			 * subscribers will flush any queued data when receiving one of
			 * these messages.
			 * [TBD] Wait until the chunk's presentation time?  */
			extern Err	DSClockSync(DSStreamCBPtr streamCBPtr,
				MemPoolPtr msgPoolPtr, uint32 nowTime);
			extern Err	DSIsMarker(DSStreamCBPtr streamCBPtr, uint32 markerValue,
				bool *fIsMarker);

			status = DSIsMarker( ctx->streamCBPtr, ccp->u.sync.value,
								 &fAtMarkerPosition );
			FAIL_NEG("ControlSubscriber DSIsMarker", status);

			if ( (status == kDSNoErr) && fAtMarkerPosition )
				{
				/* Set the clock to the new time */
				DSSetPresentationClock(ctx->streamCBPtr,
					subMsg->msg.data.branchNumber, ccp->time);

				/* Tell the world that the clock just changed. */
				status = DSClockSync( ctx->streamCBPtr, ctx->subsMsgPool,
										ccp->time );
				CHECK_NEG("ControlSubscriber DSClockSync", status);

 				/* [TBD] Do we need to adjust our own timer? */
				}
			}
#else
			PERR(("CTRL SYNC chunk unsupported\n"));
#endif
			break;

		case ALRM_CHUNK_SUBTYPE:
			/* Device to set the stream time to a new value when the stream
			 * clock reaches the time specified in the alarm chunk timestamp.
			 * Used in looping streams to allow the time to be set backwards
			 * without causing a flush as soon as the seek completes, as would
			 * happen as a result of a sync chunk. */
			DoCtlAlarm( ctx, ccp->time, ccp->u.alarm.newTime,
					 ccp->u.alarm.options );
			break;

		case PAUS_CHUNK_SUBTYPE:
			/* Pause the stream for the specified amount of time, then resume
			 * playing again. */
			DoCtlPause( ctx, ccp->time, ccp->u.pause.amount, ccp->u.pause.options );
			break;

		case STOP_CHUNK_SUBTYPE:
			/* Stop the stream entirely when we receive one of these */
			reqMsg = (DSRequestMsgPtr) AllocPoolMem( ctx->dsReqMsgPool );
			if ( reqMsg != NULL )
				{
				status = DSStopStream( reqMsg->msgItem, reqMsg,
										ctx->streamCBPtr,
										ccp->u.stop.options );
				CHECK_NEG("ControlSubscriber DSStopStream", status);
				}
			else
				status = kDSNoMsgErr;
			break;
		}

#if ENABLE_SYNC_CHUNK
FAILED:
#endif

	return status;
	}


/******************************************************************************
 * Routine to return the status bits of a given channel.
 ******************************************************************************/
static Err	DoCtlGetChan( SubscriberMsgPtr subMsg, CtrlContextPtr ctx )
	{
	Err		status = kDSChanOutOfRangeErr;
	uint32	channelNumber = subMsg->msg.channel.number;

	if ( channelNumber < CTRL_MAX_CHANNELS )
		status = ctx->channel[ channelNumber ].status;

	return status;
	}


/******************************************************************************
 * Routine to set the channel status bits of a given channel.
 ******************************************************************************/
static Err	DoCtlSetChan( SubscriberMsgPtr subMsg, CtrlContextPtr ctx )
	{
	Err		status = kDSNoErr;
	uint32	channelNumber;
	uint32	mask;

	channelNumber = subMsg->msg.channel.number;

	if ( channelNumber < CTRL_MAX_CHANNELS )
		{
		/* Allow only bits that are Read/Write to be set by this call.
		 *
		 * NOTE: 	Any special actions that might need to be taken as as
		 *			result of bits being set or cleared should be taken
		 *			now. If the only purpose served by status bits is to
		 *			enable or disable features, or some other communication,
		 *			then the following is all that is necessary.
		 *
		 * Mask off bits reserved by the system or by the subscriber */
        mask = subMsg->msg.channel.mask & ~CHAN_SYSBITS;
        ctx->channel[channelNumber].status =
			subMsg->msg.channel.status & mask |
			ctx->channel[channelNumber].status & ~mask;
		}
	else
		status = kDSChanOutOfRangeErr;

	return status;
	}


/******************************************************************************
 * Routine to perform an arbitrary subscriber defined action.
 ******************************************************************************/
#if 0
static Err	DoCtlControl( SubscriberMsgPtr subMsg, CtrlContextPtr ctx )
	{
	TOUCH(subMsg);
	TOUCH(ctx);

	/* This routine takes a long and a pointer as inputs. These are entirely
	 * user defined and can be used to do everything from adjust audio volume
	 * on a per channel basis, to ordering out for pizza. */

	return kDSNoErr;
	}
#else
	#define DoCtlControl(subMsg, ctx)	kDSNoErr
#endif


/******************************************************************************
 * Routine to do whatever is necessary when a subscriber is added to a stream, typically
 * just after the stream is opened.
 ******************************************************************************/
#if 0
static Err	DoCtlOpening( SubscriberMsgPtr subMsg, CtrlContextPtr ctx )
	{
	TOUCH(subMsg);
	TOUCH(ctx);

	return kDSNoErr;
	}
#else
	#define DoCtlOpening(subMsg, ctx)	kDSNoErr
#endif


/******************************************************************************
 * Routine to close down an open subscription.
 ******************************************************************************/
static Err	DoCtlClosing( SubscriberMsgPtr subMsg, CtrlContextPtr ctx )
	{
	uint32	channelNumber;

	TOUCH(subMsg);

	/* Halt and flush all channels for this subscription. */
	for ( channelNumber = 0; channelNumber < CTRL_MAX_CHANNELS;
		  channelNumber++ )
		FlushCtrlChannel( ctx, channelNumber );

	return kDSNoErr;
	}


/******************************************************************************
 * Routine to stop all channels for this subscription.
 ******************************************************************************/
static Err	DoCtlStop( SubscriberMsgPtr subMsg, CtrlContextPtr ctx )
	{
	uint32	channelNumber;

	TOUCH(subMsg);

	/* Stop all channels for this subscription. */
	for ( channelNumber = 0; channelNumber < CTRL_MAX_CHANNELS;
		  channelNumber++ )
		StopCtrlChannel( ctx, channelNumber );

	return kDSNoErr;
	}


/******************************************************************************
 * Routine to (normally) flush any queued data we might have. Since we are (usually) the
 * originator of this message, we simply ignore it.
 ******************************************************************************/
#if 0
static Err	DoCtlSync( SubscriberMsgPtr subMsg, CtrlContextPtr ctx )
	{
	TOUCH(subMsg);
	TOUCH(ctx);

	return kDSNoErr;
	}
#else
	#define DoCtlSync(subMsg, ctx)	kDSNoErr
#endif


/******************************************************************************
 * Routine to do whatever is necessary when EOF is asserted.
 ******************************************************************************/
#if 0
static Err	DoCtlEOF( SubscriberMsgPtr subMsg, CtrlContextPtr ctx )
	{
	TOUCH(subMsg);
	TOUCH(ctx);

	return kDSNoErr;
	}
#else
	#define DoCtlEOF(subMsg, ctx)	kDSNoErr
#endif


/******************************************************************************
 * Routine to kill all output, return all queued buffers, and generally stop everything.
 ******************************************************************************/
static Err	DoCtlAbort( SubscriberMsgPtr subMsg, CtrlContextPtr ctx )
	{
	return DoCtlClosing(subMsg, ctx);
	}



/*============================================================================
  ============================================================================
									The subscriber thread
  ============================================================================
  ============================================================================*/


/******************************************************************************
 * Do one-time initialization for the new subscriber thread: Allocate its
 * context structure (instance data), allocate system resources, etc.
 *
 * RETURNS: The new context pointer if successful or NULL if failed.
 * SIDE EFFECTS: To communicate with the spawning process, this fills in the
 *    creationStatus and requestPort fields of the creationArgs structure and
 *    then sends a signal to the spawning process.
 * NOTE: Once we signal the spawning process, the creationArgs structure will
 *    go away out from under us.
 ******************************************************************************/
static CtrlContextPtr	InitializeCtrlThread( CtrlCreationArgs *creationArgs )
	{
	CtrlContextPtr	ctx;
	Err				status;
	uint32			channelNumber;

	/* Allocate the subscriber context structure (instance data), zeroed
	 * out, and start initializing fields. */

	status = kDSNoMemErr;
	ctx = AllocMem(sizeof(*ctx), MEMTYPE_FILL);
	if ( ctx == NULL )
		goto BAILOUT;
	ctx->streamCBPtr		= creationArgs->streamCBPtr;

	/* Initialize channel structures */
	for ( channelNumber = 0; channelNumber < CTRL_MAX_CHANNELS;
		  channelNumber++ )
		ctx->channel[channelNumber].status = CHAN_ENABLED;

	/* Create the message port where the new subscriber will accept
	 * request messages from the Streamer and client threads. */
	status = NewMsgPort( &ctx->requestPortSignal );
	if ( status < 0 )
		goto BAILOUT;
	creationArgs->requestPort = ctx->requestPort = status;

	/* Open the Audio Folio for this thread */
	if ( (status = OpenAudioFolio() ) < 0 )
		goto BAILOUT;


    /* ***  ***  ***  ***  ***  ***  ***  ***  ***  ***
    **  Control subscriber specific initializations
    ** ***  ***  ***  ***  ***  ***  ***  ***  ***  ***/

	/* Init the msg reply port used to communicate with
	 * the streamer. We will need to send messages to the streamer when
	 * we process "go marker" and "sync" subchunks. */
	status = ctx->dsReqReplyPort = NewMsgPort( &ctx->dsReqReplyPortSignal );
	if ( status < 0 )
		goto BAILOUT;

	/* Allocate a pool of message blocks that we will later send to
	 * the streamer when making asynchronous requests. */
	status = kDSNoMemErr;
	ctx->dsReqMsgPool = CreateMemPool( CTRL_NUM_DS_REQS_MSGS,
										sizeof(DSRequestMsg) );
	if ( ctx->dsReqMsgPool == NULL )
		goto BAILOUT;

	status = kDSNoMsgErr;
	if ( ! FillPoolWithMsgItems( ctx->dsReqMsgPool, ctx->dsReqReplyPort ) )
		goto BAILOUT;

	/* Create a message pool for broadcast functions, such as is required
	 * when calling DSClockSync(), which broadcasts our timing message to all
	 * subscribers. */
	status = ctx->subsReplyPort = NewMsgPort( &ctx->subsReplyPortSignal );
	if ( status < 0 )
		{
		status = kDSNoPortErr;
		goto BAILOUT;
		}

	status = kDSNoMemErr;
	ctx->subsMsgPool = CreateMemPool( CTRL_MAX_SUBS_MESSAGES,
										sizeof(SubscriberMsg) );
	if ( ctx->subsMsgPool == NULL )
		goto BAILOUT;

	status = kDSNoMsgErr;
	if ( ! FillPoolWithMsgItems( ctx->subsMsgPool, ctx->subsReplyPort ) )
		goto BAILOUT;

	/* Create an Audio Cue Item for timing purposes. */
	status = ctx->cueItem =
		CreateItem(MKNODEID(AUDIONODE,AUDIO_CUE_NODE), NULL);
	if ( status < 0 )
		goto BAILOUT;

	/* Get the signal associated with the cue so we
	 * can wait on timed events in the thread's main loop. */
	status = kDSNoSignalErr;
	ctx->cueSignal = GetCueSignal( ctx->cueItem );
	if ( ctx->cueSignal == 0 )
		goto BAILOUT;

	/* Since we got here, creation must've been successful! */
	status = kDSNoErr;


BAILOUT:
	/* Inform our creator that we've finished with initialization.
	 *
	 * If initialization failed, clean up resources we allocated, letting the
	 * system release system resources. We need to free up memory we
	 * allocated and restore static state. */
	creationArgs->creationStatus = status;  /* return info to the creator task */

	SendSignal( creationArgs->creatorTask, creationArgs->creatorSignal );
	creationArgs = NULL;	/* can't access this memory after sending the signal */
	TOUCH(creationArgs);	/* avoid a compiler warning */

	if ( status < 0 )
		{
		TearDownCtrlSubscriberCB(ctx);
		ctx = NULL;
		}

	return ctx;
	} /* InitializeCtrlThread() */


/******************************************************************************
 * This thread is started by a call to NewCtrlSubscriber(). It reads
 * the subscriber message port for work requests and performs appropriate
 * actions. The subscriber message definitions are located in
 * <datastreamlib.h>.
 ******************************************************************************/
static void	CtrlSubscriberThread( int32 notUsed, CtrlCreationArgs *creationArgs )
	{
	CtrlContextPtr		ctx;
	int32				status			= kDSNoErr;
	SubscriberMsgPtr	subMsg			= NULL;
	bool				fKeepRunning	= TRUE;
	uint32				anySignal;
	uint32				signalBits;

	TOUCH(notUsed);

	/* Call a utility routine to perform all startup initialization. */
	ctx = InitializeCtrlThread( creationArgs );
	creationArgs = NULL;    /* can't access that memory anymore */
	TOUCH(creationArgs);    /* avoid a compiler warning */

	if ( ctx == NULL )
		return;

	/* All resources are now allocated and ready to use. Our creator has been
	 * informed that we are ready to accept requests for work. All that's left
	 * to do is wait for work request messages to arrive. */
	anySignal = ctx->cueSignal
				| ctx->requestPortSignal
				| ctx->subsReplyPortSignal
				| ctx->dsReqReplyPortSignal;

	while ( fKeepRunning )
		{
		signalBits = WaitSignal( anySignal );

		/***********************************************/
		/* Check for and process any timer expirations */
		/***********************************************/
		if ( signalBits & ctx->cueSignal )
			{
			DoCtlTimerExpired( ctx );
			}


		/********************************************************************/
		/* Check for and handle replies to broadcasts that have been sent.	*/
		/* Currently, these are only as a result of calling DSClockSync() 	*/
		/********************************************************************/
		if ( signalBits & ctx->subsReplyPortSignal )
			{
			while( PollForMsg( ctx->subsReplyPort, NULL, NULL,
								(void **) &subMsg, &status ) )
				ReturnPoolMem( ctx->subsMsgPool, subMsg );
			}


		/********************************************************************/
		/* Check for and handle replies to asynchronous streamer requests.	*/
		/********************************************************************/
		if ( signalBits & ctx->dsReqReplyPortSignal )
			{
			DSRequestMsgPtr 	reqMsg = NULL;

			while ( PollForMsg( ctx->dsReqReplyPort, NULL, NULL,
								(void **) &reqMsg, &status ) )
				ReturnPoolMem( ctx->dsReqMsgPool, reqMsg );
			}


		/********************************************************/
		/* Check for and process and incoming request messages. */
		/********************************************************/
		if ( signalBits & ctx->requestPortSignal )
			{
			while ( PollForMsg( ctx->requestPort, NULL, NULL,
								(void **) &subMsg, &status ) )
				{
				switch ( subMsg->whatToDo )
					{
					case kStreamOpData:		/* new data has arrived */
						/* NOTE: This never queues any messages, so we can reply
						 * to them right away.
						 * [TBD] So is the flush code (currently?) unnecessary?
						 * [TBD] Wait until the chunk's presentation time before
						 *    executing its action? */
						status = DoCtlData( subMsg, ctx );
						break;


					case kStreamOpGetChan:	/* get logical channel status */
						status = DoCtlGetChan( subMsg, ctx );
						break;


					case kStreamOpSetChan:	/* set logical channel status */
						status = DoCtlSetChan( subMsg, ctx );
						break;


					case kStreamOpControl:	/* perform subscriber-defined fcn */
						status = DoCtlControl( subMsg, ctx );
						break;


					case kStreamOpSync:		/* clock stream resynched the clock */
						status = DoCtlSync( subMsg, ctx );
						break;


				/***********************************************************
				 * NOTE: The following msgs have no extended message arguments
				 * 		 and only may use the whatToDo field in the message.
				 ***********************************************************/


					case kStreamOpOpening:	/* one time initialization call */
						status = DoCtlOpening( subMsg, ctx );
						break;


					case kStreamOpClosing:	/* stream is being closed */
						status = DoCtlClosing( subMsg, ctx );
						fKeepRunning = FALSE;
						break;


					case kStreamOpStop:		/* stream is being stopped */
						status = DoCtlStop( subMsg, ctx );
						break;


					case kStreamOpEOF:		/* physical EOF on data, no more
											 * to come */
						status = DoCtlEOF( subMsg, ctx );
						break;


					case kStreamOpAbort:	/* somebody gave up, stream is
											 * aborted */
						status = DoCtlAbort( subMsg, ctx );
						fKeepRunning = FALSE;
						break;

					case kStreamOpBranch:	/* stream discontinuity */
						/* [TBD] Cancel outstanding timers? */
						break;


					default:
						;
					}

				/* Reply to the request we just handled. */
				ReplyToSubscriberMsg(subMsg, status);
				}
			}
		}

	/* Dispose all memory we allocated and clean up any static state.
	 * The OS will automatically reclaim the Items we allocated. */
	if ( ctx != NULL )
		TearDownCtrlSubscriberCB(ctx);
	} /* CtrlSubscriberThread() */

