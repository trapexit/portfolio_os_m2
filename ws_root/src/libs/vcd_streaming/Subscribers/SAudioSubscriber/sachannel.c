/******************************************************************************
**
**  @(#) sachannel.c 96/06/05 1.2
**
******************************************************************************/

#include <string.h>                             /* for memset() */
#include <audio/audio.h>
#include <audio/soundspooler.h>
#include <kernel/list.h>
#include <kernel/tags.h>                /* for ConvertFP_TagData() */
#include <kernel/types.h>
#include <kernel/debug.h>               /* for print macros: CHECK_NEG */
#include <kernel/task.h>

#include <streaming/dserror.h>  /* for DS error codes */
#include <video_cd_streaming/dsstreamdefs.h>
#include <video_cd_streaming/mpegaudiosubscriber.h>
#include <video_cd_streaming/saudiosubscriber.h>
#include <streaming/satemplatedefs.h>
#include <video_cd_streaming/subscriberutils.h>
#include <streaming/subscribertraceutils.h>
#include <streaming/threadhelper.h>

#include "sachannel.h"
#include "sasupport.h"
#include "sasoundspoolerinterface.h"
#include "mpadecoderinterface.h"

/* Define the minimum and maximum values for amplitude */
#define MIN_AMPLITUDE_RANGE             0x0
#define AVE_AMPLITUDE_RANGE             0x4000
#define MAX_AMPLITUDE_RANGE             0x7FFF

/* Define the minimum and maximum values for pan */
#define MIN_PAN_RANGE                   0x0
#define AVE_PAN_RANGE                   0x4000
#define MAX_PAN_RANGE                   0x7FFF

/* Define the default and only acceptible sample Rate for MPEG audio */
#define MPEG_AUDIO_SAMPLE_RATE  44100

#if 0  /* Not used  (W. Hsu 5-9-96) */
#define GET_SAUDIO_NUMCHANNELS(hdr)     \
        ((SAudioHeaderChunkPtr)hdr)->sampleDesc.numChannels == 1 ) ? 1 : 2);
#endif
/*****************************************************************************
 * Compile switch implementations
 *****************************************************************************/

#if SAUDIO_TRACE_CHANNELS

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
        
        
#else  /* Trace is off */
        #define         ADD_TRACE_L1( bufPtr, event, chan, value, ptr )
        #define         ADD_TRACE_L2( bufPtr, event, chan, value, ptr )
        #define         ADD_TRACE_L3( bufPtr, event, chan, value, ptr )
#endif /* SAUDIO_TRACE_CHANNELS */


/* Static function prototype */
static Err      InternalStopSAudioChannel( SAudioContextPtr ctx,
                uint32 channelNumber );
static Err      InternalPauseSAudioChannel( SAudioContextPtr ctx,
                uint32 channelNumber );
#if 0  /* Not used  (W. Hsu 5-9-96) */
static Err      CreateThdoAudioChannelIns( SAudioContextPtr ctx,
                SAudioHeaderChunkPtr headerPtr );
#endif
static Err CreateAudioOutputIns( SAudioContextPtr ctx,
                uint32 channelNumber );
static void CloseMPEGDecoder( SAudioContextPtr ctx );
static Err SendDecompressedBfrMsgs( SAudioContextPtr ctx );

/******************************************************************************
 * Initialize each decompressed buffer message. 
******************************************************************************/
static Err SendDecompressedBfrMsgs( SAudioContextPtr ctx )
{
        DecoderMsgPtr   decompressedBfrMsgPtr;
        uint32                  *bufPtr = ctx->datatype.MPEGAudio.audioBfrs;
        Err                             status = kDSNoErr;

        while( (decompressedBfrMsgPtr =
                        ALLOC_DECOMPRESSED_BFR_MSG_POOL(ctx)) != NULL )
        {
                decompressedBfrMsgPtr->channel          = 0; /* default channel */
                decompressedBfrMsgPtr->messageType      = decompressedBfrMsg;
                decompressedBfrMsgPtr->buffer           = bufPtr;
                decompressedBfrMsgPtr->size                     = SIZE_OF_DECOMPRESSED_BUFFER;

                bufPtr += MPEG_SAMPLES_PER_AUDIO_FRAME;
                ADD_TRACE_L1( SATraceBufPtr, kTraceSendDecompressedBfrMsg,
                        decompressedBfrMsgPtr->channel,
                        decompressedBfrMsgPtr->presentationTime, decompressedBfrMsgPtr );
 
                status = SendDecoderMsg( ctx, decompressedBfrMsgPtr );
        }

        return status;

} /* SendDecompressedBfrMsgs() */

/******************************************************************************
 * Close MPEG audio decoder thread.
******************************************************************************/
static void CloseMPEGDecoder( SAudioContextPtr ctx )
{
        Err                             status;
        DecoderMsgPtr   decoderMsg = ALLOC_DECODER_MSG_POOL_MEM( ctx );

        if( decoderMsg != NULL )
        {
                ADD_TRACE_L1( SATraceBufPtr, kTraceCloseDecoderMsg,
                        decoderMsg->channel, decoderMsg->messageType, decoderMsg );

#ifdef DEBUG
                PRNT(( "CloseDecoderMsg @ addr 0x%x, msgItem = 0x%x\n",
                                decoderMsg, decoderMsg->msgItem ));
#endif

#ifdef DEBUG_PRINT
        PRNT(("CloseDecoderMsg addr: 0x%x\n", decoderMsg ));
#endif

                decoderMsg->messageType = closeDecoderMsg;
                status = SendDecoderMsg( ctx, decoderMsg );
                CHECK_NEG( "SendDecoderMsg", status );

        } /* if( decoderMsg != NULL ) */

} /* CloseMPEGDecoder() */

/******************************************************************************
 * Routine to create 3DO audio output instrument for this channel.
 *******************************************************************************/
static Err CreateAudioOutputIns( SAudioContextPtr ctx, 
                uint32 channelNumber )
{
        Err                                     status;
        Item                            tempKnob;
        SAudioChannelPtr        chanPtr                 = ctx->channel + channelNumber;
        SAudioOutputPtr         chanOutputPtr   = &chanPtr->channelOutput;

	/* Set master gain knob */
	status = CreateInstrument( ctx->envelopeTemplateItem, NULL );
	FAIL_NEG("CreateInstrument", status);

	chanOutputPtr->masterGainEnv = status;

	/* Connect Envelope to amplitude knobs for ramping */
	status = ConnectInstrumentParts (chanOutputPtr->masterGainEnv, "Output", 0,
					 chanOutputPtr->instrument, "Amplitude", 0);
	FAIL_NEG("ConnectInstrumentParts amplitude knobs", status);

	/* Grab the envelope's rate knob so we can control the speed of ramping */
	status  = CreateKnob(chanOutputPtr->masterGainEnv, "Env.incr", NULL);
	FAIL_NEG("CreateKnob rate knob", status);

	tempKnob = status;

	/* 1 = 3/4 sec; 2 = twice at fast as 1; */
	SetKnob( tempKnob, 10.0/32768.0 );
	
	DeleteKnob( tempKnob );

	/* Grab the envelope's target knob so we can set the value to ramp to */
	status = CreateKnob(chanOutputPtr->masterGainEnv, "Env.request", NULL);
	FAIL_NEG("CreateKnob", status);

	chanOutputPtr->masterGainEnvTargetKnob = status;

	/* Start the right envelope */
	status = StartInstrument( chanOutputPtr->masterGainEnv, NULL );
	FAIL_NEG("StartInstrument masterGainEnv", status);

FAILED:
        return status;

} /* CreatetAudioOutputIns() */

