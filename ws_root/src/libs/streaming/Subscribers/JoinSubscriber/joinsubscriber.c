/******************************************************************************
 **  @(#) joinsubscriber.c 95/09/27 1.13
 ******************************************************************************/

#include <stdlib.h>				/* for the exit() routine */
#include <strings.h>			/* for the memcpy() routine */
#include <audio/audio.h>
#include <kernel/debug.h>		/* for print macro: CHECK_NEG */
#include <kernel/mem.h>
#include <kernel/semaphore.h>

#include <streaming/datastreamlib.h>
#include <streaming/msgutils.h>
#include <streaming/mempool.h>
#include <streaming/threadhelper.h>
#include <streaming/subscriberutils.h>
#include <streaming/joinsubscriber.h>

/**********************/
/* Subscriber globals */
/**********************/

struct JoinSubscriberGlobals	{

	MemPoolPtr	JoinContextPool; /* The memory pool for subscriber contexts */

	} DGlobals;


/****************************/
/* Local routine prototypes */
/****************************/

static int32 DoJoinData( JoinContextPtr ctx, SubscriberMsgPtr subMsg );
static int32 DoJoinGetChan( JoinContextPtr ctx, SubscriberMsgPtr subMsg );
static int32 DoJoinSetChan( JoinContextPtr ctx, SubscriberMsgPtr subMsg );
static int32 DoJoinControl( JoinContextPtr ctx, SubscriberMsgPtr subMsg );
static int32 DoJoinOpening( JoinContextPtr ctx, SubscriberMsgPtr subMsg );
static int32 DoJoinClosing( JoinContextPtr ctx, SubscriberMsgPtr subMsg );
static int32 DoJoinStart( JoinContextPtr ctx, SubscriberMsgPtr subMsg );
static int32 DoJoinStop( JoinContextPtr ctx, SubscriberMsgPtr subMsg );
static int32 DoJoinSync( JoinContextPtr ctx, SubscriberMsgPtr subMsg );
static int32 DoJoinEOF( JoinContextPtr ctx, SubscriberMsgPtr subMsg );
static int32 DoJoinAbort( JoinContextPtr ctx, SubscriberMsgPtr subMsg );
static void	 DoJoinElementDone( JoinContextPtr ctx, JoinElementMsgPtr joMsg );

static int32	_InitializeJoinThread( JoinContextPtr ctx );
void			JoinSubscriberThread( int32 notUsed, JoinContextPtr ctx );

/**************************/
/* Local utility routines */
/**************************/

static JoinElementMsgPtr FindJoinElement( JoinContextPtr ctx,
										  int32 elemType, int32 channel );
static void RemoveMsgFromJoinQueue( JoinContextPtr ctx,
									JoinElementMsgPtr joMsg );

/*============================================================================
  ============================================================================
							Subscriber procedural interfaces
  ============================================================================
  ============================================================================*/


/******************************************************************************
|||	----DOC -public -class Streaming -group Startup -name InitJoinSubscriber
|||	  Initializes the join subscriber memory pool.
|||
|||	  Synopsis
|||
|||	    int32 InitJoinSubscriber ( void )
|||
|||	  Description
|||
|||	    Allocates and initializes the join subscriber memory pool
|||	    required for the subscriber context blocks.  This routine is
|||	    called once at program initialization time.  InitJoinSubscriber()
|||	    must be called before the join subscriber thread is started
|||	    with NewJoinSubscriber().
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
|||	    <streaming/joinsubscriber.h>, libsubscriber.a
|||
|||	  See Also
|||
|||	    InitJoinSubscriber(), NewJoinSubscriber(), DisposeJoinSubscriber()
|||
******************************************************************************/

int32	InitJoinSubscriber( void )
	{
	/* Create the memory pool for allocating data stream
	 * contexts.
	 */
	DGlobals.JoinContextPool = CreateMemPool( DATA_MAX_SUBSCRIPTIONS,
												sizeof(JoinContext) );
	if ( DGlobals.JoinContextPool == NULL )
		return kDSNoMemErr;


	/* Return success */
	return kDSNoErr;
	}


