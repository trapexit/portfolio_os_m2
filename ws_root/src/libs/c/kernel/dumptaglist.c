/* @(#) dumptaglist.c 95/08/29 1.7 */

#include <kernel/tags.h>
#include <stdio.h>

/**
|||	AUTODOC -public -class Kernel -group Tags -name DumpTagList
|||	Prints the contents of a tag list.
|||
|||	  Synopsis
|||
|||	    void DumpTagList(const TagArg *tagList, const char *desc);
|||
|||	  Description
|||
|||	    This function prints out the contents of a TagArg list to the
|||	    debugging terminal.
|||
|||	  Arguments
|||
|||	    tagList
|||	        The list of tags to print.
|||
|||	    desc
|||	        Description of tag list to print. Can be NULL.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libc.a V24.
|||
|||	  Associated Files
|||
|||	    <kernel/tags.h>, libc.a
|||
|||	  See Also
|||
|||	    NextTagArg(), GetTagArg()
|||
**/

void DumpTagList (const TagArg *tags, const char *desc)
{
    const TagArg *tstate, *t;

    if (desc) printf ("%s: ", desc);
    printf ("tag list @ $%08lx\n", tags);

    for (tstate = tags; (t = NextTagArg (&tstate)) != NULL; ) {
        printf ("{ 0x%02lx (%3lu), 0x%08lx (%ld) }\n", t->ta_Tag, t->ta_Tag, (uint32)t->ta_Arg, (int32)t->ta_Arg);
    }
}
