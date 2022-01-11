/* @(#) taskswap.c 96/10/23 1.64 */

#include <kernel/types.h>
#include <kernel/nodes.h>
#include <kernel/list.h>
#include <kernel/listmacros.h>
#include <kernel/task.h>
#include <kernel/kernel.h>
#include <kernel/time.h>
#include <kernel/timer.h>
#include <kernel/lumberjack.h>
#include <hardware/PPC.h>
#include <hardware/PPCasm.h>
#include <string.h>
#include <stdio.h>
#include <kernel/internalf.h>


/*****************************************************************************/


extern Timer *_quantaclock;

static TimerTicks taskStartTime;
static bool       timerArmed;

static void LandReady(Task *t);
static void Launch(Task *t);

#ifdef __DCC__
#pragma no_return Launch
#pragma no_return LoadTaskRegs
#endif


/*****************************************************************************/


/**
|||	AUTODOC -public -class Kernel -group Tasks -name Yield
|||	Give up the CPU to a task of equal priority.
|||
|||	  Synopsis
|||
|||	    void Yield( void )
|||
|||	  Description
|||
|||	    In Portfolio, high-priority tasks always have precedence over lower
|||	    priority tasks. Whenever a high priority task becomes ready to execute, it
|||	    will instantly interrupt lower priority tasks and start running. The lower
|||	    priority tasks do not get to finish their time quantum, and just get put
|||	    into the system's ready queue for future scheduling.
|||
|||	    If there are a number of tasks of equal priority which are all ready to
|||	    run, the kernel does round-robin scheduling of these tasks. This is a
|||	    process by which each task is allowed to run for a fixed amount of time
|||	    before the CPU is taken away from it, and given to another task of the
|||	    same priority. The amount of time a task is allowed to run before being
|||	    preempted is called the task's "quantum".
|||
|||	    The purpose of the Yield() function is to let a task voluntarily give up
|||	    the remaining time of its quantum. Since the time quantum is only an issue
|||	    when the kernel does round-robin scheduling, it means that Yield()
|||	    actually only does something when there are multiple ready tasks at the
|||	    same priority. However, since the yielding task does not know exactly
|||	    which task, if any, is going to run next, Yield() should not be used for
|||	    implicit communication amongst tasks. The way to cooperate amongst tasks
|||	    is using signals, messages, and semaphores.
|||
|||	    In short, if there are higher-priority tasks in the system, the current
|||	    task will only run if the higher-priority tasks are all in the wait queue.
|||	    If there are lower-priority tasks, these will only run if the current task
|||	    is in the wait queue. And if there are other tasks of the same priority,
|||	    the kernel automatically cycles through all the tasks, spending a quantum
|||	    of time on each task, unless a task calls Yield(), which will cut short
|||	    its quantum.
|||
|||	    If there are no other ready tasks of the same priority as the task that
|||	    calls Yield(), then that task will keep running as if nothing happened.
|||
|||	    Yield() is only useful in very specific circumstances. If you plan on
|||	    using it, think twice. In most cases it is not needed, and using it will
|||	    result in a general degradation of system performance.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/task.h>, libc.a
|||
|||	  See Also
|||
|||	    WaitSignal()
|||
|||
**/

/* Note that the autodoc above is not being quite honest. Yield()
 * will in fact give up the CPU to tasks of _greater or equal_ priority
 * as the current task. The only time a task of higher priority can be
 * in the ready q is if the current task has turned off multitasking.
 */

void internalYield(void)
{
Task   *ct;
Task   *t;
uint32  oldints;

    oldints = Disable();

    t = (Task *)FirstNode(&KB_FIELD(kb_TaskReadyQ));
    if (IsNode(&KB_FIELD(kb_TaskReadyQ),t))
    {
        ct = CURRENTTASK;
        if (t->t.n_Priority >= ct->t.n_Priority)
        {
            /* give it up! */
            ct->t.n_Flags |= TASK_QUANTUM_EXPIRED;

            if (SaveTaskRegs(ct) == 0)
            {
                LandReady(ct);
                REMOVENODE((Node *)t);
                Launch(t);

                /* execution never comes back here, Launch() is like a goto */
            }

            /* SaveTaskRegs() has returned with a non NULL value. It means that
             * we have just been rescheduled.
             */
        }
    }

    Enable(oldints);
}


/*****************************************************************************/


