/* @(#) semaphore.c 96/06/10 1.34 */

#include <kernel/types.h>
#include <kernel/nodes.h>
#include <kernel/kernelnodes.h>
#include <kernel/list.h>
#include <kernel/listmacros.h>
#include <kernel/folio.h>
#include <kernel/task.h>
#include <kernel/mem.h>
#include <kernel/kernel.h>
#include <kernel/semaphore.h>
#include <kernel/operror.h>
#include <kernel/lumberjack.h>
#include <stdio.h>
#include <kernel/internalf.h>


#ifdef BUILD_STRINGS
#define INFO(x)		printf x
#else
#define INFO(x)
#endif

#define DBUG(x)		/*printf x*/
#define DBUGCS(x)	/*printf x*/
#define DBUGLS(x)	/*printf x*/
#define DBUGUS(x)	/*printf x*/


/*****************************************************************************/


static int32 icsem_c(Semaphore *sem, void *p, uint32 tag, uint32 arg)
{
    TOUCH(p);

    switch (tag)
    {
        case CREATESEMAPHORE_TAG_USERDATA: sem->sem_UserData = (void *)arg;
                                           break;

        default                          : return BADTAG;
    }

    return 0;
}


/**
|||	AUTODOC -class Kernel -group Semaphores -name CreateSemaphore
|||	Creates a semaphore.
|||
|||	  Synopsis
|||
|||	    Item CreateSemaphore( const char *name, uint8 pri )
|||
|||	  Description
|||
|||	    This function creates a semaphore item with the specified name and
|||	    priority. You can use this function in place of CreateItem() to
|||	    create the semaphore.
|||
|||	    When you no longer need a semaphore, use DeleteSemaphore() to
|||	    delete it.
|||
|||	    If you give a name to the semaphore when creating it, other
|||	    tasks will be able to locate the semaphore by using the
|||	    FindSemaphore() function.
|||
|||	  Arguments
|||
|||	    name
|||	        The name of the semaphore, or NULL if it is unnamed.
|||
|||	    pri
|||	        The priority of the semaphore; use 0 for now.
|||
|||	  Return Value
|||
|||	    Returns the item number of the semaphore or a negative error code
|||	    for failure.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libc.a V20.
|||
|||	  Associated Files
|||
|||	    <kernel/semaphore.h>, libc.a
|||
|||	  See Also
|||
|||	    DeleteSemaphore(), LockSemaphore(), UnlockSemaphore()
|||
**/

/**
|||	AUTODOC -class Kernel -group Semaphores -name CreateUniqueSemaphore
|||	Creates a semaphore with a unique name.
|||
|||	  Synopsis
|||
|||	    Item CreateUniqueSemaphore( const char *name, uint8 pri )
|||
|||	  Description
|||
|||	    This function creates a semaphore item with the specified name and
|||	    priority. You can use this function in place of CreateItem() to
|||	    create the semaphore.
|||
|||	    When you no longer need a semaphore, use DeleteSemaphore() to
|||	    delete it.
|||
|||	    This function works much like CreateSemaphore(), except that it
|||	    guarantees that no other semaphore item of the same name already
|||	    exists. And once this semaphore created, no other semaphore of the
|||	    same name is allowed to be created.
|||
|||	  Arguments
|||
|||	    name
|||	        The name of the semaphore.
|||
|||	    pri
|||	        The priority of the semaphore; use 0 for now.
|||
|||	  Return Value
|||
|||	    Returns the item number of the semaphore or a negative error code
|||	    for failure. If a semaphore of the same name already existed when
|||	    this call was made, the UNIQUEITEMEXISTS error will be
|||	    returned.
|||
|||	  Implementation
|||
|||	    Link library call implemented in libc.a V24.
|||
|||	  Associated Files
|||
|||	    <kernel/semaphore.h>, libc.a
|||
|||	  See Also
|||
|||	    DeleteSemaphore(), LockSemaphore(), UnlockSemaphore()
|||
**/

/**
|||	AUTODOC -class Items -name Semaphore
|||	An item used to arbitrate access to shared resources.
|||
|||	  Description
|||
|||	    Semaphores are used to protect shared resources. Before accessing a
|||	    shared resource, a task must first try to lock the associated
|||	    semaphore. Only one task at a time can have the semaphore locked at
|||	    any one moment. This prevents multiple tasks from accessing the same
|||	    data at the same time.
|||
|||	  Folio
|||
|||	    Kernel
|||
|||	  Item Type
|||
|||	    SEMA4NODE
|||
|||	  Create
|||
|||	    CreateSemaphore(), CreateUniqueSemaphore()
|||
|||	  Delete
|||
|||	    DeleteSemaphore(),
|||
|||	  Query
|||
|||	    FindSemaphore()
|||
|||	  Use
|||
|||	    LockSemaphore(), UnlockSemaphore()
|||
|||	  Tags
|||
|||	    CREATESEMAPHORE_TAG_USERDATA (void *) - Create
|||	        Lets you specify the 32-bit value that gets put into the
|||	        sem_UserData field of the Semaphore structure. This can be
|||	        anything you want, and is sometimes useful to idenify a
|||	        semaphore port between tasks.
|||
**/

