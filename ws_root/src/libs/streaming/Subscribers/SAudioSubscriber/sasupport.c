/******************************************************************************
**
**  @(#) sasupport.c 96/09/06 1.12
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
#include <streaming/msgutils.h>
#include <streaming/threadhelper.h>
#include <streaming/saudiosubscriber.h>

#include <streaming/sacontrolmsgs.h>
#include <streaming/subscriberutils.h>
#include <streaming/subscribertraceutils.h>

#include "sachannel.h"
#include "sasupport.h"
#include "sasoundspoolerinterface.h"
#include "mpadecoderinterface.h"

/*****************************************************************************
 * Compile switch implementations
 *****************************************************************************/
#if SAUDIO_TRACE_MAIN

	/* Declare the actual trace buffer.  Let it be global in case other 
	 * modules in the subscriber want to write into it. */
	TraceBuffer     SAudioTraceBuffer;
	TraceBufferPtr  SATraceBufPtr   = &SAudioTraceBuffer;

#endif /* SAUDIO_TRACE_MAIN */

#if (SAUDIO_TRACE_MAIN && SAUDIO_TRACE_SUPPORT)

	#ifndef TRACE_LEVEL     /* so it can be predefined via a compiler arg */
		#define TRACE_LEVEL     2
	#endif

	/* Allow for multiple levels of tracing */
	#if (TRACE_LEVEL >= 1)
		#define	ADD_TRACE_L1(bufPtr, event, chan, value, ptr)   \
						AddTrace(bufPtr, event, chan, value, ptr)
	#else
		#define	ADD_TRACE_L1(bufPtr, event, chan, value, ptr)
	#endif
	
	#if (TRACE_LEVEL >= 2)
		#define	ADD_TRACE_L2(bufPtr, event, chan, value, ptr)   \
						AddTrace(bufPtr, event, chan, value, ptr)
	#else
        #define	ADD_TRACE_L2(bufPtr, event, chan, value, ptr)
    #endif

#else   /* Trace is off */
	#define  ADD_TRACE_L1(bufPtr, event, chan, value, ptr)
	#define  ADD_TRACE_L2(bufPtr, event, chan, value, ptr)
#endif

/******************************
 * Local function prototypes
 ******************************/
static Err ProcessMPEGAudioDataChunk( SAudioContextPtr ctx, uint32 channelNumber, SubscriberMsgPtr subMsg );


/******************************************************************************
 * Deliver arriving MPEG audio data chunks to the decoder thread.
 ******************************************************************************/
