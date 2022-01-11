/*******************************************************
**
** @(#) tb_envelope.c 96/08/27 1.4
*******************************************************/

/**
|||	AUTODOC -public -class examples -group Beep -name tb_envelope
|||	Trigger envelopes using the Beep folio.
|||
|||	  Format
|||
|||	    tb_envelope
|||
|||	  Description
|||
|||	    Play a sample and control its envelope
|||	    using the Beep Folio. Up and Down buttons
|||	    cause program to switch to higher or lower DMA channels.
|||	    
|||	    Button A triggers a fast attack and a slow release.
|||	    Button B triggers a slow attack and a fast release.
|||	    Button C forces the amplitude to one then starts a
|||	    slow envelope to zero. May pop.
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
#include <file/fileio.h>
#include <file/filefunctions.h>
#include <beep/beep.h>
#include <beep/basic_machine.h>
#include <audio/parse_aiff.h>
#include <loader/loader3do.h>
#include <misc/event.h>

#define	PRT(x)	{ printf x; }
#define	DBUG(x)	/* PRT(x) */
#define	ERR(x)	PRT(x)

/* Macro to simplify error checking. */
#define CALL_CHECK(_exp,msg) \
	DBUG(("%s\n",msg)); \
	do \
	{ \
		if ((result = (Err) (_exp)) < 0) \
		{ \
			ERR(("error code = 0x%x\n",result)); \
			PrintError(0,"\\failure in",msg,result); \
			goto clean; \
		} \
	} while(0)

Err   tSetupSampleStart( SampleInfo *smpi,  int32 channelNum, int32 signal );
Err   tSetupSampleRelease( SampleInfo *smpi,  int32 channelNum, int32 signal );
Err   tPlayVoice( int32 channelNum, float32 SampleRate, float32 Amplitude);
Err   tSetEnvelope( int32 channelNum, float32 amplitude, float32 rate );
Err   tForceEnvelope( int32 channelNum, float32 startAmplitude, float32 endAmplitude,
                      float32 rate );
Item  tInit( void );
Err   tTerm( Item  BeepMachine );

