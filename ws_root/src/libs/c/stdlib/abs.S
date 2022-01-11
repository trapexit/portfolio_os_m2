/* @(#) abs.S 96/02/07 1.2 */
/**
|||	AUTODOC -public -class ANSI_C -group Math -name abs
|||	-visible_alias labs
|||	Absolute Value
|||
|||	  Synopsis
|||
|||	    int abs( int i )
|||	    long labs( long i )
|||
|||	  Description
|||
|||	    This function returns the absolute value of an integer (or
|||	    a long integer).
|||
|||	  Arguments
|||
|||	    i
|||	        An integer (or long integer )
|||
|||	  Return Value
|||
|||	    Returns an integer (or long).  
|||
|||	  Implementation
|||
|||	    Inline assembly function in stdlib.h and a link library function
|||	    in libc.
|||
|||	  Associated Files
|||
|||	    <stdlib.h>, libc.a
|||
**/

/*
** int abs(int i)
** {
**    return ((i) < 0 ? -(i) : (i));
** }
*/

	.global	abs
	.global	labs

	.text
abs:
labs:
	cmpwi	r3,0
	bgelr
	neg	r3,r3
	blr

