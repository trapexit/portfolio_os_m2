/* @(#) convertfp_tagdata.c 95/09/20 1.1 */

#include <kernel/tags.h>

/**
|||	AUTODOC -public -class Kernel -group Tags -name ConvertFP_TagData
|||	Store a floating point value in a TagArg.
|||
|||	  Synopsis
|||
|||	    TagData ConvertFP_TagData (float32 a);
|||
|||	  Description
|||
|||	    Prepares a floating point value for storing in a TagArg.
|||
|||	    Because TagData is defined as a pointer, casting a floating point to
|||	    TagData would cause a compiler error or truncation, neither of which is
|||	    the desired result. This function simply changes the type of the floating
|||	    point value without changing its contents. The function ConvertTagData_FP()
|||	    reverses the process.
|||
|||	  Arguments
|||
|||	    a
|||	        floating point value to store in a TagArg
|||
|||	  Return Value
|||
|||	    Floating point value converted to a TagData.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libc.a V27.
|||
|||	  Examples
|||
|||	    void dosomething (Item instrument, float32 amplitude)
|||	    {
|||	        StartInstrumentVA (instrument,
|||	            AF_TAG_AMPLITUDE_FP, ConvertFP_TagData(amplitude),
|||	            TAG_END);
|||	        .
|||	        .
|||	        .
|||	    }
|||
|||	  Caveats
|||
|||	    If you don't use this function when filling out a TagArg with a floating
|||	    point value, you will get a compiler error. If you forget to use it when
|||	    passing a floating point value to a variable argument tag function (e.g.
|||	    StartInstrumentVA()), you will get a corrupted tag list without any
|||	    compile time warning.
|||
|||	  Associated Files
|||
|||	    <kernel/tags.h>, libc.a
|||
|||	  See Also
|||
|||	    ConvertTagData_FP()
**/

TagData ConvertFP_TagData (float32 a)
{
    return *(TagData *)&a;
}
