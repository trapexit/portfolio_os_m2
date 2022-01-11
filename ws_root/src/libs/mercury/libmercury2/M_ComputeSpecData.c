/*
 *    @(#) M_ComputeSpecData.c 96/09/27 1.3
 *  Copyright 1996, The 3DO Company
 */

/* Computes the 2 cubic spans of the spline used in specular lighting  */

#include "mercury.h"
#include <math.h>

typedef float Mat[4][4];

#define EPSILON  0.000001

static float det2x2(float a1, float a2, float b1, float b2)

{
    return(a1 * b2 - a2 * b1);
}

static float det3x3(float a1, float a2, float a3,
		    float b1, float b2, float b3,
		    float c1, float c2, float c3)

{
    float ans;

    ans = (  a1 * det2x2(b2, b3, c2, c3)
	   - b1 * det2x2(a2, a3, c2, c3)
	   + c1 * det2x2(a2, a3, b2, b3));

    return(ans);
}

static float adjoint(Mat src, Mat dst)

{
    float a1, a2, a3, a4, b1, b2, b3, b4, c1, c2, c3, c4;
    float d1, d2, d3, d4;
    float ans;

    a1 = src[0][0]; b1 = src[0][1]; c1 = src[0][2];
    a2 = src[1][0]; b2 = src[1][1]; c2 = src[1][2];
    a3 = src[2][0]; b3 = src[2][1]; c3 = src[2][2];
    a4 = src[3][0]; b4 = src[3][1]; c4 = src[3][2];
    d1 = src[0][3];
    d2 = src[1][3];
    d3 = src[2][3];
    d4 = src[3][3];

    dst[0][0] =   det3x3(b2, b3, b4, c2, c3, c4, d2, d3, d4);
    dst[1][0] = - det3x3(a2, a3, a4, c2, c3, c4, d2, d3, d4);
    dst[2][0] =   det3x3(a2, a3, a4, b2, b3, b4, d2, d3, d4);
    dst[3][0] = - det3x3(a2, a3, a4, b2, b3, b4, c2, c3, c4);

    dst[0][1] = - det3x3(b1, b3, b4, c1, c3, c4, d1, d3, d4);
    dst[1][1] =   det3x3(a1, a3, a4, c1, c3, c4, d1, d3, d4);
    dst[2][1] = - det3x3(a1, a3, a4, b1, b3, b4, d1, d3, d4);
    dst[3][1] =   det3x3(a1, a3, a4, b1, b3, b4, c1, c3, c4);

    dst[0][2] =   det3x3(b1, b2, b4, c1, c2, c4, d1, d2, d4);
    dst[1][2] = - det3x3(a1, a2, a4, c1, c2, c4, d1, d2, d4);
    dst[2][2] =   det3x3(a1, a2, a4, b1, b2, b4, d1, d2, d4);
    dst[3][2] = - det3x3(a1, a2, a4, b1, b2, b4, c1, c2, c4);

    dst[0][3] =   det3x3(b1, b2, b3, c1, c2, c3, d1, d2, d3);
    dst[1][3] = - det3x3(a1, a2, a3, c1, c2, c3, d1, d2, d3);
    dst[2][3] =   det3x3(a1, a2, a3, b1, b2, b3, d1, d2, d3);
    dst[3][3] = - det3x3(a1, a2, a3, b1, b2, b3, c1, c2, c3);

    ans =    a1 * dst[0][0]
	   + b1 * dst[1][0]
	   + c1 * dst[2][0]
	   + d1 * dst[3][0];

    return ans;
	
}

static Err invert(Mat dst, Mat src) {

    int i, j;
    float det;

    /* Calculate the adjoint matrix */
    det = adjoint(src, dst);

    if (fabsf(det) < EPSILON) {
	return(-1);
    }

    /* Scale the adjoint matrix to get the inverse. */
    for (i = 0; i < 4; i++)
	for (j = 0; j < 4; j++)
	    dst[i][j] /= det;

    return(0);
}

void vectormul(float *vout, float *vin, Mat m)
{
    uint32 i,j;
    
    for (i=0; i<4; i++) {
	vout[i] = 0.;
	for (j=0; j<4; j++) {
	    vout[i] += vin[j]*m[i][j];
	}
    }
}	

void
M_ComputeSpecData(Pod *ppod)
{
    Mat m,m1;
    float p,s,shine,shine1;
    float v[4];
    
    Material *mat = ppod->pmaterial;
    /* Testing use constant data */

    mat->flags = specdatadefinedFLAG;

    shine = mat->shine*15.;
    if (shine <= 1.) {
    err:
	/* Consider it to be 0 */
	mat->specdata[0] = 0.;
	mat->specdata[1] = .5;
	mat->specdata[2] = 0.;
	mat->specdata[3] = 0.;
	mat->specdata[4] = 1.;
	mat->specdata[5] = 0.;
	mat->specdata[6] = 0.;
	mat->specdata[7] = 0.;
	mat->specdata[8] = 1.;
	mat->specdata[9] = 0.;
	return;
    }

    if (shine> 12.) shine = 12.;

    shine1 = 1./shine;

    mat->specdata[0]= s = powf(1./256., shine1);
    mat->specdata[1]= p = powf(.5,shine1);


    m[0][3] = 1.;
    m[0][2] = p;
    m[0][1] = p*p;
    m[0][0] = m[0][1]*p;

    m[1][3] = 1.;
    m[1][2] = s;
    m[1][1] = s*s;
    m[1][0] = m[1][1]*s;

    m[2][3] = 0.;
    m[2][2] = 1.;
    m[2][1] = 2.*p;
    m[2][0] = 3.*p*p;

    m[3][3] = 0.;
    m[3][2] = 1.;
    m[3][1] = 2.*s;
    m[3][0] = 3.*s*s;

    if (invert(m1,m) < 0) goto err;
    v[0] = .5;
    v[1] = 1./256.;
    v[2] = shine*powf(p,shine-1.);
    v[3] = shine*powf(s,shine-1.);

    vectormul(&mat->specdata[2],v,m1);

    m[1][3] = 1.;
    m[1][2] = 1.;
    m[1][1] = 1.;
    m[1][0] = 1.;

    m[3][3] = 0.;
    m[3][2] = 1.;
    m[3][1] = 2.;
    m[3][0] = 3.;

    if (invert(m1,m) < 0) goto err;
    v[1] = 1.;
    v[3] = shine;

    vectormul(&mat->specdata[6],v,m1);
}















