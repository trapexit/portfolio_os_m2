/* @(#) list.c 96/03/01 1.26 */

#include <kernel/types.h>
#include <kernel/list.h>
#include <kernel/nodes.h>
#include <kernel/kernelnodes.h>
#include <kernel/listmacros.h>
#include <string.h>


/*****************************************************************************/


#ifdef BUILD_PARANOIA
#define BUILD_LISTDEBUG
#endif


/*****************************************************************************/


/**
|||	AUTODOC -public -class Kernel -group Lists -name PrepList
|||	Initializes a list.
|||
|||	  Synopsis
|||
|||	    void PrepList( List *l )
|||
|||	  Description
|||
|||	    When you create a List structure, you must initialize it with a
|||	    call to PrepList() before using it. PrepList() creates an empty
|||	    list by initializing the head (beginning-of-list) and tail
|||	    (end-of-list) anchors.
|||
|||	    You must initialize all List structures with PrepList() or
|||	    PREPLIST() before using them with any other List function
|||
|||	  Arguments
|||
|||	    l
|||	        A pointer to the list to be initialized.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V27.
|||
|||	  Associated Files
|||
|||	    <kernel/list.h>, libc.a
|||
|||	  See Also
|||
|||	    AddHead(), AddTail(), InsertNodeFromHead(), InsertNodeFromTail(),
|||	    IsEmptyList(), RemHead(), RemNode(), RemTail(), UniversalInsertNode()
|||
**/

void PrepList(List *l)
{
    l->l_Flags  = 0;
    l->l_Head   = (Link *)&l->l_Filler;
    l->l_Filler = NULL;
    l->l_Tail   = (Link *)&l->l_Head;
}


/*****************************************************************************/


/**
|||	AUTODOC -public -class Kernel -group Lists -name AddHead
|||	Adds a node to the head of a list.
|||
|||	  Synopsis
|||
|||	    void AddHead( List *l, Node *n )
|||
|||	  Description
|||
|||	    This function adds a node to the head (the beginning) of the
|||	    specified list.
|||
|||	  Arguments
|||
|||	    l
|||	        A pointer to the list in which to add the node.
|||
|||	    n
|||	        A pointer to the node to add.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/list.h>, libc.a
|||
|||	  Caveats
|||
|||	    Attempting to insert a node into a list while it is a member of
|||	    another list is not reported as an error, and will trash the other
|||	    list.
|||
|||	  See Also
|||
|||	    AddTail(), PrepList(), InsertNodeFromHead(), InsertNodeFromTail(),
|||	    RemHead(), RemNode(), RemTail(), UniversalInsertNode(),
|||	    InsertNodeAlpha()
|||
**/

void AddHead(List *l, Node *new)
{
    ADDHEAD(l,new);
}


/*****************************************************************************/


/**
|||	AUTODOC -public -class Kernel -group Lists -name AddTail
|||	Adds a node to the tail of a list.
|||
|||	  Synopsis
|||
|||	    void AddTail( List *l, Node *n )
|||
|||	  Description
|||
|||	    This function adds the specified node to the tail (the end) of the
|||	    specified list.
|||
|||	  Arguments
|||
|||	    l
|||	        A pointer to the list in which to add the node.
|||
|||	    n
|||	        A pointer to the node to add.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/list.h>, libc.a
|||
|||	  Caveats
|||
|||	    Attempting to insert a node into a list while it is a member of
|||	    another list is not reported as an error, and will trash the other
|||	    list.
|||
|||	  See Also
|||
|||	    AddHead(), PrepList(), InsertNodeFromHead(), InsertNodeFromTail(),
|||	    RemHead(), RemNode(), RemTail(), UniversalInsertNode(),
|||	    InsertNodeAlpha()
|||
**/

void AddTail(List *l, Node *new)
{
    ADDTAIL(l,new);
}


/*****************************************************************************/


