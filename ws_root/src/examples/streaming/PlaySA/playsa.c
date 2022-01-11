/******************************************************************************
**
**  @(#) playsa.c 96/05/01 1.23
**
******************************************************************************/

#ifndef PRINT_LEVEL		/* may be set by a compiler command-line switch */
#define PRINT_LEVEL 1	/* print error msgs */
#endif

/******************************************************************************
|||	AUTODOC -public -class Examples -group Streaming -name PlaySA
|||	A DataStream example program that plays streamed audio.
|||
|||	  Format -preformatted
|||
|||	    PlaySA <streamFile>
|||	
|||	        Where <streamFile> is a woven DataStream file containing
|||	        SquashSnd audio data.
|||	
|||	    Control Pad functions:
|||	      >/|| button:  Pause/resume
|||	      Stop button:  Stop and exit
|||	      C button:     Rewind the stream
|||	      
|||	      B button:     Cycle the channel number (0-3) for volume & pan adj
|||	      UpArrow:      Increase the volume of the current channel
|||	      DownArrow:    Decrease the volume of the current channel
|||	      LeftArrow:    Pan the current channel left
|||	      RightArrow:   Pan the current channel right
|||	      
|||	      LeftShift+A:  Mute/unmute channel 0
|||	      LeftShift+B:  Mute/unmute channel 1
|||	      LeftShift+C:  Mute/unmute channel 2
|||	      RightShift+C: Mute/unmute channel 3
|||	
|||	  Description -preformatted
|||	
|||	    This is an example 3DO application that uses DataStreaming
|||	    components to playback a stream containing SquashSnd audio data.
|||	    If the streamed audio file does not contain a streamed header,
|||	    the default stream header is used.  The defaults are:
|||	        streamblocksize 32768 (size of stream buffers)
|||	        streambuffers       4 (suggested number of stream buffers to use)
|||	        streamerDeltaPri  -10 (delta priority of streamer thread)
|||	        dataacqdeltapri    -9 (delta priority for data acquisition thread)
|||	        audioclockchan      0 (logical channel number of audio clock
|||	                               channel) 
|||	        enableaudiomask     1 (enables logical audio channel 0)
|||	        
|||	        subscriber SNDS    10 (delta priority of audio subscriber thread) 
|||	        preloadinstrument  SA_22K_16B_M_SQD2,
|||	                           SA_44K_16B_M
|||	                           (preload instrument tags:
|||	                             22KHz, 16 bit mono, 2 to 1 compresses,
|||	                             44KHz, 16 bit mono)
|||
|||	  Location
|||
|||	    {3doremote}/Examples/Streaming/PlaySA
|||
|||	  Caveats
|||
|||	    You will get "Error sending control message" error message if you hit
|||	    the mute button on a channel that is not the current channel.
|||
 ******************************************************************************/

#define	PROGRAM_VERSION_STRING		"1.4"

/* This value is the number of audio ticks PlaySSNDStream() will sleep 
 * between callbacks to the user function.  In other words, how often
 * you will be called back.  Use to keep the task from
 * busy waiting; just reading the control pad as fast as possible.
 */
#define  UI_CALL_BACK_INTERVAL		5			

#include <stdio.h>
#include <stdlib.h>				/* for exit() */

#include <audio/audio.h>
#include <kernel/debug.h>		/* for print macro: CHECK_NEG */
#include <misc/event.h>

#include <streaming/saudiosubscriber.h>
#include <streaming/sacontrolmsgs.h>

#include "playssndstream.h"


/******************************/
/* Utility routine prototypes */
/******************************/
static	Boolean	StartUp( void );
static	void 	Usage( char* programName );
static	int32	DoUserInterface( PlayerPtr ctx );



