/******************************************************************************
**
**  @(#) loadatag.c 96/02/23 1.11
**
**  Loads an ATAG file, returns audio Item.
**
**  By: Bill Barton
**
**  Copyright (c) 1995, 3DO Company.
**  This program is proprietary and confidential.
**
**-----------------------------------------------------------------------------
**
**  History:
**
**  951107 WJB  Added to music library.
**  951109 WJB  Cleaned up a bunch.
**  951110 WJB  Now using AF_TAG_AUTO_FREE_DATA with MEMTYPE_TRACKSIZE.
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**  PLB: Phil Burk (phil)
**
******************************************************************************/

#include <audio/atag.h>     /* self */
#include <audio/audio.h>
#include <audio/musicerror.h>
#include <file/fileio.h>
#include <kernel/mem.h>

#include "music_internal.h"


/* -------------------- Package ID */

MUSICLIB_PACKAGE_ID(loadatag)


/* -------------------- Debug */

#define DEBUG_Load  0       /* turn on debugging */
#if DEBUG_Load
#include <stdio.h>
#endif


/* -------------------- Internal data */

    /* IFF-like chunk header */
typedef struct ChunkHeader {
    PackedID ckhd_ID;
    uint32   ckhd_Size;
} ChunkHeader;


/* -------------------- Code */

static int32 BufferChunk (RawFile *, void **resultChunkBuf, PackedID chunkID);
static Err ReadRawFileExact (RawFile *, void *buf, int32 len);

/**
|||	AUTODOC -public -class libmusic -group ATAG -name LoadATAG
|||	Load an ATAG simple audio object format file.
|||
|||	  Synopsis
|||
|||	    Item LoadATAG (const char *fileName)
|||
|||	  Description
|||
|||	    This function loads an ATAG simple audio object format file. This format
|||	    is a low overhead file format which can contain samples, envelopes, delay
|||	    line descriptions, and tunings.
|||
|||	    The result of a successful load is an audio folio Item of the type stored
|||	    in the file. If the Item has some data (e.g., sample data, envelope table,
|||	    tuning table), then the Item is created with { AF_TAG_AUTO_FREE_DATA,
|||	    TRUE } to facilitate simple and complete deletion of the Item. The returned
|||	    Item is given the same name as the ATAG file.
|||
|||	    Use DeleteItem() to dispose of the resulting Item when done with it.
|||
|||	  Arguments
|||
|||	    fileName
|||	        Name of an ATAG file to load.
|||
|||	  Return Value
|||
|||	    The procedure returns an Audio Item of the type stored in the file (a
|||	    positive value) if successful, or an error code (a negative value) if an
|||	    error occurs.
|||
|||	  Implementation
|||
|||	    Library function implemented in libmusic.a V29.
|||
|||	  Module Open Requirements
|||
|||	    OpenAudioFolio()
|||
|||	  Associated Files
|||
|||	    <audio/atag.h>, libmusic.a, System.m2/Modules/audio
|||
|||	  See Also
|||
|||	    --ATAG-File-Format--(@), ValidateAudioTagHeader(), Sample(@), Tuning(@),
|||	    Envelope(@), LoadSample(), CreateItem(), DeleteItem()
**/
Item LoadATAG (const char *filename)
{
    RawFile *file = NULL;
    AudioTagHeader *atag = NULL;
    int32 atagSize;
    void *body = NULL;
    int32 result;

        /* open file */
    if ((result = OpenRawFile (&file, filename, FILEOPEN_READ)) < 0) goto clean;

        /* read and validate ATAG chunk */
    if ((result = atagSize = BufferChunk (file, &atag, ID_ATAG)) < 0) goto clean;
    if (!atagSize) {
        result = ML_ERR_MANGLED_FILE;
        goto clean;
    }
  #ifdef BUILD_PARANOIA
        /* quickie validate tags in paranoia mode */
    if ((result = ValidateAudioTagHeader (atag, atagSize)) < 0) goto clean;
  #endif

        /* read BODY chunk. 0 size means no BODY data, which is OK. */
    if ((result = BufferChunk (file, &body, ID_BODY)) < 0) goto clean;

        /* create Item */
    if ((result = CreateItemVA ( MKNODEID (AUDIONODE, atag->athd_NodeType),
                                 TAG_ITEM_NAME,     filename,
                                    /* avoid passing AF_TAG_ADDRESS, NULL for delay lines */
                                 body ? AF_TAG_ADDRESS : TAG_NOP,
                                                    body,
                                 body ? AF_TAG_AUTO_FREE_DATA : TAG_NOP,
                                                    TRUE,
                                 TAG_JUMP,          atag->athd_Tags )) < 0) goto clean;

        /* success */
    body = NULL;                    /* prevent data from getting freed during cleanup, now owned by Item */

clean:
    FreeMem (atag, TRACKED_SIZE);
    FreeMem (body, TRACKED_SIZE);
    CloseRawFile (file);
    return result;
}

/*
    Read chunk header, allocate buffer with MEMTYPE_TRACKSIZE for chunk
    contents (if any), and read chunk into it.

    Arguments
        file
            File to read from. Must be positioned to the beginning of a chunk.

        resultChunkBuf
            Location to write resulting allocated buffer pointer.

        chunkID
            Chunk ID that we expect. Returns an error if chunk is something else.

    Results
        Number of bytes buffered (>=0) if successful. Err code on failure.

        resultChunkBuf
            Pointer to allocated buffer on success and non-zero result. NULL when
            result is <= 0. In the case of 0, there is no data to buffer, so no
            buffer memory is allocated. This is not an error.
*/
static int32 BufferChunk (RawFile *file, void **resultChunkBuf, PackedID chunkID)
{
    void *chunkBuf = NULL;
    ChunkHeader ckhd;
    Err errcode;

        /* init result */
    *resultChunkBuf = NULL;

        /* read chunk header, and check against desired chunkID  */
    if ((errcode = ReadRawFileExact (file, &ckhd, sizeof ckhd)) < 0) goto clean;
  #if DEBUG_Load
    printf ("ATAG: id='%.4s' Size=0x%x\n", &ckhd.ckhd_ID, ckhd.ckhd_Size);
  #endif
    if (ckhd.ckhd_ID != chunkID) {
        errcode = ML_ERR_INVALID_FILE_TYPE;
        goto clean;
    }

        /* alloc and read data, if any */
    if (ckhd.ckhd_Size) {
        if (!(chunkBuf = AllocMem (ckhd.ckhd_Size, MEMTYPE_NORMAL | MEMTYPE_TRACKSIZE))) {
            errcode = ML_ERR_NOMEM;
            goto clean;
        }
        if ((errcode = ReadRawFileExact (file, chunkBuf, ckhd.ckhd_Size)) < 0) goto clean;
    }

        /* success */
    *resultChunkBuf = chunkBuf;     /* NULL for 0-size case, which is correct */
    return ckhd.ckhd_Size;

clean:
    FreeMem (chunkBuf, TRACKED_SIZE);
    return errcode;
}

/*
    Read exactly the specified number of bytes from the file. Any less
    returns an error.

    Results
        Non-negative value on success. Err code on failure, including
        insufficient bytes read.
*/
static Err ReadRawFileExact (RawFile *file, void *buf, int32 len)
{
    const int32 result = ReadRawFile (file, buf, len);

    return (result == len || result < 0) ? result : ML_ERR_MANGLED_FILE;
}
