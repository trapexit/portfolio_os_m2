/* @(#) floor.S 96/03/04 1.4 */

/**
|||	AUTODOC -public -class Math -group Conversion -name floorf
|||	round towards negative infinity
|||
|||	  Synopsis
|||
|||	    float floorf( float x )
|||
|||	  Description
|||
|||	    floorf returns the largest integral value not greater than its
|||	    input.  In other words, it rounds towards negative infinity.
|||	    point.
|||
|||	  Arguments
|||
|||	    x
|||	        input value
|||
|||	  Return Value
|||
|||	     A float equal to the largest integral value not greater than x.
|||
|||	  Implementation
|||
|||	    Assembly function in libspmath.  
|||
|||	  Associated Files
|||
|||	    <math.h>, libspmath.a
|||
|||	  See Also
|||
|||	    ceilf(), fmodf()
|||
**/

	.global	floorf
	.type   floorf,@function

	.text

floorf:
	stfs	f1,0(r1)
	lwz	r3,0(r1)
	
	rlwinm	r4,r3,9,24,31	# extract the exponent into r4
        subfic  r4,r4,150	
        cmpwi   r4,23		
        ble+    .Large

	# It's between -1 and 1, so return 0.0 or -1.0
	cmpwi	r3,0
	blt-	.LSmallNeg
	lis     r4,0
        stw     r4,0(r1)
        lfs     f1,0(r1)
	blr
	
.LSmallNeg:	# negative number - return -1.0
	lis     r4,.Lmin1@ha
        lfs     f1,.Lmin1@l(r4)	
        blr

.Large:
        cmpwi   r4,0
	blelr-	# no fractional part - just return f1 as is	

	# first mask off the fractional part
	li	r5,-1
	slw	r5,r5,r4
	and	r3,r3,r5
	stw	r3,0(r1)
	lfs	f1,0(r1)	
	cmpwi	r3,0	# check the sign
	bgelr

	# it's negative - subtract 1
	lis     r4,.Lmin1@ha
        lfs     f2,.Lmin1@l(r4)
	fadds	f1,f1,f2
	blr

        .align	2
.Lmin1:	.float	-1.0


	