/******************************************************************************
|||	----DOC -public -class Streaming -group Shutdown -name CloseJoinSubscriber
|||	  Frees the join subscriber memory pool.
|||
|||	  Synopsis
|||
|||	    int32 CloseJoinSubscriber ( void )
|||
|||	  Description
|||
|||	    Deallocates resources allocated by InitJoinSubscriber().
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
|||	    <streaming/joinsubscriber.h>, libsubscriber.a
|||
|||	  See Also
|||
|||	    InitJoinSubscriber()
|||
******************************************************************************/

int32	CloseJoinSubscriber( void )
	{
	DeleteMemPool( DGlobals.JoinContextPool );

	DGlobals.JoinContextPool = NULL;

	return kDSNoErr;
	}


/******************************************************************************
|||	----DOC -public -class Streaming -group Startup -name NewJoinSubscriber
|||	  Creates a join subscriber thread.
|||
|||	  Synopsis
|||
|||	    int32 NewJoinSubscriber ( JoinContextPtr *pCtx,
|||	                              int32 priority )
|||
|||	  Description
|||
|||	    Creates and initializes a join subscriber thread for the
|||	    calling task.  Returns a pointer to the join subscriber's
|||	    context structure, which the client application uses when making
|||	    calls to the join subscriber.
|||
|||	  Arguments
|||
|||	    pCtx
|||	        Address where pointer to join subscriber's context
|||	        structure is returned.
|||
|||	    Priority
|||	        Relative execution priority of new join subscriber thread.
|||
|||	  Return Value
|||	    On success, NewJoinSubscriber returns the value of creatorStatus
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
|||	    <streaming/joinsubscriber.h>, libsubscriber.a
|||
|||	  See Also
|||
|||	    InitJoinSubscriber(), DisposeJoinSubscriber()
|||
******************************************************************************/

int32	NewJoinSubscriber( JoinContextPtr *pCtx, int32 priority )
	{
	int32			status;
	JoinContextPtr	ctx;
	uint32			signalBits;

	/* Allocate a subscriber context */

	ctx = (JoinContextPtr) AllocPoolMem( DGlobals.JoinContextPool );
	if ( ctx == NULL )
		return kDSNoMemErr;

	/* Allocate a signal to synchronize with the completion of the
	 * subscriber's initialization. It will signal us with this when
	 * it has finished, successfully or not, when it is done initializing
	 * itself.
	 */
	ctx->creatorTask = CURRENTTASKITEM;	/* see <kernel.h> for this */
	
	ctx->creatorSignal = AllocSignal( 0 );
	if ( ctx->creatorSignal == 0 )
		{
		status = kDSNoSignalErr;
		goto CLEANUP;
		}

	/* Create the thread that will handle all subscriber 
	 * responsibilities.
	 */
	ctx->JoinElemMsgWaitingPtr = NULL;
	ctx->psubMsgWaiting = NULL;
	
	status = NewThread( 
				(void *)(int32)&JoinSubscriberThread, 	/* thread entrypoint */
				4096, 									/* initial stack size */
				(int32)CURRENT_TASK_PRIORITY + priority,	/* priority */
				"JoinSubscriber", 					/* name */
				0, 									/* first arg */
				ctx);								/* second arg */

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
		*pCtx = ctx;		/* give the caller a copy of the context pointer */
		return status;		/* return 'success' */
		}

CLEANUP:
	/* Something went wrong in creating the new subscriber.
	 * Release anything we created and return the status indicating
	 * the cause of the failure. */
	ReturnPoolMem(DGlobals.JoinContextPool, ctx);

	return status;
	}


/******************************************************************************
|||	----DOC -public -class Streaming -group Shutdown -name DisposeJoinSubscriber
|||	  Disposes of a join subscriber thread.
|||
|||	  Synopsis
|||
|||	    int32 DisposeJoinSubscriber( JoinContextPtr ctx )
|||
|||	  Description
|||
|||	    Disposes of the join subscriber thread, thread stack space, and
|||	    memory pool created with newJoinSubscriber().
|||
|||	  Arguments
|||	    ctx
|||	        Pointer to join subscriber's context structure.
|||
|||	  Return Value
|||
|||	    kDSNoErr
|||	        No error.
|||
|||	  Caveats
|||
|||	    DisposeJoinSubscriber assumes that the subscriber is in a clean
|||	    state where it has returned all resources that were passed to it,
|||	    like messages.
|||
|||	  Implementation
|||
|||	    Streaming library call.
|||
|||	  Associated Files
|||
|||	    <streaming/joinsubscriber.h>, libsubscriber.a
|||
|||	  See Also
|||
|||	    NewJoinSubscriber()
|||
******************************************************************************/

