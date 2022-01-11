/*	@(#) tanhf.c 96/02/07 1.2 */
/*	Implementation module : tanhf.c

	Copyright 1989 Diab Data AB, Sweden

	Description :
	Implemention of libm function
	float tanhf(float)

	Tables & algorithm from W. Cody & W. Waite 1980

	History :
	When	Who	What
	890503	teve	initial of tanh.c
        910708  aem     made this float version
*/

/**************	Imported modules ********************************/

#include <math.h>
#include "values.h"
#include "mathf.h"
#include "math1f.h"
#include <errno.h>

/**************	Local data, types, fns and macros ***************/

#define XBIG ((M_LN2+LN_MAXFLOAT)/2.0)
#define LN3D2 0.54930614433405484570

/**************	Implementation of exported functions ************/

float tanhf(float x)
{
    int sign = 0;
    float x2;
    static const float P[] = {
	-0.93363475652401,
	-0.21063958000245e2
    }, Q[] = {
	1.0,
	0.28077653470471e2,
	0.63191874015582e2
    };
    
    if (x < 0.0) {
	x = -x;
	sign = 1;
    }
    if (x > XBIG) x = 1.0;
    else if (x > LN3D2) x = 1.0 - 2.0/(expf(x+x)+1.0);
    else if (x > X_EPS_F) {
	x2 = x*x;
	x = x + x*x2*__POL1F(P,x2)/__POL2F(Q,x2);
    }
    if (sign) return -x;
    return x;
}

