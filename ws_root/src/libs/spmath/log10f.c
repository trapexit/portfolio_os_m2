/*	@(#) log10f.c 96/02/27 1.3 */

/**
|||	AUTODOC -public -class Math -group Power -name log10f
|||	Base-10 Logarithm
|||
|||	  Synopsis
|||
|||	    float log10f( float x ) 
|||
|||	  Description
|||
|||	    This function computes the base-10 logarithm of the input value.
|||
|||	  Arguments
|||
|||	    x
|||	        input float
|||
|||	  Return Value
|||
|||	    If x is less than or equal to 0, errno is set to EDOM and
|||	    -INF is returned.  Otherwise a float is returned.
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
|||	    logf(), powf()
|||
**/

/*	Implementation module : log10f.c

	Copyright 1989 Diab Data AB, Sweden

	Description :
	Implemention of libm function
	float log10f(float)

	Tables from Hart et al: Computer Aproximations, 1978

	History :
	When	Who	What
	890427	teve	initial
	910708	aem	made this float version
*/

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

#define C1	0.693359375
#define C2	(-2.121944400546905827679e-4)

/**************	Implementation of exported functions ************/

float log10f(float x)
{
    int n;
    float z2, z, c1, c2;
    static const float P[] = {	/* Table: 2661          */
	0.41517739,		/* Prec:  7.68          */
	0.6664407777,		/* Range: [.71,1.4]     */
	0.20000008368e1         /* Order: n=2           */
    };

    if (x <= 0.0) {
#ifdef BUILD_PARANOIA
	printf("log10f: input value of %g is out of domain\n",x);
#endif
	errno = EDOM;
	return -HUGE_VAL_F;
    }
    
    x = frexpf(x, &n);
    if (x < M_SQRT1_2) {
	n--;
	c1 = n;
	z = (x - 0.5)/(x + 0.5);
    } else {
	c1 = n;
	z = (x - 1.0)/(x + 1.0);
    }
    c2 = c1 * C2;		/* put these after division */
    c1 = c1 * C1;
    z2 = z * z;
    x = z * __POL2F(P,z2);
    return M_LOG10E * ((c2 + x) + c1);
}
