/*
   File:	texmap.c
   
   Contains:	Handles texture mapping of a lightwave file 

   Written by:	Todd Allendorf 
   
   Copyright:	© 1995 by The 3DO Company. All rights reserved.
   This material constitutes confidential and proprietary
   information of the 3DO Company and shall not be used by
   any Person or for any purpose except as expressly
   authorized in writing by the 3DO Company.
   
   Change History (most recent first):
   To Do:
   */



#include <stdlib.h>
#include <stdio.h>
#include "vec.h"
#include "M2TXTypes.h"
#include "M2Err.h"
#include "texmap.h"
typedef float gfloat;
#include "LWSURF.h"



#ifdef MSDOS
static const double atanhi[] = {
  4.63647609000806093515e-01, /* atan(0.5)hi 0x3FDDAC67, 0x0561BB4F */
  7.85398163397448278999e-01, /* atan(1.0)hi 0x3FE921FB, 0x54442D18 */
  9.82793723247329054082e-01, /* atan(1.5)hi 0x3FEF730B, 0xD281F69B */
  1.57079632679489655800e+00, /* atan(inf)hi 0x3FF921FB, 0x54442D18 */
};

static const double atanlo[] = {
  2.26987774529616870924e-17, /* atan(0.5)lo 0x3C7A2B7F, 0x222F65E2 */
  3.06161699786838301793e-17, /* atan(1.0)lo 0x3C81A626, 0x33145C07 */
  1.39033110312309984516e-17, /* atan(1.5)lo 0x3C700788, 0x7AF0CBBD */
  6.12323399573676603587e-17, /* atan(inf)lo 0x3C91A626, 0x33145C07 */
};

static const double aT[] = {
  3.33333333333329318027e-01, /* 0x3FD55555, 0x5555550D */
 -1.99999999998764832476e-01, /* 0xBFC99999, 0x9998EBC4 */
  1.42857142725034663711e-01, /* 0x3FC24924, 0x920083FF */
 -1.11111104054623557880e-01, /* 0xBFBC71C6, 0xFE231671 */
  9.09088713343650656196e-02, /* 0x3FB745CD, 0xC54C206E */
 -7.69187620504482999495e-02, /* 0xBFB3B0F2, 0xAF749A6D */
  6.66107313738753120669e-02, /* 0x3FB10D66, 0xA0D03D51 */
 -5.83357013379057348645e-02, /* 0xBFADDE2D, 0x52DEFD9A */
  4.97687799461593236017e-02, /* 0x3FA97B4B, 0x24760DEB */
 -3.65315727442169155270e-02, /* 0xBFA2B444, 0x2C6A6C2F */
  1.62858201153657823623e-02, /* 0x3F90AD3A, 0xE322DA11 */
};

static const double 
one   = 1.0,
huge   = 1.0e300;

	double myatan(double x)
{
	double w,s1,s2,z;
	int32 id,hx;
	if (x<0.0)
	  hx= -1;
	else
	  hx = 1;
	if (fabs(x) < 0.4375) {	/* |x| < 0.4375 */
	    id = -1;
	} else {
	x = fabs(x);
	if (x < 1.1875) {		/* |x| < 1.1875 */
	    if (x < 0.6875) {	/* 7/16 <=|x|<11/16 */
		id = 0; x = (2.0*x-one)/(2.0+x); 
	    } else {			/* 11/16<=|x|< 19/16 */
		id = 1; x  = (x-one)/(x+one); 
	    }
	} else {
	    if (x < 2.4375) {	/* |x| < 2.4375 */
		id = 2; x  = (x-1.5)/(one+1.5*x);
	    } else {			/* 2.4375 <= |x| < 2^66 */
		id = 3; x  = -1.0/x;
	    }
	}}
    /* end of argument reduction */
	z = x*x;
	w = z*z;
    /* break sum from i=0 to 10 aT[i]z**(i+1) into odd and even poly */
	s1 = z*(aT[0]+w*(aT[2]+w*(aT[4]+w*(aT[6]+w*(aT[8]+w*aT[10])))));
	s2 = w*(aT[1]+w*(aT[3]+w*(aT[5]+w*(aT[7]+w*aT[9]))));
	if (id<0) return x - x*(s1+s2);
	else {
	    z = atanhi[id] - ((x*(s1+s2) - atanlo[id]) - x);
	    return (hx<0)? -z:z;
	}
}