int32	DisposeJoinSubscriber( JoinContextPtr ctx )
	{
	
	if ( ctx )
		{
		if ( ctx->threadItem > 0 )		DisposeThread(ctx->threadItem);
		if ( ctx->joinElemMsgPool )		DeleteMemPool(ctx->joinElemMsgPool);
		ReturnPoolMem(DGlobals.JoinContextPool, ctx);
		}

	return kDSNoErr;
	}


/*******************************************************************************************
 * Set up a msg port for the client to receive data elements. Each port receives only
 * elements of one dataType. If the Client passed in a dataPort, then set a flag to ensure
 * that DestroyJoinPort doesn't release it.
 *******************************************************************************************/
int32	InitJoinPort( JoinContextPtr ctx, Item *joinPort, int32 dataType)
	{
	int32	curPortIndex	= 0;
	Boolean	userDefinedPort	= false;
	
	/* Make sure we can safely modify this list. */
	LockSemaphore (ctx->portListSem, 1);
	
	/* Create a port to receive a particular dataType.
	 */
	if (ctx->numPorts >= MAX_PORTS)
		return kDSNoPortErr;

	/* If the client passed in NIL for a port, create a new one.
	 */
	if (*joinPort == 0)
		{
		*joinPort = NewMsgPort( NULL );
		if ( *joinPort <= 0 )
			return kDSNoPortErr;
		}
	else
		userDefinedPort = true;
	
	/* Find the first open port slot.
	*/
	while (ctx->dataPort[curPortIndex] != 0)
		curPortIndex++;
	
	/* Fill the slot with the port and its dataType.
	*/
	ctx->dataPort[curPortIndex] = *joinPort;
	ctx->dataType[curPortIndex] = dataType;
	ctx->userPort[curPortIndex] = userDefinedPort;
	ctx->numPorts++;
	 
	UnlockSemaphore (ctx->portListSem);

	return kDSNoErr;
	}


/*******************************************************************************************
 * Release the msg port the client used to receive data elements.
 *******************************************************************************************/
void	DestroyJoinPort( JoinContextPtr ctx, DSDataType dataType)
	{
	int32	curPortIndex	= 0;

	/* Make sure we can safely modify this list. */
	LockSemaphore (ctx->portListSem, 1);

	if (ctx->numPorts == 0) return;
	
	/* Find the first open port slot.
	*/
	while (ctx->dataType[curPortIndex] != dataType)
		{
		curPortIndex++;
		if (curPortIndex >= MAX_PORTS) return;
		}
	
	DeleteMsgPort( ctx->dataPort[curPortIndex] );
		
	ctx->dataPort[curPortIndex] = 0;
	ctx->dataType[curPortIndex] = 0;
	ctx->numPorts--;

	UnlockSemaphore (ctx->portListSem);
	}


/*******************************************************************************************
 * Get the next Join Element that has been completed.
 *******************************************************************************************/
JoinElementMsgPtr	GetJoinElement( JoinContextPtr ctx, Item dataPort )
	{
	int32				status	= 0;
	JoinElementMsgPtr	dataMsg	= NULL;
	
	TOUCH(ctx);

	if (PollForMsg( dataPort, NULL, NULL, (void **) &dataMsg, &status ))
		return dataMsg;
	else
		return NULL;
	}
	
	
/*******************************************************************************************
*	use this function in conjunction with ReleaseJoinElementResources
 *******************************************************************************************/
void	FreeJoinData(void *p )
	{
		FreeMemTrack( p );
	}

/*******************************************************************************************
*  use this function after receieving the joindata message, releasing everything but the assembled data 
 *******************************************************************************************/
void	ReleaseJoinElementResources( JoinContextPtr ctx,
									 JoinElementMsgPtr joinElemPtr )
	{
	TOUCH(ctx);

	joinElemPtr->flags |= JF_RELEASERESOURCESONLY;
	ReplyMsg (joinElemPtr->msgItem, kDSNoErr, joinElemPtr, 0);
	}

