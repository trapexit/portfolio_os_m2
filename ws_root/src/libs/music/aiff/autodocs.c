/******************************************************************************
**
**  @(#) autodocs.c 96/02/14 1.3
**
**  Assorted sample-related autodocs.
**
******************************************************************************/


/* -------------------- AIFFPString */

/**
|||	AUTODOC -private -class libmusic -group AIFF -name AIFFPStringAddr
|||	Returns address of first character in AIFFPString.
|||
|||	  Synopsis
|||
|||	    char *AIFFPStringAddr (char *s)
|||
|||	  Description
|||
|||	    Returns address of first character in AIFFPString.
|||
|||	  Arguments
|||
|||	    s
|||	        AIFFPString to query.
|||
|||	  Return Value
|||
|||	    Pointer to first character.
|||
|||	  Implementation
|||
|||	    Macro implemented in <audio/aiff_format.h> V30.
|||
|||	  Associated Files
|||
|||	    <audio/aiff_format.h>
|||
|||	  See Also
|||
|||	    AIFFPStringLength(), AIFFPStringSize(), DecodeAIFFPString()
**/

/**
|||	AUTODOC -private -class libmusic -group AIFF -name AIFFPStringLength
|||	Returns AIFFPString length.
|||
|||	  Synopsis
|||
|||	    size_t AIFFPStringLength (const char *s)
|||
|||	  Description
|||
|||	    Returns number of characters in AIFFPString.
|||
|||	  Arguments
|||
|||	    s
|||	        AIFFPString to query.
|||
|||	  Return Value
|||
|||	    Number of characters in AIFFPString.
|||
|||	  Implementation
|||
|||	    Macro implemented in <audio/aiff_format.h> V30.
|||
|||	  Associated Files
|||
|||	    <audio/aiff_format.h>
|||
|||	  See Also
|||
|||	    AIFFPStringAddr(), AIFFPStringSize(), DecodeAIFFPString()
**/

/**
|||	AUTODOC -private -class libmusic -group AIFF -name AIFFPStringSize
|||	Returns size (in bytes) of AIFFPString structure.
|||
|||	  Synopsis
|||
|||	    size_t AIFFPStringSize (const char *s)
|||
|||	  Description
|||
|||	    Returns the number of bytes required to hold the entire AIFFPString (i.e.,
|||	    its length byte, characters, and even-up pad byte).
|||
|||	  Arguments
|||
|||	    s
|||	        AIFFPString to query.
|||
|||	  Return Value
|||
|||	    Size of AIFFPString in bytes. Because AIFFPStrings are always required to
|||	    have an even number of bytes, with a pad byte at the end if necessary, this
|||	    value is always even.
|||
|||	  Implementation
|||
|||	    Macro implemented in <audio/aiff_format.h> V30.
|||
|||	  Associated Files
|||
|||	    <audio/aiff_format.h>
|||
|||	  See Also
|||
|||	    AIFFPStringAddr(), AIFFPStringLength(), DecodeAIFFPString()
**/


/* -------------------- AIFFPackedMarker */

/**
|||	AUTODOC -private -class libmusic -group AIFF -name AIFFPackedMarkerSize
|||	Returns size in bytes of AIFFPackedMarker.
|||
|||	  Synopsis
|||
|||	    uint32 AIFFPackedMarkerSize (const AIFFPackedMarker *markx)
|||
|||	  Description
|||
|||	    Returns size in bytes of AIFFPackedMarker taking into account the size of
|||	    the marker's name.
|||
|||	  Arguments
|||
|||	    markx
|||	        Pointer to AIFFPackedMarker to query.
|||
|||	  Return Value
|||
|||	    Size in bytes of the AIFFPackedMarker.
|||
|||	  Implementation
|||
|||	    Macro implemented in <audio/aiff_format.h> V30.
|||
|||	  Associated Files
|||
|||	    <audio/aiff_format.h>
|||
|||	  See Also
|||
|||	    NextAIFFPackedMarker(), ValidateAIFFMarkerChunk()
**/

/**
|||	AUTODOC -private -class libmusic -group AIFF -name NextAIFFPackedMarker
|||	Returns pointer to next AIFFPackedMarker in marker chunk.
|||
|||	  Synopsis
|||
|||	    AIFFPackedMarker *NextAIFFPackedMarker (const AIFFPackedMarker *markx)
|||
|||	  Description
|||
|||	    Returns pointer to next AIFFPackedMarker in marker chunk. If the current
|||	    marker is the last one in the chunk, the returned pointer points the first
|||	    byte past the end of the chunk.
|||
|||	  Arguments
|||
|||	    markx
|||	        Pointer to AIFFPackedMarker to skip.
|||
|||	  Return Value
|||
|||	    Pointer to next AIFFPackedMarker.
|||
|||	  Implementation
|||
|||	    Macro implemented in <audio/aiff_format.h> V30.
|||
|||	  Associated Files
|||
|||	    <audio/aiff_format.h>
|||
|||	  See Also
|||
|||	    AIFFPackedMarkerSize(), ValidateAIFFMarkerChunk()
**/

/* keep the compiler happy... */
extern int foo;
