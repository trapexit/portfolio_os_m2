/*******************************************************************************
**
**	@(#) EZFlixPlayer.c 96/07/02 1.19
**
**	example of using high level EZFlix movie playback routines
**	Usage:		EZFlixPlayer [-16 | -32] <EZFlix stream file name>
**
*******************************************************************************/

#ifndef PRINT_LEVEL		/* may be set by a compiler command-line switch */
#define PRINT_LEVEL 1	/* print error msgs */
#endif

#define	PROGRAM_VERSION_STRING		"1.0a2"

#include <kernel/debug.h>
#include <kernel/task.h>
#include <graphics/graphics.h>
#include <graphics/bitmap.h>
#include <graphics/view.h>
#include <audio/audio.h>
#include <streaming/saudiosubscriber.h>

#include "JoyPad.h"
#include "PlayEZFlixStream.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define NTSC_FRAME_WIDTH 320
#define NTSC_FRAME_HEIGHT 240

/******************************/
/* Library and system globals */
/******************************/

static ScreenInfo gScreenInfo;
static uint32 gHAverageOn = 0;
static uint32 gVAverageOn = 0;

#define DEFAULT_BITMAP_DEPTH		24

/******************************/
/* Utility routine prototypes */
/******************************/
static Boolean	StartUp(uint32 bitDepth);
static Err		InitGraphics(uint32 bitDepth);


/*********************************************************************************************
 * Perform one-time graphics initializations:
 * Open the Graphics Folio, create two Bitmap Items (for double-buffering), allocate their
 * bitmap buffers (with appropriate alignment and filled with zeros), create a View Item, and
 * add the View Item to the system View List.
 *********************************************************************************************/
static Err InitGraphics(uint32 bitDepth)
	{
	int32		i;
	Err			err;
	Bitmap		*bm;

	if ( (err = OpenGraphicsFolio()) < 0 )
		return err;

	for ( i = 0; i < cntFrameBuffers; ++i )
		{
		gScreenInfo.bitmapItem[i] = err = CreateItemVA(
			MKNODEID(NST_GRAPHICS, GFX_BITMAP_NODE),
			BMTAG_WIDTH,		NTSC_FRAME_WIDTH,
			BMTAG_HEIGHT,		NTSC_FRAME_HEIGHT,
			BMTAG_TYPE,			(bitDepth == 16) ? BMTYPE_16 : BMTYPE_32,
			BMTAG_DISPLAYABLE,	TRUE,
		/*	BMTAG_BUMPDIMS,		TRUE,	*/	/* [TBD] This asks the Graphics
		 * Folio to bump the width and height to satisfy all hardware
		 * requirements. Before we can ask for BUMPDIMS, we have to use the
		 * (bumped) bm_Width and bm_Height fields rather than using the
		 * requested width and height anywhere else in the code. */
			TAG_END);
		FAIL_NEG("EZFlixPlayer.InitGraphics: Create a Bitmap Item", err);
		gScreenInfo.bitmap[i] = bm = LookupItem(gScreenInfo.bitmapItem[i]);

		gScreenInfo.allocatedBuffer[i] = AllocMemMasked(
			bm->bm_BufferSize,
			bm->bm_BufMemType | MEMTYPE_FILL,	/* cleared to black */
			bm->bm_BufMemCareBits,
			bm->bm_BufMemStateBits);
		err = ER_NoMem;
		FAIL_NIL("EZFlixPlayer.InitGraphics: Alloc a bitmap buffer",
			gScreenInfo.allocatedBuffer[i]);

		err = ModifyGraphicsItemVA(gScreenInfo.bitmapItem[i],
			BMTAG_BUFFER,	gScreenInfo.allocatedBuffer[i],
			TAG_END);
		FAIL_NEG("EZFlixPlayer.InitGraphics: Install a bitmap buffer", err);
		}

	gScreenInfo.renderSignal = AllocSignal(0);
	err = ( gScreenInfo.renderSignal == 0 ) ? kDSNoSignalErr : 0;
	FAIL_NEG("EZFlixPlayer: Alloc render signal", err);

	gScreenInfo.viewItem = err = CreateItemVA(MKNODEID(NST_GRAPHICS, GFX_VIEW_NODE),
				VIEWTAG_PIXELWIDTH,		NTSC_FRAME_WIDTH,
				VIEWTAG_BITMAP,			gScreenInfo.bitmapItem[1],
				VIEWTAG_RENDERSIGNAL,	gScreenInfo.renderSignal,
				VIEWTAG_AVGMODE,		AVGMODE_H * gHAverageOn + AVGMODE_V * gVAverageOn,
				TAG_END);
	FAIL_NEG("EZFlixPlayer.InitGraphics: Create a View Item", err);

	err = AddViewToViewList(gScreenInfo.viewItem, 0);
	FAIL_NEG("EZFlixPlayer.InitGraphics: AddViewToViewList", err);

	WaitSignal(gScreenInfo.renderSignal);	/* discard the first renderSignal */

	return 0;

FAILED:
	return err;		/* [TBD] cleanup? */
	}