/*******************************************************************************************
 * Release the msg port the client used to receive data elements.
 *******************************************************************************************/
void	ReleaseJoinElement( JoinContextPtr ctx, JoinElementMsgPtr joinElemPtr )
	{
	TOUCH(ctx);

	ReplyMsg (joinElemPtr->msgItem, kDSNoErr, joinElemPtr, 0);
	}

/*******************************************************************************************
 * Utility routine to flush uncompleted join elements.
 *******************************************************************************************/
int32	FlushJoinSubscriber( JoinContextPtr ctx )
	{
	JoinElementMsgPtr	JoinElemPtr, NextJoinElemPtr;

	JoinElemPtr = ctx->dataMsgHead;
		
	while (JoinElemPtr != NULL)
	{
		
		NextJoinElemPtr = (JoinElementMsgPtr)(JoinElemPtr->link);

		RemoveMsgFromJoinQueue( ctx,  JoinElemPtr);
		
		/* clear the link and return the memory to the pool */
		DoJoinElementDone( ctx, JoinElemPtr );

		JoinElemPtr = NextJoinElemPtr;
	}
	
	ctx->dataMsgHead = NULL;
	
	/*  this gets freed above if it is not NULL */
	ctx->JoinElemMsgWaitingPtr = NULL;

	/* Don't forget to respond to this message */
	if (ctx->psubMsgWaiting != NULL)
	{
		ReplyMsg( ctx->psubMsgWaiting->msgItem, kDSNoErr,
					ctx->psubMsgWaiting, 0 );
		ctx->psubMsgWaiting = NULL;
	}
			
	return kDSNoErr;
}

/*=============================================================================
  =============================================================================
						Routines to handle subscriber messages
  =============================================================================
  =============================================================================*/


/*******************************************************************************************
 * Crawl the data element list and return the matching element or NULL.
 *******************************************************************************************/
static JoinElementMsgPtr FindJoinElement(JoinContextPtr ctx,
										 int32 elemType, int32 channel)
	{
	JoinElementMsgPtr	dataPtr;
	
	for (dataPtr = ctx->dataMsgHead; dataPtr != NULL; 
					dataPtr = (JoinElementMsgPtr)(dataPtr->link) )
	{
		if ((dataPtr->dataType == elemType) &&
			(dataPtr->dataChannel == channel))
			return dataPtr;
	}
			
	return NULL;
	}


/*******************************************************************************************
 * Remove the join element msg from the join element queue.
 *******************************************************************************************/
static void RemoveMsgFromJoinQueue( JoinContextPtr ctx,
									JoinElementMsgPtr joMsg )
	{
	JoinElementMsgPtr	dataPtr;
	JoinElementMsgPtr	lastDataPtr	= NULL;
	
	for (dataPtr = ctx->dataMsgHead; dataPtr != NULL; 
					dataPtr = (JoinElementMsgPtr)(dataPtr->link) )
		{
		if (dataPtr == joMsg)
			{
			if (dataPtr == ctx->dataMsgHead)
				{
				ctx->dataMsgHead = (JoinElementMsgPtr)(dataPtr->link);
				return;
				}
			else
				{
				lastDataPtr->link = dataPtr->link;
				return;
				}
			}
		lastDataPtr = dataPtr;
		}
			
	return;
	}


/*******************************************************************************************
 * Routine to assemble arriving data elements. If this is the first data element of a certain
 * subType and channel number, a new data element is allocated from the pool and enough memory
 * is allocated for the entire data element. The data is then copied or appended into the data
 * element until it is complete. When the data element is complete, it is removed from the
 * pending queue and is sent as a msg to a receiving task.
 *******************************************************************************************/
