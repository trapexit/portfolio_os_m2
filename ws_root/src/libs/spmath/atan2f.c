/*	@(#) atan2f.c 96/02/23 1.4 */
/*	Implementation module : atan2f.c
	Copyright 1989 Diab Data AB, Sweden

	Tables from Hart et al: Computer Aproximations, 1978

	History :
	When	Who	What
	890427	teve	initial of atan2.c
        910708	aem	made this float version
*/


/**
|||	AUTODOC -public -class Math -group Trigonometric -name atan2f
|||	arctangent of y/x
|||
|||	  Synopsis
|||
|||	    float atan2f( float y, float x )
|||
|||	  Description
|||
|||	    This function computes the arctangent of y/x.
|||
|||	  Arguments
|||
|||	    x,y
|||	        input floating point values
|||
|||	  Return Value
|||
|||	    Returns the arctangent of y/x in the range of [-pi,+pi].
|||	    If x and y are both 0, returns 0.0.
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
|||	    atanf()
|||
**/
/**************	Imported modules ********************************/

#include <math.h>

float atan2f(float y, float x)
{
    if (x == 0.0 && y == 0.0) 
	return 0.0;

    if (y + x == y) {
	if (y < 0.0) 
	    return -M_PI_2;
	return M_PI_2;
    }
    if (x >= 0.0) 
	return atanf(y/x);
    else 
	if (y < 0.0) 
	    return( atanf(y/x) - M_PI );
    else 
	return( atanf(y/x) + M_PI );
}




