	/* @(#) div64.S 96/03/29 1.1 */

	.global __udiv64 
	.type   __udiv64,@function
	.global __div64
	.type   __div64,@function

__udiv64:
	# uses only volatile registers and calls no one, so no stack needed

	addic	r12,r6,-1
	addme	r11,r5
	and	r12,r6,r12
	and	r11,r5,r11
	xor.	r12,r11,r12
	bne	.Lcase2

	# special case 1 - we are dividing by a power of 2
	# this is just a shift right

	cntlzw	r11,r5
	cmpwi	r11,32
	beq	.Lsmall

	# We're shifting by >= 32 bits
	subfic	r11,r11,31
	srw	r4,r3,r11		
	li	r3,0
	blr

.Lsmall:
	# We're shifting by less than 32 bits
	cntlzw	r11,r6	
	subfic	r7,r11,31		# r7 contains the shift count
	addi	r11,r11,1		# 32 - shift count

	slw	r8,r3,r11
	srw	r4,r4,r7
	srw	r3,r3,r7
	or	r4,r4,r8
	blr

.Lcase2:	
	or.	r7,r3,r5
	bne	.Lcase3

	# upper registers are both zero
	# just do a normal divide
	divwu	r4,r4,r6
	blr

.Lcase3:	# default case

	cntlzw	r7,r3
	cmpwi	r7,32
	bne	.L50
	cntlzw	r7,r4
	addi	r7,r7,32
.L50:
	cntlzw	r8,r5
	cmpwi	r8,32
	bne	.L51
	cntlzw	r8,r6
	addi	r8,r8,32

.L51:
	subf	r9,r7,r8
	# now we do a 64-bit shift of r5-r6 left by r9 bits,
	# aligning the two numbers

	cmpwi	r9,32	
	blt	.Lsmallshift

	# shifting by >= 32 bits
	addi	r7,r9,-32
	slw	r5,r6,r7
	li	r6,0
	b	.Lcont

.Lsmallshift:
	# shifting by < 32 bits
	subfic	r7,r9,32
	slw	r5,r5,r9
	srw	r8,r6,r7
	or	r5,r5,r8
	slw	r6,r6,r9

.Lcont:
	# at this point we have
	# r3-r4	numerator
	# r5-r6	shifted denomerator
	# r9	bits shifted
	# r7-r8	where we will put the result
	
	li	r7,0
	li	r8,0
.Lmain:
	cmplw	r3,r5
	blt	.Lnext
	bne	.Lgo
	cmplw	r4,r6
	blt	.Lnext
.Lgo:	# subtract r5-r6 from r3-r4
	subfc	r4,r6,r4
	subfe	r3,r5,r3
	# set a bit in the result registers
	ori	r8,r8,1
.Lnext:
	addic.	r9,r9,-1
	blt	.LDone

	# shift result r7/r8 left by one
	srwi	r11,r8,31
	slwi	r7,r7,1
	slwi	r8,r8,1
	or	r7,r7,r11

	# shift r5/r6 right by one
	slwi	r11,r5,31
	srwi	r6,r6,1
	srwi	r5,r5,1
	or	r6,r6,r11

	b	.Lmain
			
.LDone:
	mr	r4,r8
	mr	r3,r7
	blr


# int64 __div64( int64 j, int64 k) 
# r3,r4 = j 
# r5,r6 = k 
# returns j/k 
# r10 = negate flag (not trashed by __udiv64)

# this function simply sets a flag if the result should be negative then
# calls __udiv64 to do the division
# not necessary to set up a stack frame because r0 and r10 are not trashed

__div64:
	mflr	r0
	li	r10,0
	cmpwi	r3,0
	bge	.LA
	subfic	r4,r4,0
	subfze	r3,r3
	li	r10,1
.LA:
	cmpwi	r5,0
	bge	.LB
	subfic	r6,r6,0
	subfze	r5,r5
	addi	r10,r10,1
.LB:
	bl	__udiv64

	mtlr	r0
	cmpwi	r10,1
	bnelr

	subfic	r4,r4,0
	subfze	r3,r3
	blr





