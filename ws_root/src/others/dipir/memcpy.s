/* @(#) memcpy.s 95/09/05 1.1 */

#include <hardware/PPCMacroequ.i>


/*****************************************************************************/


/* void *memcpy(void *dest, const void *source, size_t numBytes);
 *	            r3              r4                 r5
 *
 *    r3   : preserved for return value
 *    r4   : source pointer
 *    r5   : number of bytes left to copy
 *    r6   : destination pointer
 *    r7   : scratch register
 *    r8-r9: intermediate storage of data being copied
 */

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


/*****************************************************************************/


/* Same code as above, but copying in the reverse direction. */

lastToFirst:

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
