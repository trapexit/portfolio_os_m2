/* @(#) memlock.c 96/02/12 1.19 */

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
#include <stdio.h>
#include <kernel/internalf.h>


/*****************************************************************************/


Semaphore *_MLsem = NULL;


/*****************************************************************************/


void internalInsertMemLockHandler(MemLockHandler *handler)
{
    /* we don't really need to init these, but I'm anal... */
    handler->mlh.n_SubsysType = KERNELNODE;
    handler->mlh.n_Type       = MEMLOCKNODE;

    if (CURRENTTASK)
        internalLockSemaphore(_MLsem,SEM_WAIT);

    InsertNodeFromTail(&KB_FIELD(kb_MemLockHandlers),(Node *)handler);

    if (CURRENTTASK)
        internalUnlockSemaphore(_MLsem);
}


/*****************************************************************************/


void internalRemoveMemLockHandler(MemLockHandler *handler)
{
    if (CURRENTTASK)
        internalLockSemaphore(_MLsem,SEM_WAIT);

    REMOVENODE((Node *)handler);

    if (CURRENTTASK)
        internalUnlockSemaphore(_MLsem);
}


/*****************************************************************************/


void InvokeMemLockHandlers(Task *t, const void *p, uint32 size)
{
MemLockHandler *handler;

    if (!_MLsem)
    {
        /* we're getting called early in the boot process before we
         * have inited the semaphore. There's obviously nothing to do.
         */
        return;
    }

    if (t && t->t_ThreadTask)
        t = t->t_ThreadTask;

    internalLockSemaphore(_MLsem,SEM_WAIT);

#ifdef BUILD_PARANOIA
    if (_MLsem->sem_NestCnt > 1)
    {
        printf("WARNING: Attempting to lock the memlock semaphore recursively!\n");
        printf("         This isn't allowed, about to PANIC.\n");
        PANIC(ER_Kr_MemLockErr);
    }
#endif

    ScanList(&KB_FIELD(kb_MemLockHandlers),handler,MemLockHandler)
    {
        (* handler->mlh_CallBack)(t,p,size);
    }

    internalUnlockSemaphore(_MLsem);
}


/*****************************************************************************/

extern void AbortIOReqs(Task *t, const void *p, uint32 len);


MemLockHandler _MLioReqHandler =
{
    {NULL,NULL,KERNELNODE,MEMLOCKNODE,0,0,sizeof(MemLockHandler),"IOReq MemLock Handler"},
    AbortIOReqs
};


