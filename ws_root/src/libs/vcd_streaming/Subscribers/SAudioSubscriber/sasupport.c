/******************************************************************************
**
**  @(#) sasupport.c 96/06/11 1.4
**
******************************************************************************/

#include <string.h>
#include <stdlib.h>                             
#include <stdio.h>
#include <audio/audio.h>
#include <kernel/debug.h>               /* for print macros: PERR, PRNT, CHECK_NEG */
#include <kernel/mem.h>

#include <video_cd_streaming/datastreamlib.h>
#include <streaming/dserror.h>  /* for DS error codes */
#include <streaming/mempool.h>
#include <streaming/msgutils.h>
#include <streaming/threadhelper.h>
#include <video_cd_streaming/saudiosubscriber.h>
#include <video_cd_streaming/mpegaudiosubscriber.h>

#include <video_cd_streaming/sacontrolmsgs.h>
#include <video_cd_streaming/subscriberutils.h>
#include <streaming/subscribertraceutils.h>
#include <video_cd_streaming/mpegutils.h>


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
                #define ADD_TRACE_L1(bufPtr, event, chan, value, ptr)   \
                                                AddTrace(bufPtr, event, chan, value, ptr)
        #else
                #define ADD_TRACE_L1(bufPtr, event, chan, value, ptr)
        #endif
        
        #if (TRACE_LEVEL >= 2)
                #define ADD_TRACE_L2(bufPtr, event, chan, value, ptr)   \
                                                AddTrace(bufPtr, event, chan, value, ptr)
        #else
        #define ADD_TRACE_L2(bufPtr, event, chan, value, ptr)
    #endif

#else   /* Trace is off */
        #define  ADD_TRACE_L1(bufPtr, event, chan, value, ptr)
        #define  ADD_TRACE_L2(bufPtr, event, chan, value, ptr)
#endif

/******************************
 * Local function prototypes
 ******************************/
static Err ProcessMPEGAudioDataChunk( SAudioContextPtr ctx, uint32 channelNumber, SubscriberMsgPtr subMsg );

static Err DoHeaderInfo(SAudioContextPtr ctx, SubscriberMsgPtr subMsg);

/******************************************************************************
 * Deliver arriving MPEG audio data chunks to the decoder thread.
 ******************************************************************************/
