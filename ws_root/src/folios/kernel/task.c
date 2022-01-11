/* @(#) task.c 96/10/24 1.259 */

#include <kernel/types.h>
#include <kernel/nodes.h>
#include <kernel/kernelnodes.h>
#include <kernel/list.h>
#include <kernel/listmacros.h>
#include <kernel/task.h>
#include <kernel/folio.h>
#include <kernel/kernel.h>
#include <kernel/mem.h>
#include <kernel/semaphore.h>
#include <kernel/io.h>
#include <kernel/interrupts.h>
#include <kernel/timer.h>
#include <kernel/operror.h>
#include <kernel/debug.h>
#include <kernel/lumberjack.h>
#include <kernel/internalf.h>
#include <kernel/bitarray.h>
#include <kernel/super.h>
#include <loader/loader3do.h>
#include <kernel/monitor.h>
#include <dipir/rsa.h>
#include <hardware/PPC.h>
#include <hardware/PPCasm.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>


/*****************************************************************************/


#ifdef BUILD_STRINGS
#define INFO(x)	printf x
#else
#define INFO(x)
#endif

#define DBUG(x)     /*printf x*/
#define DBUGCT(x)   /*printf x*/
#define DBUGCL(x)   /*printf x*/
#define DBUGKILL(x) /*printf x*/
#define DBUGKBRIEF(x) /*printf x*/
#define DBUGFM(x)   /*printf x*/
#define DBUGFR(x)   /*printf x*/

#define MIN_STACK        8
#define SUPER_STACK_SIZE 2048
#define STACK_ALIGNMENT  8      /* address and size must be multiples of this */

extern void FinishInitMem(void);
extern void InitMemDebug(void);


/*****************************************************************************/


Item internalFindTask(char *name)
{

    DBUG(("FindTask(%lx):%s\n",name,name));
    DBUG(("ct=%lx name=%s\n",CURRENTTASK,CURRENTTASK->t.n_Name));

    if (name == NULL)
	return CURRENTTASKITEM;

  {
    Node *n;
    Task *t;
    Item  it;
    ScanList(&KB_FIELD(kb_Tasks),n,Node)
    {
        t = Task_Addr(n);

        /* Note: Both t->t.n_Name and name are non-NULL */
        if (strcasecmp(name, t->t.n_Name) == 0)
            return t->t.n_Item;
    }

    it = MAKEKERR(ER_SEVER,ER_C_STND,ER_NotFound);
    return it;
  }
}


/*****************************************************************************/


/* Unlock any semaphores the given task has locked */
static void UnlockSemaphores(Task *t)
{
Semaphore    *s;
SharedLocker *sl;
uint32        count;

    DBUGKILL(("Unlocking semaphores for %lx\n",(uint32)t));
    ScanList(&KB_FIELD(kb_Semaphores),s,Semaphore)
    {
        if (s->sem_Locker == t)
        {
            /* if the task is a primary locker... */
            while (s->sem_Locker == t)
            {
                internalUnlockSemaphore(s);
            }
        }
        else
        {
            /* the task might be a secondary locker */
            ScanList(&s->sem_SharedLockers,sl,SharedLocker)
            {
                if (sl->sl_Task == t)
                {
                    count = sl->sl_NestCnt;
                    while (count--)
                        internalUnlockSemaphore(s);

                    REMOVENODE((Node *)sl);
                    SuperFreeMem(sl,sizeof(SharedLocker));
                    break;
                }
            }
        }
    }
}


/*****************************************************************************/


static void FreePages(Task *t)
{
MemRegion *mr;
PagePool  *pp;
uint32     size;
uint32     firstBit, lastBit;
uint32     numBits;
uint32    *bits;

    /* transfer all pages owned by this task back to the system */
    pp = t->t_PagePool;
    if (pp)
    {
        mr       = pp->pp_MemRegion;
        bits     = pp->pp_OwnedPages;
        numBits  = mr->mr_NumPages;
        firstBit = 0;

        while (firstBit < numBits)
        {
            lastBit = firstBit;
            while ((lastBit < numBits) && IsBitSet(bits,lastBit))
                lastBit++;

            if (lastBit != firstBit)
            {
                externalControlMem((void *)((uint32)mr->mr_MemBase + (firstBit << mr->mr_PageShift)),
                                   (lastBit - firstBit) << mr->mr_PageShift,
                                   MEMC_GIVE,0);
            }
            firstBit = lastBit + 1;
        }

        size = (mr->mr_NumPages+7) / 8;
        SuperFreeMem(pp->pp_OwnedPages, size);
        SuperFreeMem(pp->pp_WritablePages, size);
        SuperFreeMem(pp,sizeof(PagePool));
        t->t_PagePool = NULL;
    }
}


/*****************************************************************************/


static void FreeTask(Task *t)
{
int32  i;
void  *tfd;

    DBUGKILL(("FreeTask: enter thread=%lx\n",(uint32)t->t_ThreadTask));
    if (t->t_FolioData)
    {
        for (i = 0; i < KB_FIELD(kb_FolioTaskDataCnt); i++)
        {
            tfd = t->t_FolioData[i];
            if (tfd)
            {
                struct Folio **DataFolios = KB_FIELD(kb_DataFolios);
                void (*fd)(Task *) = DataFolios[i]->f_FolioDeleteTask;
                if (fd)(*fd)(t);
            }
        }
        SuperFreeMem(t->t_FolioData,KB_FIELD(kb_FolioTaskDataSize));
    }

#ifdef BUILD_MEMDEBUG
    DBUGKILL(("Deleting memdebug nodes\n"));
    DeleteTaskMemDebug(t);
#endif

    if (t->t_ThreadTask == 0)
    {
        DBUGKILL(("non thread task\n"));
        FreePages(t);
    }

    /* Don't free the stack we are presently using! */
    if ((t != CURRENTTASK) && CURRENTTASK)
    {
        DBUGKILL(("Freeing super stack at %lx\n",(uint32)t->t_SuperStackBase));
        SuperFreeMem(t->t_SuperStackBase,t->t_SuperStackSize);
    }

    DBUGFM(("Freeing ResourceTable(%lx size=%d)\n",(uint32)t->t_ResourceTable,(int)(t->t_ResourceCnt * sizeof(Item))));
    SuperFreeMem(t->t_ResourceTable,t->t_ResourceCnt * sizeof(Item));

    if (t->t_UserExceptionMsg >= 0)
    {
        DBUGKILL(("Freeing user exception message\n"));
        internalDeleteMsg(MESSAGE(t->t_UserExceptionMsg), t);
    }

    DBUGKILL(("FreeString\n"));
    FreeString(t->t.n_Name);
    t->t.n_Name = NULL;     /* so we won't free it twice */

    DBUGKILL(("Return from FreeTask\n"));
}


