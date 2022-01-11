/*	@(#) asinf.c 96/02/07 1.2 */
/*	Implementation module : asinf.c

	Copyright 1989 Diab Data AB, Sweden

	Description :
	Implemention of libm function
	float asinf(float)

	Tables from Hart et al: Computer Aproximations, 1978

	History :
	When	Who	What
	890427	teve	initial of asin.c
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

/**************	Implementation of exported functions ************/

float asinf(float x)
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
    
    /* Remove sign */
    if (x < 0.0) {
	sign = 1;
	ax = -x;
    } else {
	ax = x;
    }

    if (ax <= 0.5) {
	x2 = ax*ax;
	if (ax > X_EPS_F) {
	    return x*__POL2F(P1,x2)/__POL2F(Q1,x2);
	}
    } else if (ax < M_SQRT1_2) {
	x2 = x*x;
	return x*__POL2F(P2,x2)/__POL3F(Q2,x2);
    } else if (ax <= 1.0) {
	x2 = 0.5 - 0.5*ax;
	ax = sqrtf(x2);
	x = -2.0 * ax;
	if (ax > X_EPS_F) {
	    x = x*__POL2F(P1,x2)/__POL2F(Q1,x2);
	}
	x += M_PI_2;
	if (sign) 
	    return -x;
	return x;
    } else {
	/* Argument out of range: ERROR */
#ifdef BUILD_PARANOIA
	printf("asinf: input value of %g is invalid.\n",x);
#endif
	errno = EDOM;
	return _NaN_f;
    }

    return x;
}