/* This function performs the high-level work needed to put a ready task to
 * sleep.
 *
 * This code runs with interrupts disabled.
 */
static void LandReady(Task *t)
{
TimerTicks tt;

#ifdef BUILD_PARANOIA
    if (t->t_StackBase && ((*(uint32 *)t->t_StackBase) != 0xDEADCAFE))
    {
        printf("WARNING: Task '%s' appears to have overflowed its user stack!\n",t->t.n_Name);
        printf("         cookie was 0xdeadcafe and is now 0x%08x\n", *(uint32 *)t->t_StackBase);
        *(uint32 *)t->t_StackBase = 0xDEADCAFE;
    }

    if (t->t_SuperStackBase && ((*(uint32 *)t->t_SuperStackBase) != 0xDEADCAFE))
    {
        printf("WARNING: Task '%s' appears to have overflowed its super stack!\n", t->t.n_Name);
        printf("         cookie was 0xdeadcafe and is now 0x%08x\n", *(uint32 *)t->t_SuperStackBase);
        *(uint32 *)t->t_SuperStackBase = 0xDEADCAFE;
    }
#endif

    /* set the ready bit, clear the running bit */
    t->t.n_Flags = (t->t.n_Flags | TASK_READY) & (~TASK_RUNNING);

    /* update this guy's elapsed time */
    SampleSystemTimeTT(&tt);
    SubTimerTicks(&taskStartTime,&tt,&tt);
    AddTimerTicks(&t->t_ElapsedTime,&tt,&t->t_ElapsedTime);

    if (t->t.n_Flags & TASK_QUANTUM_EXPIRED)
    {
        t->t_QuantumRemainder = t->t_Quantum;
        t->t.n_Flags          &= (~TASK_QUANTUM_EXPIRED);

        /* If the task being interrupted expired its quantum,
         * put it after other tasks of the same priority.
         */
        InsertNodeFromTail(&KB_FIELD(kb_TaskReadyQ), (Node *)t);
    }
    else
    {
        /* If the task's quantum wasn't done with, figure out how much
         * time is left.
         */
        SubTimerTicks(&tt,&t->t_QuantumRemainder,&t->t_QuantumRemainder);

        /* If the task being interrupted still had some
         * time to go, put it before other tasks of the
         * same priority.
         */
        InsertNodeFromHead(&KB_FIELD(kb_TaskReadyQ), (Node *)t);
    }
}


/*****************************************************************************/


/* This function performs the high-level work needed to put a waiting task to
 * sleep.
 *
 * This code runs with interrupts disabled.
 */
static void LandWaiter(Task *t)
{
TimerTicks tt;

#ifdef BUILD_PARANOIA
    if (t->t_StackBase && ((*(uint32 *)t->t_StackBase) != 0xDEADCAFE))
    {
        printf("WARNING: Task '%s' appears to have overflowed its user stack!\n",t->t.n_Name);
        printf("         cookie @ 0x%08x was 0xdeadcafe and is now 0x%08x\n",
	       t->t_StackBase, *(uint32 *)t->t_StackBase);
        *(uint32 *)t->t_StackBase = 0xDEADCAFE;
    }

    if (t->t_SuperStackBase && ((*(uint32 *)t->t_SuperStackBase) != 0xDEADCAFE))
    {
        printf("WARNING: Task '%s' appears to have overflowed its super stack!\n", t->t.n_Name);
        printf("         cookie was 0xdeadcafe and is now 0x%08x\n", *(uint32 *)t->t_SuperStackBase);
        *(uint32 *)t->t_SuperStackBase = 0xDEADCAFE;
    }
#endif

    /* set the waiting bit, clear the running and expired bits */
    t->t.n_Flags = (t->t.n_Flags | TASK_WAITING) & (~(TASK_READY | TASK_RUNNING | TASK_QUANTUM_EXPIRED));

    t->t_QuantumRemainder = t->t_Quantum;

    /* update this guy's elapsed time */
    SampleSystemTimeTT(&tt);
    SubTimerTicks(&taskStartTime,&tt,&tt);
    AddTimerTicks(&t->t_ElapsedTime,&tt,&t->t_ElapsedTime);
}


/*****************************************************************************/


/* This function is called to launch a new task. It does some general
 * maintenance, loads the new task state, and jumps into it.
 *
 * This function is like a goto to the new task, which means it never returns
 * to its caller.
 *
 * This code runs with interrupts disabled.
 */
