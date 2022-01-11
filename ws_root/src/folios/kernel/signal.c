/* @(#) signal.c 96/06/11 1.45 */

#define DBUG(x)	 /*printf x*/

#ifdef MASTERDEBUG
#define DBUGWAIT(x)	if (CheckDebug(KernelBase,1)) printf x
#define DBUGSIGNAL(x)	if (CheckDebug(KernelBase,2)) printf x
#else
#define DBUGWAIT(x)
#define DBUGSIGNAL(x)
#endif

#include <kernel/types.h>
#include <kernel/nodes.h>
#include <kernel/kernelnodes.h>
#include <kernel/list.h>
#include <kernel/listmacros.h>
#include <kernel/task.h>
#include <kernel/kernel.h>
#include <kernel/operror.h>
#include <kernel/lumberjack.h>
#include <stdio.h>
#include <string.h>
#include <kernel/internalf.h>


#define ILLEGAL_SIG		0x80000000
#define SUPER_SIGS		0x000000FF
#define RESERVED_SIGBITS	(ILLEGAL_SIG|SUPER_SIGS)


/*****************************************************************************/


/**
|||	AUTODOC -public -class Kernel -group Signals -name GetCurrentSignals
|||	Gets the currently received signal bits.
|||
|||	  Synopsis
|||
|||	    int32 GetCurrentSignals(void);
|||
|||	  Description
|||
|||	    This macro returns the signal bits that have been received by the
|||	    current task.
|||
|||	  Return Value
|||
|||	    A 32-bit word in which all currently received signal bits are set.
|||
|||	  Implementation
|||
|||	    Macro implemented in <kernel/task.h> V20.
|||
|||	  Associated Files
|||
|||	    <kernel/task.h>, libc.a
|||
|||	  See Also
|||
|||	    AllocSignal(), FreeSignal(), SendSignal(), WaitSignal(),
|||	    ClearCurrentSignals()
|||
**/

/**
|||	AUTODOC -public -class Kernel -group Signals -name ClearCurrentSignals
|||	Clears some received signal bits.
|||
|||	  Synopsis
|||
|||	    Err ClearCurrentSignals(int32 sigMask);
|||
|||	  Description
|||
|||	    This macro resets the requested signal bits of the current task to
|||	    0.
|||
|||	  Arguments
|||
|||	    sigMask
|||	        A 32-bit word indicating which signal bits should be cleared.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Macro implemented in <kernel/task.h> V24.
|||
|||	  Associated Files
|||
|||	    <kernel/task.h>, libc.a
|||
|||	  See Also
|||
|||	    AllocSignal(), FreeSignal(), SendSignal(), WaitSignal(),
|||	    GetCurrentSignals()
|||
**/

/**
|||	AUTODOC -public -class Kernel -group Signals -name GetTaskSignals
|||	Gets the currently received signal bits for a task.
|||
|||	  Synopsis
|||
|||	    int32 GetTaskSignals(Task *t);
|||
|||	  Description
|||
|||	    This macro returns the signal bits that have been received by the
|||	    specified task.
|||
|||	  Return Value
|||
|||	    A 32-bit word in which all currently received signal bits for the
|||	    task are set.
|||
|||	  Implementation
|||
|||	    Macro implemented in <kernel/task.h> V21.
|||
|||	  Associated Files
|||
|||	    <kernel/task.h>, libc.a
|||
|||	  See Also
|||
|||	    AllocSignal(), FreeSignal(), SendSignal(), WaitSignal()
|||
**/