/*******************************************************
** MAIN
*/
int32 main( int32 argc, char **argv )
{
	int32   result = -1;
	Item    BeepMachine;
	SampleInfo *smpi;
	char   *SampleName;
	int32   channelNum = 0, oldChannel = 0;
	ControlPadEventData cped;
	int32   doIt = TRUE, ifNewChannel = TRUE;
	uint32  buttons, oldbuttons = 0;
	
	PRT(("%s - Press A,B,C buttons to trigger envelopes.\n", argv[0]));
	SampleName = (argc>1) ? argv[1] : "/remote/System.m2/audio/aiff/sinewave.aiff";
	if( (BeepMachine = tInit()) < 0) goto clean;

/* Get SampleInfo from AIFF file including the sample data */
	if ((result = GetAIFFSampleInfo (&smpi, SampleName, 0)) < 0) goto clean;

	while(doIt)
	{
		CALL_CHECK( (result = GetControlPad (1, TRUE, &cped)), "GetControlPad");
		buttons = cped.cped_ButtonBits;

/* Switch to new channel leading edge of UP DOWN buttons. */
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
		
		if( ifNewChannel )
		{
/* Stop old channel. */
		CALL_CHECK( (result = tSetupSampleRelease( smpi, oldChannel, 0 )), "tSetupSample");
		CALL_CHECK( (result = SetBeepVoiceParameter( oldChannel, BMVP_AMPLITUDE, 
				0.0 )), "SetBeepVoiceParameter AMP");

/* Set DMA to start playing sample. */
			CALL_CHECK( (result = tSetupSampleStart( smpi, channelNum, 0 )), "tSetupSample");
/* Start DMA. */
			CALL_CHECK( (result = StartBeepChannel( channelNum )), "StartBeepChannel");
/* Set Amplitude */
			CALL_CHECK( (result = SetBeepVoiceParameter( channelNum, BMVP_AMPLITUDE, 
				0.0 )), "SetBeepVoiceParameter AMP");
/* Set LFO Depth */
			CALL_CHECK( (result = SetBeepVoiceParameter( channelNum, BMVP_FREQ_MOD_DEPTH, 
				2000.0 )), "SetBeepVoiceParameter LFO Depth");
/* Set LFO Rate */
			CALL_CHECK( (result = SetBeepVoiceParameter( channelNum, BMVP_LFO_RATE, 
				2.0 )), "SetBeepVoiceParameter LFO Rate");
			oldChannel = channelNum;
			ifNewChannel = FALSE;
			PRT(("Channel %d\n", channelNum));
		}

/* Calculate envelope rates. */
#define CALC_ENVELOPE_RATE(seconds)  (8.0 / (44100.0 * seconds))
#define FAST_RATE  CALC_ENVELOPE_RATE(0.02)
#define SLOW_RATE  CALC_ENVELOPE_RATE(2.0)

/* Trigger envelope differently depending on button. */
/* Button A has a fast attack and a slow release. */
		if(buttons & ControlA & ~oldbuttons) /* Leading edge? */
		{
			CALL_CHECK( (result = tSetEnvelope( channelNum, 1.0, FAST_RATE )), 
				"tSetEnvelope");
		}
		else if(~buttons & ControlA & oldbuttons) /* Trailing edge? */
		{
			CALL_CHECK( (result = tSetEnvelope( channelNum, 0.0, SLOW_RATE )), 
				"tSetEnvelope");
		}
		

/* Button B has a slow attack and a fast release. */
		if(buttons & ControlB & ~oldbuttons) /* Leading edge? */
		{
			CALL_CHECK( (result = tSetEnvelope( channelNum, 1.0, SLOW_RATE )), 
				"tSetEnvelope");
		}
		else if(~buttons & ControlB & oldbuttons) /* Trailing edge? */
		{
			CALL_CHECK( (result = tSetEnvelope( channelNum, 0.0, FAST_RATE )), 
				"tSetEnvelope");
		}
		
/* Button C forces the amplitude to one then starts a slow envelope to zero. May pop. */
		if(buttons & ControlC & ~oldbuttons) /* Leading edge? */
		{
			CALL_CHECK( (result = tForceEnvelope( channelNum, 1.0, 0.0, SLOW_RATE )), 
				"tForceEnvelope");
		}

		if(buttons & ControlX & ~oldbuttons)
		{
			doIt = FALSE;
		}
		oldbuttons = buttons;
	}


/* Unload Sample*/
	DeleteSampleInfo (smpi);

	tTerm( BeepMachine );
	PRT(("Finished %s\n", argv[0]));
	return 0;
	
clean:
	tTerm( BeepMachine );
	printf("Error = 0x%x\n", result );
	return result;
}

/*******************************************************
**
*/
Item tInit( void )
{
	int32 result;
	
	CALL_CHECK( (result = OpenBeepFolio()), "OpenBeepFolio");
	
/* Initialize the EventBroker. */
	result = InitEventUtility(1, 0, TRUE);
	if (result < 0)
	{
		PrintError(0,"InitEventUtility",0,result);
		goto clean;
	}

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
	
	if( BeepMachine > 0 )
	{
		CALL_CHECK( (result = UnloadBeepMachine( BeepMachine )), "UnloadBeepMachine" );
	}
	result = CloseBeepFolio();
	
clean:
	return result;
}

/*******************************************************
**  tSetEnvelope - Start envelope segment.
*/
Err tSetEnvelope( int32 channelNum, float32 amplitude, float32 rate )
{
	Item result;
/* Set Amplitude */
		CALL_CHECK( (result = SetBeepVoiceParameter( channelNum, BMVP_AMPLITUDE, 
				amplitude )), "SetBeepVoiceParameter AMP");
/* Set Envelope Rate to desired rate. */
		CALL_CHECK( (result = SetBeepVoiceParameter( channelNum, BMVP_AMPLITUDE_RATE, 
				rate )), "SetBeepVoiceParameter AMP Rate");
clean:
	return result;
}

