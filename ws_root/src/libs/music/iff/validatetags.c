/****************************************************************************
**
**  @(#) validatetags.c 96/02/23 1.2
**
****************************************************************************/

#include <audio/musicerror.h>
#include <audio/music_iff.h>        /* Self */


/* -------------------- Debug */

#define DEBUG_Load  0
#if DEBUG_Load
#include <stdio.h>
#endif


/* -------------------- Diagnostics */

#ifdef BUILD_STRINGS
#include <stdio.h>
#define ERR(x) printf x
#else
#define ERR(x)
#endif


/* -------------------- Code */

static bool IsTagInList (const uint32 invalidTagIDs[], uint32 matchTag);

/**
|||	AUTODOC -private -class libmusic -group IFF -name ValidateFileConditionedTags
|||	Validates an array of TagArgs conditioned for storage in a file.
|||
|||	  Synopsis
|||
|||	    Err ValidateFileConditionedTags (const TagArg tagArray[], int32 numTags,
|||	                                     const uint32 invalidTagIDs[])
|||
|||	  Description
|||
|||	    This function validates an array of TagArgs conditioned for storage in a
|||	    file:
|||
|||	    - Make sure thare are >0 tags.
|||
|||	    - make sure certain TAG_JUMP, premature TAG_END, and tags listed in
|||	    invalidTagIDs[] are not present in tagArray[].
|||
|||	    - make sure last tag in tagArray[] is TAG_END.
|||
|||	  Arguments
|||
|||	    tagArray
|||	        TagArg array to check.
|||
|||	    numTags
|||	        Number of elements in tagArray[], including terminating TAG_END.
|||
|||	    invalidTagIDs
|||	        Optional array of tag IDs which are considered illegal, or NULL. When
|||	        supplied this array should be terminated with TAG_END.
|||
|||	  Return Value
|||
|||	    The procedure returns a non-negative value on success, or a negative
|||	    error code on failure.
|||
|||	  Implementation
|||
|||	    Library function implemented in libmusic.a V29.
|||
|||	  Associated Files
|||
|||	    <audio/music_iff.h>, libmusic.a
|||
|||	    !!! this one probably doesn't belong in <audio/music_iff.h> because it isn't
|||	    strictly IFF related.
**/
Err ValidateFileConditionedTags (const TagArg tagArray[], int32 numTags, const uint32 invalidTagIDs[])
{
  #if DEBUG_Load
    printf ("ValidateFileConditionedTags() 0x%x, %d tags\n", tagArray, numTags);
  #endif

        /* check tag array size */
    if (numTags <= 0) return ML_ERR_MANGLED_FILE;

        /* check tag array */
    {
        const TagArg * const tagEnd = &tagArray[numTags];
        const TagArg *tag;

            /* scan all tags but last one for bogus tags */
        for (tag = tagArray; tag < tagEnd-1; tag++) {
          #if DEBUG_Load
            printf ("{ %d, 0x%x }\n", tag->ta_Tag, tag->ta_Arg);
          #endif
            switch (tag->ta_Tag) {
                default:
                        /* search invalidTagIDs, if list was provided */
                    if ( !( invalidTagIDs && IsTagInList (invalidTagIDs, tag->ta_Tag) ) ) break;
                    /* fall thru if it is in the list */

                case TAG_JUMP:              /* requires a pointer - dangerous */
                case TAG_END:               /* premature end - illegal. use NOPs for padding if necessary */
                    ERR(("ValidateFileConditionedTags: bad tag { %d, 0x%x }\n", tag->ta_Tag, tag->ta_Arg));
                    return ML_ERR_BADTAG;

                /* all the rest are OK */
            }
        }

            /* make sure last tag is TAG_END */
      #if DEBUG_Load
        printf ("{ %d, 0x%x }\n", tag->ta_Tag, tag->ta_Arg);
      #endif
        if ( tag->ta_Tag != TAG_END ) {
            ERR(("ValidateFileConditionedTags: unterminated tag list\n"));
            return ML_ERR_MANGLED_FILE;
        }
    }

    return 0;
}

/*
    Return TRUE if matchTag is in searchTagIDs

    Arguments
        searchTagIDs
            Array of tag ID (not TagArgs) to search. Terminate
            with TAG_END.

        matchTag
            Tag ID to find. TAG_END will not be found.

    Results
        TRUE if matchTag is in searchTagIDs. FALSE otherwise.
*/
static bool IsTagInList (const uint32 searchTagIDs[], uint32 matchTag)
{
    const uint32 *tagp;
    uint32 tag;

    for (tagp = searchTagIDs; tag = *tagp++; ) {
        if (tag == matchTag) return TRUE;
    }

    return FALSE;
}