static int32	DoJoinData( JoinContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	int32				status			= kDSNoErr;
	JoinChunkFirstPtr	dataPtr;
	JoinElementMsgPtr	joMsg;
	int32				curPortIndex	= 0;
	int32				dataSize;
	void				*dataStart;

	if (ctx->streamStopped == true)
		goto EXIT;
	
	dataPtr = (JoinChunkFirstPtr) subMsg->msg.data.buffer;

	/* Memory is now available.  Restore the state */
	if (ctx->JoinElemMsgWaitingPtr != NULL)
	{
		joMsg = ctx->JoinElemMsgWaitingPtr;

		dataStart = (void *) ( ((char *)dataPtr)+sizeof(JoinChunkFirst));
		dataSize = dataPtr->dataSize;
		
		ctx->JoinElemMsgWaitingPtr = NULL;
		ctx->psubMsgWaiting = NULL;
	}
	
	else if ( (joMsg = FindJoinElement(ctx, dataPtr->subChunkType,
										dataPtr->channel)) == NULL )
		{
		/* Check to make sure this is a first chunk */
		if (dataPtr->joinChunkType != JOIN_HEADER_SUBTYPE)
			{
			PERR(("Got a  JOIN_DATA_SUBTYPE -- expected a JOIN_HEADER_SUBTYPE\n")); 
			status =  kDSAbortErr;
			goto EXIT;
			}
		
		joMsg = (JoinElementMsgPtr) AllocPoolMem( ctx->joinElemMsgPool );
		if ( joMsg == NULL ) return -1;
		
		/* link the new data element onto the front of the list */
		joMsg->link = (void *)ctx->dataMsgHead;
		ctx->dataMsgHead = (JoinElementMsgPtr) joMsg;
		
		/* fill in the joMsg fields and allocate the data buffer */
		joMsg->dataType		= dataPtr->subChunkType;
		joMsg->dataChannel	= dataPtr->channel;
		joMsg->dataSize		= dataPtr->totalDataSize;
		
		joMsg->dataTime		= dataPtr->time;
		joMsg->dataOffset	= 0;
		joMsg->flags	= 0;

		/* Allocate memory.  If not available save the state. */
		if ((joMsg->dataPtr = (void *) AllocMemTrackWithOptions(
				dataPtr->totalDataSize, dataPtr->ramType)) == NULL)
			{
			ctx->JoinElemMsgWaitingPtr = joMsg;
			ctx->psubMsgWaiting = subMsg;
			return kDSNoMemErr;
			}
		
		dataStart = (void *) ( ((char *)dataPtr)+sizeof(JoinChunkFirst));
		dataSize = dataPtr->dataSize;
		}
	else
		{
		/* Check to make sure this is a data chunk */
		if (dataPtr->joinChunkType != JOIN_DATA_SUBTYPE)
			{
			PERR(("Got a  JOIN_HEADER_SUBTYPE -- expected a JOIN_DATA_SUBTYPE\n")); 
			status =  kDSAbortErr;
			goto EXIT;
			}
		
		/* Keep updating the join element with the most recent stream time
		 * from the current chunk segment. When the join element completion
		 * message is finally sent, the dataTime will represent the time from
		 * the last join chunk in the element.
		 */
		joMsg->dataTime = dataPtr->time;
		
		dataStart = (void *) ( ((char *)dataPtr)+sizeof(JoinChunkData));
		dataSize = ((JoinChunkData *)dataPtr)->dataSize;
		}


	/* Check to see if we are about to write off the end of this element.
	 */
	if ( (joMsg->dataOffset + dataSize) > joMsg->dataSize)
		CHECK_NEG("JoinSubscriber -- data past end of element. Offset = ", joMsg->dataOffset);
	
	
	/* Copy the element data into its buffer.
	 */
	memcpy( (void *) (((char *)joMsg->dataPtr)+joMsg->dataOffset), 
			dataStart, (size_t) dataSize );
	
	
	/* Bump the offset for the data we copied into the element buffer.
	 */
	joMsg->dataOffset += dataSize;
	
	
	/* Check to see if this element is complete.
	 */
	if (joMsg->dataOffset == joMsg->dataSize)
		{
		Boolean	foundPort = true;
		
		/* Pull this msg out of the dataMsg queue.
		 */
		RemoveMsgFromJoinQueue( ctx, joMsg );
		
		/* Find the port slot for this dataType and send the msg.
		*/
		while (ctx->dataType[curPortIndex] != joMsg->dataType)
			{
			curPortIndex++;
			
			if (curPortIndex >= MAX_PORTS)
				{
				foundPort = false;
				break;
				}
			}
			
		/* If no port exists for this data element, Free it's storage up. */
		if (foundPort)
			status = SendMsg( ctx->dataPort[curPortIndex],
						joMsg->msgItem, joMsg, sizeof(JoinElementMsg) );
		else
			{
			/* FreeMem( joMsg->dataPtr, joMsg->dataSize );
			FreeMemTrack( joMsg->dataPtr );

			/* Return the message to the message pool */
			joMsg->link = NULL;
			ReturnPoolMem( ctx->joinElemMsgPool, joMsg );
			}

		}

EXIT:
	/* We've copied the data so reply the msg back to the DSL */
	ReplyMsg( subMsg->msgItem, kDSNoErr, subMsg, 0 );

	return status;
	}

		
