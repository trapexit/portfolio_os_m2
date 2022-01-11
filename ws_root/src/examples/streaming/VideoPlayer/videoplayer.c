/******************************************************************************
**
**  @(#) videoplayer.c 96/08/30 1.24
**
******************************************************************************/

#ifndef PRINT_LEVEL		/* may be set by a compiler command-line switch */
#define PRINT_LEVEL 1	/* print error msgs */
#endif

#include <stdio.h>
#include <string.h>

#include <kernel/debug.h>
#include <kernel/operror.h>
#include <kernel/mem.h>
#include <kernel/task.h>
#include <graphics/graphics.h>
#include <graphics/view.h>
#include <graphics/bitmap.h>
#include <audio/audio.h>			/* OpenAudioFolio */

#include <streaming/mpegvideosubscriber.h>

#include "joypad.h"
#include "playvideostream.h"


/******************************/
/* Library and system globals */
/******************************/

static ScreenInfo screenInfo;

#define DEFAULT_BITMAP_DEPTH		24

#define DEFAULT_STREAM_FILENAME		"video1.stream"


/*********************************************************************************************
 * Perform one-time graphics initializations:
 * Open the Graphics Folio, create two Bitmap Items (for double-buffering), allocate their
 * bitmap buffers (with appropriate alignment and filled with zeros), create a View Item, and
 * add the View Item to the system View List.
 *
 * [TBD] We're using MEMTYPE_STARTPAGE here to get an alignment workable to the FMV decoder
 * hardware. Note that, as long as we DON'T ask for MEMTYPE_TRACKSIZE, any allocation will
 * happen to be adequately aligned with the current hardware. We need a better solution requiring
 * participation of the FMV driver and the OS memory allocator.
 *********************************************************************************************/
static Err InitGraphics(uint32 bitDepth)
	{
	int32		i;
	Err			err;
	Bitmap		*bm;

	memset(&screenInfo, 0, sizeof(screenInfo));

	if ( (err = OpenGraphicsFolio()) < 0 )
		return err;

	for ( i = 0; i < cntFrameBuffers; ++i )
		{
		screenInfo.bitmapItem[i] = err = CreateItemVA(
			MKNODEID(NST_GRAPHICS, GFX_BITMAP_NODE),
			BMTAG_WIDTH,		NTSC_FRAME_WIDTH,
			BMTAG_HEIGHT,		NTSC_FRAME_HEIGHT,
			BMTAG_TYPE,			(bitDepth == 16) ? BMTYPE_16 : BMTYPE_32,
			BMTAG_DISPLAYABLE,	TRUE,
			BMTAG_MPEGABLE,		TRUE,
		/*	BMTAG_BUMPDIMS,		TRUE,	*/	/* [TBD] This asks the Graphics
		 * Folio to bump the width and height to satisfy all hardware
		 * requirements. Before we can ask for BUMPDIMS, we have to use the
		 * (bumped) bm_Width and bm_Height fields rather than using the
		 * requested width and height anywhere else in the code. */
			TAG_END);
		FAIL_NEG("VideoPlayer.InitGraphics: Create a Bitmap Item", err);
		screenInfo.bitmap[i] = bm = LookupItem(screenInfo.bitmapItem[i]);

		screenInfo.allocatedBuffer[i] = AllocMemMasked(
			bm->bm_BufferSize,
			bm->bm_BufMemType | MEMTYPE_FILL,
			bm->bm_BufMemCareBits,
			bm->bm_BufMemStateBits);
		err = ER_NoMem;
		FAIL_NIL("VideoPlayer.InitGraphics: Alloc a bitmap buffer",
			screenInfo.allocatedBuffer[i]);

		err = ModifyGraphicsItemVA(screenInfo.bitmapItem[i],
			BMTAG_BUFFER,	screenInfo.allocatedBuffer[i],
			TAG_END);
		FAIL_NEG("VideoPlayer.InitGraphics: Install a bitmap buffer", err);
		}

	screenInfo.renderSignal = AllocSignal(0);
	screenInfo.displaySignal = AllocSignal(0);
	err = (screenInfo.renderSignal == 0  ||
		   screenInfo.displaySignal == 0) ? kDSNoSignalErr : 0;
	FAIL_NEG("VideoPlayer: Alloc render/display signal", err);

	screenInfo.viewItem = err = CreateItemVA(MKNODEID(NST_GRAPHICS, GFX_VIEW_NODE),
				VIEWTAG_PIXELWIDTH,		NTSC_FRAME_WIDTH,
				VIEWTAG_BITMAP,			screenInfo.bitmapItem[1],
				VIEWTAG_VIEWTYPE,       bitDepth == 16 ?
					(NTSC_FRAME_HEIGHT == 480 ? VIEWTYPE_16_640_LACE : VIEWTYPE_16) :
					(NTSC_FRAME_HEIGHT == 480 ? VIEWTYPE_32_640_LACE : VIEWTYPE_32),
				TAG_END);
	FAIL_NEG("VideoPlayer.InitGraphics: Create a View Item", err);

#ifndef STARTUP_WORKAROUND
	err = AddViewToViewList(screenInfo.viewItem, 0);
	FAIL_NEG("VideoPlayer.InitGraphics: AddViewToViewList", err);
	screenInfo.displayedFirstFrameYet = TRUE;
#endif

	err = ModifyGraphicsItemVA(screenInfo.viewItem,
								VIEWTAG_RENDERSIGNAL,	screenInfo.renderSignal,
								VIEWTAG_DISPLAYSIGNAL,	screenInfo.displaySignal,
								VIEWTAG_BESILENT,		TRUE,
								TAG_END);
	FAIL_NEG("VideoPlayer.InitGraphics: ModifyGraphicsItem to add signals", err);

	/*
	 * Because of the way the main loop is written, we need to "prime the
	 * pump" so to speak, or it will block on a display signal that is not
	 * forthcoming.  So we send ourselves a dummy display signal so that it
	 * will be consumed just before our next call to ModifyGraphicsItem().
	 */
	SendSignal(CURRENTTASKITEM, screenInfo.displaySignal);

	return 0;

FAILED:
	return err;		/* [TBD] cleanup? */
	}


