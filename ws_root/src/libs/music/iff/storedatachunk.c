/****************************************************************************
**
**  @(#) storedatachunk.c 96/02/23 1.3
**
****************************************************************************/

#include <audio/music_iff.h>        /* Self */
#include <kernel/mem.h>

#include "datachunk_internal.h"

static Err PurgeDataChunk (IFFParser *, ContextInfo *);

/**
|||	AUTODOC -private -class libmusic -group IFF -name StoreDataChunk
|||	Stores a DataChunk into the IFF context stack.
|||
|||	  Synopsis
|||
|||	    Err StoreDataChunk (IFFParser *iff, PackedID type, PackedID id,
|||	                        ContextInfoLocation pos, void *data)
|||
|||	  Description
|||
|||	    Stores a block of memory allocated with MEMTYPE_TRACKSIZE into the IFF
|||	    context stack. Once stored, if the data is not extracted by the time the
|||	    context is popped off of the context stack, the data is automatically
|||	    freed. If another data chunk with the same type and id has already been
|||	    stored at the specified context location, that data is freed before the
|||	    new chunk is stored (similar in behavior to property chunk storage).
|||
|||	    This is handy for storing a block of memory into the context stack from an
|||	    entry or exit handler which is to be picked up at some later point as if it
|||	    were a data chunk. This function is used by the entry handler installed by
|||	    RegisterDataChunks(), so the collection method is the same.
|||
|||	    Use ExtractDataChunk() to claim the contents of the data chunk. Once
|||	    extracted, the memory is the responsibility of the caller. If the data is
|||	    not extracted by the time that the context is popped off of the context
|||	    stack, the memory is automatically freed.
|||
|||	  Arguments
|||
|||	    iff
|||	        IFFParser into whose context stack the data chunk is to be stored.
|||
|||	    type, id
|||	        Type and ID to give stored data chunk.
|||
|||	    pos
|||	        Position in context stack to store data chunk.
|||
|||	    data
|||	        MEMTYPE_TRACKSIZE allocated block to store or NULL. This call does not
|||	        validate that this is the case.
|||
|||	        Because NULL is the value returned by FindDataChunk() if no data
|||	        matching the requested type and id is found, you may store NULL to
|||	        simulate this condition. This may be used to avoid having
|||	        FindDataChunk() return data from lower on the context stack.
|||
|||	  Return Value
|||
|||	    0 on success, Err code on failure.
|||
|||	    On success, the data becomes the property of the IFF context until it is
|||	    extracted. If this function fails, the data is left intact.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libmusic.a V30.
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
|||	    ExtractDataChunk(), FindDataChunk(), RegisterDataChunks(), StoreContextInfo()
**/
Err StoreDataChunk (IFFParser *iff, PackedID type, PackedID id, ContextInfoLocation pos, void *data)
{
    ContextInfo *ci;
    DataChunk *dc;
    Err errcode;

  #if DEBUG_DataChunk
    {
        const int32 dataSize = data ? GetMemTrackSize(data) : 0;

        IFFPrintf (iff, "StoreDataChunk %.4s.%.4s pos=%d data=0x%x size=0x%x (%d) bytes\n", &type, &id, pos, data, dataSize, dataSize);
        if (pos == IFF_CIL_PROP) {
            const ContextNode * const cn = FindPropContext (iff);
            if (cn) IFFPrintf (iff, "prop context: %.4s %.4s %Ld\n", &cn->cn_Type, &cn->cn_ID, cn->cn_Size);
        }
    }
  #endif

        /* allocate ContextInfo + DataChunk */
        /* ci_Data points to a DataChunk */
    if (!(ci = AllocContextInfo (type, id, MIFF_CI_DATACHUNK, sizeof (DataChunk), (IFFCallBack)PurgeDataChunk))) {
        errcode = IFF_ERR_NOMEM;
        goto clean;
    }
    dc = (DataChunk *)ci->ci_Data;
    dc->dc_Data = data;

        /* store it */
    if ((errcode = StoreContextInfo (iff, ci, pos)) < 0) goto clean;

    return 0;

clean:
        /* not calling PurgeDataChunk() because this function doesn't free the data if it fails */
    FreeContextInfo (ci);
    return errcode;
}

/*
    MIFF_CI_DATACHUNK context info purge method.
    Dispose of an unextracted data chunk. Frees the data chunk memory.

    Arguments
        iff
        ci
            Never NULL because this is only called by iff folio.
*/
static Err PurgeDataChunk (IFFParser *iff, ContextInfo *ci)
{
    DataChunk * const dc = ci->ci_Data;

    TOUCH(iff);

  #if DEBUG_DataChunk
    {
        const int32 dataSize = dc->dc_Data ? GetMemTrackSize(dc->dc_Data) : 0;

            /* @@@ ci_Type and ci_ID are private fields to the IFF folio */
        IFFPrintf (iff, "PurgeDataChunk %.4s.%.4s data=0x%x size=0x%x (%d) bytes\n", &ci->ci_Type, &ci->ci_ID, dc->dc_Data, dataSize, dataSize);
    }
  #endif

    FreeMem (dc->dc_Data, TRACKED_SIZE);
    FreeContextInfo (ci);
    return 0;
}
