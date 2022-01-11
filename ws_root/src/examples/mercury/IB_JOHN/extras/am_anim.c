/*
**	File:		AM_Anim.c	
**
**	Contains:	Cubic spline based animation evaluator 
**
**	Written by:	Ravindar Reddy
**
**	Copyright:	© 1996 by The 3DO Company. All rights reserved.
**	          	This material constitutes confidential and proprietary
**	          	information of the 3DO Company and shall not be used by
**	          	any Person or for any purpose except as expressly
**	          	authorized in writing by the 3DO Company.
**
**	Last Modified:	 96/05/08	version		1.16	
*/

#include <stdio.h>
#include <string.h>

#include "AM_Model.h" 
#include "AM_Anim.h" 

/*
** Evaluate the spline at a given time for the 
** interpolated attributes. This function assumes
** that the "outAttribs" array size is enough to
** fit "mNumAttribs".
*/  
void
Spl_EvaluateAt( 
	KfSpline	*inSpl, 
	uint32		nAtt,
	float		inTime, 
	float		*outVal 
	)
{
	register uint32 i = 0;
	float *tVect    = GetTimeVector( inSpl );
	float  *cCur    = GetAttData( inSpl );
	float inParm    = inTime;
	int nSegs       = inSpl->mNumSegs;
	float  s;
	uint32 si; 

	/*
	** Find the spline segment in which the input parameter
	** can be evaluated. If the input parameter falls outside
	** of the the spline definition bounds then clamp the 
	** evaluation the min/max bounds
	*/
	if ( ( inParm < *tVect ) )
	{
		si =  0;
		inParm     = *tVect;
	} else if ( inParm > *(tVect + nSegs) ) {
		si = nSegs - 1;
		inParm     = *(tVect + nSegs);
	} else {
		while( *(tVect+i+1) < inParm  ) 
		{
			/*printf( "tVect[%d] = %f\n", i,tVect[i] );  */
			i++;
		}
		si = i;
	}
	
	cCur += ( si * nAtt * 4 );
	s = ( inParm - *(tVect+si)  ) / ( *(tVect+si+1) - *(tVect+si) );
	/*	printf ( " ****Indx = %d, Parm = %f, %f\n", si, inTime, s ); */
		
	/* use nested multiplication for polynomial evaluation */
	for( i = 0; i < nAtt; i++ )
	{ 
		*outVal++ = s * ( s * ( s * *cCur + *(cCur+1) ) + *(cCur+2) ) + 
		              *(cCur+3);
		
		cCur     += 4;
	}	
}

/*
** calculate the local matrix of an object
** local_mat = (-pi) * (q) * (tr) * (ppi)
*/
void 
AM_BuildMatrix(
	Vector3D 		*ppi,		/* parent pivot */
	Vector3D 		*pi,		/* object pivot */
	Vector3D 		*tr,		/* object translation */
	Quaternion 	*q,			/* object rotation as a quternion */
	Matrix      *m        	/* output - local matrix */
	)
{
	/* 
	** rotation about the objects pivot point 
	** create a matrix out of quaternion
	*/
    float s = 2.0 / ( q->x*q->x + q->y*q->y + q->z*q->z + q->w*q->w );
    
    float xs = q->x*s,	      ys = q->y*s,	  zs = q->z*s;
    float wx = q->w*xs,	  wy = q->w*ys,	  wz = q->w*zs;
    float xx = q->x*xs,	  xy = q->x*ys,	  xz = q->x*zs;
    float yy = q->y*ys,	  yz = q->y*zs,	  zz = q->z*zs;
    
    m->mat[0][0] = 1.0 - (yy + zz); 
    m->mat[0][1] = xy + wz; 
    m->mat[0][2] = xz - wy;
    
    m->mat[1][0] = xy - wz; 
    m->mat[1][1] = 1.0 - (xx + zz); 
    m->mat[1][2] = yz + wx;
    
    m->mat[2][0] = xz + wy; 
    m->mat[2][1] = yz - wx; 
    m->mat[2][2] = 1.0 - (xx + yy);
	
	/* 
	** invert translate the object to its pivot - preconcatenate
	** apply the translation of the object and translate to its 
	** parents pivot - post concatenate
	*/
	m->mat[3][0] = -pi->x * m->mat[0][0] +
	             -pi->y * m->mat[1][0] +
	             -pi->z * m->mat[2][0] +
	             tr->x + ppi->x;
	m->mat[3][1] = -pi->x * m->mat[0][1] +
	             -pi->y * m->mat[1][1] +
	             -pi->z * m->mat[2][1] +
	             tr->y + ppi->y;
	m->mat[3][2] = -pi->x * m->mat[0][2] +
	             -pi->y * m->mat[1][2] +
	             -pi->z * m->mat[2][2] +
	             tr->z + ppi->z;
}

/*
** Animation evaluation routines
*/
void
AM_EvalSpline(
	Vector3D *ppi,
	Vector3D *pi,
	uint32 *splData,
	float  inTime,
	Matrix *outMat
	)
{
	KfSpline *spl;
	uint32	size;
	Vector3D	pnt;
	Quaternion quat;
	uint32 *data;

	data = splData + 1;

	/* evaluate translation key */
	if( *splData & TRANS_CONST )
	{
		size = sizeof( Vector3D );
		memcpy( &pnt, data, size );
	} else {
		spl = (KfSpline *)data; 
		Spl_EvaluateAt( spl, 3, inTime, (float *)&pnt ); 

		/* calculate the size to skip to rotational track */
		size = sizeof(KfSpline) +
			( ( spl->mNumSegs+1 ) +
			spl->mNumSegs * 3 * 4 - 1 ) *
			sizeof(gfloat);
	}

	/* evaluate rotation key */
	data = (uint32 *)( ((char *)data) + size );
	if( *splData & ROT_CONST )
		memcpy( &quat, data, sizeof( Quaternion ) );
	else Spl_EvaluateAt( (KfSpline *)data, 4, inTime, (float *)&quat );	

	/* build the local matrix */
	AM_BuildMatrix( ppi, pi, &pnt, &quat, outMat ); 
}
	
/*
** Animation evaluation routines
** Simply copies the local matrix
*/
void
AM_EvalTransform(
	Vector3D *ppi,
	Vector3D *pi,
	uint32 *splData,
	float  inTime,
	Matrix *outMat
	)
{
	(void)&ppi;
	(void)&pi;
	(void)&inTime;

	memcpy( outMat, splData, sizeof( Matrix ) );
}

