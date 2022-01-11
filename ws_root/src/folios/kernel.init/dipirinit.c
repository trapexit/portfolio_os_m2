/* @(#) dipirinit.c 96/02/12 1.1 */

/*#define DEBUG*/

#include <kernel/types.h>
#include <kernel/protlist.h>
#include <kernel/listmacros.h>
#include <kernel/item.h>
#include <kernel/semaphore.h>
#include <kernel/kernel.h>
#include <kernel/kernelnodes.h>
#include <kernel/io.h>
#include <kernel/task.h>
#include <kernel/super.h>
#include <kernel/mem.h>
#include <kernel/operror.h>
#include <stdio.h>
#include <string.h>
#include <kernel/internalf.h>
#include <kernel/dipir.h>

#ifndef BUILD_STRINGS
#undef DEBUG
#endif

#ifdef DEBUG
#define DBUG(x) printf x
#else
#define DBUG(x)
#define PrintfSysErr(err)
#endif


Err initDuckAndRecoverSems(void)
{
    /* allocate Duck and Recover list semaphores */

    Semaphore* sem;
    Item ret;

    sem= (Semaphore*)SuperAllocNode(&KB_FIELD(kb), SEMA4NODE);
    if(!sem) return NOMEM;
    ret= SuperInternalCreateSemaphore(sem, 0);
    if(ret < 0) return ret;
    KB_FIELD(kb_DuckFuncs.l_Semaphore)= ret;

    sem= (Semaphore*)SuperAllocNode(&KB_FIELD(kb), SEMA4NODE);
    if(!sem) return NOMEM;
    KB_FIELD(kb_RecoverFuncs.l_Semaphore)= SuperInternalCreateSemaphore(sem, 0);
    return KB_FIELD(kb_RecoverFuncs.l_Semaphore);
}
