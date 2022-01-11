/* @(#) cache.s 96/09/04 1.23 */

#include <kernel/PPCkernel.i>
#include <hardware/PPCequ.i>
#include <hardware/PPCMacroequ.i>
#include <hardware/bda.i>


/*****************************************************************************/


// Calculate the number of cache lines which cover a given memory range.
// The result is stored in the ctr register and the len register.
//
// Side effects:
// The cache block size is stored in the blockSize register.
// Pointer to kernel base is stored in the kb register.
// The temp register is corrupted.

	.macro
	CacheLines	&addr,&len, &kb,&blockSize,&temp
	cmpwi	&len,0
	beqlr-
	lea	&kb,KernelBase
	lwz	&kb,0(&kb)		// get ptr to KernelBase
	// blockSize = DCacheBlockSize
	// temp = DCacheBlockSize - 1
	// Assume blockSize is power of 2, so temp can also be used
	// as blockMask; that is, (x & (blockSize-1)) == (x % blockSize).
	lwz	&blockSize,KernelBaseRec.kb_DCacheBlockSize(&kb)
	subi	&temp,&blockSize,1
	// len = (len + (blockSize-1) + (addr % blockSize)) / blockSize
	add	&len,&len,&temp		// len += blockSize-1
	and	&temp,&addr,&temp	// temp = addr % blockSize
	add	&len,&len,&temp		// len += temp
	divwu	&len,&len,&blockSize	// len /= blockSize
	mtctr	&len
	.endm


/*****************************************************************************/


/* FIXME: only need this until all boards have a WHOAMI PAL */
#define CHECK_IF_WHOAMI


/* WARNING: this function must preserve r10 because DisableDCache() depends on
 *          it.
 */

	DECFN	FlushDCacheAll
	esa

#ifdef CHECK_IF_WHOAMI
	mfsprg0	r3
	lwz	r3,KernelBaseRec.kb_NumCPUs(r3)
	cmpwi	r3,1
	ble	oldFlush
#endif

	// clear MSR_DR to allow access to the WHOAMI address range
	mfmsr	r3
        rlwinm  r4,r3,0,28,26
        mtmsr	r4

	// prepare for the big event
	mfsprg0	r7					// load KernelBase
	lwz	r6,KernelBaseRec.kb_DCacheBlockSize(r7)	// size of step for flush
	lwz	r5,KernelBaseRec.kb_DCacheNumBlocks(r7)	// number of steps needed
	mtctr	r5					// for loop
	li32	r5,WHOAMI_BASE				// where we read data from

1$:     lwzux	r7,r5,r6
	bdnz	1$					// loop until done

	// reset MSR_DR and scram
	mtmsr	r3
	dsa
	blr

oldFlush:

	// prepare for the big event
        mfsprg0 r8                                      // load KernelBase
        lwz     r9,KernelBaseRec.kb_DCacheBlockSize(r8)	// size of step for flush
        lwz     r5,KernelBaseRec.kb_DCacheNumBlocks(r8)	// number of steps needed
        mtctr   r5                                      // for loop
        lwz     r4,KernelBaseRec.kb_DCacheFlushData(r8)	// where we read data from

1$:     lwzux   r5,r4,r9                                // load one block
        bdnz    1$                                      // loop until done

        sync                                            // wait till ops complete
        dsa
        blr


/*****************************************************************************/


	DECFN	WriteBackDCache

	cmpwi	r5,8192
	bge-	FlushDCacheAll

	// calculate the number of cache blocks to write back
	CacheLines r4,r5, r6,r7,r8
0$:	dcbst	0,r4
	add	r4,r4,r7
	bdnz	0$

	sync			// wait till ops complete
	blr


/*****************************************************************************/


	DECFN	FlushDCache

	cmpwi	r5,8192
	bge-	FlushDCacheAll

	// calculate the number of cache blocks to flush
	CacheLines r4,r5, r6,r7,r8
0$:	dcbf	0,r4
	add	r4,r4,r7
	bdnz	0$

	sync		// wait till ops complete
	blr


/*****************************************************************************/


/* void externalInvalidateDCache(const void *start, uint32 numBytes);
 *                                  r3                  r4
 *
 * Invalidate the given address range from the data cache. The next time
 * this address range is referenced, it is guaranteed to be fetched from
 * main memory. Any data in the cache that hasn't been written out to main
 * memory will be lost.
 */

	DECFN	externalInvalidateDCache

	// use dcbf on 602 2.0 cause dcbi is broken
	mfpvr	r5
	li32	r6,0x50200
	cmpw	r5,r6
	beq-	oldCPU

	// calculate the number of cache blocks to invalidate
	CacheLines r3,r4, r6,r7,r8
0$:	dcbi	0,r3
	add	r3,r3,r7
	bdnz	0$

	sync
	blr

oldCPU:
	// calculate the number of cache blocks to invalidate
	CacheLines r3,r4, r6,r7,r8
0$:	dcbf	0,r3
	add	r3,r3,r7
	bdnz	0$

	sync
	blr