/*********************************************************************************************/
static Boolean StartUp(uint32 bitDepth)
	{
	if ( OpenAudioFolio() < 0 )
		return false;

	if ( InitGraphics(bitDepth) < 0 )
		return false;

	return true;
	}


/*********************************************************************************************
 * This function is called each time through the player's main loop and can be used to
 * implement any user interface actions necessary. Returning a zero causes playback to
 * continue. Any other value causes playback to cease and immediately return from
 * the PlayVideoStream() function.
 *********************************************************************************************/
int32 myUserFunction(PlayerPtr ctx)
	{
	int32			status = kDSNoErr;
	JoyPadStatePtr	jp;

	/* Our only additional context is a pointer to the joypad state. */
	jp = (JoyPadStatePtr) ctx->userContext;

	/* Read control pad #1. */
	GetJoyPad(jp, 1);


	if ( jp->xBtn )
		/* "X" aka Stop button ==> Return 1 to stop the stream */
		return 1;

	else if ( jp->upArrow )
		{
		/* Jump forward to the next marker in the stream. */
		status = DSGoMarker(ctx->messageItem, NULL, ctx->streamCBPtr, 1,
							GOMARKER_FORWARD);
		if ( status == kDSBranchNotDefined )
			{
			PERR(("Can't skip forward: The stream needs a marker table.\n"));
			status = kDSNoErr;
			}
		else if ( status == kDSRangeErr )
			{
			/* Skip forward range error. Beyond end? If we're looping, jump to
			 * the start of the next loop. */
			if ( ctx->fLoop )
				status = DSGoMarker(ctx->messageItem, NULL, ctx->streamCBPtr, 0,
					GOMARKER_ABSOLUTE);
			else
				{
				PERR(("Skip forward range error. Beyond end?\n"));
				status = kDSNoErr;
				}
			}
		FAIL_NEG("DSGoMarker fwd", status);
		}

	else if ( jp->downArrow )
		{
		/* Jump backward to the previous marker in the stream. */
		status = DSGoMarker(ctx->messageItem, NULL, ctx->streamCBPtr, 1,
							GOMARKER_BACKWARD);
		if ( status == kDSBranchNotDefined )
			{
			PERR(("Can't skip backward: The stream needs a marker table.\n"));
			status = kDSNoErr;
			}
		else if ( status == kDSRangeErr )
			/* Skip backward range error. Beyond start? Jump back to start. */
			status = DSGoMarker(ctx->messageItem, NULL, ctx->streamCBPtr, 0,
				GOMARKER_ABSOLUTE);
		FAIL_NEG("DSGoMarker bwd", status);
		}

	else if ( jp->leftArrow )
		{
		/* Jump to the beginning */
		/* For streamer testing: unshifted-leftArrow ==> ASAP flush branch (the
		 * normal way to branch in response to a user button push),
		 * shifted arrow => butt-joint branch. */
		status = DSGoMarker(ctx->messageItem, NULL, ctx->streamCBPtr, 0,
			( jp->rightShift | jp->leftShift ) ?
				GOMARKER_ABSOLUTE | GOMARKER_NO_FLUSH_FLAG : GOMARKER_ABSOLUTE);
		FAIL_NEG("DSGoMarker 0", status);
		}

	else if ( jp->startBtn )
		{
		/* Pause / Unpause the stream */
		if ( DSIsRunning(ctx->streamCBPtr) )
			{
			/* Note: Stop without SOPT_FLUSH so we can later resume. */
			status = DSStopStream(ctx->messageItem, NULL, ctx->streamCBPtr, SOPT_NOFLUSH);
			FAIL_NEG("DSStopStream", status);
			}
		else
			{
			status = DSStartStream(ctx->messageItem, NULL, ctx->streamCBPtr, SOPT_NOFLUSH);
			FAIL_NEG("DSStartStream", status);
			}
		}

FAILED:
	return status;
	}