/**
|||	AUTODOC -public -class Kernel -group Signals -name WaitSignal
|||	Waits until any of a set of signals are received.
|||
|||	  Synopsis
|||
|||	    int32 WaitSignal(int32 sigMask);
|||
|||	  Description
|||
|||	    This function puts the calling task into wait state until any of
|||	    the signals specified in sigMask have been received. When a task is
|||	    in wait state, it uses no CPU time.
|||
|||	    When WaitSignal() returns, bits set in the result indicate which of
|||	    the signals the task was waiting for were received since the last
|||	    call to WaitSignal(). If the task was not waiting for certain
|||	    signals, the bits for those signals remain set in the task's signal
|||	    word, and all other bits in the signal word are cleared.
|||
|||	    See AllocSignal() for a description of the implementation of
|||	    signals.
|||
|||	  Arguments
|||
|||	    sigMask
|||	        A mask in which bits are set to specify the signals the task
|||	        wants to wait for.
|||
|||	  Return Value
|||
|||	    Returns a mask that specifies which of the signals a task
|||	    was waiting for have been received, or a negative error code
|||	    for failure. Possible error codes currently include:
|||
|||	    ILLEGALSIGNAL
|||	        One or more of the signal bits in the sigMask argument were not
|||	        allocated by the task.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/task.h>, libc.a
|||
|||	  Notes
|||
|||	    Because it is possible for tasks to send signals in error, it is up to
|||	    tasks to confirm that the actual event occurred when they receive a
|||	    signal.
|||
|||	    For example, if you were waiting for SIGF_IODONE and the return
|||	    value from WaitSignal() indicated that the signal was sent, you
|||	    should still call CheckIO() using the IOReq to make sure it is
|||	    actually done. If it was not done you should go back to
|||	    WaitSignal().
|||
|||	  See Also
|||
|||	    AllocSignal(), SendSignal(), GetCurrentSignals(),
|||	    ClearCurrentSignals()
|||
**/

int32 internalWait(int32 bits)
{
Task   *ct = CURRENTTASK;
uint32  oldints;
uint32  res;

    if (bits & (~ct->t_AllocatedSigs))
        return ILLEGALSIGNAL;

    bits |= SIGF_ABORT; /* we can always be aborted */
    oldints = Disable();
    res = bits & ct->t_SigBits;

    DBUGWAIT(("WaitSignal: task '%s', waitBits $%x, currentBits $%x\n",ct->t.n_Name,bits,ct->t_SigBits));

    LogSignalWait(bits,ct->t_SigBits);

    if (!res)
    {
        /* Have to put current task to sleep */
        ct->t_WaitBits = bits;
        SleepTask(ct);
        ct->t_WaitBits = 0;

        /* process has woken up */

        DBUGWAIT(("WaitSignal: task '%s' was rescheduled!\n",ct->t.n_Name));
        res = bits & ct->t_SigBits;  /* recompute */
    }

    DBUGWAIT(("WaitSignal: exiting with $%x\n", res));

    /* Clear the bits we just pinged off of */
    ClearSignals(ct,res);
    Enable(oldints);

    return res;
}


