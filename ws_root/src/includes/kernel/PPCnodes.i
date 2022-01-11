#ifndef __KERNEL_PPCNODES_I
#define __KERNEL_PPCNODES_I


/******************************************************************************
**
**  @(#) PPCnodes.i 96/04/24 1.10
**
******************************************************************************/


    .struct	MinNode
n_Next		.long	1		// struct MinNode *n_Next
n_Prev		.long	1		// struct MinNode *n_Prev
    .ends

    .struct	NamelessNode
n_Next		.long	1
n_Prev		.long	1
n_SubsysType	.byte	1
n_Type		.byte	1
n_Priority	.byte	1
n_Flags		.byte	1
n_Size		.long	1
    .ends

    .struct	Node
n_Next		.long	1
n_Prev		.long	1
n_SubsysType	.byte	1
n_Type		.byte	1
n_Priority	.byte	1
n_Flags		.byte	1
n_Size		.long	1
n_Name		.long	1
    .ends

    .struct	ItemNode
n_Next		.long	1
n_Prev		.long	1
n_SubsysType	.byte	1
n_Type		.byte	1
n_Priority	.byte	1
n_Flags		.byte	1
n_Size		.long	1
n_Name		.long	1
n_Version	.byte	1
n_Revision	.byte	1
n_Reserved0	.byte	1
n_ItemFlags	.byte	1
n_Item		.long	1
n_Owner		.long	1
n_Reserved1	.long	1
    .ends

    .struct	OpeningItemNode
n_Next		.long	1
n_Prev		.long	1
n_SubsysType	.byte	1
n_Type		.byte	1
n_Priority	.byte	1
n_Flags		.byte	1
n_Size		.long	1
n_Name		.long	1
n_Version	.byte	1
n_Revision	.byte	1
n_Reserved0	.byte	1
n_ItemFlags	.byte	1
n_Item		.long	1
n_Owner		.long	1
n_Reserved1	.long	1
n_OpenCount	.long	1
    .ends

// n_Flags
#define NODE_NAMEVALID	0x80
#define NODE_SIZELOCKED	0x40
#define NODE_ITEMVALID	0x20
#define NODE_OPENVALID	0x10

// n_ItemFlags
#define ITEMNODE_NOTREADY	0x80


#endif /* __KERNEL_PPCNODES_I */
