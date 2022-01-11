/* @(#) decodeaifffloat80.c 96/02/23 1.5 */

#include <audio/aiff_format.h>
#include <math.h>

/**
|||	AUTODOC -private -class libmusic -group AIFF -name DecodeAIFFFloat80
|||	Converts an AIFFFloat80 to a float32.
|||
|||	  Synopsis
|||
|||	    float32 DecodeAIFFFloat80 (const AIFFFloat80 *f80)
|||
|||	  Description
|||
|||	    Converts an AIFFFloat80 (structure containing an IEEE 80-bit floating point
|||	    value) a float32.
|||
|||	  Arguments
|||
|||	    f80
|||	        IEEE 80-bit floating point value in an AIFFFloat80 structure.
|||
|||	  Return Value
|||
|||	    32-bit IEEE floating point value approximation of 80-bit value.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libmusic.a V30.
|||
|||	  Caveats
|||
|||	    Returns junk if f80 is out of range for an float32.
|||
|||	  Associated Files
|||
|||	    <audio/aiff_format.h>, libmusic.a, libspmath.a
**/
/* !!! could probably use some further optimization */
float32 DecodeAIFFFloat80 (const AIFFFloat80 *f80)
{
    const bool neg = (f80->f80_Exponent & 0x8000) != 0;
    const int exp = f80->f80_Exponent & 0x7fff;
    const uint32 manthi = ((uint32 *)f80->f80_Mantissa)[0];
    const uint32 mantlo = ((uint32 *)f80->f80_Mantissa)[1];
    float32 f32;

        /* infinity or NaN? */
    if (exp == 32767) return HUGE_VAL_F;

        /* zero? */
    if (!manthi && !mantlo) return 0.0;

        /* create f32 */
    f32 = ldexpf ((float32)manthi * (float32)(1ULL<<32) + mantlo, exp-16383-63);
    if (neg) f32 = -f32;
    return f32;
}
