/******************************************************************************
**
**  @(#) playvideostream.c 96/09/20 1.42
**
******************************************************************************/

#ifndef PRINT_LEVEL		/* may be set by a compiler command-line switch */
#define PRINT_LEVEL 1	/* print error msgs */
#endif

#include <string.h>				/* memset */
#include <kernel/debug.h>
#include <kernel/mem.h>			/* for AllocMemTrack, FreeMemTrack   */
#include <kernel/task.h>
#include <audio/audio.h>
#include <graphics/view.h>		/* VIEWTAG_BITMAP */

#include <streaming/datastream.h>
#include <streaming/datastreamlib.h>
#include <streaming/dsstreamdefs.h>
#include <streaming/saudiosubscriber.h>
#include <streaming/mpegaudiosubscriber.h>
#include <streaming/satemplatedefs.h>
#include <streaming/sacontrolmsgs.h>
#include <streaming/mpegvideosubscriber.h>

#include "playvideostream.h"


/************************************/
/* Temporary Tracing code           */
/************************************/
#ifndef TRACE
	#define TRACE			0			/* temporary tracing code */
#endif

#if defined(DEBUG) && TRACE
	#include <kernel/time.h>
	#include <streaming/subscribertraceutils.h>

	static TraceBuffer	traceBuffer;
	#define		ADD_TRACE(event, chan, value, ptr)	\
					AddTrace(&traceBuffer, event, chan, value, ptr)
#else
	#define		ADD_TRACE(event, chan, value, ptr)
#endif

#define kRenderSignalDelay			3


/************************************/
/* Constants and structures         */
/************************************/
#undef kDefaultNumBuffers					/* override the setting in preparestream.h */
#define kDefaultNumBuffers		6			/* suggested number of stream buffers to use */


/************************************/
/* Local utility routine prototypes */
/************************************/
static int32	InitVideoPlayerFromStreamHeader(PlayerPtr ctx, char* streamFileName,
					uint32 bitDepth);
static int32	HandleMPEGFrames(PlayerPtr ctx);


/*******************************************************************************************
 * Play a stream file with MPEG video. The stream may or may not contain audio data.
 * Audio channel selection is based upon info found in the stream header.
 *******************************************************************************************/
