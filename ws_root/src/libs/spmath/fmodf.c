/* @(#) fmodf.c 96/02/23 1.4 */

/**
|||	AUTODOC -public -class Math -group Conversion -name fmodf
|||	get the remainder of a division
|||
|||	  Synopsis
|||
|||	    float fmodf( float x, float y )
|||
|||	  Description
|||
|||	    fmodf returns the floating point remainder of x/y.
|||
|||	  Arguments
|||
|||	    x,y
|||	        floating point numbers
|||
|||	  Return Value
|||
|||	     The floating point remainder of x/y.  If y is 0.0,
|||	     0.0 is returned.
|||
|||	  Implementation
|||
|||	    C function in libspmath.  
|||
|||	  Associated Files
|||
|||	    <math.h>, libspmath.a
|||
|||	  See Also
|||
|||	    floorf(), fceilf()
|||
**/

#include <math.h>

float fmodf(float x, float y)
{
    float f;
    
    if (y == 0.0 ) 
	return y;
    
    modff(x/y, &f);
    return x - f*y;
}

