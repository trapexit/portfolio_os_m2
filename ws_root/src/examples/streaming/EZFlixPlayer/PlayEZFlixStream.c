/*******************************************************************************
**
**  @(#) PlayEZFlixStream.c 96/05/02 1.23
**
**	high level EZFlix stream playback functions
**
*******************************************************************************/

#ifndef PRINT_LEVEL		/* may be set by a compiler command-line switch */
#define PRINT_LEVEL 1	/* print error msgs */
#endif

#include <string.h>		/* memset */
#include <kernel/debug.h>
#include <kernel/item.h>
#include <audio/audio.h>
#include <graphics/view.h>
#include <kernel/mem.h>
#include <streaming/datastream.h>
#include <streaming/datastreamlib.h>
#include <streaming/saudiosubscriber.h>
#include <streaming/satemplatedefs.h>
#include <streaming/sacontrolmsgs.h>
#include <streaming/dsstreamdefs.h>
#include <streaming/ezflixsubscriber.h>

#include "PlayEZFlixStream.h"



/************************************/
/* Local utility routine prototypes */
/************************************/
static int32	InitPlayerFromStreamHeader( PlayerPtr ctx, char* streamFileName );
static int32	UseDefaultStreamHeader( DSHeaderChunkPtr headerPtr );
void			DismantlePlayer( PlayerPtr ctx );


/*********************************************************************************************
 * Routine to load the default stream header.
 *********************************************************************************************/
static 	int32 UseDefaultStreamHeader( DSHeaderChunkPtr headerPtr )
	{
	memset(headerPtr, 0, sizeof(*headerPtr));
	
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

    /* Preload 22K,16 bit stereo 2:1 cubic XD decompression instrument
     * (M2 SW DSP 2:1 decompression). Use this to decompress stereo sounds */
	headerPtr->preloadInstList[ 1 ] = SA_22K_16B_S_CBD2;

	/* Set up tags for the default subscribers */
	headerPtr->subscriberList[ 0 ].subscriberType = SNDS_CHUNK_TYPE;
	headerPtr->subscriberList[ 0 ].deltaPriority = kSoundsPriority;
	headerPtr->subscriberList[ 1 ].subscriberType = EZFLIX_CHUNK_TYPE;
	headerPtr->subscriberList[ 1 ].deltaPriority = kVideoPriority;
	
	return 0;
	}


/*******************************************************************************************
 * Routine to play an EZFlix stream file. Stream may or may not contain audio data.
 * Audio channel selection is based upon info found in the stream header.
 *******************************************************************************************/