/**
|||	AUTODOC -public -class Kernel -group Lists -name RemHead
|||	Removes the first node from a list.
|||
|||	  Synopsis
|||
|||	    Node *RemHead( List *l )
|||
|||	  Description
|||
|||	    This function removes the head (first) node from a list.
|||
|||	    In a development environment, the link fields of the node
|||	    structure are modified so that an attempt to remove the same
|||	    node a second time will cause your task to crash due to
|||	    accessing non-existant memory. The address of this access gives
|||	    you a bit of information about what happened. If the address
|||	    is 0xABADC0DE or 0xABADFACE, it means that the node was first
|||	    removed from the list by a call to RemNode(). If the address of
|||	    the access is 0xAF00DBAD or 0xAFEEDBAD, the node was first
|||	    removed from the list using RemHead(). And finally, if the
|||	    address of the access is 0xABADF00D or 0xABADFEED, the node
|||	    was first removed using RemTail().
|||
|||	  Arguments
|||
|||	    l
|||	        A pointer to the list to be beheaded.
|||
|||	  Return Value
|||
|||	    The function returns a pointer to the node that was removed from
|||	    the list or NULL if the list was empty.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/list.h>, libc.a
|||
|||	  See Also
|||
|||	    AddHead(), AddTail(), InsertNodeFromTail(), RemNode(), RemTail()
|||
**/

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

#ifdef BUILD_LISTDEBUG
        head->n_Next = (void *)0xaf00dbad;
        head->n_Prev = (void *)0xafeedbad;
#endif

        return head;
    }

    return NULL;
}


/*****************************************************************************/


/**
|||	AUTODOC -public -class Kernel -group Lists -name RemTail
|||	Removes the last node from a list.
|||
|||	  Synopsis
|||
|||	    Node *RemTail( List *l )
|||
|||	  Description
|||
|||	    This function removes the tail (last) node from a list.
|||
|||	    In a development environment, the link fields of the node
|||	    structure are modified so that an attempt to remove the same
|||	    node a second time will cause your task to crash due to
|||	    accessing non-existant memory. The address of this access gives
|||	    you a bit of information about what happened. If the address
|||	    is 0xABADC0DE or 0xABADFACE, it means that the node was first
|||	    removed from the list by a call to RemNode(). If the address of
|||	    the access is 0xAF00DBAD or 0xAFEEDBAD, the node was first
|||	    removed from the list using RemHead(). And finally, if the
|||	    address of the access is 0xABADF00D or 0xABADFEED, the node
|||	    was first removed using RemTail().
|||
|||	  Arguments
|||
|||	    l
|||	        A pointer to the list to have it tail removed.
|||
|||	  Return Value
|||
|||	    The function returns a pointer to the node that was removed from the list
|||	    or NULL if the list was empty.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/list.h>, libc.a
|||
|||	  See Also
|||
|||	    AddHead(), AddTail(), InsertNodeFromTail(), RemHead(), RemNode()
|||
**/

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

#ifdef BUILD_LISTDEBUG
        tail->n_Next = (void *)0xabadf00d;
        tail->n_Prev = (void *)0xabadfeed;
#endif

        return tail;
    }

    return NULL;
}


/*****************************************************************************/


/**
|||	AUTODOC -public -class Kernel -group Lists -name RemNode
|||	Removes a node from a list.
|||
|||	  Synopsis
|||
|||	    void RemNode( Node *n )
|||
|||	  Description
|||
|||	    This function removes the specified node from a list.
|||
|||	    In a development environment, the link fields of the node
|||	    structure are modified so that an attempt to remove the same
|||	    node a second time will cause your task to crash due to
|||	    accessing non-existant memory. The address of this access gives
|||	    you a bit of information about what happened. If the address
|||	    is 0xABADC0DE or 0xABADFACE, it means that the node was first
|||	    removed from the list by a call to RemNode(). If the address of
|||	    the access is 0xAF00DBAD or 0xAFEEDBAD, the node was first
|||	    removed from the list using RemHead(). And finally, if the
|||	    address of the access is 0xABADF00D or 0xABADFEED, the node
|||	    was first removed using RemTail().
|||
|||	  Arguments
|||
|||	    n
|||	        A pointer to the node to remove.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/list.h>, libc.a
|||
|||	  See Also
|||
|||	    AddHead(), AddTail(), InsertNodeFromTail(), RemHead(), RemTail()
|||
**/

void RemNode(Node *n)
{
    REMOVENODE(n);

#ifdef BUILD_LISTDEBUG
    n->n_Next = (void *)0xabadc0de;
    n->n_Prev = (void *)0xabadface;
#endif
}


