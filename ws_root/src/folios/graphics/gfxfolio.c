/*  :ts=8 bk=0
 *
 * gfxfolio.c:	M2 Graphics Folio.
 *
 * @(#) gfxfolio.c   96/09/17  1.31
 *
 * Leo L. Schwab		(Display Stuff)			9504.11
 * David Somayajulu		(Triangle Engine Stuff)		9505.16
 ***************************************************************************
 * Copyright 1995 The 3DO Company.  All Rights Reserved.
 * Confidential and proprietary.  Do not disclose or distribute.
 ***************************************************************************
 */
#include <kernel/types.h>
#include <kernel/folio.h>
#include <kernel/task.h>
#include <kernel/mem.h>
#include <kernel/kernelnodes.h>
#include <kernel/item.h>
#include <kernel/operror.h>
#include <kernel/super.h>
#include <kernel/debug.h>
#include <kernel/kernel.h>

#include <graphics/gfx_pvt.h>
#include <graphics/projector.h>
#include <graphics/bitmap.h>
#include <graphics/view.h>

#include <loader/loader3do.h>

#include <stdio.h>
#include <string.h>

#include "protos.h"


/***************************************************************************
 * Local prototypes.
 */
static int32	InitFolio (struct GraphicsFolioBase *gb);


/***************************************************************************
 * Some globals.
 */
#ifdef DYNAMIC_FOLIO_BASE
GraphicsFolioBase	*GBase;
#else
GraphicsFolioBase	GB;
#endif


/***************************************************************************
 * Folio function tables.
 */
static void *(*swifuncs[])() = {
	(void *(*)())SuperModifyGraphicsItem,	/* 0 */
	(void *(*)())SuperAddViewToViewList,	/* 1 */
	(void *(*)())SuperRemoveView,		/* 2 */
	(void *(*)())SuperOrderViews,		/* 3 */
	(void *(*)())SuperLockDisplay,		/* 4 */
	(void *(*)())SuperUnlockDisplay,	/* 5 */
	(void *(*)())SuperActivateProjector,	/* 6 */
	(void *(*)())SuperDeactivateProjector,	/* 7 */
	(void *(*)())SuperSetDefaultProjector,	/* 8 */
};
#define	N_SWIFUNCS	(sizeof (swifuncs) / sizeof (void *(*)()))


/***************************************************************************
 * Info on nodes maintained by this folio
 */
static NodeData nodedatabase[] =
{
	{ 0,			 	0 },
	{ sizeof (Bitmap),
			NODE_ITEMVALID | NODE_NAMEVALID | NODE_SIZELOCKED },
	{ sizeof (View),
			NODE_ITEMVALID | NODE_NAMEVALID | NODE_SIZELOCKED },
	{ sizeof (ViewList),
			NODE_ITEMVALID | NODE_NAMEVALID | NODE_SIZELOCKED },
	{ sizeof (Projector),
			NODE_ITEMVALID | NODE_NAMEVALID | NODE_OPENVALID |
			 NODE_SIZELOCKED },
};
#define N_NODES		(sizeof (nodedatabase) / sizeof (NodeData))


/***************************************************************************
 * Tags to create the folio.
 */
TagArg	foliotags[] = {
#ifndef DYNAMIC_FOLIO_BASE
	{ CREATEFOLIO_TAG_BASE,		(void *) &GB			},
#endif
	{ CREATEFOLIO_TAG_DATASIZE,	(void *) sizeof (GraphicsFolioBase)	},

	{ CREATEFOLIO_TAG_NSWIS,	(void *) N_SWIFUNCS		},
	{ CREATEFOLIO_TAG_SWIS,		(void *) swifuncs		},

	/* name of graphics folio */
	{ TAG_ITEM_NAME,		(void *) GRAPHICSFOLIONAME		},
	/* initialization code */
	{ CREATEFOLIO_TAG_INIT,		(void *) ((long) InitFolio)	},
	/* our item number */
	{ CREATEFOLIO_TAG_ITEM,		(void *) NST_GRAPHICS		},
	/* for lack of a better value */
	{ TAG_ITEM_PRI,			(void *) 0			},
	/* Graphics node database */
	{ CREATEFOLIO_TAG_NODEDATABASE,	(void *) nodedatabase		},
	/* number of nodes */
	{ CREATEFOLIO_TAG_MAXNODETYPE,	(void *) N_NODES		},
	/* end of tag list */
	{ 0,				(void *) 0			},
};


