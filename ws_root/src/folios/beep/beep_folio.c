/*
 * @(#) beep_folio.c 96/06/19 1.19
 */
/****************************************************************
**
** Beep Folio
**
** By:  Phil Burk
**
** Copyright (c) 1992, 3DO Company.
** This program is proprietary and confidential.
**
****************************************************************/

#include <beep/beep.h>
#include <dspptouch/dspp_touch.h>   /* OwnDSPP() */
#include <kernel/types.h>
#include <kernel/tags.h>            /* tag iteration */
#include <kernel/kernel.h>
#include <loader/loader3do.h>       /* FindCurrentModule() */

#include "beep_internal.h"

/* Macros for debugging. */
#define DBUG(x)     /* PRT(x) */

/* Prototypes */
static int32 InitBeepFolio (BeepFolio *bb);
static int32 TermBeepFolio (BeepFolio *bb);
static Item  internalCreateBeepItem (void *n, uint8 ntype, void *args);
static int32 internalDeleteBeepItem (Item it, Task *t);
static Item  internalFindBeepItem (int32 ntype, TagArg *tp);
static Item  internalOpenBeepItem (Node *n, void *args, Task *t);
static int32 internalCloseBeepItem (Item it, Task *t);
static Err   internalSetItemOwner(ItemNode *n, Item NewOwner, struct Task *t);
static int32 internalSetItemPriority(ItemNode *n, uint8 pri, struct Task *t);


/***************************************************************\
* Static Data & necessary structures                            *
\***************************************************************/

static Item dsppLock;   /* Exclusive lock on DSPP. */
Item       gBeepFolioItem;
BeepFolio  gBeepBase;   /* Static Folio base. */

/*
** SWI - Software Interrupt Functions.
*/
void *(*BeepSWIFuncs[])() =
{
  (void *(*)()) swiConfigureBeepChannel,
  (void *(*)()) swiSetBeepParameter,
  (void *(*)()) swiSetBeepVoiceParameter,
  (void *(*)()) swiSetBeepChannelData,
  (void *(*)()) swiHackBeepChannelDataNext,
  (void *(*)()) swiStartBeepChannel,
  (void *(*)()) swiStopBeepChannel,
  (void *(*)()) swiGetBeepTime
};
#define NUM_BEEPSWIFUNCS (sizeof(BeepSWIFuncs)/sizeof(void *))

/* Item ID matches the order of the entries in this database. */
struct NodeData BeepNodeData[] = {
  { 0, 0 },
  { sizeof(BeepMachine), NODE_NAMEVALID|NODE_ITEMVALID|NODE_SIZELOCKED },
};
#define BEEPNODECOUNT (sizeof(BeepNodeData)/sizeof(NodeData))

/* Tags used when creating the Folio */
static TagArg gBeepFolioTags[] = {
  { TAG_ITEM_NAME,                  (void *) BEEPFOLIONAME },          /* name of beep folio */
  { CREATEFOLIO_TAG_DATASIZE,       (void *) sizeof (BeepFolio) },     /* size of beep folio */
  { CREATEFOLIO_TAG_BASE,           (TagData) &gBeepBase },            /* use statically allocated gBeepBase */
  { CREATEFOLIO_TAG_NSWIS,          (void *) NUM_BEEPSWIFUNCS },       /* number of SWI functions */
  { CREATEFOLIO_TAG_SWIS,           (void *) BeepSWIFuncs },           /* list of swi functions */
  { CREATEFOLIO_TAG_INIT,           (void *) ((int32)InitBeepFolio) }, /* initialization code */
  { CREATEFOLIO_TAG_DELETEF,        (void *) ((int32)TermBeepFolio) }, /* deletion code */
  { CREATEFOLIO_TAG_ITEM,           (void *) BEEPNODE },
  { CREATEFOLIO_TAG_NODEDATABASE,   (void *) BeepNodeData },           /* Beep node database */
  { CREATEFOLIO_TAG_MAXNODETYPE,    (void *) BEEPNODECOUNT },          /* number of nodes */
  { TAG_END,                        (void *) 0 },                       /* end of tag list */
};


