/* @(#) frexpf.S 96/02/27 1.2 */

/**
|||	AUTODOC -public -class Math -group Power -name frexpf
|||	get fractional and exponent parts of a float
|||
|||	  Synopsis
|||
|||	    float frexpf( float x, int *n ) 
|||
|||	  Description
|||
|||	    For an input float 'x', returns a float 'f' and integer 'n'
|||	    such that x = f * 2^n.  
|||
|||	  Arguments
|||
|||	    x
|||	        input float
|||
|||	    n
|||	        pointer to where the integer part is stored
|||
|||	  Return Value
|||
|||	    If x is 0, returns 0.  Otherwise, returns a float between 
|||	    .5 and 1.0.
|||	    
|||	  Implementation
|||
|||	    Assembly function in libspmath.  
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
|||	    ldexpf(), modff()
|||
**/

/* we do no error checking here.  If x is not a valid */
/* normalized fp number the answer will be incorrect */

	.global frexpf
	.type	frexpf,@function

	.text
frexpf:	
	stfs	f1,0(r1)
	lwz	r5,0(r1)
	cmpwi	r5,0
	beqlr-
	extrwi	r4,r5,8,1
	addi	r4,r4,-126
	stw	r4,0(r3)
	li	r4,126		
	rlwimi	r5,r4,23,1,8
	stw	r5,0(r1)
	lfs	f1,0(r1)
	blr

	