/***************************************************************************
 * Code.
 */


/* The job of this function is to run as a thread when the folio is first
 * created and to load in the projectors. When they're loaded, this thread
 * dies, never to come back again.
 */
static Err LoadProjectors(void)
{
	Projector	*p;
	DeviceStack	*node;
	Err		err;
	List		*list;

	/*
	 * Locate and load all video drivers.
	 */
	if ((err = CreateDeviceStackListVA
		    (&list, "cmds", DDF_EQ, DDF_INT, 1,
		     GFXCMD_PROJECTORMODULE,
		     NULL)) < 0)
		return (err);

	if (!IsEmptyList (list)) {
		for (node = (DeviceStack *) FIRSTNODE (list);
		     NEXTNODE (node);
		     node = (DeviceStack *) NEXTNODE (node))
		{
			if ((err = OpenDeviceStack (node)) < 0)
				break;
		}
	} else
		err = GFX_ERR_INTERNAL;

	DeleteDeviceStackList (list);
	if (err < 0) {
		PrintfSysErr (err);
		return (err);
	}

	/*
	 * Find the default Projector and turn it on.
	 * ### This is a sub-optimal hack.  Right now we just grab the
	 * ### first Projector on the list and turn it on.  Ideally there
	 * ### should be a user-preferences thing to determine who gets to
	 * ### be default Projector.
	 */
	p = (Projector *) FIRSTNODE (&GBASE(gb_ProjectorList));
	if (!NEXTNODE (p))
		return (GFX_ERR_INTERNAL);

	if ((err = OpenItemAsTask
		    (p->p.n_Item, NULL, KB_FIELD(kb_OperatorTask))) < 0)
		return (err);

	if (!(p->p_Flags & PROJF_ACTIVE)  &&  p->p_Lamp)
		if ((err = CallBackSuper
			    (p->p_Lamp, (uint32) p, TRUE, 0)) < 0)
			return (err);

	GBASE(gb_DefaultProjector) = p;

	return 0;
}

/* The job of this function is to run as a thread when the folio is first
 * created and to load in the projectors. When they're loaded, this thread
 * dies, never to come back again.
 */
static Err ProjectorLoader(Item sigTask, int32 sig)
{
        /* grab this to block out mainline code */
	LockSemaphore (GBASE(gb_ProjectorListSema4)->s.n_Item, SEM_WAIT);

        /* tell our creator that the semaphore is locked, and it can proceed */
	SendSignal (sigTask, sig);

	GBASE(gb_ProjectorLoaderResult) = LoadProjectors();

	TransferItems(KernelBase->kb_OperatorTask);

	UnlockSemaphore (GBASE(gb_ProjectorListSema4)->s.n_Item);

	return 0;
}

/* This makes sure the projector loader has completed execution successfully */
Err WaitForProjectorLoader(void)
{
    LockSemaphore (GBASE(gb_ProjectorListSema4)->s.n_Item, SEM_WAIT);
    UnlockSemaphore (GBASE(gb_ProjectorListSema4)->s.n_Item);
    return GBASE(gb_ProjectorLoaderResult);
}

static Err
__DeleteModule (void)
{
	return (NOSUPPORT);
}