/*****************************************************************************/


/**
|||	AUTODOC -public -class Kernel -group Lists -name InsertNodeFromHead
|||	Inserts a node into a list.
|||
|||	  Synopsis
|||
|||	    void InsertNodeFromHead( List *l, Node *n )
|||
|||	  Description
|||
|||	    This function inserts a new node into a list. The order of nodes in
|||	    a list is often determined by their priority. The function compares
|||	    the priority of the new node to the priorities of nodes currently
|||	    in the list, beginning at the head of the list, and inserts the new
|||	    node immediately after all nodes whose priority is higher. If the
|||	    priorities of all the nodes in the list are higher, the node is
|||	    added at the end of the list.
|||
|||	    To arrange the nodes in a list by a value or values other than
|||	    priority, use UniversalInsertNode().
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
|||	    Folio call implemented in Kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/list.h>, libc.a
|||
|||	  See Also
|||
|||	    AddHead(), AddTail(), RemHead(), RemNode(), RemTail(),
|||	    InsertNodeFromTail(), UniversalInsertNode(), InsertNodeBefore(),
|||	    InsertNodeAfter()
|||
**/

void InsertNodeFromHead( List *l, Node *new )
{
Node *n;

    ScanList(l,n,Node)
    {
        if (new->n_Priority >= n->n_Priority)
        {
            new->n_Next         = n;
            new->n_Prev         = n->n_Prev;
            new->n_Prev->n_Next = new;
            n->n_Prev           = new;
            return;
        }
    }
    ADDTAIL(l,new);
}


/*****************************************************************************/


/**
|||	AUTODOC -public -class Kernel -group Lists -name InsertNodeFromTail
|||	Inserts a node into a list.
|||
|||	  Synopsis
|||
|||	    void InsertNodeFromTail( List *l, Node *n )
|||
|||	  Description
|||
|||	    This function inserts a new node into a list. The order of nodes in
|||	    a list is often determined by their priority. The function compares
|||	    the priority of the new node to the priorities of nodes currently in
|||	    the list, beginning at the tail of the list, and inserts the new
|||	    node immediately before the nodes whose priority is lower. If there
|||	    are no nodes in the list whose priority is lower, the node is added
|||	    at the head of the list.
|||
|||	    To arrange the nodes in a list by a value or values other than
|||	    priority, use UniversalInsertNode().
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
|||	    Folio call implemented in Kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/list.h>, libc.a
|||
|||	  See Also
|||
|||	    AddHead(), AddTail(), RemHead(), RemNode(), RemTail(),
|||	    InsertNodeFromHead(), UniversalInsertNode(), InsertNodeBefore()
|||	    InsertNodeAfter()
|||
**/

void InsertNodeFromTail(List *l, Node *new)
{
Node *n;

    ScanListB(l,n,Node)
    {
        if (n->n_Priority >= new->n_Priority)
        {
            new->n_Prev         = n;
            new->n_Next         = n->n_Next;
            new->n_Next->n_Prev = new;
            n->n_Next           = new;
            return;
        }
    }
    ADDHEAD(l,new);
}


/*****************************************************************************/


/**
|||	AUTODOC -public -class Kernel -group Lists -name UniversalInsertNode
|||	Inserts a node into a list.
|||
|||	  Synopsis
|||
|||	    void UniversalInsertNode( List *l, Node *n, bool (*f)(Node *n,Node *m) )
|||
|||	  Description
|||
|||	    Every node in a list has a priority (a value from 0 to 255 that is
|||	    stored in the n_Priority field of the node structure). When a new
|||	    node is inserted with InsertNodeFromHead() or InsertNodeFromTail(),
|||	    the position at which it is added is determined by its priority. In
|||	    contrast, the UniversalInsertNode() function allows you to arrange
|||	    nodes according to values other than priority.
|||
|||	    UniversalInsertNode() uses a comparison function provided by the
|||	    calling task to determine where to insert a new node. It compares
|||	    the node to be inserted with nodes already in the list, beginning
|||	    with the first node. If the comparison function returns TRUE, the
|||	    new node is inserted immediately before the node to which it was
|||	    compared. If the comparison function never returns TRUE, the new
|||	    node becomes the last node in the list. The comparison function,
|||	    whose arguments are pointers to two nodes, can use any data in the
|||	    nodes for the comparison.
|||
|||	  Arguments
|||
|||	    l
|||	        A pointer to the list into which to insert the node.
|||
|||	    n
|||	        A pointer to the node to insert. This same pointer is passed as
|||	        the first argument to the comparison function.
|||
|||	    f
|||	        A comparison function provided by the calling task that returns
|||	        TRUE if the node to be inserted (pointed to by the first
|||	        argument to the function) should be inserted immediately before
|||	        the node to which it is compared (pointed to by the second
|||	        argument to the function).
|||
|||	    m
|||	        A pointer to the node in the list to which to compare the node
|||	        to insert.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/list.h>, libc.a
|||
|||	  See Also
|||
|||	    AddHead(), AddTail(), InsertNodeFromHead(), InsertNodeFromTail(),
|||	    RemHead(), RemNode(), RemTail(), InsertNodeBefore(),
|||	    InsertNodeAfter()
|||
**/

