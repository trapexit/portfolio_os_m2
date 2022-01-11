// @(#) nodes.h 95/06/26 1.3

//modes.h - from the kernel
#ifndef __KERNEL_NODES_H
#define __KERNEL_NODES_H
#ifndef __OperaTypes__	//in here too!
typedef uint32 Item;
/* Standard Node structure */
typedef struct Node
{
	struct Node *n_Next;	/* pointer to next node in list */
	struct Node *n_Prev;	/* pointer to previous node in list */
	uint8 n_SubsysType;	/* what folio manages this node */
	uint8 n_Type;		/* what type of node for the folio */
	uint8 n_Priority;	/* queueing priority */
	uint8 n_Flags;		/* misc flags, see below */
	int32  n_Size;		/* total size of node including hdr */
	/* Optional part starts here */
	char *n_Name;		/* ptr to null terminated string or NULL */
} Node, *NodeP;

typedef struct ItemNode
{
	struct ItemNode *n_Next; /* pointer to next itemnode in list */
	struct ItemNode *n_Prev; /* pointer to previous itemnode in list */
	uint8 n_SubsysType;	/* what folio manages this node */
	uint8 n_Type;		/* what type of node for the folio */
	uint8 n_Priority;	/* queueing priority */
	uint8 n_Flags;		/* misc flags, see below */
	int32 n_Size;		/* total size of node including hdr */
	char *n_Name;		/* ptr to null terminated string or NULL */
	uint8 n_Version;	/* version of of this itemnode */
	uint8 n_Revision;	/* revision of this itemnode */
	uint8 n_FolioFlags;	/* flags for this item's folio */
	uint8 n_ItemFlags;	/* additional system item flags */
	Item  n_Item;		/* ItemNumber for this data structure */
	Item  n_Owner;		/* creator, present owner, disposer */
	void *n_ReservedP;	/* Reserved pointer */ 
} ItemNode, *ItemNodeP;
#endif /* __OperaTypes__ */
#endif /* __KERNEL_NODES_H */
