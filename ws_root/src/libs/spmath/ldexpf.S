/* @(#) ldexpf.S 96/02/26 1.2 */

/**
|||	AUTODOC -public -class Math -group Power -name ldexpf
|||	multiply a float by a power of 2
|||
|||	  Synopsis
|||
|||	    float ldexpf( float num, int exp ) 
|||
|||	  Description
|||
|||	    For an input float 'num', and integer 'exp' returns a float
|||	    'x' such that x = num * 2^exp.  Note that this is not faster
|||	    than multiplying by a constant, but is much  faster than using
|||	    pow().  For example, using x=64.0*y is faster than x=ldexpf(y,6).
|||	    However, x=ldexpf(y,z) is much faster than x=y*powf(z,2.0).
|||
|||	  Arguments
|||
|||	    num
|||	        input float
|||
|||	    exp
|||	        power of 2 to multiply num by
|||
|||	  Return Value
|||
|||	    Returns a floating point number equal to num*2^exp.  An exception
|||	    will occur on overflow. 
|||	    
|||	    
|||	  Implementation
|||
|||	    Assembly functions in libspmath.  
|||
|||	  Caveats
|||
|||	    Assembly programmers: Destroys the contents of registers r4-r5.
|||	    Uses stack pointer (r1) for temporary storage.
|||
|||	  Associated Files
|||
|||	    <math.h>, libspmath.a
|||
|||	  See Also
|||
|||	    frexpf(), modff()
|||
**/

/* we do no error checking here.  If num is not a valid */
/* normalized fp number or exp causes a underflow or overflow */
/* the answer will be incorrect */
	
	.global	ldexpf
	.type	ldexpf,@function
	.text
ldexpf:
	stfs	f1,0(r1)
	lwz	r5,0(r1)
	cmpwi	r5,0
	beqlr-
	extrwi	r4,r5,8,1
	add	r4,r4,r3	
	rlwimi	r5,r4,23,1,8
	stw	r5,0(r1)
	lfs	f1,0(r1)
	blr


