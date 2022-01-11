/* @(#) mp_slave.s 96/08/19 1.8 */

/* slave asm code */

#include <kernel/PPCitem.i>
#include <kernel/PPCnodes.i>
#include <kernel/PPCtask.i>
#include <kernel/PPCkernel.i>
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

	DECFN	SlaveSystemCallHandler

	mfsprg0	r12				// load KernelBase
	mfsprg1	r3				// get original r3
	mfsprg2	r10				// get original lr
	mfsrr1	r8				// get srr1

	// setup the stack frame
	mr	r9,r1					// get current SP
	lwz	r7,KernelBaseRec.kb_SlaveState(r12)	// get SlaveState
	lwz	r1,SlaveState.ss_SuperSP(r7)		// load super SP
	stw	r9,SuperStackFrame.ssf_BackChain(r1)	// save original SP

	// save the system call parameters
	stw	r3,SuperStackFrame.ssf_R3(r1)
	stw	r4,SuperStackFrame.ssf_R4(r1)
	stw	r5,SuperStackFrame.ssf_R5(r1)
	stw	r6,SuperStackFrame.ssf_R6(r1)
	stw	r10,SuperStackFrame.ssf_LR(r1)		// save original LR
	stw	r8,SuperStackFrame.ssf_SRR1(r1)		// save original SRR1

	// determine which function we need to call
	mr	r3,r0				// pass call # to lookup func
	bl	SlaveGetSysCallFunction		// returns funcPtr in r3
	mtctr	r3				// load this for the call

	// reload the registers we need
	lwz	r3,SuperStackFrame.ssf_R3(r1)	// get syscall parms
	lwz	r4,SuperStackFrame.ssf_R4(r1)
	lwz	r5,SuperStackFrame.ssf_R5(r1)
	lwz	r6,SuperStackFrame.ssf_R6(r1)

	// perform the system call with ints turned off and FP ops unavailable
	bctrl

	// restore needed state and return to caller
	lwz	r5,SuperStackFrame.ssf_LR(r1)
	lwz	r7,SuperStackFrame.ssf_SRR1(r1)
	lwz	r1,SuperStackFrame.ssf_BackChain(r1)
	mtlr	r5
	mtmsr	r7
	isync
	blr


/*****************************************************************************/


/* This function is used by some of the slave's exception handlers to setup
 * their general context.
 *
 * On entry:
 *
 *   sprg0   : KernelBase
 *   sprg1   : original value of R3
 *   sprg2   : original value of LR
 *   sprg3   : original value of CR
 *   r3      : scratch
 *   lr      : return PC
 *   <others>: as they were when the exception occured
 *
 * r3 and lr must be restored before the handler does an rfi
 */

	DECFN	SlaveSetupExceptionHandler

	// setup sprg0 here for ease of use later
	lea	r3,KernelBase				// load KernelBase var
	lwz	r3,0(r3)				// load KernelBase ptr
	mtsprg0 r3					// stash here for later

	// If this is a SLAVE_SRESETRECOVER then don't bother saving anything
	lwz	r3,KernelBaseRec.kb_SlaveReq(r3)	// get pointer to action request
	lwz	r3,CPUActionReq.car_Action(r3)		// get requested action
	cmpi	r3,SLAVE_SRESETRECOVER			// is it softreset recovery?
	mfsprg0	r3					// restore KernelBase ptr
	lwz	r3,KernelBaseRec.kb_SlaveState(r3)	// get SlaveState
	beq	SetupSlaveSuperStack			// if softreset recovery skip save-state code

	// Save all registers
	stmw	r0,SlaveState.ss_RegSave.rb_GPRs+0*4(r3)// save r0-r31 (r3 is invalid)
	mfsprg1	r4					// get saved r3
	mfsprg2	r5					// get saved LR
	mfsprg3	r6					// get saved CR
	mfctr	r7					// get CTR
	mfxer	r8					// get XER
	mfsrr0	r9					// get SRR0
	mfsrr1	r10					// get SRR1
	stw	r4,SlaveState.ss_RegSave.rb_GPRs+3*4(r3)// save r3
	stw	r5,SlaveState.ss_RegSave.rb_LR(r3)	// save LR
	stw	r6,SlaveState.ss_RegSave.rb_CR(r3)	// save CR
	stw	r7,SlaveState.ss_RegSave.rb_CTR(r3)	// save CTR
	stw	r8,SlaveState.ss_RegSave.rb_XER(r3)	// save XER
	stw	r9,SlaveState.ss_RegSave.rb_PC(r3)	// save SRR0
	stw	r10,SlaveState.ss_RegSave.rb_MSR(r3)	// save SRR1

SetupSlaveSuperStack:
	// switch to slave super stack
	lwz	r1,SlaveState.ss_SuperSP(r3)		// load sp
	blr


/*****************************************************************************/


	DECFN	SlaveCleanupExceptionHandler

	mfsprg0	r3					// get KernelBase
	lwz	r3,KernelBaseRec.kb_SlaveState(r3)	// get SlaveState

	// restore all registers that were potentially trashed
	lwz	r4,SlaveState.ss_RegSave.rb_CR(r3)
	lwz	r5,SlaveState.ss_RegSave.rb_LR(r3)
	lwz	r6,SlaveState.ss_RegSave.rb_CTR(r3)
	lwz	r7,SlaveState.ss_RegSave.rb_XER(r3)
	lwz	r8,SlaveState.ss_RegSave.rb_PC(r3)
	lwz	r9,SlaveState.ss_RegSave.rb_MSR(r3)
	mtcrf	0xff,r4
	mtlr	r5
	mtctr	r6
	mtxer	r7
	mtsrr0	r8
	mtsrr1	r9
	lwz	r0,SlaveState.ss_RegSave.rb_GPRs+0*4(r3)
	lwz	r1,SlaveState.ss_RegSave.rb_GPRs+1*4(r3)
	lwz	r2,SlaveState.ss_RegSave.rb_GPRs+2*4(r3)

#ifndef BUILD_DEBUGGER
        /* when not running the debugger, only reload registers that could have
         * been trashed by calling C code.
         */
	lwz	r4,SlaveState.ss_RegSave.rb_GPRs+4*4(r3)
	lwz	r5,SlaveState.ss_RegSave.rb_GPRs+5*4(r3)
	lwz	r6,SlaveState.ss_RegSave.rb_GPRs+6*4(r3)
	lwz	r7,SlaveState.ss_RegSave.rb_GPRs+7*4(r3)
	lwz	r8,SlaveState.ss_RegSave.rb_GPRs+8*4(r3)
	lwz	r9,SlaveState.ss_RegSave.rb_GPRs+9*4(r3)
	lwz	r10,SlaveState.ss_RegSave.rb_GPRs+10*4(r3)
	lwz	r11,SlaveState.ss_RegSave.rb_GPRs+11*4(r3)
	lwz	r12,SlaveState.ss_RegSave.rb_GPRs+12*4(r3)
	lwz	r13,SlaveState.ss_RegSave.rb_GPRs+13*4(r3)
#else
	/* when running the debugger, reload all GPRs since the monitor
	 * might have modified some of them following a request from the
	 * debugger.
	 */
	lmw	r4,SlaveState.ss_RegSave.rb_GPRs+4*4(r3)
#endif

	lwz	r3,SlaveState.ss_RegSave.rb_GPRs+3*4(r3)

	// return from interrupt code
	rfi


/*****************************************************************************/


	DECFN	SlaveIdleLoop

	// Enable interrupts
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
	oris	r31,r31,(MSR_POW>>16)	// go to sleep
	mtmsr	r31
	isync
	b	0$
