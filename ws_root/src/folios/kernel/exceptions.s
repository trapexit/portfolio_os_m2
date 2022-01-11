/* @(#) exceptions.s 96/10/15 1.128 */

/* exception handlers */

#include <kernel/PPCitem.i>
#include <kernel/PPCnodes.i>
#include <kernel/PPCtask.i>
#include <kernel/PPCkernel.i>
#include <kernel/PPCmonitor.i>
#include <hardware/PPCequ.i>
#include <hardware/PPCMacroequ.i>


/*****************************************************************************/


/* SYSTEM CALL HANDLER
 *
 * On entry:
 *
 *   r0    : upper 16 bits is folio #, lower 16 bits is call #
 *   r3    : <undefined>, must be reloaded from sprg1 before servicing system call
 *   lr    : <undefined>, must be reloaded from sprg2 before performing rfi
 *   sprg0 : KernelBase
 *   sprg1 : original value of r3
 *   sprg2 : original value of lr
 *   others: as-is when sc instruction was executed
 *
 * r3 (once reloaded) through r6 contain the parameters to the system call.
 *
 * This exception handler adopts the same register-preserving conventions as a
 * regular function call. Specifically, r0, r3-r12, XER, CTR, and CR0 may be
 * trashed by this handler. This means that performing an SC instruction
 * invalidates the contents of these registers.
 */

	DECFN	systemCall

	mfsr	r3,SR14			// Grab CPU ID code
	cmpwi	r3,0			// which CPU
        bne-	SlaveSystemCallHandler	// if not CPU 0, go do slave work

	/* get some global state before reenabling interrupts */
	mfsprg0	r12					/* load KernelBase		*/
	mfsprg1	r3					/* get original r3	*/
	mfsprg2	r10					/* get original lr	*/
	mfsrr1	r8					/* get original srr1	*/

	lwz	r12,KernelBaseRec.kb_CurrentTask(r12)	/* load current TCB		*/

	/* disable task switching while we do the system call */
	lbz	r9,Task.t_Forbid(r12)			/* load Forbid count		*/
	addi	r9,r9,1					/* increase Forbid count	*/
	stb	r9,Task.t_Forbid(r12)			/* store new Forbid count	*/

	// Reenable some things that the caller had on
	andi.	r9,r8,(MSR_FE0|MSR_FE1|MSR_FP|MSR_EE|MSR_BE|MSR_SE)	// original bits from user
	mfmsr	r11					// get current msr
	or	r11,r11,r9				// or new bits
	mtmsr	r11

	/* If the caller was previously in supervisor mode, simply create a
	 * new SuperStackFrame. If the caller was previously in user-mode,
	 * switch to the super stack.
	 */

	andi.	r11,r8,MSR_PR			/* check previous super level	 	  */
	beq-	useCurrentStack			/* if previously super, use current stack */

	lwz	r11,Task.t_Flags(r12)		/* check privilege			*/
	andi.	r11,r11,TASK_SINGLE_STACK	/* see if the bit is set		*/
	bne+	useCurrentStack			/* if only using one stack, skip ahead	*/

	/* Switch to the super stack. t_ssp always points to a SuperStackFrame
	 * structure.
	 */

	mr	r9,r1
	lwz	r1,Task.t_ssp(r12)			/* load super SP	*/
	stw	r9,SuperStackFrame.ssf_BackChain(r1)	/* save original SP	*/
	b	setupSysCallParms

useCurrentStack:

	/* just use the existing stack, and create a new frame on it */
        stwu	r1,-SuperStackFrame(r1)

setupSysCallParms:

	/* save the system call parameters */
	stw	r3,SuperStackFrame.ssf_R3(r1)
	stw	r4,SuperStackFrame.ssf_R4(r1)
	stw	r5,SuperStackFrame.ssf_R5(r1)
	stw	r6,SuperStackFrame.ssf_R6(r1)
	stw	r10,SuperStackFrame.ssf_LR(r1)		/* save original LR	*/
	stw	r8,SuperStackFrame.ssf_SRR1(r1)		/* save original SRR1	*/

	/* determine which function we need to call */
	mr	r3,r0			/* pass call # to lookup func	*/
	bl	GetSysCallFunction	/* returns funcPtr in r3	*/
	mtctr	r3			/* load this for the call	*/

	/* reload the registers we need */
	lwz	r3,SuperStackFrame.ssf_R3(r1)	/* get syscall parms	*/
	lwz	r4,SuperStackFrame.ssf_R4(r1)
	lwz	r5,SuperStackFrame.ssf_R5(r1)
	lwz	r6,SuperStackFrame.ssf_R6(r1)

