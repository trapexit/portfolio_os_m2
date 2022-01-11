/* @(#) font_folio.c 96/07/31 1.20 */

#include <kernel/types.h>
#include <kernel/kernel.h>
#include <kernel/kernelnodes.h>
#include <kernel/debug.h>
#include <kernel/list.h>
#include <kernel/super.h>
#include <kernel/semaphore.h>
#include <kernel/mem.h>
#include <loader/loader3do.h>
#include "fontitem.h"
#include "font_folio.h"


/****************************************************************************/


/* Info on nodes maintained by this folio */
static NodeData FolioNodeData[] =
{
    { 0,                      0 },
    { sizeof(FontDescriptor), NODE_NAMEVALID | NODE_ITEMVALID | NODE_SIZELOCKED | NODE_OPENVALID}
};
#define NUM_NODEDATA (sizeof(FolioNodeData)/sizeof(NodeData))


/****************************************************************************/


static int32 InitFolio(FontFolio *fontBase);
static int32 DeleteFolio(FontFolio *fontBase);


/****************************************************************************/


/* Tags used when creating the Folio */
static TagArg FolioTags[] =
{
    { TAG_ITEM_NAME,                (void *) FONT_FOLIONAME},          /* name of folio              */
    { CREATEFOLIO_TAG_ITEM,         (void *) NST_FONT},
    { CREATEFOLIO_TAG_DATASIZE,     (void *) sizeof(FontFolio)},       /* size of folio          */
    { CREATEFOLIO_TAG_INIT,         (void *) ((int32)InitFolio) },     /* called when creating folio */
    { CREATEFOLIO_TAG_DELETEF,      (void *) ((int32)DeleteFolio) },   /* called when deleting folio */
    { CREATEFOLIO_TAG_NODEDATABASE, (void *) FolioNodeData },          /* node database   */
    { CREATEFOLIO_TAG_MAXNODETYPE,  (void *) NUM_NODEDATA },           /* number of nodes */
    { TAG_END,                      (void *) 0 },                      /* end of tag list */
};


/*****************************************************************************/


static Item  FolioItem;
FontFolio   *FontBase;


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
        case FONT_FONT_NODE : return (CreateFontItem((FontDescriptor *)node,(TagArg *)args));
        default             : return (FONT_ERR_BADSUBTYPE);
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
        case FONT_FONT_NODE : return (DeleteFontItem((FontDescriptor *)node));
        default             : return (FONT_ERR_BADITEM);
    }
}


/******************************************************************/


static Item FindFolioItem(int32 ntype, TagArg *args)
{
    switch (ntype)
    {
        case FONT_FONT_NODE : return (FindFontItem(args));
        default             : return (FONT_ERR_BADITEM);
    }
}


/******************************************************************/


static int32 CloseFolioItem(Item it, Task *task)
{
Node *node;

    TOUCH(task);

    node = (Node *) LookupItem(it);

    switch (node->n_Type)
    {
        case FONT_FONT_NODE : return (CloseFontItem((FontDescriptor *)node));
        default             : return (FONT_ERR_BADITEM);
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


static Item LoadFolioItem(int32 nodeType, TagArg *args)
{
    switch (nodeType)
    {
        case FONT_FONT_NODE : return (LoadFontItem(args));
        default             : return (FONT_ERR_BADSUBTYPE);
    }
}


/****************************************************************************/


/* This is called by the kernel when the folio item is created */
static int32 InitFolio(FontFolio *fontBase)
{
ItemRoutines *itr;
Item          result;

    FontBase = fontBase;

    /* Set pointers to required Folio functions. */
    itr              = fontBase->ff.f_ItemRoutines;
    itr->ir_Delete   = DeleteFolioItem;
    itr->ir_Create   = CreateFolioItem;
    itr->ir_Find     = FindFolioItem;
    itr->ir_Close    = CloseFolioItem;
    itr->ir_SetOwner = SetOwnerFolioItem;
    itr->ir_Load     = LoadFolioItem;

    PrepList(&fontBase->ff_Fonts);

    result = CreateSemaphore("Font Folio",0);
    fontBase->ff_FontLock = (Semaphore *)LookupItem(result);

    FontBase->ff_CacheLock = 0;
    FontBase->ff_Cache = AllocMemPages(CACHE_SIZE, MEMTYPE_NORMAL);
    if (FontBase->ff_Cache)
    {
        ControlMem(FontBase->ff_Cache, CACHE_SIZE, MEMC_OKWRITE, 0);
        FontBase->ff_CacheLock = CreateSemaphore(NULL, 0);
        if (FontBase->ff_CacheLock < 0)
        {
            FreeMem(FontBase->ff_Cache, CACHE_SIZE);
            FontBase->ff_Cache = NULL;
        }
    }

    return result;
}


/****************************************************************************/


/* This is called by the kernel when the folio item is deleted */
static int32 DeleteFolio(FontFolio *fontBase)
{
    SuperDeleteItem(fontBase->ff_CacheLock);
    SuperFreeMem(fontBase->ff_Cache, CACHE_SIZE);
    SuperDeleteItem(fontBase->ff_FontLock->s.n_Item);

    return (0);
}