void UniversalInsertNode(List *l, Node *new, bool (*f) (Node *n, Node *m))
{
Node *n;

    ScanList(l,n,Node)
    {
        if ((*f)(new, n))
        {
            new->n_Next         = n;
            new->n_Prev         = n->n_Prev;
            new->n_Prev->n_Next = new;
            n->n_Prev           = new;
            return;
        }
    }
    ADDTAIL(l, new);
}


/*****************************************************************************/


/**
|||	AUTODOC -public -class Kernel -group Lists -name FindNamedNode
|||	Finds a node by name.
|||
|||	  Synopsis
|||
|||	    Node *FindNamedNode( const List *l, const char *name )
|||
|||	  Description
|||
|||	    This function searches a list for a node with the specified name.
|||	    The search is not case-sensitive (that is, the kernel does not
|||	    distinguish uppercase and lowercase letters in node names).
|||
|||	  Arguments
|||
|||	    l
|||	        A pointer to the list to search.
|||
|||	    name
|||	        The name of the node to find.
|||
|||	  Return Value
|||
|||	    The function returns a pointer to the node structure or NULL if the named
|||	    node couldn't be found.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/list.h>, libc.a
|||
|||	  See Also
|||
|||	    FirstNode(), LastNode(), NextNode(), PrevNode()
**/

Node *FindNamedNode(const List *l, const char *s)
{
    Node *n;
    char *name;

    if (s)
	ScanList(l,n,Node)
	{
	    name = n->n_Name;
	    if (name && (strcasecmp(s, name) == 0))
		return n;
	}

    return NULL;
}


/*****************************************************************************/


/**
|||	AUTODOC -public -class Kernel -group Lists -name SetNodePri
|||	Changes the priority of a list node.
|||
|||	  Synopsis
|||
|||	    uint8 SetNodePri( Node *n, uint8 newpri )
|||
|||	  Description
|||
|||	    This function changes the priority of a node in a list. The kernel
|||	    arranges lists by priority, with higher-priority nodes coming before
|||	    lower-priority nodes. When the priority of a node changes, the
|||	    kernel automatically rearranges the list to reflect the new priority.
|||	    The node is moved immediately before the first node whose priority
|||	    is lower.
|||
|||	  Arguments
|||
|||	    n
|||	        A pointer to the node whose priority to change.
|||
|||	    newpri
|||	        The new priority for the node.
|||
|||	  Return Value
|||
|||	    The function returns the previous priority of the node.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/list.h>, libc.a
|||
|||	  Notes
|||
|||	    If you know the list that contains the node you wish to change
|||	    the priority of, it is generally much faster to do:
|||
|||	      RemNode(n);
|||	      n->n_Priority = newPri;
|||	      InsertNodeFromTail(l,n);
|||
|||	  See Also
|||
|||	    InsertNodeFromHead(), InsertNodeFromTail(), UniversalInsertNode()
|||
**/

uint8 SetNodePri(Node *n, uint8 pri)
{
List  *l;
Node  *lastnode;
uint8  oldpri = n->n_Priority;

    /* Find the list where this node is */
    lastnode = NextNode(n);
    while (lastnode->n_Next)
        lastnode = NextNode(lastnode);

    /* lastnode now points to the tail.links */
    l = (List *)((uint32)lastnode - offsetof(List,ListAnchor.tail.links));

    REMOVENODE(n);
    n->n_Priority = pri;
    InsertNodeFromTail(l, n);

    return oldpri;
}


