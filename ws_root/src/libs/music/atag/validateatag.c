/******************************************************************************
**
**  @(#) validateatag.c 95/12/12 1.7
**
**  Validate ATAG chunk contents.
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
**  951108 WJB  Fixed triple bangs.
**
**  Initials:
**
**  WJB: Bill Barton (peabody)
**  PLB: Phil Burk (phil)
**
******************************************************************************/

#include <audio/atag.h>         /* self */
#include <audio/audio.h>
#include <audio/musicerror.h>
#include <audio/music_iff.h>    /* ValidateFileConditionedTags() */
#include <stddef.h>             /* offsetof() */


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

/**
|||	AUTODOC -public -class libmusic -group ATAG -name ValidateAudioTagHeader
|||	Validate the contents of an AudioTagHeader (ATAG chunk).
|||
|||	  Synopsis
|||
|||	    Err ValidateAudioTagHeader (const AudioTagHeader *atag, uint32 atagSize)
|||
|||	  Description
|||
|||	    This function validates the contents of an AudioTagHeader read from an ATAG
|||	    chunk. Checks the following things:
|||
|||	    - Whether atagSize is large enough for complete AudioTagHeader and all of
|||	    the tags specified in athd_NumTags.
|||
|||	    - That there is at least 1 tag.
|||
|||	    - The tag list contains no invalid tags: TAG_JUMP, TAG_END, AF_TAG_ADDRESS,
|||	    AF_TAG_AUTO_FREE_DATA.
|||
|||	    - The tag list is terminated by a TAG_END.
|||
|||	  Arguments
|||
|||	    atag
|||	        Pointer to AudioTagHeader (ATAG chunk)
|||
|||	    atagSize
|||	        Size of the ATAG chunk in bytes.
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
|||	    <audio/atag.h>, libmusic.a
|||
|||	  See Also
|||
|||	    --ATAG-File-Format--(@), LoadATAG()
**/
Err ValidateAudioTagHeader (const AudioTagHeader *atag, uint32 atagsize)
{
        /*
            Make sure chunk is at long enough:
                . to have athd_NumTags defined
                . to hold all of the tags indicated by athd_NumTags
        */
  #if DEBUG_Load
    printf ("\nValidateAudioTagHeader: atag=0x%x %d bytes\n", atag, atagsize);
  #endif
    if ( atagsize < AudioTagHeaderSize (1) ||
         atagsize < AudioTagHeaderSize (atag->athd_NumTags) ) {

        ERR(("ValidateAudioTagHeader: short ATAG (%d bytes)\n", atagsize));
        return ML_ERR_MANGLED_FILE;
    }

  #if DEBUG_Load
    printf ("type=%d NumTags=%d\n", atag->athd_NodeType, atag->athd_NumTags);
  #endif

    {
        static const uint32 invalidTagIDs[] = {
            AF_TAG_ADDRESS,         /* has a pointer - dangerous */
            AF_TAG_AUTO_FREE_DATA,  /* conflicts with loader */
            TAG_END
        };

        return ValidateFileConditionedTags (atag->athd_Tags, atag->athd_NumTags, invalidTagIDs);
    }
}