static Err      ProcessMPEGAudioDataChunk( SAudioContextPtr ctx,
					  uint32 channelNumber, SubscriberMsgPtr subMsg )
{
  Err status = kDSNoErr;

  DecoderMsgPtr		decoderMsg, decompressedMsg;

  if( subMsg )
    {
      decoderMsg = ALLOC_DECODER_MSG_POOL_MEM( ctx );
      if( decoderMsg != NULL )
	{
	  /* Deliver this data to the MPEG Audio Decoder thread. */
	  decoderMsg->channel                     = channelNumber;
	  decoderMsg->messageType         = compressedBfrMsg;
	  decoderMsg->subMsgPtr           = subMsg;
	  /* Remember the branch number for setting time later. */
	  decoderMsg->branchNumber        = subMsg->msg.data.branchNumber;
	  decoderMsg->buffer = (uint32 *)subMsg->msg.data.buffer;
	  decoderMsg->size = subMsg->msg.data.bufferSize;
	  decoderMsg->timeIsValidFlag     = subMsg->msg.data.ptsValid;
	  decoderMsg->presentationTime    = MPEGTimestampToAudioTicks(subMsg->msg.data.pts);
	  
#ifdef DEBUG_PRINT
/*	  PRNT(("compressedBfrMsg addr: 0x%x\n", decoderMsg )); */
	  PRNT(("Compressed buffer @ addr: 0x%x is %d in size\n",
		decoderMsg->buffer, decoderMsg->size ));
#endif

	  status = SendDecoderMsg( ctx, decoderMsg );
	} /* if( decoderMsg != NULL ) */
      else
	{
	  PERR(( "Not enough decoder msgs in the pool\n"));
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
  Err                  		status  = kDSNoErr;
  Item                     	msgItem;
  SubscriberMsgPtr        	subMsg;
  Message                 	*messagePtr;

  /* Process any new requests for service as determined by
   * the incoming message data.
   */
	while( (msgItem = GetMsg(ctx->requestPort)) > 0 )
	{
	 	messagePtr      = MESSAGE(msgItem);
	  	subMsg          = messagePtr->msg_DataPtr;

		switch ( subMsg->whatToDo ) {
		case kStreamOpData:	/* new data has arrived */      
			ADD_TRACE_L1( SATraceBufPtr, kTraceDataMsg, -1,
			       0, subMsg );
			if ( !(ctx->status & MPEG_CHAN_BYPASS) )
			{
			  status = DoData( ctx, subMsg );         
			  CHECK_NEG("DoData", status );
			}
			break;
		
		
		case kStreamOpGetChan:  /* get logical channel status */
			ADD_TRACE_L1( SATraceBufPtr, kTraceGetChanMsg,
			       subMsg->msg.channel.number, 0, subMsg );
			status = DoGetChan( ctx, subMsg );              
			CHECK_NEG("DoGetChan", status );
			break;
			
		
		case kStreamOpSetChan:  /* set logical channel status */
			ADD_TRACE_L1( SATraceBufPtr, kTraceSetChanMsg,
			       subMsg->msg.channel.number,
			       subMsg->msg.channel.status, subMsg );
			status = DoSetChan( ctx, subMsg );              
			CHECK_NEG("DoSetChan", status );
			break;
		
		
		case kStreamOpControl:  /* perform subscriber defined
					 * function */
			ADD_TRACE_L1( SATraceBufPtr, kTraceControlMsg,
			       -1, 0, subMsg );
			status = DoControl( ctx, subMsg );              
			CHECK_NEG("DoControl", status );
			break;
		
		case kStreamOpSync:	/* clock stream resynched
					 		 * the clock */
			ADD_TRACE_L1( SATraceBufPtr, kTraceSyncMsg, -1,
			       0, subMsg );
			status = DoSync( ctx, subMsg );         
			CHECK_NEG("DoSync", status );
			break;
		
		case kStreamOpOpening:  /* one time initialization
					 			 * call from DSH */
			ADD_TRACE_L1( SATraceBufPtr, kTraceOpeningMsg,
			       -1, 0, subMsg );
			status = DoOpening( ctx, subMsg );              
			CHECK_NEG("DoOpening", status );
			break;
		
		case kStreamOpClosing:  /* stream is being closed */
			ADD_TRACE_L1( SATraceBufPtr, kStreamOpClosing,
			       -1, 0, subMsg );
			status = DoClosing( ctx, subMsg );              
			CHECK_NEG("DoClosing", status );
			DumpTraceData();
			*fKeepRunning = false;
			break;
		
		case kStreamOpStop:	/* stream is being stopped */
			ADD_TRACE_L1( SATraceBufPtr, kTraceStopMsg, -1,
			       0, subMsg );
			status = DoStop( ctx, subMsg );
			CHECK_NEG("DoStop", status );
			break;
		
		case kStreamOpStart:    /* stream is being started */
			ADD_TRACE_L1( SATraceBufPtr, kTraceStartMsg, -1,
			       0, subMsg );
			status = DoStart( ctx, subMsg );
			CHECK_NEG("DoStart", status );
			break;
			
		case kStreamOpEOF:	/* physical EOF on data, no
					 		 * more to come */
			ADD_TRACE_L1( SATraceBufPtr, kTraceEOFMsg, -1,
			       0, subMsg );
			status = DoEOF( ctx, subMsg );          
			CHECK_NEG("DoEOF", status );
			break;
			
		case kStreamOpAbort:    /* somebody gave up, stream
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
			
		case kStreamOpBranch:   /* data discontinuity */
			ADD_TRACE_L1( SATraceBufPtr, kTraceBranchMsg, -1,
			       0, subMsg );
			if ( !(ctx->status & MPEG_CHAN_BYPASS) )
			status = DoBranch(ctx, subMsg);
			break;
		
		case kStreamOpHeaderInfo:
			ADD_TRACE_L1( SATraceBufPtr, kTraceBranchMsg, -1,
			       0, subMsg );
			status = DoHeaderInfo(ctx, subMsg);
			break;
		
		case kStreamOpShuttle:
			/* enter "bypass" mode. Flush buffers, turn on "bypass" */
			if ((ctx->status & MPEG_CHAN_BYPASS) == 0) {
				ctx->status |= MPEG_CHAN_BYPASS;
				status = FlushSAudioChannel( ctx, 0);
			}
		  	break;
		case kStreamOpPlay:
			/* exit "bypass" mode. */
			if (ctx->status & MPEG_CHAN_BYPASS) {
				ctx->status &= ~MPEG_CHAN_BYPASS;
				status = StartSAudioChannel(ctx, 0, FALSE);
		    }
		  break;
		
		default:
		  ;
	} /* switch whattodo */

      /* Reply to the request we just handled unless this is
       * a "data arrived" message. (They're replied to
       * asynchronously when the data is actually consumed.) */
	if ( (subMsg->whatToDo == kStreamOpData && ctx->status & MPEG_CHAN_BYPASS) ||
		 (subMsg->whatToDo != kStreamOpData) ) {
		status = ReplyToSubscriberMsg(subMsg, status);
		CHECK_NEG( "ReplyToSubscriberMsg", status );
	} 

    } /* while GetMsg */

  return status;
                
} /* ProcessRequestMsgs() */

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

Err InitializeSAudioThread( SAudioContextPtr ctx )
        {
        Err                                     status;
        int32                           index;

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
                ctx->channel[index].channelOutput.currentAmp    = -1;

	/* Always assume that we're not in bypass mode */
	ctx->status &= ~MPEG_CHAN_BYPASS;

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
        Err                             status;
        Item                    decoderPort = ctx->datatype.MPEGAudio.decoderPort;

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
 * Routine to process arriving data chunks. Always assume MPEG Audio data
 * MPAU_CHUNK_TYPE.  Queue it to the audio folio.
 *
 ******************************************************************************/
Err     DoData( SAudioContextPtr ctx, SubscriberMsgPtr subMsg )
{
  Err    status;
  uint32 channelNumber = 0; /* Always assume channel 0 (W. Hsu 5-9-96) */

  ADD_TRACE_L2( SATraceBufPtr, kNewDataMsgArrived,
	       channelNumber, subMsg->msg.data.pts, subMsg );
  /* Send the new data to the decoder thread */
  status = ProcessMPEGAudioDataChunk( ctx, channelNumber, subMsg );
  CHECK_NEG(" ProcessMPEGAudioDataChunk", status );

#if 0 /* Always assume MPAU_CHUNK_TYPE (W. Hsu 5-9-96) */
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
      CHECK_NEG(" ProcessMPEGAudioDataChunk", status );
      break;

    case SHDR_CHUNK_TYPE:	/* Sample header structure */
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
                
    } /* switch */
#endif /* #if 0  Always assume MPAU_CHUNK_TYPE */
                
  return status;
}

                
/******************************************************************************
 * Routine to set the status bits of a given channel.
 ******************************************************************************/                
Err     DoSetChan( SAudioContextPtr ctx, SubscriberMsgPtr subMsg )
        {
        uint32                          chanNumber = subMsg->msg.channel.number;
        SAudioChannelPtr        chanPtr;
        int32                           wasEnabled;
        uint32                          mask;
        
        if ( chanNumber < SA_SUBS_MAX_CHANNELS )
                {
                chanPtr         = ctx->channel + chanNumber;
                
                /* Allow only bits that are Read/Write to be set by this call.
                 *
                 * NOTE:        Any special actions that might need to be taken as as
                 *                      result of bits being set or cleared should be taken
                 *                      now. If the only purpose served by status bits is to 
                 *                      enable or disable features, or some other communication,
                 *                      then the following is all that is necessary. */
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
Err     DoGetChan( SAudioContextPtr ctx, SubscriberMsgPtr subMsg )
        {
        Err                     status = kDSChanOutOfRangeErr;
        uint32          channelNumber = subMsg->msg.channel.number;

        if ( channelNumber < SA_SUBS_MAX_CHANNELS )
                status = ctx->channel[ channelNumber ].status;

        return status;
        }

                
/******************************************************************************
 * Routine to perform an arbitrary subscriber defined action. 
 ******************************************************************************/                
Err     DoControl( SAudioContextPtr ctx, SubscriberMsgPtr subMsg )
        {
        int32                           status = kDSNoErr;
        int32                           userWhatToDo;
	SAudioCtlBlockPtr       ctlBlockPtr;

        userWhatToDo = subMsg->msg.control.controlArg1;
	ctlBlockPtr = (SAudioCtlBlockPtr) subMsg->msg.control.controlArg2;

        switch ( userWhatToDo )
                {
		/* VideoCD specific Opcodes here (W. Hsu 5-21-96) */
#if 1  /* Comment out all kSAudio stuff */
		case kMPEGAudioCtlOpSetUserChoice:
		  ctx->userAudChoice = ((uint32)(subMsg->msg.control.controlArg2));
		  switch ((int32)(subMsg->msg.control.controlArg2)) {
		  case kMPEGAudDefault:
		    /* Assert stream default for this track */
		    if (ctx->trackAudioMode == aMode_dual_channel)
		      SetMixerLeftOnly(ctx, 0);
		    else
		      SetMixerStereo(ctx, 0);
		    break;
		  case kMPEGAudChooseLeft:
		    SetMixerLeftOnly(ctx, 0);   break;
		  case kMPEGAudChooseRight:
		    SetMixerRightOnly(ctx, 0);  break;
		  case kMPEGAudChooseMono:
		    SetMixerMono(ctx, 0);       break;
		  case kMPEGAudChooseStereo:
		    SetMixerStereo(ctx, 0);     break;
		  default:
		    PERR(("sasupport:  Unexpected userAudChoice %d\n",
			  subMsg->msg.control.controlArg2));
		    break;
		  }
		  break;

		case kMPEGAudioCtlOpSetVol:
		  if ( ctlBlockPtr->amplitude.channelNumber < SA_SUBS_MAX_CHANNELS )
		    {
		      status = SetSAudioChannelAmplitude( ctx,
							 ctlBlockPtr->amplitude.channelNumber, 
							 ctlBlockPtr->amplitude.value );
		    }
		  else
		    status = kDSChanOutOfRangeErr;
		  break;

  		case kMPEGAudioCtlOpBypassAudio:
		  /* enter "bypass" mode. Flush buffers, turn on "bypass" */
		  if ((ctx->status & MPEG_CHAN_BYPASS) == 0)
		    {
		      ctx->status |= MPEG_CHAN_BYPASS;
		      status = FlushSAudioChannel( ctx, 0);
		    }
		  break;

		case kMPEGAudioCtlOpUnBypassAudio:
		  /* exit "bypass" mode. */
		  if (ctx->status & MPEG_CHAN_BYPASS)
		    {
		      ctx->status &= ~MPEG_CHAN_BYPASS;
		      status = StartSAudioChannel(ctx, 0, FALSE);
		    }
		  break;

#else /* Comment out all kSAudio stuff */
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
                                status = 0;                     /* these are no-op's for now... */
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
#endif /* Comment out all kSAudio stuff */

                default:
                        status = 0;     /* ignore unknown control messages for now... */
                        break;
                }
        
        return status;
        }

                
/******************************************************************************
 * Routine to do whatever is necessary when a subscriber is added to a stream, typically
 * just after the stream is opened.
 ******************************************************************************/                
Err     DoOpening( SAudioContextPtr ctx, SubscriberMsgPtr subMsg )
{
  Err status;

  TOUCH(subMsg);

  /* Added call to InitSAudioChannel() to replace missing MAHDR chunks */
  /* (W. Hsu 5-13-96) */
  status = InitSAudioChannel(ctx);
  if (status < 0)
    {
      ERROR_RESULT_STATUS("InitSAudioChannel", status);
      PERR((" channel 0\n"));
	    
      CloseSAudioChannel( ctx, 0 );
	    
      /* Nothing all that terrible happened so Reset the
       * status so we won't get aborted by the streamer when
       * it looks at the replied message. */
      return kDSNoErr;
    }

  return kDSNoErr;
}

/******************************************************************************
 * Write out the trace logged data including event-completion pairs.
 ******************************************************************************/                
#if SAUDIO_TRACE_MAIN
void DumpTraceData(void)
        {
        Err             status;
        
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
Err     DoClosing( SAudioContextPtr ctx, SubscriberMsgPtr subMsg )
        {
        Err             status;
        
        TOUCH(subMsg);

        status = CloseAllAudioChannels(ctx);


        return status;
        }


/******************************************************************************
 * Routine to start all channels for this subscription.
 ******************************************************************************/                
Err     DoStart( SAudioContextPtr ctx, SubscriberMsgPtr subMsg )
        {
        uint32          channelNumber;
        const bool      fFlush = (subMsg->msg.start.options & SOPT_FLUSH) != 0;

	if (ctx->status & MPEG_CHAN_BYPASS)
	  return kDSNoErr;  /* just remember that we want to start when unbypassed */

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
	if (ctx->status & MPEG_CHAN_BYPASS) {
		ctx->status &= ~MPEG_CHAN_BYPASS;	/* <HPP> make it unbypassed */
		return kDSNoErr; /* just remember that we want to start when unbypassed */
	}

	return StopAllAudioChannels(ctx, (subMsg->msg.stop.options & SOPT_FLUSH) != 0);
}


/******************************************************************************
 * Flush everything waiting and queued to the AudioFolio under the
 * assumption that we have just arrived at a branch point and should be ready
 * to deal with all new data for this stream.
 ******************************************************************************/                
Err     DoSync( SAudioContextPtr ctx, SubscriberMsgPtr subMsg )
        {
        TOUCH(subMsg);
        
        return FlushAllAudioChannels(ctx);
        }


/******************************************************************************
 * Control-msg routine to respond to an EOF message.
 ******************************************************************************/                
Err     DoEOF( SAudioContextPtr ctx, SubscriberMsgPtr subMsg )
        {
        TOUCH(ctx);
        TOUCH(subMsg);

        return kDSNoErr;
        }


/******************************************************************************
 * Routine to kill all output, return all queued buffers, and generally stop everything.
 ******************************************************************************/                
Err     DoAbort( SAudioContextPtr ctx, SubscriberMsgPtr subMsg )
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
Err     DoBranch(SAudioContextPtr ctx, SubscriberMsgPtr subMsg)
        {
	Err		status = kDSNoErr;

        if ( subMsg->msg.branch.options & SOPT_FLUSH )
                return FlushAllAudioChannels(ctx);

        return status;
        }


/******************************************************************************
 * Process the header information message.  Set the corresponding left/right
 * channel mixing and de-emphasis modes.
 *****************************************************************************/
Err     DoHeaderInfo(SAudioContextPtr ctx, SubscriberMsgPtr subMsg)
{
  Err status = kDSNoErr;

  ctx->trackAudioMode = subMsg->msg.audioParam.audioModes;

  switch (subMsg->msg.audioParam.audioModes) {
  case aMode_dual_channel:
    if (ctx->userAudChoice == kMPEGAudDefault)
      SetMixerLeftOnly(ctx, 0);
    break;
  default: /* aMode_stereo, aMode_joint_stereo, aMode_single_channel */
    if (ctx->userAudChoice == kMPEGAudDefault)
      SetMixerStereo(ctx, 0);
    break;
		    
  }
  /* Handle emphasis stuff here */
  switch (subMsg->msg.audioParam.audioEmphasis) {
  case aEmphasisMode_none:
    NoDeemphasis(ctx, 0);
    break;
  case aEmphasisMode_50_15:  /* 50/15 microsecs */
    AssertDeemphasis(ctx, 0);
    break;
  default: /* aEmphasisMode_reserved, aEmphasisMode_CCITT_J_17 */
    PERR(("sasupport:  Unexpected deemphasis type %d\n",
	  subMsg->msg.audioParam.audioEmphasis));
    NoDeemphasis(ctx, 0);
    break;
  }

  return status;
}
