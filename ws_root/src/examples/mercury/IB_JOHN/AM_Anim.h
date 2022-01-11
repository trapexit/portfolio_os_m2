/*
**	File:		AM_Anim.h	
**
**	Contains:	Header file for spline based animation data 
**
**	Written by:	Ravindar Reddy
**
**	Copyright:	© 1996 by The 3DO Company. All rights reserved.
**	          	This material constitutes confidential and proprietary
**	          	information of the 3DO Company and shall not be used by
**	          	any Person or for any purpose except as expressly
**	          	authorized in writing by the 3DO Company.
**
**	Last Modified:	 96/05/08	version		1.8	
*/

#ifndef AM_Anim_INC
#define AM_Anim_INC	1

/*
** Articulated Model animation types that are supported:
** NONE			No animation, unit local transform
** CONSTANT		Local transform is calculated from 
**              point and rotational quternion
** LINEAR 		Local transform is calculated from
**              two constant keys
** QUADRATIC	Local transform is calculated from
**              point and rotational quternion
**              Cubic splines are evaluated to get above
**              point and rotational quternion
** CUBIC		Local transform is calculated from
**              point and rotational quternion
**              Cubic splines are evaluated to get above
**              point and rotational quternion
*/

/* 
** cubic spline translation path ( point )
** p = (x, y, z )
** p(t) = ap(t^3) + bp(t^2) + cp(t) + dp
*/
typedef struct 
{			
	float ax, bx, cx, dx;
	float ay, by, cy, dy;
	float az, bz, cz, dz;
} TransCubic;

/* 
** cubic spline rotation path ( quaternions )
** p = (x, y, z, w )
** p(t) = ap(t^3) + bp(t^2) + cp(t) + dp
*/
typedef struct 
{			
	float ax, bx, cx, dx;
	float ay, by, cy, dy;
	float az, bz, cz, dz;
	float aw, bw, cw, dw;
} RotCubic;

/* bit flags for animation data encoding */
#define TRANS_CONST (1 << 3)    /* Translation portion is one key */
#define TRANS_CUBIC (1 << 2)    /* Translation portion is cubic spline */

#define ROT_CONST (1 << 1)      /* Rotation portion is one key */
#define ROT_CUBIC (1 << 0)      /* Rotation portion is cubic spline */

/* time vector for the whole path, size =  mNumSegs + 1 */
#define GetTimeVector( spl )    ( spl->mData )
/* data to be interpolated, size = mNumSegs * mNumAttribs * 4  */
#define GetAttData( spl )   ( spl->mData + ( spl->mNumSegs + 1 ) )

/* cubic spline path data structure */
typedef struct
{
	uint32  mNumSegs;       /* number of path segments */
	float  mData[1];       /* time vector and attribute data */
} KfSpline;

void
Spl_EvaluateAt( 
	KfSpline	*inSpl, 
	uint32		nAtt,
	float		inTime, 
	float		*outVal 
	);

void
AM_EvalSpline(
	Vector3D *ppi,
	Vector3D *pi,
	uint32 *splData,
	float  inTime,
	Matrix *outMat
	);

void
AM_EvalTransform(
	Vector3D *ppi,
	Vector3D *pi,
	uint32 *splData,
	float  inTime,
	Matrix *outMat
	);

void
AM_BuildMatrix(
    Vector3D      *ppi,       /* parent pivot */
    Vector3D      *pi,        /* object pivot */
    Vector3D      *tr,        /* object translation */
    Quaternion  *rt,        /* object rotation as a quternion */
	Matrix 		*mat		/* output - local matrix */
    );

#endif