static void Launch(Task *t)
{
Task *firstReady;

    firstReady = (Task *)FirstNode(&KB_FIELD(kb_TaskReadyQ));
    if (IsNode(&KB_FIELD(kb_TaskReadyQ),firstReady)
     && (firstReady->t.n_Priority == t->t.n_Priority))
    {
        /* Only load the timer if there is at least one other ready task
         * of equal priority
         */
        timerArmed = TRUE;
        LoadTimer(_quantaclock,&t->t_QuantumRemainder);
    }
    else if (timerArmed)
    {
        timerArmed = FALSE;
        UnloadTimer(_quantaclock);
    }

    SampleSystemTimeTT(&taskStartTime);

    /* set the running bit, clear the waiting and ready bits */
    t->t.n_Flags = (t->t.n_Flags | TASK_RUNNING) & (~(TASK_WAITING | TASK_READY));
    t->t_NumTaskLaunch++;

    LoadFence(t);

    CopyToConst(&KB_FIELD(kb_CurrentTask),t);
    CopyToConst(&KB_FIELD(kb_CurrentTaskItem),(void *)t->t.n_Item);
    KB_FIELD(kb_PleaseReschedule) = FALSE;
    KB_FIELD(kb_NumTaskSwitches)++;

    LogTaskRunning(t);

    LoadTaskRegs(t);
}


/*****************************************************************************/


/* This function is called after a task calls WaitSignal() or after a task has
 * commited suicide in order to schedule a new task for execution.
 */
void ScheduleNewTask(void)
{
Task *t;

    Disable();

    t = (Task *)RemHead(&KB_FIELD(kb_TaskReadyQ));
    if (t)
    {
        Launch(t);

        /* execution never comes back here, Launch() is like a goto */
    }

    LogTaskRunning(NULL);

    /* There's no other task to schedule, go waste time... */
    IdleLoop();
    /* execution never comes back here */
}


/*****************************************************************************/


/* This is called when a task enters wait state. Its register context is saved
 * and a new task is scheduled. The function returns when the task gets
 * rescheduled.
 *
 * This code runs with interrupts disabled.
 */
void SleepTask(Task *t)
{
    if (SaveTaskRegs(t) == NULL)
    {
        LandWaiter(t);
        ScheduleNewTask();

        /* we never get to this point... */
    }
}


/*****************************************************************************/


/* This is called to stop an active task dead in its tracks.
 *
 * Upon entry to this routine, the register state of the task is presumed to
 * have already been saved in its TCB.
 *
 * This code must be called with interrupts disabled.
 */
void SuspendTask(Task *t)
{
    if ((t->t_Flags & TASK_SUSPENDED) == 0)
    {
        if (t == CURRENTTASK)
        {
            /* if we're suspending the current task, pretend it got preempted */
            CopyToConst(&KB_FIELD(kb_CurrentTask), NULL);
            CopyToConst(&KB_FIELD(kb_CurrentTaskItem), (void *)-1);
            KB_FIELD(kb_PleaseReschedule) = TRUE;
            LandReady(t);
        }

        if (t->t.n_Flags & TASK_READY)
        {
            /* prevent the task from being scheduled */
            RemNode((Node *)t);
        }

        t->t_Flags |= TASK_SUSPENDED;
    }
}


/*****************************************************************************/


void ResumeTask(Task *t)
{
    if (t->t_Flags & TASK_SUSPENDED)
    {
        t->t_Flags &= (~TASK_SUSPENDED);

        if (t->t.n_Flags & TASK_READY)
        {
            t->t.n_Flags |= TASK_WAITING;
            NewReadyTask(t);
        }
    }
}


/*****************************************************************************/


/* After a decrementer exception or async interrupt occurs, we come here to
 * see if we need to perform a task swap.
 *
 * We don't do a task swap if one of the following is true:
 *
 *	- the rescheduling flag is not set
 *
 *	- there is no task in the ready q
 *
 *	- the highest priority task in the ready q has lower priority
 *	  than the current task.
 *
 *	- the current task has task a forbid count greater than 0.
 *
 * When we enter here, the running state of the current task has already
 * been saved in its TCB. We must determine if a task switch is really
 * needed. If not, we just return. If yes, we complete its landing, and
 * dispatch a new task.
 *
 * This code runs with interrupts disabled.
 */
