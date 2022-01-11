/* @(#) memset.s 95/09/05 1.1 */

#include <hardware/PPCMacroequ.i>


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
