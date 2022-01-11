/* @(#) decodeaiffpstring.c 96/02/13 1.2 */

#include <audio/aiff_format.h>
#include <audio/handy_macros.h>
#include <string.h>

/**
|||	AUTODOC -private -class libmusic -group AIFF -name DecodeAIFFPString
|||	Converts an AIFFPString to a C string.
|||
|||	  Synopsis
|||
|||	    char *DecodeAIFFPString (char *dest, const char *src, size_t destsize)
|||
|||	  Description
|||
|||	    Converts an AIFFPString to a C string.
|||
|||	  Arguments
|||
|||	    dest
|||	        Destination C string buffer.
|||
|||	    src
|||	        Source AIFFPString.
|||
|||	    destsize
|||	        Number of bytes in dest buffer.
|||
|||	  Return Value
|||
|||	    Returns dest as its result. Writes no more than destsize bytes to *dest,
|||	    including '\0' (i.e., always nul terminates resulting string, and resulting
|||	    string's length is no more than destsize-1).
|||
|||	  Implementation
|||
|||	    Link library call implemented in libmusic.a V30.
|||
|||	  Associated Files
|||
|||	    <audio/aiff_format.h>, libmusic.a
|||
|||	  See Also
|||
|||	    AIFFPStringAddr(), AIFFPStringLength(), AIFFPStringSize()
**/
char *DecodeAIFFPString (char *dest, const char *src, size_t destsize)
{
    const size_t len = MIN(destsize-1,AIFFPStringLength(src));

    memcpy (dest, AIFFPStringAddr(src), len);   /* copy bytes to dest string */
    dest[len] = 0;                              /* add null termination */

    return dest;
}