static Err	ProcessMPEGAudioDataChunk( SAudioContextPtr ctx,
	uint32 channelNumber, SubscriberMsgPtr subMsg )
{
	Err					status = kDSNoErr;
	SubsChunkDataPtr	subsChunkPtr;

	DecoderMsgPtr		decoderMsg,
						decompressedMsg;

	if( subMsg )
	{
		decoderMsg = ALLOC_DECODER_MSG_POOL_MEM( ctx );
		if( decoderMsg != NULL )
		{
			subsChunkPtr = (SubsChunkDataPtr)subMsg->msg.data.buffer;
			
			/* Deliver this data to the MPEG Audio Decoder thread. */
			decoderMsg->channel			= channelNumber;
			decoderMsg->messageType		= compressedBfrMsg;
			decoderMsg->subMsgPtr		= subMsg;
			/* Remember the branch number for setting time later. */
			decoderMsg->branchNumber	= subMsg->msg.data.branchNumber;
			decoderMsg->buffer			= (uint32 *)( ((uint8 *)subsChunkPtr)
						+ MPEGAUDIO_FRAME_CHUNK_HEADER_SIZE);
			decoderMsg->size			= subsChunkPtr->chunkSize -
										  MPEGAUDIO_FRAME_CHUNK_HEADER_SIZE;
			decoderMsg->timeIsValidFlag	= ctx->datatype.MPEGAudio.lastTimeIsValid;
		
			decoderMsg->presentationTime	= subsChunkPtr->time;
		
#ifdef DEBUG_PRINT
			PRNT(("compressedBfrMsg addr: 0x%x\n", decoderMsg ));
			PRNT(("Compressed buffer @ addr: 0x%x is %d in size\n",
					decoderMsg->buffer, decoderMsg->size ));
#endif

			status = SendDecoderMsg( ctx, decoderMsg );
		} /* if( decoderMsg != NULL ) */

		else
		{
			PERR(("Not enough MPEGAudio decoder msgs in the pool\n"));
			/* [TBD] A better design would hold incoming subscriber new-data messages
			 * in a FIFO queue (a linked list) until decoder messages were available. */
			return kDSNoMsgErr;
		}

		/* Send any decompressed buffers we have in the memory
		 * pool for the decoder to store the decompressed data */
		decompressedMsg = ALLOC_DECOMPRESSED_BFR_MSG_POOL(ctx); 
		if( decompressedMsg != NULL )
		{
			ADD_TRACE_L1( SATraceBufPtr, kTraceSendDecompressedBfrMsg,
						decompressedMsg->channel, 0, decompressedMsg );
			status = SendDecoderMsg( ctx, decompressedMsg );
			CHECK_NEG("SendDecoderMsg", status );
		}
	} /* if( subMsg ) */ 

	return status;

} /* ProcessMPEGAudioDataChunk() */

/******************************************************************************
 * ProcessRequestMsgs
 * Check for and process incoming request messages.
******************************************************************************/
Err ProcessRequestMsgs( SAudioContextPtr ctx, bool *fKeepRunning )
{
	Err					status	= kDSNoErr;
	Item				msgItem;
	SubscriberMsgPtr	subMsg;
	Message				*messagePtr;

	/* Process any new requests for service as determined by
	 * the incoming message data.
	 */
	while( (msgItem = GetMsg(ctx->requestPort)) > 0 )
	{	 
		messagePtr	= MESSAGE(msgItem);
		subMsg		= messagePtr->msg_DataPtr;

		switch ( subMsg->whatToDo )
		{
			case kStreamOpData:		/* new data has arrived */	
				ADD_TRACE_L1( SATraceBufPtr, kTraceDataMsg, -1,
					0, subMsg );
				status = DoData( ctx, subMsg );		
				CHECK_NEG("DoData", status );
				break;


			case kStreamOpGetChan:	/* get logical channel status */
				ADD_TRACE_L1( SATraceBufPtr, kTraceGetChanMsg,
					subMsg->msg.channel.number, 0, subMsg );
				status = DoGetChan( ctx, subMsg );		
				CHECK_NEG("DoGetChan", status );
				break;


			case kStreamOpSetChan:	/* set logical channel status */
				ADD_TRACE_L1( SATraceBufPtr, kTraceSetChanMsg,
					subMsg->msg.channel.number,
					subMsg->msg.channel.status, subMsg );
				status = DoSetChan( ctx, subMsg );		
				CHECK_NEG("DoSetChan", status );
				break;


			case kStreamOpControl:	/* perform subscriber defined
									 * function */
				ADD_TRACE_L1( SATraceBufPtr, kTraceControlMsg,
					-1, 0, subMsg );
				status = DoControl( ctx, subMsg );		
				CHECK_NEG("DoControl", status );
				break;


			case kStreamOpSync:		/* clock stream resynched
									 * the clock */
				ADD_TRACE_L1( SATraceBufPtr, kTraceSyncMsg, -1,
					0, subMsg );
				status = DoSync( ctx, subMsg );		
				CHECK_NEG("DoSync", status );
				break;


			case kStreamOpOpening:	/* one time initialization
									 * call from DSH */
				ADD_TRACE_L1( SATraceBufPtr, kTraceOpeningMsg,
					-1, 0, subMsg );
				status = DoOpening( ctx, subMsg );		
				CHECK_NEG("DoOpening", status );
				break;


			case kStreamOpClosing:	/* stream is being closed */
				ADD_TRACE_L1( SATraceBufPtr, kStreamOpClosing,
					-1, 0, subMsg );
				status = DoClosing( ctx, subMsg );		
				CHECK_NEG("DoClosing", status );
				DumpTraceData();
				*fKeepRunning = false;
				break;


			case kStreamOpStop:		/* stream is being stopped */
				ADD_TRACE_L1( SATraceBufPtr, kTraceStopMsg, -1,
					0, subMsg );
				status = DoStop( ctx, subMsg );
				CHECK_NEG("DoStop", status );
				break;


			case kStreamOpStart:	/* stream is being started */
				ADD_TRACE_L1( SATraceBufPtr, kTraceStartMsg, -1,
					0, subMsg );
				status = DoStart( ctx, subMsg );
				CHECK_NEG("DoStart", status );
				break;


			case kStreamOpEOF:		/* physical EOF on data, no
									 * more to come */
				ADD_TRACE_L1( SATraceBufPtr, kTraceEOFMsg, -1,
					0, subMsg );
				status = DoEOF( ctx, subMsg );		
				CHECK_NEG("DoEOF", status );
				break;


			case kStreamOpAbort:	/* somebody gave up, stream
									 * is aborted */
				ADD_TRACE_L1( SATraceBufPtr, kTraceAbortMsg, -1,
					0, subMsg );
				status = DoAbort( ctx, subMsg );		
				CHECK_NEG("DoAbort", status );
				DumpTraceData();
				status = kDSAbortErr;
				TOUCH( status );
				*fKeepRunning = false;
				break;

			case kStreamOpBranch:	/* data discontinuity */
				ADD_TRACE_L1( SATraceBufPtr, kTraceBranchMsg, -1,
					0, subMsg );
				status = DoBranch(ctx, subMsg);
				break;

			default:
				;
		} /* switch whattodo */

		/* Reply to the request we just handled unless this is
		 * a "data arrived" message. (They're replied to
		 * asynchronously when the data is actually consumed.) */
		if ( subMsg->whatToDo != kStreamOpData )
		{
			status = ReplyToSubscriberMsg(subMsg, status);
			CHECK_NEG( "ReplyToSubscriberMsg", status );
		}

	} /* while GetMsg */

	return status;
		
} /* ProcessRequestMsgs() */

