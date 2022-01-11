/* @(#) list.c 96/03/11 1.8 */

#include <string.h>
#include <stdlib.h>
#include "utils.h"
#include "adx.h"


/*****************************************************************************/


#define l_Tail ListAnchor.tail.links.blink
#define l_Head ListAnchor.head.links.flink
#define l_Middle ListAnchor.head.links.blink


#define ADDHEAD(l,n) \
	{ \
	    Node *currenthead = FIRSTNODE(l); \
	    ((Node *)n)->n_Next = currenthead; \
	    ((Node *)n)->n_Prev = (Node *)&((List *)l)->l_Head; \
	    ((List *)l)->l_Head = (Link *)n; \
	    currenthead->n_Prev = (Node *)n; \
	}
#define ADDTAIL(l,n) \
	{ \
	    Node *currentlast = LASTNODE(l); \
	    ((List *)l)->l_Tail = (Link *)n; \
	    ((Node *)n)->n_Prev = currentlast; \
	    ((Node *)n)->n_Next = (Node *)&((List *)l)->l_Middle; \
	    currentlast->n_Next = (Node *)n; \
	}

/* RemoveNode with no return value */
#define REMOVENODE(n)	{ \
			    Node *r1 = (n)->n_Next; \
			    Node *r2 = (n)->n_Prev; \
			    r2->n_Next = r1; \
			    r1->n_Prev = r2; \
			}


/*****************************************************************************/


void PrepList(List *l)
{
    l->l_Flags                     = 0;
    l->ListAnchor.head.links.blink = 0;
    l->ListAnchor.head.links.flink = (Link*)(&l->ListAnchor.tail.links.flink);
    l->ListAnchor.tail.links.blink = (Link*)(&l->ListAnchor.head.links.flink);
}


/*****************************************************************************/


void AddHead( List *l, Node *new )
{
    ADDHEAD(l,new);
}


/*****************************************************************************/


void AddTail( List *l, Node *new )
{
    ADDTAIL(l,new);
}


/*****************************************************************************/


Node *RemHead(List *l)
{
Node *base;
Node *head;

    base = (Node *)&l->ListAnchor.head.links;
    head = base->n_Next;
    if (head->n_Next)
    {
        base->n_Next         = head->n_Next;
        head->n_Next->n_Prev = base;
        return head;
    }

    return NULL;
}


/*****************************************************************************/


Node *RemTail(List *l)
{
Node *base;
Node *tail;

    base = (Node *)&l->ListAnchor.tail.links;
    tail = base->n_Prev;
    if (tail->n_Prev)
    {
        base->n_Prev         = tail->n_Prev;
        tail->n_Prev->n_Next = base;
        return tail;
    }

    return NULL;
}


/*****************************************************************************/


void RemNode( Node *n )
{
    REMOVENODE(n);
}


/*****************************************************************************/


void InsertNodeBefore(Node *oldNode, Node *newNode)
{
    newNode->n_Next         = oldNode;
    newNode->n_Prev         = oldNode->n_Prev;
    oldNode->n_Prev->n_Next = newNode;
    oldNode->n_Prev         = newNode;
}


/*****************************************************************************/


void InsertNodeAlpha(List *l, Node *new)
{
Node *old;

    ScanList(l,old,Node)
    {
        if (strcasecmp(old->n_Name,new->n_Name) >= 0)
            break;
    }
    InsertNodeBefore(old, new);
}


/*****************************************************************************/


Node *FindNamedNode (const List *l, const char *s)
{
Node *n;
char *name;

    ScanList(l,n,Node)
    {
        name = n->n_Name;
        if (s && name)
        {
            if (strcasecmp(s, name) == 0)
                return n;
        }
        else if (s == name)      /* are both NULL? */
        {
            return n;
        }
    }

    return NULL;
}


/*****************************************************************************/


void *AllocNode(uint32 nodeSize, const char *name)
{
Node *n;

    if (name)
    {
        n = (Node *)malloc(nodeSize + strlen(name) + 1);
        if (n)
        {
            n->n_Name = (char *)((uint32)n + nodeSize);
            strcpy(n->n_Name,name);
        }
    }
    else
    {
        n = (Node *)malloc(nodeSize);
        if (n)
        {
            n->n_Name = NULL;
        }
    }

    return n;
}


/*****************************************************************************/


void FreeNode(void *n)
{
    free(n);
}