/*******************************************************************************************
 * Routine to return the status bits of a given channel.
 *******************************************************************************************/
static int32	DoJoinGetChan( JoinContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	TOUCH(ctx);
	TOUCH(subMsg);

	return kDSNoErr;
	}

		
/*******************************************************************************************
 * Routine to set the channel status bits of a given channel.
 *******************************************************************************************/		
static int32	DoJoinSetChan( JoinContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	TOUCH(ctx);
	TOUCH(subMsg);

	return kDSNoErr;
	}

		
/*******************************************************************************************
 * Routine to perform an arbitrary subscriber defined action. 
 *******************************************************************************************/		
static int32	DoJoinControl( JoinContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	TOUCH(ctx);
	TOUCH(subMsg);

	/* This routine takes a int32 and a pointer as inputs. These are entirely
	 * user defined and can be used to do everything from adjust audio volume
	 * on a per channel basis, to ordering out for pizza.
	 */
	return kDSNoErr;
	}

		
/*******************************************************************************************
 * Routine to do whatever is necessary when a subscriber is added to a stream, typically
 * just after the stream is opened.
 *******************************************************************************************/		
static int32	DoJoinOpening( JoinContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	TOUCH(ctx);
	TOUCH(subMsg);

	return kDSNoErr;
	}

		
/*******************************************************************************************
 * Routine to close down an open subscription.
 *******************************************************************************************/		
static int32	DoJoinClosing( JoinContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	TOUCH(ctx);
	TOUCH(subMsg);

	return kDSNoErr;
	}

		
/*******************************************************************************************
 * Routine to allow all channels to flow for this subscription.
 *******************************************************************************************/		
static int32	DoJoinStart( JoinContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	TOUCH(subMsg);

	ctx->streamStopped = false;

	return kDSNoErr;
	}

		
/*******************************************************************************************
 * Routine to stop all channels for this subscription.
 *******************************************************************************************/		
static int32	DoJoinStop( JoinContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	TOUCH(subMsg);

	FlushJoinSubscriber( ctx );
	ctx->streamStopped = true;

	return kDSNoErr;
	}

		
/*******************************************************************************************
 * Routine to handle stream clock synchronization
 *******************************************************************************************/		
static int32	DoJoinSync( JoinContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	ctx->localTimeOrigin = subMsg->msg.sync.clock;

	return kDSNoErr;
	}

		
/*******************************************************************************************
 * Routine to 
 *******************************************************************************************/		
static int32 DoJoinEOF( JoinContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	TOUCH(ctx);
	TOUCH(subMsg);

	return kDSNoErr;
	}

		
/*******************************************************************************************
 * Routine to kill all output, return all queued buffers, and generally stop everything.
 *******************************************************************************************/		
static int32 DoJoinAbort( JoinContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	TOUCH(ctx);
	TOUCH(subMsg);

	return kDSNoErr;
	}


/*******************************************************************************************
 * This routine will be called when a data element is released by the Display Task. The
 * memory allocated by the data subscriber for this element is freed and the element is
 * returned to the JoinElement memPool.
 *******************************************************************************************/		
static void	DoJoinElementDone( JoinContextPtr ctx, JoinElementMsgPtr joMsg )
	{
	
	/* Free the memory if resource only flag is not set */
	if (! (joMsg->flags & JF_RELEASERESOURCESONLY))
		FreeMemTrack( joMsg->dataPtr );
		
	/* Return the message to the message pool */
	joMsg->link = NULL;
	ReturnPoolMem( ctx->joinElemMsgPool, joMsg );
	}
	
	
/*============================================================================
  ============================================================================
									The subscriber thread
  ============================================================================
  ============================================================================*/