/******************************************************************************
 * Routine to create 3DO audio output channel instrument.
 *******************************************************************************/
#if 0  /* Not used (W. Hsu 5-9-96) */
static Err CreateThdoAudioChannelIns( SAudioContextPtr ctx,
        SAudioHeaderChunkPtr headerPtr )
{
        Err                                     status;
        SAudioChannelPtr        chanPtr = ctx->channel + headerPtr->channel;
        Item                            templateItem;
        int32                           templateTag;

        /* Set the maximum buffers that can be queued to the
         * SoundSPooler at any one time */
        chanPtr->numBuffers     = headerPtr->numBuffers;

        /* Figure out which sample player instrument we should load for
         * that data type */
        status = templateTag = GetTemplateTag( &headerPtr->sampleDesc );
        FAIL_NEG("GetTemplateTag", status);

        /* Get the instrument template item we need for building an
         * instrument */
        status = templateItem =
                GetTemplateItem( ctx, templateTag, kMaxTemplateCount );
        FAIL_NEG("GetTemplateItem", status);

        /* Make an instrument from the template */
        status = CreateInstrument( templateItem, NULL );
        FAIL_NEG("CreateInstrument", status);

        chanPtr->channelInstrument = status;

FAILED:
        return status;

} /* CreateThdoAudioChannelIns() */
#endif  /* #if 0 Not used */

/******************************************************************************
 * Routine to initalize an audio channel for a given context.  Create a sound
 * spooler for this channel.
 *******************************************************************************/
