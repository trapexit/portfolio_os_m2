/* @(#) fabsf.S 96/02/14 1.6 */

/**
|||	AUTODOC -public -class Math -group Conversion -name fabsf
|||	Floating Point Absolute Value
|||
|||	  Synopsis
|||
|||	    float fabsf( float x )
|||
|||	  Description
|||
|||	    This function returns the absolute value of a floating point
|||	    number.  Calling this functions is more efficient than using
|||	    a macro or if statement.
|||
|||	  Arguments
|||
|||	    x
|||	        A floating point number
|||
|||	  Return Value
|||
|||	    Returns x or -x  
|||
|||	  Implementation
|||
|||	    Inline assembly function in math.h and a link library function
|||	    in libspmath.
|||
|||	  Associated Files
|||
|||	    <math.h>, libspmath.a
|||
**/

	.global	fabsf
	.type	fabsf,@function
	.text

fabsf:
	fabs	f1,f1
	blr