/***************************************************************\
* Code                                                          *
\***************************************************************/

/* -------------------- Open/Close docs */

 /**
 |||	AUTODOC -class Beep -name OpenBeepFolio
 |||	Opens the beep folio.
 |||
 |||	  Synopsis
 |||
 |||	    Err OpenBeepFolio (void)
 |||
 |||	  Description
 |||
 |||	    This procedure connects a task to the beep folio and must be executed by
 |||	    each task before it makes any beep folio calls. If a task makes an beep
 |||	    folio call before it executes OpenBeepFolio(), the task fails.
 |||
 |||	    Call CloseBeepFolio() to relinquish a task's access to the beep folio.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error code
 |||	    (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in libc.a V30.
 |||
 |||	  Associated Files
 |||
 |||	    <beep/beep.h>
 |||
 |||	  See Also
 |||
 |||	    CloseBeepFolio()
 **/

 /**
 |||	AUTODOC -class Beep -name CloseBeepFolio
 |||	Closes the beep folio.
 |||
 |||	  Synopsis
 |||
 |||	    Err CloseBeepFolio (void)
 |||
 |||	  Description
 |||
 |||	    This procedure closes a task's connection with the beep folio. Call
 |||	    it when your application finishes using beep calls.
 |||
 |||	  Return Value
 |||
 |||	    The procedure returns a non-negative value if successful or an error code
 |||	    (a negative value) if an error occurs.
 |||
 |||	  Implementation
 |||
 |||	    Library call implemented in libc.a V30.
 |||
 |||	  Associated Files
 |||
 |||	    <beep/beep.h>
 |||
 |||	  See Also
 |||
 |||	    OpenBeepFolio()
 **/


/* -------------------- main() */

static Err __CreateModule (void);
static Err __DeleteModule (void);

/*
	main - loader entry point called by an Item Server thread of the Operator.

	Arguments
		op
			One of the DEMANDLOAD_MAIN_ op codes.

		task
			Corresponds to the Task parameter given to the
			create/delete/open/close module item routines.

		module
			The item of the current module.

	Results
		Non-negative value on success. No meaning is derived from this result
		other than than that the call succeeded.

		Negative error code on failure.
*/

int32 main (int32 op, Item task, Item module)
{
	TOUCH(task);
	TOUCH(module);

	switch (op)
	{
		case DEMANDLOAD_MAIN_CREATE: return __CreateModule();
		case DEMANDLOAD_MAIN_DELETE: return __DeleteModule();
		default                    : return 0;
	}
}

static Err __CreateModule(void)
{
	Err errcode;

	DBUG(("beep: __CreateModule()\n"));

	if ((errcode = dsppLock = OwnDSPP(BEEP_ERR_DSP_BUSY)) < 0) goto clean;
	if ((errcode = gBeepFolioItem = CreateItem (MKNODEID (KERNELNODE, FOLIONODE), gBeepFolioTags)) < 0) goto clean;

	return 0;

clean:
	__DeleteModule();
	return errcode;
}

static Err __DeleteModule(void)
{
	DBUG(("beep: __DeleteModule()\n"));

	DeleteItem (gBeepFolioItem);
	DisownDSPP (dsppLock);
	return 0;
}


/* -------------------- Folio */

/******************************************************************/
/* This is called by the kernel when the folio item is created. */
static int32 InitBeepFolio (BeepFolio *bb)
{
	int32 Result;
	ItemRoutines *itr;

	bb->bf_BeepModule = FindCurrentModule();

/* Set pointers to required Folio functions. */
	itr = bb->bf_Folio.f_ItemRoutines;
	itr->ir_Delete = internalDeleteBeepItem;
	itr->ir_Create = internalCreateBeepItem;
	itr->ir_Find = internalFindBeepItem;
	itr->ir_Open = internalOpenBeepItem;
	itr->ir_Close = internalCloseBeepItem;
	itr->ir_SetPriority = internalSetItemPriority;
	itr->ir_SetOwner = internalSetItemOwner;

	Result = beepInitDSP();
	if (Result) return Result;

	return Result;
}