resumeSysCall:

	/* perform the system call */
	bctrl

	/* turn off interrupts again */
	mfmsr	r11				/* get current MSR		*/
	rlwinm	r11,r11,0,17,15			/* Reset MSR_EE			*/
	mtmsr	r11				/* set new MSR			*/

	/* allow task switching once again */
	mfsprg0	r10					/* load KernelBase	*/
	lwz	r12,KernelBaseRec.kb_CurrentTask(r10)	/* load current TCB	*/
	lbz	r11,Task.t_Forbid(r12)			/* load Forbid count	*/
	addi	r11,r11,-1				/* reduce Forbid count	*/
	stb	r11,Task.t_Forbid(r12)			/* store Forbid count	*/

SysCallExit:

	/* check if we need to swap tasks */
	mfsprg0	r10					/* load KernelBase	*/
	lbz	r12,KernelBaseRec.kb_PleaseReschedule(r10) /* check reschedule	*/
	cmpi	r12,0					/* is it 0?		*/
	bne-	NeedTaskSwap				/* if != 0 need to swap	*/

NoTaskSwap:

	/* restore needed state and return to caller */
	lwz	r5,SuperStackFrame.ssf_LR(r1)
	lwz	r7,SuperStackFrame.ssf_SRR1(r1)
	lwz	r1,SuperStackFrame.ssf_BackChain(r1)
	mtlr	r5

	/* restore the state of the MSR_PR/EE bits to what the caller had em. */
	mfmsr	r8				// get current MSR
	rlwimi	r8,r7,0,16,17			// impose MSR_PR/EE from original SRR1
	mtmsr	r8				// do it
	isync

	/* return to sc caller */
	blr

NeedTaskSwap:
	/* We need to do a task swap */

	/* check the forbid count. If it ain't 0, we don't swap */
	cmpi	r11,0
	bne-	NoTaskSwap

	stw	r3,SuperStackFrame.ssf_R3(r1)	/* save sys call return value		*/
	bl	SwapTasks			/* switch to another task		*/
	lwz	r3,SuperStackFrame.ssf_R3(r1)	/* restore return value			*/
        b	SysCallExit			/* we came back, complete sys call	*/


/*****************************************************************************/


/* FLOATING POINT EXCEPTION HANDLER
 *
 * On entry:
 *
 *   sprg0 : KernelBase
 *   sprg1 : original value of r3
 *   sprg2 : original value of lr
 *
 * This routine gets called when an FP unavailable exception occurs.
 * The plan:
 *
 *	- If anyone currently "owns" FPU, save FPU registers into their
 *	  FP save area. If they don't currently have an FP save area,
 *	  it must be allocated.
 *
 *	- Restore FPU registers from the current TCB's FP save area.
 *
 *	- Mark current task as owner
 *
 *	- Enable FPU
 *
 *	- Restart the instruction that caused the exception
 *
 * All GPRs must be preserved.
 */

	DECFN	FPExceptionHandler

	/* save the condition code register */
	mfcr	r3					/* get condition codes */
	mtsprg3	r3					/* save condition codes*/

	/* save some work registers */
	mfsprg0	r3					/* load KernelBase    */
	lwz	r3,KernelBaseRec.kb_CurrentTask(r3)	/* load current TCB   */
	stw	r4,Task.t_RegisterSave.rb_GPRs+4*4(r3)	/* preserve r4	      */
	stw	r5,Task.t_RegisterSave.rb_GPRs+5*4(r3)	/* preserve r5        */

	/* turn on FP operations in the current context */
	mfmsr	r5					/* load the MSR       */
	ori	r5,r5,MSR_FP				/* turn the FP bit on */
	mtmsr	r5					/* set the MSR        */

	mfsprg0	r5					/* load KernelBase    */
	lwz	r4,KernelBaseRec.kb_FPOwner(r5)		/* load cur FPU owner */

	/* if no previous owner, just load up a new register set */
	cmpi	r4,0
	beq-	fp_load_register_set

	/* disable FP operations for the old owner */
	lwz	r3,Task.t_RegisterSave.rb_MSR(r4)
	rlwinm	r3,r3,0,19,17				/* Reset MSR_FP       */
	stw	r3,Task.t_RegisterSave.rb_MSR(r4)

	/* Save FPRs into the old owner's FP save area. */
	la	r3,Task.t_FPRegisterSave(r4)
	bl	SaveFPRegBlock

