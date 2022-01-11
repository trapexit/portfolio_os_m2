/* @(#) findnodefromhead.c 95/08/29 1.6 */

#include <kernel/types.h>
#include <kernel/nodes.h>
#include <kernel/list.h>


/*****************************************************************************/


/**
|||	AUTODOC -public -class Kernel -group Lists -name FindNodeFromHead
|||	Returns a pointer to a node appearing at a given
|||	                   ordinal position from the head of the list.
|||
|||	  Synopsis
|||
|||	    Node *FindNodeFromHead(const List *l, uint32 position);
|||
|||	  Description
|||
|||	    This function scans the supplied list and returns a pointer to the node
|||	    appearing in the list at the given ordinal position. NULL is returned if
|||	    the list doesn't contain that many items.
|||
|||	  Arguments
|||
|||	    l
|||	        A pointer to the list to scan for the node.
|||
|||	    position
|||	        The node position to look for.
|||
|||	  Return Value
|||
|||	    A pointer to the node found, or NULL if the list doesn't contain enough
|||	    nodes.
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

Node *FindNodeFromHead(const List *l, uint32 position)
{
Node *n;

    SCANLIST(l,n,Node)
    {
        if (!position)
            return n;

        position--;
    }

    return NULL;
}