/*=============================================================================*/
/* Do common initialization for an SAudio or MPEG audio subscriber thread. */
Err InitializeSAudioThread( SAudioContextPtr ctx )
	{
	Err					status;
	int32				index;

	/* Create the message port where this subscriber will accept
	 * request messages from the Streamer and client threads. */
	status = NewMsgPort(&ctx->requestPortSignal);
	if ( status < 0 )
	    goto FAILED;
	ctx->requestPort = status;

    /* ***  ***  ***  ***  ***  ***  ***
    **  Audio specific initializations
    ** ***  ***  ***  ***  ***  ***  ***/

	/* Load the output instrument template. This gets used
	 * every time a channel is opened to create an output
	 * instrument for the channel if one does not already exist. */
	status = ctx->outputTemplateItem =
		CreateMixerTemplate( MIXER_SPEC, NULL );
	if ( status < 0 )
		goto FAILED;
	
	/* Initialize once-only channel related things. 
	 *
	 * NOTE: CurrentAmp is initalized to -1 so you can set the amplitude
	 * to zero with a control message and not have it be
	 * overided by the initialAmplitude setting in the .saudio 
	 * header.  i.e. to distinguish between amp 0 and "not yet set".
	 * The rest of the fields have already been initialized to zeroes
	 * when the structure was allocated. */
	for ( index = 0; index < SA_SUBS_MAX_CHANNELS; index++ )
		ctx->channel[index].channelOutput.currentAmp	= -1;

    /* Since we got here, creation must've been successful! */
    status = kDSNoErr;

FAILED:

    return status;

	} /* InitializeSAudioThread() */