Err	PlayVideoStream(ScreenInfo *screenInfoPtr, uint32 bitDepth,
		char *streamFileName, PlayVideoUserFn userFn, void *userContext,
		bool fLoop)
	{
	Err				status;
	int32			playerResult = 0;
	PlayerPtr		ctx;
	Player			playerContext;
	DSRequestMsg	EOSMessage;

	uint32			signalMask;
	uint32			currentSignal;

	bool			keepRunning = TRUE;
	Item			msgItem;

	memset(ctx = &playerContext, 0, sizeof(playerContext));

	/* Remember the screen context pointer that we will use for drawing. */
	ctx->screenInfoPtr = screenInfoPtr;

	/* Initialize the context's fLoop flag from the fLoop arg.
	 * The userFcn can access and modify this flag. */
	ctx->fLoop = fLoop;

	/* Open the specified file, locate the stream header, and initialize
	 * all resources necessary for performing stream playback. */
	status = InitVideoPlayerFromStreamHeader(&playerContext, streamFileName, bitDepth);
	FAIL_NEG("InitVideoPlayerFromStreamHeader", status);

	/* Remember the user function pointer, if any */
	ctx->userFn = userFn;
	ctx->userContext = userContext;

	/* If we were supplied with a user function, we have to initiate a
	 * wake up siginal to make sure it gets called regularly */
	if (userFn)
		{
		status = CreateItem(MKNODEID(AUDIONODE, AUDIO_CUE_NODE), NULL);
		FAIL_NEG("Create Audio userCue Item", status);
		ctx->userCue = status;

		ctx->userCueSignal = GetCueSignal(ctx->userCue);
		}

	/* Register for end of stream notification. Do it before DSStartStream so
	 * we won't miss it in a short or empty stream. */
	status = DSWaitEndOfStream(ctx->endOfStreamMessageItem, &EOSMessage, ctx->streamCBPtr);
	FAIL_NEG("DSWaitEndOfStream", status);

	PRNT(("Prerolling the stream\n"));
	status = DSPreRollStream(ctx->messageItem, NULL, ctx->streamCBPtr, 2);
	FAIL_NEG("DSPreRollStream", status);

	/* Start the stream running */
	PRNT(("Starting the stream\n"));
	status = DSStartStream(ctx->messageItem, NULL, ctx->streamCBPtr, 0);
	FAIL_NEG("DSStartStream", status);

	/* Prime the user cue signal if present */
	if ( ctx->userCue )
		SignalAtTime(ctx->userCue, GetAudioTime() + kUserFunctionPeriod);

	/* Set up the mask for signals we're expecting */
	signalMask =	ctx->frameCompletionPortSignal |		/* Frame Completions */
					ctx->messagePortSignal |				/* End of stream replys */
					ctx->userCueSignal;						/* Periodic user events */

	/* --------------- Main playback loop ---------------
	 * NOTE: A better scheme here would be to also wait for a control port event
	 * signal (rather than an audio Cue to poll the control port) and for a
	 * time-to-display-frame signal (rather than a subroutine that sleeps
	 * until time to display and sleeps until the display has begun). */
	while ( keepRunning )
		{
		currentSignal = WaitSignal(signalMask);

		/* If a user function is defined, then call it.
		 * If it returns a non-zero value, then exit. */
		if ( currentSignal & ctx->userCueSignal )
			if ( ctx->userFn != NULL )
				{
				playerResult = (*ctx->userFn)(ctx);
				if ( playerResult )
					keepRunning = FALSE;
				else
					SignalAtTime(ctx->userCue,
						GetAudioTime() + kUserFunctionPeriod);
				}

		/* Check for a new-frame-to-display signal. */
		if ( currentSignal & ctx->frameCompletionPortSignal )
			{
			status = HandleMPEGFrames(ctx);
			FAIL_NEG("HandleMPEGFrames", status);
			}

		/* Check for the end-of-stream msg reply on ctx->messagePort. */
		if ( currentSignal & ctx->messagePortSignal )
			while ( (status = msgItem = GetMsg(ctx->messagePort)) != 0 )
				{
				FAIL_NEG("PlayVideoStream GetMsg", status);
				if ( msgItem == ctx->endOfStreamMessageItem )
					{
					/* We received the EOS msg reply. Handle these cases:
					 *   STOP chunk			print a message and prepare to resume or quit;
					 *   error EOS			print an error message and exit;
					 *   normal EOS, fLoop	loop back to the start;
					 *   otherwise			exit normally. */
					status = MESSAGE(msgItem)->msg_Result;
					if ( status == kDSSTOPChunk )
						{
						status = DSWaitEndOfStream(ctx->endOfStreamMessageItem,
							&EOSMessage, ctx->streamCBPtr);
						FAIL_NEG("DSWaitEndOfStream", status);

						APRNT(("Reached a stream STOP chunk. "
							"Press >/|| to resume or STOP to exit.\n"));
						}

					else if ( status < 0 )
						{ ERROR_RESULT_STATUS("EOS reply", status); goto FAILED; }

					else if ( ctx->fLoop )
						{
						status = DSWaitEndOfStream(ctx->endOfStreamMessageItem,
							&EOSMessage, ctx->streamCBPtr);
						FAIL_NEG("DSWaitEndOfStream", status);

						status = DSGoMarker(ctx->messageItem, NULL,
							ctx->streamCBPtr, 0,
							GOMARKER_ABSOLUTE | GOMARKER_NO_FLUSH_FLAG);
						FAIL_NEG("DSGoMarker 0", status);
						}

					else
						keepRunning = FALSE;
					}

				else
					PERR(("PlayVideoStream: dequeued an unexpected message from "
						"ctx->messagePort!\n"));
				}	/* while ( GetMsg() returns a Message Item or an error code ) */
		}

	/* Unconditionally stop the stream */
	status = DSStopStream(ctx->messageItem, NULL, ctx->streamCBPtr, SOPT_FLUSH);
	FAIL_NEG("DSStopStream", status);

	PRNT(("Stopped the stream\n"));

#if 0	/* there's no CinePak subscriber in this program */
	/* Flush anything held by the CinePak subscriber */
	if ( ctx->cpakContextPtr != NULL )
		{
		FlushCPakChannel(ctx->cpakContextPtr, ctx->cpakChannelPtr, 0);
		}
#endif

FAILED:

	/* Cleanup; deallocate */
	DismantlePlayer(ctx);

#if defined(DEBUG) && TRACE
	DumpRawTraceBuffer(&traceBuffer, "RenderSignalTrace.txt");
#endif

	return ( status < 0 ) ? status : playerResult;
	}