/*****************************************************************************/


static void FreeResources(Task *t, int32 cntype, bool opensOnly)
{
int32     i;
int32     resCnt;
Item      it;
ItemNode *n;


    DBUGFR(("FreeResources(%lx)\n",(uint32)t));

    do
    {
        resCnt = t->t_ResourceCnt;
        for (i = resCnt - 1; i >= 0; i--)
        {
            it = t->t_ResourceTable[i];
            n  = LookupItem(it & ~ITEM_WAS_OPENED);
            if (n)
            {
                if ((cntype == 0)
                 || ((n->n_Type == TypePart(cntype))
                  && (n->n_SubsysType == SubsysPart(cntype))))
                {
                    if (it & ITEM_WAS_OPENED)
                    {
                        it &= ~ITEM_WAS_OPENED;
                        DBUGFR(("closing item %lx, type %d, subsystype %d, name %s\n",it,n->n_Type,
                                                                                n->n_SubsysType,
                                                                                n->n_Name));
                        internalCloseItemSlot(it,t,i);
                    }
                    else if (it != CURRENTTASKITEM)
                    {
                        if (!opensOnly)
                        {
                            DBUGFR(("deleting item %lx, type %d, subsystype %d, name %s",it,n->n_Type,
                                                                                     n->n_SubsysType,
                                                                                     n->n_Name));

                            if ((n->n_SubsysType == KERNELNODE)
                             && (n->n_Type == IOREQNODE))
                            {
                                DBUGFR((", device %s",IOREQ(it)->io_Dev->dev.n_Name));
                            }
                            DBUGFR(("\n"));

                            internalDeleteItem(it);
                        }
                    }
                }
            }
        }
    }
    while (resCnt != t->t_ResourceCnt);

    DBUGFR(("Exiting FreeResources\n"));
}


/*****************************************************************************/


/* Kill a task.
 *
 * WARNING: Must be called with interrupts disabled
 */
void Murder(Task *t, Task *killer)
{
    t->t_Flags     |= TASK_EXITING;
    t->t_ExitStatus = KILLED;

    if (t->t_Flags & TASK_SUSPENDED)
    {
        /* restore it to a state where it can kill itself */
        ResumeTask(t);
    }

    if (t->t_RegisterSave.rb_MSR & MSR_PR)
    {
        /* Task is in user mode. Force the task to kill itself
         * as soon as it regains the CPU.
         */
        t->t_RegisterSave.rb_PC = (uint32)KillSelf;
    }
    else if (t->t_Flags & TASK_SINGLE_STACK)
    {
        /* When killing a single stack task that's running in supervisor mode,
         * we can't force it to exit. In the case of a supervisor-mode only
         * task, the task will never return to user-mode, so we can't hack
         * that return address. In the case of non-supervisor-mode only tasks,
         * we just don't know where the stack frame containing the return
         * address is stored.
         *
         * We therefore leave these tasks to fend for themselves. They are
         * responsible for detecting that they are being aborted and
         * should exit gracefully.
         */
        DBUGKILL(("WARNING: Killing task '%s' which is of type TASK_SINGLE_STACK\n", t->t.n_Name));
    }
    else
    {
    SuperStackFrame *ssf;

        /* Task is in supervisor mode. Force the task to
         * kill itself as soon as it returns to user-mode.
         */
        ssf = (SuperStackFrame *)t->t_ssp;
        ssf->ssf_LR = (uint32)KillSelf;
    }

    /* If the task was in single-step mode when it got killed, make it a
     * run in normal mode once again.
     */
    t->t_RegisterSave.rb_MSR &= ~(MSR_SE | MSR_BE);

    /* leave some fingerprints */
    t->t_Killer = killer;

    /* clean up any loose ends for sema4 waiters */
    if (t->t_WaitItem > 0)
    {
        if (t->t.n_Flags & TASK_WAITING)
        {
        ItemNode          *it;
        SemaphoreWaitNode *swn;

            it = LookupItem(t->t_WaitItem);
            if (it)
            {
                if (it->n_Type == SEMA4NODE)
                {
                    ScanList(&((Semaphore *)it)->sem_TaskWaitingList, swn, SemaphoreWaitNode)
                    {
                        if (swn->swn_Task == t)
                        {
                            REMOVENODE((Node *)swn);
                            break;
                        }
                    }
                }
            }
        }
    }

    /* This will push the task to suicide... */
    SetSignals(t, SIGF_ABORT);
    t->t_WaitItem   = ABORTED;
    t->t.n_Priority = 200;
}


/*****************************************************************************/


