/******************************************************************************
**
** @(#) playssndstream.c 96/03/29 1.25
** Contains:		high level Audio stream playback function
**
******************************************************************************/

#ifndef PRINT_LEVEL		/* may be set by a compiler command-line switch */
#define PRINT_LEVEL 1	/* print error msgs */
#endif

#include <stdio.h>
#include <string.h>
#include <audio/audio.h>
#include <kernel/msgport.h>			/* GetMsg */
#include <kernel/debug.h>			/* for print macro: CHECK_NEG */
#include <kernel/mem.h>				/* for FreeMemTrack */

#include <streaming/datastream.h>
#include <streaming/datastreamlib.h>
#include <streaming/sacontrolmsgs.h>
#include <streaming/saudiosubscriber.h>
#include <streaming/mpegaudiosubscriber.h>
#include <streaming/satemplatedefs.h>
#include <streaming/dsstreamdefs.h>

#include "playssndstream.h"


/************************************/
/* Local utility routine prototypes */
/************************************/
static int32	InitSSNDPlayerFromStreamHeader( PlayerPtr ctx,
												char* streamFileName );
void			DismantlePlayer( PlayerPtr ctx );
static int32	UseDefaultStreamHeader( DSHeaderChunkPtr headerPtr );


/******************************************************************************
 * Routine to play an SSND stream file. Stream may or may not contain audio data.
 * Audio channel selection is based upon info found in the stream header.
 ******************************************************************************/
int32	PlaySSNDStream( char* streamFileName, PlaySSNDUserFn userFn,
						void* userCB, int32 callBackInterval )
	{
	Err				status;
	int32			playerResult = 0;
	PlayerPtr		ctx;
	Player			playerContext;
	DSRequestMsg	EOSMessage;
	Item 			aCue;

	/* Open the specified file, locate the stream header, and initialize
	 * all resources necessary for performing stream playback. */
	status = InitSSNDPlayerFromStreamHeader( &playerContext, streamFileName );
	if ( status < 0 )
		return status;

	ctx = &playerContext;

	/* Remember the user function pointer and context, if any */
	ctx->userFn = userFn;
	ctx->userCB = userCB;

	/* Create an audio cue so we can wake up periodically to check user input */
	aCue = CreateItem(MKNODEID(AUDIONODE,AUDIO_CUE_NODE), NULL);
	if ( aCue <= 0 )
		return (int32)aCue;

	/* Register for end of stream notification. Do it before DSStartStream so
	 * we won't miss it in a short or empty stream. */
	status = DSWaitEndOfStream( ctx->endOfStreamMessageItem,
								&EOSMessage, ctx->streamCBPtr );
	FAIL_NEG( "DSWaitEndOfStream", status );

	/* Start the stream running */
	PRNT(("Starting the stream\n"));
	status = DSStartStream( ctx->messageItem, NULL, ctx->streamCBPtr, 0 );
	FAIL_NEG( "DSStartStream", status );

	/* --------------- Main playback loop ---------------
	 * NOTE: A better scheme here would be to WaitSignal() each time around the
	 * loop and process all received signals--audio cues indicating time to
	 * check for user input (better yet, control port event signals) and message
	 * port signals indicating time to check for the end-of-stream reply. */
	while ( TRUE )
		{
		/* If a user function is defined, then call it. If the user
		 * function returns a non-zero value, then exit. */
		if ( ctx->userFn )
			{
			playerResult = (*ctx->userFn)(ctx);
			if ( playerResult )
				break;
			}

		/* Exit if we received the end-of-stream msg reply.
		 * status < 0 is an error. status > 0 is our EOS-reply msg Item. */
		status = GetMsg(ctx->messagePort);
		FAIL_NEG("PlaySSNDStream GetMsg", status);
		if ( status > 0 )
			{
			status = MESSAGE(status)->msg_Result;
			FAIL_NEG("EOS reply", status);	/* report any error status */
			break;
			}
		
		/* Sleep until it's time to call the user function again */
		SleepUntilTime( aCue, callBackInterval + GetAudioTime() );
		}

	/* Unconditionally stop the stream */
	status = DSStopStream( ctx->messageItem, NULL, ctx->streamCBPtr, SOPT_FLUSH );
	FAIL_NEG( "DSStopStream", status );

	PRNT(("\nStopped the stream\n"));

FAILED:

	/* Free everything */
	DismantlePlayer( ctx );
	
	return ( status < 0 ) ? status : playerResult;
	}