/*********************************************************************************************
 * Routine to load the default stream header.
 *********************************************************************************************/
static int32 UseDefaultStreamHeader(DSHeaderChunkPtr headerPtr)
	{
	memset(headerPtr, 0, sizeof(*headerPtr));

	/* [TBD] Use memset() as a simpler, faster way to clear out the header */

	headerPtr->headerVersion =			DS_STREAM_VERSION;
	headerPtr->streamBlockSize =		kDefaultBlockSize;
	headerPtr->streamBuffers =			kDefaultNumBuffers;
	headerPtr->streamerDeltaPri =		kStreamerDeltaPri;
	headerPtr->dataAcqDeltaPri =		kDataAcqDeltaPri;
	headerPtr->numSubsMsgs =			kNumSubsMsgs;
	headerPtr->audioClockChan =			kAudioClockChan;
	headerPtr->enableAudioChan =		kEnableAudioChanMask;

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
	headerPtr->subscriberList[ 0 ].subscriberType = MPVD_CHUNK_TYPE;
	headerPtr->subscriberList[ 0 ].deltaPriority = kVideoPriority;
#define ENABLE_AUDIO_SUBSCRIBER		1	/* sometimes useful to disable it during debugging */
#if ENABLE_AUDIO_SUBSCRIBER
	headerPtr->subscriberList[ 1 ].subscriberType = SNDS_CHUNK_TYPE;
	headerPtr->subscriberList[ 1 ].deltaPriority = kSoundsPriority;
#endif

	return 0;
	}


/*******************************************************************************************
 * Routine to get the stream header info, and perform all necessary allocations
 * and initializations necessary for stream playback.
 *******************************************************************************************/