void CheckForTaskSwap(void)
{
Task *ct;
Task *t;

    if (KB_FIELD(kb_PleaseReschedule))
    {
	t = (Task *)FirstNode(&KB_FIELD(kb_TaskReadyQ));
	if (IsNode(&KB_FIELD(kb_TaskReadyQ),t))
	{
            ct = CURRENTTASK;

            if (ct == NULL)
            {
                /* If there is no current task, it means that there is
                 * currently no active task in the system, and we were
                 * executing the idle loop. We can just launch the new task.
                 */
                REMOVENODE((Node *)t);
                Launch(t);

                /* execution never comes back here, Launch() is like a goto */

            }
            else if (t->t.n_Priority >= ct->t.n_Priority)
            {
                if (ct->t_Forbid == 0)
                {
                    LandReady(ct);
                    REMOVENODE((Node *)t);
                    Launch(t);

                    /* execution never comes back here, Launch() is like a goto */
                }
            }
            else
            {
                /* There are no higher priority tasks on the ready q,
                 * so there's no reason to ask for rescheduling.
                 */
                KB_FIELD(kb_PleaseReschedule) = FALSE;
            }
        }
        else
        {
            /* If there are no ready tasks, there's no reason to ask for
             * rescheduling.
             */
            KB_FIELD(kb_PleaseReschedule) = FALSE;
        }
    }

    /* no need for a task swap, go back */
}


/*****************************************************************************/


/* This routine is called from the system call handler when the handler
 * has detected that kb_PleaseReschedule was set. This will happen when
 * the system call ended up sending a signal to another task of equal or
 * higher priority than itself. The function also gets called from Permit()
 * when the forbid nest count of a task drops to 0.
 *
 * The purpose of this function is to let tasks of equal or higher priority
 * run.
 *
 * This code runs with interrupts disabled.
 */
void SwapTasks(void)
{
Task *ct;
Task *t;

    t = (Task *)FirstNode(&KB_FIELD(kb_TaskReadyQ));
    if (IsNode(&KB_FIELD(kb_TaskReadyQ),(Node *)t))
    {
        ct = CURRENTTASK;
        if (t->t.n_Priority >= ct->t.n_Priority)
        {
            if (SaveTaskRegs(ct) == 0)
            {
                LandReady(ct);
                REMOVENODE((Node *)t);
                Launch(t);

                /* execution never comes back here, Launch() is like a goto */
            }

            /* SaveTaskRegs() has returned with a non NULL value. It means that
             * we have just been rescheduled. All of our running state has been
             * restored, and we can return to the system call handler to complete
             * the system call that got us here to begin with.
             */

            return; /* to system call handler */
        }
     }

     /* We only get here if there are no new tasks to run
      * so there is no reason to ask for rescheduling.
      */
     KB_FIELD(kb_PleaseReschedule) = FALSE;
}


/*****************************************************************************/


/* This routine is called from interrupt code when the quantum timer has
 * expired. It simply asks for a task reschedule when the interrupt
 * handler later calls CheckForTaskSwap().
 *
 * This code runs with interrupts disabled.
 */
int32 _cblank(Timer *timer)
{
    TOUCH(timer);

    if (CURRENTTASK)
    {
        CURRENTTASK->t.n_Flags |= TASK_QUANTUM_EXPIRED;
        KB_FIELD(kb_PleaseReschedule) = TRUE;
    }

    timerArmed = FALSE;

    return 0;
}


/*****************************************************************************/


/* This function decrements the forbid nest count. While the forbid count
 * is greater than 0, preemptive task switching is not performed. When the
 * forbid count is set to 0, and kb_PleaseReschedule is set, we need to do
 * a task switch.
 */
void Permit(void)
{
uint32 oldints;

    oldints = Disable();

    CURRENTTASK->t_Forbid--;

    LogTaskPermit();

    if (KB_FIELD(kb_PleaseReschedule))
    {
        if (CURRENTTASK->t_Forbid == 0)
        {
            SwapTasks();
        }
    }

    Enable(oldints);
}


/*****************************************************************************/


/* This function is called to move a task from wait state to the ready q
 *
 * This code runs with interrupts disabled.
 */
