/*
 *	@(#) mmu.s 96/02/12 1.9
 *	Copyright 1995, The 3DO Company
 */

#include <hardware/PPCMacroequ.i>
#include <hardware/PPCequ.i>

/*
 *  void LoadDTLBs(uint32 *WriteBits, uint32 memStart){}
 *
 *  This is the context-switch fence pre-loader.
 *  Strategy:  The WriteBits array
 *	is built elsewhere; this code simply loads it into
 *	the TLBs with the following assumptions:
 *	- The TLBs map the memory from 0x4000.0000 to 0x403F.FFFF
 *	- The range 0x4000.0000 - 0x401F.FFFF is setup as WAY 0
 *	- the range 0x4020.0000 - 0x403F.FFFF is setup as WAY 1
 *	- all TLBs are marked valid (!!!)
 *  Since the hw does store the upper 11 bits of the EA into the
 *  upper half of the TLB, this means that when we access a page
 *  in the range 0x404x.xxxx - 0x407x.xxxx, we should take a TLB miss.
 *  The miss handler then blindly loads in the proper TLB, and we
 *  will then take a data access fault if the WE bit was not on.
 */
	DECFN	LoadDTLBs
	li	r10,16			/* loop cnt */
	mfmsr	r8			/* save MSR */
	rlwinm	r6,r8,0,28,25		/* turn off xlation */
	rlwinm	r6,r6,0,17,15		/* turn off external interrupts */
	mtmsr	r6
/*
 *	r3 - ptr to WriteBits
 */
	li32	r5,XMODE_REGIONSIZE
	sub	r5,r4,r5		/* pre-decrement memStart */
	li32	r6,4
	sub	r3,r3,r6		/* pre-decrement WriteBits ptr */
	li32	r9,DCMP_V
	mtspr	DCMP,r9
	mfsrr1	r9
	rlwinm	r9,r9,0,15,13		/* clear WAY bit */
%repeatway1:
	mtsrr1	r9
	mtctr	r10

%nextdtlb:
	/*	XXX it is sub-optimal to do a dcbt for every word, but
	 *	the architecture sez it will be treated as a nop if we
	 *	take a cache hit.  This will in general be faster than
	 *	using the extra instructions to test if we are at a cache
	 *	line boundary at every loop iteration.
	 */
	dcbt	r6,r3			/* pre-load the RPA payload */
	addis	r5,r5,(XMODE_REGIONSIZE>>16)	/* incr to next Region */
	lwzux	r4,r3,r6		/* load the payload from cache */
	mtspr	RPA,r4
	tlbld	r5			/* load the DTLB */
	bdnz-	%nextdtlb		/* bdnz is always perfectly predicted */

	rlwinm.	r0,r9,0,14,14		/* WAY bit set? */
	oris	r9,r9,(SRR1_WAY>>16)	/* set way bit */
	beq	%repeatway1		/* no, repeat for way 1 */

	isync
	mtmsr	r8			/* restore orig MSR */
	blr

/***********************************/
/*
 *	GetIBats(&batarray[8]);
 */
	DECFN	GetIBATs
	mfspr	r4,IBAT0U
	stw	r4,0(r3)
	mfspr	r4,IBAT0L
	stw	r4,4(r3)

	mfspr	r4,IBAT1U
	stw	r4,8(r3)
	mfspr	r4,IBAT1L
	stw	r4,0xc(r3)

	mfspr	r4,IBAT2U
	stw	r4,0x10(r3)
	mfspr	r4,IBAT2L
	stw	r4,0x14(r3)

	mfspr	r4,IBAT3U
	stw	r4,0x18(r3)
	mfspr	r4,IBAT3L
	stw	r4,0x1c(r3)

	blr

/***********************************/
/*
 *	GetDBats(&batarray[8]);
 */
	DECFN	GetDBATs
	mfspr	r4,DBAT0U
	stw	r4,0(r3)
	mfspr	r4,DBAT0L
	stw	r4,4(r3)

	mfspr	r4,DBAT1U
	stw	r4,8(r3)
	mfspr	r4,DBAT1L
	stw	r4,0xc(r3)

	mfspr	r4,DBAT2U
	stw	r4,0x10(r3)
	mfspr	r4,DBAT2L
	stw	r4,0x14(r3)

	mfspr	r4,DBAT3U
	stw	r4,0x18(r3)
	mfspr	r4,DBAT3L
	stw	r4,0x1c(r3)

	blr


/***********************************/
/*
 *	GetSegRegs(&batarray[16]);
 */
	DECFN	GetSegRegs

	mfsr	r4,SR0
	stw	r4,0(r3)
	mfsr	r4,SR1
	stw	r4,4(r3)
	mfsr	r4,SR2
	stw	r4,8(r3)
	mfsr	r4,SR3
	stw	r4,0xc(r3)
	mfsr	r4,SR4
	stw	r4,0x10(r3)
	mfsr	r4,SR5
	stw	r4,0x14(r3)
	mfsr	r4,SR6
	stw	r4,0x18(r3)
	mfsr	r4,SR7
	stw	r4,0x1c(r3)
	mfsr	r4,SR8
	stw	r4,0x20(r3)
	mfsr	r4,SR9
	stw	r4,0x24(r3)
	mfsr	r4,SR10
	stw	r4,0x28(r3)
	mfsr	r4,SR11
	stw	r4,0x2c(r3)
	mfsr	r4,SR12
	stw	r4,0x30(r3)
	mfsr	r4,SR13
	stw	r4,0x34(r3)
	mfsr	r4,SR14
	stw	r4,0x38(r3)
	mfsr	r4,SR15
	stw	r4,0x3c(r3)

	blr