Err InitSAudioChannel( SAudioContextPtr ctx )
{
  Err    status;
  uint32 channelNumber           = 0; /* Use only one channel (W. Hsu 5-9-96) */
  SAudioChannelPtr chanPtr = ctx->channel + channelNumber;
  SAudioOutputPtr chanOutputPtr = &chanPtr->channelOutput;
  int32 sampleRate = MPEG_AUDIO_SAMPLE_RATE;
  int32 initialAmplitude = MAX_AMPLITUDE_RANGE;
  int32 numBuffers = chanPtr->numBuffers;
  Item line_out;
  
  ADD_TRACE_L2( SATraceBufPtr, kTraceChannelInit, channelNumber, 0, 0 );
#if 0 /* condition always false  (W. Hsu 5-10-96) */
  if ( channelNumber >= SA_SUBS_MAX_CHANNELS )
    return kDSChanOutOfRangeErr;
#endif 
  
  /* Make sure this stream format version is compatible with
   * this subscriber */
  
  /* Assume only MPAU_CHUNK_TYPE (W. Hsu 5-9-96) */
  /* Always make the channel active when we receive the header */
  ctx->datatype.MPEGAudio.activeChannel = channelNumber;
  
  chanOutputPtr->numChannels = 2;
  
  /* Send decompressed buffers to the decoder. */
  status = SendDecompressedBfrMsgs( ctx );
  CHECK_NEG( "SendDecompressedBfrMsgs", status );
  
  /* Default channel 0 to enabled.  All other channels must be explicitly
   * enabled with a SetChan message. */

#if 0  /* Condition always TRUE (W. Hsu 5-10-96) */  
  if ( channelNumber == 0 )
#endif
    chanPtr->status |= CHAN_ENABLED; /* MUST HAVE BOTH CHAN_ENABLED
				      * & CHAN_ACTIVE TO PLAY ANY
				      * DATA */
  /* Always make the channel active when we receive the header */
  chanPtr->status |= CHAN_ACTIVE;
  ctx->datatype.MPEGAudio.activeChannel = channelNumber;
  
  /* Set datatype */
  ctx->dataType = MPAU_CHUNK_TYPE;

  /* Set clock channel to channel 0 */
  ctx->clockChannel = channelNumber;
  
  /* If the channel does not already have an output instrument
   * (mixer), make one from the template. */
  if ( chanOutputPtr->instrument == 0 )
    {
      /* If the channel does not already have an output
       * instrument (mixer), make one from the template. */
      status = CreateInstrument( ctx->outputTemplateItem, NULL );
      FAIL_NEG( "CreateInstrument", status );
      
      chanOutputPtr->instrument = status;
      
      /* Start the mixer */
      status = StartInstrument( chanOutputPtr->instrument, NULL );
      FAIL_NEG("StartInstrument mixer", status);
      
      status = CreateAudioOutputIns( ctx, channelNumber );
      FAIL_NEG( "CreateAudioOutputIns", status );

      line_out = LoadInstrument("line_out.dsp", 0, 100);
      FAIL_NEG("LoadInstrument", line_out);
      chanOutputPtr->lineOut = line_out;

      status = ConnectInstrumentParts(chanOutputPtr->instrument, "Output", 0,
				      line_out, "Input", 0);
      FAIL_NEG("ConnectInstrumentsParts", status);
      status = ConnectInstrumentParts(chanOutputPtr->instrument, "Output", 1,
				      line_out, "Input", 1);
      FAIL_NEG("ConnectInstrumentsParts", status);

      status = StartInstrument( line_out, NULL );
      FAIL_NEG("StartInstrument line_out", status);
      
      SetMixerStereo(ctx, channelNumber);
      
    } /* if ( chanOutputPtr->instrument == 0 ) */
  
  /* Connect Sampler to output- do different things depending on
   * stereo or mono */
  
  /* If the channel's playback instrument has not been created
   * or was deleted with CloseSAudioChannel(), figure out which
   * one to make based on the header data in the audio stream. */
  /* Always assume MPAU_CHUNK_TYPE (W. Hsu 5-9-96) */
  if ( chanPtr->channelInstrument == 0 )
    {
      /* Load fixed rate stereo sample player instrument */
      status = LoadInstrument("sampler_16_f2.dsp", 0, 100 );
      FAIL_NEG("LoadInstrument", status);
      chanPtr->channelInstrument = status;

      /* Always assume stereo output */
      status = ConnectInstrumentParts(chanPtr->channelInstrument,
				      "Output", 0,
				      chanOutputPtr->instrument, "Input", 0);
      FAIL_NEG("ConnectInstrumentParts Input 0", status);
      status = ConnectInstrumentParts(chanPtr->channelInstrument,
				      "Output", 1,
				      chanOutputPtr->instrument, "Input", 1);
      FAIL_NEG("ConnectInstrumentParts Input 1", status);

      /* Load the deemphasis instrument */
      status = LoadInstrument("deemphcd.dsp", 0, 65);
      FAIL_NEG("LoadInstrument", status);
      chanOutputPtr->deemphasisLeft = status;
      status = LoadInstrument("deemphcd.dsp", 0, 65);
      FAIL_NEG("LoadInstrument", status);
      chanOutputPtr->deemphasisRight = status;

      status = ConnectInstrumentParts (chanPtr->channelInstrument,
				       "Output", 0,
				       chanOutputPtr->deemphasisLeft,
				       "Input", 0);
      FAIL_NEG("ConnectInstrumentParts deemphasisLeft", status);
      
      status = ConnectInstrumentParts (chanPtr->channelInstrument,
				       "Output", 1,
				       chanOutputPtr->deemphasisRight,
				       "Input", 0);
      FAIL_NEG("ConnectInstrumentParts deemphasisRight", status);

      status = StartInstrument( chanOutputPtr->deemphasisLeft, NULL );
      FAIL_NEG("StartInstrument deemphasisLeft", status);
      status = StartInstrument( chanOutputPtr->deemphasisRight, NULL );
      FAIL_NEG("StartInstrument deemphasisRight", status);

      NoDeemphasis(ctx, 0);

#if 0      
      status = ConnectInstrumentParts (chanPtr->channelInstrument, "Output", 0,
				       chanOutputPtr->instrument, "Input", 0);
      FAIL_NEG("ConnectInstrumentParts Input 0", status);
      
      status = ConnectInstrumentParts (chanPtr->channelInstrument,
				       "Output", 1, chanOutputPtr->instrument, "Input", 1 );
      FAIL_NEG("ConnectInstrumentParts Input 1", status);
#endif
      
      /* In order to eliminate pops once and for all, don't actually
       * set the amplitude of a channel until after StartInstrument()
       * has been called by ssplStartSpooler() in InitSoundSpooler().
       * This is accomplished by calling MuteSAudioChannel() before
       * SetSAudioChannelAmplitude().  This will cause
       * UnMuteSAudioChannel() to restore the volume we set here
       * when we want. */
      MuteSAudioChannel(ctx, channelNumber, INTERNAL_MUTE);
      
      /* CurrentAmp was initalized to -1 by the main thread so the user
       * can set the amplitude to zero with a control message before
       * the stream started and not have it be overided by the
       * InitailAmplitude setting in the .saudio header. In other words,
       * -1 means "never set" and 0 means "zero amplitude".
       * Since we've called MuteSAudioChannel(), the current amplitude
       * setting has been saved in savedAmp.  We need to check that
       * now to find out weather we've received an amplitude
       * change control message or not.
       * This is admittedly a bit ugly, but it's the only place where
       * we need to worry about such stuff - so it gets left this way.
       *
       * So, if we didn't get a control message, use the initialAmplitude
       * field from the .saudio header - otherwise MuteSAudioChannel()
       * captured whatever the user set with the control call and
       * UnMuteSAudioChannel() will restore it when appropriate.
       * Clear as mud? */
      if ( chanOutputPtr->savedAmp == -1 )
	{
	  status = SetSAudioChannelAmplitude( ctx, channelNumber,
					     initialAmplitude );
	  FAIL_NEG("SetSAudioChannelAmplitute", status);
	}
      
#ifdef DEBUG_PRINT
      PRNT((" initialAmplitude: %d, sampleRate: %d, numBuffers: %d\n",
	    initialAmplitude, sampleRate, numBuffers ));
#endif
      /* Initialize the sound spooler */
      /* Assume only MPAU_CHUNK_TYPE (W. Hsu 5-9-96) */
      status = InitSoundSpooler( ctx, channelNumber, MPAU_CHUNK_TYPE,
				numBuffers, sampleRate, initialAmplitude );
      FAIL_NEG("InitSoundSpooler", status);
      
      /* Unmute the instrument for playback. */
      UnMuteSAudioChannel(ctx, channelNumber, INTERNAL_MUTE);
      
    } /*  if ( chanPtr->channelInstrument == 0 ) */
  
  /* Success */
  status = kDSNoErr;
  
 FAILED:
  return status;
}

/*******************************************************************************************
 * Routine to begin data flow for the given channel.
 *******************************************************************************************/
Err StartSAudioChannel( SAudioContextPtr ctx, uint32 channelNumber,
		       bool doFlush )
{
  Err                                     status = kDSNoErr;
  SAudioChannelPtr        chanPtr;
  /* int32                        spoolerStatus; */

  if ( channelNumber >= SA_SUBS_MAX_CHANNELS )
    return kDSChanOutOfRangeErr;

  chanPtr  =  ctx->channel + channelNumber;

  ADD_TRACE_L2( SATraceBufPtr, kTraceChannelStart, channelNumber, 0, 0 );

  /* Make sure that we are not already running. 
   * If we already started, do nothing */
  if ( IsChanEnabled(chanPtr) && (!IsChanActive(chanPtr)) )
    {
      /* Safety check. If the channel was enabled before data started
       * flowing through this channel, then soundspooler have not been created.
       * Do nothing now. The SAudio subscriber will create the soundspooler
       * and get the data flowing when it receives the audio header chunk */
      if ( chanPtr->spooler != NULL )
	{
	  /* spoolerStatus = ssplGetSpoolerStatus(chanPtr->spooler); */
        
	  if ( doFlush )
	    /* if( SSPL_STATUS_F_PAUSED & spoolerStatus ) */        /* [TBD]
	     * Flush on demand, even if it wasn't paused. */
	    {
	      /* If we were in the paused state and now are being flushed
	       * we need to flush the channel and abort the sound spooler.
	       * FlushSAudioChannel will call internalStopSAudioChannel() if necessary */
	      status = FlushSAudioChannel( ctx, channelNumber );
	    }
        
	  chanPtr->status |= CHAN_ACTIVE;

#if 0 /* Not used  (W. Hsu 5-21-96) */
	  if( ctx->dataType == SNDS_CHUNK_TYPE )
	    {
	      /* If there are any msgs waiting in the dataQueue, use any free
	       * soundspooler buffers and try to start them playing. */
	      QueueWaitingMsgsToSoundSpooler( ctx, channelNumber );
	    }
#endif

	  BeginSAudioPlaybackIfAppropriate( ctx, channelNumber );
        
        
	} /* if ( chanPtr->spooler != NULL ) */

    } /* if ( IsChanEnabled(chanPtr) && (!IsChanActive(chanPtr)) ) */

  return status;

}