/**
|||	AUTODOC -public -class Kernel -group Signals -name AllocSignal
|||	Allocates signals.
|||
|||	  Synopsis
|||
|||	    int32 AllocSignal(int32 sigMask);
|||
|||	  Description
|||
|||	    One of the ways tasks communicate is by sending signals to each
|||	    other. Signals are 1-bit flags that indicate that a particular
|||	    event has occurred.
|||
|||	    Tasks that send and receive signals must agree on which signal bits
|||	    to use and the meanings of the signals. Except for system signals,
|||	    there are no conventions for the meanings of individual signal bits;
|||	    it is up to software developers to define their meanings.
|||
|||	    You allocate bits for new signals by calling AllocSignal(). To
|||	    allocate a single signal bit, you can do:
|||
|||	    theSignal = AllocSignal(0);
|||
|||	    This allocates the next unused bit in the signal word. In the
|||	    return value, the bit that was allocated is set. If the allocation
|||	    fails (which happens if all the non-reserved bits in the signal
|||	    word are already allocated), the function returns 0.
|||
|||	    In rare cases, you may need to define more than one signal with a
|||	    single call. You do this by creating a 32-bit mask and setting any
|||	    bits you want to allocate for new signals, then calling
|||	    AllocSignal() with this bitmask as argument. If all the signals are
|||	    successfully allocated, the bits set in the return value are the
|||	    same as the bits that were set in the argument.
|||
|||	    Signals are implemented as follows:
|||
|||	    * Each task has a 32-bit signal mask that specifies the signals it
|||	      understands. Tasks allocate bits for new signals by calling
|||	      AllocSignal(). The bits are numbered from 0 (the least-significant
|||	      bit) to 31 (the most-significant bit). Bits 0 through 7 are reserved
|||	      for system signals (signals sent by the kernel to all tasks);
|||	      remaining bits can be allocated for other signals. Bit 31 is also
|||	      reserved for the system. It is set when the kernel returns an
|||	      error code to a task instead of signals. For example, trying to
|||	      allocate a system signal or signal number 31.
|||
|||	    * A task calls SendSignal() to send one or more signals to another
|||	      task. Each bit set in the signalWord argument specifies a signal
|||	      to send. Normally, only one signal is sent at a time.
|||
|||	    * When SendSignal() is called, the kernel gets the incoming signal
|||	      word and ORs it into the received signal mask of the target task.
|||	      If the task was in wait state, it compares the received signals
|||	      with the mask of waited signals. If there are any bits set in the
|||	      target, the task is moved to the ready queue where it awaits
|||	      attention from the CPU.
|||
|||	    * A task gets incoming signals by calling WaitSignal(). If any bits
|||	      matching the supplied mask are set in the task's signal word,
|||	      WaitSignal() returns immediately. If no matching bits are set in
|||	      the task's received signal word, the task enters wait state until
|||	      a signal arrives that matches one of the signals the task is
|||	      waiting for.
|||
|||	    The system signals currently include:
|||
|||	    SIGF_IODONE
|||	        Informs the task that an asynchronous I/O request is
|||	        complete.
|||
|||	    SIGF_DEADTASK
|||	        Informs the task that one of its child tasks or
|||	        threads has been deleted.
|||
|||	  Arguments
|||
|||	    signalMask
|||	        An bitmask in which the bits to be allocated are set, or 0 to
|||	        allocate the next available bit. You should use 0 whenever
|||	        possible.
|||
|||	  Return Value
|||
|||	    The function returns bitmask value with bits set for any bits that
|||	    were allocated or 0 if not all of the requested bits could be
|||	    allocated. It returns ILLEGALSIGNAL if the signalMask specified a
|||	    reserved signal.
|||
|||	  Implementation
|||
|||	    Folio call implemented in kernel folio V20.
|||
|||	  Associated Files
|||
|||	    <kernel/task.h>, libc.a
|||
|||	  See Also
|||
|||	    FreeSignal(), GetCurrentSignals(), SendSignal(), WaitSignal(),
|||	    ClearCurrentSignals()
|||
**/

int32 internalAllocSignal(int32 sig, Task *t)
{
uint32  cursigs;
uint32  oldints;

    cursigs = t->t_AllocatedSigs;

    if (sig)
    {
        /* requesting a particular signal */

	if (sig & RESERVED_SIGBITS)
            return ILLEGALSIGNAL;

        if (sig & cursigs)
            return 0;

	t->t_AllocatedSigs |= sig;
    }
    else
    {
	/* get the first free signal bit there is */

	cursigs = ~cursigs;
	sig = ~(cursigs & (cursigs-1));	/* get another bit */
	if (sig < 0)
	{
	    sig = 0;	/* no bits available */
	}
	else
	{
	    t->t_AllocatedSigs = sig;
	    sig &= cursigs;
	}
    }

    oldints = Disable();
    ClearSignals(t,sig);
    Enable(oldints);

    return sig;
}

int32 externalAllocSignal(int32 sigs)
{
    return internalAllocSignal(sigs, CURRENTTASK);
}


/**
|||	AUTODOC -public -class Kernel -group Signals -name FreeSignal
|||	Frees signals.
|||
|||	  Synopsis
|||
|||	    Err FreeSignal(int32 sigMask);
|||
|||	  Description
|||
|||	    This function frees one or more signal bits allocated by
|||	    AllocSignal(). The freed bits can then be reallocated.
|||
|||	  Arguments
|||
|||	    sigMask
|||	        A bitmask in which any signal bits to deallocate are set.
|||
|||	  Return Value
|||
|||	    Returns >= 0 if the signal(s) were freed, or a negative error code
|||	    for failure.
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
|||	    AllocSignal(), WaitSignal(), SendSignal(), GetCurrentSignals()
|||	    ClearCurrentSignals()
|||
**/

