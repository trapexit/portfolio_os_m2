/* @(#) findAndOpenNamedItem.c 95/08/29 1.6 */

#include <kernel/types.h>
#include <kernel/nodes.h>
#include <kernel/item.h>

/**
|||	AUTODOC -public -class Kernel -group Items -name FindAndOpenNamedItem
|||	Finds an item by name and opens it.
|||
|||	  Synopsis
|||
|||	    Item FindAndOpenNamedItem(int32 ctype, const char *name);
|||
|||	  Description
|||
|||	    This function finds an item of the specified type and name.
|||	    The search is not case-sensitive. When an item is found, it is
|||	    opened and prepared for use.
|||
|||	  Arguments
|||
|||	    ctype
|||	        The type of the item to find. Use MkNodeID()
|||	        to create this value.
|||
|||	    name
|||	        The name of the item to find.
|||
|||	  Return Value
|||
|||	    Returns the number of the item that was opened, or a negative error
|||	    code for failure.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libc.a V24.
|||
|||	  Associated Files
|||
|||	    <kernel/item.h>, libc.a
|||
|||	  See Also
|||
|||	    FindAndOpenItem(), CloseItem()
|||
**/

Item FindAndOpenNamedItem(int32 cntype, const char *name)
{
TagArg tags[2];

    tags[0].ta_Tag = TAG_ITEM_NAME;
    tags[0].ta_Arg = (void *)name;
    tags[1].ta_Tag = 0;

    return FindAndOpenItem(cntype,tags);
}
