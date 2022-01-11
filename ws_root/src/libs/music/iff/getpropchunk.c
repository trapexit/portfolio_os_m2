/* @(#) getpropchunk.c 96/02/23 1.2 */

#include <audio/music_iff.h>    /* self */
#include <string.h>

/**
|||	AUTODOC -private -class libmusic -group IFF -name GetPropChunk
|||	Returns PropChunk contents into a supplied buffer.
|||
|||	  Synopsis
|||
|||	    uint32 GetPropChunk (const IFFParser *iff, PackedID type, PackedID id,
|||	                         void *resultBuf, uint32 resultBufSize)
|||
|||	  Description
|||
|||	    Finds the specified PropChunk, and copies its contents into the supplied
|||	    buffer. If there are fewer bytes in the chunk than in the buffer, the
|||	    remaining bytes in the buffer are filled with 0. If there are more bytes
|||	    in the chunk than fit in the buffer, the buffer is filled completely and the
|||	    additional bytes from the chunk are discared.
|||
|||	    This is handy for avoiding the case where the file's chunk size differs
|||	    from the structure size.
|||
|||	  Arguments
|||
|||	    iff
|||	        IFFParser to scan.
|||
|||	    type, id
|||	        Type (e.g., AIFF) and ID (e.g., COMM) of PropChunk to find.
|||
|||	    resultBuf
|||	        Pointer to location to return chunk contents.
|||
|||	    resultBufSize
|||	        Number of bytes in resultBuf.
|||
|||	  Return Value
|||
|||	    Number of bytes in PropChunk, or 0 if PropChunk not found.
|||
|||	    When this function returns non-zero, resultBuf is filled with contents of
|||	    chunk, and padded with zeros if there are fewer bytes are in chunk than
|||	    buffer. When this function returns 0, resultBuf is not written to.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libmusic.a V30.
|||
|||	  Examples
|||
|||	    if (!GetPropChunk (iff, ID_AIFF, ID_COMM, &comm, sizeof comm))
|||	        return ML_ERR_BAD_FORMAT;
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
|||	    RegisterPropChunks(), FindPropChunk()
**/
uint32 GetPropChunk (const IFFParser *iff, PackedID type, PackedID id, void *resultBuf, uint32 resultBufSize)
{
    const PropChunk * const pc = FindPropChunk (iff, type, id);

        /* trap no chunk and zero-length chunk */
    if (!pc || !pc->pc_DataSize) return 0;

    if (pc->pc_DataSize >= resultBufSize) {
        memcpy (resultBuf, pc->pc_Data, resultBufSize);
    }
    else {
        memcpy (resultBuf, pc->pc_Data, pc->pc_DataSize);
        memset ((char *)resultBuf + pc->pc_DataSize, 0, resultBufSize - pc->pc_DataSize);
    }

    return pc->pc_DataSize;
}
