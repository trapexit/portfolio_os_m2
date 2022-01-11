#ifndef __KERNEL_FOLIO_H
#define __KERNEL_FOLIO_H


/******************************************************************************
**
**  @(#) folio.h 96/06/11 1.30
**
**  Kernel folio management definitions
**
******************************************************************************/


#ifdef EXTERNAL_RELEASE
#error "This file may not be used in externally released source code or link lib"
#endif

#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_NODES_H
#include <kernel/nodes.h>
#endif

#ifndef __KERNEL_ITEM_H
#include <kernel/item.h>
#endif

#ifndef __KERNEL_TASK_H
#include <kernel/task.h>
#endif


/*****************************************************************************/


typedef int32 (*FolioFunc)();

/* This is the structure of the Node Creation DataBase that each folio must
 * have to use the CreateItem() interface.
 *
 * The upper 4 bits of the flags field are put in the n_Flags field of the
 * created Node. The other four bits are used to tell the allocator other
 * things for initialization.
 */

typedef struct NodeData
{
        uint16 size;
        uint8 flags;
        uint8 pad;
} NodeData;

typedef struct ItemRoutines
{
	/* Folio helper routines for its Items */
	Item  (*ir_Find)(int32 ntype, TagArg *tp);
	Item  (*ir_Create)(void *n, uint8 ntype, void *args);
	int32 (*ir_Delete)(Item it, struct Task *t);
	Item  (*ir_Open)(Node *n, void *args, struct Task *t);
	int32 (*ir_Close)(Item it,struct Task *t);
	int32 (*ir_SetPriority)(ItemNode *n, uint8 pri, struct Task *t);
	Err   (*ir_SetOwner)(ItemNode *n, Item newOwner, struct Task *t);
	Item  (*ir_Load)(int32 ntype, TagArg *tp);
} ItemRoutines;

typedef struct Folio
{
	ItemNode         fn;
        bool             f_DontFree;
        uint8            f_TaskDataIndex;
        uint8            f_MaxSwiFunctions;
        uint8            f_MaxNodeType;
        NodeData        *f_NodeDB;
        ItemRoutines    *f_ItemRoutines;
	FolioFunc       *f_SwiFunctions;
        int32          (*f_DeleteFolio)(struct Folio *f);
        int32          (*f_FolioCreateTask)(struct Task *t,TagArg *tagpt);
        void           (*f_FolioDeleteTask)(struct Task *t);
	int32	         f_Reserved;
} Folio, *FolioP;


enum folio_tags
{
	CREATEFOLIO_TAG_DATASIZE = TAG_ITEM_LAST+1,
	CREATEFOLIO_TAG_INIT,
	CREATEFOLIO_TAG_NODEDATABASE,
	CREATEFOLIO_TAG_MAXNODETYPE,
	CREATEFOLIO_TAG_ITEM,
	CREATEFOLIO_TAG_DELETEF,
	CREATEFOLIO_TAG_NSWIS,		/* number of swis */
	CREATEFOLIO_TAG_SWIS,		/* ptr to SWI table */
	CREATEFOLIO_TAG_TASKDATA,	/* per task data struct */
	CREATEFOLIO_TAG_BASE		/* ptr to Folio base structure to use */
};


/*****************************************************************************/


#endif	/* __KERNEL_FOLIO_H */
