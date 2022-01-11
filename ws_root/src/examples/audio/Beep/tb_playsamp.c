/**
|||	AUTODOC -public -class examples -group Beep -name tb_playsamp
|||	Play a sample using the Beep Folio.
|||
|||	  Format
|||
|||	    tb_playsamp  {samplename}
|||
|||	  Description
|||
|||	    Play a sample using the Beep Folio. Up and Down buttons
|||	    cause program to switch to higher or lower DMA channels.
|||	    
|||	    A,B and C buttons can be played like a Trumpet.
|||	    RightShift button shifts up an octave.
|||
|||	    Tuning is based on the following 7 ratios:
|||	    1/1  9/8  5/4  4/3  3/2  5/3  7/4
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
Item  tInit( void );
Err   tTerm( Item  BeepMachine );

/* 7 note tuning based on just intonation */
static float32 gTuning[] =
{
	0.0, 1.0, (9.0/8.0), (5.0/4.0),
	(4.0/3.0), (3.0/2.0), (5.0/3.0), (7.0/4.0)
};
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
	float32 rate;
	int32   pitchIndex, oldPitch = 0;
	
	PRT(("%s - Press A,B,C buttons to play notes. RShift to double.\n", argv[0]));
	SampleName = (argc>1) ? argv[1] : "/remote/System.m2/audio/aiff/sinewave.aiff";
	if( (BeepMachine = tInit()) < 0) goto clean;

/* Print Beep Time for no particular reason. */
	PRT(( "Beep Time = 0x%x\n", GetBeepTime() ));

/* Get SampleInfo from AIFF file including the sample data */
	if ((result = GetAIFFSampleInfo (&smpi, SampleName, 0)) < 0) goto clean;

	while(doIt)
	{
		CALL_CHECK( (result = GetControlPad (1, TRUE, &cped)), "GetControlPad");
		buttons = cped.cped_ButtonBits;

/* Convert buttons to 3 bit index. */
		pitchIndex = (buttons & ControlC) ? 1 : 0;
		pitchIndex |= (buttons & ControlB) ? 2 : 0;
		pitchIndex |= (buttons & ControlA) ? 4 : 0;

		rate = 22000.0 * gTuning[pitchIndex];
		if( buttons & ControlRightShift ) rate *= 2.0; /* Octave shift */

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

/* Set Sample Rate */
		if( pitchIndex > 0)
		{
			CALL_CHECK( (result = SetBeepVoiceParameter( channelNum, BMVP_SAMPLE_RATE, 
				rate )), "SetBeepVoiceParameter SR");

			if( oldPitch == 0 )
			{
/* Set DMA to start playing sample. */
				CALL_CHECK( (result = tSetupSampleStart( smpi, channelNum, 0 )), "tSetupSample");
/* Start DMA. */
				CALL_CHECK( (result = StartBeepChannel( channelNum )), "StartBeepChannel");
			}
		}
		else
		{
			CALL_CHECK( (result = tSetupSampleRelease( smpi, channelNum, 0 )), "tSetupSample");
		}
		
		if( ifNewChannel )
		{
/* Set Amplitude */
			CALL_CHECK( (result = SetBeepVoiceParameter( oldChannel, BMVP_AMPLITUDE, 
				0.0 )), "SetBeepVoiceParameter AMP");
			CALL_CHECK( (result = SetBeepVoiceParameter( channelNum, BMVP_AMPLITUDE, 
				1.0 )), "SetBeepVoiceParameter AMP");
/* Set LFO Depth */
			CALL_CHECK( (result = SetBeepVoiceParameter( oldChannel, BMVP_FREQ_MOD_DEPTH, 
				0.0 )), "SetBeepVoiceParameter LFO Depth");
			CALL_CHECK( (result = SetBeepVoiceParameter( channelNum, BMVP_FREQ_MOD_DEPTH, 
				2000.0 )), "SetBeepVoiceParameter LFO Depth");
/* Set LFO Rate */
			CALL_CHECK( (result = SetBeepVoiceParameter( channelNum, BMVP_LFO_RATE, 
				2.0 )), "SetBeepVoiceParameter LFO Rate");

/* Stop DMA */
			CALL_CHECK( (result = StopBeepChannel( oldChannel )), "StopBeepChannel");
			oldChannel = channelNum;
			ifNewChannel = FALSE;
			PRT(("Channel %d\n", channelNum));
		}

		oldPitch = pitchIndex;
		if(buttons & ControlX & ~oldbuttons)
		{
			PRT(("ControlX hit. Buttons = 0x%x\n", buttons));
			doIt = FALSE;
		}
	}


/* Unload Sample*/
	DeleteSampleInfo (smpi);

/* Print Beep Time for no particular reason. */
	PRT(( "Beep Time = 0x%x\n", GetBeepTime() ));

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
