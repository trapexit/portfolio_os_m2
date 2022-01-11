/******************************************************************************
**
**  @(#) sasoundspoolerinterface.c 96/06/05 1.2
**
*****************************************************************************/

#include <audio/audio.h>
#include <audio/soundspooler.h>
#include <kernel/debug.h>       /* for print macros: CHECK_NEG, PERR, PRNT */
#include <kernel/task.h>        /* for GetCurrentSignals() */
#include <kernel/tags.h>                                /* for ConvertFP_TagData() */
#include <streaming/mempool.h>
#include <video_cd_streaming/saudiosubscriber.h>
#include <streaming/subscribertraceutils.h>

#include "sasoundspoolerinterface.h"
#include "mpadecoderinterface.h"
#include "sasupport.h"

/*****************************************************************************
 * Compile switch implementations
 *****************************************************************************/

#if SAUDIO_TRACE_BUFFERS

        #define TRACE_LEVEL             3
        
        /* Find the actual trace buffer. It's declared in SAMain.c. */
        extern  TraceBufferPtr  SATraceBufPtr;
        
        /* Allow for multiple levels of tracing */
        #if (TRACE_LEVEL >= 1)
                #define         ADD_TRACE_L1( bufPtr, event, chan, value, ptr ) \
                                                AddTrace( bufPtr, event, chan, value, ptr )
        #else
                #define         ADD_TRACE_L1( bufPtr, event, chan, value, ptr )
        #endif
        
        #if (TRACE_LEVEL >= 2)
                #define         ADD_TRACE_L2( bufPtr, event, chan, value, ptr ) \
                                                AddTrace( bufPtr, event, chan, value, ptr )
        #else
                #define         ADD_TRACE_L2( bufPtr, event, chan, value, ptr )
        #endif
        
        #if (TRACE_LEVEL >= 3)
                #define         ADD_TRACE_L3( bufPtr, event, chan, value, ptr ) \
                                                AddTrace( bufPtr, event, chan, value, ptr )
        #else
                #define         ADD_TRACE_L3( bufPtr, event, chan, value, ptr )
        #endif


#else   /* Trace is off */
        #define         ADD_TRACE_L1( bufPtr, event, chan, value, ptr )
        #define         ADD_TRACE_L2( bufPtr, event, chan, value, ptr ) 
        #define         ADD_TRACE_L3( bufPtr, event, chan, value, ptr )
#endif  /* SAUDIO_TRACE_BUFFERS */

#ifdef DEBUG
        static uint32   spoolerStarvationCount; /* # times we starved the sound spooler */
#endif

/* This macro encapsulate deallocation from our MemPools, esp. the typecasts. */

static Err SpoolMPABufCompletionFunc( SoundSpooler *sspl,
        SoundBufferNode *sbn, int32 msg );
static Err SpoolBufCompletionFunc( SoundSpooler *sspl,
        SoundBufferNode *sbn, int32 msg );


/*******************************************************************************************
 * Return the first sound buffer node in the sound spooler's Active list,
 * or NULL if none.
 * NOTE: This is a questionable breach of the SoundSpooler's encapsulation.
 *******************************************************************************************/
SoundBufferNode *FirstActiveNode(SoundSpooler *spooler)
        {
        SoundBufferNode *sbn;
        
        sbn = (SoundBufferNode *)FirstNode(&spooler->sspl_ActiveBuffers);
        if ( !IsNode(&spooler->sspl_ActiveBuffers, (Node *)sbn) )
                sbn = NULL;

        return sbn;
        }

/*******************************************************************************************
 * Initializes the sound spooler
 *******************************************************************************************/