/******************************************************************************
 * Send decompressed MPEG audio buffers to the decoder.
******************************************************************************/
Err SendDecoderMsg( SAudioContextPtr ctx, DecoderMsgPtr decoderMsg )
{
	Err				status;
	Item			decoderPort = ctx->datatype.MPEGAudio.decoderPort;

	ADD_TRACE_L1( SATraceBufPtr, kSendDecoderRequestMsg,
		decoderMsg->channel, decoderMsg->messageType, decoderMsg );

	/* Send the message to the decoder. */
	status = SendMsg( decoderPort, decoderMsg->msgItem, decoderMsg,
			sizeof(*decoderMsg) );
	CHECK_NEG( "SendMsg", status );
	ctx->datatype.MPEGAudio.pendingReplies++;

#ifdef DEBUG_PRINT
		PRNT(("decoderMsg addr: 0x%x, messageType %d\n", decoderMsg,
				decoderMsg->messageType ));
		PRNT(("buffer addr: 0x%x is %d in size\n",
			decoderMsg->buffer, decoderMsg->size ));
#endif

	return status;

} /* SendDecoderMsg() */

/*=============================================================================
  =============================================================================
				High level interfaces used by the main thread to process
									incoming messages. 
  =============================================================================
  =============================================================================*/

/******************************************************************************
 * Process arriving data chunks. If message points to sample data, then queue it
 * to the MPEG audio decoder thread or directly to the Sound Spooler. If message
 * points to header, initialize the channel that the header was received on.
 * In any case, either queue the message for later processing, or return it to the
 * streamer now.
 *
 * NOTE:	Fields 'streamChunkPtr->streamChunkType' and 'streamChunkPtr->streamChunkSize'
 *			contain the 4 character stream data type and size, in BYTES, of the actual
 *			chunk data.
 ******************************************************************************/
Err	DoData( SAudioContextPtr ctx, SubscriberMsgPtr subMsg )
{
	Err						status;
	const SubsChunkDataPtr	subsChunkPtr = 
		(SubsChunkDataPtr)subMsg->msg.data.buffer;
	uint32					channelNumber = subsChunkPtr->channel;

	const uint32			chunkSubType =
		( subsChunkPtr->chunkSize >= sizeof(SubsChunkData) ) ?
		 	subsChunkPtr->subChunkType : 0;

	/* Incoming message could be data or header chunk */
	switch (chunkSubType)
		{
		case SSMP_CHUNK_TYPE:	/* Sample data */

			ADD_TRACE_L2( SATraceBufPtr, kNewDataMsgArrived,
							channelNumber, 0, subMsg );
            QueueNewAudioBuffer( ctx, subMsg );
            BeginSAudioPlaybackIfAppropriate( ctx, channelNumber );
            status = kDSNoErr;
            break;

		/* MPEG audio chunk can arrive containing one whole frame,
		 * or 4 whole frames, or frames split into parts per chunk. */
		case MA1FRME_CHUNK_SUBTYPE:	/* MPEG audio sample data */
		case MA2FRME_CHUNK_SUBTYPE:	/* MPEG audio sample data */
		case MA3FRME_CHUNK_SUBTYPE:	/* MPEG audio sample data */
		case MA4FRME_CHUNK_SUBTYPE:	/* MPEG audio sample data */
		case MA1FRME_SPLIT_START_CHUNK_SUBTYPE:
		case MA2FRME_SPLIT_START_CHUNK_SUBTYPE:
		case MA3FRME_SPLIT_START_CHUNK_SUBTYPE:
		case MA4FRME_SPLIT_START_CHUNK_SUBTYPE:
		case MA1FRME_SPLIT_END_CHUNK_SUBTYPE:
		case MA2FRME_SPLIT_END_CHUNK_SUBTYPE:
		case MA3FRME_SPLIT_END_CHUNK_SUBTYPE:
		case MA4FRME_SPLIT_END_CHUNK_SUBTYPE:
			ADD_TRACE_L2( SATraceBufPtr, kNewDataMsgArrived,
							channelNumber, subsChunkPtr->time, subMsg );
			/* Send the new data to the decoder thread */
			status = ProcessMPEGAudioDataChunk( ctx, channelNumber, subMsg );
			if ( status < 0 )
				{
				ERROR_RESULT_STATUS("ProcessMPEGAudioDataChunk", status);
				status = ReplyToSubscriberMsg(subMsg, status);
				}
            break;

		case SHDR_CHUNK_TYPE:		/* Sample header structure */
		case MAHDR_CHUNK_SUBTYPE:	/* MPEG audio Sample header structure */

			ADD_TRACE_L2( SATraceBufPtr, kNewHeaderMsgArrived,
							channelNumber, subsChunkPtr->time, subMsg );
			status = InitSAudioChannel(ctx, subsChunkPtr);
			if (status < 0) 
				{
				ERROR_RESULT_STATUS("InitSAudioChannel", status);
				PERR((" channel #%ld\n", channelNumber));

				CloseSAudioChannel( ctx, channelNumber );
				
				/* Nothing all that terrible happened so Reset the
				 * status so we won't get aborted by the streamer when
				 * it looks at the replied message. */
				status = kDSNoErr;
				}

			/* Because the subscriber considers audio headers and audio
			 * data to both be "Data" messages we have to reply to the
			 * header chunk message here if init was successful. */
			status = ReplyToSubscriberMsg(subMsg, status); 
			break;
		

		default:
			/* For unrecognized or hosed chunk subtypes */
			status = ReplyToSubscriberMsg(subMsg, kDSNoErr); 
			break;
		
		}	/* switch */
		
	return status;
	}

		
