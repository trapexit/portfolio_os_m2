/* @(#) devicesinit.c 96/07/03 1.2 */

#include <kernel/types.h>
#include <kernel/list.h>
#include <kernel/listmacros.h>
#include <kernel/item.h>
#include <kernel/device.h>
#include <kernel/driver.h>
#include <kernel/ddfnode.h>
#include <kernel/kernel.h>
#include <kernel/kernelnodes.h>
#include <kernel/io.h>
#include <kernel/task.h>
#include <kernel/usermodeservices.h>
#include <kernel/mem.h>
#include <kernel/operror.h>
#include <kernel/semaphore.h>
#include <kernel/super.h>
#include <stdio.h>
#include <string.h>
#include <kernel/internalf.h>

#define NO_DDF ((DDFNode*)1)

#ifdef DEBUG
#define DBUG(x)		  printf x
#undef DEBUG_SAMESTACK
#else
#define DBUG(x)
#undef DEBUG_SAMESTACK
#endif

extern Semaphore *_DevSemaphore;


/*****************************************************************************/


static const TagArg dtags[2] =
{	TAG_ITEM_NAME, (void *)"DevList",
	TAG_END,	0,
};

Err initDevSem(void)
{
    /* allocate device list semaphore */

    _DevSemaphore = (Semaphore *)SuperAllocNode(KernelBase, SEMA4NODE);
    if (!_DevSemaphore) return NOMEM;
    KB_FIELD(kb_DevSemaphore) = SuperInternalCreateSemaphore(_DevSemaphore,dtags);
    return KB_FIELD(kb_DevSemaphore);
}