Err     InitSoundSpooler( SAudioContextPtr ctx, uint32 channelNum,
        DSDataType datatype, int32 numBuffers, int32 sampleRate,
        int32 initialAmplitude )
        {
        SAudioChannelPtr        chanPtr;
        Err                                     status;
        
        chanPtr = ctx->channel + channelNum;
        if( chanPtr->spooler != 0 )
                {
                /* This channel is already in use. */
                status = kDSChannelAlreadyInUse;
                goto FAILED;
                }

        /* Create sound spooler data structure */
        status = kDSSoundSpoolerErr;
        chanPtr->spooler = ssplCreateSoundSpooler( numBuffers,
                        chanPtr->channelInstrument );
        FAIL_NIL("ssplCreateSoundSpooler", chanPtr->spooler);   

        if( datatype == MPAU_CHUNK_TYPE )
                {
                status = ssplSetSoundBufferFunc( chanPtr->spooler,
                        (SoundBufferFunc)SpoolMPABufCompletionFunc );
                FAIL_NEG("ssplSetSoundBufferFunc", status);
                }
        else
                {
                status = ssplSetSoundBufferFunc( chanPtr->spooler,
                        (SoundBufferFunc)SpoolBufCompletionFunc );
                FAIL_NEG("ssplSetSoundBufferFunc", status);
                }

        /* Start the soundspooler for playback.  The sound spooler will not begin
         * playback until a buffer is submitted. */
        status = ssplStartSpoolerTagsVA(chanPtr->spooler,
                AF_TAG_SAMPLE_RATE_FP, ConvertFP_TagData((float32)sampleRate),
                AF_TAG_AMPLITUDE_FP, ConvertFP_TagData(initialAmplitude/32768.0),
                TAG_END);
        FAIL_NEG("ssplStartSpoolerTagVA", status);

        /* OR of all Cue signals for this spooler */
        ctx->SpoolersSignalMask |= chanPtr->spooler->sspl_SignalMask;

        /* success */
        status = kDSNoErr;

FAILED:

        return status;

        }

/*******************************************************************************************
 * Routine to pass the sample data to be passed to the soundspooler for playback.
 *******************************************************************************************/
void QueueNewAudioBuffer( SAudioContextPtr ctx, SubscriberMsgPtr newSubMsg )
        {
        int32                           channelNumber;
        SAudioChannelPtr        chanPtr;

        channelNumber   = 0; /* Always assume channel 0 (W. Hsu 5-10-96) */
        chanPtr                 = ctx->channel + channelNumber;

        if (IsChanEnabled(chanPtr))
                {
                AddDataMsgToTail( &chanPtr->dataQueue, newSubMsg );
                QueueWaitingMsgsToSoundSpooler( ctx, channelNumber );
                }
         else
                {
                /* got a data chunk but never got a header chunk to init the
                 * channel */
                ReplyToSubscriberMsg(newSubMsg, kDSNoErr);
                }
        }

/******************************************************************************
 * Queue decompressed MPEG audio buffers to the soundspooler for
 * playback.
******************************************************************************/
Err QueueDecompressedBfrsToSoundSpooler( SAudioContextPtr ctx,
                        DecoderMsgPtr decoderMsg )
{
        SAudioChannelPtr        chanPtr = ctx->channel + decoderMsg->channel;
        uint32                          *audioDataPtr;
        SoundBufferNode         *sbn;
        int32                           status  = kDSNoErr;

        /* If channel is active, and as long as there are data messages and
         * free buffers, submit those buffers to the AudioFolio. */
        if ( IsChanActive(chanPtr) )
        {
                /* Sanity check... If no header has ever arrived for this 
                 * channel (i.e. output instrument = 0), don't 
                 * try to queue buffers. */     
                if ( !IsChanInitalized(chanPtr) ) 
                {
                        PERR( ("\n\n SERIOUS ERROR on SAudio channel %ld: Data buffers arrived on an uninitalized channel!\n\n", decoderMsg->channel) );
                        return kDSInitErr;
                }
                /* Request for an available SoundBufferNode */
                if( ( sbn = ssplRequestBuffer( chanPtr->spooler ) ) != NULL )
                {
                        audioDataPtr = decoderMsg->buffer;
                
                        /* set the Sample Item to point to the sample data from
                         * this chunk */
                        status = ssplSetBufferAddressLength( chanPtr->spooler, sbn,
                                        (char *)audioDataPtr, decoderMsg->size );
                        CHECK_NEG("ssplSetBufferAddressLength", status);
                
        
                        /* link the sbn to the decoderMsg, and the decoderMsg to
                         * the ctx so we can process it when the sample data
                         * finishes playing */
                        decoderMsg->Link = (void *)ctx;
                        ssplSetUserData( chanPtr->spooler, sbn, (void *)decoderMsg );
        
                        /* Submit this buffer for playback */
                        status = ssplSendBuffer( chanPtr->spooler, sbn );
                        CHECK_NEG("ssplSendBuffer", status);
        
                        chanPtr->inuseCount++;

                } /* if( ( sbn = ssplRequestBuffer( chanPtr->spooler ) ) != NULL ) */
                else
		{
			ReturnPoolMem( ctx->datatype.MPEGAudio.decompressedBfrMsgPool,
							 decoderMsg );
			PERR(("No available soundspooler node\n"));
		}

        } /* if ( IsChanActive(chanPtr) ) */

        return status;

} /* QueueDecompressedBfrsToSoundSpooler() */

