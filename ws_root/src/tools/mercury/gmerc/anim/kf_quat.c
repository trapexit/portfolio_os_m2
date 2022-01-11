#include <math.h>
#include "kf_types.h"
#include "kf_quat.h"

/*
** Create a rotation matrix out of "rotation about an
** arbitrary axis
*/
void
AxisRotToMat( 
	AxisRot3D v, 
	Matrix4D mat 
	)
{
	float sv = sin( v.ang );
	float cv = cos( v.ang );
	float tmp = 1.0 - cv;

	mat[0][0] = v.x*v.x + (1.0 - v.x*v.x) * cv ;
	mat[0][1] = v.x * v.y * tmp + v.z * sv ;
	mat[0][2] = v.x * v.z * tmp - v.y * sv ;

	mat[1][0] = v.x * v.y * tmp - v.z * sv ;
	mat[1][1] = v.y*v.y + (1.0 - v.y*v.y) * cv ;
	mat[1][2] = v.y * v.z * tmp + v.x * sv ;

	mat[2][0] = v.x * v.z * tmp + v.y * sv ;
	mat[2][1] = v.y * v.z * tmp - v.x * sv ;
	mat[2][2] = v.z*v.z + (1.0 - v.z*v.z) * cv ;

	mat[0][3] = 0.0; mat[1][3] = 0.0; mat[2][3] = 0.0;
	mat[3][0] = 0.0; mat[3][1] = 0.0; mat[3][2] = 0.0; 
	mat[3][3] = 1.0;
}

/* 
** Return quaternion product qL * qR.  Note: order is important!
** To combine rotations, use the product Mul(qSecond, qFirst),
** which gives the effect of rotating by qFirst then qSecond. 
*/

void
Quat_Mul( 
	Quat qL, 
	Quat qR, 
	Quat qq 
	)
{
    qq[W] = qL[W]*qR[W] - qL[X]*qR[X] - qL[Y]*qR[Y] - qL[Z]*qR[Z];
    qq[X] = qL[W]*qR[X] + qL[X]*qR[W] + qL[Y]*qR[Z] - qL[Z]*qR[Y];
    qq[Y] = qL[W]*qR[Y] + qL[Y]*qR[W] + qL[Z]*qR[X] - qL[X]*qR[Z];
    qq[Z] = qL[W]*qR[Z] + qL[Z]*qR[W] + qL[X]*qR[Y] - qL[Y]*qR[X];
}


/* 
** Construct rotation matrix from (possibly non-unit) quaternion.
** Assumes matrix is used to multiply column vector on the left:
** vnew = mat vold.  Works correctly for right-handed coordinate system
** and right-handed rotations. 
*/

void
Quat_ToMatrix( 
	Quat q, 
	Matrix4D mat 
	)
{
    float s = 2.0 / ( q[X]*q[X] + q[Y]*q[Y] + q[Z]*q[Z] + q[W]*q[W] );
    
    float xs = q[X]*s,	      ys = q[Y]*s,	  zs = q[Z]*s;
    float wx = q[W]*xs,	  wy = q[W]*ys,	  wz = q[W]*zs;
    float xx = q[X]*xs,	  xy = q[X]*ys,	  xz = q[X]*zs;
    float yy = q[Y]*ys,	  yz = q[Y]*zs,	  zz = q[Z]*zs;
    
    mat[X][X] = 1.0 - (yy + zz); 
    mat[X][Y] = xy + wz; 
    mat[X][Z] = xz - wy;
    
    mat[Y][X] = xy - wz; 
    mat[Y][Y] = 1.0 - (xx + zz); 
    mat[Y][Z] = yz + wx;
    
    mat[Z][X] = xz + wy; 
    mat[Z][Y] = yz - wx; 
    mat[Z][Z] = 1.0 - (xx + yy);
    
    mat[X][W] = mat[Y][W] = mat[Z][W] = 0.0;
    mat[W][X] = mat[W][Y] = mat[W][Z] = 0.0;
    mat[W][W] = 1.0;
}

void
Quat_FromMatrix( 
	Matrix4D mat, 
	Quat q 
	)
{
	float tr, s;
	float halfAng;
	int i, j, k;
	int nxt[] = { 1, 2, 0 };
	
	tr = mat[0][0] + mat[1][1] + mat[2][2];
	if ( tr > 0.0 )
	{
		s = sqrt( tr + 1.0 );
		q[3] = s * 0.5;
		s = 0.5 / s;
		
		q[0] = ( mat[1][2] - mat[2][1] ) * s;
		q[1] = ( mat[2][0] - mat[0][2] ) * s;
		q[2] = ( mat[0][1] - mat[1][0] ) * s;
	} else {
		i = 0;
		if ( mat[1][1] > mat[0][0] ) i = 1;
		if ( mat[2][2] > mat[i][i] ) i = 2;
		j = nxt[i]; k= nxt[j];
				
		s = sqrt( ( mat[i][i] - ( mat[j][j] + mat[k][k] ) ) + 1.0 );
		
		q[i] = s * 0.5;
		s = 0.5 / s;
		
		q[3] = ( mat[j][k] - mat[k][j] ) * s;
		q[j] = ( mat[i][j] + mat[j][i] ) * s;
		q[k] = ( mat[i][k] + mat[k][i] ) * s;
	}
}		

/*
** Spherical linear interpolation between two quternions
*/
void 
Quat_Slerp( 
	Quat p, 
	Quat q, 
	float t, 
	Quat qt 
	)
{
	float omega, cosom, sinom, sclp, sclq;
	int i;
	
	cosom = p[X] * q[X] + p[Y] * q[Y] + p[Z] * q[Z] + p[W] * q[W];
	
	if ( ( 1.0 + cosom ) > EPSILON )
	{
		if ( ( 1.0 - cosom ) > EPSILON )
		{
			omega = acos( cosom );
			sinom = sin( omega );
			sclp = sin( ( 1.0 - t ) * omega ) / sinom;
			sclq = sin( t * omega ) / sinom;
		} else {
			sclp = 1.0 - t;
			sclq = t;
		}
		for ( i = 0; i < 4; i++ ) qt[i] = sclp * p[i] + sclq * qt[i];
	} else {
		qt[X] = -p[Y]; qt[Y] = p[X];
		qt[Z] = -p[W]; qt[W] = p[Z];
		sclp = sin( ( 1.0 - t ) * HALFPI );
		sclq = sin( t * HALFPI );
		for ( i = 0; i < 3; i++ ) qt[i] = sclp * p[i] + sclq * qt[i];
	}
}
		

/* 
** Return conjugate of quaternion. 
*/

void 
Quat_Conj( 
	Quat q,
	Quat qq 
	)
{
    qq[X] = -q[X]; qq[Y] = -q[Y]; qq[Z] = -q[Z]; qq[W] = q[W];
}