int32 internalKill(Task *t, Task *dummy)
{
uint32 oldints;

	TOUCH(dummy);

	DBUGKBRIEF(("Kill: t=%lx (%s), CURRENTTASK=%lx (%s)\n",t,t->t.n_Name,CURRENTTASK,CURRENTTASK->t.n_Name));
	DBUGKILL(("Kill: t=%lx (%s), CURRENTTASK=%lx (%s)\n",t,t->t.n_Name,CURRENTTASK,CURRENTTASK->t.n_Name));

	t->t_Flags |= TASK_EXITING;

        if (t != CURRENTTASK)
        {
            /* reset this bit so that KillSelf() will work */
            t->t.n_ItemFlags &= ~(ITEMNODE_DELETED);

	    oldints = Disable();

	    Murder(t, CURRENTTASK);

            if (t->t.n_Flags & TASK_READY)
            {
                REMOVENODE((Node *)t);
                t->t.n_Flags |= TASK_WAITING;  /* so NewReadyTask() will do something */
            }
            NewReadyTask(t);

            CURRENTTASK->t_WaitItem = t->t.n_Item;
            SleepTask(CURRENTTASK);

	    Enable(oldints);

	    return 1;
        }

	/* Commit suicide */

	DBUGKILL(("Freeing task items\n"));
	FreeResources(t, MKNODEID(KERNELNODE, TASKNODE), FALSE);

	DBUGKILL(("Freeing ioreq items\n"));
	FreeResources(t, MKNODEID(KERNELNODE, IOREQNODE), FALSE);

	DBUGKILL(("Closing device items\n"));
	FreeResources(t, MKNODEID(KERNELNODE, DEVICENODE), TRUE);

	DBUGKILL(("Freeing device items\n"));
	FreeResources(t, MKNODEID(KERNELNODE, DEVICENODE), FALSE);

	DBUGKILL(("Closing remaining items\n"));
	FreeResources(t, 0, TRUE);

	DBUGKILL(("Freeing remaining items\n"));
	FreeResources(t, 0, FALSE);

	DBUGKILL(("Now unlock semaphores\n"));
	UnlockSemaphores(t);
	DBUGKILL(("Now reply held messages\n"));
	ReplyHeldMessages(t);
	DBUGKILL(("CurrentTask=%lx\n",(uint32)CURRENTTASK));

	if ((t->t_Flags & TASK_FREE_STACK)
        &&  (t->t_Flags & TASK_SINGLE_STACK) == 0)
	{
            SuperFreeUserMem(t->t_StackBase, t->t_StackSize, t);
            t->t_StackBase = NULL;
            t->t_StackSize = 0;
	}

	if (t->t_Killer)
	{
	    oldints = Disable();
	    NewReadyTask(t->t_Killer);
	    Enable(oldints);
	}
	else
	{
	    /* suicide I tell you! */
	    t->t_Killer = t;
        }

	if (t->t_ExitMessage >= 0)
	{
	    /* Send a message to the owner task about the child dying */
	    Msg *msg = (Msg *)CheckItem(t->t_ExitMessage,KERNELNODE,MESSAGENODE);
	    if (msg)
		internalReplyMsg(msg, t->t_ExitStatus, (void *)t->t.n_Item, t->t_Killer->t.n_Item);
	}
	else
	{
	    /* send a signal to the owner that its child is dying */
	    internalSignal((Task *)LookupItem(t->t.n_Owner),SIGF_DEADTASK);
	}

	/* Remove the task from the list of tasks in the system */
	REMOVENODE((Node *)&(t->t_TasksLinkNode));

	LogTaskDied(t);

#ifdef BUILD_DEBUGGER
#ifdef BUILD_MACDEBUGGER
	if (t->t_ThreadTask == NULL)
#endif
	{
	    /* notify debugger that task is going to die */
	    oldints = Disable();
	    Dbgr_TaskDeleted(t);
	    Enable(oldints);
	}
#endif

	/* Save context and tidy up */
	DBUGKILL(("Kill: Suicide t=%lx\n",(uint32)t));
	FreeTask(t);

	/* Add it to the dead task list so its TCB and super stack will get
	 * freed by the next call to CreateItem().
	 */
	ADDTAIL(&KB_FIELD(kb_DeadTasks),(Node *)t);

	if (KB_FIELD(kb_FPOwner) == t)
	    KB_FIELD(kb_FPOwner) = NULL;

        if (KB_FIELD(kb_CurrentFenceTask) == t)
            KB_FIELD(kb_CurrentFenceTask) = NULL;

        if (KB_FIELD(kb_CurrentSlaveTask) == t->t.n_Item)
            KB_FIELD(kb_CurrentSlaveTask) = -1;

        LogItemDeleted((ItemNode *)t);
	RemoveItem((Task *)LookupItem(t->t.n_Owner),t->t.n_Item);
        FreeItem(t->t.n_Item);
	ScheduleNewTask();

        /* we never come back here */
}


/*****************************************************************************/


int32 internalChangeTaskPri(Task *t, uint8 newpri)
{
uint8  oldpri;
uint32 oldints;

    if ((t->t.n_ItemFlags & ITEMNODE_PRIVILEGED) == 0) /* no checks on privileged tasks */
    {
	if ((newpri > 199) || (newpri < 10))
	    return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_BadPriority);
    }

    oldpri  = t->t.n_Priority;
    oldints = Disable();

    LogTaskPriority(t, newpri);

    if (t == CURRENTTASK)
    {
        t->t.n_Priority = newpri;
        if (newpri < oldpri)
        {
            /* if the new priority is lower than the old, it's possible some
             * task on the ready queue should now get the CPU. To make life
             * simple, we just yank the first task from the ready q
             */
            t = (Task *)RemHead(&KB_FIELD(kb_TaskReadyQ));
            if (t)
            {
                t->t.n_Flags |= TASK_WAITING;  /* so NewReadyTask() will do something */
                NewReadyTask(t);
            }
        }
    }
    else if (t->t.n_Flags & TASK_WAITING)
    {
        /* in wait state, just update priority */
        t->t.n_Priority = newpri;
    }
    else
    {
        /* on the ready q */
        REMOVENODE((Node *)t);
        t->t.n_Priority = newpri;
        t->t.n_Flags   |= TASK_WAITING;   /* so NewReadyTask() will do something */
        NewReadyTask(t);
    }

    Enable(oldints);

    return oldpri;
}


/*****************************************************************************/


Err externalSetExitStatus(int32 status)
{
    CURRENTTASK->t_ExitStatus = status;
    return 0;
}


/*****************************************************************************/


/* Build an argv array.
 * Return argc, the number of arguments in the array.
 * If argv == NULL, don't really build the array; just return argc.
 */
static uint32 BuildArgv(char *cmdline, char **argv)
{
uint32 argc;
bool   terminator;

    argc = 0;

    for (;;)
    {
        /* Skip leading spaces */
        while (*cmdline == ' ')
            cmdline++;

        if (*cmdline == '\0')
        {
            if (argv != NULL)
                argv[argc] = NULL;

            /* End of string; we're done */
            break;
        }

        if (*cmdline == '"')
        {
            cmdline++;
            terminator = '"';
        }
        else
        {
            terminator = ' ';
        }

        /* Store pointer to the argument */
        if (argv != NULL)
            argv[argc] = cmdline;

        argc++;

        /* Skip to the end of the argument */
        while ((*cmdline != terminator) && (*cmdline != '\0'))
            cmdline++;

        if (*cmdline == terminator && argv != NULL)
        {
            /* Null-terminate the argument */
            *cmdline++ = '\0';
        }
    }

    return argc;
}


/*****************************************************************************/


/* Size of a buffer needed to hold a string (including the null terminator),
 * and rounded up to a word boundary.
 */
#define	ROUNDED_SIZE(s)	((strlen(s)+1 + (STACK_ALIGNMENT-1)) & (~(STACK_ALIGNMENT-1)))

/* Given a command line, return the # of bytes needed to hold
 * the command line and all of the argv vectors.
 */
static uint32 SizeArguments(const char *cmdline)
{
uint32 argc;
uint32 size;

    argc = BuildArgv(cmdline, NULL);
    size = ROUNDED_SIZE(cmdline) + ((argc+1) * sizeof(char*));
    return (size + STACK_ALIGNMENT - 1) & (~(STACK_ALIGNMENT - 1));
}


/*****************************************************************************/


/* Given a command line, copy the arguments into the supplied buffer,
 * and construct an argv array for it.
 * buf is assumed to be word aligned.
*/
static uint32 BuildArguments(const char *cmdline, char *buf, char ***p_argv)
{
    /* Copy the command string */
    strcpy(buf, cmdline);

    /* Build the argv array after the command string */
    *p_argv = (char **)(buf + ROUNDED_SIZE(buf));

    return BuildArgv(buf, *p_argv);
}


