#ifndef __AUDIO_ATAG_H
#define __AUDIO_ATAG_H


/******************************************************************************
**
**  @(#) atag.h 95/11/14 1.4
**
**  ATAG File Definitions and Loader
**
******************************************************************************/

/*
    This format is IFF-like in that it has chunks, but is not actually an IFF
    FORM. This is because this format is meant to be as simple to parse as
    possible. Because of its IFF-like chunk nature, such a file can be embedded
    directly into an IFF FORM and parsed using the IFF folio.

    {
            // header
        PackedID 'ATAG'
        uint32   size of AudioTagHeader
        AudioTagHeader

            // data
        PackedID 'BODY'
        uint32   size of body data in bytes. Use 0 size when no body data
                 is present
        uint8 data [size]
    }
*/

#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __STDDEF_H
#include <stddef.h>     /* offsetof() */
#endif


/* -------------------- IDs and structures */

#define ID_ATAG MAKE_ID('A','T','A','G')
#ifndef ID_BODY
#define ID_BODY MAKE_ID('B','O','D','Y')
#endif

typedef struct AudioTagHeader {
    uint8    athd_NodeType;         /* AUDIO_<type>_NODE value */
    uint8    athd_Reserved0;        /* room for future expansion. Must be 0 for now. */
    uint16   athd_NumTags;          /* number of tags in tag array (including TAG_END) */
    uint32   athd_Reserved1;        /* room for future expansion. Must be 0 for now. */
    TagArg   athd_Tags[1];          /* really athd_Tags[athd_NumTags]. Audio TagArg array
                                       terminated by TAG_END. Not allowed to contain
                                       AF_TAG_ADDRESS, TAG_JUMP, or early TAG_END. */
} AudioTagHeader;

#define AudioTagHeaderSize(numTags) (offsetof (AudioTagHeader, athd_Tags) + (numTags) * sizeof (TagArg))


/* -------------------- Functions */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    /* General purpose parser support */
Err ValidateAudioTagHeader (const AudioTagHeader *, uint32 atagsize);

    /* ATAG file loader */
Item LoadATAG (const char *filename);

#ifdef __cplusplus
}
#endif /* __cplusplus */


/*****************************************************************************/


#endif /* __AUDIO_ATAG_H */
