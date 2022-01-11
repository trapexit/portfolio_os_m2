/* @(#) sampleitemtoinsname.c 96/02/23 1.4 */

#include <audio/audio.h>
#include <audio/parse_aiff.h>   /* self */

#define DBUG(x)  /* printf x */

/**
|||	AUTODOC -public -class libmusic -group Sample -name SampleItemToInsName
|||	Builds the name of the DSP Instrument Template to play the Sample(@).
|||
|||	  Synopsis
|||
|||	    Err SampleItemToInsName (Item sample, bool ifVariable, char *nameBuffer,
|||	                             int32 nameBufferSize)
|||
|||	  Description
|||
|||	    This procedure builds the name of the appropriate instrument that will play
|||	    the given sample item. If there is no appropriate instrument to play the
|||	    sample, it returns an error code.
|||
|||	    This function queries the Sample for its sample rate, number of channels
|||	    (mono or stereo), sample width (8 or 16), and compression type, and then
|||	    calls SampleFormatToInsName() to get the correct instrument name. It also
|||	    lets you specify whether you need to vary the pitch or play only at the
|||	    original pitch.
|||
|||	  Arguments
|||
|||	    sample
|||	        The item number of the sample to be played.
|||
|||	    ifVariable
|||	        A value indicating whether or not the pitch is to vary. If TRUE, the
|||	        pitch will vary. If FALSE, the pitch won't vary.
|||
|||	        Setting this to FALSE does not guarantee that you will get a fixed-rate
|||	        instrument. This function picks an instrument capable of playing the
|||	        sample at the sample rate stored in the Sample Item. Since all of the
|||	        fixed-rate sample players play at a sample rate of 44100 Hz, a
|||	        variable-rate instrument will be chosen to play all samples with an
|||	        original sample rate other than 44100. A fixed-rate instrument is chosen
|||	        only if ifVariable is FALSE and the sample was recorded at 44100 Hz.
|||
|||	    nameBuffer
|||	        A character buffer to write the name into.
|||
|||	    nameBufferSize
|||	        The size of the buffer including the NUL byte.
|||
|||	  Return Value
|||
|||	    This procedure returns a negative error code or zero.
|||
|||	  Implementation
|||
|||	    Library call implemented in libmusic.a V27.
|||
|||	  Module Open Requirements
|||
|||	    OpenAudioFolio()
|||
|||	  Associated Files
|||
|||	    <audio/parse_aiff.h>, libmusic.a, System.m2/Modules/audio
|||
|||	  See Also
|||
|||	    SampleFormatToInsName(), LoadSample(), Sample(@)
**/
Err SampleItemToInsName( Item Sample, bool IfVariable, char *nameBuffer, int32 nameBufferSize )
{
	int32 numChannels;
	uint32 compressionType;
	int32 numBits;
	uint32 iSampleRate;
	float32 fSampleRate;
	int32 Result;
	TagArg Tags[5];
	int32 IfNormalSampleRate;

/* Get information from sample. */
	Tags[0].ta_Tag = AF_TAG_CHANNELS;
	Tags[1].ta_Tag = AF_TAG_COMPRESSIONTYPE;
	Tags[2].ta_Tag = AF_TAG_NUMBITS;
	Tags[3].ta_Tag = AF_TAG_SAMPLE_RATE_FP;
	Tags[4].ta_Tag = TAG_END;
	Result = GetAudioItemInfo( Sample, Tags );
	if( Result < 0) return Result;

/* Get information from tags. */
	numChannels = (int32) Tags[0].ta_Arg;
	compressionType = (uint32) Tags[1].ta_Arg;
	numBits = (int32) Tags[2].ta_Arg;
	fSampleRate = ConvertTagData_FP(Tags[3].ta_Arg);
DBUG(("SampleItemToInsName: compressionType = %.4s\n", &compressionType));
DBUG(("SampleItemToInsName: SampleRate = %g\n", fSampleRate));

/* Set ifVariable depending on sample rate and parameter. */
	iSampleRate = fSampleRate + 0.5; /* Round to nearest integer. */
	IfNormalSampleRate = (iSampleRate == 44100);
	if( !IfNormalSampleRate ) IfVariable = TRUE;

/* If no compression, pass bits. */
	if( compressionType == 0 ) compressionType = numBits;

/* Build name. */
	Result = SampleFormatToInsName( compressionType, IfVariable, numChannels,
	          nameBuffer, nameBufferSize );
	if( Result >= 0 ) DBUG(("SampleItemToInsName: built %s\n", nameBuffer ));
	return Result;
}
