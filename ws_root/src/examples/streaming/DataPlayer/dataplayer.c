/****************************************************************************
**
**  @(#) dataplayer.c 96/07/02 1.9
**
**	Contains:		Simple example of playing test data through the DATA Subscriber.
**
** 	Usage:		DATAPlayer [-l[oop]] <streamFile>
** 				"A"		button: rewinds the stream
**				"B"		button: and up/down arrows cycle through the channel displayed
**				"C"		button: toggles on-screen data display
** 				"Stop"	button: exits the program
**
*****************************************************************************/

#include <kernel/debug.h>
#include <kernel/task.h>
#include <misc/event.h>
#include <misc/debugconsole.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <streaming/datastreamlib.h>

#include "joypad.h"
#include "initstream.h"

/******************************/
/*      	defines		      */
/******************************/

/* undefine "USE_DEBUG_CONSOLE" to redirect output from this program to the debugger console
 *  instead of to the screen
 */
#define	USE_DEBUG_CONSOLE
#ifndef	USE_DEBUG_CONSOLE
#define	DebugConsolePrintf	printf
#endif

#define	PROGRAM_VERSION_STRING		"1.0"

/******************************/
/*      	types		      */
/******************************/

/* application context */
typedef struct PlayerCtx_tag
{
	int32			showData;		/* show on-screen data about subscriber? */
	int32			currChannel;	/* subscriber channel to use */
	DSRequestMsg	eosMessage;		/* end of stream message */
	bool			haveConsole;	/* did we successfully create a debug console */
	bool			loop;			/* does the player loop at end of stream? */
} PlayerCtx, *PlayerCtxPtr;


/******************************/
/*      Global variables      */
/******************************/



/******************************/
/* Utility routine prototypes */
/******************************/
static	void 	Usage(char* programName);
static	int32	CheckForEndOfStream(PlayerCtxPtr playerCtxPtr, StreamBlockPtr streamPtr);
static	int32	PlayerUI(PlayerCtxPtr playerCtxPtr, StreamBlockPtr streamPtr);
static	Err		StartUp(int argc, char **argv, PlayerCtxPtr playerCtx, StreamBlockPtr streamCtx);


/*******************************************************************************************
 * Display command usage instructions.
 *******************************************************************************************/
static void
Usage(char* programName)
{
	TOUCH(programName);

	APRNT(("%s version %s\n", programName, PROGRAM_VERSION_STRING));
	APRNT(("usage: %s [-l[oop]] <streamFile> \n", programName));
	APRNT(("\t\"-l\" or \"-loop\" to loop until the STOP key is pressed.\n"));
	APRNT(("Control Pad functions:\n"));
	APRNT(("\t\"A\"    button: rewinds the stream\n"));
	APRNT(("\t\"B\"    button: and up/down arrows cycle through the channel displayed\n"));
	APRNT(("\t\"C\"    button: toggle loop stream (off by default)\n"));
	APRNT(("\t\"STOP\" button: exits the program\n"));
}


/*
 * see if we're at the end of the stream, return non-zero if it's time to quit
 */
static int32
CheckForEndOfStream(PlayerCtxPtr playerCtxPtr, StreamBlockPtr streamPtr)
{
	int32			err = 0;
	uint32			signals;
	Item			msgItem;

	/* peek at incoming signals.
	 * if we don't have a signal set on our port, nothing to do */
	if ( (0 == (signals = GetCurrentSignals())) || (0 == (signals & streamPtr->messagePortSignal)) )
		goto EXIT;

	while ( 0 != (err = msgItem = GetMsg(streamPtr->messagePort)) )
	{
		CHECK_OS_ERR(err, ("main: failed in GetMsg()\n"), EXIT);
		if ( msgItem == streamPtr->endOfStreamMessageItem )
		{
			/* Process the EOS msg reply. */
			err = MESSAGE(msgItem)->msg_Result;
			CHECK_OS_ERR(err, ("main: failed in EOS reply\n"), EXIT);
			if ( true == playerCtxPtr->loop )
			{
				err = DSWaitEndOfStream(streamPtr->endOfStreamMessageItem,
							&playerCtxPtr->eosMessage, streamPtr->streamCBPtr);
				CHECK_OS_ERR(err, ("main: failed in DSWaitEndOfStream\n"), EXIT);

				err = DSGoMarker(streamPtr->messageItem, NULL,
							streamPtr->streamCBPtr, 0, GOMARKER_ABSOLUTE);
				CHECK_OS_ERR(err, ("main: failed in DSGoMarker\n"), EXIT);
			}
			else
				err = 1;	/* return non-zero if it's time to quit */
			break;
		}
	}

EXIT:
	return err;
}


