/*	@(#) acosf.c 96/02/08 1.3 */
/*	Implementation module : acosf.c
	Copyright 1989 Diab Data AB, Sweden

	Tables from Hart et al: Computer Aproximations, 1978

	History :
	When	Who	What
	890427	teve	initial of acos.c
        910708  aem     made this float version
*/
/**
|||	AUTODOC -public -class Math -group Trigonometric -name acosf
|||	-visible_alias asinf
|||	-visible_alias atanf
|||	Inverse Trigonometric Functions
|||
|||	  Synopsis
|||
|||	    float acosf( float x )
|||	    float asinf( float x )
|||	    float atanf( float x )
|||
|||	  Description
|||
|||	    These functions return a number whose cosine/sine/tangent
|||	    is x;
|||
|||	  Arguments
|||
|||	    x
|||	        input value
|||
|||	  Return Value
|||
|||	    Returns a floating point number whose cosine/sine/tangent 
|||	    is equal to x.  For acosf() and asinf(), if the absolute
|||	    value of x is greater than 1.0, errno will be set to
|||	    EDOM and NaN will be returned.
|||	  
|||	  Implementation
|||
|||	    C functions in libspmath.  
|||
|||	  Associated Files
|||
|||	    <math.h>, libspmath.a
|||
|||	  See Also
|||
|||	    sinf(), cosf(), tanf()
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

/**************	Local data, types, fns and macros ***************/



/**************	Implementation of exported functions ************/

float acosf(float x)
{
    int sign = 0;
    float x2, ax;
    static const float P1[] = {		/* Table:	4693	*/
	0.49559947478731,		/* Range:	[0,0.5]	*/
	-0.46145309466645e1,		/* Prec:	8,60	*/
	0.5603629044813127e1		/* Order:	N=2,M=2 */
    }, Q1[] = {				/*		P(x^2)	*/
	0.1e1,				/* arcsin x = x ------	*/
	-0.554846659934668e1,		/*              Q(x^2)	*/
	0.5603629030606043e1
    }, P2[] = {				/* Table:	4724	*/
	-0.177366138666e2,		/* Range:	[0,0.7]	*/
	0.754773929085e2,		/* Prec:	8,07	*/
	-0.6596189741153e2		/* Order:	N=2,M=3 */
    }, Q2[] = {				/*		P(x^2)	*/
	0.1e1,				/* arcsin x = x ------	*/
	-0.272024126625e2,		/*              Q(x^2)	*/
	0.864711037203e2,
	-0.6596189797656e2
    };
    
    if (x < 0.0) {		/* Delete negative sign */
	ax = -x;
	sign = 1;
    } else {
	ax = x;
    }
    
    if (ax <= 0.5) {
	
	/* Use #4693 if in range [0, 0.5) */
	x2 = ax*ax;
	if (ax > X_EPS_F) {
	    x = x*__POL2F(P1,x2)/__POL2F(Q1,x2);
	}
	
    } else if (ax < M_SQRT1_2) {
	
	/* Use #4724 if in [0.5, 0,7) */
	x2 = ax*ax;
	x = x*__POL2F(P2,x2)/__POL3F(Q2,x2);
	
    } else if (ax <= 1.0) {
	
	/* Use #4693 if in [0.7 1.0] and
	 * identity acos(x) = 2 * asin( sqrt(0.5 - 0.5*x) ) */
	
	x2 = 0.5 - 0.5*ax;
	ax = sqrtf(x2);
	x = -2.0F * ax;
	if (ax > X_EPS_F) {
	    x = x*__POL2F(P1,x2)/__POL2F(Q1,x2);
	}
	if (sign) 
	    return M_PI + x;
	return -x;
	
    } else {
	/* Argument out of range: ERROR */
#ifdef BUILD_PARANOIA
	printf("acosf: input value of %g is invalid.\n",x);
#endif
	errno = EDOM;
	return _NaN_f;
    }
    return M_PI_2 - x;
}
