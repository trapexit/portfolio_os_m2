/* @(#) getnodeposfromtail.c 95/08/29 1.6 */

#include <kernel/types.h>
#include <kernel/nodes.h>
#include <kernel/list.h>


/*****************************************************************************/


/**
|||	AUTODOC -public -class Kernel -group Lists -name GetNodePosFromTail
|||	Gets the ordinal position of a node within a list,
|||	                      counting from the tail of the list.
|||
|||	  Synopsis
|||
|||	    int32 GetNodePosFromTail(const List *l, const Node *n);
|||
|||	  Description
|||
|||	    This function scans the supplied list backward looking for the supplied
|||	    node, and returns the ordinal position of the node within the list. If the
|||	    node doesn't appear in the list, the function returns -1.
|||
|||	  Arguments
|||
|||	    l
|||	        A pointer to the list to scan for the node.
|||
|||	    n
|||	        A pointer to a node to locate in the list.
|||
|||	  Return Value
|||
|||	    The ordinal position of the node within the list counting from the tail of
|||	    the list, or -1 if the node isn't in the list. The last node in the list
|||	    has position 0, the second to last node has position 1, etc.
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

int32 GetNodePosFromTail(const List *l, const Node *n)
{
Node   *nn;
uint32  count;

    count = 0;
    SCANLISTB(l,nn,Node)
    {
        if (n == nn)
            return (int32)count;

        count++;
    }

    return -1;
}
