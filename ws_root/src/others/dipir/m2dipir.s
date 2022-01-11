//
//	@(#) m2dipir.s 96/09/16 1.57
//	Copyright 1995, The 3DO Company
//
//	ASM level DIPIR entrypoint and routines
//

#include "hardware/PPCequ.i"
#include "hardware/PPCMacroequ.i"
#include "dipir.i"
#include "hardware/cde.i"

	.globl	Dipir
	
//=============================================================================
//	Define constants
//=============================================================================

IBR_ADDR   		.equ	DRAMSTART

//=============================================================================
//	Local record definitions
//=============================================================================

.struct DipirFrame
	dpr_sp		.long	1	// Saved SP
	dpr_for_lr	.long	1	// Callee saves LR here 
	dpr_lr		.long	1	// Saved LR
	dpr_pad1	.long	1
	dpr_pad2	.long	1
	dpr_pad3	.long	1
.ends
	
//=============================================================================
//	Declare static data
//=============================================================================

//=============================================================================
//	Macros
//=============================================================================


//=============================================================================
//	Start code
//=============================================================================

	.text

//-------------------------------------------------------------------------
// int32 DipirEntry(uint32 (*QueryROMSysInfo)(), uint32 *DipirPrivateBuf);
//	Initial entrypoint for dipir (from boot/softreset)
//	r3 - Pointer to QueryROMSysInfo function (passed on to c environment)
//	r4 - Pointer to private dipir buffer (NOT passed on to c environment)
//
	DECFN	DipirEntry
	
	// Setup a stack at end of DipirSharedBuf
	li32	r9,DIPIRBUFSTART+DIPIRBUFSIZE
#ifdef PREFILLSTACK
	// Initialize the stack with a known pattern so I can see
	// how much stack is being used while executing dipir.
	mr	r11,r9				// Fill from the top
	li32	r7,DIPIRSTACKSIZE/4
	mtctr	r7				// Fill all but bottom 16 bytes
	li32	r8,0xDEADFACE
StackInitLoop:
	stwu	r8,-4(r11)
	bdnz	StackInitLoop
#endif

	// Setup a stack frame to save some registers.
	mr	r11,r9				// 
	stwu	sp,-DipirFrame(r11)		// Allocate a frame on the stack
	mr	sp,r11				// Set sp to new stack
	mflr	r8				// Save old LR in stack frame
	stw	r8,DipirFrame.dpr_lr(sp)	//

	bl	Dipir				// Do the real work in C

	// Cleanup and exit.

	lwz	r0,DipirFrame.dpr_lr(sp)	// Restore LR (return addr)
	mtlr	r0				//
	lwz	sp,0(sp)			// Restore SP
	blr					// Return

//-------------------------------------------------------------------------
//	Passes r4,r5,r6,r7 as arguments to function at r3.
//	Normally should not return.  If it does return it
//	is up to routine called to follow ABI register calling convention.
//
	DECFN	BranchTo
	mtctr	r3			// pEntrypoint
	mr	r3,r4			// Load address
	mr	r4,r5			// arg 1
	mr	r5,r6			// arg 2
	mr	r6,r7			// arg 3
	bctr				// jump to pEntrypoint

//-------------------------------------------------------------------------
// void ReadTimeBase(TimerState *tm);
//	Returns the 64-bit time base (TB) in TimerState pointed to by r3
//
	DECFN	ReadTimeBase
	mftbu	r11			// read TBU - upper 32 bits
	mftb	r12			// read TBL - lower 32 bits
	mftbu	r5			// read TBU again
	cmp	r5,r11			// see if 'old' == 'new'
	bne-	ReadTimeBase		// loop if a carry occurred
	stw	r11,0(r3)		// return time base TBU and TBL 
	stw	r12,4(r3)
	blr