/*******************************************************
**  tForceEnvelope - force envelope to start from a given level.
*/
Err tForceEnvelope( int32 channelNum, float32 startAmplitude, float32 endAmplitude, float32 rate )
{
	Item result;
	
/* Set Envelope Rate to zero so envelope doesn't jump before we set new rate. */
		CALL_CHECK( (result = SetBeepVoiceParameter( channelNum, BMVP_AMPLITUDE_RATE, 
				0.0 )), "SetBeepVoiceParameter AMP Rate");
				
/* Force envelope to starting value. This will probably cause a pop. */
		CALL_CHECK( (result = SetBeepVoiceParameter( channelNum, BMVP_ENV_TARGET, 
				startAmplitude )), "SetBeepVoiceParameter Env Target");
		CALL_CHECK( (result = SetBeepVoiceParameter( channelNum, BMVP_ENV_PHASE, 
				1.0 )), "SetBeepVoiceParameter Env Phase");
		CALL_CHECK( (result = SetBeepVoiceParameter( channelNum, BMVP_ENV_CURRENT, 
				startAmplitude )), "SetBeepVoiceParameter Env Current");
				
/* Set Amplitude. This will reset phase to zero and set source to current. */
		CALL_CHECK( (result = SetBeepVoiceParameter( channelNum, BMVP_AMPLITUDE, 
				endAmplitude )), "SetBeepVoiceParameter AMP");
				
/* Set Envelope Rate to desired rate. */
		CALL_CHECK( (result = SetBeepVoiceParameter( channelNum, BMVP_AMPLITUDE_RATE, 
				rate )), "SetBeepVoiceParameter AMP Rate");
clean:
	return result;
}

/*******************************************************
**  tSetupSampleStart - Set up Beep channel start based on AIFF info.
*/
Err tSetupSampleStart( SampleInfo *smpi,  int32 channelNum, int32 signal )
{
	Item result;
	int32 sampleWidth;
	uint32 flags;

/* Configure audio playback channel based on sample compression type. */
	if( smpi->smpi_CompressionType == ID_SQS2 )
	{
		sampleWidth = 1;
		flags = BEEP_F_CHAN_CONFIG_SQS2 | BEEP_F_CHAN_CONFIG_8BIT;
	}
	else if( smpi->smpi_CompressionType == 0 )
	{
		sampleWidth = 2;
		flags = 0;
	}
	else
	{
		ERR(("tSetupSampleStart: unsupported format.\n"));
		return -1;
	}
	DBUG(("tSetupSampleStart: sampleWidth = %d, flags = 0x%d\n", sampleWidth, flags ));
	CALL_CHECK( (result = ConfigureBeepChannel( channelNum, flags )), "ConfigureBeepChannel");

/* Point channel at this sample. */
	if( smpi->smpi_SustainBegin == -1)
	{
/* No loop. */
		CALL_CHECK( (result = SetBeepChannelData( channelNum,
			smpi->smpi_Data, smpi->smpi_NumFrames )), "SetBeepChannelData");
	}
	else
	{
/* Sustain loop. */
		CALL_CHECK( (result = SetBeepChannelData( channelNum,
			smpi->smpi_Data, smpi->smpi_SustainEnd )), "SetBeepChannelData");
		CALL_CHECK( (result = SetBeepChannelDataNext( channelNum,
			((char *)smpi->smpi_Data) + smpi->smpi_SustainBegin*sampleWidth,
			smpi->smpi_SustainEnd - smpi->smpi_SustainBegin,
			BEEP_F_IF_GO_FOREVER, signal )), "SetBeepChannelDataNext");
	}

clean:
	return result;
}

/*******************************************************
**  tSetupSampleRelease - Set up Beep channel based on AIFF info.
*/
Err tSetupSampleRelease( SampleInfo *smpi,  int32 channelNum, int32 signal )
{
	Item result = 0;
	int32 sampleWidth = 2;

	if( smpi->smpi_CompressionType == ID_SQS2 )
	{
		sampleWidth = 1;
	}

/* Point channel at this sample. */
	if( smpi->smpi_SustainBegin )
	{
/* Play decay portion. */
		CALL_CHECK( (result = SetBeepChannelDataNext( channelNum,
			((char *)smpi->smpi_Data) + smpi->smpi_SustainEnd*sampleWidth,
			smpi->smpi_NumFrames - smpi->smpi_SustainEnd,
			0, signal )), "SetBeepChannelDataNext");
	}

clean:
	return result;
}
