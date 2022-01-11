/* @(#) international_folio.c 96/07/31 1.29 */

/* #define TRACING */

#include <kernel/types.h>
#include <kernel/folio.h>
#include <kernel/task.h>
#include <kernel/nodes.h>
#include <kernel/kernelnodes.h>
#include <kernel/item.h>
#include <kernel/operror.h>
#include <kernel/super.h>
#include <kernel/debug.h>
#include <loader/loader3do.h>
#include <stdio.h>
#include <string.h>
#include "locales.h"
#include "international_folio.h"


/****************************************************************************/


/* Info on nodes maintained by this folio */
static NodeData FolioNodeData[] =
{
    { 0,              0 },
    { sizeof(Locale), NODE_ITEMVALID | NODE_NAMEVALID | NODE_SIZELOCKED | NODE_OPENVALID }
};
#define NUM_NODEDATA (sizeof(FolioNodeData)/sizeof(NodeData))


/****************************************************************************/


static int32 InitFolio(InternationalFolio *internationalBase);
static int32 DeleteFolio(InternationalFolio *internationalBase);


/****************************************************************************/


/* Tags used when creating the Folio */
static TagArg FolioTags[] =
{
    { TAG_ITEM_NAME,                (void *) INTL_FOLIONAME},              /* name of folio              */
    { CREATEFOLIO_TAG_ITEM,         (void *) NST_INTL},
    { CREATEFOLIO_TAG_DATASIZE,     (void *) sizeof(InternationalFolio)},  /* size of folio          */
    { CREATEFOLIO_TAG_INIT,         (void *) ((int32)InitFolio) },   /* called when creating folio */
    { CREATEFOLIO_TAG_DELETEF,      (void *) ((int32)DeleteFolio) },   /* called when deleting folio */
    { CREATEFOLIO_TAG_NODEDATABASE, (void *) FolioNodeData },          /* node database   */
    { CREATEFOLIO_TAG_MAXNODETYPE,  (void *) NUM_NODEDATA },           /* number of nodes */
    { TAG_END,                      (void *) 0 },                      /* end of tag list */
};


/*****************************************************************************/


static Item         FolioItem;
InternationalFolio *InternationalBase;


/****************************************************************************/


int main(int32 op)
{
    switch (op)
    {
	case DEMANDLOAD_MAIN_CREATE: return (FolioItem = CreateItem(MKNODEID(KERNELNODE,FOLIONODE), FolioTags));
        case DEMANDLOAD_MAIN_DELETE: return DeleteItem(FolioItem);
	default                    : return 0;
    }
}


/****************************************************************************/


static Item CreateFolioItem(void *node, uint8 nodeType, void *args)
{
    switch (nodeType)
    {
        case INTL_LOCALE_NODE : return (CreateLocaleItem((Locale *)node,(TagArg *)args));
        default               : return (INTL_ERR_BADSUBTYPE);
    }
}


/****************************************************************************/


static int32 DeleteFolioItem(Item it, Task *task)
{
Node *node;

    TOUCH(task);

    node = (Node *)LookupItem(it);

    switch (node->n_Type)
    {
        case INTL_LOCALE_NODE : return (DeleteLocaleItem((Locale *)node));
        default               : return (INTL_ERR_BADITEM);
    }
}


/*****************************************************************************/


static int32 CloseFolioItem(Item it, Task *task)
{
Node *node;

    TOUCH(task);

    node = (Node *)LookupItem(it);

    switch (node->n_Type)
    {
        case INTL_LOCALE_NODE : return (CloseLocaleItem((Locale *)node));
        default               : return (INTL_ERR_BADITEM);
    }
}


/******************************************************************/


static Item FindFolioItem(int32 ntype, TagArg *args)
{
    TRACE(("FINDFOLIOITEM: entering with ntype = %ld\n",ntype));

    switch (ntype)
    {
        case INTL_LOCALE_NODE : return (FindLocaleItem(args));
        default               : return (INTL_ERR_BADITEM);
    }
}


/******************************************************************/


static Item LoadFolioItem(int32 ntype, TagArg *args)
{
    TRACE(("LOADFOLIOITEM: entering with ntype = %ld\n",ntype));

    switch (ntype)
    {
        case INTL_LOCALE_NODE : return (LoadLocaleItem(args));
        default               : return (INTL_ERR_BADITEM);
    }
}


/******************************************************************/


static Err SetOwnerFolioItem(ItemNode *n, Item newOwner, Task *task)
{
    TOUCH(n);
    TOUCH(newOwner);
    TOUCH(task);
    return 0;
}


/****************************************************************************/


/* This is called by the kernel when the folio item is created */
static int32 InitFolio(InternationalFolio *internationalBase)
{
ItemRoutines *itr;
int32         result;

    TRACE(("INITFOLIO: entering\n"));

    InternationalBase = internationalBase;

    /* Set pointers to required Folio functions. */
    itr              = InternationalBase->iff.f_ItemRoutines;
    itr->ir_Delete   = DeleteFolioItem;
    itr->ir_Create   = CreateFolioItem;
    itr->ir_Close    = CloseFolioItem;
    itr->ir_Find     = FindFolioItem;
    itr->ir_Load     = LoadFolioItem;
    itr->ir_SetOwner = SetOwnerFolioItem;

    InternationalBase->if_DefaultLocale = INTL_ERR_ITEMNOTFOUND;
    InternationalBase->if_LocaleLock = result = CreateSemaphore("International Folio",0);

    TRACE(("INITFOLIO: exiting with %d\n",result));

    return result;
}


/****************************************************************************/


/* This is called by the kernel when the folio item is deleted */
static int32 DeleteFolio(InternationalFolio *internationalBase)
{
    TRACE(("DELETEFOLIO: entering\n"));

    SuperDeleteSemaphore(internationalBase->if_LocaleLock);

    TRACE(("DELETEFOLIO: exiting with 0\n"));

    return (0);
}
