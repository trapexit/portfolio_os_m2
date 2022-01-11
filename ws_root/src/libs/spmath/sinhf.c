/*	@(#) sinhf.c 96/02/07 1.2 */
/*	Implementation module : sinhf.c

	Copyright 1989 Diab Data AB, Sweden

	Description :
	Implemention of libm function
	float sinh(float)

	Compute as (exp(x) - exp(-x))/2 if x > 1.0
	Otherwise use series
	Tables from W. Cody & W. Waite, 1980

	History :
	When	Who	What
	890501	teve	initial
	910408	teve	fixed so large negative numbers work
        910708  aem     made this float version
*/

/**************	Imported modules ********************************/

#include <math.h>
#include "values.h"
#include "mathf.h"
#include "math1f.h"
#include <errno.h>

/**************	Local data, types, fns and macros ***************/

#define LNV	0.6931610107421875
#define VD2M1	0.13830277879601902638e-4

/**************	Implementation of exported functions ************/

float sinhf(float x)
{
    float x2;
    static const float P[] = {		/* page 225 */
	-0.190333399,
	-0.713793159e1
    }, Q[] = {
	1.0,
	-0.428277109e2
    };
    
    x2 = x;
    if (x < 0.0) x2 = -x2;
    if (x2 > 1.0) {
	if (x2 > LN_MAXFLOAT) {
	    if (x2 < LN_MAXFLOAT + M_LN2) {
		x2 = expf(x2 - LNV);
		x2 = x2 + x2*VD2M1;
		if (x < 0.0) return -x2;
		else return x2;
	    }
	    errno = ERANGE;
	    return HUGE_VAL_F;
	}
	x2 = expf(x2);
	x2 = 0.5*x2 - 0.5/x2;
	if (x < 0.0) return -x2;
	else return x2;
    } else {
	if (x2 < X_EPS_F) return x;
	x2 = x*x;
	return x + x*x2*__POL1F(P,x2)/__POL1F(Q,x2);
    }
}