/******************************************************************************
 * Routine to pause data flow for the given channel.  This is different from "Stop"
 * in the sense that "pause" halts the instrument (i.e. DMA) in it's tracks, so
 * BeginPlaybackIfAppropriate() will resume exactly where we left off.  StopChannel()
 * will flush the currently playing buffer so playback will resume from the next buffer in
 * the queue, this is necessary to maintian the proper functioning of Stop with Flush.
 *
 * ASSUMES: The given channel has a sound spooler.
 ******************************************************************************/
static Err      InternalPauseSAudioChannel( SAudioContextPtr ctx,
                uint32 channelNumber )
        {
        Err                                     status;
        SAudioChannelPtr        chanPtr;

    if ( channelNumber >= SA_SUBS_MAX_CHANNELS )
        return kDSChanOutOfRangeErr;

        chanPtr  =  ctx->channel + channelNumber;

#if (TRACE_LEVEL >= 2)
                {
                DSClock                         dsClock;
                
                DSGetPresentationClock(ctx->streamCBPtr, &dsClock);
                ADD_TRACE_L2( SATraceBufPtr, kTraceInternalChannelPause, channelNumber,
                                                dsClock.branchNumber, (void*)dsClock.streamTime );
                }
#endif

        /* Make sure mixer knobs are at zero */
        status = MuteSAudioChannel( ctx, channelNumber, INTERNAL_MUTE );
        CHECK_NEG("MuteSAudioChannel", status);

        /* Pause the soundspooler */
        status = ssplPause( chanPtr->spooler );
        CHECK_NEG("ssplPause", status);

#if (TRACE_LEVEL >= 3)
                {
                SoundBufferNode         *sbn;
        
                /* Get a pointer to the buffer that was playing when the stop
                 * was initiated. */
                sbn = FirstActiveNode(chanPtr->spooler);
        
                /* Leave a trace so we can tell which buffer was playing when pause
                 * was called */
		if( sbn != NULL )
			ADD_TRACE_L3( SATraceBufPtr, kCurrentBufferOnPause, channelNumber,
						(sbn == NULL ? 0 : sbn->sbn_Signal), sbn );
                }
#endif

        return status;

        /* Don't need to check to see if a buffer completed playback here.
         * Since the soundSpooler will signal us when done, leave it to
         * HandleCompletedBuffers() to handle buffer completion. */
        } /* InternalPauseSAudioChannel() */

/******************************************************************************
 * Routine to perform low level halt of data flow for the channel.
 ******************************************************************************/
static Err      InternalStopSAudioChannel( SAudioContextPtr ctx,
                uint32 channelNumber )
        {
        Err                                     status;
        SAudioChannelPtr        chanPtr;

        if ( channelNumber >= SA_SUBS_MAX_CHANNELS )
                return kDSChanOutOfRangeErr;

        chanPtr  =  ctx->channel + channelNumber;

        ADD_TRACE_L2( SATraceBufPtr, kTraceInternalChannelStop, channelNumber,
                                        0, 0 );

        /* Make sure mixer knobs are at zero */
        status = MuteSAudioChannel( ctx, channelNumber, INTERNAL_MUTE );

        /* Abort the sound spooler */
        if ( chanPtr->spooler != NULL )
                {
#if (TRACE_LEVEL >= 3)
                SoundBufferNode         *sbn;

                /* Leave a trace so we can tell which buffer was playing when
                 * stop was called */
                sbn = FirstActiveNode(chanPtr->spooler);
                ADD_TRACE_L3( SATraceBufPtr, kCurrentBufferOnStop, channelNumber,
                        (sbn == NULL) ? 0 : sbn->sbn_Signal, sbn );
#endif

                status = ssplAbort( chanPtr->spooler, NULL );
                CHECK_NEG("ssplAbort", status);
                }

        return status;
        }

/******************************************************************************
 * Routine to halt data flow for the given channel.
 * ASSUMES: channelNumber < SA_SUBS_MAX_CHANNELS.
 ******************************************************************************/
Err StopSAudioChannel( SAudioContextPtr ctx, uint32 channelNumber,
                bool doFlush )
        {
        Err                                     status = kDSNoErr;
        SAudioChannelPtr        chanPtr;

        if ( channelNumber >= SA_SUBS_MAX_CHANNELS )
                return kDSChanOutOfRangeErr;

        chanPtr  =  ctx->channel + channelNumber;

        ADD_TRACE_L2( SATraceBufPtr, kTraceChannelStop, channelNumber, 0, 0 );

        /* Make sure that we are running, if not... do nothing.
         * NOTE: Need to flush even if !IsChanActive(), e.g. if paused. */
        if ( IsChanEnabled(chanPtr) )
                {
                if ( doFlush )
                        {
                        /* Flush will call InternalStopSAudioChannel() if necessary */
                        status = FlushSAudioChannel( ctx, channelNumber );
                        CHECK_NEG("FlushSAudioChannel", status);
                        }
                else if ( IsChanActive(chanPtr) )
                        {
                        status = InternalPauseSAudioChannel( ctx, channelNumber );
                        CHECK_NEG("InternalPauseSAudioChannel", status);
                        }
                }

        chanPtr->status &= ~CHAN_ACTIVE;

        return status;
        }


/******************************************************************************
 * Routine to disable further data flow for the given channel, and to cause
 * any queued data to be flushed.
 *
 * NOTE: This mutes and aborts the sound spooler but doesn't clear CHAN_ACTIVE
 * (Call StopSAudioChannel() to do both) so the channel is somewhere between
 * stopped and paused. Calling BeginSAudioPlaybackIfAppropriate() and maybe also
 * QueueWaitingMsgsToSoundSpooler() should get it going again.
 * [TBD] So does the kSAudioCtlOpFlushChannel control msg get stuck?
 ******************************************************************************/
     Err FlushSAudioChannel( SAudioContextPtr ctx, uint32 channelNumber )
{
  Err                                     status  = kDSNoErr;
  SAudioChannelPtr        chanPtr;
        
  if ( channelNumber >= SA_SUBS_MAX_CHANNELS )
    return kDSChanOutOfRangeErr;

  ADD_TRACE_L2( SATraceBufPtr, kTraceChannelFlush, channelNumber, 0, 0 );
  
  chanPtr  =  ctx->channel + channelNumber;
  
  /* NOTE: Need to flush even if !IsChanActive(), e.g. if paused. */
  if ( IsChanEnabled(chanPtr) )
    {
      DecoderMsgPtr decoderMsg;

      /* Abort the soundspooler.  This will give back to the streamer, 
       * all buffers already queued to the soundspooler. */
      status = InternalStopSAudioChannel( ctx, channelNumber );

      /* Give back all queued chunks for this channel to the
       * stream parser. We do this by replying to all the
       * "chunk arrived" messages that we have queued. */

      /* Always assume MPAU_CHUNK_TYPE (W. Hsu 5-21-96) */

      decoderMsg = ALLOC_DECODER_MSG_POOL_MEM( ctx );
        
      /* Send flush command to the decoder */
      if( decoderMsg != NULL )
	{
#ifdef DEBUG_PRINT
	  PRNT(( "flushReadWriteMsg @ addr 0x%x, channel = %d, msgItem = 0x%x\n",
		decoderMsg, channelNumber, decoderMsg->msgItem ));
	  PRNT(("flushReadWriteMsg addr: 0x%x\n", decoderMsg ));
#endif
        
	  ADD_TRACE_L1( SATraceBufPtr, kTraceFlushReadWriteMsg,
		       decoderMsg->channel, decoderMsg->messageType,
		       decoderMsg );
	  decoderMsg->messageType = flushReadWriteMsg;
	  status = SendDecoderMsg( ctx, decoderMsg );
	  CHECK_NEG( "SendDecoderMsg", status );
        
	} /* if( decoderMsg != NULL ) */
        
    } /* if ( IsChanEnabled(chanPtr) ) */
  
  return status;
} /* FlushSAudioChannel() */