static int32	_InitializeJoinThread( JoinContextPtr ctx )
	{
	int32			status;
	int32			i;

	/* Initialize fields in the context record */

	ctx->creatorStatus		= -1;	/* assume initialization will fail */
	ctx->localTimeOrigin	= 0;
	ctx->requestPort		= 0;
	ctx->numPorts			= 0;
	ctx->dataMsgHead		= NULL;
	ctx->streamStopped		= true;
	
	ctx->portListSem		= CreateSemaphore( "portListQueue", 100 );

	/* Open the Audio Folio for this thread */
	if ( (status = OpenAudioFolio() ) < 0 )
		{
		ctx->creatorStatus = status;
		goto BAILOUT;
		}

	/* Initialize the ports */
	for (i=0; i<MAX_PORTS; ++i)
		{
		ctx->dataPort[i] = 0;
		ctx->dataType[i] = 0;
		}
		
	ctx->joinElemMsgPool = CreateMemPool( DATA_MAX_ELEMENTS,
											sizeof(JoinElementMsg) );
	if ( ctx->joinElemMsgPool == NULL )
		goto BAILOUT;


	/* Create the message port where the new subscriber will accept
	 * request messages.
	 */
	status = NewMsgPort( &ctx->requestPortSignal );
	if ( status <= 0 )
		goto BAILOUT;
	else
		ctx->requestPort = status;


	/* Create the message port where the data elements 
	 * will be returned from the display task.
	 */
	status = NewMsgPort( &ctx->replyPortSignal );
	if ( status <= 0 )
		goto BAILOUT;
	else
		ctx->replyPort = status;

	
	/* Initialize each data element message pool entry with
	 * the necessary system resources.
	 */
	if ( ! FillPoolWithMsgItems( ctx->joinElemMsgPool, ctx->replyPort ) )
		goto BAILOUT;


	ctx->creatorStatus = 0;

BAILOUT:
	/* Inform our creator that we've finished with initialization. We are either
	 * ready and able to accept requests for work, or we failed to initialize
	 * ourselves. If the latter, we simply exit after informing our creator.
	 * All resources we created are thankfully cleaned up for us by the system.
	 */
	status = SendSignal( ctx->creatorTask, ctx->creatorSignal );
	if ( (ctx->creatorStatus < 0) || (status < 0) )
		return -1;
	else
		return 0;
	}


/*******************************************************************************************
 * This thread is started by a call to InitJoinSubscriber(). It reads the subscriber message
 * port for work requests and performs appropriate actions. The subscriber message
 * definitions are located in "datastreamlib.h>.
 *******************************************************************************************/
/******************************************************************************
|||	  Starts join subscriber thread.
|||
|||	  Synopsis
|||
|||	    void JoinSubscriberThread ( int32 notUsed, JoinContextPtr ctx )
|||
|||	  Description
|||
|||	    This thread is started by a call to InitJoinSubscriber(). It reads
|||	    the subscriber message port for work requests and performs appropriate
|||	    actions. The subscriber message definitions are located in
|||	    <datastreamlib.h>.
|||
|||	  Arguments
|||
|||	    notUsed
|||	        Currently  not used.
|||
|||	    ctx
|||	        Pointer to join subscriber's context structure.
|||
|||	  Implementation
|||
|||	    Streaming library call.
|||
|||	  Associated Files
|||
|||	    <streaming/joinsubscriber.h>, libsubscriber.a
|||
|||	  See Also
|||
|||	    InitJoinSubscriber()
|||
******************************************************************************/