static Err	InitVideoPlayerFromStreamHeader(PlayerPtr ctx, char* streamFileName,
		uint32 bitDepth)
	{
	int32				status;
	long				subscriberIndex;
	long				channelNum;
	int32				i;

	SAudioCtlBlock		ctlBlock;	/* Control blocks for communicating with subscribers */
	MPEGVideoCtlBlock	mpegCtl;

	DSHeaderSubsPtr		subsPtr;

	Boolean				fStreamHasAudio = false;	/* assume no audio subscriber */

	/* Get the stream header loaded */
	status = FindAndLoadStreamHeader(&ctx->hdr, streamFileName);

	/* If there was no stream header then use a default */
	if ( status == kDSHeaderNotFound )
		status = UseDefaultStreamHeader(&ctx->hdr);

	if ( status != 0 )
		return status;


	/* Make sure this playback code is compatible with the version of the
	 * data in the stream file. */
	if ( ctx->hdr.headerVersion != DS_STREAM_VERSION )
		return kDSVersionErr;


	/* Allocate the stream buffers and build the linked list of
	 * buffers that are input to the streamer. */
	ctx->bufferList = CreateBufferList(ctx->hdr.streamBuffers, ctx->hdr.streamBlockSize);
	if ( ctx->bufferList == NULL )
		return kDSNoMemErr;


	/* We need to create a message port and message items
	 * to communicate with the data streamer. */
	status = ctx->messagePort = NewMsgPort(&ctx->messagePortSignal);
	FAIL_NEG("NewMsgPort", status);


	status = ctx->messageItem = CreateMsgItem(ctx->messagePort);
	FAIL_NEG("CreateMsgItem", status);


	status = ctx->endOfStreamMessageItem = CreateMsgItem(ctx->messagePort);
	FAIL_NEG("Create EOS MsgItem", status);


	status = ctx->synchCue = CreateItem(MKNODEID(AUDIONODE, AUDIO_CUE_NODE), NULL);
	FAIL_NEG("Create Audio synchCue Item", status);


	/* Also allocate a message port for recieving frame completion signals */
	status = ctx->frameCompletionPort = NewMsgPort(&ctx->frameCompletionPortSignal);
	FAIL_NEG("NewMsgPort frameCompletionPort", status);


	/* Open a data acquisition thread on the specified file */
	status = ctx->acqMsgPort =
		NewDataAcq(streamFileName, ctx->hdr.dataAcqDeltaPri);
	FAIL_NEG("NewDataAcq", status);


	status = NewDataStream(&ctx->streamCBPtr, 		/* output: stream control block ptr */
					ctx->bufferList, 				/* pointer to buffer list */
					ctx->hdr.streamBlockSize, 		/* size of each buffer */
					ctx->hdr.streamerDeltaPri,		/* streamer thread relative priority */
					ctx->hdr.numSubsMsgs);			/* number of subscriber messages */
	FAIL_NEG("NewDataStream", status);

	/* Connect the stream to its data supplier */
	status = DSConnect(ctx->messageItem, NULL, ctx->streamCBPtr,
					ctx->acqMsgPort);
	FAIL_NEG("DSConnect", status);

	/* Loop through the subscriber descriptor table and initialize all
	 * subscribers specified in the table. */
	for ( subscriberIndex = 0;
			subsPtr = ctx->hdr.subscriberList + subscriberIndex,
			subsPtr->subscriberType != 0;
			subscriberIndex++ )
		{
		switch ( subsPtr->subscriberType )
			{
			case MPVD_CHUNK_TYPE:
				/* Remember the target bit depth of the playback we're starting */
				ctx->bitDepth = bitDepth;

				/* Instantiate an MPEG video subscriber and sign it up for
				 * a DataStreamer subscription (now in one easy step!) */
				status = NewMPEGVideoSubscriber(ctx->streamCBPtr,
							subsPtr->deltaPriority, ctx->messageItem, bitDepth);
				FAIL_NEG("NewMPEGVideoSubscriber", status);
				ctx->mpegVideoSubMsgPort = status;


				/* The MPEG Subscriber requires a message port to send information
				 * about decompressed frames to.  This enables the application to
				 * take care of all display activities. */
				mpegCtl.SetClientPort.clientPort = ctx->frameCompletionPort;
				status = DSControl(ctx->messageItem,
							NULL,
							ctx->streamCBPtr,
							MPVD_CHUNK_TYPE,
							kMPEGVideoCtlOpSetClientPort,
							&mpegCtl);
				FAIL_NEG("DSControl kMPEGVideoCtlOpSetClientPort", status);


				/* Now send the MPEGVideoSubscriber a pointer to the frame buffer to
				 * decompress into.  Note that we're only sending a chunk of memory, so we
				 * could theoretically provide a pointer to any kind of DMA-able memory. */
#if ( cntFrameBuffers > MPVD_SUBS_MAX_FRAMEBUFFERS )
	#error "InitVideoPlayerFromStreamHeader error: cntFrameBuffers > MPVD_SUBS_MAX_FRAMEBUFFERS"
#endif
				mpegCtl.SetFrameBuffers.frameBuffers.numFrameBuffers = cntFrameBuffers;
				mpegCtl.SetFrameBuffers.frameBuffers.frameBufferSize = (bitDepth == 16) ?
					NTSC_FRAME_WIDTH * NTSC_FRAME_HEIGHT * 2 :
					NTSC_FRAME_WIDTH * NTSC_FRAME_HEIGHT * 4;

				mpegCtl.SetFrameBuffers.frameBuffers.bitDepth = bitDepth;

				for ( i = 0; i < cntFrameBuffers; i++ )
					mpegCtl.SetFrameBuffers.frameBuffers.frameBuffer[i] =
						ctx->screenInfoPtr->bitmap[i]->bm_Buffer;

				status = DSControl(ctx->messageItem,
							NULL,
							ctx->streamCBPtr,
							MPVD_CHUNK_TYPE,
							kMPEGVideoCtlOpSetFrameBuffers,
							&mpegCtl);
				FAIL_NEG("DSControl kMPEGVideoCtlOpSetFrameBuffers", status);
				break;

			case SNDS_CHUNK_TYPE:
				status = NewSAudioSubscriber(ctx->streamCBPtr, subsPtr->deltaPriority,
							ctx->messageItem);
				FAIL_NEG("NewSAudioSubscriber", status);
				ctx->datatype = SNDS_CHUNK_TYPE;
				fStreamHasAudio = true;

				break;

			case MPAU_CHUNK_TYPE:
				status = NewMPEGAudioSubscriber( ctx->streamCBPtr,
							subsPtr->deltaPriority,
							ctx->messageItem, NUMBER_MPEG_AUDIO_BUFFER );
				FAIL_NEG( "NewMPEGAudioSubscriber", status );
				ctx->datatype = MPAU_CHUNK_TYPE;

				break;

			case CTRL_CHUNK_TYPE:
				/* The ControlSubscriber is obsolete */
				break;

			default:
				PERR(("InitVideoPlayerFromStreamHeader - unknown subscriber type '%.4s'\n",
					(char*)&subsPtr->subscriberType));
				status = kDSInvalidTypeErr;
				goto FAILED;
			}
		}

	/* If the stream has audio, then do some additional initializations. */
	if ( fStreamHasAudio )
		{
		/* Preload audio instrument templates, if any are specified */
		if ( ctx->hdr.preloadInstList != 0 )
			{
			ctlBlock.loadTemplates.tagListPtr = ctx->hdr.preloadInstList;

			status = DSControl(ctx->messageItem, NULL, ctx->streamCBPtr, ctx->datatype,
								 kSAudioCtlOpLoadTemplates, &ctlBlock);
			FAIL_NEG("DSControl", status);
			}

		/* Enable any audio channels whose enable bit is set.
		 * NOTE: Channel zero is enabled by default, so we don't check it. */
		for ( channelNum = 1; channelNum < 32; channelNum++ )
			{
			/* If the bit corresponding to the channel number is set,
			 * then tell the audio subscriber to enable that channel. */
			if ( ctx->hdr.enableAudioChan & (1L << channelNum) )
				{
				status = DSSetChannel(ctx->messageItem, NULL, ctx->streamCBPtr,
					ctx->datatype, channelNum, CHAN_ENABLED, CHAN_ENABLED);
				FAIL_NEG("DSSetChannel", status);
				}
			}

		/* Set the audio clock to use the selected channel */
		ctlBlock.clock.channelNumber = ctx->hdr.audioClockChan;
		status = DSControl(ctx->messageItem, NULL, ctx->streamCBPtr, ctx->datatype,
								 kSAudioCtlOpSetClockChan, &ctlBlock);
		FAIL_NEG("DSControl - setting audio clock chan", status);
		}

	return kDSNoErr;

FAILED:
	DismantlePlayer(ctx);
	return status;
	}


