/* @(#) mmuexceptions.s 96/08/09 1.4 */

/* exception handlers */

#include <kernel/PPCitem.i>
#include <kernel/PPCnodes.i>
#include <kernel/PPCtask.i>
#include <kernel/PPCkernel.i>
#include <hardware/PPCequ.i>
#include <hardware/PPCMacroequ.i>


	.globl	dataAccessHandler

/****************************************************************************/


	DECFN	iLoadMissHandler

/*	Permissive X-mode I TLB Miss Handler
 *	update the ITLB assuming we can exectue anything that responds.
 */
xitlbmiss_nofence:
	mfspr	r3,IMISS	/* grab the miss address */
	lis	r0,0		/* turn off all the NE bits */
	mtspr	RPA,r0		/* hand payload to IMMU */
	tlbli	r3		/* update entry in ITLB */
	rfi
	sync

	.globl	iLoadMissHandlerEnd
iLoadMissHandlerEnd:

/*****************************************************************************/
/* note, experimentation suggests that gpr4-gpr31 are unusable at this time */

/*#define WATCHDLOADMISS*/

	DECFN	dLoadMissHandler
	/*
	 *	Data load/store miss Handler
	 */
	mfspr	r3,DMISS
	mfcr	r0

#ifdef WATCHDLOADMISS
	mtsprg	1,r4
	mfspr	r1,IBR
	stw	r3,0(r1)
	stw	r2,4(r1)
#endif

	mfsr	r2,SR12
	cmp	r3,r2		/* is the address beyond the end of RAM */
	bgt-	dtlbfault	/* if so, bail */

	mfsr	r2,SR11
	cmp	r3,r2		/* is the address less than RAM start address */
	blt-	dtlbfault	/* if so, bail */

	sub	r2,r3,r2	/* r2 = DMISS - RAMStart */
        rlwinm	r2,r2,17,15,29	/* r2 = (DMISS - RAMStart) / 128K */
	mfsr	r1,SR13		/* get kb_WriteFencePtr */
        lwzx	r2,r2,r1	/* get the 32bits of fence data */

	mtspr	RPA,r2		/* load the fence bits in RPA */
	tlbld	r3		/* use the hw-recommended WAY setting */
#if 0
	mfsrr1	r2
	mtcrf	1,r2		/* not needed iff there's no change to CRF1 */
#endif
	mtcrf	0xff,r0
	rfi
	sync

dtlbfault:		/* This is a real data fault, not a tlb error */
			/* need to restore the machine state */
			/* turn of the TGPRs */
			/* and branch to IBR+0x300 */
	mtcr	r0	/* restore cr */
	mflr	r0
	mfmsr	r1
	li32	r0,MSR_TGPR+MSR_IR+MSR_DR
	xor	r1,r1,r0	/* clear TGPR, set IR and DR */
	mtmsr	r1
	isync

	mtsprg	1,r3		/* similar to bootvectors.s */
	mflr	r3

	lea	r3,dataAccessHandler
	mtlr	r3		
	blr			/* jump to data exception vector */


	.globl	dLoadMissHandlerEnd
dLoadMissHandlerEnd:


/*****************************************************************************/


	DECFN	dStoreMissHandler
	/*
	 *	Data load/store miss Handler
	 */
	mfspr	r3,DMISS
	mfcr	r0

	mfsr	r2,SR12
	cmp	r3,r2		/* is the address beyond the end of RAM */
	bgt-	dtlberror	/* if so, bail */

	mfsr	r2,SR11
	cmp	r3,r2		/* is the address less than RAM start address */
	blt-	dtlberror	/* if so, bail */

	sub	r2,r3,r2	/* r2 = DMISS - RAMStart */
        rlwinm	r2,r2,17,15,29	/* r2 = (DMISS - RAMStart) / 128K */
	mfsr	r1,SR13		/* get kb_WriteFencePtr */
        lwzx	r2,r2,r1	/* get the 32bits of fence data */

dtlbexit:
	mtspr	RPA,r2
	tlbld	r3		/* use the hw-reccomended WAY setting */
#if 0
	mfsrr1	r2
	mtcrf	1,r2		/* not needed iff there's no change to CRF1 */
#endif
	mtcrf	0xff,r0
	rfi
	sync

dtlberror:
	li	r2,0
	b	dtlbexit


	.globl	dStoreMissHandlerEnd
dStoreMissHandlerEnd:


