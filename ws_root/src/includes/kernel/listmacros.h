#ifndef __KERNEL_LISTMACROS_H
#define __KERNEL_LISTMACROS_H


/******************************************************************************
**
**  @(#) listmacros.h 96/02/20 1.7
**
**  List routines in macro format
**
******************************************************************************/


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

#endif /* __KERNEL_LISTMACROS_H */