/*******************************************************************************************
 * Routine to free all resources associated with a Player structure. Assumes that all
 * relevant fields are set to ZERO when the struct is initialized.
 *
 *	NOTE:	THE ORDER OF THE FOLLOWING DISPOSALS IS IMPORTANT. DO NOT CHANGE UNLESS
 *			YOU KNOW WHAT YOU ARE DOING.
 *
 *******************************************************************************************/
void	DismantlePlayer(PlayerPtr ctx)
	{
	/* If we have a frame hanging out, return it to the subscriber before
	 * we turn off the subscribers. */
	if ( ctx->lastFrameMsg )
		ReplyMsg(ctx->lastFrameMsg, 0, ctx->lastMPEGBufferPtr,
			sizeof(MPEGBuffer));

	DSConnect(ctx->messageItem, NULL, ctx->streamCBPtr, 0);
	DisposeDataAcq(ctx->acqMsgPort);
	DisposeDataStream(ctx->messageItem, ctx->streamCBPtr);

	FreeMemTrack(ctx->bufferList);
	DeleteMsg(ctx->messageItem);
	DeleteMsg(ctx->endOfStreamMessageItem);
	DeleteMsgPort(ctx->messagePort);
	DeleteMsgPort(ctx->frameCompletionPort);
	DeleteItem(ctx->synchCue);

	memset(ctx, 0, sizeof(*ctx));		/* ensures double-dismantle is safe */
	}