//-------------------------------------------------------------------------
// void SubTimer(TimerState *tm1, TimerState *tm2, TimerState *result);
//	Subtract two 64-bit numbers.
//	r3 = ptr to first number
//	r4 = ptr to second number
//	r5 = ptr to result
//
	DECFN	SubTimer
	lwz	r7,4(r3)		// get operand 1 [32-63]
	lwz	r8,4(r4)		// get operand 2 [32-63]
	subfco.	r6,r7,r8		// sub operand 1 from operand 2
	lwz	r7,0(r3)		// get operand 1 [0-31]
	lwz	r8,0(r4)		// get operand 2 [0-31]
	subfe.	r7,r7,r8		// sub with carry
	bge	0$			// positive result?
	li	r6,0			// return zero result if < 0
	mr	r7,r6
0$:	stw	r6,4(r5)		// store bits [32-63] of sum
	stw	r7,0(r5)		// store sum[0-31]
	blr

//-------------------------------------------------------------------------
// uint32 GetMSR(void);
//	Return value of MSR (machine state register)
//
	DECFN	GetMSR
	mfmsr	r3			// store MSR into ret reg
	blr

//-------------------------------------------------------------------------
// void SetMSR(uint32 msr);
//	Set MSR (machine state register)
//
	DECFN	SetMSR
	mtmsr	r3			// store r3 to MSR
	isync
	blr

//-------------------------------------------------------------------------
// void SetSPRG0(uint32 sprg0);
//	Set SPRG0 (special purpose register 0)
//
	DECFN	SetSPRG0
	mtsprg	0,r3			// store r3 to SPRG0
	blr

//-------------------------------------------------------------------------
// void FlushDCacheAll(void);
//	Flush the entire data cache
//
#define DCACHE_BLOCK_SIZE	32
#define DCACHE_BLOCK_BITS	5		// log2(DCACHE_BLOCK_SIZE)
#define DCACHE_SIZE		4096
#define DCACHE_FLUSH_BASE	(DRAMSTART)	// some arbitrary address
#define DCACHE_NUM_LINES	(DCACHE_SIZE / DCACHE_BLOCK_SIZE)

	DECFN   FlushDCacheAll
	li32	r3,DCACHE_FLUSH_BASE	// address of where to use for flushing
	li32	r4,DCACHE_NUM_LINES	// number of cache lines to flush
	mtctr   r4
0$:     dcbf    0,r3
	dcbt	0,r3
	addi    r3,r3,DCACHE_BLOCK_SIZE
	bdnz    0$
	sync            // wait till ops complete
	blr

//-------------------------------------------------------------------------
// void FlushDCache(void *addr, uint32 len);
//	Flush a range of addresses from the data cache
//
	DECFN   FlushDCache
	// calculate the number of cache blocks to flush
	addi    r4,r4,(DCACHE_BLOCK_SIZE-1)
	rlwinm  r6,r3,0,32-DCACHE_BLOCK_BITS,31
	add     r4,r4,r6
	rlwinm  r4,r4,32-DCACHE_BLOCK_BITS,DCACHE_BLOCK_BITS,31
	mtctr   r4
0$:     dcbf    0,r3
	addi    r3,r3,DCACHE_BLOCK_SIZE
	bdnz    0$
	sync            // wait till ops complete
	blr

//-------------------------------------------------------------------------
// void InvalidateDCache(void *addr, uint32 len);
//	Invalidate a range of addresses in the data cache
//
	DECFN	InvalidateDCache
	// dcbi doesn't work anyway, so just use FlushDCache.
	b	FlushDCache

//-------------------------------------------------------------------------
// void InvalidateICache(void);
//	Invalidate the instruction cache.
//	Note: don't use r3-r7 because AsmVisaConfigDownload calls
//	InvalidateICache without saving/restoring those registers.
//
	DECFN	InvalidateICache
	mfspr	r8,HID0			// r8 = current value of HID0
	li32	r9,HID_ICFI		
	or	r9,r9,r8		// r9 = (r8 | HID_ICFI)
	mtspr	HID0,r9			// Pulse the ICFI bit (flash invalidate)
	isync
	mtspr	HID0,r8			// restore original value of HID0
	isync
	blr

//-------------------------------------------------------------------------
// uint32 GetIBR(void);
//	Return the IBR (interrupt base register)
//
	DECFN	GetIBR
	mfspr	r3,IBR
	blr

