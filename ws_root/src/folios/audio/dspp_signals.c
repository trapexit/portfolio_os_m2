/* @(#) dspp_signals.c 96/02/20 1.13 */
/****************************************************************
**
** DSPP Signal Conversion
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
*****************************************************************
****************************************************************/

#include "audio_internal.h"
#include "dspp.h"               /* self */

#define DBUG(x)    /* PRT(x) */

#define DSPP_GENERIC_SCALAR   (32768.0)

/*****************************************************************
** Nomenclature:
**   Signal = Floating Point in signal units, ie. Hertz
**   Generic = Floating Point in signal with range = -1.0 to 1.0, or 0.0 to 2.0
**   Raw = Integer DSP raw value range = -0x8000 to 0x7FFF, or 0 to 0xFFFF
*****************************************************************/

static const struct {
	int32 min, max;
} RawLimits[AF_SIGNAL_TYPE_MANY] = {        /* @@@ indexed by AF_SIGNAL_TYPE_ */
	{ DSPP_MIN_SIGNED,   DSPP_MAX_SIGNED },     /* AF_SIGNAL_TYPE_GENERIC_SIGNED */
	{ DSPP_MIN_UNSIGNED, DSPP_MAX_UNSIGNED },   /* AF_SIGNAL_TYPE_GENERIC_UNSIGNED */
	{ DSPP_MIN_SIGNED,   DSPP_MAX_SIGNED },     /* AF_SIGNAL_TYPE_OSC_FREQ */
	{ DSPP_MIN_SIGNED,   DSPP_MAX_SIGNED },     /* AF_SIGNAL_TYPE_LFO_FREQ */
	{ DSPP_MIN_UNSIGNED, DSPP_MAX_UNSIGNED },   /* AF_SIGNAL_TYPE_SAMPLE_RATE */
	{ DSPP_MIN_SIGNED,   DSPP_MAX_SIGNED },     /* AF_SIGNAL_TYPE_WHOLE */
};


/*****************************************************************/
/**
|||	AUTODOC -public -class audio -group Signal -name GetAudioSignalInfo
|||	Get information about audio signal types.
|||
|||	  Synopsis
|||
|||	    Err GetAudioSignalInfo (AudioSignalInfo *info, uint32 infoSize,
|||	                            uint32 signalType)
|||
|||	  Description
|||
|||	    This function returns range and precision information for a specified audio
|||	    signal type. The information is returned in an AudioSignalInfo structure.
|||
|||	    The AudioSignalInfo structure contains the following fields:
|||
|||	    sinfo_Min
|||	        Minimum legal value for the signal type.
|||
|||	    sinfo_Max
|||	        Maximum legal value for the signal type.
|||
|||	    sinfo_Precision
|||	        The finest amount by which a signal of this signal type can be changed.
|||
|||	  Arguments
|||
|||	    info
|||	        A pointer to an AudioSignalInfo structure where the information will
|||	        be stored.
|||
|||	    infoSize
|||	        The size in bytes of the AudioSignalInfo structure.
|||
|||	    signalType
|||	        The signal type to query. This must be one of the AF_SIGNAL_TYPE_ values
|||	        defined in <audio/audio.h>.
|||
|||	  Return Value
|||
|||	    Non-negative if successful or an error code (a negative value) if an error
|||	    occurs. On failure, the contents of info are left unchanged.
|||
|||	  Implementation
|||
|||	    Folio call implemented in audio folio V30.
|||
|||	  Caveats
|||
|||	    Frequency amounts returned by this function are for instruments running at
|||	    full execution rate. Divide these values by the AF_TAG_CALCRATE_DIVIDE
|||	    parameter to compute the correct value for instruments running at other
|||	    execution rates. This applies to AF_SIGNAL_TYPE_OSC_FREQ,
|||	    AF_SIGNAL_TYPE_LFO_FREQ, and AF_SIGNAL_TYPE_SAMPLE_RATE.
|||
|||	    A simple way to do execution rate conversion is:
|||
|||	    ConvertGenericToAudioSignal (<signal type>, <rate divide>,
|||	    ConvertAudioSignalToGeneric (<signal type>, 1, <value>));
|||
|||	  Associated Files
|||
|||	    <audio/audio.h>, System.m2/Modules/audio
|||
|||	  See Also
|||
|||	    ConvertAudioSignalToGeneric(), ConvertGenericToAudioSignal(),
|||	    GetInstrumentPortInfoByIndex(), GetInstrumentPortInfoByName()
**/