/******************************************************************************
 * Stop and Flush a channel; Then free up all it's resources.  Should leave a channel
 * in pre-initalized state.
 ******************************************************************************/
Err             CloseSAudioChannel( SAudioContextPtr ctx, uint32 channelNumber )
        {
        Err                                     status;
        SAudioChannelPtr        chanPtr;
        SAudioOutputPtr         chanOutputPtr;

        if ( channelNumber >= SA_SUBS_MAX_CHANNELS )
                return kDSChanOutOfRangeErr;

        chanPtr = ctx->channel + channelNumber;
        chanOutputPtr = &chanPtr->channelOutput;

        ADD_TRACE_L2( SATraceBufPtr, kTraceChannelClose, channelNumber, 0, 0 );

        /* Stop any activity and flush pending buffers if any */
        status = StopSAudioChannel( ctx, channelNumber, TRUE );
        FAIL_NEG("StopSAudioChannel", status);

        /* Clean up and free the pool of AudioFolio buffers that we created
         * for this channel. */
        if ( chanPtr->spooler != NULL )
                {
                /* Remove the all the cue signals for this spooler. */
                ctx->SpoolersSignalMask &= (~chanPtr->spooler->sspl_SignalMask);

                status = ssplDeleteSoundSpooler( chanPtr->spooler );
                CHECK_NEG("ssplDeleteSoundSpooler", status);
                chanPtr->spooler = NULL;

                }

        /* If the channel was initialized, free it up */
        /* No need to worry about deleting channelInstrument (the instrument used
         * for playback.  ssplDeleteSoundSpooler will delete it */
        if ( chanPtr->channelInstrument > 0 )
                {
		/* Disconnect all inputs of from the deemphasis and mixer
		   instruments */
                status = StopInstrument(chanOutputPtr->deemphasisLeft, NULL);
                FAIL_NEG("StopInstrument deemphasisLeft", status);
                status = StopInstrument(chanOutputPtr->deemphasisRight, NULL);
                FAIL_NEG("StopInstrument deemphasisRight", status);

		status = DisconnectInstrumentParts( chanOutputPtr->deemphasisLeft, "Input", 0);
		FAIL_NEG("DisconnectInstrumentParts", status);
		status = DisconnectInstrumentParts( chanOutputPtr->deemphasisRight, "Input", 0);
		FAIL_NEG("DisconnectInstrumentParts", status);
		
                status = DisconnectInstrumentParts( chanOutputPtr->instrument, "Input", 0 );
                FAIL_NEG("DisconnectInstrumentParts", status);

		status = DisconnectInstrumentParts( chanOutputPtr->instrument, "Input", 1 );
		FAIL_NEG("DisconnectInstrumentParts", status);

		status = DeleteInstrument(chanOutputPtr->deemphasisLeft);
                FAIL_NEG("DeleteInstrument", status);
		chanOutputPtr->deemphasisLeft = 0;
		status = DeleteInstrument(chanOutputPtr->deemphasisRight);
                FAIL_NEG("DeleteInstrument", status);
		chanOutputPtr->deemphasisRight = 0;

                status = DeleteInstrument(chanPtr->channelInstrument);
                FAIL_NEG("DeleteInstrument", status);
                chanPtr->channelInstrument = 0;
                }

        if ( chanOutputPtr->instrument > 0 )
                {
                /* The 2x2 mixer */
                status = StopInstrument(chanOutputPtr->instrument, NULL);
                FAIL_NEG("StopInstrument mixer", status);

		/* The line-out instrument */
		status = StopInstrument(chanOutputPtr->lineOut, NULL);
		FAIL_NEG("StopInstrument lineOut", status);

                /* The master gain envelope */
                status = StopInstrument(chanOutputPtr->masterGainEnv, NULL);
                FAIL_NEG("StopInstrument master gain envelope", status);

                status = DisconnectInstrumentParts( chanOutputPtr->lineOut, "Input", 0);
                FAIL_NEG("DisconnectInstrumentParts", status);
                status = DisconnectInstrumentParts( chanOutputPtr->lineOut, "Input", 1);
                FAIL_NEG("DisconnectInstrumentParts", status);

                status = DisconnectInstrumentParts( chanOutputPtr->instrument, "Amplitude", 0);
                FAIL_NEG("DisconnectInstrumentParts", status);

		/* Delete the master gain envelop */
                status = DeleteInstrument(chanOutputPtr->masterGainEnv);
                FAIL_NEG("DeleteInstrument", status);
                chanOutputPtr->masterGainEnv = 0;

		/* Delete the master gain knob */
		status = DeleteKnob(chanOutputPtr->masterGainEnvTargetKnob);
		FAIL_NEG("DeleteKnob", status);
		chanOutputPtr->masterGainEnvTargetKnob = 0;

		/* Delete the lineOut instrument */
                status = DeleteInstrument(chanOutputPtr->lineOut);
                FAIL_NEG("DeleteInstrument", status);
                chanOutputPtr->lineOut = 0;
		
		/* Now delete the mixer instrument */
                status = DeleteInstrument(chanOutputPtr->instrument);
                FAIL_NEG("DeleteInstrument", status);
                chanOutputPtr->instrument = 0;

        }

FAILED:

        /* Reset all of the channel's variables */
        memset( chanPtr, 0, sizeof(SAudioChannel) );
        chanPtr->channelOutput.currentAmp                       = -1;

        return status;
        }

/******************************************************************************
 * Routine to set a channel's amplitude.
 ******************************************************************************/
