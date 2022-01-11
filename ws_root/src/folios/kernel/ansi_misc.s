/* @(#) ansi_misc.s 96/08/01 1.3 */

#include <hardware/PPCMacroequ.i>
#include <hardware/PPCequ.i>
#include <kernel/PPCsetjmp.i>
#include <kernel/PPCtask.i>
#include <kernel/PPCkernel.i>


/*****************************************************************************/


	DECFN	setjmp

	/* save primary registers into jmpbuf */
	mfcr	r4			/* fetch CR	*/
	stw	r4,JMP_BUF.jb_CR(r3)	/* save CR	*/
	mflr	r4			/* fetch LR	*/
	stw	r4,JMP_BUF.jb_LR(r3)	/* save LR	*/
	stw	r1,JMP_BUF.jb_R1(r3)	/* save SP	*/
	stw	r2,JMP_BUF.jb_R2(r3)	/* small data   */
	stmw	r13,JMP_BUF.jb_GPRs(r3)	/* save r13-r31	*/

	/* if there's no FP context, then skip FP save */

	lea	r4,KernelBase				/* load KernelBase	*/
	lwz	r4,0(r4)
	lwz	r4,KernelBaseRec.kb_CurrentTask(r4)	/* load current TCB   */
	cmpwi	r4,0					/* if 0 then no FP context */
	beq-	0$

	/* Setup the SP/LT bits to claim all registers contain single-precision
	 * value, so the sequence of stfs below will work
	 */

	li32	r7,0
	li32	r8,0xffffffff

	esa
	mfspr	r5,fSP
	mfspr	r6,LT
	mtspr	LT,r7
	mtspr	fSP,r8
	dsa
	stw	r5,JMP_BUF.jb_SP(r3)	// Old SP in r5
	stw	r6,JMP_BUF.jb_LT(r3)	// Old LT in r6

	/* save non-volatile FP registers */
	stfs	f14,JMP_BUF.jb_FPRs+0*4(r3)
	stfs	f15,JMP_BUF.jb_FPRs+1*4(r3)
	stfs	f16,JMP_BUF.jb_FPRs+2*4(r3)
	stfs	f17,JMP_BUF.jb_FPRs+3*4(r3)
	stfs	f18,JMP_BUF.jb_FPRs+4*4(r3)
	stfs	f19,JMP_BUF.jb_FPRs+5*4(r3)
	stfs	f20,JMP_BUF.jb_FPRs+6*4(r3)
	stfs	f21,JMP_BUF.jb_FPRs+7*4(r3)
	stfs	f22,JMP_BUF.jb_FPRs+8*4(r3)
	stfs	f23,JMP_BUF.jb_FPRs+9*4(r3)
	stfs	f24,JMP_BUF.jb_FPRs+10*4(r3)
	stfs	f25,JMP_BUF.jb_FPRs+11*4(r3)
	stfs	f26,JMP_BUF.jb_FPRs+12*4(r3)
	stfs	f27,JMP_BUF.jb_FPRs+13*4(r3)
	stfs	f28,JMP_BUF.jb_FPRs+14*4(r3)
	stfs	f29,JMP_BUF.jb_FPRs+15*4(r3)
	stfs	f30,JMP_BUF.jb_FPRs+16*4(r3)
	stfs	f31,JMP_BUF.jb_FPRs+17*4(r3)

	// Restore the SP/LT bits
	esa
	mtspr	LT,r6
	mtspr	fSP,r5
	dsa

	/* save FPSCR */
	mffs	f0
	li	r5,JMP_BUF.jb_FPSCR
	stfiwx	f0,r3,r5
0$:
	/* return 0 for real call */
	li	r3,0
	blr


/*****************************************************************************/


	DECFN	longjmp

	/* see if the return value is valid */
	cmpi	r4,0		/* trying to return 0?	*/
	bne+	0$		/* no, so skip ahead	*/
	li	r4,1		/* return 1 instead	*/