Err GetAudioSignalInfo (AudioSignalInfo *resultInfo, uint32 resultInfoSize, uint32 signalType)
{
	AudioSignalInfo info;

		/* init info */
	memset (&info, 0, sizeof info);

		/* validate signal type */
	if (signalType > AF_SIGNAL_TYPE_MAX) return AF_ERR_BAD_SIGNAL_TYPE;

		/* fill out info */
	info.sinfo_Min       = dsppGetSignalMin (signalType, 0);
	info.sinfo_Max       = dsppGetSignalMax (signalType, 0);
	info.sinfo_Precision = dsppConvertRawToSignal (signalType, 0, 1);

		/* success: copy to client's buffer */
	memset (resultInfo, 0, resultInfoSize);
	memcpy (resultInfo, &info, MIN (resultInfoSize, sizeof info));

	return 0;
}


/**
|||	AUTODOC -private -class audio -group Signal -name dsppClipRawValue
|||	Clips raw (1.15 fixed point) value into range for the specified signal type.
|||
|||	  Synopsis
|||
|||	    int32 dsppClipRawValue (int32 signalType, int32 rawValue)
|||
|||	  Description
|||
|||	    Clips raw (1.15 fixed point) value into range for the specified signal type.
|||
|||	  Arguments
|||
|||	    signalType
|||	        AF_SIGNAL_TYPE_ value. Assumed to be valid.
|||
|||	    rawValue
|||	        Raw value to clip into range.
|||
|||	  Return Value
|||
|||	    Raw value clipped into range.
|||
|||	  Implementation
|||
|||	    Folio call implemented in audio folio V30.
|||
|||	  Associated Files
|||
|||	    <audio/dspp_template.h>, System.m2/Modules/audio
|||
|||	  See Also
|||
|||	    GetAudioSignalInfo(), ConvertAudioSignalToGeneric(),
|||	    ConvertGenericToAudioSignal()
**/
int32 dsppClipRawValue (int32 signalType, int32 rawValue)
{
	return CLIPRANGE (rawValue, RawLimits[signalType].min, RawLimits[signalType].max);
}

/******************************************************************/
float32 dsppGetSignalMin( int32 signalType, int32 rateShift )
{
	return dsppConvertRawToSignal (signalType, rateShift, RawLimits[signalType].min);
}

/******************************************************************/
float32 dsppGetSignalMax( int32 signalType, int32 rateShift )
{
	return dsppConvertRawToSignal (signalType, rateShift, RawLimits[signalType].max);
}