static int32
__CreateModule (void)
{
Item  result;
int32 sig;

    result = CreateItem (MKNODEID(KERNELNODE, FOLIONODE), foliotags);
    if (result >= 0)
    {
        /* now get the projectors loaded */
        sig = AllocSignal(0);
        if (sig > 0)
        {
            result = CreateThreadVA((void *)ProjectorLoader, "Projector Loader", 0, 4096,
                                    CREATETASK_TAG_PRIVILEGED,   TRUE,
                                    CREATETASK_TAG_SINGLE_STACK, TRUE,
                                    CREATETASK_TAG_ARGC,         CURRENTTASKITEM,
                                    CREATETASK_TAG_ARGP,         sig,
                                    TAG_END);
            if (result >= 0)
            {
                WaitSignal(sig);
                FreeSignal(sig);
            }
        }
    }

    return result;
}


int main(int32 op, Item it)
{
	TOUCH(it);

	switch (op)
	{
		case DEMANDLOAD_MAIN_CREATE:	return __CreateModule();
		case DEMANDLOAD_MAIN_DELETE:	return __DeleteModule();
		default:			return 0;
	}
}

/***************************************************************************
 * Folio Item management routines.
 */
static Item
dfCreateItem (
void	*node,
uint8	nodeType,
void	*args
)
{
	Err err;

	err = WaitForProjectorLoader();
	if (err < 0)
	    return err;

	switch (nodeType) {
	case GFX_BITMAP_NODE:
		return (createbitmap ((Bitmap *) node, args));
	case GFX_VIEW_NODE:
		return (createview ((View *) node, args));
	case GFX_VIEWLIST_NODE:
		return (createviewlist ((ViewList *) node, args));
	case GFX_PROJECTOR_NODE:
		return (createprojector ((Projector *) node, args));
	default:
		return (GFX_ERR_BADSUBTYPE);
	}
}


static Err
dfDeleteItem (
Item		it,
struct Task	*task
)
{
	Node *node;

	TOUCH(task);

	node = (Node *) LookupItem (it);

	switch (node->n_Type) {
	case GFX_BITMAP_NODE:
		return (deletebitmap ((Bitmap *) node));
	case GFX_VIEW_NODE:
		return (deleteview ((View *) node));
	case GFX_VIEWLIST_NODE:
		return (deleteviewlist ((ViewList *) node));
	case GFX_PROJECTOR_NODE:
		return (deleteprojector ((Projector *) node));
	default:
		return (GFX_ERR_BADITEM);
	}
}


static Item
dfFindItem (
int32		ntype,
struct TagArg	*args
)
{
	switch (ntype) {
	case GFX_BITMAP_NODE:
		return (findbitmap (args));
	case GFX_VIEW_NODE:
		return (findview (args));
	case GFX_VIEWLIST_NODE:
		return (findviewlist (args));
	case GFX_PROJECTOR_NODE:
		return (findprojector (args));
	default:
		return (GFX_ERR_BADITEM);
	}
}


static Item
dfOpenItem (
struct Node	*node,
void		*args,
struct Task	*t
)
{
	TOUCH (t);
	switch (node->n_Type) {
	case GFX_BITMAP_NODE:
		return (openbitmap ((Bitmap *) node, args));
	case GFX_VIEW_NODE:
		return (openview ((View *) node, args));
	case GFX_VIEWLIST_NODE:
		return (openviewlist ((ViewList *) node, args));
	case GFX_PROJECTOR_NODE:
		return (openprojector ((Projector *) node, args));
	default:
		return (GFX_ERR_BADITEM);
	}
}


static Err
dfCloseItem (
Item		it,
struct Task	*task
)
{
	Node *node;

	node = (Node *) LookupItem (it);

	switch (node->n_Type) {
	case GFX_BITMAP_NODE:
		return (closebitmap ((Bitmap *) node, task));
	case GFX_VIEW_NODE:
		return (closeview ((View *) node, task));
	case GFX_VIEWLIST_NODE:
		return (closeviewlist ((ViewList *) node, task));
	case GFX_PROJECTOR_NODE:
		return (closeprojector ((Projector *) node, task));
	default:
		return (GFX_ERR_BADITEM);
	}
}


