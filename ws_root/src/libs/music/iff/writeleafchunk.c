/****************************************************************************
**
**  @(#) writeleafchunk.c 96/02/23 1.4
**
****************************************************************************/

#include <audio/music_iff.h>        /* Self */


/**
|||	AUTODOC -private -class libmusic -group IFF -name WriteLeafChunk
|||	Writes a single block of data as a leaf chunk (convenience).
|||
|||	  Synopsis
|||
|||	    Err WriteLeafChunk (IFFParser *iff, PackedID id, const void *chunkData,
|||	                        uint32 chunkSize)
|||
|||	  Description
|||
|||	    Writes a single block of data verbatim as a leaf chunk.
|||
|||	  Arguments
|||
|||	    iff
|||	        IFFParser to write into. Assumes that the top context on the context
|||	        stack is a container chunk suitable for containing leaf chunks (i.e.,
|||	        FORM or PROP).
|||
|||	    id
|||	        PackedID of leaf chunk. Type is inherited from the containing FORM or
|||	        PROP.
|||
|||	    chunkData
|||	        Pointer to block of memory containing data to write into leaf chunk.
|||
|||	    chunkSize
|||	        Number of bytes to write. The written chunk will contain precisely this
|||	        many bytes.
|||
|||	  Return Value
|||
|||	    Non-negative value on success, negative error code on failure.
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
|||	    WriteChunk()
**/

Err WriteLeafChunk (IFFParser *iff, PackedID id, const void *chunkData, uint32 chunkSize)
{
    Err errcode;

    if ((errcode = PushChunk (iff, 0, id, chunkSize)) < 0) goto clean;
    if ((errcode = WriteChunk (iff, chunkData, chunkSize)) < 0) goto clean;
    if ((errcode = PopChunk (iff)) < 0) goto clean;

    return 0;

clean:
    return errcode;
}