/******************************************************************/
/**
|||	AUTODOC -public -class audio -group Signal -name ConvertAudioSignalToGeneric
|||	Converts audio signal value of specified signal type to generic signal value.
|||
|||	  Synopsis
|||
|||	    float32 ConvertAudioSignalToGeneric (int32 signalType, int32 rateDivide,
|||	                                         float32 signalValue)
|||
|||	  Description
|||
|||	    This function converts a signal value of the specified signal type (e.g.,
|||	    oscillator frequency in Hertz) to a generic signal value (e.g., signed
|||	    floating point value in the range of -1.0 to 1.0).
|||
|||	    Certain signal types are affected by the execution rate division of the
|||	    associated instrument:
|||
|||	    - AF_SIGNAL_TYPE_SAMPLE_RATE
|||
|||	    - AF_SIGNAL_TYPE_OSC_FREQ
|||
|||	    - AF_SIGNAL_TYPE_LFO_FREQ
|||
|||	    (see description of the Instrument(@) AF_TAG_CALCRATE_DIVIDE tag)
|||
|||	    This function does no boundary checking or signal value clipping.
|||
|||	  Arguments
|||
|||	    signalType
|||	        The signal type of the signal to convert.
|||
|||	    rateDivide
|||	        The execution rate denominator for the signal (e.g., 1=full rate,
|||	        2=half rate, etc).
|||
|||	    signalValue
|||	        Signal value (in the units appropriate for signalType) to convert.
|||
|||	  Return Value
|||
|||	    Generic value in the range of -1.0 to 1.0 (for signed signals) or 0 to 2.0
|||	    (for unsigned signals). Returns 0 when an undefined signalType or rateDivide
|||	    of 0 is specified.
|||
|||	  Implementation
|||
|||	    Folio call implemented in audio folio V30.
|||
|||	  Associated Files
|||
|||	    <audio/audio.h>, System.m2/Modules/audio
|||
|||	  See Also
|||
|||	    ConvertGenericToAudioSignal(), GetAudioSignalInfo(), Instrument(@)
**/

float32 ConvertAudioSignalToGeneric (int32 signalType, int32 rateDivide, float32 signalValue)
{
	float32 genericValue;

	if (!rateDivide) {
		ERR(("ConvertAudioSignalToGeneric: bad rate divide (%d)\n", rateDivide));
		return 0.0;
	}

	switch(signalType)
	{
		case AF_SIGNAL_TYPE_GENERIC_SIGNED:
		case AF_SIGNAL_TYPE_GENERIC_UNSIGNED:
			genericValue = signalValue;
			break;

		case AF_SIGNAL_TYPE_SAMPLE_RATE:
			genericValue = signalValue * rateDivide / DSPPData.dspp_SampleRate;
			break;

		case AF_SIGNAL_TYPE_OSC_FREQ:
			genericValue = signalValue * (rateDivide * 2) / DSPPData.dspp_SampleRate;
			break;

		case AF_SIGNAL_TYPE_LFO_FREQ:
			genericValue = signalValue * (rateDivide * 2 * 256) / DSPPData.dspp_SampleRate;
			break;

		case AF_SIGNAL_TYPE_WHOLE_NUMBER:
			genericValue = signalValue / DSPP_GENERIC_SCALAR;
			break;

		default:
			ERR(("ConvertAudioSignalToGeneric: bad signal type (%d)\n", signalType));
			genericValue = 0.0;
			break;
	}

DBUG(("ConvertAudioSignalToGeneric: signalValue = %g, genericValue = %g\n", signalValue, genericValue ));
	return genericValue;
}


/******************************************************************/
float32 dsppConvertRawToSignal( int32 signalType, int32 rateShift, int32 rawValue )
{
    return dsppConvertGenericToSignal (signalType, rateShift, dsppConvertRawToGeneric (signalType, rawValue));
}

/******************************************************************/
float32 dsppConvertRawToGeneric( int32 signalType, int32 rawValue )
{
	float32 genericValue = 0.0;

	switch(signalType)
	{
/* Signed types. */
		case AF_SIGNAL_TYPE_GENERIC_SIGNED:
		case AF_SIGNAL_TYPE_OSC_FREQ:
		case AF_SIGNAL_TYPE_LFO_FREQ:
		case AF_SIGNAL_TYPE_WHOLE_NUMBER:
			genericValue = ((int32) ((int16) rawValue)) / DSPP_GENERIC_SCALAR; /* Sign extend 16 bit value. */
			break;

/* UNsigned types. */
		case AF_SIGNAL_TYPE_SAMPLE_RATE:
		case AF_SIGNAL_TYPE_GENERIC_UNSIGNED:
			genericValue = ((uint32) ((uint16) rawValue)) / DSPP_GENERIC_SCALAR;
			break;

		default:
			ERR(("dsppConvertRawToGeneric: bad knob type = 0x%x\n", signalType));
			break;
	}

DBUG(("dsppConvertRawToGeneric: rawValue = %g, genericValue = %g\n", rawValue, genericValue ));
	return genericValue;
}

