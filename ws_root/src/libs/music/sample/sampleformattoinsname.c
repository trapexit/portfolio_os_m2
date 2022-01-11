/* @(#) sampleformattoinsname.c 96/02/23 1.21 */
/****************************************************************
**
** Select the appropriate sample playing instrument for a sample.
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
*****************************************************************
** 931110 PLB Moved SampleItemToInsName() out to own file.
** 940727 WJB Added autodocs.
** 941024 PLB Added dcsqxdvarmono.dsp and adpcmvarmono.dsp
** 950614 PLB Added support for ID_SQS2 for M2 hardware.
** 960212 WJB Split into 2 modules.
****************************************************************/

#include <audio/audio.h>
#include <audio/musicerror.h>
#include <audio/parse_aiff.h>   /* self */
#include <ctype.h>              /* tolower() */
#include <string.h>             /* strcpy() */

#include "music_internal.h"     /* version stuff */

MUSICLIB_PACKAGE_ID(selectsampler)

#define DBUG(x)  /* printf x */

/**
|||	AUTODOC -public -class libmusic -group Sample -name SampleFormatToInsName
|||	Builds the name of the DSP Instrument Template to play a sample of the specified
|||	format.
|||
|||	  Synopsis
|||
|||	    Err SampleFormatToInsName (uint32 compressionType, bool ifVariable,
|||	                               uint32 numChannels, char *nameBuffer,
|||	                               int32 nameBufferSize)
|||
|||	  Description
|||
|||	    This procedure builds the name of the instrument that will play a sample of
|||	    the given format. There is no guarantee that there is an instrument with
|||	    this name but if there is, it's the right one. This procedure builds the
|||	    name by concatenating the 4 characters from the compressionType with the
|||	    fixed or variable flag, and the number of channels. The name will be of the
|||	    form "sampler_CCCC_PN.dsp" where:
|||
|||	    CCCC = the 4 characters of the compression type in lower case, e.g., "sqd2"
|||
|||	    P = 'f' for fixed pitch or 'v' for variable pitch
|||
|||	    N = the number of channels
|||
|||	    For example, an instrument that plays mono ID_SQS2 samples at a fixed rate is:
|||	    sampler_sqs2_f1.dsp(@).
|||
|||	  Arguments
|||
|||	    compressionType
|||	        The 4 character ID from the AIFC file, or the number of bits (either 8
|||	        or 16).
|||
|||	    ifVariable
|||	        A value indicating whether or not the pitch is to vary. If TRUE, the
|||	        pitch will vary. If FALSE, the pitch won't vary.
|||
|||	    numChannels
|||	        The number of channels, 1 for mono, 2 for stereo, and so on, up to 8.
|||
|||	    nameBuffer
|||	        A character buffer to write the name into.
|||
|||	    nameBufferSize
|||	        The size of the buffer including the space for the NUL termination.
|||
|||	  Return Value
|||
|||	    This procedure returns a negative error code or zero.
|||
|||	  Implementation
|||
|||	    Library call implemented in libmusic.a V27.
|||
|||	  Caveats
|||
|||	    GIGO: This function will happily return an instrument name for a
|||	    non-existent instrument (e.g., and unsupported compression type or number
|||	    of channels).
|||
|||	    The matrix of sample player instruments is not complete, so even if all of
|||	    the individual parameters are legal, there may not be an instrument to play
|||	    that format (e.g., sampler_adp4_v1.dsp(@) exists but sampler_adp4_f1.dsp
|||	    does not).
|||
|||	  Associated Files
|||
|||	    <audio/parse_aiff.h>, libmusic.a, System.m2/Modules/audio
|||
|||	  See Also
|||
|||	    SampleItemToInsName(), GetAIFFSampleInfo(), Sample(@)
**/
Err SampleFormatToInsName( uint32 compressionType, bool ifVariable,
                           uint32 numChannels, char *nameBuffer, int32 nameBufferSize)
{
/*                             sampler_CCCC_PN.dsp \0 */
#define MIN_NAME_BUFFER_SIZE (    7  +1 +4+1+2 +4  +1 )

	static const char gSamplerPrefix[] = "sampler_";
	char *s;

/* Validate input. */
	if( numChannels > 8 ) return ML_ERR_OUT_OF_RANGE;

	if( nameBufferSize < MIN_NAME_BUFFER_SIZE ) return ML_ERR_BUFFER_TOO_SMALL;

/* Start with "sampler_" */
	strcpy( nameBuffer, gSamplerPrefix );
	s = &nameBuffer[sizeof(gSamplerPrefix)-1];

DBUG(("SampleFormatToInsName: comp = 0x%x\n", compressionType ));
DBUG(("SampleFormatToInsName: built %s at 0x%x, s = 0x%x\n", nameBuffer, nameBuffer, s ));

/* Append Compression ID or numBits */
	if( (compressionType & 0xFF000000) == 0)
	{
		if( compressionType == 16 )
		{
			*s++ = '1';
			*s++ = '6';
		}
		else if( compressionType == 8 )
		{
			*s++ = '8';
		}
		else
		{
			return ML_ERR_BAD_ARG;
		}
	}
	else
	{
		*s++ = tolower((char) ((compressionType >> 24) & 0xFF));
		*s++ = tolower((char) ((compressionType >> 16) & 0xFF));
		*s++ = tolower((char) ((compressionType >> 8) & 0xFF));
		*s++ = tolower((char) ((compressionType) & 0xFF));
	}
	*s++ = '_';

/* Append fixed or variable */
	*s++ = (ifVariable) ? 'v' : 'f';

/* Append number of channels. */
	*s++ = numChannels + '0';

/* Stick ".dsp" on end */
	strcpy( s, ".dsp" );
	return 0;
}
