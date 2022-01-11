/* @(#) findnodefromtail.c 95/08/29 1.6 */

#include <kernel/types.h>
#include <kernel/nodes.h>
#include <kernel/list.h>


/*****************************************************************************/


/**
|||	AUTODOC -public -class Kernel -group Lists -name FindNodeFromTail
|||	Returns a pointer to a node appearing at a given
|||	                   ordinal position from the tail of the list.
|||
|||	  Synopsis
|||
|||	    Node *FindNodeFromTail(const List *l, uint32 position);
|||
|||	  Description
|||
|||	    This function scans the supplied list and returns a pointer to the node
|||	    appearing in the list at the given ordinal position counting from the end
|||	    of the list. NULL is returned if the list doesn't contain that many items.
|||
|||	  Arguments
|||
|||	    l
|||	        A pointer to the list to scan for the node.
|||
|||	    position
|||	        The node position to look for, relative to the end of the list.
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

Node *FindNodeFromTail(const List *l, uint32 position)
{
Node *n;

    SCANLISTB(l,n,Node)
    {
        if (!position)
            return n;

        position--;
    }

    return NULL;
}