0$:
	/* if there's no FP context, then skip FP restore */
	lea	r5,KernelBase				/* load KernelBase	*/
	lwz	r5,0(r5)
	lwz	r5,KernelBaseRec.kb_CurrentTask(r5)	/* load current TCB   */
	cmpwi	r5,0					/* if 0 then no FP context */
	beq-	1$

	/* load FPSCR */
	lfs	f0,JMP_BUF.jb_FPSCR(r3)
	lis	r5,0x8000	/* set LT to mark as an integer */
	li	r6,0		/* clear SP so we don't have SP/LT both set, which is illegal */
	esa
	mtspr	LT,r5
	mtspr	fSP,r6
	dsa
	mtfsf	0xff,f0

	/* restore non-volatile FP registers */
	lfs	f14,JMP_BUF.jb_FPRs+0*4(r3)
	lfs	f15,JMP_BUF.jb_FPRs+1*4(r3)
	lfs	f16,JMP_BUF.jb_FPRs+2*4(r3)
	lfs	f17,JMP_BUF.jb_FPRs+3*4(r3)
	lfs	f18,JMP_BUF.jb_FPRs+4*4(r3)
	lfs	f19,JMP_BUF.jb_FPRs+5*4(r3)
	lfs	f20,JMP_BUF.jb_FPRs+6*4(r3)
	lfs	f21,JMP_BUF.jb_FPRs+7*4(r3)
	lfs	f22,JMP_BUF.jb_FPRs+8*4(r3)
	lfs	f23,JMP_BUF.jb_FPRs+9*4(r3)
	lfs	f24,JMP_BUF.jb_FPRs+10*4(r3)
	lfs	f25,JMP_BUF.jb_FPRs+11*4(r3)
	lfs	f26,JMP_BUF.jb_FPRs+12*4(r3)
	lfs	f27,JMP_BUF.jb_FPRs+13*4(r3)
	lfs	f28,JMP_BUF.jb_FPRs+14*4(r3)
	lfs	f29,JMP_BUF.jb_FPRs+15*4(r3)
	lfs	f30,JMP_BUF.jb_FPRs+16*4(r3)
	lfs	f31,JMP_BUF.jb_FPRs+17*4(r3)

	// Restore the SP/LT bits
	lwz	r5,JMP_BUF.jb_SP(r3)
	lwz	r6,JMP_BUF.jb_LT(r3)
	esa
	mtspr	fSP,r5
	mtspr	LT,r6
	dsa
1$:
	lmw	r13,JMP_BUF.jb_GPRs(r3)		/* restore r13-r31	*/
	lwz	r2,JMP_BUF.jb_R2(r3)		/* get saved R2		*/
	lwz	r5,JMP_BUF.jb_LR(r3)		/* get saved LR		*/
	mtlr	r5				/* restore LR		*/
	lwz	r5,JMP_BUF.jb_CR(r3)		/* get saved CR		*/
	mtcrf	0xff,r5				/* restore CR		*/
	lwz	r1,JMP_BUF.jb_R1(r3)		/* get saved SP		*/
	mr	r3,r4                           /* set return value     */
	blr


/*****************************************************************************/


/* void *memmove(void *dest, const void *source, size_t numBytes);
 *	            r3              r4                 r5
 *
 *    r3   : preserved for return value
 *    r4   : source pointer
 *    r5   : number of bytes left to copy
 *    r6   : destination pointer
 *    r7   : scratch register
 *    r8-r9: intermediate storage of data being copied
 */

	DECFN	memmove
	DECFN	memcpy

	cmp	r4,r3			// see if src < dst
	mr	r6,r3			// preserve r3 for result value
	blt	lastToFirst		// if src < dst, do last to first

	cmpi	r5,12			// compare numBytes to 12
	blt-	firstToLastSmallLoop	// if numBytes < 12, do simple byte loop

	rlwinm	r9,r4,0,30,31		// get bottom 2 bits of src ptr

	/* copy the first word of data and word-align the src ptr */
	subfic	r7,r9,4			// get number of bytes until aligned
	lwz	r8,0(r4)		// do one potentially unaligned load
        stw	r8,0(r6)		// do one potentially unaligned store
	rlwinm	r4,r4,0,0,29		// word-align source pointer
	subf	r6,r9,r6		// adjust destination appropriately
	subf	r5,r7,r5		// remove byte count from numBytes

	rlwinm	r7,r5,29,3,31		// numDoubleWords = numBytes / 8
        mtctr	r7			// move count to CTR

