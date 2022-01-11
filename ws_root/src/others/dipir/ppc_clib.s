//
//	@(#) ppc_clib.s 95/03/06 1.3
//	Copyright 1994, The 3DO Company
//
//	File:		ppc_clib.s
//
//	Contains:	ASM PPC clib routines for dipir
//			and a few PPC specific rtns used by m2dipirutils.c
//
//	Written by:	Tim Wiegman
//
//	Copyright:	(c) 1994 by The 3DO Company. All rights reserved.
//			This material constitutes confidential and proprietary
//			information of the 3DO Company and shall not be used by
//			any Person or for any purpose except as expressly
//			authorized in writing by the 3DO Company.
//
//	Change History (most recent first):
//
//		 <1>	  9/13/94	tlw	first checked in
//
//	To Do:
//

#include <hardware/PPCequ.i>
#include <hardware/PPCMacroequ.i>

//=============================================================================
//	Declare static data
//=============================================================================

//=============================================================================
//	Local record definitions
//=============================================================================

//=============================================================================
//	Macros
//=============================================================================

//=============================================================================
//	Start code
//=============================================================================

//-------------------------------------------------------------------------
//
//	setjmp:
//
//	description:	Save enough state to recover to call pt using longjmp
//			The following are saved at these offsets in jmpbuf:
//			0:	not used
//			4:	SP
//			8:	r2
//			C:	CR
//			10:	LR
//			14:	GPRs 13-31
//	input:		r3 - points to jmpbuf (array)
//	output:		returns 0
//	trashes		r4
//
	DECFN	setjmp

	stw	SP,0x04(r3)		// save stack pointer
	stw	r2,0x08(r3)		// save r2
	mfcr	r4			// fetch condition register
	stw	r4,0x0C(r3)		// save it
	mflr	r4			// fetch link register
	stw	r4,0x10(r3)		// save link register
	stmw	r13,0x14(r3)		// save volatile GPRs

	li	r3,0			// always return 0 for setjmp case
	blr


//-------------------------------------------------------------------------
//
//	longjmp:
//
//	description:	Return executuin/state to previous setjmp call site
//			The following are restored from these jmpbuf offsets:
//			0:	not used
//			4:	SP
//			8:	r2
//			C:	CR
//			10:	LR
//			14:	GPRs 13-31
//	input:		r3 - points to jmpbuf (array)
//			r4 - setjmp's return value
//	output:		returns r4
//	trashes		r5
//
	DECFN	longjmp

	lmw	r13,0x14(r3)		// restore volatile GPRs
	lwz	r5,0x10(r3)		// fetch link register
	mtlr	r5			// restore link register
	lwz	r5,0x0C(r3)		// fetch condition register
	mtcrf	0xff,r5			// restore it
	lwz	r2,0x08(r3)		// restore r2
	lwz	SP,0x04(r3)		// retore stack pointer

	// Return non-zero value in all cases
	mr	r3,r4			// assume return is non-zero
	cmpwi	r4,0			// check ret value for zero (invalid)
	bnelr				// non-zero, return now
	li	r3,1			// return value zero so set it to 1
	blr				// return


//-------------------------------------------------------------------------
//
//	GetSPRG0:
//
//	description: return value of SPRG0 (kernel base)
//	input:		none
//	output:		r3 - SPRG0
//	trashes		r3
//
	DECFN	GetSPRG0

	mfsprg	r3,0		// store SPRG0 into ret reg
	blr			// return


//-------------------------------------------------------------------------
//
//	_dcbf:
//
//	description:	invalidate 32 byte block that may contain addr (r3)
//	input:		r3 - addr to invalidate
//	output:		no changes
//	trashes
//
	DECFN	_dcbf

	dcbf	0,r3
	blr


