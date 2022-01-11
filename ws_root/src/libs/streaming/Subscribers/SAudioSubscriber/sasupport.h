/******************************************************************************
**
**  @(#) sasupport.h 96/04/02 1.4
**
******************************************************************************/
#ifndef SASUPPORT_H
#define SASUPPORT_H

#include <string.h>
#include <stdlib.h>				
#include <stdio.h>

#include <streaming/datastream.h>
#include <streaming/subscriberutils.h>
#include <streaming/subscribertraceutils.h>

#include "mpadecoderinterface.h"

/*****************************************************************************
 * Compile switch implementations
 *****************************************************************************/

#if SAUDIO_TRACE_MAIN

	/* Locate the buffer.  It's define in sasupport.c */
	extern  TraceBufferPtr  SATraceBufPtr;

#endif

/************************************
 * Local types and constants
 ************************************/

/* This structure is used temporarily for communication between the spawning
 * (client) process and the nascent subscriber.
 *
 * Thread-interlock is handled as follows: NewSAudioSubscriber() or
 * NewMPEGAudioSubscriber() allocates this struct on the stack, fills it
 * in, and passes it's address as an arg to the subscriber thread. The
 * subscriber then owns access to it until sending a signal back to
 * the spawning thread (using the first 2 args in the struct).  Before
 * sending this signal, the subscriber fills in the "output" fields of
 * the struct, thus returning its creation status result code and request msg
 * port Item. After sending this signal, the subscriber may no longer
 * touch this memory as NewxAudioSubscriber() will deallocate it. */
typedef struct SAudioCreationArgs {
    /* --- input parameters from the client to the new subscriber --- */
    Item			creatorTask;	/* who to signal when done initializing */
    uint32			creatorSignal;	/* signal to send when done initializing */
    DSStreamCBPtr	streamCBPtr;	/* stream this subscriber belongs to */
 
    /* --- output results from spawing the new subscriber --- */
    int32			creationStatus;	/* < 0 ==> failure */
    Item			requestPort;	/* new thread's request msg port */
	uint32			numberOfBuffers;/* number of buffers for storing
									 * MPEG audio decompressed data */
    } SAudioCreationArgs;

/*****************************************
 * trace log routine
 *****************************************/
#if SAUDIO_TRACE_MAIN
	void DumpTraceData( void );
#else
	#define DumpTraceData()
#endif

/*****************************************
 * Main Subscriber initalization routine
 *****************************************/
Err    InitializeSAudioThread( SAudioContextPtr ctx );

/* Send request message to the deocoder */
Err	SendDecoderMsg( SAudioContextPtr ctx, DecoderMsgPtr decoderMsg );

/***********************************************************************/
/* Routines to handle incoming messages from the stream parser thread  */
/***********************************************************************/
Err ProcessRequestMsgs( SAudioContextPtr ctx, bool *fKeepRunning );
Err	DoData( SAudioContextPtr ctx, SubscriberMsgPtr subMsg );
Err	DoGetChan( SAudioContextPtr ctx, SubscriberMsgPtr subMsg );
Err	DoSetChan( SAudioContextPtr ctx, SubscriberMsgPtr subMsg );
Err	DoControl( SAudioContextPtr ctx, SubscriberMsgPtr subMsg );
Err	DoOpening( SAudioContextPtr ctx, SubscriberMsgPtr subMsg );
Err	DoClosing( SAudioContextPtr ctx, SubscriberMsgPtr subMsg );
Err	DoStart( SAudioContextPtr ctx, SubscriberMsgPtr subMsg );
Err	DoStop( SAudioContextPtr ctx, SubscriberMsgPtr subMsg );
Err	DoSync( SAudioContextPtr ctx, SubscriberMsgPtr subMsg );
Err	DoEOF( SAudioContextPtr ctx, SubscriberMsgPtr subMsg );
Err	DoAbort( SAudioContextPtr ctx, SubscriberMsgPtr subMsg );
Err DoBranch(SAudioContextPtr ctx, SubscriberMsgPtr subMsg);

#endif /* SASUPPORT_H */