0$:	lwz	r8,4(r4)
	lwzu	r9,8(r4)
	stw	r8,4(r6)
	stwu	r9,8(r6)
	bdnz	0$

	rlwinm.	r5,r5,0,29,31		// get new byte count
	beqlr+				// if count == 0, return to caller

	addi	r6,r6,3			// fixup pointer for byte loop
	addi	r4,r4,3			// fixup pointer for byte loop

firstToLastByteLoop:
	mtctr	r5			// move to count register
0$:	lbzu	r8,1(r4)		// load one byte and update
	stbu	r8,1(r6)		// store one byte and update
	bdnz	0$			// if count not 0, do more
        blr				// return to caller

firstToLastSmallLoop:
	cmpi	r5,0
	blelr-				// if numBytes <= 0, return
	addi	r6,r6,-1		// adjust pointer for stbu
	addi	r4,r4,-1		// adjust pointer for stbu
	b	firstToLastByteLoop	// do bytes

lastToFirst:
	/* Same code as above, but copying in the reverse direction. */

	add	r4,r4,r5		// point to end of source
	add	r6,r6,r5		// point to end of dest

	cmpi	r5,12			// compare numBytes to 12
	blt-	lastToFirstSmallLoop	// if numBytes < 12, do simple byte loop

	rlwinm	r7,r4,0,30,31		// get bottom 2 bits of src ptr

	/* copy the first word of data and word-align the src ptr */
	lwz	r8,-4(r4)		// do one potentially unaligned load
        stw	r8,-4(r6)		// do one potentially unaligned store
	rlwinm	r4,r4,0,0,29		// word-align source pointer
	subf	r6,r7,r6		// align destination appropriately
	subf	r5,r7,r5		// remove byte count from numBytes

	rlwinm	r7,r5,29,3,31		// numDoubleWords = numBytes / 8
        mtctr	r7			// move count to CTR

0$:	lwz	r8,-4(r4)
	lwzu	r9,-8(r4)
	stw	r8,-4(r6)
	stwu	r9,-8(r6)
	bdnz	0$

	rlwinm.	r5,r5,0,29,31		// get new byte count
	beqlr+				// if count == 0, return to caller

lastToFirstByteLoop:
	mtctr	r5			// move to count register
0$:	lbzu	r8,-1(r4)		// load one byte and update
	stbu	r8,-1(r6)		// store one byte and update
	bdnz	0$			// if count not 0, do more
        blr				// return to caller

lastToFirstSmallLoop:
	cmpi	r5,0
	blelr-				// if numBytes <= 0, return
	b	lastToFirstByteLoop	// do bytes


/*****************************************************************************/


/* void *memset(void *p, int val, size_t numBytes)
 *    r3           r3       r4          r5
 *
 * This routine will first align p on a 32-byte boundary (a cache block).
 * It'll then proceed to fill memory one cache block at a time.
 *
 *    r3   : preserved for return value
 *    r4   : value to copy into the destination
 *    r5   : number of bytes left to set
 *    r6   : main destination pointer
 *    r7-r9: scratch register
 */

	DECFN	memset

	/* replicate the byte to set into a full word */
        rlwimi	r4,r4,8,16,23		// r4 = r4 | (r4 << 8)
	rlwimi	r4,r4,16,0,15		// r4 = r4 | (r4 << 16)

	cmpi	r5,32			// compare numBytes to 32
	addi	r6,r3,-4		// r6 becomes destination pointer
	blt-	smallLoop		// if numBytes < 32, do small loop

	/* First, we align the pointer to a cache block (32 bytes) */

	rlwinm.	r7,r3,0,27,31		// extract low 5 bits of original pointer
	beq-	cacheBlockLoop		// if already aligned, skip ahead

	subfic	r7,r7,32		// get number of bytes until aligned
	mr	r9,r6			// keep pointer
	add	r6,r6,r7		// bump up to cache block - 4
	addi	r8,r7,3			// round up
	subf	r5,r7,r5		// remove byte count from numBytes
	rlwinm	r8,r8,30,2,31		// number of words to do
	mtctr	r8			// load into count register
0$:	stwu	r4,4(r9)		// store one word and update
	bdnz	0$			// if not done, do one more word