/*****************************************************************************/


/**
|||	AUTODOC -public -class Kernel -group Lists -name IsListEmpty
|||	Checks whether a list is empty.
|||
|||	  Synopsis
|||
|||	    bool IsListEmpty( List *l )
|||
|||	  Description
|||
|||	    This macro checks whether a list is empty.
|||
|||	  Arguments
|||
|||	    l
|||	        A pointer to the list to check.
|||
|||	  Return Value
|||
|||	    The macro returns TRUE if the list is empty or FALSE if it isn't.
|||
|||	  Implementation
|||
|||	    Macro implemented in <kernel/list.h> V24.
|||
|||	  Associated Files
|||
|||	    <kernel/list.h>, libc.a
|||
|||	  See Also
|||
|||	    FirstNode(), IsNode(), IsNodeB(), LastNode(), NextNode(), PrevNode(),
|||	    ScanList()
|||
**/

/**
|||	AUTODOC -public -class Kernel -group Lists -name IsEmptyList
|||	Checks whether a list is empty.
|||
|||	  Synopsis
|||
|||	    bool IsEmptyList( List *l )
|||
|||	  Description
|||
|||	    This macro checks whether a list is empty.
|||
|||	  Arguments
|||
|||	    l
|||	        A pointer to the list to check.
|||
|||	  Return Value
|||
|||	    The macro returns TRUE if the list is empty or FALSE if it isn't.
|||
|||	  Implementation
|||
|||	    Macro implemented in <kernel/list.h> V20.
|||
|||	  Associated Files
|||
|||	    <kernel/list.h>, libc.a
|||
|||	  See Also
|||
|||	    FirstNode(), IsNode(), IsNodeB(), LastNode(), NextNode(), PrevNode(),
|||	    ScanList(), IsListEmpty()
|||
**/

/**
|||	AUTODOC -public -class Kernel -group Lists -name ScanList
|||	Walks through all the nodes in a list.
|||
|||	  Synopsis
|||
|||	    ScanList( const List *1, void *n, <node type>)
|||
|||	  Description
|||
|||	    This macro lets you easily walk through all the elements in a list
|||	    from the first to the last.
|||
|||	  Arguments
|||
|||	    l
|||	        A pointer to the list to scan.
|||
|||	    n
|||	        A variable which will be altered to hold a pointer to every
|||	        node in the list in succession.
|||
|||	    <node type>
|||	        The data type of the nodes in the list. This is used for type
|||	        casting within the macro.
|||
|||	  Implementation
|||
|||	    Macro implemented in <kernel/list.h> V22.
|||
|||	  Example
|||
|||	    {
|||	    List       *l;
|||	    DataStruct *d;
|||	    uint32      i;
|||
|||	        i = 0;
|||	        ScanList(l,d,DataStruct)
|||	        {
|||	            printf("Node %d is called %sn",i,d->d.n_Name);
|||	            i++;
|||	        }
|||	    }
|||
|||	  Associated Files
|||
|||	    <kernel/list.h>, libc.a
|||
|||	  Warning
|||
|||	    You cannot remove nodes from the list as you are scanning it with
|||	    this macro.
|||
|||	  See Also
|||
|||	    ScanListB()
|||
**/

/**
|||	AUTODOC -public -class Kernel -group Lists -name ScanListB
|||	Walks through all the nodes in a list backwards.
|||
|||	  Synopsis
|||
|||	    ScanListB( const List *1, void *n, <node type>)
|||
|||	  Description
|||
|||	    This macro lets you easily walk through all the elements in a list
|||	    from the last to the first.
|||
|||	  Arguments
|||
|||	    l
|||	        A pointer to the list to scan.
|||
|||	    n
|||	        A variable which will be altered to hold a pointer to every
|||	        node in the list in succession.
|||
|||	    <node type>
|||	        The data type of the nodes on the list. This is used for type
|||	        casting within the macro.
|||
|||	  Implementation
|||
|||	    Macro implemented in <kernel/list.h> V24.
|||
|||	  Example
|||
|||	    {
|||	    List       *l;
|||	    DataStruct *d;
|||	    uint32      i;
|||
|||	        i = 0;
|||	        ScanListB(l,d,DataStruct)
|||	        {
|||	            printf("Node %d (counting from the end) is called %sn",i,
|||	                   d->d.n_Name);
|||	            i++;
|||	        }
|||	    }
|||
|||	  Associated Files
|||
|||	    <kernel/list.h>, libc.a
|||
|||	  Warning
|||
|||	    You cannot remove nodes from the list as you are scanning it with
|||	    this macro.
|||
|||	  See Also
|||
|||	    ScanList()
|||
**/

