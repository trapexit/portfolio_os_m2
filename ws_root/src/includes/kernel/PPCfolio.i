#ifndef __KERNEL_PPCFOLIO_I
#define __KERNEL_PPCFOLIO_I


/******************************************************************************
**
**  @(#) PPCfolio.i 96/06/11 1.18
**
******************************************************************************/


#ifndef __KERNEL_PPCNODES_I
#include <kernel/PPCnodes.i>
#endif

    .struct	NodeData
size		.byte	2	// uint16 size;
flags		.byte	1	// uint8 flags;
pad		.byte	1	// uint8 pad;
    .ends

//  The upper 4 bits of the flags field are put in the n_Flags
//  field of the created Node. The other four bits are used to
//  tell the allocator other things for initialization

    .struct	ItemRoutines
// Folio helper routines for its Items
ir_Find		.long	1	// Item  (*ir_Find)(int32 ntype, TagArg *tp);
ir_Create	.long	1	// Item  (*ir_Create)(void *n, uint8 ntype, void *args);
ir_Delete	.long	1	// int32 (*ir_Delete)(Item it, struct Task *t);
ir_Open		.long	1	// Item  (*ir_Open)(Node *n, void *args);
ir_Close	.long	1	// int32 (*ir_Close)(Item it,struct Task *t);
ir_SetPriority	.long	1	// int32 (*ir_SetPriority)(ItemNode *n, uint8 pri, struct Task *t);
ir_SetOwner	.long	1	// Err   (*ir_SetOwner)(ItemNode *n, Item newOwner, struct Task *t);
ir_Load		.long	1
    .ends

    .struct	Folio
fn			.byte	ItemNode	// ItemNode fn;
f_DontFree		.byte	1
f_TaskDataIndex		.byte	1		// uint8  f_TaskDataIndex;
f_MaxSwiFunctions	.byte	1		// uint8 f_MaxSwiFunctions;
f_MaxNodeType		.byte	1		// uint8 f_MaxNodeType;
f_NodeDB		.long	1		// NodeData *f_NodeDB;
f_ItemRoutines		.long	1		// ItemRoutines *f_ItemRoutines;
f_SwiFunctions		.long	1		// FolioFunc *f_UserFunctions;
f_DeleteFolio		.long	1		// int32  (*f_DeleteFolio)(struct Folio *f);
f_FolioCreateTask	.long	1		// int32  (*f_FolioCreateTask)(struct Task *t,TagArg *tagpt);
f_FolioDeleteTask	.long	1		// void (*f_FolioDeleteTask)(struct Task *t);
f_Reserved		.long	1		// int32	reserved2[2];
    .ends

#define CREATEFOLIO_TAG_DATASIZE	TAG_ITEM_LAST+1
#define CREATEFOLIO_TAG_INIT		TAG_ITEM_LAST+2
#define CREATEFOLIO_TAG_NODEDATABASE	TAG_ITEM_LAST+3
#define CREATEFOLIO_TAG_MAXNODETYPE	TAG_ITEM_LAST+4
#define CREATEFOLIO_TAG_ITEM		TAG_ITEM_LAST+5
#define CREATEFOLIO_TAG_DELETEF		TAG_ITEM_LAST+6
#define CREATEFOLIO_TAG_NSWIS		TAG_ITEM_LAST+7
#define CREATEFOLIO_TAG_SWIS		TAG_ITEM_LAST+8
#define CREATEFOLIO_TAG_TASKDATA	TAG_ITEM_LAST+9


#endif /* __KERNEL_PPCFOLIO_I */