//-------------------------------------------------------------------------
// uint32 Discard(uint32 value);
//	Fool the compiler into not optimizing out a memory read.
//
	DECFN	Discard
	blr

//-------------------------------------------------------------------------
//	CatchException is the code that is copied into the
//	exception vectors at IBR.
//	WARNING: CatchException must be exactly EXCEPTION_SIZE/4 instructions.
//
	.globl	CatchException
	.globl	CatchExceptionEnd
CatchException:
	lea	r3,longjmp		// Return to start of longjmp()
	mtspr	SRR0,r3
	lea	r3,dtmp			// Call longjmp(&dtmp->dt_JmpBuf,1)
	lwz	r3,0(r3)
	addi	r3,r3,DipirTemp.dt_JmpBuf
	li	r4,JMP_ABORT
	rfi
	sync
CatchExceptionEnd:

#ifdef DIPIR_INTERRUPTS

//-------------------------------------------------------------------------
//	CatchInterrupt is the code that is copied into the
//	interrupt vector at IBR.
//	WARNING: CatchInterrupt must be exactly INTERRUPT_SIZE/4 instructions.
//
	.globl	CatchInterrupt
	.globl	CatchInterruptEnd
CatchInterrupt:
	mtsprg1	r3				// save r3 in sprg1
	mfmsr	r3
	ori	r3,r3,MSR_IR|MSR_DR
	mtmsr	r3
	isync
	mflr	r3				// save LR in sprg2
	mtsprg2	r3
	lea	r3,AsmInterruptHandler		// jump to AsmInterruptHandler
	mtlr	r3
	blr
CatchInterruptEnd:


//-------------------------------------------------------------------------
//	Interrupt handler glue code.
//
AsmInterruptHandler:
	mfcr	r3				// save CR in sprg3
	mtsprg3	r3
	lea	r3,dtmp
	lwz	r3,0(r3)
	addi	r3,r3,DipirTemp.dt_InterruptSaveArea
	stmw	r0,RegBlock.rb_GPRs(r3)		// save r0-r31 (r3 is bogus) 
	mfsprg1	r0				// get saved r3
	mfsprg2	r4				// get LR
	mfsprg3	r5				// get CR
	mfctr	r6				// get CTR
	mfxer	r7				// get XER
	mfsrr0	r8				// get SRR0
	mfsrr1	r9				// get SRR1
	stw	r0,RegBlock.rb_GPRs+3*4(r3)	// save r3
	stw	r4,RegBlock.rb_LR(r3)		// save LR
	stw	r5,RegBlock.rb_CR(r3)		// save CR
	stw	r6,RegBlock.rb_CTR(r3)		// save CTR
	stw	r7,RegBlock.rb_XER(r3)		// save XER
	stw	r8,RegBlock.rb_PC(r3)		// save SRR0
	stw	r9,RegBlock.rb_MSR(r3)		// save SRR1

	lea	r3,InterruptHandler		// Call C code to handle intr
	mtctr	r3
	bctrl

	// restore all registers that were potentially trashed 
	lea	r3,dtmp
	lwz	r3,0(r3)
	addi	r3,r3,DipirTemp.dt_InterruptSaveArea
	lwz	r4,RegBlock.rb_LR(r3)		// get saved LR
	lwz	r5,RegBlock.rb_CR(r3)		// get saved CR
	lwz	r6,RegBlock.rb_CTR(r3)		// get saved CTR
	lwz	r7,RegBlock.rb_XER(r3)		// get saved XER
	lwz	r8,RegBlock.rb_PC(r3)		// get saved PC
	lwz	r9,RegBlock.rb_MSR(r3)		// get saved MSR
	mtcrf	0xff,r5				// restore CR
	mtlr	r4				// restore LR
	mtctr	r6				// restore CTR
	mtxer	r7				// restore XER
	mtsrr0	r8				// restore SRR0
	mtsrr1	r9				// restore SRR1
	lwz	r0,RegBlock.rb_GPRs+0*4(r3)	// restore GPRs
	lwz	r1,RegBlock.rb_GPRs+1*4(r3)
	lwz	r2,RegBlock.rb_GPRs+2*4(r3)
	lwz	r4,RegBlock.rb_GPRs+4*4(r3)
	lwz	r5,RegBlock.rb_GPRs+5*4(r3)
	lwz	r6,RegBlock.rb_GPRs+6*4(r3)
	lwz	r7,RegBlock.rb_GPRs+7*4(r3)
	lwz	r8,RegBlock.rb_GPRs+8*4(r3)
	lwz	r9,RegBlock.rb_GPRs+9*4(r3)
	lwz	r10,RegBlock.rb_GPRs+10*4(r3)
	lwz	r11,RegBlock.rb_GPRs+11*4(r3)
	lwz	r12,RegBlock.rb_GPRs+12*4(r3)
	lwz	r13,RegBlock.rb_GPRs+13*4(r3)
	lwz	r3,RegBlock.rb_GPRs+3*4(r3)
	rfi					// return from interrupt
	sync