Item
internalCreateSemaphore(Semaphore *s, TagArg *a)
{
    int32 ret;

    DBUGCS(("CreateSemaphore\n"));

    ret = TagProcessor(s, a, icsem_c, 0);

    if (ret >= 0)
    {
    	PrepList(&s->sem_TaskWaitingList);
    	PrepList(&s->sem_SharedLockers);
    	InsertNodeFromTail(&KB_FIELD(kb_Semaphores),(Node *)s);
    	DBUGCS(("returning: %d\n",s->s.n_Item));
    	ret = s->s.n_Item;
    }

    return ret;
}


/*****************************************************************************/


static SharedLocker *GetSharedLocker(Semaphore *s, Task *t)
{
SharedLocker *sl;

    /* see if the task is already in the list of lockers */
    ScanList(&s->sem_SharedLockers,sl,SharedLocker)
    {
        if (sl->sl_Task == t)
        {
            /* found it */
            return sl;
        }
    }

    /* not in the list, so create a new node for it */
    sl = (SharedLocker *)SuperAllocMem(sizeof(SharedLocker),MEMTYPE_ANY);
    if (!sl)
        return NULL;

    ADDHEAD(&s->sem_SharedLockers,(Node *)sl);
    sl->sl_Task    = t;
    sl->sl_NestCnt = 0;

    return sl;
}


/**
|||	AUTODOC -class Kernel -group Semaphores -name LockSemaphore
|||	Locks a semaphore.
|||
|||	  Synopsis
|||
|||	    int32 LockSemaphore( Item s, uint32 flags )
|||
|||	  Description
|||
|||	    This function locks a semaphore.
|||
|||	    Semaphores are used to control access to shared resources
|||	    from different tasks or threads. By common agreement, tasks
|||	    commit to locking a semaphore before accessing data that is
|||	    associated with it.
|||
|||	    Semaphores can be locked in one of two modes. In exclusive mode,
|||	    only one task can have the semaphore locked at a time. In shared
|||	    mode, any number of tasks can have the semaphore locked in shared
|||	    mode. Typically, when you simply need to "read" a shared resource,
|||	    you should lock the semaphore in shared mode. If you need to modify
|||	    the resource, you should lock it in exclusive mode.
|||
|||	    Semaphore locks nest. If you currently have a semaphore locked in
|||	    exclusive mode and you try to relock it in shared mode, it is
|||	    simply relocked in exclusive mode. It is not legal to relock a
|||	    shared mode semaphore in exclusive mode.
|||
|||	  Arguments
|||
|||	    s
|||	        The item number of the semaphore to lock.
|||
|||	    flags
|||	        Semaphore flags.
|||
|||	    Only two flags are currently defined:
|||
|||	    SEM_WAIT
|||	        If the semaphore can't be locked because
|||	        another task or thread currently has it
|||	        locked, setting this bit will cause
|||	        the current task to go to sleep until
|||	        the semaphore becomes available.
|||
|||	    SEM_SHAREDREAD
|||	        This lets you lock the semaphore in
|||	        shared mode. Multiple clients can lock
|||	        a semaphore in shared mode, but only
|||	        one client can lock it in exclusive
|||	        mode. If you're locking the semaphore
|||	        to simply read data, you should use
|||	        this flag. If you are locking the
|||	        semaphore to modify data, you should
|||	        NOT use this flag.
|||
|||	  Return Value
|||
|||	    Returns 1 if the semaphore was successfully locked, or 0 if the
|||	    semaphore is already locked and the SEM_WAIT flag was not set.
|||	    Returns a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/semaphore.h>, libc.a
|||
|||	  See Also
|||
|||	    FindSemaphore(), UnlockSemaphore()
|||
**/