// EABI compiler support code follows...

	.text
	.globl	_savegpr_14_l
	.globl	_savegpr_15_l
	.globl	_savegpr_16_l
	.globl	_savegpr_17_l
	.globl	_savegpr_18_l
	.globl	_savegpr_19_l
	.globl	_savegpr_20_l
	.globl	_savegpr_21_l
	.globl	_savegpr_22_l
	.globl	_savegpr_23_l
	.globl	_savegpr_24_l
	.globl	_savegpr_25_l
	.globl	_savegpr_26_l
	.globl	_savegpr_27_l
	.globl	_savegpr_28_l
	.globl	_savegpr_29_l
	.globl	_savegpr_30_l
	.globl	_savegpr_31_l

_savegpr_14_l:	stw	r14,-72(r11)
_savegpr_15_l:	stw	r15,-68(r11)
_savegpr_16_l:	stw	r16,-64(r11)
_savegpr_17_l:	stw	r17,-60(r11)
_savegpr_18_l:	stw	r18,-56(r11)
_savegpr_19_l:	stw	r19,-52(r11)
_savegpr_20_l:	stw	r20,-48(r11)
_savegpr_21_l:	stw	r21,-44(r11)
_savegpr_22_l:	stw	r22,-40(r11)
_savegpr_23_l:	stw	r23,-36(r11)
_savegpr_24_l:	stw	r24,-32(r11)
_savegpr_25_l:	stw	r25,-28(r11)
_savegpr_26_l:	stw	r26,-24(r11)
_savegpr_27_l:	stw	r27,-20(r11)
_savegpr_28_l:	stw	r28,-16(r11)
_savegpr_29_l:	stw	r29,-12(r11)
_savegpr_30_l:	stw	r30,-8(r11)
_savegpr_31_l:	stw	r31,-4(r11)
		stw	r0,4(r11)
		blr

	.text
	.globl	_restgpr_14_l
	.globl	_restgpr_15_l
	.globl	_restgpr_16_l
	.globl	_restgpr_17_l
	.globl	_restgpr_18_l
	.globl	_restgpr_19_l
	.globl	_restgpr_20_l
	.globl	_restgpr_21_l
	.globl	_restgpr_22_l
	.globl	_restgpr_23_l
	.globl	_restgpr_24_l
	.globl	_restgpr_25_l
	.globl	_restgpr_26_l
	.globl	_restgpr_27_l
	.globl	_restgpr_28_l
	.globl	_restgpr_29_l
	.globl	_restgpr_30_l
	.globl	_restgpr_31_l

_restgpr_14_l:	lwz	r14,-72(r11)
_restgpr_15_l:	lwz	r15,-68(r11)
_restgpr_16_l:	lwz	r16,-64(r11)
_restgpr_17_l:	lwz	r17,-60(r11)
_restgpr_18_l:	lwz	r18,-56(r11)
_restgpr_19_l:	lwz	r19,-52(r11)
_restgpr_20_l:	lwz	r20,-48(r11)
_restgpr_21_l:	lwz	r21,-44(r11)
_restgpr_22_l:	lwz	r22,-40(r11)
_restgpr_23_l:	lwz	r23,-36(r11)
_restgpr_24_l:	lwz	r24,-32(r11)
_restgpr_25_l:	lwz	r25,-28(r11)
_restgpr_26_l:	lwz	r26,-24(r11)
_restgpr_27_l:	lwz	r0,4(r11)
		lwz	r27,-20(r11)
		mtlr	r0
		lwz	r28,-16(r11)
		lwz	r29,-12(r11)
		lwz	r30,-8(r11)
		lwz	r31,-4(r11)
		mr	sp,r11
		blr
_restgpr_28_l:	lwz	r28,-16(r11)
_restgpr_29_l:	lwz	r29,-12(r11)
_restgpr_30_l:	lwz	r30,-8(r11)
_restgpr_31_l:	lwz	r0,4(r11)
		lwz	r31,-4(r11)
		mtlr	r0
		mr	sp,r11
		blr