/******************************************************************************
 * Routine to get the stream header info, and perform all necessary allocations
 * and initializations necessary for stream playback.
 ******************************************************************************/
static int32	InitSSNDPlayerFromStreamHeader( PlayerPtr ctx,
												char* streamFileName )
	{
	int32			status;
	int32			subscriberIndex;
	int32			channelNum;
	SAudioCtlBlock	ctlBlock;
	DSHeaderSubsPtr	subsPtr;

	/* Initialize fields to zero so that CLEANUP can
	 * tell what has been allocated.
	 */
	memset( ctx, 0, sizeof(Player) );
	
	/* Get the stream header loaded */
	status = FindAndLoadStreamHeader( &ctx->hdr, streamFileName );

    if ( status == kDSHeaderNotFound )
		status = UseDefaultStreamHeader( &ctx->hdr );

	if ( status != 0 )
		return status;

	/* Make sure this playback code is compatible with the version of the
	 * data in the stream file.
	 */
	if ( ctx->hdr.headerVersion != DS_STREAM_VERSION )
		return kDSVersionErr;


	/* Allocate the stream buffers and build the linked list of
	 * buffers that are input to the streamer.
	 */
	ctx->bufferList = CreateBufferList( ctx->hdr.streamBuffers,
										ctx->hdr.streamBlockSize );
	if ( ctx->bufferList == NULL )
		return kDSNoMemErr;

	/* We need to create a message port and message items
	 * to communicate with the data streamer. */
	status = ctx->messagePort = NewMsgPort( NULL );
	FAIL_NEG("NewMsgPort", status);

	status = ctx->messageItem = CreateMsgItem( ctx->messagePort );
	FAIL_NEG("CreateMsgItem", status);

	status = ctx->endOfStreamMessageItem = CreateMsgItem( ctx->messagePort );
	FAIL_NEG("CreateMsgItem", status);


	/* Initialize data acquisition for the specified file */
	status = ctx->acqMsgPort = NewDataAcq(streamFileName, ctx->hdr.dataAcqDeltaPri);
	FAIL_NEG("NewDataAcq", status);

	status = NewDataStream(
		&ctx->streamCBPtr,			/* output: stream control block ptr */
		ctx->bufferList,			/* pointer to buffer list */
		ctx->hdr.streamBlockSize,   /* size of each buffer */
		ctx->hdr.streamerDeltaPri,  /* streamer thread relative priority */
		ctx->hdr.numSubsMsgs );		/* number of subscriber messages */
	FAIL_NEG("InitSSNDPlayerFromStreamHeader NewDataStream", status );

	/* Connect the stream to its data supplier */
	status = DSConnect(ctx->messageItem, NULL, ctx->streamCBPtr, ctx->acqMsgPort);
	FAIL_NEG("DSConnect", status);

	/* Loop through the subscriber descriptor table and initialize all
	 * subscribers specified in the table.
	 */
	for ( subscriberIndex = 0;
		  ctx->hdr.subscriberList[ subscriberIndex ].subscriberType != 0;
			subscriberIndex++ )
		{
		subsPtr = ctx->hdr.subscriberList + subscriberIndex;
		
		switch ( subsPtr->subscriberType )
			{
			case SNDS_CHUNK_TYPE:
				status = NewSAudioSubscriber( ctx->streamCBPtr, subsPtr->deltaPriority,
							ctx->messageItem );
				FAIL_NEG( "NewSAudioSubscriber", status );
				ctx->datatype = SNDS_CHUNK_TYPE;

				break;

			case MPAU_CHUNK_TYPE:
				status = NewMPEGAudioSubscriber( ctx->streamCBPtr, subsPtr->deltaPriority,
							ctx->messageItem, NUMBER_MPEG_AUDIO_BUFFER );
				FAIL_NEG( "NewMPEGAudioSubscriber", status );
				ctx->datatype = MPAU_CHUNK_TYPE;
			
				break;

			case CTRL_CHUNK_TYPE:
				/* The ControlSubscriber is obsolete */
				break;

			default:
				status = kDSSubNotFoundErr;
				PRNT(("\n InitSSNDPlayerFromStreamHeader -- Unknown subscriber ident %.4s found in stream header!\n",
				(char*) &subsPtr->subscriberType));
				goto FAILED;
			}
		}

	/* Preload audio instrument templates, if any are specified
	 */
	if ( ctx->hdr.preloadInstList != 0 )
		{
		ctlBlock.loadTemplates.tagListPtr = ctx->hdr.preloadInstList;
	
		status = DSControl( ctx->messageItem, NULL, ctx->streamCBPtr,
							ctx->datatype,
							kSAudioCtlOpLoadTemplates, &ctlBlock );
		FAIL_NEG( "DSControl kSAudioCtlOpLoadTemplates", status );
		}

	/* Enable any audio channels whose enable bit is set.
	 * NOTE: Channel zero is enabled by default, so we don't check it.
	 */
	for ( channelNum = 1; channelNum < 32; channelNum++ )
		{
		/* If the bit corresponding to the channel number is set,
		 * then tell the audio subscriber to enable that channel.
		 */
		if ( ctx->hdr.enableAudioChan & (1L << channelNum) )
			{
			status = DSSetChannel(ctx->messageItem, NULL, ctx->streamCBPtr, 
				ctx->datatype, channelNum, CHAN_ENABLED, CHAN_ENABLED);
			FAIL_NEG( "DSSetChannel", status );

			/* Mute the channels that just got enabled.  We should only
			 * hear audio from the current channel. The current channel
			 * by default is channel zero. */
			ctlBlock.mute.channelNumber = channelNum;
			status = DSControl( ctx->messageItem, NULL, ctx->streamCBPtr,
								ctx->datatype, kSAudioCtlOpMute, &ctlBlock );
			FAIL_NEG( "DSControl - muting all but channel zero", status );
			}
		}

	/* Set the audio clock to use the selected channel */	
	ctlBlock.clock.channelNumber = ctx->hdr.audioClockChan;
	status = DSControl( ctx->messageItem, NULL, ctx->streamCBPtr,
						ctx->datatype, kSAudioCtlOpSetClockChan, &ctlBlock );
	FAIL_NEG( "DSControl - setting audio clock chan", status );

	return kDSNoErr;

FAILED:
	DismantlePlayer( ctx );
	return status;
	}


