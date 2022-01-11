/* @(#) task.s 96/09/06 1.54 */

/* PowerPC low-level task swapping code */

#include <kernel/PPCitem.i>
#include <kernel/PPCtask.i>
#include <kernel/PPCkernel.i>
#include <hardware/PPCequ.i>
#include <hardware/PPCMacroequ.i>


/*****************************************************************************/


/* void LoadTaskRegs(Task *t)
 *                      r3
 *
 * Load a task's register set. This function doesn't return to the original
 * caller as execution resumes at the place where the task was when it was
 * swapped out.
 */

	DECFN	LoadTaskRegs

	/* Does this task need a full or a quicky register load? */
	lbz	r0,Task.t_CalledULand(r3)
	cmpi	r0,0
	beq+	fullLoad

	/* The task went to sleep by calling SaveTaskRegs(). We therefore don't
	 * need to restore a complete task running state, and just need to
	 * do the minimum needed to satisfy an EABI function call.
	 */

	/* clear this state flag */
	li	r0,0
	stb	r0,Task.t_CalledULand(r3)

	/* Restore needed misc registers */
	lwz	r4,Task.t_RegisterSave.rb_LR(r3)
	lwz	r5,Task.t_RegisterSave.rb_CR(r3)
	lwz	r6,Task.t_RegisterSave.rb_MSR(r3)

	/* now copy everything we've read into the target registers */
	mtlr	r4
	mtcrf	0xff,r5
	mtmsr	r6

	/* restore needed GPRs */
	lmw	r13,Task.t_RegisterSave.rb_GPRs+13*4(r3)/* restore r13-r31	*/
	lwz	r1,Task.t_RegisterSave.rb_GPRs+1*4(r3)	/* restore sp		*/
	lwz	r2,Task.t_RegisterSave.rb_GPRs+2*4(r3)	/* restore r2		*/

	/* resume task execution */
	blr

fullLoad:
	/* The task was preempted. We must therefore restore a complete
	 * running state.
	 */

	/* Restore all misc registers */
	lwz	r4,Task.t_RegisterSave.rb_CTR(r3)
	lwz	r5,Task.t_RegisterSave.rb_LR(r3)
	lwz	r6,Task.t_RegisterSave.rb_XER(r3)
	lwz	r7,Task.t_RegisterSave.rb_CR(r3)
	lwz	r8,Task.t_RegisterSave.rb_PC(r3)
	lwz	r9,Task.t_RegisterSave.rb_MSR(r3)

	/* now copy everything we've read into the target registers */
	mtctr	r4
	mtlr	r5
	mtxer	r6
	mtcrf	0xff,r7
	mtsrr0	r8					/* restore user's PC	*/
	mtsrr1	r9					/* restore user's MSR	*/

	/* restore the GPRs */
	mr	r1,r3					/* simplify lmw		*/
	lmw	r2,Task.t_RegisterSave.rb_GPRs+2*4(r1)	/* restore r2-r31	*/
	lwz	r0,Task.t_RegisterSave.rb_GPRs+0*4(r1)	/* restore r0		*/
	lwz	r1,Task.t_RegisterSave.rb_GPRs+1*4(r1)	/* restore sp		*/

	/* resume task execution */
	rfi


/*****************************************************************************/


/* uint32 SaveTaskRegs(Task *t)
 *   r3                  r3
 *
 * Save the current CPU state. SaveTaskRegs() is a pretty funky routine. When it
 * is called, it saves the current CPU state into the supplied TCB and returns
 * 0. When the task later gets reactivated, SaveTaskRegs() will return to the
 * same place it was first called from, but this time it returns != 0.
 * Basically, it does a task-level setjmp/longjmp.
 *
 * Since SaveTaskRegs() is called from C code through EABI, the caller
 * anticipates that many registers are gonna get trashed by the function call.
 * We therefore don't need to save these registers into the TCB. We must also
 * clear any outstanding reservations that might exist, to make sure another
 * task won't accidentally use that reservation.
 *
 * On exit:
 *
 *    r3: 0 when saving the registers
 *        != 0 when waking up
 */

	DECFN	SaveTaskRegs

	/* tell LoadTaskRegs() that this task went to sleep of its own volition */
	li	r0,1
	stb	r0,Task.t_CalledULand(r3)

	/* clear any outstanding reservations */
	la	r4,Task.t_RegisterSave.rb_LR(r3)	/* use this as scratch */
	stwcx.	r4,0,r4

	/* preload a cache line for this area to speed things up */
	li	r0,Task.t_RegisterSave.rb_GPRs+20*4
	dcbtst	r3,r0

	/* save all non-volatile registers into the TCB */
	mflr	r0
	mfmsr	r4
	mfcr	r5
	stw	r0,Task.t_RegisterSave.rb_LR(r3)
	stw	r4,Task.t_RegisterSave.rb_MSR(r3)
	stw	r5,Task.t_RegisterSave.rb_CR(r3)
	stw	r1,Task.t_RegisterSave.rb_GPRs+1*4(r3)
	stw	r2,Task.t_RegisterSave.rb_GPRs+2*4(r3)
	stmw	r13,Task.t_RegisterSave.rb_GPRs+13*4(r3)

	/* return 0 to indicate we've just been put to sleep */
	li	r3,0
	blr


/*****************************************************************************/


/* This is where we come when there is no ready task in the system.
 *
 * Once the system is running this routine, the only way that normal
 * execution can resume is if an external interrupt comes in and
 * CheckForTaskSwap() is called and a new task is launched.
 *
 * This code is entered with interrupts disabled.
 */

	DECFN	IdleLoop

	/* set kb_CurrentTask to NULL to indicate we're in the idle loop */
	mfsprg0	r3					/* load KernelBase	*/
	li	r4,0
	stw	r4,KernelBaseRec.kb_CurrentTask(r3)	/* no current task	*/

	/* switch to the interrupt stack */
	lwz	r1,KernelBaseRec.kb_InterruptStack(r3)	/* load interrupt sp */

	/* Enable interrupts since that's the only way a task will
	 * ever get scheduled.
	 */
	mfmsr	r31
	ori	r31,r31,MSR_EE
	mtmsr	r31

	/* Just sit in a loop continuously turning on the processor's
	 * doze mode. Only registers we can use are those that we know
	 * the interrupt handler that'll wake us up won't trash.
	 */
0$:
	sync
	mfmsr	r31
	oris	r31,r31,(MSR_POW>>16)	/* go to sleep */
	mtmsr	r31
	isync
	b	0$


/*****************************************************************************/


	DECFN	IsUser
	esa			/* enter super mode */
	mfspr	r3,ESASRR	/* get old msr bits */
	dsa			/* exit super mode */
	rlwinm	r3,r3,29,31,31	/* check priv bit, shift=32-LOG2(ESASRR_PR) */
	blr			/* rtrn 1 if MSR_PR set (usermode), 0 otherwise */


/*****************************************************************************/


/* void KillSelf(void);
 *
 * This function is used by the kernel to cause a task to kill itself. The
 * kernel just sets things up so that as soon as a task exits supervisor mode,
 * it ends up calling this routine in user-mode.
 *
 * This function may not use any stack space, since it is possible for there
 * not to be a valid stack.
 */

	DECFN	KillSelf
	esa
	mfsprg0	r3
	dsa
	lwz	r3,KernelBaseRec.kb_CurrentTaskItem(r3)
	b	DeleteItem
