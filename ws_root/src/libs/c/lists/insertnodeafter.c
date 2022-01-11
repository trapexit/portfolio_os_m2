/* @(#) insertnodeafter.c 95/08/29 1.6 */

#include <kernel/types.h>
#include <kernel/nodes.h>
#include <kernel/list.h>


/*****************************************************************************/


/**
|||	AUTODOC -public -class Kernel -group Lists -name InsertNodeAfter
|||	Inserts a node into a list after another node already in
|||	                  the list.
|||
|||	  Synopsis
|||
|||	    void InsertNodeAfter(Node *oldNode, Node *newNode);
|||
|||	  Description
|||
|||	    This function lets you insert a new node into a list, AFTER another node
|||	    that is already in the list.
|||
|||	  Arguments
|||
|||	    oldNode
|||	        The node after which to insert the new node.
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
|||	    RemHead(), RemNode(), RemTail(), InsertNodeBefore()
|||
**/

void InsertNodeAfter(Node *oldNode, Node *newNode)
{
    newNode->n_Prev         = oldNode;
    newNode->n_Next         = oldNode->n_Next;
    oldNode->n_Next->n_Prev = newNode;
    oldNode->n_Next         = newNode;
}