int32	PlayEZFlixStream(ScreenInfo *screenInfoPtr, char *streamFileName, 
				PlayEZFlixUserFn userFn, void *userContext)
	{
	Err				status;
	int32			playerResult = 0;
	PlayerPtr		ctx;
	Player			playerContext;
	DSRequestMsg	EOSMessage;
	Bitmap*			bitmap;
	uint32			time;
	uint32			frameCount;
	int32			drawingScreen;

	/* Open the specified file, locate the stream header, and initialize
	 * all resources necessary for performing stream playback.
	 */
	status = InitPlayerFromStreamHeader( &playerContext, streamFileName );
	FAIL_NEG("InitPlayerFromStreamHeader", status);

	ctx = &playerContext;

	/* Remember the screen context pointer that we will use
	 * for drawing purposes.
	 */
	ctx->screenInfoPtr = screenInfoPtr;

	/* Remember the user function pointer, if any */
	ctx->userFn = userFn;
	ctx->userContext = userContext;

	/* Make sure the first screen is displayed, and we're drawing into
	 * the second screen.
	 */
	status = ModifyGraphicsItemVA(ctx->screenInfoPtr->viewItem,
							VIEWTAG_BITMAP,
								ctx->screenInfoPtr->bitmapItem[0],
							TAG_END);
	drawingScreen = 1;
	FAIL_NEG("ModifyGraphicsItemVA", status);

	/* Register for end of stream notification. Do it before DSStartStream so
	 * we won't miss it in a short or empty stream. */
	status = DSWaitEndOfStream( ctx->endOfStreamMessageItem, &EOSMessage, ctx->streamCBPtr );
	FAIL_NEG("DSWaitEndOfStream", status);

	/* Prefill buffers with data */
	status = DSPreRollStream(ctx->messageItem, NULL, ctx->streamCBPtr, 2);
	FAIL_NEG("DSPreRollStream", status);

	/* Start the stream running */
	status = DSStartStream( ctx->messageItem, NULL, ctx->streamCBPtr, 0 );
	FAIL_NEG("DSStartStream", status);

	/* Get a pointer to the bitmap we will draw into */
	bitmap = ctx->screenInfoPtr->bitmap[ drawingScreen ];

	time = GetAudioTime();
	frameCount = 0;

#	if USE_TASK_TIMING
	TaskTimeStamps(START);
#	endif

	/* --------------- Main playback loop ---------------
	 * NOTE: A better scheme here would be to WaitSignal() each time around the
	 * loop and process all received signals--audio cues indicating time to
	 * check for user input (or better yet, control port event signals), frame
	 * signals indicating time to display a frame, and message port signals
	 * indicating time to check for the end-of-stream reply.
	 * This current implementation polls everything as often as it can, soaking
	 * up all idle CPU cycles and using extra bus bandwidth. */
	while ( TRUE )
	{
		/* If a user function is defined, then call it. If the user function
		 * returns a non-zero value, then exit. */
		if ( ctx->userFn )
		{
			playerResult = (*ctx->userFn)(ctx);
			if ( playerResult )
				break;
		}

		/* Exit if we received the end-of-stream msg reply.
		 * status < 0 is an error. status > 0 is our EOS-reply msg Item. */
		{
			Item		msgItem;
	
			status = msgItem = GetMsg(ctx->messagePort);
			FAIL_NEG("PlayEZFlixStream GetMsg", status);
			if ( msgItem > 0 )
			{
				time = GetAudioTime() - time;
				TOUCH(time);	/* avert compiler warning of variable set but unused */
				status = MESSAGE(msgItem)->msg_Result;
				FAIL_NEG("EOS reply", status);
				break;
			}
		}

		/* In case there's no EZFlix in this stream, skip
		 * trying to draw something that doesn't exist.
		 */
		if ( ctx->contextPtr != 0 )
		{
			/*
			 * Draw next EZFlix frame directly into the screen buffer.
			 */
			if (DrawEZFlixToBuffer( ctx->contextPtr, ctx->channelPtr, bitmap )) 
			{
				/* Display the screen we just drew, and switch to drawing into
				 * the next screen.
				 * [TBD] CAVEAT: We should wait for the render signal after
				 * ModifyGraphicsItemVA()--which indicates that the previous
				 * frame buffer is no longer visible--before drawing into it.
				 * We tend to get away with it here since DrawEZFlixToBuffer()
				 * won't draw into the frame buffer until it's nearly time to
				 * display a new frame. */
				status = ModifyGraphicsItemVA(ctx->screenInfoPtr->viewItem,
							VIEWTAG_BITMAP,
								ctx->screenInfoPtr->bitmapItem[drawingScreen],
							TAG_END);
				FAIL_NEG("ModifyGraphicsItemVA", status);

				if (++drawingScreen >= cntFrameBuffers)
					drawingScreen = 0;
				bitmap = ctx->screenInfoPtr->bitmap[ drawingScreen ];

				/* TBD
				 * Flash-write black into the next screen.  The size of frames coming
				 * from the stream can change at any time, so we want to be sure we always
				 * draw over a black background to avoid garbage on the edges when a smaller
				 * frame follows a larger frame.
				 */

				++frameCount;
			}
		}
	}

	/* Unconditionally stop the stream */
	status = DSStopStream( ctx->messageItem, NULL, ctx->streamCBPtr, SOPT_FLUSH );
	FAIL_NEG("DSStopStream", status);

	PRNT(("\nStopped the stream\n"));

#	if USE_TASK_TIMING
	TaskTimeStamps(STOP);
	PrintTimeStampDeltas((outputFP) NULL);
#	endif

	/* Don't flush if EZFlix data was not present */
	if ( ctx->contextPtr != 0 )
	{
		/* Flush anything held by the EZFlix subscriber */
		FlushEZFlixChannel( ctx->contextPtr, ctx->channelPtr );
	}

	/* Get rid of stuff we allocated */
	DismantlePlayer( ctx );
	
FAILED:
	return ( status < 0 ) ? status : playerResult;
	}


/*******************************************************************************************
 * Routine to get the stream header info, and perform all necessary allocations
 * and initializations necessary for stream playback.
 *******************************************************************************************/
