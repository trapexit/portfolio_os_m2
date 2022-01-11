/* @(#) cache.s 96/07/24 1.3 */

#include <kernel/PPCtask.i>
#include <hardware/PPCMacroequ.i>


/*****************************************************************************/


/* void DirtyTest(RegBlock *rb, TimerTicks *start, TimerTicks *end, void *buffer)
 *                      r3             r4                 r5             r6
 */
	DECFN	DirtyTest

	// preserve registers we'll trash
	stmw	r13,RegBlock.rb_GPRs+13*4(r3)
	mflr	r9
        stw	r9,RegBlock.rb_LR(r3)

	// setup control registers
	li32	r21,128			// number of cache lines
	mr	r22,r6			// trash buffer
	mr	r28,r4			// start time buffer
	mr	r29,r5			// end time buffer
	mr	r30,r3			// register save area

	// load the bottom 4M in the TLB
        li32	r3,0x40000000
        li32	r4,1024
        mtctr	r4
7$:	lwz	r5,8(r3)
	addi	r3,r3,4096
	bdnz	7$

	// ping the analyzer
        li32	r3,0x40000000
        stw	r3,0(r3)
        dcbst	0,r3

	// dirty the entire cache
	mtctr	r21			// get loop count
	addi	r5,r22,-32		// prepare for stbu
0$:	stbu	r3,32(r5)		// store one byte per cache line
	bdnz	0$			// keep looping until all cache lines done

	// start timing
1$:	mftbu	r23			// read TBU - upper 32 bits
	mftb	r24			// read TBL - lower 32 bits
	mftbu	r10			// read TBU again
	cmp	r10,r23			// see if 'old' == 'new'
	bne-	1$			// loop if a carry occurred

	// do it!
        bl	FlushDCacheAll

	// stop timing
2$:	mftbu	r25			// read TBU - upper 32 bits
	mftb	r26			// read TBL - lower 32 bits
	mftbu	r10			// read TBU again
	cmp	r10,r25			// see if 'old' == 'new'
	bne-	2$			// loop if a carry occurred

	// store start and end times
	stw	r23,0(r28)
	stw	r24,4(r28)
	stw	r25,0(r29)
	stw	r26,4(r29)

	// restore registers
	mr	r3,r30
	lmw	r13,RegBlock.rb_GPRs+13*4(r3)
        lwz	r4,RegBlock.rb_LR(r3)
        mtlr	r4
	blr


/*****************************************************************************/


/* void CleanTest(RegBlock *rb, TimerTicks *start, TimerTicks *end, void *buffer)
 *                      r3             r4                 r5             r6
 */
	DECFN	CleanTest

	// preserve registers we'll trash
	stmw	r13,RegBlock.rb_GPRs+13*4(r3)
	mflr	r9
        stw	r9,RegBlock.rb_LR(r3)

	// setup control registers
	li32	r21,128			// number of cache lines
	mr	r22,r6			// trash buffer
	mr	r28,r4			// start time buffer
	mr	r29,r5			// end time buffer
	mr	r30,r3			// register save area

	// load the bottom 4M in the TLB
        li32	r3,0x40000000
        li32	r4,1024
        mtctr	r4
7$:	lwz	r5,8(r3)
	addi	r3,r3,4096
	bdnz	7$

	// ping the analyzer
        li32	r3,0x40000000
        stw	r3,0(r3)
        dcbst	0,r3

	// load up the entire cache
	mtctr	r21			// get loop count
	addi	r5,r22,-32		// prepare for lbzu
0$:	lbzu	r3,32(r5)		// load one byte per cache line
	bdnz	0$			// keep looping until all cache lines done

	// start timing
1$:	mftbu	r23			// read TBU - upper 32 bits
	mftb	r24			// read TBL - lower 32 bits
	mftbu	r10			// read TBU again
	cmp	r10,r23			// see if 'old' == 'new'
	bne-	1$			// loop if a carry occurred

	// do it!
        bl	FlushDCacheAll

	// stop timing
2$:	mftbu	r25			// read TBU - upper 32 bits
	mftb	r26			// read TBL - lower 32 bits
	mftbu	r10			// read TBU again
	cmp	r10,r25			// see if 'old' == 'new'
	bne-	2$			// loop if a carry occurred

	// store start and end times
	stw	r23,0(r28)
	stw	r24,4(r28)
	stw	r25,0(r29)
	stw	r26,4(r29)

	// restore registers
	mr	r3,r30
	lmw	r13,RegBlock.rb_GPRs+13*4(r3)
        lwz	r4,RegBlock.rb_LR(r3)
        mtlr	r4
	blr
