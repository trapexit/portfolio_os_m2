#ifndef __KERNEL_MEMLOCK_H
#define __KERNEL_MEMLOCK_H


/******************************************************************************
**
**  @(#) memlock.h 96/02/20 1.6
**
**  Interface to the memory lock manager.
**
**  Supervisor mode code can install a memlock handler by initializing a
**  MemLockHandler structure, and passing it in to
**  SuperInternalInsertMemLockHandler().
**
**  Whenever tasks loose write permission to a memory region, the kernel
**  invokes each of the memlock handlers in succession to give them a
**  chance to abort any asynchronous operation that might be occuring on
**  the memory region on behalf of the task loosing write permission.
**
**  Each handler gets called with a pointer to the task which is loosing
**  write permission to the memory region. If the task pointer is NULL,
**  it means all tasks are loosing write permission. The handlers also
**  get passed the address and size of the memory region being affected.
**
**  IMPORTANT: When your handler gets passed a task pointer, it must abort
**             all operations being done on that memory for the given task,
**             AS WELL AS ANY THREADS OF THAT TASK. Your handler will always
**             be called with a true task pointer, not a thread pointer.
**
******************************************************************************/


#ifdef EXTERNAL_RELEASE
#error "This file may not be used in externally released source code or link lib"
#endif

#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_NODES_H
#include <kernel/nodes.h>
#endif

#ifndef __KERNEL_TASK_H
#include <kernel/task.h>
#endif


/*****************************************************************************/


typedef void (* MemLockCallBack)(Task *t, const void *, uint32);

typedef struct MemLockHandler
{
    Node            mlh;
    MemLockCallBack mlh_CallBack;
} MemLockHandler;


/*****************************************************************************/


/* client routines */
void SuperInternalInsertMemLockHandler(MemLockHandler *handler);
void SuperInternalRemoveMemLockHandler(MemLockHandler *handler);

/* kernel private */
void InvokeMemLockHandlers(Task *t, const void *p, uint32 size);
Err InitMemLock(void);
void internalInsertMemLockHandler(MemLockHandler *handler);
void internalRemoveMemLockHandler(MemLockHandler *handler);


/*****************************************************************************/


#endif /* __KERNEL_MEMLOCK_H */