/* __ieee754_atan2(y,x)
 * Method :
 *	1. Reduce y to positive by atan2(y,x)=-atan2(-y,x).
 *	2. Reduce x to positive by (if x and y are unexceptional): 
 *		ARG (x+iy) = arctan(y/x)   	   ... if x > 0,
 *		ARG (x+iy) = pi - arctan[y/(-x)]   ... if x < 0,
 *
 * Special cases:
 *
 *	ATAN2((anything), NaN ) is NaN;
 *	ATAN2(NAN , (anything) ) is NaN;
 *	ATAN2(+-0, +(anything but NaN)) is +-0  ;
 *	ATAN2(+-0, -(anything but NaN)) is +-pi ;
 *	ATAN2(+-(anything but 0 and NaN), 0) is +-pi/2;
 *	ATAN2(+-(anything but INF and NaN), +INF) is +-0 ;
 *	ATAN2(+-(anything but INF and NaN), -INF) is +-pi;
 *	ATAN2(+-INF,+INF ) is +-pi/4 ;
 *	ATAN2(+-INF,-INF ) is +-3pi/4;
 *	ATAN2(+-INF, (anything but,0,NaN, and INF)) is +-pi/2;
 *
 * Constants:
 * The hexadecimal values are the intended ones for the following 
 * constants. The decimal values may be used, provided that the 
 * compiler will convert from decimal to binary accurately enough 
 * to produce the hexadecimal values shown.
 */
static const double
tiny  = 1.0e-300,
  zero  = 0.0,
  pi_o_4  = 7.8539816339744827900E-01, /* 0x3FE921FB, 0x54442D18 */
  pi_o_2  = 1.5707963267948965580E+00, /* 0x3FF921FB, 0x54442D18 */
  pi      = 3.1415926535897931160E+00, /* 0x400921FB, 0x54442D18 */
  pi_lo   = 1.2246467991473531772E-16; /* 0x3CA1A626, 0x33145C07 */

static double atan3(double y, double x)
{  
  double z;
  
  if(x==1.0)
    return atan(y);   /* x=1.0 */
  
  /* when y = 0 */
  if (y==0.0)
    {
      if (x<0.0)
	return(pi+tiny);
      else
	return(0.0);
    }
  /* when x = 0 */
  if (x==0.0)
    if (y<0.0)
      return(-pi_o_2-tiny);
    else
      return(pi_o_2+tiny);
  
  z=myatan(fabs(y/x));		/* safe to do y/x */
  if (x>0.0) 
    {
      if (y>0.0)
	return z;
      else
	return -z;
    }
  else
    {
      if (y>0.0)
	return pi-(z-pi_lo);
      else
	return (z-pi_lo)-pi;
    }
}

#endif

#define step(min, val)   ((val)<(min) ? 0.0 : 1.0 )

#define PI 3.14159265359



#define C_EPS 1.0e-15

#define FP_EQUAL(s, t) (fabs(s - t) <= C_EPS)

