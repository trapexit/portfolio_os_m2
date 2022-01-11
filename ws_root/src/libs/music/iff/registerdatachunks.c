/****************************************************************************
**
**  @(#) registerdatachunks.c 96/02/23 1.6
**
****************************************************************************/

#include <audio/music_iff.h>        /* Self */
#include <kernel/mem.h>

#include "datachunk_internal.h"

#if DEBUG_DataChunk
#include <stdio.h>
#endif

static Err HandleDataChunk (IFFParser *, void *dummy);

/**
|||	AUTODOC -private -class libmusic -group IFF -name RegisterDataChunks
|||	Defines data chunks to buffer during IFF parsing.
|||
|||	  Synopsis
|||
|||	    Err RegisterDataChunks (IFFParser *iff, const IFFTypeID typeids[])
|||
|||	  Description
|||
|||	    Registers chunks to be collected as Data Chunks. This is similar in behavior
|||	    to RegisterPropChunks(). The only difference is that the data chunk contents
|||	    is buffered in a separate MEMTYPE_TRACKSIZE allocated block, so that the
|||	    allocated memory block containing the contents of the chunk can be given to
|||	    caller rather than remaining intrinsically bound to the IFF context stack.
|||	    This permits collecting and keeping the contents of chunks verbatim without
|||	    having to do another allocation and copy.
|||
|||	    Use ExtractDataChunk() to claim the contents of the data chunk. Once
|||	    extracted, the memory is the responsibility of the caller. If the data is
|||	    not extracted by the time that the context is popped off of the context
|||	    stack, the memory is automatically freed.
|||
|||	    The data chunk handler stores data into the context using StoreDataChunk().
|||
|||	  Arguments
|||
|||	    iff
|||	        IFFParser to operate on.
|||
|||	    typeids
|||	        An array of IFFTypeID structures defining which chunks should be stored.
|||	        The array is terminated by a structures whose Type field is set to 0.
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
|||	    ExtractDataChunk(), FindDataChunk(), StoreDataChunk(), RegisterPropChunks()
**/
Err RegisterDataChunks (IFFParser *iff, const IFFTypeID *typeids)
{
    Err errcode;

  #if DEBUG_DataChunk
    IFFPrintf (iff, "RegisterDataChunks\n");
  #endif

    for (; typeids->Type; typeids++) {
      #if DEBUG_DataChunk
        printf ("  %.4s.%.4s\n", &typeids->Type, &typeids->ID);
      #endif
        if ((errcode = InstallEntryHandler (iff, typeids->Type, typeids->ID, IFF_CIL_TOP, HandleDataChunk, NULL)) < 0) return errcode;
    }

    return 0;
}

/*
    Handle data chunks. Adds an MIFF_CI_DATACHUNK context item to the context
    stack, replacing any previously stored context at the same level with the
    same id and type.

    Stores NULL if the data chunk has 0 length.

    Arguments
        iff

    Results
        IFF_CB_CONTINUE on success. Err code on failure.
*/
static Err HandleDataChunk (IFFParser *iff, void *dummy)
{
    ContextNode * const cn = GetCurrentContext (iff);
    void *data = NULL;
    Err errcode;

    TOUCH(dummy);

  #if DEBUG_DataChunk
    IFFPrintf (iff, "HandleDataChunk: %.4s.%.4s 0x%Lx (%Ld) bytes", &cn->cn_Type, &cn->cn_ID, cn->cn_Size, cn->cn_Size);
  #endif

        /* alloc/load data (leaves dc_Data NULL if 0-length chunk) */
    if (cn->cn_Size) {

            /* @@@ truncates 64-bit size */
        if (!(data = AllocMem (cn->cn_Size, MEMTYPE_NORMAL | MEMTYPE_TRACKSIZE))) {
            errcode = IFF_ERR_NOMEM;
            goto clean;
        }

        {
            int32 result;

            if ((result = ReadChunk (iff, data, cn->cn_Size)) != cn->cn_Size) {
                errcode = result < 0 ? result : IFF_ERR_MANGLED;
                goto clean;
            }
        }

      #if DEBUG_DataChunk
        printf (" @ 0x%x", data);
      #endif
    }
  #if DEBUG_DataChunk
    printf ("\n");
  #endif

        /* store data in context. this doesn't free the data if it fails. */
    if ((errcode = StoreDataChunk (iff, cn->cn_Type, cn->cn_ID, IFF_CIL_PROP, data)) < 0) goto clean;

        /* success */
    return IFF_CB_CONTINUE;

clean:
    FreeMem (data, TRACKED_SIZE);
    return errcode;
}