/******************************************************************/
/**
|||	AUTODOC -public -class audio -group Signal -name ConvertGenericToAudioSignal
|||	Converts generic signal value to specified audio signal type.
|||
|||	  Synopsis
|||
|||	    float32 ConvertGenericToAudioSignal (int32 signalType, int32 rateDivide,
|||	                                         float32 genericValue)
|||
|||	  Description
|||
|||	    This function converts a generic signal value (e.g., signed floating point
|||	    value in the range of -1.0 to 1.0) to a signal value of the specified audio
|||	    signal type (e.g., oscillator frequency in Hertz).
|||
|||	    Certain signal types are affected by the execution rate division of the
|||	    associated instrument:
|||
|||	    - AF_SIGNAL_TYPE_SAMPLE_RATE
|||
|||	    - AF_SIGNAL_TYPE_OSC_FREQ
|||
|||	    - AF_SIGNAL_TYPE_LFO_FREQ
|||
|||	    (see description of the Instrument(@) AF_TAG_CALCRATE_DIVIDE tag)
|||
|||	    This function does no boundary checking or signal value clipping.
|||
|||	  Arguments
|||
|||	    signalType
|||	        The signal type to convert the generic value to.
|||
|||	    rateDivide
|||	        The execution rate denominator for the signal (e.g., 1=full rate,
|||	        2=half rate, etc).
|||
|||	    genericValue
|||	        Generic value in the range of -1.0 to 1.0 (for signed signals) or 0 to
|||	        2.0 for unsigned signals) to convert.
|||
|||	  Return Value
|||
|||	    Audio signal value in the units appropraite for signalType. Returns 0 when
|||	    an undefined signalType or rateDivide of 0 is specified.
|||
|||	  Implementation
|||
|||	    Folio call implemented in audio folio V30.
|||
|||	  Associated Files
|||
|||	    <audio/audio.h>, System.m2/Modules/audio
|||
|||	  See Also
|||
|||	    ConvertAudioSignalToGeneric(), GetAudioSignalInfo(), Instrument(@)
**/

float32 ConvertGenericToAudioSignal (int32 signalType, int32 rateDivide, float32 genericValue)
{
	float32 signalValue;

	if (!rateDivide) {
		ERR(("ConvertGenericToAudioSignal: bad rate divide (%d)\n", rateDivide));
		return 0.0;
	}

	switch(signalType)
	{
		case AF_SIGNAL_TYPE_GENERIC_SIGNED:
		case AF_SIGNAL_TYPE_GENERIC_UNSIGNED:
			signalValue = genericValue;
			break;

		case AF_SIGNAL_TYPE_SAMPLE_RATE:
			signalValue = genericValue * DSPPData.dspp_SampleRate / rateDivide;
			break;

		case AF_SIGNAL_TYPE_OSC_FREQ:
			signalValue = genericValue * DSPPData.dspp_SampleRate / (rateDivide * 2);
			break;

		case AF_SIGNAL_TYPE_LFO_FREQ:  /* 940227 For extended precision oscillators. */
			signalValue = genericValue * DSPPData.dspp_SampleRate / (rateDivide * 2 * 256);
			break;

		case AF_SIGNAL_TYPE_WHOLE_NUMBER:
			signalValue = genericValue * DSPP_GENERIC_SCALAR;
			break;

		default:
			ERR(("ConvertGenericToAudioSignal: bad signal type (%d)\n", signalType));
			signalValue = 0.0;
			break;
	}

DBUG(("ConvertGenericToAudioSignal: genericValue = %g, signalValue = %g\n", genericValue, signalValue ));
	return signalValue;
}
