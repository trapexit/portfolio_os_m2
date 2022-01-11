/* @(#) memlockinit.c 96/07/03 1.2 */

#include <kernel/types.h>
#include <kernel/semaphore.h>
#include <kernel/nodes.h>
#include <kernel/list.h>
#include <kernel/listmacros.h>
#include <kernel/task.h>
#include <kernel/item.h>
#include <kernel/kernelnodes.h>
#include <kernel/operror.h>
#include <kernel/folio.h>
#include <kernel/kernel.h>
#include <kernel/memlock.h>
#include <kernel/panic.h>
#include <kernel/super.h>
#include <stdio.h>
#include <kernel/internalf.h>


/*****************************************************************************/


extern Semaphore *_MLsem;


/*****************************************************************************/


static const TagArg tags[2] =
{
    TAG_ITEM_NAME, (void *)"MemLock Handlers",
    TAG_END,	   0,
};

extern MemLockHandler _MLioReqHandler;



Err InitMemLock(void)
{
    _MLsem = (Semaphore *)SuperAllocNode((Folio *) KernelBase,SEMA4NODE);
    if (!_MLsem)
        return NOMEM;

    KB_FIELD(kb_MemLockSemaphore) = SuperInternalCreateSemaphore(_MLsem,tags);
    if (KB_FIELD(kb_MemLockSemaphore) > 0)
        InsertNodeFromTail(&KB_FIELD(kb_MemLockHandlers),(Node *)&_MLioReqHandler);

    return KB_FIELD(kb_MemLockSemaphore);
}
