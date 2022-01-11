/*	@(#) powf.c 96/05/10 1.4 */
/**
|||	AUTODOC -public -class Math -group Power -name powf
|||	Power Function
|||
|||	  Synopsis
|||
|||	    float powf( float x, float y ) 
|||
|||	  Description
|||
|||	    This function does exponentiation.  It returns the first number
|||	    raised to the second number.
|||
|||	  Arguments
|||
|||	    x
|||	        input float
|||
|||	    y
|||	        float representing the power to raise x to.
|||
|||	  Return Value
|||
|||	    If x is equal to 0 and y is less than or equal to 0, or
|||	    y is not integral, errno is set to EDOM and 0 is returned.
|||	    Otherwise, x^y is returned.
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
|||	    logf(), log10f()
|||
**/
/*	Implementation module : powf.c

	Copyright 1989 Diab Data AB, Sweden

	Description :
	Implemention of libm function
	float powf(float, float)

	Tables & algorithm from Cody & Waite, 1980

	History :
	When	Who	What
	890427	teve	initial
	901210	dag	declared reduce() as static
	930114	aem	Corrected the a1 table constants
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

static float reducef(float x)		/* clear all but 4 fraction bits */
{
    /* Use for big-endians 32-bit int IEEE machines */
    int clr;		/* bits to clear */
    union ieee_bits_f *up;
    
    up = (union ieee_bits_f *)&x;
    clr = IEEE_MANT_F - ((int)up->b.exp - IEEE_BIAS_F) - 4;
    if (clr > IEEE_MANT_F) {                /* All is franction */
	return 0.0;
    } else if ( clr > 0 ) {
	up->i.i &= (-1L)<<clr;  /* Mask away unwanted fraction */
    } /* else all is integer -- nothing needs to be masked away */
    return x;
}


/* The following table must be exact. This is the IEEE constants*/

static const union {
    long l[17];
    float d[17];
} a1 = {
    0x3F800000,
    0x3f75257d,
    0x3f6ac0c6,
    0x3f60ccde,
    0x3f5744fc,
    0x3f4e248c,
    0x3f45672a,
    0x3f3d08a3,
    0x3f3504f3,
    0x3f2d583e,
    0x3f25fed6,
    0x3f1ef532,
    0x3f1837f0,
    0x3f11c3d3,
    0x3f0b95c1,
    0x3f05aac3,
    0x3f000000,
};

#define A1(i)	a1.d[(i)-1]

static const union {
    long l[8];
    float d[8];
} a2 = {
    0x24858f73,
    0x2388832c,
    0x2363e235,
    0x24d7d510,
    0x24319260,
    0x23d6d048,
    0x244d83f5,
    0x24453172,
};

#define A2(i)	a2.d[(i)-1]

#define MAX_IW1 ((127*16)-1)
#define MIN_IW1 ((-126*16)-1)

static const float P[] = {
    0.125064850052e-1,
    0.833333286245e-1
};

static const float Q[] = {
    0.130525515942810e-2,
    0.961620659583789e-2,
    0.555040488130765e-1,
    0.240226506144710,
    0.693147180556341
};

#define K 0.44269504088896340736

/* #define REDUCE(x) ((float)((int)(16.0F * x)) * 0.0625F) */
#define REDUCE(x) reducef(x)

static float pow_errf(int err, float val)
{	
    if (err == DOMAIN) {
#ifdef BUILD_PARANOIA
	printf("powf: domain error\n");
#endif
	errno = EDOM;	
    } else {
#ifdef BUILD_PARANOIA
	printf("powf: range error\n");
#endif	
	errno = ERANGE;
    }
    return val;
}

/**************	Implementation of exported functions ************/

float powf(float x, float y)
{
    float g, z, v, r, u2, u1, y1, y2, w, w1, w2;
    int sign = 0;
    int m, p, iw1, olderr;
    
    if (x <= 0.0) {	/* check that x == 0 or y is integral */
	int err = 0;
	if (x == 0.0) {
	    if (y > 0.0) 
		return x;
	    err = DOMAIN;
	} else {
	    float f, i;
	    f = modff(0.5 * y, &i);
	    if (f != 0.0) {
		if (f != 0.5) 
		    err = DOMAIN;
		else sign = 1;
	    }
	}
	if (err) return pow_errf(err,0.0);
	x = -x;
    }
    g = frexpf(x,&m);
    p = 1;
    if (g <= A1(9)) p = 9;
    if (g <= A1(p+4)) p += 4;
    if (g <= A1(p+2)) p += 2;
    z = 2.0 * ((g - A1(p+1)) - A2((p+1)>>1)) / (g + A1(p+1));
    v = z * z;
    r = __POL1F(P,v) * v * z;
    r = r + K*r;
    u2 = (r + z*K) + z;
    u1 = (m*16 - p) * 0.0625;
    y1 = REDUCE(y);
    y2 = y - y1;
    w = u2*y + u1*y2;
    w1 = REDUCE(w);
    w2 = w - w1;
    w = w1 + u1*y1;
    w1 = REDUCE(w);
    w = w - w1;
    w2 = w2 + w;
    w = REDUCE(w2);
    w1 = w1 + w;
    if (w1 > INT_MAX/32 || w1 < -INT_MAX/32) {
	if (w1 < 0.0) return pow_errf(UNDERFLOW,0.0);
	return pow_errf(OVERFLOW,sign ? -HUGE_VAL_F : HUGE_VAL_F);
    }
    iw1 = 16.0 * w1;
    w2 = w2 - w;
    if (w2 > 0.0) {
	w2 -= 0.0625;
	iw1++;
    }
    if (iw1 < 0) m = iw1/16;
    else m = (unsigned)iw1/16 + 1;
    p = 16*m - iw1;
    z = w2 * __POL4F(Q,w2);
    z = A1(p+1) + A1(p+1)*z;
    olderr = errno;
    errno = 0;
    z = ldexpf(z,m);
#if 0 /* because of the errno = 0 above, the enclosed code won't be run ... */
    if (errno) {
	if (m < 0) 
	    return pow_errf(UNDERFLOW,0.0);
	else 
	    return pow_errf(OVERFLOW,sign ? 
			    -HUGE_VAL_F : HUGE_VAL_F);
    } else {
#else
    {
#endif
	errno = olderr;
    }
    if (sign) return -z;
    return z;
}