/******************************************************************************
 * As long as there are waiting messages and free buffers, queue the msgs to
 * to the soundspooler.
 ******************************************************************************/
void QueueWaitingMsgsToSoundSpooler( SAudioContextPtr ctx, int32 channelNumber )
        {
        SubscriberMsgPtr        subMsg;
        SAudioChannelPtr        chanPtr;
        SoundBufferNode         *sbn;
        int32                           status;

        chanPtr = ctx->channel + channelNumber;

        /* If channel is active, and as long as there are data messages and
         * free buffers, submit those buffers to the AudioFolio. */
        if ( IsChanActive(chanPtr) )
                {
                /* Sanity check... If no header has ever arrived for this 
                 * channel (i.e. output instrument = 0), don't 
                 * try to queue buffers. */     
                if ( (!IsChanInitalized(chanPtr)) && (chanPtr->dataQueue.head != NULL) )        
                        {
                        PERR( ("\n\n SERIOUS ERROR on SAudio channel %ld: Data buffers arrived on an uninitalized channel!\n\n", channelNumber) );
                        return;
                        }
                /* Request for an available SoundBufferNode */
                while( ( sbn = ssplRequestBuffer( chanPtr->spooler ) ) != NULL )
                        {
                        subMsg = GetNextDataMsg( &chanPtr->dataQueue );
                        if( subMsg != NULL )
                                {
                
                                /* set the Sample Item to point to the sample data from
                                 * this chunk */
                                status = ssplSetBufferAddressLength( chanPtr->spooler, sbn,
                                        (char *)subMsg->msg.data.buffer, subMsg->msg.data.bufferSize );
                                CHECK_NEG("ssplSetBufferAddressLength", status);
        
                                /* link the sbn to the subMsg, and the subMsg to the ctx so
                                 * we can process it when the sample data finishes playing */
                                subMsg->link = (void *)ctx;
                                ssplSetUserData( chanPtr->spooler, sbn, (void *)subMsg );
        
                                /* Submit this buffer for playback */
                                status = ssplSendBuffer( chanPtr->spooler, sbn );
                                CHECK_NEG("ssplSendBuffer", status);
        
                                chanPtr->inuseCount++;
                                } /* if( subMsg != NULL ) */
                        else
                                {
                                /* No more data from the streamer. Return the buffer request from the
                                 * spooler. */
                                status = ssplUnrequestBuffer( chanPtr->spooler, sbn );
                                CHECK_NEG("ssplUnRequestBuffer", status);
                                break;
                                }

                        } /* while( ( sbn = ssplRequestBuffer( chanPtr->spooler ) ) != NULL ) */
                } /* if ( IsChanActive(chanPtr) ) */
        }

/******************************************************************************
 * ProcessClockChannelCompletion
 ******************************************************************************/                
static Err ProcessClockChannelCompletion( SAudioContextPtr ctx,
					 uint32 channelNumber, uint32 signalBits )
{
	Err                                     status = kDSNoErr;
	SAudioChannelPtr        chanPtr = ctx->channel + channelNumber;

	if ( channelNumber < SA_SUBS_MAX_CHANNELS && chanPtr->spooler != NULL )
	{
     	 status = ssplProcessSignals( chanPtr->spooler, signalBits, NULL );
      	CHECK_NEG("ssplProcessSignals", status);

      	/* If any buffers completed on the clock channel, update the stream
      	 * presentation clock from the {branchNumber, time} of the buffer
       	 * that just started playing. */
		if ( status > 0 ) {					/* one or more buffers completed */
	
	  		chanPtr->inuseCount -= status;

	  		if ( ssplIsSpoolerActive(chanPtr->spooler) ) {
	      		/* Peek at the first buffer in the spooler's active queue. */
				SoundBufferNode		*sbn = FirstActiveNode(chanPtr->spooler);
	      		void             	*msgPtr;
	      		int32           	time;
	      		int32          		branchNumber;

	      		if ( sbn != NULL ) {
		  			msgPtr = ssplGetUserData( chanPtr->spooler, sbn );
					/* Always assume MPAU_CHUNK_TYPE (W. Hsu 5-10-96) */
					time = ((DecoderMsgPtr)msgPtr)->presentationTime; 
					branchNumber = ((DecoderMsgPtr)msgPtr)->branchNumber;
					/* <HPP> Don't set the time if invalid time stamp */
					if( ((DecoderMsgPtr)msgPtr)->timeIsValidFlag )                      
						DSSetPresentationClock(ctx->streamCBPtr, branchNumber, time);
				} /* if( sbn != NULL ) */
	    	} /* if ( ssplIsSpoolerActive(chanPtr->spooler) ) */
		} /* if ( status > 0 ) */
    } 

  return (status);
} /* ProcessClockChannelCompletion() */

