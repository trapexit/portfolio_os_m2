#ifndef __KERNEL_LIST_H
#define __KERNEL_LIST_H


/******************************************************************************
**
**  @(#) list.h 95/11/26 1.13
**
**  Kernel list management definitions
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_NODES_H
#include <kernel/nodes.h>
#endif


/*****************************************************************************/


typedef struct Link
{
	struct Link *flink;	/* forward (next) link */
	struct Link *blink;	/* backward (prev) link */
} Link;

typedef union ListAnchor
{
    struct			/* ptr to first node */
    {				/* anchor for lastnode */
	Link links;
	Link *filler;
    } head;
    struct
    {
	Link *filler;
	Link links;		/* ptr to lastnode */
    } tail;			/* anchore for firstnode */
} ListAnchor;

typedef struct List
{
	uint32     l_Flags;	/* Dale's unused flags */
	ListAnchor ListAnchor;	/* Anchor point for list of nodes */
} List, *ListP;

#define l_Head   ListAnchor.head.links.flink
#define l_Filler ListAnchor.head.links.blink
#define l_Tail   ListAnchor.tail.links.blink


/*****************************************************************************/


/* return the first node on the list or the anchor if empty */
#define FirstNode(l)	((Node *)((l)->l_Head))
#define FIRSTNODE(l)	((Node *)((l)->l_Head))

/* return the last node on the list or the anchor if empty */
#define LastNode(l)	((Node *)((l)->l_Tail))
#define LASTNODE(l)	((Node *)((l)->l_Tail))

/* for finding end while walking a list from head to tail */
#define IsNode(l,n)	(((Link**)(n)) != &((l)->l_Filler))
#define ISNODE(l,n)	(((Link**)(n)) != &((l)->l_Filler))

/* for finding end while walking a list from tail to head */
#define IsNodeB(l,n)	(((Link**)(n)) != &((l)->l_Head))
#define ISNODEB(l,n)	(((Link**)(n)) != &((l)->l_Head))

/* determine if a list is empty */
#define IsListEmpty(l)	(!IsNode((l),FirstNode(l)))
#define IsEmptyList(l)	(!IsNode((l),FirstNode(l)))
#define ISLISTEMPTY(l)	(!IsNode((l),FirstNode(l)))
#define ISEMPTYLIST(l)	(!IsNode((l),FirstNode(l)))

/* get the following node, or the anchor if already at last node */
#define NextNode(n)	(((Node *)(n))->n_Next)
#define NEXTNODE(n)	(((Node *)(n))->n_Next)

/* get the previous node, or the anchor if already at first node */
#define PrevNode(n)	(((Node *)(n))->n_Prev)
#define PREVNODE(n)	(((Node *)(n))->n_Prev)

/* Scan list l, for nodes n, of type t.
 *
 * WARNING: You cannot remove the current node from the list when using this
 *          macro.
 */
#define ScanList(l,n,t) for (n=(t *)FirstNode(l);IsNode(l,n);n=(t *)NextNode(n))
#define SCANLIST(l,n,t) for (n=(t *)FirstNode(l);IsNode(l,n);n=(t *)NextNode(n))

/* Scan a list backward, from tail to head */
#define ScanListB(l,n,t) for (n=(t *)LastNode(l);IsNodeB(l,n);n=(t *)PrevNode(n))
#define SCANLISTB(l,n,t) for (n=(t *)LastNode(l);IsNodeB(l,n);n=(t *)PrevNode(n))

/* A macro to let you define statically initialized lists. You use this macro
 * like this:
 *
 *   static List l = PREPLIST(l);
 *
 * or in a structure, you can do:
 *
 *   typedef struct Foo
 *   {
 *       uint32 x;
 *       List   l;
 *       uint32 y;
 *   } Foo;
 *
 *   Foo z = {0, PREPLIST(z.l), 2};
 *
 * Both cases initialize the List structures statically which means you
 * don't need to call the PrepList() function separately.
 */
#define PREPLIST(l) {0,{(Link *)&l.l_Filler,NULL,(Link *)&l.l_Head}}

/* These are for backward source compatibility, do not use in new code */
#define INITLIST(l,dummy) PREPLIST(l)
#define InitList(l,dummy) PrepList(l)


/*****************************************************************************/


#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */


/* initialize a list to the empty list */
void PrepList(List *l);

/* add a node to the head of a list */
void AddHead(List *l, Node *n);

/* add a node to the end of a list */
void AddTail(List *l, Node *n);

/* insert a node in the prioritized list starting from the head */
void InsertNodeFromHead(List *l, Node *n);

/* insert a node in the prioritized list starting from the tail */
void InsertNodeFromTail(List *l, Node *n);

/* set the priority of a node in a list */
uint8 SetNodePri(Node *n, uint8 newpri);	/* returns old pri */

/* insert a node in the prioritized list using (*f)() as a cmp func */
void UniversalInsertNode(List *l, Node *n, bool (*f)(Node *n1, Node *n2));

/* remove the first node on the list, return a ptr to it */
Node *RemHead(List *l);

/* remove the last node on the list, return a ptr to it */
Node *RemTail(List *l);

/* remove a node from a list */
void RemNode(Node *n);

/* find a Node in a List with the name of <name> */
Node *FindNamedNode(const List *l, const char *name);

/* convenience routines */
uint32 GetNodeCount(const List *l);
void InsertNodeBefore(Node *oldNode, Node *newNode);
void InsertNodeAfter(Node *oldNode, Node *newNode);
void InsertNodeAlpha(List *l, Node *n);
Node *FindNodeFromHead(const List *l, uint32 position);
Node *FindNodeFromTail(const List *l, uint32 position);
int32 GetNodePosFromHead(const List *l, const Node *n);
int32 GetNodePosFromTail(const List *l, const Node *n);

/* debugging aid */
void DumpNode(const Node *n, const char *banner);


#ifdef  __cplusplus
}
#endif  /* __cplusplus */


/*****************************************************************************/


#ifdef __DCC__
#pragma no_side_effects GetNodePosFromHead, GetNodePosFromTail, GetNodeCount
#pragma no_side_effects FindNodePosFromHead, FindNodePosFromTail
#pragma no_side_effects FindNamedNode
#pragma no_side_effects PrepList(1), DumpNode
#endif


/*****************************************************************************/


#endif /* __KERNEL_LIST_H */