/*****************************************************************************/


typedef struct
{
    Module *ti_Module;
    char   *ti_CmdLine;
    uint32  ti_CmdLineLen;
    Item    ti_ExitMsgPort;
    Item    ti_ExceptionMsgPort;
    bool    ti_DefaultMsgPort;
    bool    ti_Thread;
} TaskInfo;


static int32 ict_c(Task *t, TaskInfo *ti, uint32 tag, uint32 arg)
{
    DBUGCT(("ict_c task=%lx tag=%lx arg=%lx\n",t,tag,arg));
    switch (tag)
    {
	case CREATETASK_TAG_PC             : t->t_RegisterSave.rb_PC = (uint32)arg;
                                             break;

	case CREATETASK_TAG_MODULE         : ti->ti_Module = (Module *)CheckItem((Item)arg, KERNELNODE, MODULENODE);
                                             break;

	case CREATETASK_TAG_STACKSIZE      : t->t_StackSize = (int32)arg;
                                             break;

	case CREATETASK_TAG_SP             : t->t_RegisterSave.rb_SP = (uint32) arg;
				             break;

	case CREATETASK_TAG_ARGC           : t->t_RegisterSave.rb_GPRs[3] = arg;
                                             break;

	case CREATETASK_TAG_ARGP           : t->t_RegisterSave.rb_GPRs[4] = arg;
                                             break;

	case CREATETASK_TAG_MAXQ           : t->t_MaxUSecs = arg;
				             break;

	case CREATETASK_TAG_FREESTACK      : t->t_Flags |= TASK_FREE_STACK;
                                             break;

	case CREATETASK_TAG_MSGFROMCHILD   : ti->ti_ExitMsgPort = (Item)arg;
                                             break;

	case CREATETASK_TAG_CMDSTR         : ti->ti_CmdLine    = (char *)arg;
	                                     ti->ti_CmdLineLen = strlen(ti->ti_CmdLine);
	                                     /* Even though we don't use the length
	                                      * value, we want to do a strlen() in order
	                                      * to validate the pointer.
	                                      *
	                                      * If the pointer is bogus, strlen() will cause
	                                      * an exception to occur, and we'll land back
	                                      * in TagProcessor()
	                                      */
	                                     break;

	case CREATETASK_TAG_SUPERVISOR_MODE: t->t_RegisterSave.rb_MSR &= ~(MSR_PR);
	                                     t->t_Forbid       = 1;
	                                     t->t_Flags       |= TASK_SUPERVISOR_ONLY;
	case CREATETASK_TAG_SINGLE_STACK   : t->t_Flags       |= TASK_SINGLE_STACK;
        case CREATETASK_TAG_PRIVILEGED     : t->t.n_ItemFlags |= ITEMNODE_PRIVILEGED;
                                             break;

	case CREATETASK_TAG_USERDATA       : t->t_UserData = (void *)arg;
                                             break;

	case CREATETASK_TAG_USEREXCHANDLER : ti->ti_ExceptionMsgPort = (Item)arg;
                                             break;

        case CREATETASK_TAG_DEFAULTMSGPORT : ti->ti_DefaultMsgPort = TRUE;
                                             break;

        case CREATETASK_TAG_THREAD         : ti->ti_Thread = TRUE;
                                             break;

	default                            : DBUGCT(("parsing tagargs for ct: tag=%lx\n",tag));
                                             if (tag < 0x10000)
                                             {
                                                 DBUGCT(("bad tag arg:%d\n",(int32)tag));
                                                 return BADTAG;
                                             }
                                             /* ignore the tag */
                                             break;
    }

    return 0;
}


/**
|||	AUTODOC -public -class items -name Task
|||	An executable context.
|||
|||	  Description
|||
|||	    A task item contains all the information needed by the kernel to
|||	    support multitasking. It contains room to store CPU registers when
|||	    the associated task context goes to sleep, it contains pointers to
|||	    various resources used by the task, and it specifies the task's
|||	    priority. There is one task item for every task or thread that
|||	    exists.
|||
|||	  Folio
|||
|||	    kernel
|||
|||	  Item Type
|||
|||	    TASKNODE
|||
|||	  Create Call
|||
|||	    CreateThread(), CreateTask()
|||
|||	  Delete
|||
|||	    DeleteThread(), DeleteItem()
|||
|||	  Query
|||
|||	    FindTask()
|||
|||	  Modify
|||
|||	    SetItemOwner(), SetItemPri()
|||
|||	  Tags
|||
|||	    TAG_ITEM_NAME (const char *) - Create
|||	        Lets you specify the name of the task. This is a required
|||	        tag.
|||
|||	    TAG_ITEM_PRI (uint8) - Create
|||	        Provide a priority for the task in the range 10 to 199. If this
|||	        tag is not given, the task gets it either from the supplied
|||	        module item, or inherits it from the creator.
|||
|||	    CREATETASK_TAG_MODULE (Item) - Create
|||	        This tag specifies the module item where the code and data
|||	        for this task can be found.
|||
|||	    CREATETASK_TAG_PC (void *) - Create
|||	        Provide a pointer to the code to be executed.
|||
|||	    CREATETASK_TAG_SP (void *) - Create
|||	        Provide a pointer to the memory buffer to use as stack for a
|||	        thread. This tag is only valid when starting a thread.
|||	        This pointer must be aligned on an 8 byte boundary, and
|||	        points to one byte past the end of the allocated stack area.
|||
|||	    CREATETASK_TAG_STACKSIZE (uint32) - Create
|||	        Specifies the size in bytes of the memory buffer reserved for a
|||	        thread's stack. This must be a multiple of 8 bytes.
|||
|||	    CREATETASK_TAG_ARGC (uint32) - Create
|||	        A 32-bit value that will be passed to the task or thread being
|||	        launched as its first argument. If this is omitted, the
|||	        first argument will be 0.
|||
|||	    CREATETASK_TAG_ARGP (void *) - Create
|||	        A 32-bit value that will be passed to the task or thread being
|||	        launched as a second argument. If this is omitted, the second
|||	        argument will be 0.
|||
|||	    CREATETASK_TAG_MSGFROMCHILD (Item) - Create
|||	        Provides the item number of a message port.  The kernel will
|||	        send a status message to this port whenever the thread or
|||	        task being created exits. The message is sent by the kernel
|||	        after the task has been deleted. The msg_Result field of the
|||	        message contains the exit status of the task. This
|||	        is the value the task provided to exit(), or the value returned
|||	        by the task's main() function. The msg_DataPtr field of the
|||	        message contains the item number of the task that just
|||	        terminated. Finally, the msg_DataSize field contains the item
|||	        number of the thread or task that terminated the task. If the
|||	        task exited on its own, this will be the item number of the
|||	        task itself. It is the responsibility of the task that
|||	        receives the status message to delete it when it is no longer
|||	        needed by using DeleteMsg().
|||
|||	    CREATETASK_TAG_FREESTACK (void) - Create
|||	        When this tag is supplied, it tells the kernel to automatically
|||	        free the memory used for the stack when the thread dies. If
|||	        the tag is not provided, the creator is responsible to free the
|||	        stack. When using this tag, the memory cannot have been
|||	        allocated with MEMTYPE_TRACKSIZE.
|||
|||	    CREATETASK_TAG_MAXQ (uint32) - Create
|||	        A value indicating the maximum quanta for the task in
|||	        microseconds.
|||
|||	    CREATETASK_TAG_USERDATA (void *) - Create
|||	        This specifies an arbitrary 32-bit value that is put in the
|||	        new task's t_UserData field. This is a convenient way to pass
|||	        a pointer to a shared data structure when starting a thread.
|||
|||	    CREATETASK_TAG_USEREXCHANDLER (Item) - Create
|||	        Provides the item number of a message port. The kernel will
|||	        send a message to this port whenever the thread or
|||	        task being created triggers an exception. See the
|||	        RegisterUserExceptionHandler() function for more details.
|||
|||	    CREATETASK_TAG_DEFAULTMSGPORT (void) - Create
|||	        When this tag is present, the kernel automatically creates a
|||	        message port for the new task or thread being started. The
|||	        item number of this port is stored in the Task structure's
|||	        t_DefaultMsgPort field. This is a convenient way to quickly
|||	        establish a communication channel between a parent and a child.
|||
|||	    CREATETASK_TAG_THREAD (void)
|||	        This tag specifies that you wish to start a thread instead of
|||	        a full-fledged task.
|||
**/