/*
 * PlayerUI
 *  Handle all UI for the player.  Return true if it's time to quit the app
 */
static int32
PlayerUI(PlayerCtxPtr playerCtxPtr, StreamBlockPtr streamPtr)
{
	int32		err = 0;
	JoyPadState	padState;

	/* read the current joypad state, bail now if we're supposed to quit */
	GetJoyPad(&padState, 1);
	if ( true == padState.xBtn )
	{
		err = 1;
		goto EXIT;
	}

	if ( true == padState.aBtn )
	{
		/* The "A" button causes a rewind  */

		/* Stop the current stream with the flush option so all subscribers
		 * will flush any data they currently have.  Otherwise they might
		 * hang onto it forever... keeping stream buffers in use forever.
		 */
		err = DSGoMarker(streamPtr->messageItem, NULL, streamPtr->streamCBPtr, 0,
							GOMARKER_ABSOLUTE);
		CHECK_OS_ERR(err, ("DSGoMarker() failed\n"), EXIT);
	}
	else if ( true == padState.bBtn )
	{
		/* "B" button and up/down arrows cycle through the channel we display */
		if ( padState.upArrow )
		{
			if ( ++playerCtxPtr->currChannel >= kDATA_SUB_MAX_CHANNELS )
				playerCtxPtr->currChannel = 0;
		}
		else if ( padState.downArrow )
		{
			if ( --playerCtxPtr->currChannel < 0 )
				playerCtxPtr->currChannel = kDATA_SUB_MAX_CHANNELS - 1;
		}
	}
	else if ( true == padState.cBtn )
	{
		/* "C" button toggles the on-screen data display flag */
		playerCtxPtr->showData = !playerCtxPtr->showData;
	}

EXIT:
	return err;
}


/*
 * OurAllocMem
 *	Simple wrapper around AllocMem() to ensure that the subscriber alloc
 *	 function can be reset
 */
void *
OurAllocMem(uint32 size, uint32 typeBits, uint32 chunkType, int32 channel, int32 time)
{
	/* "touch" the variables to avoid compiler warning when compiling with DEBUG off */
	TOUCH(size);
	TOUCH(typeBits);
	TOUCH(chunkType);
	TOUCH(channel);
	TOUCH(time);

#if 0
	/* uncomment the following lines if you want to be sure it works (or believe */
	/*  me that it does...) */
	PRNT(("OurAllocMem called\n"));
	PRNT(("    size     TypeBits    SubType     Channel    Time\n"));
	PRNT(("  --------- ---------   --------   --------   --------  \n"));
	PRNT((" %8ld %8X   %8.4s  %9ld %10ld \n\n",
				size, 					/* size of chunk to allocate */
				typeBits, 				/* memory type bits */
				(char *)&chunkType,		/* chunk subtype */
				channel,				/* channel number */
				time));					/* stream time */
#endif

	return AllocMem(size, typeBits | MEMTYPE_TRACKSIZE);
}

/*
 * OurFreeMem
 *	Simple wrapper around FreePtr() to ensure that the subscriber free
 *	 function can be reset
 */
