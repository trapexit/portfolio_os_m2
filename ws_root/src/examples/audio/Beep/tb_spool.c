/*******************************************************
**
** @(#) tb_spool.c 96/08/27 1.4
*******************************************************/

/**
|||	AUTODOC -public -class examples -group Beep -name tb_spool
|||	Spool audio from memory using the Beep folio.
|||
|||	  Format
|||
|||	    tb_spool
|||
|||	  Description
|||
|||	    Spool continuous audio to the beep folio.
|||	    Plays random tones until stopped. Up and Down buttons
|||	    cause program to switch to higher or lower DMA channels.
|||	    Demonstrate use of SetBeepChannelDataNext()  with signal.
|||	    This technique can be used to spool audio off of disk.
|||
|||	  Associated Files
|||
|||	    <beep/beep.h>  <beep/basic_machine.h>
|||
|||	  Location
|||
|||	    Examples/Audio/Beep
|||
**/

/******************************************************
** Author: Phil Burk
** Copyright 1995 3DO
** All Rights Reserved
*******************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <kernel/types.h>
#include <beep/beep.h>
#include <beep/basic_machine.h>
#include <audio/parse_aiff.h>
#include <loader/loader3do.h>
#include <misc/event.h>

#define	PRT(x)	{ printf x; }
#define	DBUG(x)	/* PRT(x) */

/* Macro to simplify error checking. */
#define CALL_CHECK(_exp,msg) \
	DBUG(("%s\n",msg)); \
	do \
	{ \
		if ((result = (Err) (_exp)) < 0) \
		{ \
			PRT(("error code = 0x%x\n",result)); \
			PrintError(0,"\\failure in",msg,result); \
			goto clean; \
		} \
	} while(0)

Err   FillAndSpoolBuffer( int32 channelNum, int16 *buff, int32 signal, int32 freq );
Item  tInit( void );
Err   tTerm( Item  BeepMachine );

/* For spooling sound from a file, use long buffers to prevent starvation.
** For spooling real-time sound use short buffers for low latency.
*/
#define SAMPLES_PER_BUFFER  (4000)

/* Declare two buffer for audio double buffering similar to graphics double buffering. */
static int16 gBuffer0[SAMPLES_PER_BUFFER];
static int16 gBuffer1[SAMPLES_PER_BUFFER];

/*******************************************************
** MAIN
*/
int32 main( int32 argc, char **argv )
{
	int32   result = -1;
	Item    BeepMachine;
	int32   channelNum = 0, oldChannel = 0;
	ControlPadEventData cped;
	int32   doIt = TRUE, ifNewChannel = TRUE;
	uint32  buttons, oldbuttons = 0;
	int32   sig, sig0, sig1;
	int32   freq;
	
	TOUCH(argc);

	if( (BeepMachine = tInit()) < 0) goto clean;

/* Allocate a signal for each buffer. */
	CALL_CHECK( (sig0 = AllocSignal(0)),"AllocSignal");
	CALL_CHECK( (sig1 = AllocSignal(0)),"AllocSignal");

	while(doIt)
	{

		if( ifNewChannel )
		{
/* Set Amplitude */
			CALL_CHECK( (result = SetBeepVoiceParameter( oldChannel, BMVP_AMPLITUDE, 
				0.0 )), "SetBeepVoiceParameter AMP");
			CALL_CHECK( (result = SetBeepVoiceParameter( channelNum, BMVP_AMPLITUDE, 
				1.0 )), "SetBeepVoiceParameter AMP");
/* Stop DMA */
			CALL_CHECK( (result = StopBeepChannel( oldChannel )), "StopBeepChannel");

/* Start spooling on new channel. */
			CALL_CHECK( (result = SetBeepChannelData( channelNum,
				gBuffer0, SAMPLES_PER_BUFFER )), "SetBeepChannelData");
			CALL_CHECK( (result = FillAndSpoolBuffer( channelNum, gBuffer1, sig1, 111 )), "FillAndSpoolBuffer");
			PRT(("Channel %d\n", channelNum));
			CALL_CHECK( (result = StartBeepChannel( channelNum )), "StartBeepChannel");
			oldChannel = channelNum;
			ifNewChannel = FALSE;
		}

/* Generate random frequency. */
		freq = (rand() & 255) + 100;

/* When signal is received, that means buffer is now playing and opposite buffer is available
** for writing.
*/
		sig = WaitSignal( sig0 | sig1 );
		if( sig & sig0 )
		{
			CALL_CHECK( (result = FillAndSpoolBuffer( channelNum, gBuffer1,
				sig1, freq )), "FillAndSpoolBuffer");
		}
		else if( sig & sig1 )
		{
			CALL_CHECK( (result = FillAndSpoolBuffer( channelNum, gBuffer0,
				sig0, freq )), "FillAndSpoolBuffer");
		}

/* Poll control pad. */
		CALL_CHECK( (result = GetControlPad (1, FALSE, &cped)), "GetControlPad");
		buttons = cped.cped_ButtonBits;

/* Switch to new channel using UP DOWN buttons. */
		if(buttons & ControlUp & ~oldbuttons)
		{
			ifNewChannel = TRUE;
			if( channelNum < (BM_NUM_CHANNELS-1) ) channelNum++;
		}
		else if(buttons & ControlDown & ~oldbuttons)
		{
			ifNewChannel = TRUE;
			if( channelNum > 0 ) channelNum--;
		}

/* Are we done? */
		if(buttons & ControlX & ~oldbuttons)
		{
			PRT(("ControlX hit. Buttons = 0x%x\n", buttons));
			doIt = FALSE;
		}
	}

	tTerm( BeepMachine );
	PRT(("Finished %s\n", argv[0]));
	return 0;
	
clean:
	tTerm( BeepMachine );
	printf("Error = 0x%x\n", result );
	return result;
}

/*******************************************************
**  Fill a buffer with sawtooth waves at a given frequency.
**  Then schedule it to be played after the current buffer.
*/
Err FillAndSpoolBuffer( int32 channelNum, int16 *buff, int32 signal, int32 freq )
{
	int32 i, result;
	int16 *b;
	static int16 phase = 0;

	b = buff;
	for( i = 0; i<SAMPLES_PER_BUFFER; i++ )
	{
		phase += freq;
		*b++ = phase;
	}
	
	CALL_CHECK( (result = SetBeepChannelDataNext( channelNum,
				buff, SAMPLES_PER_BUFFER,
	/* BEEP_F_IF_GO_FOREVER will prevent stall after data starvation. */
				BEEP_F_IF_GO_FOREVER,
				signal )), "SetBeepChannelDataNext");
clean:
	return result;
}

/*******************************************************
**
*/
Item tInit( void )
{
	int32 result;
/* Initialize the EventBroker. */
	result = InitEventUtility(1, 0, TRUE);
	if (result < 0)
	{
		PrintError(0,"InitEventUtility",0,result);
		goto clean;
	}

	CALL_CHECK( (result = OpenBeepFolio()), "OpenBeepFolio");
	CALL_CHECK( (result = LoadBeepMachine( BEEP_MACHINE_NAME )), "LoadBeepMachine");
clean:
	return result;
}

/*******************************************************
**
*/
int32 tTerm( Item  BeepMachine )
{
	int32 result;
	CALL_CHECK( (result = UnloadBeepMachine( BeepMachine )), "UnloadBeepMachine" );
	result = CloseBeepFolio();
clean:
	return result;
}
