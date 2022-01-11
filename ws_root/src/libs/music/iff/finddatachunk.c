/****************************************************************************
**
**  @(#) finddatachunk.c 96/02/23 1.3
**
****************************************************************************/

#include <audio/music_iff.h>        /* Self */

#include "datachunk_internal.h"

/**
|||	AUTODOC -private -class libmusic -group IFF -name FindDataChunk
|||	Finds a data chunk in the IFF context stack without removing it from the context
|||	stack.
|||
|||	  Synopsis
|||
|||	    void *FindDataChunk (IFFParser *iff, PackedID type, PackedID id)
|||
|||	  Description
|||
|||	    Finds a data chunk in the IFF context stack without removing it from the
|||	    context stack (as is done by ExtractDataChunk()).
|||
|||	    The returned MEMTYPE_TRACKSIZE memory block remains the responsibility of
|||	    the IFF context. Use GetMemTrackSize() to determine the size of the block.
|||
|||	  Arguments
|||
|||	    iff
|||	        IFFParser to scan.
|||
|||	    type, id
|||	        Type and ID to of data chunk to search for.
|||
|||	  Return Value
|||
|||	    On success, returns a pointer to a MEMTYPE_TRACKSIZE allocated buffer
|||	    containing the data chunk contents. Returns NULL on failure, which usually
|||	    means that the data chunk wasn't found or it was empty (zero-length).
|||
|||	  Implementation
|||
|||	    Link library call implemented in libmusic.a V29.
|||
|||	  Module Open Requirements
|||
|||	    OpenIFFFolio()
|||
|||	  Associated Files
|||
|||	    <audio/music_iff.h>, libmusic.a, System.m2/Modules/iff
|||
|||	  See Also
|||
|||	    RegisterDataChunks(), ExtractDataChunk(), FindPropChunk()
**/
void *FindDataChunk (IFFParser *iff, PackedID type, PackedID id)
{
    ContextInfo * const ci = FindContextInfo (iff, type, id, MIFF_CI_DATACHUNK);
    void *data = NULL;

        /* if not found, return NULL */
    if (ci) {
        DataChunk * const dc = (DataChunk *)ci->ci_Data;

      #if DEBUG_DataChunk
        IFFPrintf (iff, "FindDataChunk %.4s.%.4s data=0x%x\n", &type, &id, dc->dc_Data);
      #endif

            /* return data pointer (which might itself be NULL) */
        data = dc->dc_Data;
    }

    return data;
}
