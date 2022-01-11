/* @(#) rsqrtf.S 96/03/11 1.3 */

/**
|||	AUTODOC -public -class Math -group Power -name rsqrtf
|||	-visible_alias rsqrtff
|||	-visible_alias rsqrtfff
|||	Reciprocal Square Root Functions
|||
|||	  Synopsis
|||
|||	    float rsqrtf( float x )
|||	    float rsqrtff( float x )
|||	    float rsqrtfff( float x )
|||
|||	  Description
|||
|||	    These functions return the reciprocal square root of the input value.
|||	    rsqrtf() is as accurate as possible with single precision floating
|||	    point. rsqrtff() is less accurate, but faster.  rsqrtfff() is a quick
|||	    approximation.
|||
|||	    Function     Speed          Max Error
|||	    --------     -----           -----
|||	    rsqrtf          xx           0.00003%
|||	    rsqrtff         xx           0.04%
|||	    rsqrtfff        xx           1.7%
|||
|||	  Arguments
|||
|||	    x
|||	        input value
|||
|||	  Return Value
|||
|||	    Returns 1.0/sqrtf(x).  If x is less than or equal to 0, then an
|||	    exception will occur.
|||
|||	  Implementation
|||
|||	    Assembly functions in libspmath.  rsqrtfff() is also declared as an
|||	    inline assembly function in math.h
|||
|||	  Associated Files
|||
|||	    <math.h>, libspmath.a
|||
|||	  See Also
|||
|||	    sqrtf(), sqrtff(), sqrtfff()
|||
**/
	.global	rsqrtf

	.text
rsqrtf:
	lis     r3,.L3@ha
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
	fnmsubs	f5, f1, f5, f6
	fmuls	f4, f4, f5
	fmuls	f1, f4, f7
	blr

        .align  2
.L3:	.float	3.0
.L0.5:	.float	0.5
