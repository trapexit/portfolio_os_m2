/* @(#) getnodecount.c 95/08/29 1.6 */

#include <kernel/types.h>
#include <kernel/nodes.h>
#include <kernel/list.h>


/*****************************************************************************/


/**
|||	AUTODOC -public -class Kernel -group Lists -name GetNodeCount
|||	Counts the number of nodes in a list.
|||
|||	  Synopsis
|||
|||	    uint32 GetNodeCount(const List *l);
|||
|||	  Description
|||
|||	    This function counts the number of nodes currently in the list.
|||
|||	  Arguments
|||
|||	    l
|||	        A pointer to the list to count the nodes of.
|||
|||	  Return Value
|||
|||	    The number of nodes in the list.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libc.a V24.
|||
|||	  Associated Files
|||
|||	    <kernel/list.h>, libc.a
|||
|||	  See Also
|||
|||	    AddHead(), AddTail(), InsertNodeFromHead(), InsertNodeFromTail(),
|||	    RemHead(), RemNode(), RemTail()
|||
**/

uint32 GetNodeCount(const List *l)
{
uint32  result;
Node   *n;

    result = 0;
    SCANLIST(l,n,Node)
    {
        result++;
    }

    return result;
}
