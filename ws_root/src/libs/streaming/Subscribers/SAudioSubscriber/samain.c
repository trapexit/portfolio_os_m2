/******************************************************************************
**
**  @(#) samain.c 96/09/06 1.33
**
******************************************************************************/

#include <string.h>
#include <stdlib.h>				
#include <stdio.h>
#include <audio/audio.h>
#include <kernel/debug.h>		/* for print macros: PERR, PRNT, CHECK_NEG */
#include <kernel/mem.h>

#include <streaming/datastreamlib.h>
#include <streaming/dserror.h>	/* for DS error codes */
#include <streaming/mempool.h>
#include <streaming/threadhelper.h>
#include <streaming/saudiosubscriber.h>

#include <streaming/sacontrolmsgs.h>
#include <streaming/subscriberutils.h>
#include <streaming/subscribertraceutils.h>

#include "sachannel.h"
#include "sasupport.h"
#include "sasoundspoolerinterface.h"

/*****************************************************************************
 * Compile switch implementations
 *****************************************************************************/

#if SAUDIO_TRACE_MAIN

	#ifndef TRACE_LEVEL		/* so it can be predefined via a compiler arg */
		#define TRACE_LEVEL		2
	#endif

	/* Allow for multiple levels of tracing */
	#if (TRACE_LEVEL >= 1)
		#define		ADD_TRACE_L1(bufPtr, event, chan, value, ptr)	\
						AddTrace(bufPtr, event, chan, value, ptr)
	#else
		#define		ADD_TRACE_L1(bufPtr, event, chan, value, ptr)
	#endif
	
	#if (TRACE_LEVEL >= 2)
		#define		ADD_TRACE_L2(bufPtr, event, chan, value, ptr)	\
						AddTrace(bufPtr, event, chan, value, ptr)
	#else
		#define		ADD_TRACE_L2(bufPtr, event, chan, value, ptr)
	#endif
	
	#if (TRACE_LEVEL >= 3)
		#define		ADD_TRACE_L3(bufPtr, event, chan, value, ptr)	\
						AddTrace(bufPtr, event, chan, value, ptr)
	#else
		#define		ADD_TRACE_L3(bufPtr, event, chan, value, ptr)
	#endif

#else	/* Trace is off */
	#define		ADD_TRACE_L1(bufPtr, event, chan, value, ptr)
	#define		ADD_TRACE_L2(bufPtr, event, chan, value, ptr)	
	#define		ADD_TRACE_L3(bufPtr, event, chan, value, ptr)
#endif

static void	SAudioSubscriberThread( int32 notUsed, SAudioCreationArgs *creationArgs );
/********************************************************
 * Main Subscriber thread and its initalization routine 
 ********************************************************/
static SAudioContextPtr	InitializeThdoAudioThread( SAudioCreationArgs *creationArgs );
static void				SAudioSubscriberThread( int32 notUsed, SAudioCreationArgs *creationArgs );

  /*=============================================================================*/
/* Do one-time initialization for the new subscriber thread: Allocate its
 * context structure (instance data), allocate system resources, etc.
 *
 * RETURNS: The new context pointer if successful or NULL if failed.
 * SIDE EFFECTS: To communicate with the spawning process, this fills in the
 *    creationStatus and requestPort fields of the creationArgs structure and
 *    then sends a signal to the spawning process.
 * NOTE: Once we signal the spawning process, the creationArgs structure will
 *    go away out from under us. */