void *
OurFreeMem(void *ptr, uint32 chunkType, int32 channel, int32 time)
{
	/* "touch" the variables to avoid compiler warning when compiling with DEBUG off */
	TOUCH(ptr);
	TOUCH(chunkType);
	TOUCH(channel);
	TOUCH(time);

#if 0
	/* uncomment the following lines if you want to be sure it works (or believe */
	/*  me that it does...) */
	PRNT(("OurFreeMem called\n"));
	PRNT(("    Ptr       SubType     Channel    Time   \n"));
	PRNT(("  ---------   --------   --------   --------  \n"));
	PRNT((" 0x%8p  %8.4s  %9ld %10ld \n\n",
				ptr, 					/* ptr to free */
				(char *)&chunkType,		/* chunk subtype */
				channel,				/* channel number */
				time));					/* stream time */
#endif

	FreeMem(ptr, -1);
	return NULL;
}


/*********************************************************************************************
 * Initialize the DataStreamer and play back DATA subscriber data.
 *********************************************************************************************/
static Err
StartUp(int argc, char **argv, PlayerCtxPtr playerCtx, StreamBlockPtr streamCtx)
{
	Err		err = -1;

	/* clear the context... */
	memset(playerCtx, 0, sizeof(PlayerCtx));
	memset(streamCtx, 0, sizeof(*streamCtx));

	/* sanity check params, get any options */
	if ( (argc < 2) || (argc > 3) )
	{
		APERR(("%s: wrong number of arguments\n", argv[0]));
		Usage(argv[0]);
		goto ERROR_EXIT;
	}
	if ( argc == 3 )
	{
		if ( (0 == strcasecmp(argv[1], "-l")) ||  (0 == strcasecmp(argv[1], "-loop")) )
			playerCtx->loop = TRUE;
		else
		{
			APERR(("%s: unhappy argument \"%s\", expected \"-l\" or \"-loop\"\n",
				argv[0], argv[1]));
			Usage(argv[0]);
			goto ERROR_EXIT;
		}
		++argv;
	}

#ifdef	USE_DEBUG_CONSOLE
	#define X_OFFSET		0
	#define Y_OFFSET		40	/* get the top line into the screen's safe area */

	err = CreateDebugConsoleVA(DEBUGCONSOLE_TAG_HEIGHT, 480, TAG_END);
	CHECK_OS_ERR(err, ("StartUp: failed in CreateDebugConsole()\n"), ERROR_EXIT);
	DebugConsoleMove(X_OFFSET, Y_OFFSET);
	playerCtx->haveConsole = true;
#endif		/* USE_DEBUG_CONSOLE */

	/* start-up streaming */
	err = StartStreamFromHeader(streamCtx, argv[1], kDataSubFlag);
	CHECK_OS_ERR(err, ("StartUp: failed in StartStreamFromHeader()\n"), ERROR_EXIT);

	/* set the alloc/free functions to wrappers in this file so we can make sure
	 *  the callback process works correctly
	 */
	err = SetDataMemoryFcns(streamCtx->dataCBPtr, (DataAllocMemFcn)OurAllocMem,
							(DataFreeMemFcn)OurFreeMem);
	CHECK_OS_ERR(err, ("StartUp: failed in SetDataMemoryFcns()\n"), ERROR_EXIT);

	/* Register for end of stream notification. Do it before DSStartStream so
	 * we won't miss it in a short or empty stream.
	 */
	err = DSWaitEndOfStream(streamCtx->endOfStreamMessageItem, &playerCtx->eosMessage,
					streamCtx->streamCBPtr);
	CHECK_OS_ERR(err, ("StartUp: failed in DSWaitEndOfStream"), ERROR_EXIT);

	/* start the stream... */
	err = DSStartStream(streamCtx->messageItem, NULL, streamCtx->streamCBPtr, SOPT_FLUSH);
	CHECK_OS_ERR(err, ("StartUp: failed in DSStartStream()\n"), ERROR_EXIT);

ERROR_EXIT:
	return err;
}



/*
 * Exiting now, free any resources we allocated.
 */