/**********************************************************/
/* Context Block for maintaining state in the UI function */
/**********************************************************/
typedef struct AudioCB {
	ControlPadEventData	jpState;	/* JoyPad context */
	int32			curChannel;		/* Channel on which to send volume and pan
									 * messages */
	int32			volume;			
	int32			pan;
	uint32			muteStatusBits;	/* Bits represent which channels are
									 * currently muted */
	SAudioCtlBlock	ctlBlock;		/* For sending control messages to the
									 * subscriber */

	} AudioCB, *AudioCBPtr;

AudioCB		UICtx;

/******************************************************************************
 * Routine to perform any one-time initializations
 ******************************************************************************/
static Boolean		StartUp( void )
	{
	if ( (OpenAudioFolio() < 0) )
		return false;

	if ( InitEventUtility ( 1, 0, LC_FocusListener) != 0 )
		return false;
		
	return true;
	}


/******************************************************************************
 * Routine to display command usage instructions.
 ******************************************************************************/
static void 
Usage( char* programName )
{
	APRNT(("%s version %s\n", programName, PROGRAM_VERSION_STRING));
	APRNT(("usage: %s <streamFile>\n"
		"\tControl Pad functions:\n"
		"\t  >/|| button:  Pause/resume\n"
		"\t  Stop button:  Stop and exit\n"
		"\t  C button:     Rewind the stream\n\n"
		"\t  B button:     Cycle the channel number (0-3) for volume & pan adj\n"
		"\t  UpArrow:      Increase the volume of the current channel\n"
		"\t  DownArrow:    Decrease the volume of the current channel\n"
		"\t  LeftArrow:    Pan the current channel left\n"
		"\t  RightArrow:   Pan the current channel right\n\n"
		"\t  LeftShift+A:  Mute/unmute channel 0\n"
		"\t  LeftShift+B:  Mute/unmute channel 1\n"
		"\t  LeftShift+C:  Mute/unmute channel 2\n"
		"\t  RightShift+C: Mute/unmute channel 3\n",
		programName));
}


/******************************************************************************
 * This function is called each time through the player's main loop and is used to
 * implement a user interface. Returning a zero causes playback to continue. Any other 
 * value causes playback to cease and immediately return from the PlaySSNDStream() function.
 ******************************************************************************/