Err SetSAudioChannelAmplitude( SAudioContextPtr ctx, uint32 channelNumber,
                int32 newAmp )
        {
        SAudioChannelPtr        chanPtr;
        SAudioOutputPtr         chanOutputPtr;
        Err                                     status;

        chanPtr                 = ctx->channel + channelNumber;
        chanOutputPtr   = &chanPtr->channelOutput;

        ADD_TRACE_L3( SATraceBufPtr, kBeginSetChannelAmp, channelNumber,
                                                        chanOutputPtr->currentAmp, 0 );

        if ( (newAmp < MIN_AMPLITUDE_RANGE) || (newAmp > MAX_AMPLITUDE_RANGE) )
                return kDSVolOutOfRangeErr;

        /* If instrument == 0 then the channel has not been initalized so
         * just store the value and don't try to change a knob that doesn't
         * exist yet. If channel is muted, save value in savedAmp.
         */
        if ( (chanOutputPtr->instrument > 0) && (!chanOutputPtr->muted) )
                {
		  /* Set the ramp target */
		  status = SetKnob(chanOutputPtr->masterGainEnvTargetKnob, newAmp/32768.0 );
		  CHECK_NEG( "SetKnob ramp target", status );
                }

        /* If the channel is muted or the instrument was not created yet,
         * real amplitude values were not changed; but, we want
         * GetSAudioChannelAmplitude() to return the new setting AND we
         * want UnMuteSAudioChannel() to restore to the proper value.  */
        chanOutputPtr->currentAmp = newAmp;

        if ( chanOutputPtr->muted )
                chanOutputPtr->savedAmp = newAmp;

        ADD_TRACE_L3( SATraceBufPtr, kEndSetChannelAmp, channelNumber,
                                                        chanOutputPtr->currentAmp, 0 );

        return kDSNoErr;
        }

/******************************************************************************
 * Routine to set a channel's pan.
 ******************************************************************************/
#if 0  /* Not used */
Err SetSAudioChannelPan( SAudioContextPtr ctx, uint32 channelNumber,
                int32 newPan )
        {
        SAudioChannelPtr        chanPtr;
        SAudioOutputPtr         chanOutputPtr;
        int32                           leftAmp;
        int32                           rightAmp;
        Err                                     status;

        if ( channelNumber >= SA_SUBS_MAX_CHANNELS )
                return kDSChanOutOfRangeErr;

        chanPtr                 = ctx->channel + channelNumber;
        chanOutputPtr   = &chanPtr->channelOutput;

        ADD_TRACE_L3( SATraceBufPtr, kBeginSetChannelPan, channelNumber,
                                                        chanOutputPtr->currentPan, 0 );

        if ( (newPan < 0x0) || (newPan > 0x7FFF) )
                return kDSPanOutOfRangeErr;

        /* If instrument == 0 then the channel has not been initalized so
         * just store the value and don't try to change a knob that doesn't
         * exist yet.  If the channel is muted don't tweak knobs.
         */
        if ( (chanOutputPtr->instrument > 0) && (!chanOutputPtr->muted) )
                {
                /* Ignore pan commands if this is a stereo stream */
                if ( chanOutputPtr->numChannels == 1 )
                        {
                        if (newPan < 0x4000)
                                {
                                leftAmp = chanOutputPtr->currentAmp;
                                rightAmp = (newPan * chanOutputPtr->currentAmp) >> 14;
                                }
                        else
                                {
                                rightAmp = chanOutputPtr->currentAmp;
                                leftAmp = ((0x8000 - newPan) *
                                        chanOutputPtr->currentAmp) >> 14;
                                }

                        /* Set the left ramp target */
                        status = SetKnob(chanOutputPtr->leftEnvTargetKnob, (float32)leftAmp/32768.0 );
                        CHECK_NEG( "SetKnob left", status );

                        /* Set the right ramp target */
                        status = SetKnob(chanOutputPtr->rightEnvTargetKnob, (float32)rightAmp/32768.0 );
                        CHECK_NEG( "SetKnob right knob", status );
                        }
                else
                        {
                        /* if this is a stereo stream return an error and don't
                         * change any knobs. */
                        return kDSAudioChanErr;
                        }
                }

        chanOutputPtr->currentPan = newPan;

        ADD_TRACE_L3( SATraceBufPtr, kEndSetChannelPan, channelNumber,
                                                        chanOutputPtr->currentPan, 0 );

        return kDSNoErr;
        }
#endif

/******************************************************************************
 * Routine to get a channel's amplitude.  If the channel is currently muted, return
 * the amplitude that it will be restored to when unmute occurs.
 ******************************************************************************/
Err GetSAudioChannelAmplitude( SAudioContextPtr ctx, uint32 channelNumber,
                int32* Amp )
        {
        SAudioChannelPtr        chanPtr;
        SAudioOutputPtr         chanOutputPtr;

        if ( channelNumber >= SA_SUBS_MAX_CHANNELS )
                return kDSChanOutOfRangeErr;

        chanPtr                 = ctx->channel + channelNumber;
        chanOutputPtr   = &chanPtr->channelOutput;

        if ( chanOutputPtr->muted )
                *Amp = chanOutputPtr->savedAmp;
        else
                *Amp = chanOutputPtr->currentAmp;

        return kDSNoErr;
        }

/******************************************************************************
 * Routine to get a channel's pan.
 ******************************************************************************/
#if 0  /* Not used */
Err GetSAudioChannelPan( SAudioContextPtr ctx, uint32 channelNumber,
                int32* Pan )
        {
        SAudioChannelPtr        chanPtr;
        SAudioOutputPtr         chanOutputPtr;

        if ( channelNumber >= SA_SUBS_MAX_CHANNELS )
                return kDSChanOutOfRangeErr;

        chanPtr                 = ctx->channel + channelNumber;
        chanOutputPtr   = &chanPtr->channelOutput;

        *Pan = chanOutputPtr->currentPan;

        return kDSNoErr;
        }
#endif

/******************************************************************************
 * Routine to save current amplitude and then set it to 0.
 ******************************************************************************/
Err MuteSAudioChannel( SAudioContextPtr ctx, uint32 channelNumber,
                bool external )
        {
        Err                                     status = kDSNoErr;
        SAudioChannelPtr        chanPtr;
        SAudioOutputPtr         chanOutputPtr;

        if ( channelNumber >= SA_SUBS_MAX_CHANNELS )
                return kDSChanOutOfRangeErr;

        chanPtr                 = ctx->channel + channelNumber;
        chanOutputPtr   = &chanPtr->channelOutput;

        /* Remember if we were muted from a control message so we won't
         * UnMute if the channel is stopped and restarted. */
        if ( external )
                chanOutputPtr->externalMute = external;

        /* If the channel is not already muted, save current amplitude and
         * turn volume down. */
        if ( !chanOutputPtr->muted )
                {
                GetSAudioChannelAmplitude( ctx, channelNumber,
                        &chanOutputPtr->savedAmp );
                status = SetSAudioChannelAmplitude( ctx, channelNumber, 0 );
                chanOutputPtr->muted = TRUE;

                ADD_TRACE_L3( SATraceBufPtr, kChannelMuted, channelNumber,
                                                        chanOutputPtr->savedAmp, 0 );
                }

        return status;
        }

