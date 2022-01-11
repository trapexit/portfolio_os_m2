/* @(#) createUniqueSema4.c 95/08/29 1.4 */

#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/kernelnodes.h>
#include <kernel/semaphore.h>


/*****************************************************************************/


Item
CreateUniqueSemaphore(const char *name, uint8 pri)
{
TagArg ta[4];

    ta[0].ta_Tag = TAG_ITEM_PRI;
    ta[0].ta_Arg = (void *)pri;

    ta[1].ta_Tag = TAG_ITEM_UNIQUE_NAME;
    ta[1].ta_Arg = 0;

    if (name)
    {
        ta[2].ta_Tag = TAG_ITEM_NAME;
        ta[2].ta_Arg = (void *)name;
    }
    else
    {
        ta[2].ta_Tag = TAG_NOP;
    }

    ta[3].ta_Tag = TAG_END;

    return CreateItem(MKNODEID(KERNELNODE,SEMA4NODE),ta);
}