int32 internalLockSemaphore(Semaphore *s, int32 flags)
{
Task              *ct;
bool               sharedState;
bool               sharedReq;
SharedLocker      *sl;
SemaphoreWaitNode  swn;
uint32             oldints;
Err                result;

    DBUGLS(("\ninternalLockSemaphore: entering with ($%x,$%x) '%s'\n",s,flags,s->s.n_Name));

    if (s->sem_Locker == NULL)
    {
        /* It's not locked, so just grab it! */
        s->sem_Locker  = CURRENTTASK;
        s->sem_NestCnt = 1;

        if (flags & SEM_SHAREDREAD)
            s->s.n_Flags |= SEMAPHORE_SHAREDREAD;

        DBUGLS(("internalLockSemaphore (a): returning 1\n"));

        LogSemaphoreLocked(s,(flags & SEM_SHAREDREAD ? TRUE : FALSE));

        return 1;
    }

    /* The semaphore is currently locked */

    ct          = CURRENTTASK;
    sharedState = (s->s.n_Flags & SEMAPHORE_SHAREDREAD) ? TRUE : FALSE;
    sharedReq   = (flags & SEM_SHAREDREAD) ? TRUE : FALSE;

    if (s->sem_Locker == CURRENTTASK)
    {
        /* The current task is the primary locker */

        if ((sharedState == sharedReq) || (sharedReq && !sharedState))
        {
            DBUGLS(("internalLockSemaphore (b): returning 1\n"));

            /* If the task is asking for the same lock mode, or if
             * the lock mode is currently exclusive, and the task wants
             * a shared lock, just bump the nest count.
             */
            s->sem_NestCnt++;

            LogSemaphoreLocked(s,TRUE);

            return 1;
        }

        /* Trying to lock a shared semaphore in exclusive. This ain't
         * allowed bub.
         *
         * Note that we could check to see if the current task is the only
         * semaphore locker, and promote the shared lock to an exclusive lock.
         * However, this would have the tendancy to hide bugs in applications.
         * The bugs would only surface is timing was just right and multiple
         * readers are accessing the semaphore at the time this condition
         * occurs.
         */
        INFO(("WARNING: Attempting to relock a shared read semaphore in exclusive mode\n"));
        INFO(("         semaphore name '%s', locker's name '%s'\n",s->s.n_Name,ct->t.n_Name));

        return MakeKErr(ER_SEVERE,ER_C_NSTND,ER_Kr_BadLockArg);
    }

    /* We're not the primary locker. We can still lock the semaphore if:
     *
     *  - We want to lock the semaphore in shared mode.
     *
     *  - Tne semaphore is currently in shared mode.
     *
     *  - We have already got a lock on it, or there are no exclusive waiters
     *    in the queue.
     */
    if (sharedReq && sharedState)
    {
        sl = GetSharedLocker(s,ct);
        if (!sl)
            return NOMEM;

        if ((sl->sl_NestCnt) || IsEmptyList(&s->sem_TaskWaitingList))
        {
            DBUGLS(("internalLockSemaphore (c): returning 1\n"));

            sl->sl_NestCnt++;

            LogSemaphoreLocked(s, TRUE);

            return 1;
        }
    }

    /* can't get the semaphore... */

    if ((flags & SEM_WAIT) == 0)
    {
        DBUGLS(("internalLockSemaphore (d): returning 0\n"));

        LogSemaphoreFailed(s);

        /* caller is impatient and doesn't want to wait... */
        return 0;
    }

    DBUGLS(("internalLockSemaphore: about to wait on sem $%x\n",s));
    DBUGLS(("                       ct $%x, locker $%x, nest %d\n",CURRENTTASK,s->sem_Locker,s->sem_NestCnt));

#ifdef BUILD_PARANOIA
    /* detect and report simple dead lock conditions */
    {
    Task      *competitor;
    Semaphore *sem;

        /* if the task that has the semaphore we want is waiting for
         * a semaphore that the current has got locked, then we've got
         * a deadlock on our hands...
         */

        competitor = (Task *)s->sem_Locker;
        if (competitor)
        {
            sem = CheckItem(competitor->t_WaitItem,KERNELNODE,SEMA4NODE);
            if (sem)
            {
                if (sem->sem_Locker == ct)
                {
                    INFO(("WARNING: A semaphore dead lock has been detected.\n"));
                    INFO(("         Task '%s' is trying to lock semaphore '%s' which is locked by Task '%s'\n",ct->t.n_Name,s->s.n_Name));
                    INFO(("         Task '%s' is already waiting for semaphore '%s' which is locked by '%s'\n",competitor->t.n_Name,sem->s.n_Name));
                }
            }
        }
    }
#endif

    /* stick ourselves in the semaphore's wait q */
    swn.swn_Shared = sharedReq;
    swn.swn_Task   = ct;

    /* go to sleep until something useful happens */
    LogSemaphoreWait(s);
    ADDTAIL(&s->sem_TaskWaitingList,(Node *)&swn);

    oldints = Disable();
    ct->t_WaitItem = s->s.n_Item;
    SleepTask(ct);
    result = ct->t_WaitItem;
    ct->t_WaitItem = -1;
    Enable(oldints);

    if (result < 0)
    {
        /* Either the semaphore got deleted, there wasn't enough
         * memory to let us lock it in shared mode, or the current task
         * is being aborted.
         */

        LogSemaphoreFailed(s);
        return result;
    }

    /* We got the semaphore! Everything has been done by the unlocking code,
     * so all we gotta do is return success. Aren't we lucky?
     */

    DBUGLS(("internalLockSemaphore (e): returning 1\n"));

    LogSemaphoreLocked(s,(flags & SEM_SHAREDREAD ? TRUE : FALSE));

    return 1;
}