/******************************************************************************
 * Routine to restore amplitude from state saved by MuteSAudioChannel().
 ******************************************************************************/
Err     UnMuteSAudioChannel( SAudioContextPtr ctx, uint32 channelNumber,
                bool external )
        {
        int32                           status = kDSNoErr;
        SAudioChannelPtr        chanPtr;
        SAudioOutputPtr         chanOutputPtr;

        if ( channelNumber >= SA_SUBS_MAX_CHANNELS )
                return kDSChanOutOfRangeErr;

        chanPtr                 = ctx->channel + channelNumber;
        chanOutputPtr   = &chanPtr->channelOutput;

        /* Don't do anything if we're not muted. */
        if ( chanOutputPtr->muted )
                {
                /* If we were Muted by the user with a control message and s/he is now
                 * trying to unmute us, do it.  If we are being unmuted by an internal
                 * call and we were last muted by the user, don't fuck with the outside
                 * user's desires. */
                if ( (external && chanOutputPtr->externalMute) ||
                         (!external && !chanOutputPtr->externalMute) )
                        {
                        chanOutputPtr->muted = FALSE;
                        status = SetSAudioChannelAmplitude( ctx, channelNumber,
                                chanOutputPtr->savedAmp );
                        ADD_TRACE_L3( SATraceBufPtr, kChannelUnMuted, channelNumber,
                                                        chanOutputPtr->currentAmp, 0 );

                        /* Always Clear flag */
                        chanOutputPtr->externalMute = FALSE;
                        }
                }

        return status;
        }

/******************************************************************************
 * Stop all channels for this subscription.
 ******************************************************************************/                
Err     StopAllAudioChannels(SAudioContextPtr ctx, bool fFlush)
        {
        uint32          channelNumber;

        for ( channelNumber = 0; channelNumber < SA_SUBS_MAX_CHANNELS;
                        channelNumber++ )
                StopSAudioChannel(ctx, channelNumber, fFlush);  /* [TBD] check status */

        return kDSNoErr;
        }


/******************************************************************************
 * Close all channels for this subscription.
 ******************************************************************************/                
Err     CloseAllAudioChannels(SAudioContextPtr ctx)
        {
        uint32          channelNumber;

        for ( channelNumber = 0; channelNumber < SA_SUBS_MAX_CHANNELS;
                        channelNumber++ )
                CloseSAudioChannel(ctx, channelNumber); /* [TBD] check status */

        if( ctx->dataType == MPAU_CHUNK_TYPE )
                /* Close the mpeg decoder. */
                CloseMPEGDecoder( ctx );

        return kDSNoErr;
        }


/******************************************************************************
 * Flush all channels for this subscription without stopping data flow.
 ******************************************************************************/                
Err     FlushAllAudioChannels(SAudioContextPtr ctx)
{
        Err                     status = kDSNoErr;
        uint32          channelNumber;

        for ( channelNumber = 0; channelNumber < SA_SUBS_MAX_CHANNELS;
                        channelNumber++ )
                status = FlushSAudioChannel(ctx, channelNumber);

        return status;  /* [TBD] combine status values */
}

/******************************************************************************
 * Routine to figure out if it's time to start playing.
 ******************************************************************************/
Err BeginSAudioPlaybackIfAppropriate( SAudioContextPtr ctx,
				     uint32 channelNumber )
{
  SAudioChannelPtr        chanPtr;
  int32                           spoolerStatus;
  SoundBufferNode         *sbn;
  Err                                     status = kDSNoErr;

  ADD_TRACE_L2( SATraceBufPtr, kBeginPlayback, channelNumber, 0, 0 );

  if ( channelNumber >= SA_SUBS_MAX_CHANNELS )
    {
      PERR(("BeginSAudioPlaybackIfAppropriate invalid channel # %d\n",
	    channelNumber));
      return kDSChanOutOfRangeErr;

    }

  chanPtr  = ctx->channel + channelNumber;


  if( IsChanEnabled(chanPtr) && IsChanActive(chanPtr) ) 
    {
      spoolerStatus = ssplGetSpoolerStatus( chanPtr->spooler );

      /* If the soundspooler has data queued to it and it's stopped, start things up. */
      if( (spoolerStatus & SSPL_STATUS_F_ACTIVE) &&
	 !(spoolerStatus & SSPL_STATUS_F_STARTED) )
	{
	  /* Start up the spooler. The soundspooler will call
	   * StartAttachment(). */
	  status = ssplStartSpoolerTags( chanPtr->spooler, NULL );
	  CHECK_NEG("ssplStartSpoolerTags", status);
	  ADD_TRACE_L2( SATraceBufPtr, kStartSpooler, channelNumber, 0, 0 );
        
	  sbn = FirstActiveNode(chanPtr->spooler);
	  if( sbn != NULL )
	    {
	      /* Always assume MPAU_CHUNK_TYPE (W. Hsu 5-9-96) */
	      DecoderMsgPtr decoderMsgPtr =
			(DecoderMsgPtr)ssplGetUserData( chanPtr->spooler, sbn );

	      /* Update clock with current stream time from this chunk
	       * if this data belongs to the clock channel. */
	      /* <HPP> check for valid pts. If not valid do not set the clock */
	      if ( channelNumber == ctx->clockChannel && decoderMsgPtr->timeIsValidFlag )
				DSSetPresentationClock(ctx->streamCBPtr,
					decoderMsgPtr->branchNumber, decoderMsgPtr->presentationTime);
        
	    } /* if( sbn != NULL ) */

	  /* Restore gain on the mixer after StartAttachment() has
	   * been called.  This should guarantee no more pops. */
	  status = UnMuteSAudioChannel( ctx, channelNumber, INTERNAL_UNMUTE );

	} /* if( !(spoolerStatus & SSPL_STATUS_F_STARTED) ) */

      /* Otherwise see if we are paused and if so, resume from where
       * we were */
      else if ( spoolerStatus & SSPL_STATUS_F_PAUSED )
	{
	  status = ssplResume( chanPtr->spooler );
	  CHECK_NEG("ssplResume", status);
        
	  /* ReStart the spooler exactly where it left off */
	  ADD_TRACE_L2( SATraceBufPtr, kResumeSpooler, channelNumber,
		       0, 0 );

	  /* The Streamer resumes the clock so we don't need to do it here. */

	  /* Restore gain on the mixer after ResumeInstrument() has
	   * been called.  This should guarantee no more pops. */
	  status = UnMuteSAudioChannel( ctx, channelNumber, INTERNAL_UNMUTE );
        
	} /* else if ( spoolerStatus & SSPL_STATUS_PAUSED ) */
    } /* if( (IsChanEnabled(chanPtr) && (IsChanActive(chanPtr) ) */

  return status;

}

