/* @(#) getaiffcompressionratio.c 96/02/13 1.2 */

#include <audio/aiff_format.h>

/**
|||	AUTODOC -private -class libmusic -group AIFF -name GetAIFFCompressionRatio
|||	Returns compression ratio of specified compression type.
|||
|||	  Synopsis
|||
|||	    uint32 GetAIFFCompressionRatio (PackedID compressionType)
|||
|||	  Description
|||
|||	    This function tries to determine the compression ratio for the specified
|||	    compression type. Compression type is typically in the form of XXXN, where
|||	    XXX is a 3-letter abbreviation of the compression type and N is digit in
|||	    the range of 1..9 representing the compression ratio. There some special
|||	    cases:
|||
|||	    If compressionType == 0 or 'NONE', there is no compression, so the
|||	    compression return ratio is 1.
|||
|||	    If compressionType doesn't end in a digit, then this function punts and
|||	    returns 1.
|||
|||	  Arguments
|||
|||	    compressionType
|||	        PackedID of compression type. 0 and 'NONE' both mean no compression.
|||
|||	  Return Value
|||
|||	    Compression ratio. This is always in the range of 1..9.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libmusic.a V30.
|||
|||	  Associated Files
|||
|||	    <audio/aiff_format.h>, libmusic.a
**/
uint32 GetAIFFCompressionRatio (PackedID compressionType)
{
    const uint32 compressionGuess = (uint8)compressionType; /* get last character of ID */

        /*
        ** If last character is anything other than a digit in the range of
        ** 1..9, return 1. This conveniently catches 'NONE' and 0.
        */
    return (compressionGuess >= '1' && compressionGuess <= '9')
        ? compressionGuess - '0'
        : 1;
}