int32 externalLockSemaphore(Item is, int32 flags)
{
Semaphore *s;

    s = (Semaphore *)CheckItem(is,KERNELNODE,SEMA4NODE);
    if (!s)
    {
        /* bad semaphore item */
        return BADITEM;
    }

    if (flags & (~(SEM_WAIT | SEM_SHAREDREAD)))
    {
        /* illegal flag specified */
        return MakeKErr(ER_SEVERE,ER_C_NSTND,ER_Kr_BadLockArg);
    }

    return internalLockSemaphore(s,flags);
}


/**
|||	AUTODOC -class Kernel -group Semaphores -name UnlockSemaphore
|||	Unlocks a semaphore.
|||
|||	  Synopsis
|||
|||	    Err UnlockSemaphore( Item s )
|||
|||	  Description
|||
|||	    This function unlocks the specified semaphore, allowing other
|||	    task to gain access to the semaphore.
|||
|||	  Arguments
|||
|||	    s
|||	        The item number of the semaphore to unlock.
|||
|||	  Return Value
|||
|||	    Returns 0 if successful or a negative error code for failure.
|||	    Possible error codes currently include:
|||
|||	    BADITEM
|||	        The item number supplied does not refer to a valid semaphore
|||	        item.
|||
|||	    NOTOWNER
|||	        The semaphore specified by the s argument is not owned by the
|||	        task.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/semaphore.h>, libc.a
|||
|||	  See Also
|||
|||	    LockSemaphore()
|||
**/

int32 internalUnlockSemaphore(Semaphore *s)
{
SemaphoreWaitNode *swn;
SharedLocker      *sl;
uint32             oldints;

    DBUGUS(("\ninternalUnlockSemaphore entering with $%x ('%s')\n",s,s->s.n_Name));

    if (s->sem_Locker == CURRENTTASK)
    {
        /* current task is the primary locker */
        s->sem_NestCnt--;

        if (s->sem_NestCnt)
        {
            DBUGUS(("internalUnlockSemaphore (a): returning with 0\n"));

            LogSemaphoreUnlocked(s);

            /* until the count drops to 0, the state isn't actually changing */
            return 0;
        }
    }
    else
    {
        /* could this be a secondary locker? */

        ScanList(&s->sem_SharedLockers,sl,SharedLocker)
        {
            if (sl->sl_Task == CURRENTTASK)
            {
                if (sl->sl_NestCnt == 0)
                {
                    /* trying to unlock more times than it was locked */
                    return NOTOWNER;
                }

                DBUGUS(("internalUnlockSemaphore (b): returning with 0\n"));

                sl->sl_NestCnt--;

                LogSemaphoreUnlocked(s);

                return 0;
            }
        }

        DBUGUS(("internalUnlockSemaphore (c): returning with NOTOWNER\n"));

        /* neither a primary or secondary locker */
        return NOTOWNER;
    }

    /* At this point, we know we're the primary locker of this semaphore,
     * and the nest count just dropped to 0. We must now either make one of
     * the secondary lockers the primary locker, or wake up a waiter and make
     * it the locker.
     */

    ScanList(&s->sem_SharedLockers,sl,SharedLocker)
    {
        if (sl->sl_NestCnt)
        {
            /* this shared locker is becoming the primary locker */
            s->sem_Locker  = sl->sl_Task;
            s->sem_NestCnt = sl->sl_NestCnt;
            sl->sl_NestCnt = 0;

            /* We leave the node in the list, since it is quite likely
             * that it will be needed again... If the task dies, the
             * node will get yanked by the task termination code.
             */

            DBUGUS(("internalUnlockSemaphore (d): returning with 0\n"));

            LogSemaphoreUnlocked(s);

            return 0;
        }
    }

    /* didn't find any secondary lockers to turn into primary lockers */

    /* The first waiter on the list becomes the primary locker */
    swn = (SemaphoreWaitNode *)RemHead(&s->sem_TaskWaitingList);
    if (swn)
    {
        DBUGUS(("internalUnlockSemaphore waking up '%s'\n",swn->swn_Task->t.n_Name));

        /* wake up the primary locker */
        s->sem_Locker  = swn->swn_Task;
        s->sem_NestCnt = 1;

        oldints = Disable();
        NewReadyTask(swn->swn_Task);
        Enable(oldints);

        if (swn->swn_Shared)
        {
            /* we have a shared primary locker */
            s->s.n_Flags |= SEMAPHORE_SHAREDREAD;

            /* if there are any shared clients at the head of the wait q,
             * they must be woken up too.
             */
            while (!IsListEmpty(&s->sem_TaskWaitingList))
            {
                swn = (SemaphoreWaitNode *)FirstNode(&s->sem_TaskWaitingList);
                if (!swn->swn_Shared)
                {
                    /* stop scanning with the first exclusive client found */
                    break;
                }

                REMOVENODE((Node *)swn);

                sl = GetSharedLocker(s,swn->swn_Task);
                if (!sl)
                {
                    swn->swn_Task->t_WaitItem = NOMEM;
                }
                else
                {
                    sl->sl_NestCnt++;
                }

                oldints = Disable();
                NewReadyTask(swn->swn_Task);
                Enable(oldints);
            }
        }
        else
        {
            /* we have an exclusive primary locker */
            s->s.n_Flags &= ~(SEMAPHORE_SHAREDREAD);
        }
    }
    else
    {
        /* semaphore is unlocked */
        s->s.n_Flags  &= ~(SEMAPHORE_SHAREDREAD);
        s->sem_Locker  = NULL;
    }

    DBUGUS(("internalUnlockSemaphore (e): returning with 0\n"));

    LogSemaphoreUnlocked(s);

    return 0;
}