/******************************************************************************
 * Process all completed MPEG audio buffers: queue up new decompressed
 * buffers to the decoder (reusing completed buffers). Also adjust the
 * stream clock if any buffers completed on the clock channel.  Process
 * the clock channel first in order to adjust the clock ASAP after its
 * completion signals.
 * 
 * NOTE: ssplProcessSignals returns a negative error code or else a
 * non-negative number of buffers that completed.
 ******************************************************************************/                
void HandleCompletedMPABuffers( SAudioContextPtr ctx, uint32 signalBits )
{
        Err                                     status;
        SAudioChannelPtr        chanPtr;
        uint32                          channelNumber;
	DecoderMsgPtr		decoderMsg;

        ADD_TRACE_L3( SATraceBufPtr, kBufferCompleted, -1, signalBits,
                (void *)(signalBits & ctx->SpoolersSignalMask) );

        /* Process the clock channel first. */
        channelNumber = ctx->clockChannel;
        status = ProcessClockChannelCompletion( ctx, channelNumber,
                signalBits );
        CHECK_NEG( "ProcessClockChannelCompletion", status );

        /* Process all the other channels. */
        for ( channelNumber = 0; channelNumber < SA_SUBS_MAX_CHANNELS;
                        channelNumber++ )
        {
                chanPtr = ctx->channel + channelNumber;
                if ( channelNumber == ctx->clockChannel || chanPtr->spooler == NULL )
                        continue;

                status = ssplProcessSignals( chanPtr->spooler, signalBits, NULL );
                CHECK_NEG("ssplProcessSignals", status);
                
                if ( status > 0 )       /* one or more buffers completed */
                        chanPtr->inuseCount -= status;
        }

	/* Send any decompressed buffers we have in the memory
	 * pool for the decoder to store the decompressed data */
	while( (decoderMsg =
		ALLOC_DECOMPRESSED_BFR_MSG_POOL(ctx)) != NULL )
	{
		ADD_TRACE_L1( SATraceBufPtr, kTraceSendDecompressedBfrMsg,
           			decoderMsg->channel, decoderMsg->presentationTime,
					decoderMsg );

		status = SendDecoderMsg( ctx, decoderMsg );
		CHECK_NEG("SendDecoderMsg", status );
	}

} /* HandleCompletedMPABuffers() */

/******************************************************************************
 * Process all completed buffers: Reply the subscriber messages to the streamer
 * and queue up new data to the sound spooler (reusing completed buffers). Also
 * adjust the stream clock if any buffers completed on the clock channel.
 * Process the clock channel first in order to adjust the clock ASAP after its
 * completion signals.
 * 
 * NOTE: ssplProcessSignals returns a negative error code or else a non-negative
 * number of buffers that completed.
 ******************************************************************************/                
void HandleCompletedBuffers( SAudioContextPtr ctx, uint32 signalBits )
        {
        Err                                     status;
        SAudioChannelPtr        chanPtr;
        uint32                          channelNumber;

        ADD_TRACE_L3( SATraceBufPtr, kBufferCompleted, -1, signalBits,
                (void *)(signalBits & ctx->SpoolersSignalMask) );

        /* Process the clock channel first. */
        channelNumber = ctx->clockChannel;
        status = ProcessClockChannelCompletion( ctx, channelNumber,
                        signalBits );

        if( status > 0 )
                        QueueWaitingMsgsToSoundSpooler( ctx, channelNumber );

        /* Process all the other channels. */
        for ( channelNumber = 0; channelNumber < SA_SUBS_MAX_CHANNELS;
                        channelNumber++ )
                {
                chanPtr = ctx->channel + channelNumber;
                if ( channelNumber == ctx->clockChannel || chanPtr->spooler == NULL )
                        continue;

                status = ssplProcessSignals( chanPtr->spooler, signalBits, NULL );
                CHECK_NEG("ssplProcessSignals", status);
                
                if ( status > 0 )       /* one or more buffers completed */
                        {
                        chanPtr->inuseCount -= status;
                        QueueWaitingMsgsToSoundSpooler( ctx, channelNumber );
                        }
                }

        } /* HandleCompletedBuffers() */

