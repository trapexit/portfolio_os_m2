/* @(#) sqrtf.S 96/03/11 1.3 */

/**
|||	AUTODOC -public -class Math -group Power -name sqrtf
|||	-visible_alias sqrtff
|||	-visible_alias sqrtfff
|||	Square Root Functions
|||
|||	  Synopsis
|||
|||	    float sqrtf( float x )
|||	    float sqrtff( float x )
|||	    float sqrtfff( float x )
|||
|||	  Description
|||
|||	    These functions return the square root of the input value. sqrtf()
|||	    is as accurate as possible with single precision floating point.
|||	    sqrtff() is less accurate, but faster.  sqrtfff() is a quick
|||	    approximation.
|||
|||	    Function    Speed           Max Error
|||	    --------    -----           -----
|||	    sqrtf         xx             0.00002%
|||	    sqrtff        xx             0.04%
|||	    sqrtfff       xx             1.7%
|||
|||	  Arguments
|||
|||	    x
|||	        input value
|||
|||	  Return Value
|||
|||	    Returns the square root.  If x is negative, then an exception
|||	    will occur.
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
|||	    rsqrtf(), rsqrtff(), rsqrtfff()
|||
**/

	.global sqrtf

	.text

	/* float sqrtf( float x ) */
	/* This square root algorithm is accurate to within .00002% */

sqrtf:
	# if x is 0, return now
	lis     r3,.L0@ha
	lfs	f0,.L0@l(r3)
	fcmpu	0,f0,f1
	lis     r3,.L3@ha
	beqlr


	frsqrte	f4, f1		# let y be our approximation of 1.0/sqrt(x)
				# Apply 2 reciproot iterations
				# y = 0.5 * y( 3.0 - x*y*y )
	lfs	f6,.L3@l(r3)
	fmuls	f5, f4, f4	# f5 = y*y
	lis     r3,.L0.5@ha
	fnmsubs	f5, f1, f5, f6	# f5 = 3.0 - x*y*y
	lfs	f7,.L0.5@l(r3)
	fmuls	f4, f4, f5	# f4 = y( 3.0 - x*y*y )
	fmuls	f4, f4, f7	# y = 0.5 * y( 3.0 - x*y*y )

	fmuls	f5, f4, f4	# do it again
	lis     r3,.L1@ha
	fnmsubs	f5, f1, f5, f6
	fmuls	f4, f4, f5
	lfs	f3,.L1@l(r3)
	fmuls	f4, f4, f7
	fmuls	f5,f1,f4	# multiply y by x to get sqrt(x)

	# finally, we apply the secret formula to get even more accuracy!
	# let z = our approximation of sqrt(x), we get a better approximation by
	# z = z+0.5*z*(1-z*y)
	fnmsubs	f4,f4,f5,f3
	fmuls	f4,f4,f5
	fmadds	f1,f7,f4,f5
	blr

        .align  2
.L3:	.float	3.0
.L0:	.float	0.0
.L0.5:	.float	0.5
.L1:	.float	1.0
