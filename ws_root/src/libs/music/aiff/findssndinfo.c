/* @(#) findssndinfo.c 96/02/23 1.4 */

#include <audio/aiff_format.h>
#include <misc/iff.h>

/**
|||	AUTODOC -private -class libmusic -group AIFF -name FindSSNDInfo
|||	Finds SSNDInfo stored by HandleSSND().
|||
|||	  Synopsis
|||
|||	    const SSNDInfo *FindSSNDInfo (const IFFParser *iff, PackedID formType)
|||
|||	  Description
|||
|||	    Finds SSNDInfo stored by HandleSSND().
|||
|||	  Arguments
|||
|||	    iff
|||	        IFFParser to query.
|||
|||	    formType
|||	        Form type of the context containing the SSND: ID_AIFF or ID_AIFC.
|||
|||	  Return Value
|||
|||	    Pointer to SSNDInfo, or NULL if not found. The pointer remains valid until
|||	    the context which contains it is purged.
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
|||	    HandleSSND()
**/
const SSNDInfo *FindSSNDInfo (const IFFParser *iff, PackedID formType)
{
    const ContextInfo *ci;

    return (ci = FindContextInfo (iff, formType, ID_SSND, ID_SSND))
        ? (SSNDInfo *)ci->ci_Data
        : NULL;
}