/**
|||	AUTODOC -public -class Kernel -group Lists -name FirstNode
|||	Gets the first node in a list.
|||
|||	  Synopsis
|||
|||	    Node *FirstNode( List *l )
|||
|||	  Description
|||
|||	    This macro returns a pointer to the first node in the specified list.
|||	    If the list is empty, the macro returns a pointer to the tail
|||	    (end-of-list) anchor. To determine if the return value is an actual
|||	    node rather than the tail anchor, call the IsNode() function.
|||
|||	  Arguments
|||
|||	    l
|||	        A pointer to the list from which to get the first node.
|||
|||	  Return Value
|||
|||	    The macro returns a pointer to first node in the list or, if the
|||	    list is empty, a pointer to the tail (end-of-list) anchor.
|||
|||	  Implementation
|||
|||	    Macro implemented in <kernel/list.h> V20.
|||
|||	  Example
|||
|||	    for (n = FirstNode(list); IsNode(list, n); n = NextNode(n))
|||	    {
|||	        // n will iteratively point to every node in the list
|||	    }
|||
|||	  Associated Files
|||
|||	    <kernel/list.h>, libc.a
|||
|||	  See Also
|||
|||	    IsListEmpty(), IsNode(), IsNodeB(), LastNode(), NextNode(), PrevNode(),
|||	    ScanList()
|||
**/

/**
|||	AUTODOC -public -class Kernel -group Lists -name LastNode
|||	Gets the last node in a list.
|||
|||	  Synopsis
|||
|||	    Node *LastNode( const List *l )
|||
|||	  Description
|||
|||	    This macro returns a pointer to the last node in a list. If the list
|||	    is empty, the macro returns a pointer to the head
|||	    (beginning-of-list) anchor. To determine if the return value is an
|||	    actual node rather than the head anchor, call the IsNodeB() function.
|||
|||	  Arguments
|||
|||	    l
|||	        A pointer to the list structure to be examined.
|||
|||	  Return Value
|||
|||	    The macro returns a pointer to last node in the list or, if the list
|||	    is empty, a pointer to the head (beginning-of-list) anchor.
|||
|||	  Implementation
|||
|||	    Macro implemented in <kernel/list.h> V20.
|||
|||	  Example
|||
|||	    for (n = LastNode(list); IsNodeB(list, n); n = PrevNode(n))
|||	    {
|||	        // n will iteratively point to every node in the list
|||	    }
|||
|||	  Associated Files
|||
|||	    <kernel/list.h>, libc.a
|||
|||	  See Also
|||
|||	    FirstNode(), IsListEmpty(), IsNode(), IsNodeB(), NextNode(), PrevNode(),
|||	    ScanList()
|||
**/

/**
|||	AUTODOC -class Kernel -group Lists -name IsNode
|||	Validates a node when moving forward through a list.
|||
|||	  Synopsis
|||
|||	    bool IsNode( const List *l, const Node *n )
|||
|||	  Description
|||
|||	    This macro is used to test whether the specified node is an actual
|||	    node or is the tail (end-of-list) anchor. Use this macro when
|||	    traversing a list from head to tail. When traversing a list from
|||	    tail to head, use the IsNodeB() macro.
|||
|||	  Arguments
|||
|||	    l
|||	        A pointer to the list containing the node to check.
|||
|||	    n
|||	        A pointer to the node to check.
|||
|||	  Return Value
|||
|||	    The macro returns TRUE if the node is an actual node or FALSE if it
|||	    is the tail (end-of-list) anchor. This macro will return TRUE for
|||	    any node that is not the tail anchor, whether or not the node is
|||	    in the specified list.
|||
|||	  Implementation
|||
|||	    Macro implemented in <kernel/list.h> V20.
|||
|||	  Associated Files
|||
|||	    <kernel/list.h>, libc.a
|||
|||	  See Also
|||
|||	    FirstNode(), IsListEmpty(), IsNodeB(), LastNode(), NextNode(), PrevNode(),
|||	    ScanList()
|||
**/

