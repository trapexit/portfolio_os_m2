//	File:		rsa.s
//
//	Contains:	ASM helper routines for rsa.c
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
//		 <1>	  9/9/94	tlw	first checked in
//
//	To Do:
//

#include "hardware/PPCequ.i"
#include "hardware/PPCMacroequ.i"

//=============================================================================
//	Declare static data
//=============================================================================

//=============================================================================
//	Local record definitions
//=============================================================================

//=============================================================================
//	Start code
//=============================================================================

#if 0
//-------------------------------------------------------------------------
//
//	FirstBit:
//
//	description:	return mask of first bit that is not a zero
//	input:		r3 - number to find high order bit in
//	regs used:	r4 - count of leading zero bits
//			r5 - temp
//	output:		the highest bit that is 1
//	trashes		r4,r5
//
	DECFN	FirstBit

	cntlzw	r4,r3
	lis	r5,0x8000
	srw	r3,r5,r4
	blr
#endif

	
//-------------------------------------------------------------------------
//
//	ShiftAddMod:
//
//	description:	do a multiply (shift and add) of two large nums
//	input:		r3 - pResult (to be shifted, added to, mod'd)
//			r4 - pMod (for ModNum rtn below)
//			r5 - len in words
//			r6 - pAdd (add this to result)
//			r7 - add? (add this time)
//			r8 - not used now
//	regs used:	r9 - *pResult
//			r10 - *pAdd
//			r11 - temp result of shift/add
//			r12 - prev carry
//	output:		none
//	trashes		r3,r4,r5,r6,r7,r8,r9,r10,r11,r12
//
	DECFN	ShiftAddMod

	// Clear the carry flag
	li	r11,0				// set r11 to 0
	addco	r11,r11,r11			// add 0 to 0 to clr carry

	// Init prev shifted word (saved for carry) to 0
	mr	r12,r11

	// Init counter
	mtctr	r5
	
	// Convert word count to byte count and point to end of arrays
	slwi	r11,r5,2
	add	r3,r3,r11
	add	r6,r6,r11

ShiftAddLoop:
	// Get the next result value
	lwzu	r9,-4(r3)			// result = *--pResult

	// Shift the result
	slwi	r11,r9,1
	rlwimi	r11,r12,1,31,31			// Shift prev words bit 0
	mr	r12,r9				// Save for next shift

	// Add to result if requested
	or.	r7,r7,r7
	beq	AfterAdd

	// Get the next addend and add to result
	lwzu	r10,-4(r6)			// add = *--pAdd
	addeo	r11,r11,r10			// add addend and carry

AfterAdd:
	// Store the result back
	stw	r11,0(r3)

	// Decrement counter and loop if necessary
	bdnz	ShiftAddLoop

	// At this point r3 points back to the beginning of pResult, r4 (pMod)
	// has not changed, the count was preserved and r6 and r7 are now
	// available.  So we can fall thru to the Mod function.
	

//-------------------------------------------------------------------------
//
//	ModBigNum:
//
//	description:	do a mod of two large nums
//	input:		r3 - pResult (to be mod'd)
//			r4 - pMod (mod result with this)
//			r5 - len in words
//	regs used:	r9 - *pResult
//			r10 - *pMod
//			r11 - temp
//	output:		none
//	trashes		r3,r4,r5,r6,r7,r8,r9,r10
//
	DECFN	ModBigNum
	
	// Init counter (for the mod comparison)
	mtctr	r5
	b	FirstCompare

CompareLoop:
	addi	r3,r3,4				// pResult++
	addi	r4,r4,4				// pMod++
	
FirstCompare:
	// Get the next result and mod values
	lwz	r9,0(r3)			// result = *pResult
	lwz	r10,0(r4)			// mod = *pMod

	// Compare to see if a mod is needed.
	cmplw	r9,r10

	bltlr					// return if result < mod
	bgt	SubtractNeeded			// mod needed if greater
	bdnz	CompareLoop			// check next word

	// This is slow to have to add this here and then
	// do the calculations below, but its pretty rare
	// when the number and the mod are exactly the same
	addi	r3,r3,4				// pResult++
	addi	r4,r4,4				// pMod++
	
SubtractNeeded:
	// Point to the end of pResult and pMod
	mfctr	r6				// how much more to get to end
	slwi	r11,r6,2
	add	r3,r3,r11
	add	r4,r4,r11

	// Set the carry flag
	li	r11,-1				// set r11 to 0xFFFFFFFF
	addco	r11,r11,r11			// cause overflow to set carry

	// Init counter (for subtract this time)
	mtctr	r5

SubtractLoop:	
	// Get the next result and mod values
	lwzu	r9,-4(r3)			// result = *--pResult
	lwzu	r10,-4(r4)			// mod = *--pMod

	// Subtract mod from result
	subfeo	r9,r10,r9			// subtract mod w/carry
	
	// Store the result back
	stw	r9,0(r3)

	// Decrement counter and loop if necessary
	bdnz	SubtractLoop

	// After the subtract, further mod'ing may be necessary
	// At this point r3 should point to the original pResult
	// and r4 should point to the original pMod.  The count
	// value, r5, should still be its original value.
	b	ModBigNum

		end