void NewReadyTask(Task *t)
{
TimerTicks tt;
Task       *ct;

    /* if the task is already in the ready q or is currently running, then
     * don't do anything.
     */
    if (t->t.n_Flags & TASK_WAITING)
    {
        LogTaskReady(t);

        t->t.n_Flags = (t->t.n_Flags | TASK_READY) & (~TASK_WAITING);

        if (t->t_Flags & TASK_SUSPENDED)
        {
            /* can't schedule a suspended task */
            return;
        }

        InsertNodeFromTail(&KB_FIELD(kb_TaskReadyQ),(Node *)t);

        ct = CURRENTTASK;

        if ((ct == NULL) || (t->t.n_Priority > ct->t.n_Priority))
        {
            /* There's no current task or the priority of the new task
             * is greater than the priority of the current task.
             */
            KB_FIELD(kb_PleaseReschedule) = TRUE;
        }
        else if (t->t.n_Priority == ct->t.n_Priority)
        {
            /* The priority of the new task matches that of the current task */

            if (ct->t.n_Flags & TASK_QUANTUM_EXPIRED)
            {
                /* If the current task expired its quantum, ask for a reschedule */
                KB_FIELD(kb_PleaseReschedule) = TRUE;
            }
            else if (!timerArmed)
            {
                /* The timer is not currently armed. This means that the
                 * current running task didn't have any competing tasks
                 * at the same priority as it is. We therefore compute how
                 * much time the current task has been running for.
                 * If it's more than a quantum's worth, we ask for a
                 * reschedule. Otherwise, start the timer to trigger us
                 * when the task will have spent its quantum.
                 */
                SampleSystemTimeTT(&tt);
                SubTimerTicks(&taskStartTime,&tt,&tt);
                SubTimerTicks(&tt,&ct->t_QuantumRemainder,&ct->t_QuantumRemainder);

                if (ct->t_QuantumRemainder.tt_Hi || ct->t_QuantumRemainder.tt_Lo)
                {
                    /* this task has run for less than its quantum, let it go... */
                    timerArmed = TRUE;
                    LoadTimer(_quantaclock,&ct->t_QuantumRemainder);
                }
                else
                {
                    /* Task has run for more than its quantum */
                    ct->t.n_Flags |= TASK_QUANTUM_EXPIRED;
                    KB_FIELD(kb_PleaseReschedule) = TRUE;
                }
            }
        }
    }
}


/*****************************************************************************/


/**
|||	AUTODOC -public -class Kernel -group Tasks -name InvalidateFPState
|||	Invalidates the current contents of the floating-point registers
|||	to prevent them from being saved during a context switch.
|||
|||	  Synopsis
|||
|||	    void InvalidateFPState(void);
|||
|||	  Description
|||
|||	    Portfolio tries to handle floating-point registers in a smart way
|||	    to improve overall system performance. This function can be used
|||	    by savvy clients to improve multitasking speed.
|||
|||	    When performing a task switch operation, the state of the FP
|||	    registers is not saved along with the regular registers. Instead,
|||	    Portfolio has the concept of a floating-point owner task.
|||	    If a task attempts to do a floating-point operation without
|||	    being the current owner of the FPU, the kernel automatically
|||	    saves the FP state of the current owner, loads the FP state of
|||	    the task attempting to do an FP operation, and then marks this
|||	    task as the FPU owner.
|||
|||	    The reason behind this scheme is to keep to a minimum the number
|||	    of times FP state needs to be saved and restored. For example,
|||	    if thread A does floating-point calculations, and thread B starts
|||	    running and doesn't do any floating-point, and thread A starts
|||	    running again, there will have been no FP save/restore operation
|||	    performed. This saves a considerable amount of time which improves
|||	    effective task switch performance.
|||
|||	    As a result of this architecture, you will get optimal performance
|||	    if only one of your threads does FP operations, and all the others
|||	    are purely integer based. With such a setup, no FP state will ever
|||	    need to be saved or restored.
|||
|||	    This function is intended for savvy clients that realize at
|||	    some point that the current contents of the FP registers is no
|||	    longer of any value, and so does not need to be preserved. Making
|||	    this call effectively invalidates the contents of the FP registers.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V27.
|||
**/

void internalInvalidateFPState(void)
{
uint32 msr;

    if (KB_FIELD(kb_FPOwner) == CURRENTTASK)
    {
        KB_FIELD(kb_FPOwner) = NULL;

	msr = _mfmsr();
	_mtmsr(msr & (~MSR_FP));
    }
}
