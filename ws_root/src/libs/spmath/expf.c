/*	@(#) expf.c 96/02/23 1.4 */

/*	Implementation module : expf.c
	Copyright 1989 Diab Data AB, Sweden
	Tables and algorithms from Cody & Waite

	History :
	When	Who	What
	890427	teve	initial
        910708	aem	made this float version
*/

/**
|||	AUTODOC -public -class Math -group Power -name expf
|||	Exponential Function
|||
|||	  Synopsis
|||
|||	    float expf( float x )
|||
|||	  Description
|||
|||	    This function computes the exponential function of x.
|||
|||	  Arguments
|||
|||	    x
|||	        input value
|||
|||	  Return Value
|||
|||	    Returns e^x.  If x is too large, the result will overflow, 
|||	    errno will be set to ERANGE, and INF will be returned.  If x
|||	    is too small, the result will underflow, errno will be set to
|||	    ERANGE, and 0.0 will be returned.
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
|||	    logf()
|||
**/

/**************	Imported modules ********************************/

#include <math.h>
#include "values.h"  
#include <errno.h>
#include "mathf.h" 
#include "math1f.h"  

#ifdef BUILD_PARANOIA
#include <stdio.h>
#endif


/**************	Implementation of exported functions ************/

float expf(float x)
{
    float x2;
    int sign = 0, n;
    static const float P[] = {    /* Cody and Waite p. 69 */
	0.41602886268e-2,
	0.2499999995,
    }, Q[] = {
	0.49987178778e-1,
	0.5
    };
    
    if (x >= LN_MAXFLOAT) {
#ifdef BUILD_PARANOIA
	printf("expf:  input value %g causes overflow\n",x);
#endif
	errno = ERANGE;
	return HUGE_VAL_F;
    }
    if (x < 0.0) {
	if (x <= LN_MINFLOAT) {
	    errno = ERANGE;
	    return 0.0;
	}
	x = -x;
	sign = 1;
    }
    if (x < X_EPS_F) {
	if (sign) 
	    return 1.0 - x;
	return 1.0 + x;
    }
    x = x * M_LOG2E;
    n = (int) x;
    x = x - n;
    x = x * M_LN2;
    if (sign) {
	x = -x;
	n = -n;
    }
    x2 = x * x;
    x = x * __POL1F(P,x2);
    x = 0.5 + x / (__POL1F(Q,x2) - x);
    return ldexpf(x,n+1);
}
