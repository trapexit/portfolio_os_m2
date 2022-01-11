/* @(#) handlessnd.c 96/02/23 1.5 */

#include <audio/aiff_format.h>
#include <audio/music_iff.h>        /* StoreDataChunk() */
#include <audio/musicerror.h>
#include <kernel/mem.h>
#include <misc/iff.h>

#define DEBUG_AIFF  0

#if DEBUG_AIFF
#include <audio/music_iff.h>        /* IFFPrint() */
#include <stdio.h>                  /* printf() */
#endif

static Err StoreSSNDInfo (IFFParser *, PackedID formType, uint32 dataOffset, uint32 dataSize);
static Err StoreSSNDData (IFFParser *, PackedID formType, uint32 dataSize);

/**
|||	AUTODOC -private -class libmusic -group AIFF -name HandleSSND
|||	AIFF/AIFC SSND chunk entry handler - stores SSNDInfo and optionally stores
|||	sample data as a DataChunk.
|||
|||	  Synopsis
|||
|||	    Err HandleSSND (IFFParser *iff, bool storeData)
|||
|||	  Description
|||
|||	    This is an IFFCallBack which can be used as an AIFF/AIFC SSND chunk entry
|||	    handler. It stores an SSNDInfo structure in the property context such that
|||	    it can be located with FindSSNDInfo(). It optionally allocates space for
|||	    and stores the SSND sound data so that it may be extracted with:
|||
|||	    ExtractDataChunk (iff, formType, ID_SSND)
|||
|||	    Each call to this entry handler replaces a previously stored SSNDInfo and
|||	    SSND DataChunk. The SSNDInfo is automatically purged when the context which
|||	    contains it is purged. Any unextracted SSND data is automatically freed when
|||	    the context which contains it is purged.
|||
|||	  Arguments
|||
|||	    iff
|||	        IFFParser positioned at the entry of an SSND chunk. This is assumed to
|||	        be the case.
|||
|||	    storeData
|||	        TRUE to cause SSND data to be read and stored. FALSE to skip it.
|||
|||	  Return Value
|||
|||	    IFF_CB_CONTINUE on success, Err code on failure.
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
|||	    <audio/aiff_format.h>, libmusic.a, System.m2/Modules/iff
|||
|||	  See Also
|||
|||	    FindSSNDInfo(), ExtractDataChunk()
**/
Err HandleSSND (IFFParser *iff, bool storeData)
{
    ContextNode * const top = GetCurrentContext (iff);
    AIFFSoundDataHeader ssndHead;
    int32 dataOffset;
    uint32 dataSize;
    Err errcode;

  #if DEBUG_AIFF
    IFFPrintf (iff, "HandleSSND: storeData=%d ", storeData);
  #endif

        /* read/validate SSNDHead:
        ** - make sure ssnd_Offset is in range of chunk.
        */
    {
        int32 result;

        if ((result = ReadChunk (iff, &ssndHead, sizeof ssndHead)) != sizeof ssndHead) {
            errcode = result >= 0 ? ML_ERR_BAD_FORMAT : result;
            goto clean;
        }
    }
    if (sizeof ssndHead + ssndHead.ssnd_Offset > top->cn_Size) {
        errcode = ML_ERR_BAD_FORMAT;
        goto clean;
    }

  #if DEBUG_AIFF
    printf ("ssnd_Offset=0x%x (%d)\n", ssndHead.ssnd_Offset, ssndHead.ssnd_Offset);
  #endif

        /* seek to beginning of data */
    if ((errcode = SeekChunk (iff, ssndHead.ssnd_Offset, IFF_SEEK_CURRENT)) < 0) goto clean;

        /* compute data offset from beginning of stream and size (remaining bytes in chunk) */
    if ((errcode = dataOffset = GetIFFOffset (iff)) < 0) goto clean;
    dataSize = top->cn_Size - top->cn_Offset;

        /* store SSNDInfo */
    if ((errcode = StoreSSNDInfo (iff, top->cn_Type, dataOffset, dataSize)) < 0) goto clean;

        /* store SSND data if requested and it's non-zero sized */
    if (storeData && dataSize) {
        if ((errcode = StoreSSNDData (iff, top->cn_Type, dataSize)) < 0) goto clean;
    }

        /* success: keep parsing */
    return IFF_CB_CONTINUE;

clean:
    return errcode;
}

