/******************************************************************************
**
**  @(#) sachannel.h 96/06/05 1.2
**
******************************************************************************/
#ifndef __SACHANNEL_H__
#define __SACHANNEL_H__

#include <audio/soundspooler.h>
#ifndef __KERNEL_TYPES_H
	#include <kernel/types.h>
#endif

#ifndef __STREAMING_DATASTREAM_H
	#include <video_cd_streaming/datastream.h>
#endif

#ifndef __STREAMING_DSSTREAMDEFS_H
	#include <video_cd_streaming/dsstreamdefs.h> /* for SNDS chunk */
#endif

#include <streaming/satemplatedefs.h>
#include <video_cd_streaming/subscriberutils.h>

/********************
 * Constants        *
 ********************/

/* max # of logical channels per subscription */
/* Use only one channel  (W. Hsu 5-9-96) */
#define	SA_SUBS_MAX_CHANNELS		1

/* max # of sample buffers which can be queued 
 * to the folio at on time; per channel 
 */
#define	SA_SUBS_MAX_BUFFERS			8		

#define MIXER_SPEC					MakeMixerSpec( 2, 2, AF_F_MIXER_WITH_AMPLITUDE )

/* For loading instrument templates */
#define SA_ENVELOPE_INSTRUMENT_NAME		"envelope.dsp"

/* Passed to MuteSAudioChannel() so that we know if we were muted
 * by the user, or internally.
 */
#define		INTERNAL_MUTE		FALSE
#define		INTERNAL_UNMUTE		FALSE
#define		USER_MUTE			TRUE
#define		USER_UNMUTE			TRUE

#define SIZE_OF_DECOMPRESSED_BUFFER (MPEG_SAMPLES_PER_AUDIO_FRAME * 4)

/* MPEG status bits (ctx->status) */
#define MPEG_CHAN_BYPASS  (1<<19)

/********************************************/
/* Handy macros for testing channel status. */
/********************************************/
#define IsChanInitalized(x)	((x)->channelOutput.instrument > 0)
#define IsChanEnabled(x)	((x)->status & CHAN_ENABLED)
#define IsChanActive(x)		((x)->status & CHAN_ACTIVE)

/* This macro encapsulate allocation from our MemPools, esp. the typecasts. */
#define ALLOC_DECOMPRESSED_BFR_MSG_POOL(SAudioContextPtr) \
	((DecoderMsgPtr)AllocPoolMem((SAudioContextPtr)->datatype.MPEGAudio.decompressedBfrMsgPool))
#define ALLOC_DECODER_MSG_POOL_MEM(SAudioContextPtr) \
	((DecoderMsgPtr)AllocPoolMem((SAudioContextPtr)->datatype.MPEGAudio.decoderMsgPool))


/*****************************************************************
 * output instrument Item and fields necessary to control
 * volume and panning.
 *****************************************************************/

typedef struct SAudioOutput {
	Item		instrument;			/* instrument item */
	Item            deemphasisLeft;                 /* deemphasis item */
	Item            deemphasisRight;                /* deemphasis item */
	Item            lineOut;                        /* line out item */
	int32		numChannels;		/* is instrument mono or stereo or ? */
	Item            masterGainEnv;          /* master gain target envelope */
	Item            masterGainEnvTargetKnob;   /* knob to set target envelope */
	int32 		currentAmp;				
	int32 		savedAmp;			/* previous amplitude to restore,
									 * used for muting */
	bool		muted;				/* is channel in mute mode */
	bool		externalMute;		/* Should channel be unmuted by
									 * StartSAudioChannel()? */
	bool            deemphasisOn;             /* Is deemphasis asserted? */
	} SAudioOutput, *SAudioOutputPtr;

/************************************************/
/* Channel context, one per channel, per stream */
/************************************************/

struct SAudioBufferPtr;   /* forward ref */

typedef struct SAudioChannel {
	uint32				status; 			/* state bits (see below) */
	SoundSpooler		*spooler;			/* sound spooler for this channel */
	uint32				numBuffers;			/* # buffers that can be
											 * queued to soundspooler */

	Item				channelInstrument;	/* DSP instrument to play
											 * channel's data chunks */
	SubsQueue			dataQueue;			/* waiting data chunks */
	int32				inuseCount;			/* number of buffers currently
											 * in the in use queue */
	SAudioOutput		channelOutput;		/* contains output instrument */
											/* and control knobs */
	} SAudioChannel, *SAudioChannelPtr;
	

/****************************************************************************
 * Structure to describe instrument template items that may be dynamically
 * loaded. An array of these describes all the possible templates we know
 * about. We actually load an instrument template when explicitly asked to
 * do so with a control message or when we encounter a header in a stream
 * that requires an instrument type for which we do not have a loaded
 * template.
 * Each instantiation of an audio subscriber gets a clean copy of an
 * initial array of these structures which it uses to cache templates.
 ****************************************************************************/
typedef struct TemplateRec {
	int32		templateTag;	/* used to match caller input value */
	Item		templateItem;	 /* item for the template or zero */
	char*		instrumentName;	 /* ptr to string of filename */
} TemplateRec, *TemplateRecPtr;