#endif /* DIPIR_INTERRUPTS */

//-------------------------------------------------------------------------
// void AsmVisaConfigDownload(uint32 cdeBase, 
//		uint32 configReg, uint32 visa_download_bit,
//		uint32 statusReg, uint32 visa_DIP_bit);
//
// Download VISA configuration information safely.
// Due to hardware limitations, we cannot access ROM while the
// download is in progress.  We must copy code to RAM and execute there.
//
//	r3 = CDE base address
//	r4 = offset of card config register
//	r5 = VISA_DOWNLOAD bit (in config register)
//	r6 = offset of card state register
//	r7 = VISA_DIP bit (in state register)

// VisaDownloadCode is the code we will copy to RAM.
VisaDownloadCode:
	or	r4,r3,r4			// r4 = addr of config register
	or	r6,r3,r6			// r6 = addr of state register
	stw	r5,CDE_CLEAR_OFFSET(r4)		// clear VISA_DOWNLOAD
	sync
	stw	r5,0(r4)			// set VISA_DOWNLOAD
	sync
VisaCheckDIP:					// Wait for VISA_DIP == 0
	lwz	r4,0(r6)			// read state register into r4
	and.	r4,r4,r7			// (state & VISA_DIP) ?
	bne	VisaCheckDIP			// nonzero, try again
	blr
VisaDownloadCodeEnd:

VisaCodeSize	.equ	VisaDownloadCodeEnd - VisaDownloadCode

	DECFN	AsmVisaConfigDownload
	mflr	r0				// Save LR in r0
	mfctr	r8				// Save CTR in r8
	// Copy VisaDownloadCode to the stack.
	addi	sp,sp,-VisaCodeSize		// Make room on the stack
	li	r12,VisaCodeSize/4		// Number of words to copy
	mtctr	r12				// 
	lea	r11,VisaDownloadCode		// r11 = source of copy
	addi	r11,r11,-4
	addi	r12,sp,-4			// r12 = destination of copy
	// Now copy the code to the stack.
0$:	lwzu	r10,4(r11)
	stw	r10,4(r12)
	dcbf	0,r12				// Make sure write is flushed
	addi	r12,r12,4
	bdnz	0$
	bl	InvalidateICache		// Toss stale data in I cache
	// Now call the code which has been copied to the stack.
	mtctr	sp
	bctrl
	// Restore registers and exit.
	addi	sp,sp,VisaCodeSize		// Restore SP
	mtctr	r8				// Restore CTR
	mtlr	r0				// Restore LR
	blr

//-------------------------------------------------------------------------
// int32 AsmSoftReset(uint32 cdeBase, uint32 resetReg, uint32 resetBit);
//	Do a Soft Reset.
//
	DECFN	AsmSoftReset
	stwu	sp,-12(sp)
	mflr	r6
	stw	r6,8(sp)			// save lr from C SoftReset()

	or	r4,r4,r3			// r4 = addr of CDE_RESET_CNTL
	stw	r5,0(r4)			// Pound the reset bit;
	sync					// trap to the reset handler.

	lwz	r6,8(sp)
	mtlr	r6
	isync

	// Reset handler returns a value in r3.
	addi	sp,sp,12
	blr


	end
