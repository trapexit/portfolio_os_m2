/*	@(#) atanf.c 96/02/07 1.2 */
/*	Implementation module : atanf.c

	Copyright 1989 Diab Data AB, Sweden

	Description :
	Implemention of libm function
	float atanf(float)

	Tables from Hart et al: Computer Aproximations, 1978

	History :
	When	Who	What
	890427	teve	initial of atan.c
        910708i	aem	made this float version
*/

/**************	Imported modules ********************************/

#include <math.h>
#include "values.h"
#include "mathf.h"
#include "math1f.h"


/**************	Local data, types, fns and macros ***************/

#define NEG	1
#define MID	2
#define HIGH	4

/**************	Implementation of exported functions ************/

float atanf(float x)
{
    int range = 0;
    float x2;
    static const float P[] = {		/* Table:	4962	*/
	0.79866237e-1,			/* Prec:	7.74	*/
	-0.1385205411,			/* Range:	[0,0.41]*/
	0.19974467215,			/* Order:	N=4	*/
	-0.33332799106,			/* arctan x = xP(x^2)	*/
	0.99999998199
    };
    
    if (x < 0.0) {
	x = -x;
	range = NEG;
    }
	
    if (x > M_SQRT2 - 1.0) {
	if (x > M_SQRT2 + 1.0) {
	    x = 1.0/x;
	    range |= HIGH;
	} else {
	    x = (x - 1.0)/(x + 1.0);
	    range |= MID;
	}
    }
    
    x2 = x*x;
    if (x2 > X_EPS_F*X_EPS_F) 
	x = x*__POL4F(P,x2);
    
    switch(range) {
    case NEG:
	return -x;
    case MID:
	return M_PI_4 + x;
    case MID|NEG:
	return -M_PI_4 - x;
    case HIGH:
	return M_PI_2 - x;
    case HIGH|NEG:
	return -M_PI_2 + x;
    }
    return x;
}
