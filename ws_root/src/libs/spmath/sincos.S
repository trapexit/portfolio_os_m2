/* @(#) sincos.S 96/04/20 1.7 */

/**
|||	AUTODOC -public -class Math -group Trigonometric -name cosf
|||	-visible_alias sinf
|||	Cosine and Sine Functions
|||
|||	  Synopsis
|||
|||	    float cosf( float x )
|||	    float sinf( float x )
|||
|||	  Description
|||
|||	    These functions return the cosine or sine of the input value.
|||	    These are as accurate as possible with single precision floating
|||	    point.
|||
|||	  Arguments
|||
|||	    x
|||	        input in radians
|||
|||	  Implementation
|||
|||	    Assembly functions in libspmath.
|||
|||	  Associated Files
|||
|||	    <math.h>, libspmath.a
|||
|||	  See Also
|||
|||	    tanf(), asinf(), acosf()
|||
**/



/* Based on code by Bill Budge  */

	#offsets into myconstants
	.equ	magic1, 0
	.equ	magic2, 4
	.equ	magic3, 8
	.equ	c4, 12
	.equ	c3, 16
	.equ	c2, 20
	.equ	c1, 24
	.equ	c0, 28
	.equ	s2, 32
	.equ	s1, 36
	.equ	s0, 40

	.global	sinf
	.global cosf
	.type	sinf,@function
	.type	cosf,@function

/* ----------------------------------------------------------------------------

 The first job is to reduce the argument x to the range [-¹/4, ¹/4] so the Taylor
 series converges.
 1.  Use a magic constant to divide by ¹/2
 2.  Use another magic constant to force fractional bits off the right.  Rounding
     mode must be round-to-nearest for correct quadrant selection.
 3.  Move integer part to a gpr, and move quadrant bits to CR for later quadrant adjustment.
 4.  Subtract quadrant from angle to get angle in range.

 This code has fp stalls.
*/

	.text
sinf:
	li	r3,0
	b	_sincos
cosf:

	li	r3,1
_sincos:
	lis	r4, myconstants@h
	ori	r4, r4, myconstants@l
	lfs	f5, magic1(r4)	#get quadrant by dividing by ¹/2
	fmuls	f2, f1, f5
	lfs	f6, magic2(r4)	#force fractional part off right
	fadds	f3, f2, f6	#current mode must be round to nearest
	lfs	f5, magic3(r4)
	fctiwz	f4, f3
	fsubs	f3, f3, f6
	stfiwx	f4, 0, r1
	lwz	r0, 0(r1)
	fnmsubs	f1, f3, f5, f1
	add	r3, r3, r0
	mtcrf	1, r3			#quadrant bits to cr7
	bt	31, taylor_cos		#quadrant adjust (do sin or cos?)

/* Calculate Taylor series approximation using Horner's rule.  There aren't enough */
/* independent instructions to eliminate fp stalls. */

/* Note that both these routines must use f4 to reset LT bit and set SP bit, */
/* because we left it as an integer above */

taylor_sin:
	fmuls	f4, f1, f1	#Taylor series for sin
	lfs	f2, s2(r4)	#interleave coefficient loads to reduce stalls
	lfs	f3, s1(r4)
	fmadds	f2, f4, f2, f3
	lfs	f3, s0(r4)
	fmadds	f2, f4, f2, f3
	fmuls	f2, f2, f4
	fmadds	f1, f1, f2, f1
	bflr	30		#quadrant adjust (negate)
	fneg	f1, f1
	blr

taylor_cos:
	fmuls	f1, f1, f1	#Taylor series for cos
	lfs	f4, c4(r4)
	lfs	f3, c3(r4)
	fmadds	f4, f1, f4, f3
	lfs	f3, c2(r4)
	fmadds	f4, f1, f4, f3
	lfs	f3, c1(r4)
	fmadds	f4, f1, f4, f3
	lfs	f3, c0(r4)
	fmadds	f1, f1, f4, f3
	bflr	30
	fneg	f1, f1
	blr

myconstants:
        .float  0.63661977	# 2/pi
	.float	12582912.0	# 12 *1024 * 1024
	.float	1.5707963	# pi/2
	.float	.0000248016
	.float	-0.001388889
	.float	.041666667
	.float	-0.5
	.float	1.0
	.float	-0.000198413
	.float	.008333333
	.float	-0.166666667