int32 externalUnlockSemaphore(Item is)
{
Semaphore *s;

    s = (Semaphore *)CheckItem(is,KERNELNODE,SEMA4NODE);
    if (!s)
    {
        /* bad semaphore item */
        return BADITEM;
    }

    return internalUnlockSemaphore(s);
}


/**
|||	AUTODOC -class Kernel -group Semaphores -name DeleteSemaphore
|||	Deletes a semaphore.
|||
|||	  Synopsis
|||
|||	    Err DeleteSemaphore( Item s )
|||
|||	  Description
|||
|||	    This macro deletes a semaphore that was created with
|||	    CreateSemaphore().
|||
|||	  Arguments
|||
|||	    s
|||	        The item number of the semaphore to be deleted.
|||
|||	  Return Value
|||
|||	    Returns >= 0 if successful or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Macro implemented in <kernel/semaphore.h> V20.
|||
|||	  Associated Files
|||
|||	    <kernel/semaphore.h>, libc.a
|||
|||	  See Also
|||
|||	    CreateSemaphore()
|||
**/

int32 internalDeleteSemaphore(Semaphore *s, Task *t)
{
SemaphoreWaitNode *swn;
SharedLocker      *sl;
uint32             oldints;

    TOUCH(t);

    DBUG(("DeleteSemaphore(%d) s=%lx\n",s->s.n_Item,(uint32)s));

    /* Notify all waiters that the semaphore went *poof* */

    while (TRUE)
    {
        swn = (SemaphoreWaitNode *)RemHead(&s->sem_TaskWaitingList);
        if (!swn)
            break;

        oldints = Disable();
        swn->swn_Task->t_WaitItem = BADITEM;
        NewReadyTask(swn->swn_Task);
        Enable(oldints);
    }
    REMOVENODE((Node *)s);

    /* drain the shared locker queue, freeing all associated resources */
    while (TRUE)
    {
        sl = (SharedLocker *)RemHead(&s->sem_SharedLockers);
        if (!sl)
            break;

        SuperFreeMem(sl,sizeof(SharedLocker));
    }

    return 0;
}


Err
internalSetSemaphoreOwner(Semaphore *s, Item newOwner)
{
    TOUCH(newOwner);

    if (s->sem_Locker == NULL)
    {
        /* Only allow semaphores to be transferred ownership when they are
         * not locked by anyone.
         *
         * Note that we could also transfer the ownership if the semaphore
         * is locked by the current thread... Maybe later.
         */
        return 0;
    }

    return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_CantSetOwner);
}