/*
    Store SSNDInfo in current prop context. Locate with FindSSNDInfo().

    Arguments
        iff
            IFFParser positioned inside SSND chunk.

        formType
            ID_AIFF or ID_AIFC under which to store SSNDInfo.

        dataOffset
            Byte offset from beginning of file to sound data.

        dataSize
            Number of bytes in sound data. Can be 0.

    Results
        0 on success, Err on failure.
*/
static Err StoreSSNDInfo (IFFParser *iff, PackedID formType, uint32 dataOffset, uint32 dataSize)
{
    ContextInfo *ci;
    SSNDInfo *ssndInfo;
    Err errcode;

  #if DEBUG_AIFF
    IFFPrintf (iff, "StoreSSNDInfo formType='%.4s' dataOffset=0x%x (%d) dataSize=0x%x (%d)\n",
        &formType,
        dataOffset, dataOffset,
        dataSize, dataSize);
  #endif

    if (!(ci = AllocContextInfo (formType, ID_SSND, ID_SSND, sizeof (SSNDInfo), NULL))) {
        errcode = ML_ERR_NOMEM;
        goto clean;
    }
    ssndInfo = (SSNDInfo *)ci->ci_Data;
    ssndInfo->ssnd_DataOffset = dataOffset;
    ssndInfo->ssnd_DataSize   = dataSize;

        /* store in prop context */
        /* @@@ do this last to avoid having to RemoveContextInfo() on failure */
    if ((errcode = StoreContextInfo (iff, ci, IFF_CIL_PROP)) < 0) goto clean;

        /* success */
    return 0;

clean:
    FreeContextInfo (ci);
    return errcode;
}

/*
    Read and store SSND data in current prop context. Find/Extract with
    Find/ExtractDataChunk (formType, ID_SSND). SSND data is automatically freed
    if not extracted before prop context is purged.

    Arguments
        iff
            IFFParser positioned to beginning of SSND data.

        formType
            ID_AIFF or ID_AIFC under which to store SSNDInfo.

        dataSize
            Number of bytes in sound data. Must not be 0.

    Results
        0 on success, Err code on failure.

        IFFParser cursor is positioned to end of chunk on success, undefined on
        failure.
*/
static Err StoreSSNDData (IFFParser *iff, PackedID formType, uint32 dataSize)
{
    void *ssndData;
    int32 result;
    Err errcode;

  #if DEBUG_AIFF
    IFFPrintf (iff, "StoreSSNDData formType='%.4s' dataSize=0x%x (%d) ",
        &formType,
        dataSize, dataSize);
  #endif

        /* alloc */
    if (!(ssndData = AllocMem (dataSize, MEMTYPE_NORMAL | MEMTYPE_TRACKSIZE))) {
        errcode = ML_ERR_NOMEM;
        goto clean;
    }

        /* read */
    if ((result = ReadChunk (iff, ssndData, dataSize)) != dataSize) {
        errcode = result >= 0 ? ML_ERR_BAD_FORMAT : result;
        goto clean;
    }

        /* store data chunk in context so Find/ExtractDataChunk() can retrieve it */
    if ((errcode = StoreDataChunk (iff, formType, ID_SSND, IFF_CIL_PROP, ssndData)) < 0) goto clean;

  #if DEBUG_AIFF
    printf ("@ 0x%x\n", ssndData);
    IFFPrintf (iff, "0x%08x 0x%08x 0x%08x 0x%08x\n", ((uint32 *)ssndData)[0], ((uint32 *)ssndData)[1], ((uint32 *)ssndData)[2], ((uint32 *)ssndData)[3]);
    IFFPrintf (iff, "0x%08x 0x%08x 0x%08x 0x%08x\n", ((uint32 *)ssndData)[4], ((uint32 *)ssndData)[5], ((uint32 *)ssndData)[6], ((uint32 *)ssndData)[7]);
  #endif

        /* success */
    return 0;

clean:
    FreeMem (ssndData, TRACKED_SIZE);
    return errcode;
}