fp_load_register_set:

	mfsprg0	r4					/* load KernelBase    */
	lwz	r5,KernelBaseRec.kb_CurrentTask(r4)	/* load current TCB   */
	stw	r5,KernelBaseRec.kb_FPOwner(r4)		/* cur task owns FPU  */

	la	r3,Task.t_FPRegisterSave(r5)		/* load fp save area  */
	bl	RestoreFPRegBlock

	/* enable FP operations for the new owner */
	mfsrr1	r4					/* load the task's MSR*/
	ori	r4,r4,MSR_FP				/* turn the FP bit on */
	mtsrr1	r4					/* set the task's MSR */

	/* restore registers and get outta here! */
	mfsprg3	r4					/* get saved CR       */
	mtcrf	0xff,r4					/* restore CR         */
	mfsprg2	r4					/* get saved LR       */
	mtlr	r4					/* restore LR         */

	lwz	r4,Task.t_RegisterSave.rb_GPRs+4*4(r5)	/* get saved r4       */
	lwz	r5,Task.t_RegisterSave.rb_GPRs+5*4(r5)	/* get saved r5       */
	mfsprg1	r3					/* get saved r3       */
	rfi						/* return             */


/*****************************************************************************/


/* This function is used by some of the exception handlers to setup their
 * general context.
 *
 * On entry:
 *
 *   sprg0   : KernelBase
 *   sprg1   : original value of r3
 *   sprg2   : original value of lr
 *   sprg3   : scratch
 *   r3      : scratch
 *   lr      : return PC
 *   <others>: as they were when the exception occured
 *
 * r3 and lr must be restored before the handler does an rfi
 */

SetupExceptionHandler:

	/* preserve CR */
	mfcr	r3
	mtsprg3	r3

	/* are we on the slave? */
	mfsr	r3,SR14				// Grab CPU ID code
	cmpwi	r3,0				// which CPU?
        bne-	SlaveSetupExceptionHandler

	/* get current task pointer */
	mfsprg0	r3					// load KernelBase
	lwz	r3,KernelBaseRec.kb_CurrentTask(r3)	// load current TCB
	/* Now do it again.  This is to workaround an apparent 602 chip bug
	 * which causes the first memory reference in an interrupt routine
	 * (like the one above) to return incorrect data on rare occasions. */
	mfsprg0	r3
	lwz	r3,KernelBaseRec.kb_CurrentTask(r3)

	/* is there a current task? */
	cmpi	r3,0
	beqlr-		// if no current task, then nothing to save...

	/* Save all registers into the current TCB. We use a bunch of different
	 * GPRs and separate the register reads from the memory writes
	 * in an attempt to avoid interlocks in the CPU, and therefore
	 * improve performance.
	 */
	stmw	r0,Task.t_RegisterSave.rb_GPRs+0*4(r3)	/* save r0-r31 (r3 is invalid) */
	mfsprg1	r0					/* get saved r3	*/
	mfsprg2	r4					/* get saved LR	*/
	mfsprg3	r5					/* get saved CR */
	mfctr	r6					/* get CTR	*/
	mfxer	r7					/* get XER	*/
	mfsrr0	r8					/* get SRR0	*/
	mfsrr1	r9					/* get SRR1	*/
	stw	r0,Task.t_RegisterSave.rb_GPRs+3*4(r3)	/* save r3	*/
	stw	r4,Task.t_RegisterSave.rb_LR(r3)	/* save LR	*/
	stw	r5,Task.t_RegisterSave.rb_CR(r3)	/* save CR	*/
	stw	r6,Task.t_RegisterSave.rb_CTR(r3)	/* save CTR	*/
	stw	r7,Task.t_RegisterSave.rb_XER(r3)	/* save XER	*/
	stw	r8,Task.t_RegisterSave.rb_PC(r3)	/* save SRR0	*/
	stw	r9,Task.t_RegisterSave.rb_MSR(r3)	/* save SRR1	*/

	/* switch to interrupt stack */
	mfsprg0	r3					/* load KernelBase	*/
	lwz	r1,KernelBaseRec.kb_InterruptStack(r3)	/* load interrupt sp	*/

	blr


