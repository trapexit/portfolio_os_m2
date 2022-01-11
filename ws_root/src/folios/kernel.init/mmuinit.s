/*
 *	@(#) mmuinit.s 96/08/19 1.3
 *	Copyright 1995, The 3DO Company
 */

#include <hardware/PPCMacroequ.i>
#include <hardware/PPCequ.i>


/*******************************************************************************/
/*
 *	segreg_init() inits the MMU segment register 0
 *	in an identity fashion.
 */
	DECFN segreg_init

	li32	r3,0x20000000
	li	r4,0
	mtsrin	r3,r4
	blr

	DECFN	XTLBInit

        lis     r4,(DCMP_V>>16)
        mtspr   DCMP,r4
        mtspr   ICMP,r4
 
        li      r4,0
        mtspr   RPA,r4

        li      r4,32
        mtctr   r4
 
nextentry:
        tlbie	r3 

        mfsrr1  r4
        rlwimi  r4,r3,28,14,14	/* Set 1M-2M as WAY 0, 3M-4M as WAY 1 */
        mtsrr1  r4
 
        tlbli	r3 
        tlbld   r3
 
        addis   r3,r3,(XMODE_REGIONSIZE>>16)
	bdnz    nextentry

        isync
	blr
