/*
 *  @(#) icon_folio.c 96/07/31 1.9
 *  Primary folio creation code.
 */

#include <kernel/types.h>
#include <kernel/kernel.h>
#include <kernel/kernelnodes.h>
#include <kernel/debug.h>
#include <kernel/list.h>
#include <kernel/super.h>
#include <kernel/semaphore.h>
#include <kernel/mem.h>
#include <loader/loader3do.h>
#include <misc/iff.h>
#include <ui/icon.h>
#include <dipir/dipirpub.h>
#include "icon_protos.h"


/*****************************************************************************/


static void *(*switable[])() =
{
    (void *(*)())internalGetHWIcon			/* 0 */
};
#define NSWIS		(sizeof(switable) / sizeof(void *(*)()))


/*****************************************************************************/


/* Tags used when creating the Folio */
static TagArg FolioTags[] =
{
    { TAG_ITEM_NAME,            (void *) ICON_FOLIONAME },          /* name of folio              */
    { CREATEFOLIO_TAG_NSWIS,    (void *) NSWIS },
    { CREATEFOLIO_TAG_SWIS,     (void *) switable},
    { CREATEFOLIO_TAG_ITEM,     (void *) NST_ICON },
    { TAG_END,                  (void *) 0 }                       /* end of tag list */
};


/*****************************************************************************/


static Item FolioItem;


/*****************************************************************************/


int main(int32 op)
{
    switch (op)
    {
	case DEMANDLOAD_MAIN_CREATE: return (FolioItem = CreateItem(MKNODEID(KERNELNODE,FOLIONODE), FolioTags));
        case DEMANDLOAD_MAIN_DELETE: return DeleteItem(FolioItem);
	default                    : return 0;
    }
}