/*********************************************************************************************
 * Print command-line usage information.
 *********************************************************************************************/
static void Usage(void)
	{
	APRNT(("VideoPlayer [-16 | -24] [-l | -loop] [-?] [<stream file>]\n"
		"  -16 for 16 bits/pixel. -24 for 24 bits/pixel. Default is %d.\n"
		"  -l or -loop to loop until the STOP key is pressed.\n"
		"  -? to just print usage info.\n"
		"  Default stream file is \"" DEFAULT_STREAM_FILENAME "\".\n"
		"\n"
		"  Control Pad functions:\n"
		"    []            Stop\n"
		"    >/||          Pause/Resume\n"
		"    Up            Jump forward to the next marker (requires markers in the stream)\n"
		"    Down          Jump backward to the previous marker (requires markers)\n"
		"    Left          Jump to start of stream ASAP\n"
		"    Shifted-Left  'Cut' to start of stream\n",
		DEFAULT_BITMAP_DEPTH));
	}


/*********************************************************************************************
 * Play a video stream, given a display pixel size and a filename.
 *********************************************************************************************/
static void VideoPlayer(uint32 bitDepth, char *filename, bool fLoop)
	{
	int32			status;
	JoyPadState 	jpState;

	/* Initialize the library code */
	if ( !StartUp(bitDepth) )
		{
		APRNT(("VideoPlayer StartUp() initialization failed!\n"));
		return;
		}

	/* Ask JoyPad to treat the shift keys as "continuous buttons",
	 * so GetJoyPad() will say whether they're down instead of whether
	 * they just went down. */
	SetJoyPadContinuous(PADSHIFT, 1);

	/* Play the movie */
	status = PlayVideoStream(&screenInfo, bitDepth, filename,
		myUserFunction, &jpState, fLoop);
	FAIL_NEG("PlayVideoStream", status);

	if ( status == 0 )
		APRNT(("VideoPlayer end of stream reached\n"));
	else if ( status == 1 )
		APRNT(("VideoPlayer stream stopped by control pad\n"));

FAILED:
	/* Free resources allocated by GetJoyPad. */
	KillJoypad();

	/* [TBD] Free resources allocated by InitGraphics, in particular, the memory allocated.
	 * Probably have to delete the Items first. */
	}


/******************************************************************************
|||	AUTODOC -public -class Examples -group Streaming -name VideoPlayer
|||	A DataStream example program that plays synchronized video and audio.
|||
|||	  Format -preformatted
|||
|||	    VideoPlayer [-16 | -24] [-l | -loop] [-?] [<streamFile>]
|||	        -16 for 16 bits/pixel. -24 for 24 bits/pixel. Default is 24.
|||	        -l or -loop to loop until the STOP key is pressed.
|||	        -? to just print usage info.
|||	        The default <streamFile> is "video1.stream".
|||
|||	        Control Pad functions:
|||	          []            Stop
|||	          >/||          Pause/Resume
|||	          Up            Jump forward to the next marker (requires markers in the stream)
|||	          Down          Jump backward to the previous marker (requires markers)
|||	          Left          Jump to start of stream ASAP
|||	          Shifted-Left  'Cut' to start of stream
|||
|||	  Description
|||
|||	    This is an example M2 application that uses DataStreaming
|||	    components to playback a stream containing synchronized video and
|||	    audio data.
|||
|||	  Location
|||
|||	    {3doremote}/Examples/Streaming/VideoPlayer
|||
 ******************************************************************************/
void main(int argc, char **argv)
	{
	char			*filename = DEFAULT_STREAM_FILENAME;
	uint32			bitDepth = DEFAULT_BITMAP_DEPTH;
	int32			i;
	bool			fLoop = FALSE;

	APRNT(("\n\n--- VideoPlayer startup ---\n"
		"  You can press STOP to stop the stream before it's done.\n"
		"  Run \"VideoPlayer -?\" for usage info.\n"));

	/* Parse command line args */
	for ( i = 1; i < argc; ++i )
		{
		if ( argv[i] == NULL )
			break;
		if ( strcmp(argv[i], "-16") == 0 )
			bitDepth = 16;
		else if ( strcmp(argv[i], "-24") == 0 || strcmp(argv[i], "-32") == 0 )
			bitDepth = 24;
		else if ( strcasecmp(argv[i], "-l") == 0 || strcasecmp(argv[i], "-loop") == 0 )
			fLoop = TRUE;
		else if ( argv[i][0] == '-' )
			{
			Usage();
			return;
			}
		else if ( argv[i][0] != '\0' )
			filename = argv[i];
		}

	VideoPlayer(bitDepth, filename, fLoop);

	APRNT(("--- VideoPlayer done ---\n"));
	}