/*******************************************************
 * Subscriber private context structure, one per stream
 *******************************************************/

typedef struct SAudioContext {
    Item            requestPort;        /* message port item for */
                                        /* subscriber requests */
    uint32          requestPortSignal;  /* signal to detect request port messages */

    DSStreamCBPtr   streamCBPtr;        /* stream this subscriber belongs to */

	uint32   		SpoolersSignalMask;	/* the ORd signals for all the active spoolers */

    Item            outputTemplateItem; /* template item for making channel's output instrument */

    uint32			clockChannel;       /* which logical channel to use for clock */

	SAudioChannel	channel[SA_SUBS_MAX_CHANNELS];
                                        /* an array of channels */

    Item			envelopeTemplateItem;
										/* template item for making channel's */
										/* envelope instrument(s) */
	DSDataType		dataType;			/* 'SNDS' or 'MPAU' */

	union {
		struct {
    		TemplateRecPtr	templateArray;
										/* ptr to array of template records */
			Item		decodeADPCMIns;	/* Support for playing ADPCM data */
			} ThdoAudio;

		struct {
			Item		decoderPort;	/* message port to send data &
										 * commands to the decoder */
			MemPoolPtr	decoderMsgPool;	/* pool of decoder message blocks */
			MemPoolPtr	decompressedBfrMsgPool;
										/* pool of decompressed buffer
										 * message blocks */

			uint32		*audioBfrs;		/* pointer to decompressed MPEG 
										 * audio buffers block */

    		Item		replyPort;		/* reply port for MPEG audio */
										/* decoder thread */

			uint32		replyPortSignal;
										/* signal for request port */

			uint32		pendingReplies;	/* number of outstanding replies */

    		uint32		activeChannel;	/* the currently active channel */
 
    		uint32		lastPTS;		/* the PTS of the last frame we decompressed */
    		uint32		lastTimeIsValid;
			} MPEGAudio;

		} datatype;

    uint32 trackAudioMode;    /* enum MPEGAudioMode */
    uint32 userAudChoice;     /* enum MPEGUserAudioChoice */
    uint32 status;            /* MPEG state bits */

    } SAudioContext, *SAudioContextPtr;

/*******************************************************
 * extern constants
 *******************************************************/

extern const		TemplateRec gInitialTemplates[/* kMaxTemplateCount */];
extern const uint32	kMaxTemplateCount;


/*******************************************************
 * Public procedures
 *******************************************************/

#ifdef __cplusplus 
extern "C" {
#endif

int32	GetTemplateTag( SAudioSampleDescriptorPtr descPtr );
Item	GetTemplateItem( SAudioContextPtr ctx,
                         int32 templateTag, uint32 templateCount );
Err		LoadTemplates( SAudioContextPtr ctx, int32* tagPtr,
                        uint32 templateCount );

Err		InitSAudioChannel( SAudioContextPtr ctx);

Err		StartSAudioChannel(SAudioContextPtr ctx,
							uint32 channelNumber, bool doFlush );
Err		StopSAudioChannel( SAudioContextPtr ctx,
							uint32 channelNumber, bool doFlush );
Err		StopAllAudioChannels( SAudioContextPtr ctx, bool fFlush );

Err		FlushSAudioChannel( SAudioContextPtr ctx, uint32 channelNumber );
Err		FlushAllAudioChannels( SAudioContextPtr ctx );

Err		CloseSAudioChannel( SAudioContextPtr ctx, uint32 channelNumber );
Err		CloseAllAudioChannels(SAudioContextPtr ctx);

Err		SetSAudioChannelAmplitude( SAudioContextPtr ctx,
							uint32 channelNumber, int32 newAmp );
Err		SetSAudioChannelPan( SAudioContextPtr ctx,
							uint32 channelNumber, int32 newPan );
Err		GetSAudioChannelAmplitude( SAudioContextPtr ctx,
							uint32 channelNumber, int32* Amp );
Err		GetSAudioChannelPan( SAudioContextPtr ctx,
							uint32 channelNumber, int32* Pan );
Err		MuteSAudioChannel( SAudioContextPtr ctx,
							uint32 channelNumber, bool callerFlag );
Err		UnMuteSAudioChannel( SAudioContextPtr ctx,
							uint32 channelNumber, bool callerFlag );
Err		BeginSAudioPlaybackIfAppropriate( SAudioContextPtr ctx,
							uint32 channelNumber );
Err             SetMixerStereo(SAudioContextPtr ctx,
			       uint32 channelNumber);
Err             SetMixerMono(SAudioContextPtr ctx,
			       uint32 channelNumber);
Err             SetMixerLeftOnly(SAudioContextPtr ctx,
			       uint32 channelNumber);
Err             SetMixerRightOnly(SAudioContextPtr ctx,
			       uint32 channelNumber);
Err             AssertDeemphasis(SAudioContextPtr ctx,
				 uint32 channelNumber);
Err             NoDeemphasis(SAudioContextPtr ctx,
			     uint32 channelNumber);

#ifdef __cplusplus
}
#endif

#endif	/* __SACHANNEL_H__ */
