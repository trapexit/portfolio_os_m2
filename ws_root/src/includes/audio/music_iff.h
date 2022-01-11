#ifndef __AUDIO_MUSIC_IFF_H
#define __AUDIO_MUSIC_IFF_H


/****************************************************************************
**
**  @(#) music_iff.h 96/02/13 1.8
**
**  Private music library IFF services
**
****************************************************************************/


#ifdef EXTERNAL_RELEASE
#error This file may not be used in externally released source code.
#endif

#ifndef __MISC_IFF_H
#include <misc/iff.h>
#endif


/* -------------------- ID registry */

    /* ContextInfo identifiers (@@@ these must be unique) */
#define MIFF_CI_DATACHUNK MAKE_ID('d','a','t','a')      /* DataChunk ContextInfo */
#define MIFF_CI_ITEM      MAKE_ID('i','t','e','m')      /* Stored Item ContextInfo */


/* -------------------- Functions */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


    /*
        Stored Item

        A mechanism to store an Item into the IFF context stack such that if not
        extracted, it is automatically deleted when the context is purged.
    */
Err StoreItemInContext (IFFParser *, PackedID type, PackedID id, ContextInfoLocation pos, Item item);
Item ExtractStoredItem (IFFParser *, PackedID type, PackedID id);
Item FindStoredItem (IFFParser *, PackedID type, PackedID id);

    /*
        Data Chunk

        Similar to PropChunks, but makes a separate memory allocation to store
        the contents of the chunk. This allocated memory may be decoupled from
        the context stack allowing the caller to avoid another allocation and
        copy to keep the contents of a chunk. Handy when the chunk contents is
        kept verbatim.
    */
Err RegisterDataChunks (IFFParser *, const IFFTypeID *typeids);
Err StoreDataChunk (IFFParser *, PackedID type, PackedID id, ContextInfoLocation pos, void *data);
void *FindDataChunk (IFFParser *, PackedID type, PackedID id);
void *ExtractDataChunk (IFFParser *, PackedID type, PackedID id);

    /* Misc handy IFF stuff */
uint32 GetPropChunk (const IFFParser *, PackedID type, PackedID id, void *resultBuf, uint32 resultBufSize);
Err WriteLeafChunk (IFFParser *, PackedID id, const void *chunkData, uint32 chunkSize);

    /* general (not necessarily IFF) parsing */
Err ValidateFileConditionedTags (const TagArg tagArray[], int32 numTags, const uint32 invalidTagIDs[]);


#ifdef BUILD_STRINGS
    /* Debugging */
void IFFPrintf (const IFFParser *, const char *fmt, ...);
#endif


#ifdef __cplusplus
}
#endif /* __cplusplus */


/*****************************************************************************/


#endif /* __AUDIO_MUSIC_IFF_H */