/*****************************************************************************/


/* INTERRUPT HANDLER
 *
 * On entry:
 *
 *   sprg0 : KernelBase
 *   sprg1 : original value of r3
 *   sprg2 : original value of lr
 *   r3    : scratch
 *   lr    : scratch
 *   others: as they were when the exception occured
 */

	DECFN	interruptHandler
	bl	SetupExceptionHandler		// standard setup

	mfsr	r3,SR14				// Grab CPU ID code
	cmpwi	r3,0				// which CPU?
	bne-	0$

	bl	ServiceExceptions
	bl	CheckForTaskSwap		// need to task swap?
	b	CleanupExceptionHandler		// clean up and leave

0$:	bl	ServiceSlaveExceptions
	b	CleanupExceptionHandler		// clean up and leave


/*****************************************************************************/


/* Note: the following exceptions all have the same conditions on entry.
 * On entry:
 *
 *   sprg0 : KernelBase
 *   sprg1 : original value of r3
 *   sprg2 : original value of lr
 *   r3    : scratch
 *   lr    : scratch
 *   others: as they were when the exception occured
 *
 * Also, the following exceptions are all handled the same way:
 *	- do the standard setup
 *	- branch to C code to deal with it. If we could handle
 *	  the exception, we modify the state saved in standard setup.
 *	  If we can not handle the exception, in development mode
 *	  we print out some useful info and kill the task,
 *	  in production mode we Panic.
 *
 * In many cases, we kill the task and startup a new one, thus
 * never returning from the C handler.  Other times, we may
 * recover from the exception and return from the C code.
 */

	DECFN	machineCheckHandler	// machine check exception
	bl	SetupExceptionHandler	// standard setup
	li	r3, CRASH_MachineCheck	// what happened...
	bl	HandleException		// deal with it
	b	CleanupExceptionHandler // cleanup & exit

	DECFN	dataAccessHandler	// data access exception
	bl	SetupExceptionHandler	// standard setup
	li	r3, CRASH_DataAccess	// what happened...
	bl	HandleException		// deal with it
	b	CleanupExceptionHandler // cleanup & exit

	DECFN	instrAccessHandler	// instr access exception
	bl	SetupExceptionHandler	// standard setup
	li	r3, CRASH_InstructionAccess	// what happened...
	bl	HandleException		// deal with it
	b	CleanupExceptionHandler // cleanup & exit

	DECFN	alignmentHandler	// alignment exception
	bl	SetupExceptionHandler	// standard setup
	li	r3, CRASH_Alignment	// what happened...
	bl	HandleException		// deal with it
	b	CleanupExceptionHandler // cleanup & exit

	DECFN	programHandler		// program exception
	bl	SetupExceptionHandler	// standard setup
	li	r3, CRASH_ProgramException	// what happened...
	bl	HandleException		// deal with it
	b	CleanupExceptionHandler // cleanup & exit

	DECFN	ioErrorHandler		// io error exception
	bl	SetupExceptionHandler	// standard setup
	li	r3, CRASH_IOError	// what happened...
	bl	HandleException		// deal with it
	b	CleanupExceptionHandler // cleanup & exit

	DECFN	handle602		// unimplemented 602 instr exception */
	bl	SetupExceptionHandler	// standard setup
	li	r3, CRASH_Bad602	// what happened...
	bl	HandleException		// deal with it
	b	CleanupExceptionHandler // cleanup & exit

	DECFN	traceHandler		// trace exception
	bl	SetupExceptionHandler	// standard setup
	li	r3, CRASH_TraceException// what happened...
	bl	HandleException		// deal with it
	b	CleanupExceptionHandler // cleanup & exit

	DECFN	instrAddrHandler	// instr address exception
	bl	SetupExceptionHandler	// standard setup
	li	r3, CRASH_Misc		// what happened...
	bl	HandleException		// deal with it
	b	CleanupExceptionHandler // cleanup & exit

	DECFN	smiHandler		// system mgmt exception
	bl	SetupExceptionHandler	// standard setup
	li	r3, CRASH_SMI		// what happened...
	bl	HandleException		// deal with it
	b	CleanupExceptionHandler // cleanup & exit