/******************************************************************************
 * Routine to free all resources associated with a Player structure. Assumes that all
 * relevant fields are set to ZERO when the struct is initialized.
 ******************************************************************************/
void		DismantlePlayer( PlayerPtr ctx )
	{
	DSConnect(ctx->messageItem, NULL, ctx->streamCBPtr, 0);
	DisposeDataAcq( ctx->acqMsgPort );
	DisposeDataStream( ctx->messageItem, ctx->streamCBPtr );

	FreeMemTrack( ctx->bufferList );

	DeleteMsg( ctx->messageItem );
	DeleteMsg( ctx->endOfStreamMessageItem );
	DeleteMsgPort( ctx->messagePort );
	}

/******************************************************************************
 * Routine to load the default stream header.
 ******************************************************************************/
int32 UseDefaultStreamHeader( DSHeaderChunkPtr headerPtr )
	{
	/* Clear the header chunk */
	memset( headerPtr, 0, sizeof(DSHeaderChunk) );

	headerPtr->headerVersion = DS_STREAM_VERSION;
	headerPtr->streamBlockSize = kDefaultBlockSize;
	headerPtr->streamBuffers = kDefaultNumBuffers;
	headerPtr->streamerDeltaPri = kStreamerDeltaPri;
	headerPtr->dataAcqDeltaPri = kDataAcqDeltaPri;
	headerPtr->numSubsMsgs = kNumSubsMsgs;
	headerPtr->audioClockChan = kAudioClockChan;
	headerPtr->enableAudioChan = kEnableAudioChanMask;
	
	/* Preload 22K, 16 bit mono Squareroot-Delta-Exact shifted decompression instrument
	 * (M2 HW DSP 2:1 decompression). Use this to decompress mono sounds */
	headerPtr->preloadInstList[ 0 ] = SA_22K_16B_M_SQS2;

	/* Preload 44K, 16 bit mono instrument */
	headerPtr->preloadInstList[ 1 ] = SA_44K_16B_M;

	/* Preload 44K, 16 bit stereo instrument */
	headerPtr->preloadInstList[ 2 ] = SA_44K_16B_S;

	/* Preload 22K,16 bit stereo 2:1 cubic XD decompression instrument
	 * (M2 SW DSP 2:1 decompression). Use this to decompress stereo sounds */
	headerPtr->preloadInstList[ 3 ] = SA_22K_16B_S_CBD2;

	/* Set up tags for the default subscribers */
	headerPtr->subscriberList[ 0 ].subscriberType = SNDS_CHUNK_TYPE;
	headerPtr->subscriberList[ 0 ].deltaPriority = kSoundsPriority;
	
	return 0;
	}