Item internalCreateTask(Task *t, TagArg *tagpt)
{
Task       *ct;
uint32      oldints;
bool        isthread;
LoaderInfo *li;
Item        ret;
TaskInfo    tinfo;
PagePool   *pp;
void       *sp;

    DBUGCT(("CT: entering, current task '%s'\n",CURRENTTASK ? CURRENTTASK->t.n_Name : "None"));

    ct = CURRENTTASK;

    /* init defaults */
    t->t.n_Flags                  |= TASK_WAITING;
    t->t_AllocatedSigs             = 0x000000ff;
    t->t_FreeResourceTableSlot     = -1;
    t->t_MaxUSecs                  = DEFAULT_QUANTA_USECS;
    t->t_StackSize                 = 0;
    t->t_DefaultMsgPort            = -1;
    t->t_ExitMessage               = -1;
    t->t_WaitItem                  = -1;
    t->t_RegisterSave.rb_MSR       = MSR_EE | MSR_PR | MSR_ME | MSR_IR | MSR_DR;
    t->t_RegisterSave.rb_LR        = (uint32)exit;
    t->t_FPRegisterSave.fprb_SP    = 0xffffffff;
    t->t_FPRegisterSave.fprb_LT    = 0x00000000;
    t->t_FPRegisterSave.fprb_FPSCR = FPSCR_NI;
    t->t_UserExceptionMsg          = -1;

#ifdef BUILD_DEBUGGER
    /* in development mode, trap FP exceptions by default */
    t->t_RegisterSave.rb_MSR       |= (MSR_FE0 | MSR_FE1);
    t->t_FPRegisterSave.fprb_FPSCR |= (FPSCR_VE | FPSCR_OE | FPSCR_ZE);
    t->t_CapturedExceptions         = (USEREXC_FP_INVALID_OP | USEREXC_FP_OVERFLOW | USEREXC_FP_ZERODIVIDE);
#endif

    t->t_RegisterSave.rb_MSR     |= (MSR_IP & _mfmsr());

    if (ct)
    {
        /* Inherit some things from creator */
        t->t.n_Priority = ct->t.n_Priority;
    }

    PrepList(&t->t_MessagesHeld);

    DBUGCT(("CT: Calling TagProcessor()\n"));

    memset(&tinfo, 0, sizeof(tinfo));
    tinfo.ti_ExitMsgPort = -1;
    tinfo.ti_ExceptionMsgPort = -1;

    ret = TagProcessor(t, tagpt, ict_c, &tinfo);

    DBUGCT(("CT: TagProcessor() returned %d\n",ret));

    if (ret < 0)
        return ret;

    if (t->t.n_Name == NULL)
    {
        /* Use the module name if available */
        if (tinfo.ti_Module)
        {
            if (tinfo.ti_Module->n.n_Name)
                t->t.n_Name = AllocateString(tinfo.ti_Module->n.n_Name);
        }

	if (t->t.n_Name == NULL)
	{
	    /* tasks must have names */
	    ret = BADNAME;
	    goto abort;
	}
    }

    if ((tinfo.ti_Module && t->t_RegisterSave.rb_PC)
    ||  (!tinfo.ti_Module && !t->t_RegisterSave.rb_PC))
    {
        /* must have one and only one of PC and Module */
        ret = BADTAGVAL;
        goto abort;
    }

    if (t->t_RegisterSave.rb_PC && !IsMemReadable((void *)t->t_RegisterSave.rb_PC,4))
    {
        /* supplied PC is not in RAM */
        DBUGCT(("Bad PC\n"));
	ret = BADPTR;
	goto abort;
    }

    /* find the loader info record */
    li = tinfo.ti_Module ? tinfo.ti_Module->li : NULL;

    isthread = tinfo.ti_Thread;

    if (isthread)
    {
        /* You can't currently have single-stack and free-stack
         * at the same time, as it causes a brain hemorage at
         * termination time.
         */
        if ((t->t_Flags & TASK_SINGLE_STACK) && (t->t_Flags & TASK_FREE_STACK))
        {
            t->t_Flags &= ~(TASK_SINGLE_STACK);
        }

        /* was a stack supplied by the creator? */
        if (t->t_RegisterSave.rb_SP)
        {
            if (!IsMemWritable((void *)(t->t_RegisterSave.rb_SP - t->t_StackSize),t->t_StackSize))
            {
                /* is caller trusted? */
                if (!IsPriv(ct))
                {
                    /* supplied stack is not fully writable, or not suitably aligned */
                    ret = BADPTR;
                    goto abort;
                }
            }

            t->t_StackBase = (uint32 *)(t->t_RegisterSave.rb_SP - t->t_StackSize);

            /* guarantee EABI alignment */
            t->t_RegisterSave.rb_SP &= ~(STACK_ALIGNMENT-1);
        }
        else
        {
            /* no stack ptr was given, should a stack be automatically allocated? */

            if (t->t_StackSize == 0)
            {
                /* see if we can determine a stack size from a bin header */
                if (li)
                {
                _3DOBinHeader *_3do = li->header3DO;

                    t->t_StackSize = _3do->_3DO_Stack;
                }
            }

            if (t->t_StackSize)
            {
                /* client wants us to auto-allocate the stack */
                if (t->t_Flags & TASK_SUPERVISOR_ONLY)
                {
                    /* allocate supervisor-memory since this guy only runs in super mode */
                    sp = SuperAllocMem(t->t_StackSize, MEMTYPE_NORMAL);
                }
                else
                {
                    /* allocate memory from the current task */
                    sp = SuperAllocUserMem(t->t_StackSize, MEMTYPE_NORMAL, CURRENTTASK);
                }

                if (sp == NULL)
                {
                    ret = NOMEM;
                    goto abort;
                }

                t->t_Flags             |= TASK_FREE_STACK;
                t->t_RegisterSave.rb_SP = (uint32)sp + t->t_StackSize;
                t->t_StackBase          = sp;

                /* guarantee EABI alignment */
                t->t_RegisterSave.rb_SP &= ~(STACK_ALIGNMENT-1);
            }
            else if (t->t_Flags & TASK_SINGLE_STACK)
            {
                /* can't specify single-stack, and not have a stack at all! */
                ret = BADTAGVAL;
                goto abort;
            }
        }

        /* Set t_ThreadTask to the current Task, or to
         * its t_ThreadTask if it is also a thread.
         */
        t->t_ThreadTask  = ct->t_ThreadTask ? ct->t_ThreadTask : ct;
    }
    else
    {
        if (t->t_RegisterSave.rb_SP)
        {
            /* can't supply a stack for a task */
            ret = BADTAGVAL;
            goto abort;
        }

        if (t->t_Flags & TASK_FREE_STACK)
        {
            /* asking for auto stack free for tasks doesn't make sense */
            ret = BADTAGVAL;
            goto abort;
        }
    }

    if (t->t.n_ItemFlags & ITEMNODE_PRIVILEGED)
    {
        if (ct && ((ct->t.n_ItemFlags & ITEMNODE_PRIVILEGED) == 0))
        {
            /* trying to start a privileged child without being privilege */
            ret = BADPRIV;
            goto abort;
        }
    }

    DBUGCT(("CT: Launching a %s called %s\n", (isthread ? "thread" : "task"), t->t.n_Name));

    if ((t->t_Flags & TASK_SINGLE_STACK) == 0)
    {
        /* allocate supervisor stack */

        t->t_SuperStackSize = SUPER_STACK_SIZE;
        t->t_SuperStackBase = SuperAllocMemAligned(t->t_SuperStackSize, MEMTYPE_NORMAL, 8);
        if (t->t_SuperStackBase == NULL)
        {
            ret = NOMEM;
            goto abort;
        }

        t->t_ssp = (uint32 *)((uint32)t->t_SuperStackBase
                              + t->t_SuperStackSize
                              - sizeof(SuperStackFrame));
    }

    /* call the CreateTask entry point for each folio
     * to allocate per-folio data for the new task.
     */
    DBUGCT(("CT: allocate per task data for folios ds=%d\n", KB_FIELD(kb_FolioTaskDataSize)));

    if (KB_FIELD(kb_FolioTaskDataSize))
    {
    Folio *f;

        t->t_FolioData = SuperAllocMem(KB_FIELD(kb_FolioTaskDataSize), MEMTYPE_FILL);
        if (!t->t_FolioData)
        {
            ret = NOMEM;
            goto abort;
        }

        ScanList(&KB_FIELD(kb_FolioList), f, Folio)
        {
            DBUGCT(("CT: Folio=%lx\n",(ulong)f));
            if (f->f_FolioCreateTask)
            {
                DBUGCT(("CT: Calling FolioCreateTask folio=%s at:%lx\n", f->fn.n_Name,f->f_FolioCreateTask));
                ret = (*f->f_FolioCreateTask)(t,tagpt);
                if (ret < 0)
                    goto abort;
            }
        }
    }

    DBUGCT(("CT: Setting up page pool\n"));

    if (isthread)
    {
        t->t_PagePool = ct->t_PagePool;
    }
    else
    {
        /* starting a full-blown task */

        pp = SuperAllocMem(sizeof(PagePool), MEMTYPE_NORMAL);
        if (!pp)
        {
            ret = NOMEM;
            goto abort;
        }

        InitPagePool(pp);

        pp->pp_Owner         = t;
        pp->pp_OwnedPages    = SuperAllocMem((pp->pp_MemRegion->mr_NumPages+7) / 8, MEMTYPE_NORMAL | MEMTYPE_FILL);
        pp->pp_WritablePages = SuperAllocMem((pp->pp_MemRegion->mr_NumPages+7) / 8, MEMTYPE_NORMAL);
        t->t_PagePool        = pp;

        if (ret < 0)
            goto abort;

	/* We give ownership of the memlist semaphore to the task that it
	   belongs to - This makes sure that the semaphore dies with the
	   memory it is guarding

	   We can't transfer the semaphore for the operator, because
	   SetItemOwner assumes CURRENTTASK is nonnull.  This is not a problem
	   because the operator never dies.
	*/

	if (ct)
	{
	    ret = externalSetItemOwner(pp->pp_Lock, t->t.n_Item);
            if (ret < 0)
                goto abort;
	}

        if ((pp->pp_OwnedPages == NULL) || (pp->pp_WritablePages == NULL))
        {
            ret = NOMEM;
            goto abort;
        }

        /* initialize the writable pages to be the publically writable pages */
        memcpy(pp->pp_WritablePages, pp->pp_MemRegion->mr_PublicPages, (pp->pp_MemRegion->mr_NumPages + 7) / 8);

        DBUGCT(("CT: allocated task's page pool, pp $%x\n",t->t_PagePool));
    }

    if (li)
    {
        t->t_RegisterSave.rb_PC = (uint32) li->entryPoint;

        /* FIXME: extra bytes at the end of the new task's data pages that
         *        aren't used up by the DATA+BSS space should be added to the
         *        task's free list.
         */

        if (li->header3DO)
        {
            /* override passed in information with values from 3DOBinHeader */
            _3DOBinHeader *_3do = li->header3DO;

            /* can't override the stack size for a thread since the stack is
             * already allocated by now
             */
            if (!isthread)
            {
                if (_3do->_3DO_Stack)
                    t->t_StackSize = _3do->_3DO_Stack;
            }

            if (_3do->_3DO_MaxUSecs)
                t->t_MaxUSecs = _3do->_3DO_MaxUSecs;

            t->t.n_Version  = _3do->_3DO_Item.n_Version;
            t->t.n_Revision = _3do->_3DO_Item.n_Revision;

            /* The loader tells us this thing is privileged */
            if (tinfo.ti_Module->n.n_ItemFlags & ITEMNODE_PRIVILEGED)
                t->t.n_ItemFlags |= ITEMNODE_PRIVILEGED;

	    /* We allow executables which are signed and marked as needing
	       PCMCIA support to access PCMCIA devices	*/
	    if((_3do->_3DO_Flags & (_3DO_SIGNED | _3DO_PCMCIA_OK)) ==
	               (_3DO_SIGNED | _3DO_PCMCIA_OK))
		t->t_Flags |= TASK_PCMCIA_PERM;

            if (isthread)
            {
                /* when starting a thread, if the parent is privileged, then
                 * the child must be signed
                 */

                if (IsPriv(ct) && ((_3do->_3DO_Flags & _3DO_SIGNED) == 0))
                {
                    ret = BADPRIV;
                    goto abort;
                }
            }

            if (_3do->_3DO_Item.n_Priority)
                t->t.n_Priority = _3do->_3DO_Item.n_Priority;
        }
    }

    if (!isthread)
    {
        if ((t->t_StackSize < MIN_STACK) || (t->t_StackSize & (STACK_ALIGNMENT-1)))
        {
            /* for tasks, must have at least MIN_STACK bytes of stack, and the size
             * must be suitably aligned
             */
            ret = MAKEKERR(ER_SEVERE, ER_C_NSTND, ER_Kr_BadStackSize);
            goto abort;
        }
    }

    if (ct && !(ct->t.n_ItemFlags & ITEMNODE_PRIVILEGED)
    && (t->t.n_Priority < TASK_MIN_PRIORITY || t->t.n_Priority > TASK_MAX_PRIORITY))
    {
        ret = MAKEKERR(ER_SEVERE, ER_C_NSTND, ER_Kr_BadPriority);
        goto abort;
    }

    if ((t->t_MaxUSecs < MIN_QUANTA_USECS) || (t->t_MaxUSecs > MAX_QUANTA_USECS))
    {
        ret = MAKEKERR(ER_SEVERE, ER_C_NSTND, ER_Kr_BadQuanta);
        goto abort;
    }

    {
    TimeVal tv;

       tv.tv_Seconds      = 0;
       tv.tv_Microseconds = t->t_MaxUSecs;
       ConvertTimeValToTimerTicks(&tv, &t->t_Quantum);
    }
    t->t_QuantumRemainder = t->t_Quantum;

    /* pass a few things to the task being started */
    t->t_RegisterSave.rb_GPRs[5] = tinfo.ti_Module ? tinfo.ti_Module->n.n_Item : 0;
    t->t_RegisterSave.rb_GPRs[6] = (uint32) KernelBase;

    if (li)
    {
        DBUGCT(("CT: allocating user stack and other things\n"));

        if (!isthread)
        {
        uint32  userSize;
        void   *userMem;
        void   *baseUserMem;
        uint32  actualSize;
        uint32  cmdLineSize;

            /* allocate stack, cmdline buffer, dummy free block */

            if (tinfo.ti_CmdLine)
                cmdLineSize = SizeArguments(tinfo.ti_CmdLine);
            else
                cmdLineSize = 0;

            userSize = t->t_StackSize + cmdLineSize + sizeof(FreeBlock)*2;

            DBUGCT(("CT: allocating %d for user\n",userSize));

            baseUserMem = externalAllocMemPages(userSize, MEMTYPE_NORMAL);
            if (baseUserMem == NULL)
            {
                ret = NOMEM;
                goto abort;
            }

            userMem    = baseUserMem;
            actualSize = *(uint32 *)userMem;

            DBUGCT(("CT: user chunk at $%x\n",userMem));

            t->t_RegisterSave.rb_SP = (uint32)userMem + t->t_StackSize;
            t->t_StackBase          = userMem;

            userMem = (void *)((uint32)userMem + t->t_StackSize);

            if (tinfo.ti_CmdLine)
            {
            uint32  argc;
            char  **argv;

                DBUGCT(("CT: building command-line arguments\n"));

                argc = BuildArguments(tinfo.ti_CmdLine, (char *)userMem, &argv);

                t->t_RegisterSave.rb_GPRs[3] = argc;
                t->t_RegisterSave.rb_GPRs[4] = (uint32) argv;

                userMem = (void *)((uint32)userMem + cmdLineSize);
            }

            t->t_PagePool->pp_FreeBlocks            = userMem;
            t->t_PagePool->pp_FreeBlocks[0].fb_Next = &t->t_PagePool->pp_FreeBlocks[1];
            t->t_PagePool->pp_FreeBlocks[0].fb_Size = sizeof(FreeBlock);
            t->t_PagePool->pp_FreeBlocks[1].fb_Next = NULL;
            t->t_PagePool->pp_FreeBlocks[1].fb_Size = actualSize - userSize + sizeof(FreeBlock);

            DBUGCT(("CT: StackBase $%x, stack size %d, FreeBlock $%x\n",t->t_StackBase,t->t_StackSize,t->t_PagePool->pp_FreeBlocks));
            DBUGCT(("CT: Giving blocks of memory to new task\n"));

            if (ct)
            {
                externalControlMem(baseUserMem, actualSize, MEMC_GIVE, t->t.n_Item);
                externalControlMem(baseUserMem, actualSize, MEMC_NOWRITE, ct->t.n_Item);
            }
            else
            {
                ControlPagePool(KB_FIELD(kb_PagePool), baseUserMem, actualSize, MEMC_GIVE, t->t.n_Item);
            }
        }
    }

    if (t->t_Flags & TASK_SINGLE_STACK)
    {
        /* so the memory gets freed on task exit */
        t->t_SuperStackSize = t->t_StackSize;
        t->t_SuperStackBase = t->t_StackBase;
    }

    if (tinfo.ti_DefaultMsgPort)
    {
    TagArg tags[2];

        tags[0].ta_Tag = TAG_ITEM_NAME;
        tags[0].ta_Arg = t->t.n_Name;
        tags[1].ta_Tag = 0;

        ret = internalCreateItem(MKNODEID(KERNELNODE,MSGPORTNODE), tags);
        if (ret < 0)
            goto abort;

        t->t_DefaultMsgPort = ret;

        ret = externalSetItemOwner(ret, t->t.n_Item);
        if (ret < 0)
        {
            externalDeleteItem(t->t_DefaultMsgPort);
            goto abort;
        }
    }

    if (tinfo.ti_ExitMsgPort >= 0)
    {
    TagArg tags[5];

        tags[0].ta_Tag = TAG_ITEM_NAME;
        tags[0].ta_Arg = (void *)"Task Exit";
        tags[1].ta_Tag = TAG_ITEM_PRI;
        tags[1].ta_Arg = (void *)t->t.n_Priority;
        tags[2].ta_Tag = CREATEMSG_TAG_REPLYPORT;
        tags[2].ta_Arg = (void *)tinfo.ti_ExitMsgPort;
        tags[3].ta_Tag = CREATEMSG_TAG_MSG_IS_SMALL;
        tags[4].ta_Tag = TAG_END;

        ret = internalCreateItem(MKNODEID(KERNELNODE,MESSAGENODE), tags);
        if (ret < 0)
            goto abort;

        t->t_ExitMessage = ret;
    }

    if (tinfo.ti_ExceptionMsgPort >= 0)
    {
        ret = externalRegisterUserExceptionHandler(t->t.n_Item, tinfo.ti_ExceptionMsgPort);
        if (ret < 0)
            goto abort;
    }

#ifdef BUILD_PARANOIA
    /* initialize the stacks to determine how much of them we're using */
    if (t->t_StackBase)
    {
      uint32 *p;
      uint32 top;
      top = (uint32)t->t_StackBase+t->t_StackSize;
      p = t->t_StackBase;
      while((uint32)p < top)
        *p++ = 0xDEADCAFE;
    }

    if (t->t_SuperStackBase)
    {
      uint32 *p;
      uint32 top;
      top = (uint32)t->t_SuperStackBase + t->t_SuperStackSize - sizeof(SuperStackFrame);
      p = t->t_SuperStackBase;
      while((uint32)p < top)
        *p++ = 0xDEADCAFE;
    }
#endif

    /* Create an EABI stack frame for use by the function being
     * launched, and also by exit() if the task
     * exits by falling off the end of the world...
     */

    t->t_RegisterSave.rb_SP -= 8;
    *((uint32 *)t->t_RegisterSave.rb_SP) = 0;  /* NULL backchain */

    if (!ct)
    {
        /* open the operator's module by hand */
        internalOpenModule(tinfo.ti_Module, NULL, t);

        /* We store away info about the operator for easy access */
        KB_FIELD(kb_OperatorTask) = t->t.n_Item;
    }
    else if (tinfo.ti_Module)
    {
        t->t_Module = tinfo.ti_Module->n.n_Item;

        ret = externalOpenModuleTree(tinfo.ti_Module->n.n_Item, t->t.n_Item);
        if (ret < 0)
            goto abort;

        t->t_Module = ret;	/* Use the opened item # */
    }

#ifdef BUILD_STRINGS
    if (KB_FIELD(kb_ShowTaskMessages))
    {
        printf("***** %s \"%s\": text %x, data %x, PPC PC %x, item %x\n",
               (isthread) ? "Thread" : "Task",
               t->t.n_Name,
               !li ? NULL : li->codeBase,
               !li ? NULL : li->dataBase,
               t->t_RegisterSave.rb_PC,
               t->t.n_Item);
        printf("      Stacks: user (%x,%x), super (%x,%x)\n",
               t->t_StackBase, t->t_StackSize,
               t->t_SuperStackBase, t->t_SuperStackSize);
        printf("      SP %x, Super SP %x\n",
               t->t_RegisterSave.rb_SP, t->t_ssp);
    }
#endif

    /* Add the new task to the list of tasks in the system */
    ADDTAIL(&KB_FIELD(kb_Tasks), (Node *)&(t->t_TasksLinkNode));

    LogTaskCreated(t);

    oldints = Disable();

#ifdef BUILD_MACDEBUGGER
    if (li)
    {
        if (t->t_ThreadTask == NULL)
        {
            Dbgr_TaskCreated(t, li);
        }
    }
#endif

    if (ct == NULL)
    {
#ifdef BUILD_MEMDEBUG
        DBUGCT(("CT: calling InitMemDebug()\n"));
        InitMemDebug();
#endif

        DBUGCT(("CT: calling FinishInitMem()\n"));
        FinishInitMem();

        /* add the task to the ready q */
        InsertNodeFromTail(&KB_FIELD(kb_TaskReadyQ), (Node *)t);

#ifdef BUILD_PCDEBUGGER
        Dbgr_TaskCreated(t, NULL);
#endif

        DBUGCT(("CT: calling ScheduleNewTask() for operator\n"));
        ScheduleNewTask();
    }

    DBUGCT(("CT: calling NewReadyTask(%lx)\n",t));
    NewReadyTask(t);

#ifdef BUILD_PCDEBUGGER
    Dbgr_TaskCreated(t, NULL);
#endif

    Enable(oldints);
    return t->t.n_Item;

abort:
    if (t->t_ExitMessage >= 0)
        internalDeleteItem(t->t_ExitMessage);

    if (t->t_DefaultMsgPort >= 0)
        internalDeleteItem(t->t_DefaultMsgPort);

    if (t->t_Flags & TASK_FREE_STACK)
        t->t_StackBase = NULL;

#ifdef BUILD_STRINGS
    printf("CT: failed to create '%s'\n",t->t.n_Name);
    printf("CT: exiting with: ");
    PrintfSysErr(ret);
#endif

    FreeTask(t);

    return ret;
}