static SAudioContextPtr	InitializeThdoAudioThread( SAudioCreationArgs *creationArgs )
{
	SAudioContextPtr 	ctx;
	Err					status;

	/* Allocate the subscriber context structure (instance data), zeroed
	 * out, and start initializing fields.
	 * Default clock channel to 0 */

	status = kDSNoMemErr;
	ctx = AllocMem(sizeof(*ctx), MEMTYPE_FILL);
	if ( ctx == NULL )
	    goto BAILOUT;
	ctx->streamCBPtr        = creationArgs->streamCBPtr;

	/* Create a place to copy the initial template records for this
	 * instantiation of the audio subscriber. */
	status = kDSNoMemErr;
	ctx->datatype.ThdoAudio.templateArray =
		AllocMem(sizeof(TemplateRec) * kMaxTemplateCount, MEMTYPE_FILL);
	if ( ctx->datatype.ThdoAudio.templateArray == NULL )
		goto BAILOUT;

	/* Copy the (unloaded) template stuff to our own space. We modify
	 * this copy of the template records as we are told to load templates
	 * or as header data in a stream tells us that we need to load a new
	 * instrument. */
	memcpy(ctx->datatype.ThdoAudio.templateArray, gInitialTemplates,
			sizeof(TemplateRec) * kMaxTemplateCount);

	/* Open the Audio Folio for this thread */
	if ( (status = OpenAudioFolio() ) < 0 )
		goto BAILOUT;
	
	/* Load the output instrument template. This gets used every time
	 * a channel is opened to create an output instrument for the channel
	/* Load the envelope instrument template. This gets used every time
	 * a channel is opened to create an envelope instrument for the channel
	 * so we can ramp amplitudes on start and stop. */
	status = ctx->envelopeTemplateItem =
		LoadInsTemplate( SA_ENVELOPE_INSTRUMENT_NAME, 0 );
	if ( status < 0 )
		goto BAILOUT;

	/* Do the rest of initialization. */
	status = InitializeSAudioThread( ctx );

	creationArgs->requestPort = ctx->requestPort;

BAILOUT:
	/* Inform our creator that we've finished with initialization.
	 *
	 * If initialization failed, clean up resources we allocated, letting the
	 * system release system resources. We need to free up memory we
	 * allocated and restore static state. */
	creationArgs->creationStatus = status;  /* return info to the creator task */

	SendSignal( creationArgs->creatorTask, creationArgs->creatorSignal );
    creationArgs = NULL;    /* can't access this memory after sending the signal */
    TOUCH(creationArgs);    /* avoid a compiler warning */

	if ( status < 0 && ctx != NULL )
		{
		FreeMem(ctx->datatype.ThdoAudio.templateArray,
				sizeof(TemplateRec) * kMaxTemplateCount);
		FreeMem(ctx, sizeof(*ctx));
		ctx = NULL;
		}

    return ctx;

} /* InitializeThdoAudioThread() */

/****************************************
 * Main audio Subscriber thread routine
*****************************************/

static void             SAudioSubscriberThread( int32 notUsed, SAudioCreationArgs *creationArgs );

/*=============================================================================
  =============================================================================
							Subscriber procedural interfaces
  =============================================================================
  =============================================================================*/


/******************************************************************************
|||	AUTODOC -public -class streaming -group startup -name NewSAudioSubscriber
|||	  Create a new SAudio subscriber thread.
|||
|||	  Synopsis
|||
|||	    Item NewSAudioSubscriber(DSStreamCBPtr streamCBPtr,
|||	         int32 deltaPriority, Item msgItem)
|||
|||	  Description
|||
|||	    Instantiate a new SAudioSubscriber.  This creates the subscriber
|||	    thread, waits for initialization to complete, *and* signs it up for
|||	    a subscription with the Data Stream parser thread. This returns the new
|||	    subscriber's request message port Item number or a negative error code.
|||	    The subscriber will clean itself up and exit when it receives a
|||	    kStreamOpClosing or kStreamOpAbort message. (If DSSubscribe() fails, the
|||	    streamer will send a kStreamOpAbort message to the subscriber-wannabe.)
|||	    
|||	    This temporarily allocates and frees a signal in the caller's thread.
|||	    Currently, 4096 bytes are allocated for the subscriber's stack.
|||
|||	  Arguments
|||
|||	    streamCBPtr
|||	        Pointer to control block for data stream. The new subscriber will
|||	        subscribe to this Streamer.
|||
|||	    deltaPriority
|||	        Execution priority of new SAudio subscriber thread relative to the
|||	        caller.
|||
|||	    msgItem
|||	        A Message Item to use temporarily for a synchronous DSSubscribe()
|||	        call.
|||
|||	  Return Value
|||	
|||	    A positive Item number of the new subscriber's request message
|||	    port or a negative error code indicating initialization errors, e.g.
|||	    kDSNoMemErr (couldn't allocate memory), kDSNoSignalErr (couldn't
|||	    allocate a signal), kDSSignalErr (problem sending or receiving a signal),
|||	    kDSRangeErr (parameter such as bitDepth out of range), or kDSInitErr
|||	    (initialization problem, e.g. allocating IOReq Items).
|||
|||	  Implementation
|||
|||	    Streaming library call.
|||
|||	  Associated Files
|||
|||	    <streaming/saudiosubscriber.h>, libsubscriber.a
|||
******************************************************************************/
Item NewSAudioSubscriber( DSStreamCBPtr streamCBPtr,
							int32 deltaPriority,
							Item msgItem )
	{
	Err					status;
    SAudioCreationArgs  creationArgs;
	uint32				signalBits;


	ADD_TRACE_L1( SATraceBufPtr, kTraceNewSubscriber, 0, 0, 0 );

	/* Setup the creation args, including a signal to synchronize with the
	 * completion of the subscriber's initialization. It will signal us when it
	 * is done initializing itself, successfully or not. */
	status = kDSNoSignalErr;
	creationArgs.creatorTask    = CURRENTTASKITEM;  /* cf. <kernel/kernel.h>, included by <streaming /threadhelper.h> */
	creationArgs.creatorSignal  = AllocSignal(0);
	if ( creationArgs.creatorSignal == 0 )
		goto CLEANUP;
	creationArgs.streamCBPtr        = streamCBPtr;
	creationArgs.creationStatus     = -1;
	creationArgs.requestPort        = -1;


	/* Create the thread that will handle all subscriber responsibilities. */
	status = NewThread( 
		(void *)(int32)&SAudioSubscriberThread,			/* thread entry point */
		4096, 											/* stack size */
		(int32)CURRENT_TASK_PRIORITY + deltaPriority,	/* priority */
		"SAudioSubscriber", 							/* name */
		0, 												/* first arg to the thread */
		&creationArgs);									/* second arg to the thread */

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
	 * failed, the 'ctx->creatorStatus' field will be set to a system
	 * result code. If this is >= 0 then initialization was successful. */
	status = creationArgs.creationStatus;
	if ( status < 0 )
		goto CLEANUP;

	status = DSSubscribe(msgItem, NULL,     /* a synchronous request */
				streamCBPtr,                /* stream context block */
				SNDS_CHUNK_TYPE,            /* subscriber data type */
				creationArgs.requestPort);  /* subscriber message port */
	if ( status < 0 )
		goto CLEANUP;

	return creationArgs.requestPort;        /* success! */

CLEANUP:
	/* Something went wrong.
	 * Release any resources we allocated and return the result status.
	 * The subscriber thread will clean up after itself. */
	if ( creationArgs.creatorSignal )	FreeSignal(creationArgs.creatorSignal);
	return status;
	}