/*****************************************************************************/


/* DECREMENTER EXCEPTION HANDLER
 *
 * This gets control whenever the decrementer counter expires. This means that
 * either the timer device has an IOReq which is done, or that a task's
 * time quantum has just expired and rescheduling needs to happen.
 *
 * In any case, we handle the nitty gritty interrupt-handling details here,
 * and let the higher-level PPCtimer.c code handle the real work that needs
 * to be done.
 *
 * On entry:
 *
 *   sprg0 : KernelBase
 *   sprg1 : original value of r3
 *   sprg2 : original value of lr
 *   r3    : scratch
 *   lr    : scratch
 *   others: as they were when the exception occured
 *
 */

	DECFN	decrementerHandler

	/* do the standard setup */
	bl	SetupExceptionHandler
	bl	ServiceDecrementer		/* execute the timer clients	*/
	bl	CheckForTaskSwap		/* need to swap?		*/
//	b	CleanupExceptionHandler		/* clean up and leave		*/

	/* WARNING: FALLS THROUGH */


/*****************************************************************************/


	/* WARNING: decrementer handler falls directly into this code */

CleanupExceptionHandler:

	/* are we on the slave? */
	mfsr	r3,SR14				// Grab CPU ID code
	cmpwi	r3,0				// which CPU?
	bne-	SlaveCleanupExceptionHandler

	/* OK, so the exception has been serviced and we just want to get
	 * back to whatever was happening at the time the exception occured.
	 *
	 * The entire running state was saved in the current TCB. By virtue of
         * careful coding for the ASM code, and by virtue of the EABI for the C
	 * code, we don't have to restore the entire state at this point. This
	 * is because a boat load of registers have not been altered. We
         * therefore selectively restore only the registers that may have been
         * touched.
	 */

	/* check the current TCB */
	mfsprg0	r3					/* load KernelBase */
	lwz	r3,KernelBaseRec.kb_CurrentTask(r3)	/* load TCB	   */
	cmpi	r3,0
	beq-	idleCleanup

	/* restore all registers that were potentially trashed */
	lwz	r0,Task.t_RegisterSave.rb_CR(r3)
	lwz	r4,Task.t_RegisterSave.rb_LR(r3)
	lwz	r5,Task.t_RegisterSave.rb_CTR(r3)
	lwz	r6,Task.t_RegisterSave.rb_XER(r3)
	lwz	r7,Task.t_RegisterSave.rb_PC(r3)
	lwz	r8,Task.t_RegisterSave.rb_MSR(r3)
	mtcrf	0xff,r0
	mtlr	r4
	mtctr	r5
	mtxer	r6
	mtsrr0	r7
	mtsrr1	r8
	lwz	r0,Task.t_RegisterSave.rb_GPRs+0*4(r3)
	lwz	r1,Task.t_RegisterSave.rb_GPRs+1*4(r3)
	lwz	r2,Task.t_RegisterSave.rb_GPRs+2*4(r3)