/******************************************************************************
 * Process a MPEG audio buffer-completion signal from the Sound Spooler:
 * [TBD] Can we catch SSPL_SBMSG_LINK_START messages (or maybe all start msgs)
 *    and use them (on ctx->clockChannel) to set the stream clock? If
 *    ssplProcessSignals() sends us one SSPL_SBMSG_LINK_START message even if
 *    several buffers finished playing, then this would work, and it'd be
 *    cleaner than peeking at the first active buffer in the spooler after
 *    calling ssplProcessSignals().
 ******************************************************************************/                
static Err SpoolMPABufCompletionFunc( SoundSpooler *sspl,
        SoundBufferNode *sbn, int32 msg )
{
        DecoderMsgPtr           decoderMsg;
        SAudioContextPtr        ctx;
        
        switch ( ssplGetSBMsgClass(msg) )
                {
                case SSPL_SBMSGCLASS_END:
                        
                        decoderMsg = (DecoderMsgPtr)ssplGetUserData( sspl, sbn );
                        ctx = (SAudioContextPtr)decoderMsg->Link;

                        /* Recycle this buffer back to the decompressed bfr
                         * message pool. */
                        ReturnPoolMem( ctx->datatype.MPEGAudio.decompressedBfrMsgPool,
                                decoderMsg );
                        break;

                default:
#ifdef DEBUG
                        if ( msg == SSPL_SBMSG_STARVATION_START )
                                {
                                spoolerStarvationCount++;

                                /* NOTE: We could increment a channel-specific starvation count.
                                 * But to do that we'd have to recover the subMsg, ctx,
                                 * audioDataPtr, channelNumber, and chanPtr. sbn's user data
                                 * field points to subMsg. subMsg's link field points to ctx.
                                 * subMsg's channel field is the channelNumber. */

                                /* [TBD] Trace-log this starvation (error) event. */
 
                                }
#endif
                        break;
 
                } /* switch ( msg ) */

        return kDSNoErr;

} /* SpoolMPABufCompletionFunc() */

/******************************************************************************
 * Process a buffer-completion signal from the Sound Spooler:
 * Reply to the streamer's subMsg and update debug info.
 * [TBD] Can we catch SSPL_SBMSG_LINK_START messages (or maybe all start msgs)
 *    and use them (on ctx->clockChannel) to set the stream clock? If
 *    ssplProcessSignals() sends us one SSPL_SBMSG_LINK_START message even if
 *    several buffers finished playing, then this would work, and it'd be
 *    cleaner than peeking at the first active buffer in the spooler after
 *    calling ssplProcessSignals().
 ******************************************************************************/                
static Err      SpoolBufCompletionFunc( SoundSpooler *sspl,
                SoundBufferNode *sbn, int32 msg )
        {
        Err                                     status = kDSNoErr;
        SubscriberMsgPtr        subMsg;
        
        switch ( ssplGetSBMsgClass(msg) )
                {
                case SSPL_SBMSGCLASS_END:
                        /* Recover the subcriber message pointer from the SoundBufferNode that
                         * just completed */
                        status = kDSBadPtrErr;
                        subMsg = (SubscriberMsgPtr)ssplGetUserData( sspl, sbn );
                        FAIL_NIL("SpoolBufCompletionFunc", subMsg);     

                        /* Jam a magic number in the first longword of the sample data
                         * so the outside world can tell we've played and replied to
                         * this data message. */
                        /* [TBD] If this is debug code, it should be gated by #ifdef DEBUG.
                         * If not, what is it for? */
			*(uint32 *)(subMsg->msg.data.buffer) = 0xdeadbeef;

                        /* Reply to the message */
                        status = ReplyToSubscriberMsg(subMsg, kDSNoErr);
                        break;

                default:
#ifdef DEBUG
                        if ( msg == SSPL_SBMSG_STARVATION_START )
                                {
                                spoolerStarvationCount++;

                                /* NOTE: We could increment a channel-specific starvation count.
                                 * But to do that we'd have to recover the subMsg, ctx,
                                 * audioDataPtr, channelNumber, and chanPtr. sbn's user data
                                 * field points to subMsg. subMsg's link field points to ctx.
                                 * subMsg's channel field is the channelNumber. */

                                /* [TBD] Trace-log this starvation (error) event. */

                                /* PERR(("SpoolBufCompletionFunc: Sound Spooler starved\n")); */
                                }
#endif
                        break;

                } /* switch ( msg ) */

FAILED:
                return status;

        } /* SpoolBufCompletionFunc */

