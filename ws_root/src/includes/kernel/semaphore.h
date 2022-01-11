#ifndef __KERNEL_SEMAPHORE_H
#define __KERNEL_SEMAPHORE_H


/******************************************************************************
**
**  @(#) semaphore.h 95/09/12 1.16
**
**  Kernel semaphore management definitions
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __KERNEL_NODES_H
#include <kernel/nodes.h>
#endif

#ifndef __KERNEL_KERNELNODES_H
#include <kernel/kernelnodes.h>
#endif

#ifndef __KERNEL_LIST_H
#include <kernel/list.h>
#endif

#ifndef __KERNEL_ITEM_H
#include <kernel/item.h>
#endif

#ifndef __KERNEL_TASK_H
#include <kernel/task.h>
#endif


/*****************************************************************************/


#ifndef EXTERNAL_RELEASE

typedef struct SemaphoreWaitNode
{
    MinNode  swn;
    Task    *swn_Task;
    bool     swn_Shared;
} SemaphoreWaitNode;

typedef struct SharedLocker
{
    MinNode  sl;
    Task    *sl_Task;
    uint32   sl_NestCnt;
} SharedLocker;

#endif /* EXTERNAL_RELEASE */

typedef struct Semaphore
{
    ItemNode  s;
    Task     *sem_Locker;	     /* primary locker task            */
    uint32    sem_NestCnt;	     /* # times primary locker locked  */
    void     *sem_UserData;          /* user-private data              */
#ifndef EXTERNAL_RELEASE
    List      sem_TaskWaitingList;   /* folks waiting on the semaphore */
    List      sem_SharedLockers;     /* list of shared lockers         */
#endif
} Semaphore;

/* for Semaphore.n_Flags */
#define SEMAPHORE_SHAREDREAD 0x1  /* semaphore currently locked in read-mode */


enum semaphore_tags
{
    CREATESEMAPHORE_TAG_USERDATA = TAG_ITEM_LAST+1    /* value to put in sem_UserData */
};

/* flags for LockSemaphore() */
#define SEM_WAIT        1
#define SEM_SHAREDREAD  2

/* convenient way to access a Semaphore structure starting from its item number */
#define SEMAPHORE(semItem) ((Semaphore *)LookupItem(semItem))


/*****************************************************************************/


#ifdef  __cplusplus
extern "C" {
#endif  /* __cplusplus */


extern int32 LockSemaphore(Item sem, uint32 flags);
extern Err UnlockSemaphore(Item sem);
extern Item CreateSemaphore(const char *name, uint8 pri);
extern Item CreateUniqueSemaphore(const char *name, uint8 pri);


#ifdef  __cplusplus
}
#endif  /* __cplusplus */


#define FindSemaphore(name)  FindNamedItem(MKNODEID(KERNELNODE,SEMAPHORENODE),(name))
#define DeleteSemaphore(sem) DeleteItem(sem)


/*****************************************************************************/


#endif /* __KERNEL_SEMAPHORE_H */