#ifndef BUILD_DEBUGGER
        /* when not running the debugger, only reload registers that could have
         * been trashed by calling C code.
         */
	lwz	r4,Task.t_RegisterSave.rb_GPRs+4*4(r3)
	lwz	r5,Task.t_RegisterSave.rb_GPRs+5*4(r3)
	lwz	r6,Task.t_RegisterSave.rb_GPRs+6*4(r3)
	lwz	r7,Task.t_RegisterSave.rb_GPRs+7*4(r3)
	lwz	r8,Task.t_RegisterSave.rb_GPRs+8*4(r3)
	lwz	r9,Task.t_RegisterSave.rb_GPRs+9*4(r3)
	lwz	r10,Task.t_RegisterSave.rb_GPRs+10*4(r3)
	lwz	r11,Task.t_RegisterSave.rb_GPRs+11*4(r3)
	lwz	r12,Task.t_RegisterSave.rb_GPRs+12*4(r3)
	lwz	r13,Task.t_RegisterSave.rb_GPRs+13*4(r3)
#else
	/* when running the debugger, reload all GPRs since the monitor
	 * might have modified some of them following a request from the
	 * debugger.
	 */
	lmw	r4,Task.t_RegisterSave.rb_GPRs+4*4(r3)
#endif

	lwz	r3,Task.t_RegisterSave.rb_GPRs+3*4(r3)

idleCleanup:

	/* return from interrupt code */
	rfi


/*****************************************************************************/


/* Save Floating Point Registers
 *
 * On entry:
 *
 *	r3: Address of FPRegBlock Save Area
 *
 * On exit:
 *
 *	r3: Address of FPRegBlock Save Area
 *	r4: Trashed
 *
 */
	DECFN	SaveFPRegBlock

	/* Save the SP/LT bits off, since we'll be mucking with them */

	mfspr	r4,fSP
	stw	r4,FPRegBlock.fprb_SP(r3)
	mfspr	r4,LT
	stw	r4,FPRegBlock.fprb_LT(r3)

	/* Setup SP/LT to claim that all of our data is	single-precision
	 * FP values, so that the stfs sequence below will work.
	 */

	li32	r4,0
	mtspr	LT,r4
	li32	r4,0xffffffff
	mtspr	fSP,r4

	stfs	f0,FPRegBlock.fprb_FPRs+0*4(r3)
	stfs	f1,FPRegBlock.fprb_FPRs+1*4(r3)
	stfs	f2,FPRegBlock.fprb_FPRs+2*4(r3)
	stfs	f3,FPRegBlock.fprb_FPRs+3*4(r3)
	stfs	f4,FPRegBlock.fprb_FPRs+4*4(r3)
	stfs	f5,FPRegBlock.fprb_FPRs+5*4(r3)
	stfs	f6,FPRegBlock.fprb_FPRs+6*4(r3)
	stfs	f7,FPRegBlock.fprb_FPRs+7*4(r3)
	stfs	f8,FPRegBlock.fprb_FPRs+8*4(r3)
	stfs	f9,FPRegBlock.fprb_FPRs+9*4(r3)
	stfs	f10,FPRegBlock.fprb_FPRs+10*4(r3)
	stfs	f11,FPRegBlock.fprb_FPRs+11*4(r3)
	stfs	f12,FPRegBlock.fprb_FPRs+12*4(r3)
	stfs	f13,FPRegBlock.fprb_FPRs+13*4(r3)
	stfs	f14,FPRegBlock.fprb_FPRs+14*4(r3)
	stfs	f15,FPRegBlock.fprb_FPRs+15*4(r3)
	stfs	f16,FPRegBlock.fprb_FPRs+16*4(r3)
	stfs	f17,FPRegBlock.fprb_FPRs+17*4(r3)
	stfs	f18,FPRegBlock.fprb_FPRs+18*4(r3)
	stfs	f19,FPRegBlock.fprb_FPRs+19*4(r3)
	stfs	f20,FPRegBlock.fprb_FPRs+20*4(r3)
	stfs	f21,FPRegBlock.fprb_FPRs+21*4(r3)
	stfs	f22,FPRegBlock.fprb_FPRs+22*4(r3)
	stfs	f23,FPRegBlock.fprb_FPRs+23*4(r3)
	stfs	f24,FPRegBlock.fprb_FPRs+24*4(r3)
	stfs	f25,FPRegBlock.fprb_FPRs+25*4(r3)
	stfs	f26,FPRegBlock.fprb_FPRs+26*4(r3)
	stfs	f27,FPRegBlock.fprb_FPRs+27*4(r3)
	stfs	f28,FPRegBlock.fprb_FPRs+28*4(r3)
	stfs	f29,FPRegBlock.fprb_FPRs+29*4(r3)
	stfs	f30,FPRegBlock.fprb_FPRs+30*4(r3)
	stfs	f31,FPRegBlock.fprb_FPRs+31*4(r3)

	/* store old owner's FPSCR into the save area */
	mffs	f0
	li	r4,FPRegBlock.fprb_FPSCR
	stfiwx	f0,r3,r4

	blr