/**
|||	AUTODOC -class Kernel -group Lists -name IsNodeB
|||	Validates a node when moving backward through a list.
|||
|||	  Synopsis
|||
|||	    bool IsNodeB( const List *l, const Node *n )
|||
|||	  Description
|||
|||	    This macro is used to test whether the specified node is an actual
|||	    node or is the head (beginning-of-list) anchor. Use this macro when
|||	    traversing a list from tail to head. When traversing a list from
|||	    head to tail, use the IsNode() macro.
|||
|||	  Arguments
|||
|||	    l
|||	        A pointer to the list containing the node to check.
|||
|||	    n
|||	        A pointer to the node to check.
|||
|||	  Return Value
|||
|||	    The macro returns TRUE if the node is an actual node or FALSE if it
|||	    is the head (beginning-of-list) anchor.  This macro will return TRUE
|||	    for any node that is not the head anchor, whether or not the node is
|||	    in the specified list.
|||
|||	  Implementation
|||
|||	    Macro implemented in <kernel/list.h> V20.
|||
|||	  Associated Files
|||
|||	    <kernel/list.h>, libc.a
|||
|||	  See Also
|||
|||	    FirstNode(), IsListEmpty(), IsNode(), LastNode(), NextNode(), PrevNode(),
|||	    ScanListB()
|||
**/

/**
|||	AUTODOC -public -class Kernel -group Lists -name NextNode
|||	Gets the next node in a list.
|||
|||	  Synopsis
|||
|||	    Node *NextNode( const Node *n )
|||
|||	  Description
|||
|||	    This macro gets a pointer to the next node in a list. If the
|||	    current node is the last node in the list, the result is a pointer
|||	    to the tail (end-of-list) anchor. To determine if the return value
|||	    is an actual node rather than the tail anchor, call the IsNode()
|||	    function.
|||
|||	  Arguments
|||
|||	    n
|||	        Pointer to the current node.
|||
|||	  Return Value
|||
|||	    The macro returns a pointer to the next node in the list or, if the
|||	    current node is the last node in the list, to the tail (end-of-list)
|||	    anchor.
|||
|||	  Implementation
|||
|||	    Macro implemented in <kernel/list.h> V20.
|||
|||	  Associated Files
|||
|||	    <kernel/list.h>, libc.a
|||
|||	  Caveats
|||
|||	    Assumes that n is a node in a list. If not, watch out.
|||
|||	  See Also
|||
|||	    FirstNode(), IsListEmpty(), IsNode(), IsNodeB(), LastNode(), PrevNode(),
|||	    ScanList()
|||
**/

/**
|||	AUTODOC -public -class Kernel -group Lists -name PrevNode
|||	Gets the previous node in a list.
|||
|||	  Synopsis
|||
|||	    Node *PrevNode( const Node *node )
|||
|||	  Description
|||
|||	    This macro returns a pointer to the previous node in a list. If the
|||	    current node is the first node in the list, the result is a pointer
|||	    to the head (beginning-of-list) anchor. To determine whether the
|||	    return value is an actual node rather than the head anchor, use the
|||	    IsNodeB() function.
|||
|||	  Arguments
|||
|||	    node
|||	        A pointer to the current node.
|||
|||	  Return Value
|||
|||	    The macro returns a pointer to the previous node in the list or, if
|||	    the current node is the first node in the list, to the head
|||	    (beginning-of-list) anchor.
|||
|||	  Implementation
|||
|||	    Macro implemented in <kernel/list.h> V20.
|||
|||	  Associated Files
|||
|||	    <kernel/list.h>, libc.a
|||
|||	  See Also
|||
|||	    FirstNode(), IsListEmpty(), IsNode(), IsNodeB(), LastNode(), ScanList()
|||
**/
