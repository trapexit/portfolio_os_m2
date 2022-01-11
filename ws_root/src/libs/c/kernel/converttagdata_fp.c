/* @(#) converttagdata_fp.c 95/09/20 1.1 */

#include <kernel/tags.h>

/**
|||	AUTODOC -public -class Kernel -group Tags -name ConvertTagData_FP
|||	Extract a floating point value stored in a TagArg.
|||
|||	  Synopsis
|||
|||	    float32 ConvertTagData_FP (TagData a);
|||
|||	  Description
|||
|||	    Extracts a floating point value stored in a TagArg.
|||
|||	  Arguments
|||
|||	    a
|||	        TagData value from ta_Arg of a TagArg.
|||
|||	  Return Value
|||
|||	    Floating point value stored in a.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libc.a V27.
|||
|||	  Examples
|||
|||	    void dosomething (const TagArg *tags)
|||	    {
|||	        float32 amplitude = ConvertTagData_FP (GetTagData (tags,
|||	                                AF_TAG_AMPLITUDE_FP, ConvertFP_TagData(0.0)));
|||	        .
|||	        .
|||	        .
|||	    }
|||
|||	  Associated Files
|||
|||	    <kernel/tags.h>, libc.a
|||
|||	  See Also
|||
|||	    ConvertFP_TagData()
**/

float32 ConvertTagData_FP (TagData a)
{
    return *(float32 *)&a;
}