static Err
dfSetOwnerItem (
struct ItemNode	*n,
Item		newOwner,
struct Task	*task
)
{
	/*
	 * I'm getting to it...
	 */
	switch (n->n_Type) {
	case GFX_PROJECTOR_NODE:
		return (setownerprojector ((Projector *) n, newOwner, task));
	default:
		return (GFX_ERR_NOTSUPPORTED);
	}
}


/**
|||	AUTODOC -public -class graphicsfolio -name ModifyGraphicsItem
|||	Modify the characteristics of a Graphics Folio Item.
|||
|||	  Synopsis
|||
|||	    Item ModifyGraphicsItem (Item graphicsItem, TagArg *args)
|||
|||	    Item ModifyGraphicsItemVA (Item graphicsItem, uint32 tag, ...)
|||
|||	  Description
|||
|||	    This function permits the client to modify the characteristics
|||	    of an existing Item.  Like CreateItem(), Tag arguments are used
|||	    to specify characteristics.  However, instead of creating a new
|||	    Item, an existing Item is modified according to the Tag
|||	    arguments.
|||
|||	  Arguments
|||
|||	    graphicsItem
|||	        Item to be modified.
|||
|||	    args
|||	        Pointer to array of Tag args indicating the characteristics
|||	        to be modified.
|||
|||	  Return Value
|||
|||	    If successful, the specified modifications are applied to the
|||	    Item, and the Item number is returned.
|||
|||	    If there is an error, the Item is left unchanged, and a
|||	    (negative) error code is returned.
|||
|||	  Caveats
|||
|||	    There are certain rare conditions where an error will occur, but
|||	    the Item will have been modified.  This occurs when the
|||	    requested modification fails, but the original condition can't
|||	    be restored.  These cases are called out in the autodocs for the
|||	    Items in question.
|||
|||	  Associated Files
|||
|||	    <graphics/graphics.h>, <graphics/bitmap.h>,
|||	    <graphics/view.h>
|||
|||	  See Also
|||
|||	    CreateItem(), Bitmap(@), View(@), ViewList(@)
|||
**/
Item
SuperModifyGraphicsItem (
Item		it,
struct TagArg	*ta
)
{
	ItemNode	*n;

	/* ### ownership checks  */
	if (!(n = LookupItem (it)))
		return (GFX_ERR_BADITEM);

	if (n->n_SubsysType != NST_GRAPHICS)
		return (GFX_ERR_BADITEM);

	switch (n->n_Type) {
	case GFX_BITMAP_NODE:
		return (modifybitmap ((Bitmap *) n, ta));
	case GFX_VIEW_NODE:
		return (modifyview ((View *) n, ta));
	case GFX_VIEWLIST_NODE:
		return (modifyviewlist ((ViewList *) n, ta));
	default:
		return (GFX_ERR_BADITEM);
	}
}


/***************************************************************************
 * Folio startup.
 * There is no teardown facility, since the VDL hardware must continue to be
 * managed by someone...
 */
static int32
InitFolio (
struct GraphicsFolioBase *gb
)
{
	ItemRoutines	*ir;
	Err		err;


	DDEBUG(("INITFOLIO: entering\n"));

#ifdef DYNAMIC_FOLIO_BASE
	GBase = gb;
#endif

	/*
	 * Set pointers to required Folio functions.
	 */
	ir		= gb->gb.f_ItemRoutines;
	ir->ir_Delete	= dfDeleteItem;
	ir->ir_Create	= dfCreateItem;
	ir->ir_Find	= dfFindItem;
	ir->ir_Open	= dfOpenItem;
	ir->ir_Close	= dfCloseItem;
	ir->ir_SetOwner	= dfSetOwnerItem;

	if ((err = initgbase ()))
		return (err);

	DDEBUG(("INITFOLIO: exiting succesfully.\n"));

	return (0);
}