/* To display a picture in sync with the audio, we'll wait (delay #1) until the
 * audio clock says it's time to display the picture, then schedule the frame-flip
 * to occur at the next video field, which incurs delay #2. We should subtract
 * an offset from delay #1 so the flip will occur at the right time even if
 * delay #1 ends when video retrace happens +/- a little bit, and also to
 * subtract the effect of delay #2 on A/V sync.
 *
 * How much to subtract?
 *   * We could subtract the average video flip delay (0.5 video fields) to
 *     make A/V sync average at top dead center.
 *   * Or we could subtract 1 video field time so delay #1 never makes the
 *     picture late.
 *   * Or, since people can notice when video is late by > 1 frame or early
 *     by > 2 frames, midway between tolerance limits would be early by
 *     0.5 frames = 1 field on avg, suggesting an offset of 1.5 video fields.
 *   * To display 60 field/second interlaced material by flipping between 30
 *     interlaced pictures/second, we'll need to ask the Graphics Folio
 *     to flip at the next TOP (or is it BOTTOM?) field. This suggests an
 *     offset of 2 video fields to prevent being late.
 * 
 * The effective delay offset is limited by how early the pictures arrive in
 * the stream and when the previous frame buffers get refilled. */
#define DELAY_OFFSET	(1 * 240 / 60)

/*******************************************************************************************
 * Handle an MPEG frame completion message: schedule its display time and return the previous
 * frame buffer to the streamer for refilling.
 *
 * [TBD] This is currently implemented by waiting here, in this subroutine. This delays the
 *   app's response to other signals like control port signals. The player's main loop should
 *   instead wait on these two additional signals (time to display a buffer, time to return a
 *   buffer for refilling) and do the work then.
 * [TBD] When we're stopped--at least at EOS--don't return buffers for refilling.
 *******************************************************************************************/