Err
internalFreeSignal(int32 sigs, Task *t)
{
uint32 oldints;
uint32 cursigs = t->t_AllocatedSigs;

    if (sigs & RESERVED_SIGBITS)
        return ILLEGALSIGNAL;

    if ((~cursigs) & sigs)
        return ILLEGALSIGNAL;

    cursigs &= ~sigs;
    t->t_AllocatedSigs = cursigs;

    oldints = Disable();
    ClearSignals(t,sigs);           /* clear the freed signals */
    Enable(oldints);

    return 0;
}


/*****************************************************************************/


Err externalFreeSignal(int32 sigs)
{
    return internalFreeSignal(sigs,CURRENTTASK);
}


/*****************************************************************************/


/* This function is callable from interrupt code. */
Err internalSignal(Task *t, int32 bits)
{
uint32  oldints;
uint32  res;

    if (bits & (~t->t_AllocatedSigs))
        return ILLEGALSIGNAL;

    oldints = Disable();

    DBUGSIGNAL(("SendSignal: to task '%s', bits $%x, from task '%s'\n",t->t.n_Name,bits,CURRENTTASK->t.n_Name));

    /* set the incoming signals */
    SetSignals(t,bits);

    LogSignalSent(t,bits);

    res = t->t_SigBits & t->t_WaitBits;
    if (res)
    {
        DBUGSIGNAL(("SendSignal: waking up task '%s'\n",t->t.n_Name));
        NewReadyTask(t);
    }
    Enable(oldints);

    return 0;
}


/*****************************************************************************/


Err ValidateSignal(int32 bits)
{
    if (bits & ILLEGAL_SIG) return ILLEGALSIGNAL;

    if (bits & SUPER_SIGS)
    {
	if ((CURRENTTASK->t.n_ItemFlags & ITEMNODE_PRIVILEGED) == 0)
	{
#ifdef BUILD_DEBUGGER
            if (strcasecmp(CURRENTTASK->t.n_Name,"Shell Console") == 0)
            {
                /* allow the shell's SendSignal command to go through... */
                return 0;
            }
#endif
	    return BADPRIV;
	}
    }

    return 0;
}

/**
|||	AUTODOC -public -class Kernel -group Signals -name SendSignal
|||	Sends signals to another task.
|||
|||	  Synopsis
|||
|||	    Err SendSignal(Item task, int32 sigMask);
|||
|||	  Description
|||
|||	    This function sends one or more signals to the specified task.
|||
|||	  Arguments
|||
|||	    task
|||	        The item number of the task to send signals to. If this
|||	        parameter is 0, then the signals are sent to the calling task.
|||	        This is sometimes useful to set initial conditions.
|||
|||	    sigMask
|||	        The signals to send.
|||
|||	  Return Value
|||
|||	    The function returns 0 if successful or a negative error code
|||	    for failure. Possible error codes currently include:
|||
|||	    BADPRIV
|||	        The task attempted to send a system signal.
|||
|||	    ILLEGALSIGNAL
|||	        The task attempted to send a signal to a task that was not
|||	        allocated by that task, or bit 31 in the sigMask argument
|||	        (which is reserved) was set.
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
|||	    AllocSignal(), FreeSignal(), GetCurrentSignals(), WaitSignal()
|||
**/

Err externalSignal(Item taskItem, int32 bits)
{
    /* NOT Callable from interrupts with taskItem == 0 */

    Task *t;
    Err  ret;

    if (taskItem == 0)
    {
	t = CURRENTTASK;
        DBUGSIGNAL(("External Signal(%lx,%lx) ct=%lx\n",(uint32)t,bits,(uint32)CURRENTTASK));
    }
    else
    {
	t = (Task *)CheckItem(taskItem,KERNELNODE,TASKNODE);
        DBUGSIGNAL(("External Signal(%lx,%lx) ct=%lx\n",(uint32)t,bits,(uint32)CURRENTTASK));
	if (!t)	return BADITEM;
    }

    ret = ValidateSignal(bits);

    if (ret >= 0)
	ret = internalSignal(t,bits);

    return ret;
}
