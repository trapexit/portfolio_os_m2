/* @(#) insertnodealpha.c 95/08/29 1.2 */

#include <kernel/types.h>
#include <kernel/list.h>
#include <string.h>


/*****************************************************************************/


/**
|||	AUTODOC -public -class Kernel -group Lists -name InsertNodeAlpha
|||	Inserts a node into a list according to alphabetical order.
|||
|||	  Synopsis
|||
|||	    void InsertNodeAlpha(List *l, Node *n);
|||
|||	  Description
|||
|||	    This function lets you insert a new node into a list, according
|||	    to alphabetical order of the name of the node. The list is assumed
|||	    to already be sorted in alphabetical order.
|||
|||	  Arguments
|||
|||	    l
|||	        A pointer to the list into which to insert the node.
|||
|||	    n
|||	        A pointer to the node to insert.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libc.a V27.
|||
|||	  Associated Files
|||
|||	    <kernel/list.h>, libc.a
|||
|||	  Notes
|||
|||	    A node can be included only in one list.
|||
|||	  See Also
|||
|||	    AddHead(), AddTail(), InsertNodeFromHead(), InsertNodeFromTail(),
|||	    RemHead(), RemNode(), RemTail(), InsertNodeAfter(),
|||	    InsertNodeBefore()
|||
**/

void InsertNodeAlpha(List *l, Node *n)
{
Node *old;

    ScanList(l,old,Node)
    {
        if (strcasecmp(old->n_Name,n->n_Name) >= 0)
            break;
    }
    InsertNodeBefore(old, n);
}