void CylinMap(double *p, double *u, double *v, double *maporigin, 
	 double *xaxis, double *yaxis, double *zaxis)
{
  double V[3], Vn[3];
  double XX[3], YY[3], ZZ[3];
  double xx, yy;
  double myU, myV;
  double len;

  VMV3(V, p, maporigin);

  if(!ISZEROVEC3(V))
    {
      len = NORMSQRD3(V);
/*      len = 1.0/sqrt(len); */
      len = 1.0/len;
      SXV3(Vn, len, V);
    }
  else
    SET3(Vn, V);

  SET3(XX, xaxis);
  if(!ISZEROVEC3(XX))
    {
      len = NORMSQRD3(XX);
/*      len = 1.0/sqrt(len); */
     len = 1.0/len;
      SXV3(XX, len, XX);
    }

  SET3(YY, yaxis);
  if(!ISZEROVEC3(YY))
    {
      len = NORMSQRD3(YY);
/*      len = 1.0/sqrt(len); */
      len = 1.0/len;
      SXV3(YY, len, YY);
    }
  SET3(ZZ, zaxis);

  if(!ISZEROVEC3(ZZ))
    {
      len = NORMSQRD3(ZZ);
/*      len = 1.0/sqrt(len); */
      len = 1.0/len;
      SXV3(ZZ, len, ZZ);
    }

  xx = DOT3(Vn, XX);
  yy = DOT3(Vn, YY);
  
  myV = DOT3(V, ZZ);
  
  if (FP_EQUAL(yy,0.0) && FP_EQUAL(xx,0.0))
    *u = .5;
  else
    {
#ifdef MSDOS
      myU = atan3(yy, xx) / (2.0*PI);   /* -.5 -> .5 */
#else
      myU = atan2(yy, xx) / (2.0*PI);   /* -.5 -> .5 */
#endif
      myU = myU + step(0.0,-myU); /* remaps to 0->1*/
      *u = myU;
    }

  *v = myV;
}
  
void PlaneMap(double *p, double *u, double *v, double *maporigin, 
	 double *xaxis, double *yaxis, double *zaxis)
{
  double V[3];
  double XX[3], YY[3];
  double len;
  double *Z=zaxis;

  VMV3(V, p, maporigin);
 
  SET3(XX, xaxis);

  if(!ISZEROVEC3(XX))
    {
      len = NORMSQRD3(XX);
/*      len = 1.0/sqrt(len); */
      len = 1.0/len;
      SXV3(XX,len,XX);
    }


  SET3(YY, yaxis);

  if(!ISZEROVEC3(YY))
    {
      len = NORMSQRD3(YY);
      /*      len = 1.0/sqrt(len);*/
      len = 1.0/len;
      SXV3(YY,len,YY);
    }

  *u = DOT3(V, XX); 
  *v = DOT3(V, YY); 
  
}


void SphereMap(double *p, double *u, double *v, double *maporigin, 
	  double *xaxis, double *yaxis, double *zaxis)
{
  double V[3];
  double XX[3], YY[3], ZZ[3];
  double xx, yy, zz;
  double len;

  VMV3(V, p, maporigin); 
  if(!ISZEROVEC3(V))
    {
      len = NORMSQRD3(V);
      len = 1.0/sqrt(len); 
/*      len = 1.0/(len); */
      SXV3(V,len,V);
    }
  
  SET3(XX, xaxis);
  if(!ISZEROVEC3(XX))
    {
      len = NORMSQRD3(XX);
      len = 1.0/sqrt(len); 
/*      len = 1.0/len; */
      SXV3(XX,len,XX);
    }

  SET3(YY, yaxis);
  if(!ISZEROVEC3(YY))
    {
      len = NORMSQRD3(YY);
      len = 1.0/sqrt(len); 
/*      len = 1.0/(len); */
      SXV3(YY,len,YY);
    }

  SET3(ZZ, zaxis);
  if(!ISZEROVEC3(ZZ))
    {
      len = NORMSQRD3(ZZ);
      len = 1.0/sqrt(len); 
/*      len = 1.0/(len); */
      SXV3(ZZ,len,ZZ);
    }
  xx = DOT3(V, XX);
  yy = DOT3(V, YY);
  zz = DOT3(V, ZZ);

  if (FP_EQUAL(yy,0.0) && FP_EQUAL(xx,0.0))
    *u = .5;
  else
    {
#ifdef MSDOS
      *u = ((double)atan3(yy, xx)) / ((double)(2.0*PI));   /* .5 -. .5 */      
      /*
	printf("atan3=%g  \tatan2=%g  \txx=%g  \tyy=%g  \tU=%g\n",
	atan3(yy,xx),atan2(yy,xx), xx,yy,*u);
	*/
#else
      *u = ((double)atan2(yy, xx)) / ((double)(2.0*PI));   /* .5 -. .5 */
#endif
      *u = *u + step(0,-(*u));  /* remaps to 0->1*/
    }
  *v = acos(-zz)/PI;
}