cacheBlockLoop:
	rlwinm.	r7,r5,27,5,31		// numBlocks = numBytes / 32
	beq-	wordLoop		// if less than a cache block, skip
	rlwinm	r5,r5,0,27,31		// get leftover byte count
	mtctr	r7			// move numBlocks to count register

0$:	stw	r4,4(r6)
	stw	r4,8(r6)
	stw	r4,12(r6)
	stw	r4,16(r6)
	stw	r4,20(r6)
	stw	r4,24(r6)
	stw	r4,28(r6)
	stwu	r4,32(r6)
	bdnz	0$

wordLoop:
	rlwinm.	r7,r5,30,2,31		// numWords = numBytes / 4
	beq-	byteLoop		// if less than a word to do, skip
	rlwinm	r5,r5,0,30,31		// get leftover byte count
	mtctr	r7			// load into count register
0$:	stwu	r4,4(r6)		// store one word and update
	bdnz	0$			// if not done, do one more word

byteLoop:
	cmpi	r5,0			// see what's to do
	beqlr+				// if numBytes <= 0, return
	addi	r6,r6,3			// adjust pointer for stbu
	mtctr	r5			// move to count register
0$:	stbu	r4,1(r6)		// set one byte and update
	bdnz	0$			// if count not 0, do more
        blr				// return to caller

smallLoop:
	cmpi	r5,0			// see what's to do
	blelr-				// if numBytes <= 0, return
	b	wordLoop		// go do work


/*****************************************************************************/


/* void memswap(void *m1, void *m2, size_t numBytes);
 *	            r3        r4            r5
 *
 *    r3      : m1 pointer
 *    r4      : m2 pointer
 *    r5      : number of bytes left to copy
 *    r7      : scratch register
 *    r8-r9   : intermediate storage of data being copied
 *    r11-r12 : swap register
 */

	DECFN	memswap

	cmp	r3,r4			// see if m1 < m2
	blt	0$			// if m1 < m2, proceed

	// swap m1 and m2
	mr	r11,r3
	mr	r3,r4
	mr	r4,r11

0$:	cmpi	r5,12			// compare numBytes to 12
	blt-	swap_SmallLoop		// if numBytes < 12, do simple byte loop

	rlwinm	r9,r4,0,30,31		// get bottom 2 bits of m2 ptr

	// swap the first word of data and word-align m2
	subfic	r7,r9,4			// get number of bytes until aligned
	lwz	r11,0(r3)		// do one potentially unaligned load
	lwz	r12,0(r4)		// do one potentially unaligned load
        stw	r12,0(r3)		// do one potentially unaligned store
        stw	r11,0(r4)		// do one potentially unaligned store
	rlwinm	r4,r4,0,0,29		// word-align m2 pointer
	subf	r3,r9,r3		// adjust m1 appropriately
	subf	r5,r7,r5		// remove byte count from numBytes

	rlwinm	r7,r5,29,3,31		// numDoubleWords = numBytes / 8
        mtctr	r7			// move count to CTR

1$:	lwz	r8,4(r3)
	lwz	r9,8(r3)
	lwz	r11,4(r4)
	lwz	r12,8(r4)
	stw	r11,4(r3)
	stwu	r12,8(r3)
	stw	r8,4(r4)
	stwu	r9,8(r4)
	bdnz	1$

	rlwinm.	r5,r5,0,29,31		// get new byte count
	beqlr+				// if count == 0, return to caller

	addi	r3,r3,3			// fixup pointer for byte loop
	addi	r4,r4,3			// fixup pointer for byte loop

swap_ByteLoop:
	mtctr	r5			// move to count register
0$:	lbz	r11,1(r3)		// load one byte
	lbz	r12,1(r4)		// load one byte
	stbu	r12,1(r3)		// store one byte and update
	stbu	r11,1(r4)		// store one byte and update
	bdnz	0$			// if count not 0, do more
        blr				// return to caller

swap_SmallLoop:
	cmpi	r5,0
	blelr-				// if numBytes <= 0, return
	addi	r3,r3,-1		// adjust pointer for stbu
	addi	r4,r4,-1		// adjust pointer for stbu
	b	swap_ByteLoop	// do bytes