/*********************************************************************************************
 * Routine to perform any one-time initializations
 *********************************************************************************************/
static Boolean StartUp( uint32 bitDepth )
	{
	if ( (OpenAudioFolio() < 0) )
		return false;

	if ( InitGraphics(bitDepth) < 0 )
		return false;

	return true;
	}


/*********************************************************************************************
 * This function is called each time through the player's main loop and can be used to
 * implement any user interface actions necessary. Returning a zero causes playback to
 * continue. Any other value causes playback to cease and immediately return from
 * the PlayEZFlixStream() function.
 *********************************************************************************************/
int32 myUserFunction( PlayerPtr ctx )
	{
	int32			status;
	JoyPadStatePtr	jp;

	/* We don't have much context other than the player context.
	 * In fact, our only additional context is a pointer to the
	 * joypad state block to interface with the joypad functions.
	 */
	jp = (JoyPadStatePtr) ctx->userContext;

	/* Read control pad #1. */
	GetJoyPad( jp, 1 );

	if ( jp->xBtn )
		return 1;				/* [] button ==> stop the stream. */

	else if ( jp->leftArrow )	/* leftArrow ==> flush-branch to start of stream */
		{
		status = DSGoMarker( ctx->messageItem, NULL, ctx->streamCBPtr, 0,
			GOMARKER_ABSOLUTE );
		FAIL_NEG( "DSGoMarker", status );
		}

	else if ( jp->upArrow )		/* upArrow ==> non-flush-branch to start of stream */
		{
		status = DSGoMarker( ctx->messageItem, NULL, ctx->streamCBPtr, 0,
			GOMARKER_ABSOLUTE | GOMARKER_NO_FLUSH_FLAG );
		FAIL_NEG( "DSGoMarker no-flush", status );
		}

	else if ( jp->startBtn )	/* >/|| button ==> pause/unpause */
		{
		if ( DSIsRunning(ctx->streamCBPtr) )
			{
			/* Note: we don't use the SOPT_FLUSH option when we stop
			 * here so that the button acts like a pause/resume
			 * key. Using the SOPT_FLUSH option will cause buffered
			 * data to be flushed, so that once resumed, any queued
			 * audio will vanish. This is not what is usually desired
			 * for pause/resume.
			 */
			status = DSStopStream( ctx->messageItem, NULL, ctx->streamCBPtr, SOPT_NOFLUSH );
			FAIL_NEG( "DSStopStream", status );
			}
		else
			{
			status = DSStartStream( ctx->messageItem, NULL, ctx->streamCBPtr, SOPT_NOFLUSH );
			FAIL_NEG( "DSStartStream", status );
			}
		}
	else if ( jp->leftShift )	/* LeftShift ==> Toggle H pixel averaging */
		{
		 	gHAverageOn = 1 - gHAverageOn;
			APRNT(("EZFlixPlayer: Horizontal averaging is: %d\n",gHAverageOn));
			status = ModifyGraphicsItemVA(gScreenInfo.viewItem,
				VIEWTAG_AVGMODE,		AVGMODE_H * gHAverageOn + AVGMODE_V * gVAverageOn,
				TAG_END);
			FAIL_NEG("EZFlixPlayer.ModifyGraphicsItemVA: Changing h/v averaging", status);
			status = 0;
		}
	else if ( jp->rightShift )	/* RightShift ==> Toggle V pixel averaging */
		{
		 	gVAverageOn = 1 - gVAverageOn;
			APRNT(("EZFlixPlayer: Vertical averaging is: %d\n",gVAverageOn));
			status = ModifyGraphicsItemVA(gScreenInfo.viewItem,
				VIEWTAG_AVGMODE,		AVGMODE_H * gHAverageOn + AVGMODE_V * gVAverageOn,
				TAG_END);
			FAIL_NEG("EZFlixPlayer.ModifyGraphicsItemVA: Changing h/v averaging", status);
			status = 0;
		}

	else
		status = 0;

FAILED:
#if 0	/* If we really wanted to do this, it should be done only after
		 * DSGoMarker() calls that go to a potentially out-of-range position.
		 * This procedure never does that. */
	/* If one of the positioning requests resulted in our requesting a position
	 * past the end of the stream or before the beginning, then ignore the
	 * error. */
	if ( status == kDSRangeErr )
		status = 0;
#endif

	return status;
	}