/*****************************************************************************/


/*
 * Restore Floating Point Registers
 *
 * On entry:
 *
 *	r3: Address Of Area To Restore FPRegBlock From
 *
 * On exit:
 *
 *	r3: Address of FPRegBlock Saved Area
 *	r4: Trashed
 *
 */
	DECFN	RestoreFPRegBlock

	/* load new owner's FPSCR */
	lfs	f0,FPRegBlock.fprb_FPSCR(r3)
	lis	r4,0x8000	/* set LT to mark as an integer */
	mtspr	LT,r4
	li	r4,0		/* clear SP so we don't have SP/LT both set, which is illegal */
	mtspr	fSP,r4
	mtfsf	0xff,f0

	/* Load FPRs from the new owner's FP save area.	*/

	lfs	f0,FPRegBlock.fprb_FPRs+0*4(r3)
	lfs	f1,FPRegBlock.fprb_FPRs+1*4(r3)
	lfs	f2,FPRegBlock.fprb_FPRs+2*4(r3)
	lfs	f3,FPRegBlock.fprb_FPRs+3*4(r3)
	lfs	f4,FPRegBlock.fprb_FPRs+4*4(r3)
	lfs	f5,FPRegBlock.fprb_FPRs+5*4(r3)
	lfs	f6,FPRegBlock.fprb_FPRs+6*4(r3)
	lfs	f7,FPRegBlock.fprb_FPRs+7*4(r3)
	lfs	f8,FPRegBlock.fprb_FPRs+8*4(r3)
	lfs	f9,FPRegBlock.fprb_FPRs+9*4(r3)
	lfs	f10,FPRegBlock.fprb_FPRs+10*4(r3)
	lfs	f11,FPRegBlock.fprb_FPRs+11*4(r3)
	lfs	f12,FPRegBlock.fprb_FPRs+12*4(r3)
	lfs	f13,FPRegBlock.fprb_FPRs+13*4(r3)
	lfs	f14,FPRegBlock.fprb_FPRs+14*4(r3)
	lfs	f15,FPRegBlock.fprb_FPRs+15*4(r3)
	lfs	f16,FPRegBlock.fprb_FPRs+16*4(r3)
	lfs	f17,FPRegBlock.fprb_FPRs+17*4(r3)
	lfs	f18,FPRegBlock.fprb_FPRs+18*4(r3)
	lfs	f19,FPRegBlock.fprb_FPRs+19*4(r3)
	lfs	f20,FPRegBlock.fprb_FPRs+20*4(r3)
	lfs	f21,FPRegBlock.fprb_FPRs+21*4(r3)
	lfs	f22,FPRegBlock.fprb_FPRs+22*4(r3)
	lfs	f23,FPRegBlock.fprb_FPRs+23*4(r3)
	lfs	f24,FPRegBlock.fprb_FPRs+24*4(r3)
	lfs	f25,FPRegBlock.fprb_FPRs+25*4(r3)
	lfs	f26,FPRegBlock.fprb_FPRs+26*4(r3)
	lfs	f27,FPRegBlock.fprb_FPRs+27*4(r3)
	lfs	f28,FPRegBlock.fprb_FPRs+28*4(r3)
	lfs	f29,FPRegBlock.fprb_FPRs+29*4(r3)
	lfs	f30,FPRegBlock.fprb_FPRs+30*4(r3)
	lfs	f31,FPRegBlock.fprb_FPRs+31*4(r3)

	/* Load user's SP/LT bits */

	lwz	r4,FPRegBlock.fprb_SP(r3)
	mtspr	fSP,r4
	lwz	r4,FPRegBlock.fprb_LT(r3)
	mtspr	LT,r4

	blr