void	JoinSubscriberThread( int32 notUsed, JoinContextPtr ctx )
	{
	int32				status			= kDSNoErr;
	SubscriberMsgPtr	subMsg			= NULL;
	JoinElementMsgPtr	joMsg			= NULL;
	uint32				signalBits;
	uint32				anySignal;
	Boolean				fKeepRunning	= true;

	TOUCH(notUsed);

	/* Call a utility routine to perform all startup initialization.
	 */
	if ( (status = _InitializeJoinThread( ctx )) != 0 )
		{
		PERR(("_InitializeJoinThread failed %ld\n", status));
		exit( 0 );
		}

	/* All resources are now allocated and ready to use. Our creator has
	 * been informed that we are ready to accept requests for work. All
	 * that's left to do is wait for work request messages to arrive.
	 */
	anySignal = ctx->requestPortSignal | ctx->replyPortSignal;

	while ( fKeepRunning )
		{
		signalBits = WaitSignal( anySignal );

		/******************************************************/
		/* Check for and process any replys from Display Task */
		/******************************************************/
		if ( signalBits & ctx->replyPortSignal )
			{
			while( PollForMsg( ctx->replyPort, NULL, NULL,
								(void **) &joMsg, &status ) )
				{	
				DoJoinElementDone( ctx, joMsg );
				}
			}

		/* Check if memory is now available */
		if (ctx->JoinElemMsgWaitingPtr != NULL)
		{
			JoinChunkFirstPtr dataPtr;
			
			dataPtr = (JoinChunkFirstPtr) ctx->psubMsgWaiting->msg.data.buffer;
			
			if ((ctx->JoinElemMsgWaitingPtr->dataPtr =
				(void *)AllocMemTrackWithOptions(dataPtr->totalDataSize,
					dataPtr->ramType)) != NULL)
				DoJoinData( ctx, ctx->psubMsgWaiting);
				
			else if  (signalBits & ctx->requestPortSignal)
				ctx->bRequestSignal = 1;
				
		}

		/********************************************************/
		/* Check for and process and incoming request messages. */
		/********************************************************/
		/* Don't process if we're waiting on memory */
		if (ctx->JoinElemMsgWaitingPtr == NULL)
		{
			if  (( signalBits & ctx->requestPortSignal)  || ctx->bRequestSignal)
				{
				/* Process any new requests for service as determined by
				 * the incoming message data.
				 */
				while( PollForMsg( ctx->requestPort, NULL, NULL,
									(void **) &subMsg, &status ) )
					{	
					switch ( subMsg->whatToDo )
						{
						case kStreamOpData:		/* new data has arrived */	
							status = DoJoinData( ctx, subMsg );		
							break;
			
			
						case kStreamOpGetChan:	/* get logical channel status */
							status = DoJoinGetChan( ctx, subMsg );		
							break;
			
			
						case kStreamOpSetChan:	/* set logical channel status */
							status = DoJoinSetChan( ctx, subMsg );		
							break;
			
			
						case kStreamOpControl:	/* perform subscriber
												 * defined function */
							status = DoJoinControl( ctx, subMsg );		
							break;
			
	
						case kStreamOpSync:		/* clock stream resynched
												 * the clock */
							status = DoJoinSync( ctx, subMsg );		
							break;
			
			
					/**********************************************************
					 * NOTE:	The following msgs have no extended message
					 *          arguments and only may use the whatToDo 
					 * 			and context fields in the message.
					 **********************************************************/
	
	
						case kStreamOpOpening:	/* one time initialization
												 * call */
							status = DoJoinOpening( ctx, subMsg );		
							break;
			
			
						case kStreamOpClosing:	/* stream is being closed */
							status = DoJoinClosing( ctx, subMsg );		
							fKeepRunning = false;
							break;
			
			
						case kStreamOpStart:	/* stream is being started */
							status = DoJoinStart( ctx, subMsg );
							break;
							
							
						case kStreamOpStop:		/* stream is being stopped */
							status = DoJoinStop( ctx, subMsg );
							break;
			
			
						case kStreamOpEOF:		/* physical EOF on data,														 * no more to come */
							status = DoJoinEOF( ctx, subMsg );		
							break;
			
			
						case kStreamOpAbort:	/* somebody gave up,
												 * stream is aborted */
							status = DoJoinAbort( ctx, subMsg );		
							break;
			
			
						default:
							;
						}
			
					/* Reply to the request we just handled unless this
					 * is a "data arrived" message. These are replied
					 * to asynchronously as the data is actually
					 * consumed and must not be replied to here.
					 */
					if ( subMsg->whatToDo != kStreamOpData )
						ReplyMsg( subMsg->msgItem, status, subMsg, 0 );
						
					/* subMsg->whatToDo == kStreamOpData and no memory
					 * break out of the poll loop */
					else if (status == kDSNoMemErr)
						break;
				
				} /* end while */
			
			}

		}

	}
	/* Halt and flush all channels for this subscription. */
	FlushJoinSubscriber( ctx );

	/* Exiting should cause all resources we allocated to be
	 * returned to the system. */
	exit( 0 );
	}