int32		HandleMPEGFrames(PlayerPtr ctx)
	{
	int32					status = kDSNoErr;
	Item					theMsg;
	Message					*msgPtr;
	ScreenInfo				*const screenInfo = ctx->screenInfoPtr;
#if defined(DEBUG) && TRACE
	TimerTicks				startTimeTicks, endTimeTicks, timeTicks;
	static TimerTicks		prevEndTimeTicks = {0, 0};
	TimeVal					waitTimeVal, interPictureTimeVal;
#endif

	/* Get any pending frames off the frame completion port and display them */
	while ( (theMsg = GetMsg(ctx->frameCompletionPort)) != 0 )
		{
		if ( theMsg > 0 )
			{
			msgPtr = (Message *)LookupItem(theMsg);
				/* [TBD] Should probably error check item lookup */

			if ( msgPtr != NULL )
				{
				const MPEGBufferPtr		theBuffer = (MPEGBufferPtr)msgPtr->msg_DataPtr;
				const int32				screenIndex = (int32)theBuffer->userData;

				/* wait until the frame is ready to be displayed, then throw it
				 * up on the screen */
				SleepUntilTime(ctx->synchCue, theBuffer->displayTime - DELAY_OFFSET);

#if 0
				/* Wait for displaySignal IF you want to ensure the previous
				 * frame got displayed. Otherwise, skipping a picture when
				 * we're late can help us catch up. */
				WaitSignal(screenInfo->displaySignal);
#endif

				status = ModifyGraphicsItemVA(screenInfo->viewItem,
							VIEWTAG_BITMAP, screenInfo->bitmapItem[screenIndex],
#if NTSC_FRAME_HEIGHT == 480
	/* Define FIELD_TO_DISPLAY_FIRST in the "make" file iff each MPEG frame in
	 * your source material really contains two interlaced fields, and set it
	 * even if the top scan line should be displayed first, or odd if the
	 * second scan line is part of the field to display first.
	 * Suggestion: 0 for interlaced, undefined for progressive source. */
	#ifdef FIELD_TO_DISPLAY_FIRST
							VIEWTAG_FIELDSTALL_BITMAPLINE, FIELD_TO_DISPLAY_FIRST,
	#endif
#endif
							TAG_END);
				FAIL_NEG("HandleMPEGFrames ModifyGraphicsItemVA", status);

#ifdef STARTUP_WORKAROUND
				if ( !screenInfo->displayedFirstFrameYet )
					{
					status = AddViewToViewList(screenInfo->viewItem, 0);
					FAIL_NEG("HandleMPEGFrames: AddViewToViewList", status);
					screenInfo->displayedFirstFrameYet = TRUE;
					}
#endif

#if defined(DEBUG) && TRACE
				/* Time the wait-for-render-signal, in microseconds. */
				SampleSystemTimeTT(&startTimeTicks);
#endif

				/* wait until the previous buffer is no longer being displayed */
				WaitSignal(screenInfo->renderSignal);

#if defined(DEBUG) && TRACE
				/* Trace the wait-for-render-signal delay time and the time since
				 * we finished waiting for the previous render-signal. */
				SampleSystemTimeTT(&endTimeTicks);
				
				SubTimerTicks(&startTimeTicks, &endTimeTicks, &timeTicks);
				ConvertTimerTicksToTimeVal(&timeTicks, &waitTimeVal);
				
				SubTimerTicks(&prevEndTimeTicks, &endTimeTicks, &timeTicks);
				ConvertTimerTicksToTimeVal(&timeTicks, &interPictureTimeVal);
				
				ADD_TRACE(kRenderSignalDelay, interPictureTimeVal.tv_usec,
					waitTimeVal.tv_usec, (void *)screenIndex);
				prevEndTimeTicks = endTimeTicks;
#endif

				/* Now that we're done with it, return the last frame to the
				 * subscriber to be recycled. */
				if (ctx->lastFrameMsg != 0)
					{
					status = ReplyMsg(ctx->lastFrameMsg, 0, ctx->lastMPEGBufferPtr,
								sizeof(MPEGBuffer));
					FAIL_NEG("HandleMPEGFrames ReplyMsg", status);
					}

				/* Remember this frame so that we can reply to it when the next
				 * frame is ready to display. */
				ctx->lastFrameMsg		= theMsg;
				ctx->lastMPEGBufferPtr	= theBuffer;
				}
			}
		}

FAILED:
	return status;
	}
