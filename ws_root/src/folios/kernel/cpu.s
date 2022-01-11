/* @(#) cpu.s 96/09/03 1.32 */

/* Interrupt control functions, and other simple misc utilities */

#include <kernel/PPCkernel.i>
#include <hardware/PPCequ.i>
#include <hardware/PPCMacroequ.i>


/*****************************************************************************/


/* void Enable(uint32 mask)
 *                 r3
 *
 * Enable/Disable specific interrupts. Bits that are on within the
 * mask turn that interrupt on, while bits that are off turn the interrupt
 * off.
 *
 * Note that M2 currently supports a single interrupt enable bit (MSR_EE).
 * So either everything's on, or everything's off.
 */

	DECFN	Enable
	mfmsr	r4		/* get MSR				*/
	rlwimi	r4,r3,0,16,16	/* store the MSR_EE bit from r3 into r4	*/
	mtmsr	r4		/* set the new MSR			*/
	isync
	blr


/*****************************************************************************/


/* uint32 Disable(void)
 *   r3
 *
 * Disable all interrupts. Returns a mask of the interrupt bits.
 */

	DECFN	Disable
	mfmsr	r3		/* get MSR		*/
	rlwinm	r4,r3,0,17,15	/* clear the MSR_EE bit */
	mtmsr	r4		/* set the new MSR	*/
	isync
	blr


/*****************************************************************************/


	DECFN	EnableICache
	mfspr	r3,HID0
	ori	r4,r3,HID_ICFI		// invalidate by strobing HID_ICFI
	mtspr	HID0,r4
	mtspr	HID0,r3
        ori	r3,r3,HID_ICE		// now turn it on
        sync
	mtspr	HID0,r3
	isync
	blr


/*****************************************************************************/


	DECFN	DisableICache
	mfspr	r3,HID0
        rlwinm  r3,r3,0,17,15   // clear the HID_ICE bit
        sync
	mtspr	HID0,r3
	isync
	blr


/*****************************************************************************/


	DECFN	InvalidateICache
	esa
	mfspr	r3,HID0
	ori	r4,r3,HID_ICFI
	mtspr	HID0,r4
	mtspr	HID0,r3
	dsa
	blr


/*****************************************************************************/


	DECFN	EnableDCache
	mfspr	r3,HID0
	ori	r4,r3,HID_DCI		// invalidate by strobing HID_DCI
	mtspr	HID0,r4
	mtspr	HID0,r3
        ori	r3,r3,HID_DCE		// now turn it on
	sync
	mtspr	HID0,r3
	isync
	blr


/*****************************************************************************/


	DECFN	DisableDCache
	mflr	r10
	bl	FlushDCacheAll
	mtlr	r10
	mfspr	r3,HID0
        rlwinm  r3,r3,0,18,16   // clear the HID_DCE bit
        sync
	mtspr	HID0,r3
	isync
	blr


/*****************************************************************************/


	DECFN	WriteThroughDCache
	mfspr	r3,HID0
	ori	r3,r3,HID_WIMG_WRTHU
	mtspr	HID0,r3
	blr


/*****************************************************************************/


	DECFN	CopyBackDCache
	mfspr	r3,HID0
	rlwinm	r3,r3,0,29,27	// clear the HID_WRTHU bit
	mtspr	HID0,r3
	blr


/*****************************************************************************/


	DECFN	_GetCacheState
	esa
	mfspr	r3,HID0
	dsa
	li32	r4,(HID_DCE|HID_ICE|HID_WIMG_WRTHU|HID_DLOCK|HID_ILOCK)
	and	r3,r3,r4
        blr


/*****************************************************************************/


	DECFN	IsMasterCPU
	esa
	mfsr	r3,SR14			// Grab CPU ID code
	dsa
	cmpwi	r3,0			// which CPU
        bne-	0$

	// on the master
        li	r3,1
        blr

0$:     // on the slave
	li	r3,0
	blr