/******************************************************************/
/* This is called by the kernel when the folio item is deleted. */
static int32 TermBeepFolio (BeepFolio *bb)
{
	int32 Result;

	TOUCH(bb);

	Result = beepTermDSP();
	if (Result) return Result;

	return Result;
}

/******************************************************************/
/*
** Internal routines required of all folios.
*/

/*
** This routine is passed an item pointer in n.
** The item is (I think) allocated by CreateItem in the kernel
** based on information in the BeepNodeData array.
*/
static Item internalCreateBeepItem (void *n, uint8 ntype, void *args)
{
	Item Result;

DBUG(("internalCreateBeepItem(0x%lx, %d, 0x%lx)\n", n, ntype, args));
	CHECK_VALID_RAM(args, sizeof(int32) );

	if(!IS_BEEP_OPEN) return BEEP_ERR_FOLIO_NOT_OPEN;

	switch (ntype)
	{
		case BEEP_MACHINE_NODE:
			Result = internalCreateBeepMachine ((BeepMachine *)n, (TagArg *)args);
			break;

		default:
			ERR(("internalCreateBeepItem: unrecognised type = %d\n",
				((ItemNode *)n)->n_Type ));
			Result = BEEP_ERR_BADITEM;
			break;
	}

DBUG(("internalCreateBeepItem returns 0x%lx\n", Result));
  return (Result);
}

/******************************************************************/
static int32 internalDeleteBeepItem (Item it, Task *t)
{
	Node *n;
	int32 Result;

	TOUCH(t);
DBUG(("DeleteBeepItem (it=0x%lx, Task=0x%lx)\n", it, t));

	n = (Node *) LookupItem (it);
DBUG(("DeleteBeepItem: n = $%x, type = %d\n", n, n->n_Type));

	switch (n->n_Type)
	{
		case BEEP_MACHINE_NODE:
			Result = internalDeleteBeepMachine ((BeepMachine *)n);
			break;

		default:
			ERR(("internalDeleteBeepItem: unrecognised type = %d\n", n->n_Type));
			Result = BEEP_ERR_BADITEM;
			break;
	}

DBUG(("DeleteBeepItem: Result = $%x\n", Result));
  return Result;
}

/******************************************************************/
static Item internalFindBeepItem (int32 ntype, TagArg *tp)
{
DBUG(("internalFindBeepItem (0x%lx, 0x%lx)\n", ntype, tp));

	TOUCH(ntype);
	TOUCH(tp);
	return BEEP_ERR_UNIMPLEMENTED;
}

/******************************************************************/
static Item internalOpenBeepItem (Node *n, void *args, Task *t)
{
DBUG(("OpenBeepItem (0x%lx, 0x%lx)\n", n, args));
	TOUCH(n);
	TOUCH(args);
	TOUCH(t);
	return BEEP_ERR_UNIMPLEMENTED;
}

/******************************************************************/
static int32 internalCloseBeepItem (Item it, Task *t)
{
	TOUCH(it);
	TOUCH(t);
	return BEEP_ERR_UNIMPLEMENTED;
}

/******************************************************************/
static int32 internalSetItemPriority(ItemNode *n, uint8 pri, Task *t)
{
	TOUCH(n);
	TOUCH(pri);
	TOUCH(t);
	return BEEP_ERR_UNIMPLEMENTED;
}

/***************************************************************** 940617 */
static Err internalSetItemOwner(ItemNode *n, Item NewOwner, Task *t)
{
	TOUCH(n);
	TOUCH(NewOwner);
	TOUCH(t);
	return BEEP_ERR_UNIMPLEMENTED;
}