int32 DoUserInterface( PlayerPtr ctx )
{

	int32		status;
	AudioCBPtr	UICtx;
	ControlPadEventData	*jpStatePtr;

	
	/* For readability, get the user interface context block from the player
	 * context */
	UICtx = (AudioCBPtr) ctx->userCB;
	
	jpStatePtr	= &UICtx->jpState;

	/* Read control pad #1. */
	status = GetControlPad( 1, false, jpStatePtr );

	if ( status == 0 )
		/* No problems, don't want to exit stream */
		return status;

	switch ( jpStatePtr->cped_ButtonBits )
	{
		case ControlX:		/* [] ==> stop the stream */
			return 1;
			break;

		case ControlStart:	/* Pause/play button ==> pause/resume */
		{
		
			/* If we're transitioning from stopped to running, start the
			 * stream. Otherwise, stop the stream.
			 */
			if ( !DSIsRunning(ctx->streamCBPtr) )
			{
				status = DSStartStream( ctx->messageItem, NULL,
										ctx->streamCBPtr, 0 );
				FAIL_NEG( "DSStartStream", status );
				PRNT((" resumed\n"));
			}
			else
			{
				/* Note: we don't use the SOPT_FLUSH option when we stop
				 * here so that the "A" button acts like a pause/resume
				 * key. Using the SOPT_FLUSH option will cause buffered
				 * data to be flushed, so that once resumed, any queued
				 * audio will vanish. This is not what is usually desired 
				 * for pause/resume.
				 */
				status = DSStopStream( ctx->messageItem, NULL,
										ctx->streamCBPtr, 0 );
				FAIL_NEG( "DSStopStream", status );
				PRNT((" paused\n"));
			}
		
			/* APRNT((" stream flags = %lx\n", ctx->streamCBPtr->streamFlags )); */
			break;
		} /* ControlStart */
			
		case ControlB:	/* B ==> increment the current channel for volume & pan adjustment */
		{

			/* Mute the channel that was the current channel. */
			UICtx->ctlBlock.mute.channelNumber = UICtx->curChannel;
			status = DSControl( ctx->messageItem, NULL, ctx->streamCBPtr,
								ctx->datatype, kSAudioCtlOpMute, &UICtx->ctlBlock );
			FAIL_NEG( "DSControl - muting the 'was' current channel", status );

			UICtx->curChannel += 1;

			/* Wrap from 3 to 0 */
			if ( UICtx->curChannel >= 4 )
				UICtx->curChannel = 0;
				
			if ( ctx->hdr.enableAudioChan & (1L << UICtx->curChannel) )
				{
				/* Unmute the channel that just became the current channel. */
				UICtx->ctlBlock.unMute.channelNumber = UICtx->curChannel;
				status = DSControl( ctx->messageItem, NULL, ctx->streamCBPtr,
						ctx->datatype, kSAudioCtlOpUnMute, &UICtx->ctlBlock );
				FAIL_NEG( "DSControl - Unmuting the 'is' current channel", status );

				}
			APRNT(("Current channel = %ld.\n", UICtx->curChannel));

			break;
		} /* ControlB */
		
		case ControlC:		/* C ==> rewind the stream */
		{
			APRNT(("Rewinding...\n"));
			status = DSGoMarker( ctx->messageItem, NULL, ctx->streamCBPtr,
						0, GOMARKER_ABSOLUTE );
			FAIL_NEG( "DSGoMarker", status );
			break;
		} /* ControlC */
			
		/* The Up Arrow increases the amplitude of the "current channel". */
		case ControlUp:
		{
			UICtx->ctlBlock.amplitude.channelNumber = UICtx->curChannel;
			status = DSControl( ctx->messageItem, NULL,
								ctx->streamCBPtr, ctx->datatype,
								kSAudioCtlOpGetVol, &UICtx->ctlBlock );
			FAIL_NEG("DoUserInterface DSControl", status);
			UICtx->volume = UICtx->ctlBlock.amplitude.value;
	
			if ( UICtx->volume < 0x7A00 )
			{
				UICtx->volume += 0x500;
	
				UICtx->ctlBlock.amplitude.channelNumber = UICtx->curChannel;
				UICtx->ctlBlock.amplitude.value = UICtx->volume;
			}
			else
			{
				UICtx->volume = 0x7FFF;
	
				UICtx->ctlBlock.amplitude.channelNumber = UICtx->curChannel;
				UICtx->ctlBlock.amplitude.value = UICtx->volume;
			}
				
			status = DSControl( ctx->messageItem, NULL,
								ctx->streamCBPtr,ctx->datatype,
								kSAudioCtlOpSetVol, &UICtx->ctlBlock );
			FAIL_NEG("DoUserInterface DSControl 2", status);
	
			break;
		} /* ControlUp */
			
		/* The Down Arrow Decreases the amplitude of the "current channel". */
		case ControlDown:
		{
			UICtx->ctlBlock.amplitude.channelNumber = UICtx->curChannel;
			status = DSControl( ctx->messageItem, NULL,
								ctx->streamCBPtr, ctx->datatype,
								kSAudioCtlOpGetVol, &UICtx->ctlBlock );
			FAIL_NEG("DoUserInterface DSControl 3", status);

			UICtx->volume = UICtx->ctlBlock.amplitude.value;
	
			if ( UICtx->volume > 0x600 )
			{
				UICtx->volume -= 0x500;
	
				UICtx->ctlBlock.amplitude.channelNumber = UICtx->curChannel;
				UICtx->ctlBlock.amplitude.value = UICtx->volume;
			}
			else
			{
				UICtx->volume = 0x0;
	
				UICtx->ctlBlock.amplitude.channelNumber = UICtx->curChannel;
				UICtx->ctlBlock.amplitude.value = UICtx->volume;
			}
			
			status = DSControl( ctx->messageItem, NULL,
								ctx->streamCBPtr, ctx->datatype,
								kSAudioCtlOpSetVol, &UICtx->ctlBlock );
			FAIL_NEG("DoUserInterface DSControl 4", status);

			break;
		} /* ControlDown */
			
		/* The Left Arrow pans the "current channel" to the left. */
		case ControlLeft:
		{
			UICtx->ctlBlock.pan.channelNumber = UICtx->curChannel;
			status = DSControl( ctx->messageItem, NULL,
								ctx->streamCBPtr, ctx->datatype,
								kSAudioCtlOpGetPan, &UICtx->ctlBlock );
			FAIL_NEG("DoUserInterface DSControl 5", status);

			UICtx->pan = UICtx->ctlBlock.pan.value;
	
			if ( UICtx->pan > 0x600 )
			{
				UICtx->pan -= 0x500;
	
				UICtx->ctlBlock.pan.channelNumber = UICtx->curChannel;
				UICtx->ctlBlock.pan.value = UICtx->pan;
	
				status = DSControl( ctx->messageItem, NULL,
									ctx->streamCBPtr, ctx->datatype,
									kSAudioCtlOpSetPan, &UICtx->ctlBlock );
				if( status == kDSAudioChanErr )
					APRNT(("Can't pan stereo stream\n"));
				else if( status < kDSNoErr )
					goto FAILED;

			}

			break;
		} /* ControlLeft */
			
		/* The Right Arrow pans the "current channel" to the right. */
		case ControlRight:
		{
			UICtx->ctlBlock.pan.channelNumber = UICtx->curChannel;
			status = DSControl( ctx->messageItem, NULL,
								ctx->streamCBPtr, ctx->datatype,
								kSAudioCtlOpGetPan, &UICtx->ctlBlock );
			FAIL_NEG("DoUserInterface DSControl 7", status);

			UICtx->pan = UICtx->ctlBlock.pan.value;
	
			if ( UICtx->pan < 0x7A00 )
			{
				UICtx->pan += 0x500;
	
				UICtx->ctlBlock.pan.channelNumber = UICtx->curChannel;
				UICtx->ctlBlock.pan.value = UICtx->pan;
	
				status = DSControl( ctx->messageItem, NULL,
									ctx->streamCBPtr, ctx->datatype,
									kSAudioCtlOpSetPan, &UICtx->ctlBlock );
							
				if( status == kDSAudioChanErr )
					APRNT(("Can't pan stereo stream\n"));
				else if( status < kDSNoErr )
					goto FAILED;

			}

			break;
		
		} /* ControlRight */

		/* The combination LeftShift-A, toggles the mute on channel 0 */
		case ControlA | ControlLeftShift :
		{
			UICtx->ctlBlock.mute.channelNumber = 0;
	
			if ( UICtx->muteStatusBits & 0x1 )
			{
				status = DSControl( ctx->messageItem, NULL,
									ctx->streamCBPtr, ctx->datatype,
									kSAudioCtlOpUnMute, &UICtx->ctlBlock );
				
				/* Clear the mute status bit for this channel */
				UICtx->muteStatusBits &= ~0x1;
			}
			else
			{
				status = DSControl( ctx->messageItem, NULL,
									ctx->streamCBPtr, ctx->datatype,
									kSAudioCtlOpMute, &UICtx->ctlBlock );
	
				/* Set the mute status bit for this channel */
				UICtx->muteStatusBits |= 0x1;
			}
				
			FAIL_NEG("DoUserInterface DSControl 9", status);

			break;
		} /* ControlA | ControlLeftShift */
			
		/* The combination LeftShift-B, toggles the mute on channel 1 */
		case ( ControlB | ControlLeftShift ):
		{
			UICtx->ctlBlock.mute.channelNumber = 1;

			if ( UICtx->muteStatusBits & 0x2 )
			{
				status = DSControl( ctx->messageItem, NULL,
									ctx->streamCBPtr, ctx->datatype,
									kSAudioCtlOpUnMute, &UICtx->ctlBlock );
				/* Clear the mute status bit for this channel */
				UICtx->muteStatusBits &= ~0x2;
			}
			else
			{
				status = DSControl( ctx->messageItem, NULL,
									ctx->streamCBPtr, ctx->datatype,
									kSAudioCtlOpMute, &UICtx->ctlBlock );

				/* Set the mute status bit for this channel */
				UICtx->muteStatusBits |= 0x2;
			}
				
			FAIL_NEG("DoUserInterface DSControl 10", status);

			break;
		} /* ControlB | ControlLeftShift */

		/* The combination LeftShift-C, toggles the mute on channel 2 */
		case ( ControlC | ControlLeftShift ):
		{
			UICtx->ctlBlock.mute.channelNumber = 2;

			if ( UICtx->muteStatusBits & 0x4 )
			{
				status = DSControl( ctx->messageItem, NULL,
									ctx->streamCBPtr, ctx->datatype,
									kSAudioCtlOpUnMute, &UICtx->ctlBlock );

				/* Clear the mute status bit for this channel */
				UICtx->muteStatusBits &= ~0x4;
			}
			else
			{
				status = DSControl( ctx->messageItem, NULL,	
									ctx->streamCBPtr, ctx->datatype,
									kSAudioCtlOpMute, &UICtx->ctlBlock );

				/* Set the mute status bit for this channel */
				UICtx->muteStatusBits |= 0x4;
			}
				
			FAIL_NEG("DoUserInterface DSControl 11", status);

			break;
		} /* ControlC | ControlLeftShift */
				
		/* The combination RightShift-C, toggles the mute on channel 3 */
		case ( ControlC | ControlRightShift ):
		{
			UICtx->ctlBlock.mute.channelNumber = 3;

			if ( UICtx->muteStatusBits & 0x8 )
			{
				status = DSControl( ctx->messageItem, NULL,
									ctx->streamCBPtr, ctx->datatype,
									kSAudioCtlOpUnMute, &UICtx->ctlBlock );

				/* Clear the mute status bit for this channel */
				UICtx->muteStatusBits &= ~0x8;
			}
			else
			{
				status = DSControl( ctx->messageItem, NULL,
									ctx->streamCBPtr, ctx->datatype,
									kSAudioCtlOpMute, &UICtx->ctlBlock );

				/* Set the mute status bit for this channel */
				UICtx->muteStatusBits |= 0x8;
			}
				
			FAIL_NEG("DoUserInterface DSControl 12", status);

			break;
		} /* ControlC | ControlRightShift */

		default:
			break;
		
	} /* switch ( ctx->CtrlPadState ) */

	/* No problems, don't want to exit stream */
	return 0;

FAILED:
	return status;
	
}


/******************************************************************************
 * Main program
 ******************************************************************************/
int	 main( int argc, char **argv )
	{
	int32	status;

	APRNT(("--- PlaySA startup ---\n"));

	/* Sanity check, need at least the name of a file */
	if ( argc < 2 )
		{
		APRNT(("Error: %s - need a stream file name.\n",
				argv[0]));
		Usage(argv[0]);
		exit(1);
		}

	/* Initialize the library code */
	if ( ! StartUp() )
		{
		APRNT(( "StartUp() initialization failed!\n" ));
		exit(1);
		}


	/* Play the stream, specify how often to call back */
	status = PlaySSNDStream( argv[1], DoUserInterface,
							 (void*) &UICtx, 
							 (int32) UI_CALL_BACK_INTERVAL );
	CHECK_NEG( "PlaySSNDStream", status );

	if ( status == 1 )
		APRNT(( "Stream stopped by control pad input\n" ));

	/* Free resources allocated by GetControlPad (specifically by
	 * InitEventUtility which is called in StartUp.  KillEventUtility
	 * returns an error code, but we're not checking for it.
	 */
	KillEventUtility();

	APRNT(("--- PlaySA done ---\n"));

	return 0;
	}

