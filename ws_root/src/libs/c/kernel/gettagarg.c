/* @(#) gettagarg.c 95/08/29 1.6 */

#include <kernel/types.h>
#include <kernel/tags.h>

/**
|||	AUTODOC -public -class Kernel -group Tags -name GetTagArg
|||	Finds a TagArg in list and returns its ta_Arg field.
|||
|||	  Synopsis
|||
|||	    TagData GetTagArg(const TagArg *tagList, uint32 tag,
|||	                      TagData defaultValue);
|||
|||	  Description
|||
|||	    This function calls FindTagArg() to locate the specified tag. If it
|||	    is found, it returns the ta_Arg value from the found TagArg.
|||	    Otherwise, it returns the default value supplied. This is handy when
|||	    resolving a tag list that has optional tags that have suitable
|||	    default values.
|||
|||	  Arguments
|||
|||	    tagList
|||	        The list of tags to scan. Can be NULL.
|||
|||	    tag
|||	        The tag ID to look for.
|||
|||	    defaultValue
|||	        Default value to use when specified tag isn't found in tagList.
|||
|||	  Return Value
|||
|||	    ta_Arg value from found TagArg or defaultValue.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libc.a V24.
|||
|||	  Examples
|||
|||	    void dosomething (const TagArg *tags)
|||	    {
|||	        uint32 amplitude = (uint32)GetTagData (tags, MY_TAG_AMPLITUDE, (TagData)0x7fff);
|||	        .
|||	        .
|||	        .
|||	    }
|||
|||	  Caveats
|||
|||	    It's a good idea to always use casts for the default value and
|||	    result. Don't assume anything about the type definition of
|||	    TagData other than that it is a 32-bit value.
|||
|||	  Associated Files
|||
|||	    <kernel/tags.h>, libc.a
|||
|||	  See Also
|||
|||	    FindTagArg(), NextTagArg()
|||
**/

TagData GetTagArg (const TagArg *ta, uint32 tag, TagData defaultval)
{
    return ((ta = FindTagArg (ta, tag)) != NULL) ? ta->ta_Arg : defaultval;
}
