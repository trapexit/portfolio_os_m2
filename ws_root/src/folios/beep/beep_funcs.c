/*******************************************************
**
** @(#) beep_funcs.c 96/03/01 1.11
**
** Beep folio basic functions.
**
** Author: Phil Burk
** Copyright 1995 3DO
** All Rights Reserved
*******************************************************/

#include <beep/beep.h>
#include <audio/handy_macros.h>
#include <kernel/types.h>
#include <kernel/debug.h>       /* print_vinfo() */
#include <kernel/tags.h>        /* tag iteration */
#include <kernel/kernel.h>
#include <kernel/task.h>
#include <kernel/internalf.h>
#include <dspptouch/dspp_touch.h>
#include "beep_internal.h"

#define DBUG(x)    /* PRT(x) */

#define CHANNEL_RANGE_CHECK(chan) \
	if( (chan < 0) || (chan >= BEEP_NUM_CHANNELS) return BEEP_ERR_CHANNEL_RANGE;

/* -------------------- Local Data */
static int16 gZeroData[DSPI_FIFO_MAX_DEPTH];  /* Used as source for DMA. */

static int32 beepConvertRealToInternal( uint32 ParameterID, float32 Value );

#define DSPP_GENERIC_SCALAR   (32768.0)
#define DSPP_SAMPLE_RATE      (44100.0)

/* ======================= BEGIN SIGNAL CONVERSION =================== */
static const struct {
	int32 min, max;
} RawLimits[AUDIO_SIGNAL_TYPE_MANY] = {        /* @@@ indexed by AUDIO_SIGNAL_TYPE_ */
	{ DSPP_MIN_SIGNED,   DSPP_MAX_SIGNED },     /* AUDIO_SIGNAL_TYPE_GENERIC_SIGNED */
	{ DSPP_MIN_UNSIGNED, DSPP_MAX_UNSIGNED },   /* AUDIO_SIGNAL_TYPE_GENERIC_UNSIGNED */
	{ DSPP_MIN_SIGNED,   DSPP_MAX_SIGNED },     /* AUDIO_SIGNAL_TYPE_OSC_FREQ */
	{ DSPP_MIN_SIGNED,   DSPP_MAX_SIGNED },     /* AUDIO_SIGNAL_TYPE_LFO_FREQ */
	{ DSPP_MIN_UNSIGNED, DSPP_MAX_UNSIGNED },   /* AUDIO_SIGNAL_TYPE_SAMPLE_RATE */
	{ DSPP_MIN_SIGNED,   DSPP_MAX_SIGNED },     /* AUDIO_SIGNAL_TYPE_WHOLE */
};

int32 dsphClipRawValue (int32 signalType, int32 rawValue)
{
	return CLIPRANGE (rawValue, RawLimits[signalType].min, RawLimits[signalType].max);
}

/************************************************************************************************/
float32 dsphConvertSignalToGeneric (int32 signalType, int32 rateDivide, float32 signalValue, float32 SampleRate)
{
	float32 genericValue;

	if (!rateDivide) {
		ERR(("dsphConvertSignalToGeneric: bad rate divide (%d)\n", rateDivide));
		return 0.0;
	}

	switch(signalType)
	{
		case AUDIO_SIGNAL_TYPE_GENERIC_SIGNED:
		case AUDIO_SIGNAL_TYPE_GENERIC_UNSIGNED:
			genericValue = signalValue;
			break;

		case AUDIO_SIGNAL_TYPE_SAMPLE_RATE:
			genericValue = signalValue * rateDivide / SampleRate;
			break;

		case AUDIO_SIGNAL_TYPE_OSC_FREQ:
			genericValue = signalValue * (rateDivide * 2) / SampleRate;
			break;

		case AUDIO_SIGNAL_TYPE_LFO_FREQ:
			genericValue = signalValue * (rateDivide * 2 * 256) / SampleRate;
			break;

		case AUDIO_SIGNAL_TYPE_WHOLE_NUMBER:
			genericValue = signalValue / DSPP_GENERIC_SCALAR;
			break;

		default:
			ERR(("dsphConvertSignalToGeneric: bad signal type (%d)\n", signalType));
			genericValue = 0.0;
			break;
	}

DBUG(("dsphConvertSignalToGeneric: signalValue = %g, genericValue = %g\n", signalValue, genericValue ));
	return genericValue;
}


/* ======================= END SIGNAL CONVERSION ===================== */
/*******************************************************
** SetBeepParameter - set parameter in DSP
*/
static int32 beepValidateParamID( uint32 ParameterID )
{
	uint32 ParamKey;
	uint32 machineIndex;

DBUG(("beepValidateParamID: ParameterID = 0x%x\n", ParameterID ));

/* Is this a Beep Machine parameter? */
	ParamKey = (ParameterID >> BEEP_ID_KEY_SHIFT) & BEEP_ID_KEY_MASK;
	if( ParamKey != BEEP_ID_VALID_KEY )
	{
		ERR(("beepValidateParamID: Bad ParamKey = 0x%x\n", ParamKey));
		return BEEP_ERR_INVALID_PARAM;
	}

/* Is it for the right machine? */
	machineIndex = (ParameterID >> BEEP_ID_MACHINE_SHIFT) & BEEP_ID_MACHINE_MASK  ;
	if( machineIndex != BB_FIELD(bf_Machine)->bm_Info.bminfo_MachineID )
	{
		ERR(("beepValidateParamID: Bad index = 0x%x\n", machineIndex));
		return BEEP_ERR_INVALID_PARAM;  /* Different error. */
	}
	return (ParameterID >> BEEP_ID_INDEX_SHIFT) & BEEP_ID_INDEX_MASK ;
}

/*******************************************************
** beepCalcParamAddress - range check voice
*/
static int32 beepCalcParamAddress( uint32 voiceNum, uint32 paramIndex)
{
	uint32 voiceBase;
	uint32 voiceBaseNext;
	uint16 *voiceBases;
	voiceBases = BB_FIELD(bf_Machine)->bm_VoiceDataOffsets;
	voiceBase = voiceBases[voiceNum];
	voiceBaseNext = voiceBases[voiceNum+1];
/* Make sure index is within range. */
	if( paramIndex > (voiceBaseNext - voiceBase)) return BEEP_ERR_INVALID_PARAM;
	return voiceBase + paramIndex;
}

/*******************************************************
** beepConvertRealToInternal - convert and range clip
*/
static int32 beepConvertRealToInternal( uint32 ParameterID, float32 Value )
{
	int32 signalType;
	int32 rateDivide;
	float32 rawValue;
	int32 intValue;

	signalType = (ParameterID >> BEEP_ID_SIGTYPE_SHIFT) & BEEP_ID_SIGTYPE_MASK;
	rateDivide = 1 << ((ParameterID >> BEEP_ID_CALCRATE_SHIFT) & BEEP_ID_CALCRATE_MASK);
	rawValue = dsphConvertSignalToGeneric ( signalType, rateDivide, Value, DSPP_SAMPLE_RATE );
	intValue = dsphClipRawValue ( signalType, rawValue * DSPP_GENERIC_SCALAR);
	return intValue;
}

/*******************************************************
******************* SWIs *******************************
*******************************************************/

 /**
 |||	AUTODOC -class Beep -name SetBeepParameter
 |||	Set a global Beep parameter.
 |||
 |||	  Synopsis
 |||
 |||	    Err SetBeepParameter ( uint32 ParameterID, float32 Value )
 |||
 |||	  Description
 |||
 |||	    Set a global Beep parameter.
 |||
 |||	  Arguments
 |||
 |||	    ParameterID
 |||	        Identifies parameter.  Defined in the beep machines include file.
 |||
 |||	    Value
 |||	        Value to set parameter to.  See Beep Machine document for
 |||	        information on the range of this Value.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error code
 |||	    (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    V30.
 |||
 |||	  Associated Files
 |||
 |||	    <beep/beep.h>, System.m2/Modules/beep
 |||
 |||	  Module Open Requirements
 |||
 |||	    OpenBeepFolio()
 |||
 |||	  See Also
 |||
 |||	    SetBeepVoiceParameter()
 **/
Err swiSetBeepParameter( uint32 ParameterID, float32 Value )
{
	int32 paramAddress;
	int32 intValue;

DBUG(("swiSetBeepParameter: ParamID = 0x%x, val = %g\n", ParameterID, Value ));
	CHECK_MACHINE_LOADED;

	paramAddress = beepValidateParamID(ParameterID);
DBUG(("swiSetBeepParameter: paramAddress = 0x%x\n", paramAddress ));
	if( paramAddress < 0 ) return paramAddress;
	intValue = beepConvertRealToInternal( ParameterID, Value );
DBUG(("swiSetBeepParameter: intValue = 0x%x\n", intValue ));
	dsphWriteDataMem( paramAddress, intValue );
	return 0;
}

 /**
 |||	AUTODOC -class Beep -name SetBeepVoiceParameter
 |||	Set a Beep voice parameter.
 |||
 |||	  Synopsis
 |||
 |||	    Err SetBeepVoiceParameter ( uint32 voiceNum,
 |||	                uint32 parameterID, float32 value  )
 |||
 |||	  Description
 |||
 |||	    Set a Beep parameter specific to a single voice.
 |||
 |||	  Arguments
 |||
 |||	    voiceNum
 |||	        Voice index. See Beep Machine document for
 |||	        information on the different voices available.
 |||
 |||	    parameterID
 |||	        Identifies parameter.  Defined in the beep machines include file.
 |||
 |||	    value
 |||	        Value to set parameter to.  See Beep Machine document for
 |||	        information on the range of this Value.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error code
 |||	    (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    V30.
 |||
 |||	  Associated Files
 |||
 |||	    <beep/beep.h>, System.m2/Modules/beep
 |||
 |||	  Module Open Requirements
 |||
 |||	    OpenBeepFolio()
 |||
 |||	  See Also
 |||
 |||	    SetBeepParameter()
 **/

Err lowSetBeepVoiceParameter( uint32 voiceNum, uint32 ParameterID, int16 intValue  )
{
	int32 paramIndex, paramAddress;

	CHECK_VOICE_RANGE(voiceNum);

	paramIndex = beepValidateParamID(ParameterID);
DBUG(("lowSetBeepVoiceParameter: paramIndex = 0x%x, intValue = 0x%x\n", paramIndex, intValue ));
	if( paramIndex < 0 ) return paramIndex;

	paramAddress = beepCalcParamAddress(voiceNum, paramIndex);
DBUG(("lowSetBeepVoiceParameter: paramAddress = 0x%x\n", paramAddress ));

	dsphWriteDataMem( paramAddress, intValue );

	return 0;
}

Err swiSetBeepVoiceParameter( uint32 voiceNum, uint32 ParameterID, float32 Value  )
{
	int16 intValue;

DBUG(("swiSetBeepVoiceParameter: vnum = %d, ParamID = 0x%x, val = %g\n",
		voiceNum, ParameterID, Value ));
	CHECK_MACHINE_LOADED;

	intValue = beepConvertRealToInternal( ParameterID, Value );

	return lowSetBeepVoiceParameter(voiceNum, ParameterID, intValue );
}

 /**
 |||	AUTODOC -class Beep -name ConfigureBeepChannel
 |||	Configure a DMA channel.
 |||
 |||	  Synopsis
 |||
 |||	    Err ConfigureBeepChannel( uint32 channelNum, uint32 flags )
 |||
 |||	  Description
 |||
 |||	    Configure a Beep Machine DMA channel.  This is used to
 |||	    control decompression and sample width.
 |||
 |||	  Arguments
 |||
 |||	    channelNum
 |||	        Channel index. See Beep Machine document for
 |||	        information on the different channels available.
 |||	        The channelNum must be less than BEEP_NUM_CHANNELS.
 |||	        Note that only sample playing voices have an associated
 |||	        DMA channel.          
 |||
 |||	    flags
 |||	        OR together these flags to configure DMA channel:
 |||
 |||	        BEEP_F_CHAN_CONFIG_8BIT = 8 bit data
 |||
 |||	        BEEP_F_CHAN_CONFIG_SQS2 = do SQS2 decompression
 |||
 |||	        If you set BEEP_F_CHAN_CONFIG_SQS2 then you must
 |||	        also set BEEP_F_CHAN_CONFIG_8BIT.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error code
 |||	    (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    V30.
 |||
 |||	  Associated Files
 |||
 |||	    <beep/beep.h>, System.m2/Modules/beep
 |||
 |||	  Module Open Requirements
 |||
 |||	    OpenBeepFolio()
 |||
 |||	  See Also
 |||
 |||	    StartBeepChannel()
 **/

Err swiConfigureBeepChannel( uint32 ChannelNum, uint32 Flags )
{
DBUG(("swiConfigureBeepChannel: ChannelNum = 0x%x, Flags = 0x%x\n", ChannelNum, Flags ));
	CHECK_MACHINE_LOADED;
	CHECK_CHANNEL_RANGE(ChannelNum);
	if( (Flags & BEEP_F_CHAN_CONFIG_SQS2) &&
	    !(Flags & BEEP_F_CHAN_CONFIG_8BIT) )
	{
		return BEEP_ERR_INVALID_CONFIGURATION;
	}
	dsphSetChannelDecompression( ChannelNum, (Flags & BEEP_F_CHAN_CONFIG_SQS2) );
	dsphSetChannel8Bit( ChannelNum, (Flags & BEEP_F_CHAN_CONFIG_8BIT) );
	return 0;
}

 /**
 |||	AUTODOC -class Beep -name SetBeepChannelData
 |||	Specify data for a DMA channel.
 |||
 |||	  Synopsis
 |||
 |||	    Err SetBeepChannelData( uint32 channelNum, void *addr, int32 numSamples )
 |||
 |||	  Description
 |||
 |||	    Specify audio sample data block for a given DMA channel.
 |||	    When the channel is started, this data will be played.
 |||	    Once started, the data block must be reset with this function
 |||	    in order to replay the same data.
 |||
 |||	  Arguments
 |||
 |||	    channelNum
 |||	        Channel index. See Beep Machine document for
 |||	        information on the different channels available.
 |||	        The channelNum must be less than BEEP_NUM_CHANNELS.
 |||	        Note that only sample playing voices have an associated
 |||	        DMA channel.          
 |||
 |||	    addr
 |||	        Address of first data sample. 16 bit data must be 2 byte aligned.
 |||	        8 bit data can be byte aligned.
 |||
 |||	    numSamples
 |||	        Number of samples in the block. Note that for 16 bit data this
 |||	        is the number of bytes divided by 2.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error code
 |||	    (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    V30.
 |||
 |||	  Associated Files
 |||
 |||	    <beep/beep.h>, System.m2/Modules/beep
 |||
 |||	  Module Open Requirements
 |||
 |||	    OpenBeepFolio()
 |||
 |||	  See Also
 |||
 |||	    StartBeepChannel(), SetBeepChannelDataNext()
 **/

/*******************************************************
** SetBeepChannelData - Specify the audio data to be played
** on a particular channel.  Stops Channel.
*/
Err swiSetBeepChannelData( uint32 ChannelNum, void *Addr, int32 NumSamples )
{
	int32 Result;

DBUG(("swiSetBeepChannelData(ChannelNum = 0x%x, Addr = 0x%x, nums = 0x%x\n",
		ChannelNum, Addr, NumSamples ));

	CHECK_MACHINE_LOADED;
	CHECK_CHANNEL_RANGE(ChannelNum);
/* Note that for 16 bit samples, this will not check the entire range
** but it will prevent hardware access. */
	CHECK_VALID_RAM( Addr, NumSamples ); 

	Result = swiStopBeepChannel( ChannelNum );
	if( Result < 0 ) return Result;
	DBUG(("swiSetBeepChannelData: SetDMA ........\n"));
	dsphSetInitialDMA ( ChannelNum, Addr, NumSamples );
	swiSetBeepChannelDataNext( ChannelNum, gZeroData, DSPI_FIFO_MAX_DEPTH,
		0, 0);
	DBUG(("swiSetBeepChannelData: DONE\n"));
	return 0;
}

 /**
 |||	AUTODOC -class Beep -name SetBeepChannelDataNext
 |||	Specify data to play after current data finishes.
 |||
 |||	  Synopsis
 |||
 |||	    Err SetBeepChannelDataNext( uint32 channelNum, void *addr, int32 numSamplesm,
 |||	             uint32 flags, int32 Signal )
 |||
 |||	  Description
 |||
 |||	    Specify audio sample data block to be played when the current block
 |||	    finishes.  This is used to set up sample loops for continuous sound.
 |||	    It can also be used to chain sound buffers for spooling off of disc.
 |||	    Note that if you call this routine a second time, before
 |||	    the first data block has begun playing, then the first
 |||	    call will be ignored.  To chain multiple sound buffers
 |||	    you must wait for each block to be started before
 |||	    calling SetBeepChannelDataNext() again. See
 |||	    Examples/Audio/Beep/ta_spool.c for an example. 
 |||
 |||	  Arguments
 |||
 |||	    channelNum
 |||	        Channel index. See Beep Machine document for
 |||	        information on the different channels available.
 |||	        The channelNum must be less than BEEP_NUM_CHANNELS.
 |||	        Note that only sample playing voices have an associated
 |||	        DMA channel.          
 |||
 |||	    addr
 |||	        Address of first data sample. 16 bit data must be 2 byte aligned.
 |||	        8 bit data can be byte aligned.
 |||
 |||	    numSamples
 |||	        Number of samples in the block. Note that for 16 bit data this
 |||	        is the number of bytes divided by 2.
 |||
 |||	    flags
 |||	        Flags for controlling sample playback.
 |||
 |||	        BEEP_F_IF_GO_FOREVER = play this block over and over.
 |||	        If not set then it will play this block once and stop.
 |||
 |||	    signal
 |||	        Request that the calling task be sent this signal when this
 |||	        block has begun playing.  At that time you may specify a
 |||	        new block to be played after this one finishes.  This can
 |||	        be used to spool multiple blocks of data.
 |||
 |||	        You can disable the request by passing zero for the value of Signal.
 |||	        One can pass Addr = NULL to cancel a DataNext request.  The DMA
 |||	        will then stop after the current block.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error code
 |||	    (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    V30.
 |||
 |||	  Associated Files
 |||
 |||	    <beep/beep.h>, System.m2/Modules/beep
 |||
 |||	  Module Open Requirements
 |||
 |||	    OpenBeepFolio()
 |||
 |||	  See Also
 |||
 |||	    StartBeepChannel(), SetBeepChannelData() Examples/Audio/Beep/ta_spool.c
 **/

Err swiSetBeepChannelDataNext( uint32 ChannelNum, void *Addr, int32 NumSamples, uint32 Flags, int32 Signal )
{
	Err    result = 0;
	uint32 dmaFlags = 0;

DBUG(("swiSetBeepChannelDataNext(ChannelNum = 0x%x, Addr = 0x%x, nums = 0x%x)\n",
		ChannelNum, Addr, NumSamples ));

	CHECK_MACHINE_LOADED;
	CHECK_CHANNEL_RANGE(ChannelNum);

/* Check Flags. */
	if( Flags & ~BEEP_F_IF_GO_FOREVER )
	{
		return BEEP_ERR_ILLEGAL_FLAG;
	}

	if( Addr == NULL )
	{
/* Clear NextValid flag in hardware. */
		dsphSetNextDMA ( ChannelNum, Addr, NumSamples, dmaFlags );
		return BEEP_ERR_UNIMPLEMENTED;
	}
	else
	{
		uint32 intState;
		CHECK_VALID_RAM( Addr, NumSamples );
		intState = Disable();
		BB_FIELD(bf_Signals)[ChannelNum] = Signal;
		if(Signal)
		{
			BB_FIELD(bf_TasksToSignal)[ChannelNum] = CURRENTTASKITEM;
			dmaFlags |= DSPH_F_DMA_INT_ENABLE; /* Enable interrupt. */
		}
		else
		{
			BB_FIELD(bf_TasksToSignal)[ChannelNum] = 0;
		}
		dmaFlags |= ( Flags & BEEP_F_IF_GO_FOREVER ) ? DSPH_F_DMA_LOOP : 0;
		dsphSetNextDMA ( ChannelNum, Addr, NumSamples, dmaFlags );
		Enable(intState);
	}
	return result;
}

/* HACK because SWIs don't support 5 parameters!  */
Err swiHackBeepChannelDataNext( uint32 *params )
{
	return swiSetBeepChannelDataNext( (uint32) *params++, (void *) *params++,
		(int32) *params++, (uint32) *params++, (int32) *params );
}

Err SetBeepChannelDataNext( uint32 ChannelNum, void *Addr, int32 NumSamples, uint32 Flags, int32 Signal )
{
	uint32 params[5];
	params[0] = (uint32) ChannelNum;
	params[1] = (uint32) Addr;
	params[2] = (uint32) NumSamples;
	params[3] = (uint32) Flags;
	params[4] = (uint32) Signal;
	return HackBeepChannelDataNext( params );
}

 /**
 |||	AUTODOC -class Beep -name StartBeepChannel
 |||	Start a DMA channel.
 |||
 |||	  Synopsis
 |||
 |||	    Err StartBeepChannel( uint32 channelNum )
 |||
 |||	  Description
 |||
 |||	    Enable DMA so that the current sample block begins playing.
 |||	    If DMA has been disabled by calling StopBeepChannel(), then
 |||	    the playback will resume where it left off.  To restart the
 |||	    sample at the beginning, you must call SetBeepChannelData().
 |||
 |||	  Arguments
 |||
 |||	    channelNum
 |||	        Channel index. See Beep Machine document for
 |||	        information on the different channels available.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error code
 |||	    (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    V30.
 |||
 |||	  Associated Files
 |||
 |||	    <beep/beep.h>, System.m2/Modules/beep
 |||
 |||	  Module Open Requirements
 |||
 |||	    OpenBeepFolio()
 |||
 |||	  See Also
 |||
 |||	    StopBeepChannel(), SetBeepChannelData()
 **/

Err swiStartBeepChannel( uint32 ChannelNum )
{
DBUG(("swiStartBeepChannel(ChannelNum = 0x%x)\n", ChannelNum ));
	CHECK_MACHINE_LOADED;
	CHECK_CHANNEL_RANGE(ChannelNum);

	dsphEnableDMA( ChannelNum );
	return 0;
}
 /**
 |||	AUTODOC -class Beep -name StopBeepChannel
 |||	Stop a DMA channel.
 |||
 |||	  Synopsis
 |||
 |||	    Err StopBeepChannel( uint32 channelNum )
 |||
 |||	  Description
 |||
 |||	    Disable DMA on the specified channel.
 |||
 |||	  Arguments
 |||
 |||	    channelNum
 |||	        Channel index. See Beep Machine document for
 |||	        information on the different channels available.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error code
 |||	    (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    V30.
 |||
 |||	  Associated Files
 |||
 |||	    <beep/beep.h>, System.m2/Modules/beep
 |||
 |||	  Module Open Requirements
 |||
 |||	    OpenBeepFolio()
 |||
 |||	  See Also
 |||
 |||	    StartBeepChannel(), SetBeepChannelData()
 **/

Err swiStopBeepChannel( uint32 ChannelNum )
{
DBUG(("swiStopBeepChannel(ChannelNum = 0x%x)\n", ChannelNum ));
	CHECK_MACHINE_LOADED;
	CHECK_CHANNEL_RANGE(ChannelNum);

	dsphDisableDMA( ChannelNum );
	return 0;
}
 /**
 |||	AUTODOC -class Beep -name GetBeepTime
 |||	Return the current Beep time.
 |||
 |||	  Synopsis
 |||
 |||	    uint32 GetBeepTime( void )
 |||
 |||	  Description
 |||
 |||	    Return the current Beep time. This is a count of the number of
 |||	    frames that have elapsed since the Beep Folio was first started.
 |||	    Each frame corresponds to the output of a stereo pair at 44100 Hz.
 |||	    Note that this is a 32 bit quantity that will wrap around
 |||	    approximately every 27 hours.
 |||
 |||	  Arguments
 |||
 |||	    none
 |||
 |||	  Return Value
 |||
 |||	    BeepTime in as an unsigned frame count.
 |||
 |||	  Implementation
 |||
 |||	    V30.
 |||
 |||	  Associated Files
 |||
 |||	    <beep/beep.h>, System.m2/Modules/beep
 |||
 |||	  Module Open Requirements
 |||
 |||	    OpenBeepFolio()
 |||
 |||	  See Also
 |||
 |||	    GetAudioTime()
 **/

uint32 swiGetBeepTime( void )
{
	return ReadHardware(DSPX_FRAME_UP_COUNTER);
}
