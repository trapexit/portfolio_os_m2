/* @(#) validateaiffmarkerchunk.c 96/02/13 1.2 */

#include <audio/aiff_format.h>      /* self */
#include <audio/musicerror.h>

#define DEBUG_AIFF  0

#if DEBUG_AIFF
#include <stdio.h>
#endif

/**
|||	AUTODOC -private -class libmusic -group AIFF -name ValidateAIFFMarkerChunk
|||	Validates integrity of MARK chunk contents.
|||
|||	  Synopsis
|||
|||	    Err ValidateAIFFMarkerChunk (const AIFFMarkerChunk *markerChunk, uint32 markerChunkSize)
|||
|||	  Description
|||
|||	    Checks markerChunkSize for minimum MARK chunk length (enough to store
|||	    mark_NumMarkers). Then makes sure that all of the markers fit within
|||	    markerChunkSize.
|||
|||	    After doing this, you can reliably scan the marker chunk given only a
|||	    pointer to it.
|||
|||	  Arguments
|||
|||	    markerChunk
|||	        Pointer to AIFFMarkerChunk to validate. Assumed to not be NULL.
|||
|||	    markerChunkSize
|||	        Number of bytes pointed to by markerChunk
|||
|||	  Return Value
|||
|||	    0 on success, Err code on failure.
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
|||	    NextAIFFPackedMarker(), AIFFPackedMarkerSize()
**/
Err ValidateAIFFMarkerChunk (const AIFFMarkerChunk *markerChunk, uint32 markerChunkSize)
{
    const char * const markerChunkEnd = ((char *)markerChunk) + markerChunkSize;
    const AIFFPackedMarker *markx;
    uint16 i;

        /* get number of markers */
    if (markerChunkSize < AIFF_MARKER_CHUNK_MIN_SIZE) return ML_ERR_BAD_FORMAT;

  #if DEBUG_AIFF
    printf ("ValidateAIFFMarkerChunk: 0x%x, 0x%x bytes, %d markers\n", markerChunk, markerChunkSize, markerChunk->mark_NumMarkers);
  #endif

        /* scan markers. make sure they're all there */
    for (i=0, markx = markerChunk->mark_Markers; i<markerChunk->mark_NumMarkers; i++, markx = NextAIFFPackedMarker(markx)) {

            /* validate that there's enough data left for the shortest possible AIFFPackedMarker */
        if ((char *)markx + AIFF_PACKED_MARKER_MIN_SIZE > markerChunkEnd) return ML_ERR_BAD_FORMAT;

      #if DEBUG_AIFF
        {
            char b[AIFF_PSTR_MAX_LENGTH+1];

            printf ("  0x%04x: 0x%08x '%s'\n", markx->mark_ID, UNPACK_UINT32(markx->markx_Position), DecodeAIFFPString (b, markx->mark_Name, sizeof b));
        }
      #endif
    }

        /* make sure last marker fits in chunk */
    if ((char *)markx > markerChunkEnd) return ML_ERR_BAD_FORMAT;

        /* success */
    return 0;
}
