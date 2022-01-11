#ifndef __KERNEL_NODES_H
#define __KERNEL_NODES_H


/******************************************************************************
**
**  @(#) nodes.h 96/02/28 1.25
**
**  Kernel node definitions
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif


/*****************************************************************************/


/* defines which system component is responsible for a given node */
typedef enum NodeSubsysTypes
{
    NST_UNKNOWN,        /* unassigned            */
    NST_KERNEL,         /* kernel                */
    NST_GRAPHICS,       /* graphics folio        */
    NST_FILESYS,        /* file folio            */
    NST_AUDIO,          /* audio folio           */
    NST_INTL,           /* international folio   */
    NST_FONT,           /* font folio            */
    NST_ICON,           /* icon folio            */
    NST_BEEP,           /* beep folio            */
    NST_BATT            /* battery-backed folio  */
} NodeSubsysTypes;

#ifndef EXTERNAL_RELEASE
#define	NST_SYSITEMMASK	0xff
#define	NodeToSubsys(a)	((uint32)(((ItemNode *)(a))->n_Item & ITEM_INDX_MASK))
#define	NODETOSUBSYS(a)	((uint32)(((ItemNode *)(a))->n_Item & ITEM_INDX_MASK))
#endif /* EXTERNAL_RELEASE */

/* combine a subsystype and a type to form a node ID */
#define MkNodeID(subsys,type) (int32)( ((subsys)<<8) | (type))
#define MKNODEID(subsys,type) (int32)( ((subsys)<<8) | (type))

/* extract the subsys type from a node ID */
#define SubsysPart(nodeid)    ((uint32)(((nodeid)>>8) & 0xff))
#define SUBSYSPART(nodeid)    ((uint32)(((nodeid)>>8) & 0xff))

/* extract the node type from a node ID */
#define TypePart(nodeid)      ((uint8)((nodeid) & 0xff))
#define TYPEPART(nodeid)      ((uint8)((nodeid) & 0xff))


/*****************************************************************************/


/* Minimal node structure used for linking only */
typedef struct MinNode
{
    struct MinNode *n_Next;         /* pointer to next in list     */
    struct MinNode *n_Prev;         /* pointer to previous in list */
} MinNode;

/* Node structure used when the name is not needed */
typedef struct NamelessNode
{
    struct NamelessNode *n_Next;    /* pointer to next in list             */
    struct NamelessNode *n_Prev;    /* pointer to previous in list         */
    uint8     n_SubsysType;         /* what component manages this node    */
    uint8     n_Type;               /* what type of node for the component */
    uint8     n_Priority;           /* queueing priority                   */
    uint8     n_Flags;              /* misc flags, see below               */
    int32     n_Size;               /* total size of node including hdr    */
} NamelessNode, *NamelessNodeP;

/* Standard node structure */
typedef struct Node
{
    struct Node *n_Next;     /* pointer to next in list             */
    struct Node *n_Prev;     /* pointer to previous in list         */
    uint8     n_SubsysType;  /* what component manages this node    */
    uint8     n_Type;        /* what type of node for the component */
    uint8     n_Priority;    /* queueing priority                   */
    uint8     n_Flags;       /* misc flags, see below               */
    int32     n_Size;        /* total size of node including hdr    */
    char     *n_Name;        /* name of node, or NULL               */
} Node, *NodeP;

/* Extended node to represent items */
typedef struct ItemNode
{
    struct ItemNode *n_Next;    /* pointer to next in list              */
    struct ItemNode *n_Prev;    /* pointer to previous in list          */
    uint8     n_SubsysType;     /* what component manages this node     */
    uint8     n_Type;           /* what type of node for the component  */
    uint8     n_Priority;       /* queueing priority                    */
    uint8     n_Flags;          /* misc flags, see below                */
    int32     n_Size;           /* total size of node including hdr     */
    char     *n_Name;           /* name of item, or NULL                */
    uint8     n_Version;        /* version of of this Item              */
    uint8     n_Revision;       /* revision of this Item                */
    uint8     n_Reserved0;      /* reserved for future use              */
    uint8     n_ItemFlags;      /* additional system item flags         */
    Item      n_Item;           /* Item number representing this struct */
    Item      n_Owner;          /* creator, present owner, disposer     */
    void     *n_Reserved1;      /* reserved for future use              */
} ItemNode, *ItemNodeP;

/* Extended node to represent items that can be opened */
typedef struct OpeningItemNode
{
    struct OpeningItemNode *n_Next;   /* pointer to next in list              */
    struct OpeningItemNode *n_Prev;   /* pointer to previous in list          */
    uint8     n_SubsysType;           /* what component manages this node     */
    uint8     n_Type;                 /* what type of node for the component  */
    uint8     n_Priority;             /* queueing priority                    */
    uint8     n_Flags;                /* misc flags, see below                */
    int32     n_Size;                 /* total size of node including hdr     */
    char     *n_Name;                 /* name of item, or NULL                */
    uint8     n_Version;              /* version of of this Item              */
    uint8     n_Revision;             /* revision of this Item                */
    uint8     n_Reserved0;            /* for future use                       */
    uint8     n_ItemFlags;            /* additional system item flags         */
    Item      n_Item;                 /* Item number representing this struct */
    Item      n_Owner;                /* creator, present owner, disposer     */
    void     *n_Reserved1;            /* for future use                       */
    uint32    n_OpenCount;            /* number of times this item was opened */
} OpeningItemNode, *OpeningItemNodeP;

/* Generic bits for n_Flags. The meaning of bits 0-3 vary based on the
 * type and subsystype of the node.
 */
#define NODE_NAMEVALID	     0x80  /* This node's n_Name field is valid  */
#define NODE_SIZELOCKED	     0x40  /* The size of this node is fixed     */
#define NODE_ITEMVALID	     0x20  /* This is an ItemNode                */
#define NODE_OPENVALID	     0x10  /* This is an OpeningItemNode         */

/* bits for n_ItemFlags. */
#define ITEMNODE_NOTREADY    0x80  /* item is not yet ready for use      */
#define ITEMNODE_PRIVILEGED  0x40  /* privileged item                    */
#define ITEMNODE_UNIQUE_NAME 0x20  /* item has a unique name in its type */
#define ITEMNODE_DELETED     0x10  /* item in the process of going away  */


/*****************************************************************************/


#endif /* __KERNEL_NODES_H */