/******************************************************************************
 * Routine to set the status bits of a given channel.
 ******************************************************************************/		
Err	DoSetChan( SAudioContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	uint32				chanNumber = subMsg->msg.channel.number;
	SAudioChannelPtr	chanPtr;
	int32				wasEnabled;
	uint32				mask;
	
	if ( chanNumber < SA_SUBS_MAX_CHANNELS )
		{
		chanPtr		= ctx->channel + chanNumber;
		
		/* Allow only bits that are Read/Write to be set by this call.
		 *
		 * NOTE: 	Any special actions that might need to be taken as as
		 *			result of bits being set or cleared should be taken
		 *			now. If the only purpose served by status bits is to 
		 *			enable or disable features, or some other communication,
		 *			then the following is all that is necessary. */
		wasEnabled = IsChanEnabled(chanPtr);

		mask = subMsg->msg.channel.mask & ~CHAN_SYSBITS; 
		chanPtr->status = subMsg->msg.channel.status & mask | chanPtr->status & ~mask;

		/* If the channel is becoming disabled, flush data and reset; if it is
		 * becoming enabled, start it up. */
		if (wasEnabled && (!IsChanEnabled(chanPtr)))
			CloseSAudioChannel(ctx, chanNumber);
		else if (!wasEnabled && (IsChanEnabled(chanPtr)))
			StartSAudioChannel(ctx, chanNumber, FALSE);

		return kDSNoErr;
		}

	return kDSChanOutOfRangeErr;
	}

		
/******************************************************************************
 * Routine to return the status bits of a given channel.
 ******************************************************************************/
Err	DoGetChan( SAudioContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	Err			status = kDSChanOutOfRangeErr;
	uint32		channelNumber = subMsg->msg.channel.number;

	if ( channelNumber < SA_SUBS_MAX_CHANNELS )
		status = ctx->channel[ channelNumber ].status;

	return status;
	}

		
/******************************************************************************
 * Routine to perform an arbitrary subscriber defined action. 
 ******************************************************************************/		
