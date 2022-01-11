/* @(#) dumpnode.c 95/08/29 1.6 */

#include <kernel/types.h>
#include <kernel/nodes.h>
#include <kernel/list.h>
#include <stdio.h>


/*****************************************************************************/


/**
|||	AUTODOC -public -class Kernel -group Lists -name DumpNode
|||	Prints contents of a node.
|||
|||	  Synopsis
|||
|||	    void DumpNode (const Node *node, const char *banner)
|||
|||	  Description
|||
|||	    This function prints out the contents of a Node structure for debugging
|||	    purposes.
|||
|||	  Arguments
|||
|||	    node
|||	        The node to print.
|||
|||	    banner
|||	        Descriptive text to print before the node
|||	        contents. May be NULL.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libc.a V24.
|||
|||	  Associated Files
|||
|||	    <kernel/list.h>, libc.a
|||
**/

void DumpNode(const Node *n, const char *banner)
{
    if (banner)
	printf("%s: ",banner);

    printf("node @ $%08lx, subsys %u, type %u, ", n, n->n_SubsysType, n->n_Type);
    printf("size %d, pri %u, ",n->n_Size, n->n_Priority);

    if (n->n_Flags & NODE_NAMEVALID)
    {
        if (n->n_Name)
            printf("name '%s'\n",n->n_Name);
        else
            printf("<null name>\n");
    }
    else
    {
        printf("<unnamed>\n");
    }
}
