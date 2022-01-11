/*	@(#) coshf.c 96/02/23 1.4 */
/*	Implementation module : coshf.c

	Copyright 1989 Diab Data AB, Sweden

	Description :
	Implemention of libm function
	float cosh(float)

	Compute as (exp(x) + exp(-x))/2

	History :
	When	Who	What
	890501	teve	initial of cosh.c
	910708	aem	made this float version
*/


/**
|||	AUTODOC -public -class Math -group Trigonometric -name coshf
|||	-visible_alias sinhf
|||	-visible_alias tanhf
|||	Hyperbolic Functions
|||
|||	  Synopsis
|||
|||	    float coshf( float x )
|||	    float sinhf( float x )
|||	    float tanhf( float x )
|||
|||	  Description
|||
|||	    These functions return the hyperbolic cosine/sine/tangent.
|||
|||	  Arguments
|||
|||	    x
|||	        input value in radians
|||
|||	  Return Value
|||
|||	    Returns a floating point number in the range of the function.
|||	    If the return value is too large to be represented in a float,
|||	    errno will be set to ERANGE and INF will be returned.
|||	  
|||	  Implementation
|||
|||	    C functions in libspmath.  
|||
|||	  Associated Files
|||
|||	    <math.h>, libspmath.a
|||
**/


#include <math.h>
#include <errno.h>
#include "mathf.h" 
#include "values.h"

#define LNV	0.6931610107421875
#define VD2M1	0.13830277879601902638e-4

float coshf(float x)
{
    if (x < 0.0) 
	x = -x;
    if (x > LN_MAXFLOAT) {
	if (x < LN_MAXFLOAT + M_LN2) {
	    x = expf(x - LNV);
	    x = x + x*VD2M1;
	    return x;
	}
	errno = ERANGE;
	return HUGE_VAL_F;
    }
    x = expf(x);
    return 0.5*x + 0.5/x;
}
