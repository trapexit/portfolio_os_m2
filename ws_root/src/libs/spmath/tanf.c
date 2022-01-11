/*	@(#) tanf.c 96/02/07 1.2 */
/*	Implementation module : tanf.c

	Copyright 1989 Diab Data AB, Sweden

	Description :
	Implemention of libm function
	float tanf(float)

	Tables from Hart et al: Computer Aproximations, 1978

	History :
	When	Who	What
	890427	teve	initial of tan.c
        910708  aem     made this float version
*/

/**************	Imported modules ********************************/

#include <math.h>
#include "values.h"
#include "mathf.h"
#include "math1f.h"
#include <errno.h>

/**************	Local data, types, fns and macros ***************/

#define M_4_PI	(2.0*M_2_PI)
#define NEG	1
#define INV	2

/**************	Implementation of exported functions ************/

float tanf(float x)
{
    int type = 0;
    float x_org, x2, p, q;
    static const float P[] = {	        /* Table: 4282          */
	-0.1255329742424e2,		/* Prec:  7.85          */
	0.21242445758263e3             /* Order: n=1,m=2       */
    }, Q[] = {
	1.0,
	-0.7159606050466e2,
	0.27046722349399e3
    };
    
    if (x < 0.0) {
	x = -x;
	type = NEG;
    }
    x_org = x;
    if (x < M_PI_2) {			/* minimize loss of precision*/
	if (x > (float)M_PI_4) {
	    x = M_PI_2 - x;
	    type |= INV;
	}
	x = x * M_4_PI;
    } else {
	x = x * M_1_PI;
	if (x_org > X_TLOSS_F/2) {
	    errno = ERANGE;
	    return 0.0;
	}
	/* reduce */
	
	if (x < (float)MAXLONG) {	/* use integer */
	    long l;
	    l = (long) x;
	    x = x - (float)l;
	} else {
	    float xi;
	    x = modff(x,&xi);
	}
	if (x > 0.5) {
	    x = 1.0 - x;
	    type ^= NEG;
	}
	if (x > 0.25) {
	    x = 0.5 - x;
	    type ^= INV;
	}
	x = x * 4.0;
    }
    x2 = x*x;
    if (x > X_EPS_F) {
	p = x * __POL1F(P,x2);
	q = __POL2F(Q,x2);
    } else {
	p = x * M_PI_4;
	q = 1.0;
	if (p == 0.0 && type & INV) p = 1.0/X_TLOSS_F;
    }
    if (x_org > X_PLOSS_F/2) {
	p = (type & INV) ? p/q : q/p;
	if (type & NEG) p = -p;
	errno = ERANGE;
	return p;
    }
    switch(type) {
    case 0:
	return p/q;
    case NEG:
	return -p/q;
    case INV:
	return q/p;
    case INV|NEG:
	return -q/p;
    }
    return 0.0; /* not reached */
}