Err SetMixerStereo(SAudioContextPtr ctx, uint32 channelNumber)
{
  Err status;
  Item tempKnob;
  SAudioChannelPtr chanPtr = ctx->channel + channelNumber;
  SAudioOutputPtr chanOutputPtr = &chanPtr->channelOutput;

  status = CreateKnob( chanOutputPtr->instrument, "Gain", NULL );
  FAIL_NEG("CreateKnob", status);
  tempKnob = status;

  SetKnobPart ( tempKnob, 0, MAX_AMPLITUDE_RANGE );
  SetKnobPart ( tempKnob, 2, MIN_AMPLITUDE_RANGE );
  SetKnobPart ( tempKnob, 1, MIN_AMPLITUDE_RANGE );
  SetKnobPart ( tempKnob, 3, MAX_AMPLITUDE_RANGE );

  DeleteKnob( tempKnob );
  
 FAILED:
  return status;
}

Err SetMixerMono(SAudioContextPtr ctx, uint32 channelNumber)
{
  Err status;
  Item tempKnob;
  SAudioChannelPtr chanPtr = ctx->channel + channelNumber;
  SAudioOutputPtr chanOutputPtr = &chanPtr->channelOutput;

  status = CreateKnob( chanOutputPtr->instrument, "Gain", NULL );
  FAIL_NEG("CreateKnob", status);
  tempKnob = status;

  SetKnobPart ( tempKnob, 0, AVE_AMPLITUDE_RANGE );
  SetKnobPart ( tempKnob, 2, AVE_AMPLITUDE_RANGE );
  SetKnobPart ( tempKnob, 1, AVE_AMPLITUDE_RANGE );
  SetKnobPart ( tempKnob, 3, AVE_AMPLITUDE_RANGE );

  DeleteKnob( tempKnob );

 FAILED:
  return status;
}

Err SetMixerLeftOnly(SAudioContextPtr ctx, uint32 channelNumber)
{
  Err status;
  Item tempKnob;
  SAudioChannelPtr chanPtr = ctx->channel + channelNumber;
  SAudioOutputPtr chanOutputPtr = &chanPtr->channelOutput;

  status = CreateKnob( chanOutputPtr->instrument, "Gain", NULL );
  FAIL_NEG("CreateKnob", status);
  tempKnob = status;

  SetKnobPart ( tempKnob, 0, MAX_AMPLITUDE_RANGE );
  SetKnobPart ( tempKnob, 2, MAX_AMPLITUDE_RANGE );
  SetKnobPart ( tempKnob, 1, MIN_AMPLITUDE_RANGE );
  SetKnobPart ( tempKnob, 3, MIN_AMPLITUDE_RANGE );

  DeleteKnob( tempKnob );
  
 FAILED:
  return status;
}

Err SetMixerRightOnly(SAudioContextPtr ctx, uint32 channelNumber)
{
  Err status;
  Item tempKnob;
  SAudioChannelPtr chanPtr = ctx->channel + channelNumber;
  SAudioOutputPtr chanOutputPtr = &chanPtr->channelOutput;

  status = CreateKnob( chanOutputPtr->instrument, "Gain", NULL );
  FAIL_NEG("CreateKnob", status);
  tempKnob = status;

  SetKnobPart ( tempKnob, 0, MIN_AMPLITUDE_RANGE );
  SetKnobPart ( tempKnob, 2, MIN_AMPLITUDE_RANGE );
  SetKnobPart ( tempKnob, 1, MAX_AMPLITUDE_RANGE );
  SetKnobPart ( tempKnob, 3, MAX_AMPLITUDE_RANGE );

  DeleteKnob( tempKnob );

 FAILED:
  return status;
}

Err AssertDeemphasis(SAudioContextPtr ctx, uint32 channelNumber)
{
  Err status;
  SAudioChannelPtr chanPtr = ctx->channel + channelNumber;
  SAudioOutputPtr chanOutputPtr = &chanPtr->channelOutput;

  if (chanOutputPtr->deemphasisOn != TRUE) {
    chanOutputPtr->deemphasisOn = TRUE;

    /* First disconnect any inputs to the mixer instrument */
    status = DisconnectInstrumentParts(chanOutputPtr->instrument,
				       "Input", 0);
    FAIL_NEG("DisconnectInstrumentParts", status);
    status = DisconnectInstrumentParts(chanOutputPtr->instrument,
				       "Input", 1);
    FAIL_NEG("DisconnectInstrumentParts", status);

    /* Now connect the deemphasis instrument to input of the mixer */
    status = ConnectInstrumentParts(chanOutputPtr->deemphasisLeft,
				    "Output", 0,
				    chanOutputPtr->instrument, "Input", 0);
    FAIL_NEG("ConnectInstrumentParts Input 0", status);
      
    status = ConnectInstrumentParts(chanOutputPtr->deemphasisRight,
				    "Output", 0,
				    chanOutputPtr->instrument, "Input", 1);
    FAIL_NEG("ConnectInstrumentParts Input 1", status);
  }
  
  status = kDSNoErr;

 FAILED:
  return status;
}

Err NoDeemphasis(SAudioContextPtr ctx, uint32 channelNumber)
{
  Err status;
  SAudioChannelPtr chanPtr = ctx->channel + channelNumber;
  SAudioOutputPtr chanOutputPtr = &chanPtr->channelOutput;

  if (chanOutputPtr->deemphasisOn != FALSE) {
    chanOutputPtr->deemphasisOn = FALSE;

    /* First disconnect any inputs to the mixer instrument */
    status = DisconnectInstrumentParts(chanOutputPtr->instrument,
				       "Input", 0);
    FAIL_NEG("DisconnectInstrumentParts", status);
    status = DisconnectInstrumentParts(chanOutputPtr->instrument,
				       "Input", 1);
    FAIL_NEG("DisconnectInstrumentParts", status);

    /* Now connect the sampler instrument directly to input of the mixer */
    status = ConnectInstrumentParts(chanPtr->channelInstrument,
				    "Output", 0,
				    chanOutputPtr->instrument, "Input", 0);
    FAIL_NEG("ConnectInstrumentParts Input 0", status);
      
    status = ConnectInstrumentParts(chanPtr->channelInstrument,
				    "Output", 1,
				    chanOutputPtr->instrument, "Input", 1);
    FAIL_NEG("ConnectInstrumentParts Input 1", status);
  }
  
  status = kDSNoErr;

 FAILED:
  return status;
}