/******************************************************************************
|||	AUTODOC -public -class Examples -group Streaming -name EZFlixPlayer
|||	A DataStream example program that plays synchronized video and audio.
|||
|||	  Format -preformatted
|||
|||	    EZFlixPlayer [-16 | -32] [<stream filename>]
|||	        -16 for 16 bits/pixel. -32 for 32 bits/pixel (the default).
|||
|||	        Control Pad functions:
|||	            []          Stop
|||	            >/||        Pause/Resume
|||	            LeftArrow   Jump to start of stream
|||	            UpArrow     "Cut" to start of stream without flushing
|||	            LeftShift   Toggle H pixel averaging
|||	            RightShift  Toggle V pixel averaging
|||
|||	  Description
|||
|||	    This is an example 3DO application that uses DataStreaming
|||	    components to playback a stream containing EZFlix video synchronized
|||	    with SquashSound audio data.
|||
|||	  Location
|||
|||	    {3doremote}/Examples/Streaming/EZFlixPlayer
|||
 ******************************************************************************/
/*********************************************************************************************
 * Main program
 *********************************************************************************************/
int	 main( int argc, char **argv )
{
	int32			status;
	JoyPadState 	jpState;
	uint32			bitDepth = DEFAULT_BITMAP_DEPTH;
	char*			filename = NULL;
	uint32			argNum = 0;

	APRNT(("--- EZFlixPlayer startup ---\n"));

	/* check the arguments */
	if (argc > 3) {
		PERR(("### %s - too many arguments were specified\n", argv[0]));
		goto UsageExit;
	}

	/* scan for a bitdepth argument */
	while (++argNum < argc) {
		char*			pEnd;

		if (argv[argNum][0] == '-')
		{
			bitDepth = strtol(&argv[argNum][1], &pEnd, 10);
			if (pEnd[0] != 0 || (bitDepth != 16 && bitDepth != 32))
			{
				PERR(("### %s - pixel bit depth arg must be '-16' or '-32', not '%s'\n",
					argv[0], argv[argNum]));
				goto UsageExit;
			}
		}
		else if (filename == NULL)
		{
			filename = argv[argNum];
		} else {
			PERR(("### %s - extra filename argument: %s\n", argv[0],
				argv[argNum]));
			goto UsageExit;
		}
	}

	if (filename == NULL) {
		PERR(("### %s - need a stream filename argument\n", argv[0]));
		goto UsageExit;
	}

	/* Initialize the library code */
	if ( ! StartUp(bitDepth) )
	{
		PERR(("StartUp initialization failed!\n"));
		return 0;
	}

	/* Play the movie */
	status = PlayEZFlixStream( &gScreenInfo, filename,
				myUserFunction, &jpState );
	FAIL_NEG( "PlayEZFlixStream", status );

	if ( status == 0 )
		APRNT(("End of stream reached\n"));

	else if ( status == 1 )
		APRNT(("Stream stopped by control pad input\n"));

FAILED:

	APRNT(("--- EZFlixPlayer done ---\n"));

	/* Free resources allocated by GetJoyPad (specifically by InitEventUtility which
	 * is called by GetJoyPad).  KillJoypad returns an error code, but we're not
	 * checking for it. */
	KillJoypad();

	return 0;

UsageExit:
	APRNT(("Usage:  %s [-16 | -32] <stream filename>\n", argv[0]));
	APRNT(("--- EZFlixPlayer done ---\n"));
	return 0;
}