void
ShutDown(PlayerCtxPtr playerCtx, StreamBlockPtr streamCtx)
{
	KillJoypad();
	ShutDownStreaming(streamCtx);
	if ( true == playerCtx->haveConsole )
		DeleteDebugConsole();
}


/******************************************************************************
|||	AUTODOC -public -class Examples -group Streaming -name DataPlayer
|||	A DataStream DATA subscriber example program.
|||
|||	  Format -preformatted
|||
|||	    DataPlayer [-l[oop]] <streamFile>
|||
|||	        Control Pad functions:
|||	          "A"     button: rewinds the stream
|||	          "B"     button: and up/down arrows cycle through the channel displayed
|||	          "C"     button: toggles on-screen data display
|||	          "Stop"  button: exits the program
|||
|||	  Description
|||
|||	    This is an example M2 application that uses DataStreaming
|||	    components to playback a stream containing DATA subscriber data.
|||
|||	  Location
|||
|||	    {3doremote}/Examples/Streaming/DataPlayer
|||
 ******************************************************************************/
int
main(int argc, char **argv)
{
	PlayerCtx		playerCtx;			/* DATA player context */
	StreamBlock		streamBlock;
	DataChunk		dataChunk;
	DSClock			streamClock;
	Err				err;
	char			*space = " ";

	APRNT(("\n\n--- DataPlayer startup ---\n"));

	/* startup the whole thing... */
	if ( 0 != StartUp(argc, argv, &playerCtx, &streamBlock) )
		goto ERROR_EXIT;

	playerCtx.currChannel = kEVERY_DATA_CHANNEL;
	playerCtx.showData = 1;

	DebugConsolePrintf("%14s  stream time    ptr         user[0]    user[1]    size   channel\n\n", space);

	/* Run the stream and check for user action */
	while ( true )
	{
		/* Read the control pad. If the start button is pressed, then
		 * cause the stream to stop playing and exit everything.
		 */
		if ( 0 != PlayerUI(&playerCtx, &streamBlock) )
			break;

		/* and see if it we're at the end of the stream */
		if ( 0 != CheckForEndOfStream(&playerCtx, &streamBlock) )
			break;

		/* Get the next data chunk from the subscriber.  Poll ALL channels even though
		 *  we only show data on one channel so the stream buffers don't back up
		 */
		if ( (err = GetDataChunk(streamBlock.dataCBPtr, playerCtx.currChannel, &dataChunk)) < 0)
			ERROR_RESULT_STATUS("GetDataChunk() failed: ", err);

		/* display the current channel info */
		if ( (true == playerCtx.showData) && (err > 0) )
		{
			DSGetPresentationClock(streamBlock.streamCBPtr, &streamClock);
			DebugConsolePrintf("%22ld       0x%.8p %8.4s  %8.4s  %8ld  %9ld\n",
						streamClock.streamTime,				/* stream time */
						dataChunk.dataPtr, 					/* ptr to data */
						(char *)&(dataChunk.userData[0]),	/* user data */
						(char *)&(dataChunk.userData[1]),	/* user data */
						dataChunk.size,						/* data size */
						dataChunk.channel);					/* channel number */

			/* and free the chunk since we own it now and we don't plan to do anything
			 *  more with it.
			 *
			 * NOTE: since the subscriber is using our own memory allocation functions (set
			 *  with "SetDataMemoryFcns()" in "Startup()" above), we could call "OurFreeMem()"
			 *  (or for that matter, "FreeMemTracked(dataChunk.dataPtr, -1)" since that's all
			 *  IT does) but we'll use the fcn pointer for clarity since that is what we would
			 *  be REQUIRED to do if we hadn't used "SetDataMemoryFcns()"
			 */
			streamBlock.dataCBPtr->freeMemFcn(dataChunk.dataPtr, 0, dataChunk.channel, streamClock.streamTime);
		}
	}

ERROR_EXIT:

	/* Exiting now, free any resources we allocated. */
	ShutDown(&playerCtx, &streamBlock);

	APRNT(("--- DataPlayer done ---\n"));

	return 0;
}