Err	DoControl( SAudioContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	int32				status;
	int32				userWhatToDo;
	SAudioCtlBlockPtr	ctlBlockPtr;

	userWhatToDo = subMsg->msg.control.controlArg1;
	ctlBlockPtr = (SAudioCtlBlockPtr) subMsg->msg.control.controlArg2;

	switch ( userWhatToDo )
		{
		case kSAudioCtlOpLoadTemplates:
			status = LoadTemplates( ctx, ctlBlockPtr->loadTemplates.tagListPtr,
									kMaxTemplateCount );
			break;

		case kSAudioCtlOpSetVol:
			if ( ctlBlockPtr->amplitude.channelNumber < SA_SUBS_MAX_CHANNELS )
				{
				status = SetSAudioChannelAmplitude( ctx,
								ctlBlockPtr->amplitude.channelNumber, 
								ctlBlockPtr->amplitude.value );
				}
			else
				status = kDSChanOutOfRangeErr;
			break;
			
		case kSAudioCtlOpSetPan:
			if ( ctlBlockPtr->pan.channelNumber < SA_SUBS_MAX_CHANNELS )
				{
				status = SetSAudioChannelPan( ctx,
								ctlBlockPtr->pan.channelNumber, 
								ctlBlockPtr->pan.value );
				}
			else
				status = kDSChanOutOfRangeErr;
			break;

		case kSAudioCtlOpGetVol:
			if ( ctlBlockPtr->amplitude.channelNumber < SA_SUBS_MAX_CHANNELS )
				{
				status = GetSAudioChannelAmplitude( ctx,
								ctlBlockPtr->amplitude.channelNumber, 
								&ctlBlockPtr->amplitude.value);
				}
			else
				status = kDSChanOutOfRangeErr;
			break;
			
		case kSAudioCtlOpGetPan:
			if ( ctlBlockPtr->pan.channelNumber < SA_SUBS_MAX_CHANNELS )
				{
				status = GetSAudioChannelPan( ctx,
								ctlBlockPtr->pan.channelNumber, 
								&ctlBlockPtr->pan.value);
				}
			else
				status = kDSChanOutOfRangeErr;
			break;

		case kSAudioCtlOpMute:
			if ( ctlBlockPtr->mute.channelNumber < SA_SUBS_MAX_CHANNELS )
				{
				status = MuteSAudioChannel( ctx,
						ctlBlockPtr->mute.channelNumber, USER_MUTE );
				}
			else
				status = kDSChanOutOfRangeErr;
			break;

		case kSAudioCtlOpUnMute:
			if ( ctlBlockPtr->unMute.channelNumber < SA_SUBS_MAX_CHANNELS )
				{
				status = UnMuteSAudioChannel( ctx,
					ctlBlockPtr->unMute.channelNumber, USER_UNMUTE );
				}
			else
				status = kDSChanOutOfRangeErr;
			break;

		case kSAudioCtlOpSetClockChan:
			if ( ctlBlockPtr->clock.channelNumber < SA_SUBS_MAX_CHANNELS )
				{
				ctx->clockChannel = ctlBlockPtr->clock.channelNumber;
				status = 0;			/* these are no-op's for now... */
				}
			else
				status = kDSChanOutOfRangeErr;
			break;

		case kSAudioCtlOpGetClockChan:
			if ( ctlBlockPtr->clock.channelNumber < SA_SUBS_MAX_CHANNELS )
				{
				ctlBlockPtr->clock.channelNumber = ctx->clockChannel;
				status = kDSNoErr;			
				}
			else
				status = kDSChanOutOfRangeErr;
			break;

		case kSAudioCtlOpCloseChannel:
			status = CloseSAudioChannel( ctx,
						ctlBlockPtr->CloseSAudioChannel.channelNumber );
			break;

		case kSAudioCtlOpFlushChannel:
			status = FlushSAudioChannel( ctx,
						ctlBlockPtr->FlushSAudioChannel.channelNumber );
			break;

		default:
			status = 0;	/* ignore unknown control messages for now... */
			break;
		}
	
	return status;
	}

		
