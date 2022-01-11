/* @(#) insertnodebefore.c 95/08/29 1.6 */

#include <kernel/types.h>
#include <kernel/nodes.h>
#include <kernel/list.h>


/*****************************************************************************/


/**
|||	AUTODOC -public -class Kernel -group Lists -name InsertNodeBefore
|||	Inserts a node into a list before another node already
|||	                   in the list.
|||
|||	  Synopsis
|||
|||	    void InsertNodeBefore(Node *oldNode, Node *newNode);
|||
|||	  Description
|||
|||	    This function lets you insert a new node into a list, BEFORE another node
|||	    that is already in the list.
|||
|||	  Arguments
|||
|||	    oldNode
|||	        The node before which to insert the new node.
|||
|||	    newNode
|||	        The node to insert in the list.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libc.a V24.
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
|||	    RemHead(), RemNode(), RemTail(), InsertNodeAfter()
|||
**/

void InsertNodeBefore(Node *oldNode, Node *newNode)
{
    newNode->n_Next         = oldNode;
    newNode->n_Prev         = oldNode->n_Prev;
    oldNode->n_Prev->n_Next = newNode;
    oldNode->n_Prev         = newNode;
}