/*=============================================================================
  =============================================================================
							The subscriber thread
  =============================================================================
  =============================================================================*/

/******************************************************************************
 * SAudioSubscriberThread.
 * This thread reads the subscriber message port for work requests and performs
 * appropriate actions.
 ******************************************************************************/
static void	SAudioSubscriberThread( int32 notUsed, SAudioCreationArgs *creationArgs )
	{
	SAudioContextPtr    ctx;
	Err					status;
	uint32				signalBits;
	uint32				anySignal;
	bool				fKeepRunning = true;

	TOUCH(notUsed);			/* avoid a compiler warning */

	/* Call a subroutine to perform all startup initialization. */
	ctx = InitializeThdoAudioThread(creationArgs );
	creationArgs = NULL;    /* can't access that memory anymore */
	TOUCH(creationArgs);	/* avoid a compiler warning */

    if ( ctx == NULL )
        goto FAILED;

	/* All resources are now allocated and ready to use. Our creator has
	 * been informed that we are ready to accept requests for work. All
	 * that's left to do is wait for work request messages to arrive.
	 */
	while ( fKeepRunning )
		{

		/* Note: This must be within the main loop because the signals are
		 * assigned when a soundspooler (one for each channel) is created in 
		 * InitSAudioChannel() .  InitSAudioChannel() is called whenever a new
		 * audio header chunk is received which may be anywhere within the stream. 
		 */
		anySignal = ctx->requestPortSignal | ctx->SpoolersSignalMask;

		ADD_TRACE_L1( SATraceBufPtr, kTraceWaitingOnSignal, -1, 
						(ctx->requestPortSignal | ctx->SpoolersSignalMask), 0 );

		signalBits = WaitSignal( anySignal );
		ADD_TRACE_L1( SATraceBufPtr, kTraceGotSignal, -1, signalBits, 0 );

		/*******************************************************/
		/* Check for and process any sample buffer completions */
		/*******************************************************/
		if ( signalBits & ctx->SpoolersSignalMask )
			{
			HandleCompletedBuffers( ctx, signalBits );
			}
	

		/********************************************************/
		/* Check for and process and incoming request messages. */
		/********************************************************/
		if ( signalBits & ctx->requestPortSignal )
			{
			status = ProcessRequestMsgs( ctx, &fKeepRunning );
			CHECK_NEG( "ProcessRequestMsgs", status );
			} /* if RequestPortSignal */

		} /* while (true) */


FAILED:

	/* Dispose all memory we allocated and clean up any static state.
	 * The OS will automatically reclaim the Items we allocated. */
	if ( ctx != NULL )
		{
		CloseAllAudioChannels(ctx);

		/* Remove DSP code to support ADPCM data playback, this
		 * instrument is loaded only if an ADPCM template is requested
		 * at LoadTemplates() time. */
		if ( ctx->datatype.ThdoAudio.decodeADPCMIns > 0 )
			UnloadInstrument( ctx->datatype.ThdoAudio.decodeADPCMIns );

		/* Get rid of any other memory we allocated */
		FreeMem(ctx->datatype.ThdoAudio.templateArray,
				sizeof(TemplateRec) * kMaxTemplateCount);
		FreeMem(ctx, sizeof(*ctx));
		} /* if( ctx != NULL ) */

	} /* SAudioSubscriberThread() */