static int32	InitPlayerFromStreamHeader( PlayerPtr ctx, char* streamFileName )
	{
	int32			status;
	int32			subscriberIndex;
	int32			channelNum;
	SAudioCtlBlock	ctlBlock;
	DSHeaderSubsPtr	subsPtr;

	/* Assume no audio */
	ctx->streamHasAudio = false;

	/* Initialize fields to zero so that cleanup can
	 * tell what has been allocated.
	 */
	ctx->bufferList					= 0;
	ctx->streamCBPtr				= 0;
	ctx->acqMsgPort					= 0;
	ctx->messageItem				= 0;
	ctx->endOfStreamMessageItem 	= 0;
	ctx->messagePort				= 0;
	ctx->channelPtr					= 0;
	ctx->contextPtr					= 0;
	
	
	/* Get the stream header loaded */
	status = FindAndLoadStreamHeader( &ctx->hdr, streamFileName );
	
	/* If there was no stream header then use a default */
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
	ctx->bufferList = CreateBufferList( ctx->hdr.streamBuffers, ctx->hdr.streamBlockSize );
	if ( ctx->bufferList == NULL )
		return kDSNoMemErr;

	/* We need to create a message port and one message item
	 * to communicate with the data streamer.
	 */
	ctx->messagePort = NewMsgPort( NULL );
	if ( ctx->messagePort < 0 )
		goto FAILED;

	ctx->messageItem = CreateMsgItem( ctx->messagePort );
	if ( ctx->messageItem < 0 )
		goto FAILED;

	ctx->endOfStreamMessageItem = CreateMsgItem( ctx->messagePort );
	if ( ctx->endOfStreamMessageItem < 0 )
		goto FAILED;


	/* Initialize data acquisition for the specified file */
	ctx->acqMsgPort = status =
		NewDataAcq(streamFileName, ctx->hdr.dataAcqDeltaPri);
	FAIL_NEG("NewDataAcq", status);

	status = NewDataStream( &ctx->streamCBPtr, 		/* output: stream control block ptr */
					ctx->bufferList, 				/* pointer to buffer list */
					ctx->hdr.streamBlockSize, 		/* size of each buffer */
					ctx->hdr.streamerDeltaPri,		/* streamer thread relative priority */
					ctx->hdr.numSubsMsgs );			/* number of subscriber messages */
	FAIL_NEG("NewDataStream", status);

	/* Connect the stream to its data supplier */
	status = DSConnect(ctx->messageItem, NULL, ctx->streamCBPtr,
				ctx->acqMsgPort);
	FAIL_NEG("DSConnect", status);

	/* Loop through the subscriber descriptor table and initialize all
	 * subscribers specified in the table.
	 */
	for ( subscriberIndex = 0; ctx->hdr.subscriberList[ subscriberIndex ].subscriberType != 0;
			subscriberIndex++ )
	{
		subsPtr = ctx->hdr.subscriberList + subscriberIndex;
		
		switch ( subsPtr->subscriberType )
		{
			case EZFLIX_CHUNK_TYPE:
				status = NewEZFlixSubscriber( ctx->streamCBPtr, subsPtr->deltaPriority, 
												ctx->messageItem, 1, &ctx->contextPtr);
				FAIL_NEG( "NewEZFlixSubscriber", status );

				status = InitEZFlixStreamState(ctx->streamCBPtr,/* Need this for DSGetClock in sub */
							ctx->contextPtr,					/* The subscriber's context */
							&ctx->channelPtr,					/* The channel's context */
							0,									/* The channel number */
							true );								/* true = flush on synch msg from DS */
				FAIL_NEG( "InitEZFlixStreamState", status );
				break;

			case SNDS_CHUNK_TYPE:
				status = NewSAudioSubscriber( ctx->streamCBPtr, subsPtr->deltaPriority,
							ctx->messageItem );
				FAIL_NEG( "NewSAudioSubscriber", status );

				ctx->streamHasAudio	= true;
				break;

			case CTRL_CHUNK_TYPE:
				/* The ControlSubscriber is obsolete */
				break;

			default:
				PERR(("InitPlayerFromStreamHeader: unknown subscriber in "
					"stream header '%.4s'\n", (char*)&subsPtr->subscriberType));
				status = kDSSubNotFoundErr;
				goto FAILED;
			}
		}

	/* If the stream has audio, then do some additional initializations.
	 */
	if ( ctx->streamHasAudio )
	{
		/* Preload audio instrument templates, if any are specified
		 */
		if ( ctx->hdr.preloadInstList != 0 )
		{
			ctlBlock.loadTemplates.tagListPtr = ctx->hdr.preloadInstList;
		
			status = DSControl( ctx->messageItem, NULL, ctx->streamCBPtr, SNDS_CHUNK_TYPE,
								 kSAudioCtlOpLoadTemplates, &ctlBlock );
			FAIL_NEG( "DSControl", status );
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
					SNDS_CHUNK_TYPE, channelNum, CHAN_ENABLED, CHAN_ENABLED);
				FAIL_NEG( "DSSetChannel", status );
			}
		}
	
		/* Set the audio clock to use the selected channel */	
		ctlBlock.clock.channelNumber = ctx->hdr.audioClockChan;
		status = DSControl( ctx->messageItem, NULL, ctx->streamCBPtr, SNDS_CHUNK_TYPE,
								 kSAudioCtlOpSetClockChan, &ctlBlock );
		FAIL_NEG( "DSControl - setting audio clock chan", status );
	}

	return kDSNoErr;

FAILED:
	DismantlePlayer( ctx );
	return status;
}


/*******************************************************************************
 * Routine to free all resources associated with a Player structure. Assumes the
 * relevant fields are set to ZERO when the struct is initialized.
 *
 *	NOTE:	THE ORDER OF THE FOLLOWING DISPOSALS IS IMPORTANT. DO NOT CHANGE
 *			UNLESS YOU KNOW WHAT YOU ARE DOING.
 *
 *******************************************************************************/
void		DismantlePlayer( PlayerPtr ctx )
	{
	DestroyEZFlixStreamState( ctx->contextPtr, ctx->channelPtr );

	DSConnect(ctx->messageItem, NULL, ctx->streamCBPtr, 0);
	DisposeDataAcq(ctx->acqMsgPort);
	DisposeDataStream(ctx->messageItem, ctx->streamCBPtr);

	FreeMemTrack(ctx->bufferList);
	DeleteMsg(ctx->messageItem);
	DeleteMsg(ctx->endOfStreamMessageItem);
	DeleteMsgPort(ctx->messagePort);
	}