/******************************************************************************
 * Routine to do whatever is necessary when a subscriber is added to a stream, typically
 * just after the stream is opened.
 ******************************************************************************/		
Err	DoOpening( SAudioContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	TOUCH(ctx);
	TOUCH(subMsg);

	return kDSNoErr;
	}

/******************************************************************************
 * Write out the trace logged data including event-completion pairs.
 ******************************************************************************/		
#if SAUDIO_TRACE_MAIN
void DumpTraceData(void)
	{
	Err		status;
	
	status = DumpRawTraceBuffer(SATraceBufPtr, "SAudioRawTrace.txt");
	CHECK_NEG("DumpRawTraceBuffer", status);

	status = DumpTraceCompletionStats( SATraceBufPtr,
					kSendDecoderRequestMsg,
					kGotDecoderReply, "SAudioTractStats.txt" );
	CHECK_NEG("DumpTraceCompletionStats", status);
	}
#endif

/******************************************************************************
 * Routine to close down an open subscription.
 ******************************************************************************/		
Err	DoClosing( SAudioContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	Err		status;
	
	TOUCH(subMsg);

	status = CloseAllAudioChannels(ctx);


	return status;
	}


/******************************************************************************
 * Routine to start all channels for this subscription.
 ******************************************************************************/		
Err	DoStart( SAudioContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	uint32		channelNumber;
	const bool	fFlush = (subMsg->msg.start.options & SOPT_FLUSH) != 0;

	/* Start all channels for this subscription. */
	for ( channelNumber = 0; channelNumber < SA_SUBS_MAX_CHANNELS;
			channelNumber++ )
		StartSAudioChannel( ctx, channelNumber, fFlush );
		
	return kDSNoErr;
	}


/******************************************************************************
 * Control-msg routine to stop all channels for this subscription.
 ******************************************************************************/		
Err	DoStop( SAudioContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	return StopAllAudioChannels(ctx,
		(subMsg->msg.stop.options & SOPT_FLUSH) != 0);
	}


/******************************************************************************
 * Flush everything waiting and queued to the AudioFolio under the
 * assumption that we have just arrived at a branch point and should be ready
 * to deal with all new data for this stream.
 ******************************************************************************/		
Err	DoSync( SAudioContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	TOUCH(subMsg);
	
	return FlushAllAudioChannels(ctx);
	}


/******************************************************************************
 * Control-msg routine to respond to an EOF message.
 ******************************************************************************/		
Err	DoEOF( SAudioContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	TOUCH(ctx);
	TOUCH(subMsg);

	return kDSNoErr;
	}


/******************************************************************************
 * Routine to kill all output, return all queued buffers, and generally stop everything.
 ******************************************************************************/		
Err	DoAbort( SAudioContextPtr ctx, SubscriberMsgPtr subMsg )
	{
	TOUCH(subMsg);

	return CloseAllAudioChannels(ctx);
	}

/*******************************************************************************
 * Process a stream-branched message: If it's a flush-branch, then flush all
 * data now, else handle a discontinuity at this point in the stream: for DSP
 * audio there's nothing we have to do, but [TBD] for MPEG audio we must enqueue
 * a "branch" request "plug" in the data pipeline of all active channels.
 *
 * [TBD] For a non-flush branch, we might want to reply to subMsg *after*
 * all channels process the discontinuity in their streams. Then the streamer
 * could detect when all subscribers have completed the branch, and inform the
 * client application.
 *******************************************************************************/
Err	DoBranch(SAudioContextPtr ctx, SubscriberMsgPtr subMsg)
{
	Err		status = kDSNoErr;

	if ( subMsg->msg.branch.options & SOPT_FLUSH )
			return FlushAllAudioChannels(ctx);

	return status;
}