/*****************************************************************************/


Err
internalSetTaskOwner(Task *t, Item newowner)
{
	Task	*pt = t->t_ThreadTask;
	Item	towner;
	Item	ti;

	if (pt)
	{
		/* Trying to change ownership of a thread */

		Task	*nt = (Task *)LookupItem(newowner);

		/* Don't allow thread to chown to another task or its thread */
		if ((pt != nt) && (pt != nt->t_ThreadTask))
			return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_CantSetOwner);

		/* Don't allow thread to chown to itself */
		if (t == nt)
			return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_CantSetOwner);
	}

	towner = newowner;
	ti = t->t.n_Item;
	do
	{
		/* Check for cyclic ownership relation */

		towner = ((Task *)LookupItem(towner))->t.n_Owner;

		if (towner == ti)
			return MAKEKERR(ER_SEVER,ER_C_NSTND,ER_Kr_CantSetOwner);
	}
	while (towner);

	return 0;
}

void FreeDeadTasks(void)
{
        ItemNode *n;

        /* Drain the list of dead tasks. This'll free up their task structures,
         * as well as their super stacks.
         */
	while (1)
	{
	    n = (ItemNode *)RemHead(&KB_FIELD(kb_DeadTasks));
	    if (!n)
	        break;

	    SuperFreeMem(((Task *)n)->t_SuperStackBase,((Task *)n)->t_SuperStackSize);
	    FreeNode((Folio *)&KB,n);
	}
}
