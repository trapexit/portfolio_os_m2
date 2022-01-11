/*
	File:		TlTransform.cp

	Contains:	TlTransform class

	Written by:	Ravindar Reddy

	Change History (most recent first):

		<1+>	 10/4/94	Reddy	Fix transformation concatanation order

	To Do:
*/

#include <math.h>
#include "TlTransform.h"

TlTransform::TlTransform()
{
	int i, j;
	
	for ( i = 0; i < 4; i++ )
		for ( j = 0; j < 4; j++ )
			if ( i == j ) mat[ i ][ j ] = 1.0;
			else mat[ i ][ j ] = 0.0;
}

TlTransform::TlTransform( TlTransform const& tr )
{
	int i, j;
	
	for ( i = 0; i < 4; i++ )
		for ( j = 0; j < 4; j++ )
			mat[ i ][ j ] = tr.mat[ i ][ j ];
}

TlTransform::TlTransform( Matrix4x4  mtx )
{
	int i, j;
	
	for ( i = 0; i < 4; i++ )
		for ( j = 0; j < 4; j++ )
			mat[ i ][ j ] = mtx[ i ][ j ];
}

/*
** TlTransform::TlTransform() 
** Transformation about a vector (v) going through origin 
** and has a anti-clockwise rotation of 'ang' radians
*/

TlTransform::TlTransform( Vector3D const& v, double ang )
{
	double sv =  sin( ang );
	double cv =  cos( ang );
	double tmp = 1.0 - cv;

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
** TlTransform::MatToAxisRot() 
** Convert rotation transform into a 'rotation axis' and
** 'CCW rotation about this axis' 
*/
void 
TlTransform::AxisRotation( Vector3D *axis, double *ang )    
{
	double tr, s;
	double halfAng;
	int i, j, k;
	double q[4];
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
	
	/* clamp the q[3] values */
	if( q[3] > 1.0 ) q[3] = 1.0;
	else if ( q[3] < 0.0 ) q[3] = 0.0;
	
	halfAng = acos( q[3] );
	*ang = 2.0 * halfAng;
	// normalize the vector
	s = sqrt(
			q[0] * q[0] +
			q[1] * q[1] +
			q[2] * q[2]
			);
	if ( s != 0.0 )
	{
		axis->x = q[0] / s;
		axis->y = q[1] / s;
		axis->z = q[2] / s;
	} else {
		axis->x = 0.0;
		axis->y = 0.0;
		axis->z = 0.0;
	}
		
#if 0
		if( q[3] > 1.0 )
		fprintf( stderr, "Ang = %f, x = %f, y = %f, z = %f, halfAng = %e, w = %e\n",
			*ang, axis->x, axis->y, axis->z, halfAng, q[3] );
#endif
}		

/*
** TlTransform::TlTransform()
** Constructs TlTransform out of a
** po : position, pi : pivot, ps : scale, 
** rot : rotation angle and axis - rotation clockwise
** Note : rotation and scale is done about pivot point
** Warning : This method is specifically devoted to address
**           3DS quirky keyframe data
*/
TlTransform::TlTransform( 
Point3D const& po,	// node position
Point3D const& pi,	// node pivot point
Point3D const& ppi,	// node parents pivot point
Point3D const& ps,	// node scale vector
double rots[] )		// node rotation
{
	TlTransform T, T1, Ts, Tp1, Tpi1;
	Vector3D raxis; 
	double ang;

	ang = rots[0];
	raxis.x = rots[1];
	raxis.y = rots[2];
	raxis.z = rots[3];
				
	T.SetVal(3,0,-pi.x); 
	T.SetVal(3,1,-pi.y); 
	T.SetVal(3,2,-pi.z);
	T1.SetVal(3,0,pi.x); 
	T1.SetVal(3,1,pi.y); 
	T1.SetVal(3,2,pi.z);
	Tpi1.SetVal(3,0,ppi.x); 
	Tpi1.SetVal(3,1,ppi.y); 
	Tpi1.SetVal(3,2,ppi.z);
	Tp1.SetVal(3,0,po.x); 
	Tp1.SetVal(3,1,po.y); 
	Tp1.SetVal(3,2,po.z);
	Ts.SetVal(0,0,ps.x); 
	Ts.SetVal(1,1,ps.y); 
	Ts.SetVal(2,2,ps.z);
	TlTransform tr( raxis, -ang );
	*this = (T*Ts*tr*Tp1*Tpi1);
}

TlTransform& 
TlTransform::operator=( TlTransform const& tr )
{
	if (this != &tr ) {
		int i, j;
	
		for ( i = 0; i < 4; i++ )
			for ( j = 0; j < 4; j++ )
				mat[ i ][ j ] = tr.mat[ i ][ j ];
	}
	
	return (*this);
}

TlTransform 
operator*(const TlTransform& t1, const TlTransform& t2 )
{
	int i, j;
	TlTransform tmp;
	
	for ( i = 0; i < 4; i++ )
		for ( j = 0; j < 4; j++ )
			tmp.mat[i][j] = t1.mat[i][0] * t2.mat[0][j] +
			                t1.mat[i][1] * t2.mat[1][j] +
			                t1.mat[i][2] * t2.mat[2][j] +
			                t1.mat[i][3] * t2.mat[3][j] ;
		
	return tmp;
}

/*
** TlTransform::Invert()
** Inverts the TlTransform 
*/

TlTransform 
TlTransform::Invert() const
{
	Matrix4x4 mtx; 

	
	double a0 = mat[0][0];
	double a1 = mat[0][1];
	double a2 = mat[0][2];
	double a3 = mat[0][3];
	double a4 = mat[1][0];
	double a5 = mat[1][1];
	double a6 = mat[1][2];
	double a7 = mat[1][3];
	double a8 = mat[2][0];
	double a9 = mat[2][1];
	double a10 = mat[2][2];
	double a11 = mat[2][3];
	double a12 = mat[3][0];
	double a13 = mat[3][1];
	double a14 = mat[3][2];
	double a15 = mat[3][3];

	double a1a11a14a4 = a1*a11*a14*a4;
	double a1a10a15a4 = a1*a10*a15*a4;
	double a11a13a2a4 = a11*a13*a2*a4;
	double a10a13a3a4 = a10*a13*a3*a4;
	double a0a11a14a5 = a0*a11*a14*a5;
	double a0a10a15a5 = a0*a10*a15*a5;
	double a11a12a2a5 = a11*a12*a2*a5;
	double a10a12a3a5 = a10*a12*a3*a5;
	double a1a11a12a6 = a1*a11*a12*a6;
	double a0a11a13a6 = a0*a11*a13*a6;
	double a1a10a12a7 = a1*a10*a12*a7;
	double a0a10a13a7 = a0*a10*a13*a7;
	double a15a2a5a8 = a15*a2*a5*a8;
	double a14a3a5a8 = a14*a3*a5*a8;
	double a1a15a6a8 = a1*a15*a6*a8;
	double a13a3a6a8 = a13*a3*a6*a8;
	double a1a14a7a8 = a1*a14*a7*a8;
	double a13a2a7a8 = a13*a2*a7*a8;
	double a15a2a4a9 = a15*a2*a4*a9;
	double a14a3a4a9 = a14*a3*a4*a9;
	double a0a15a6a9 = a0*a15*a6*a9;
	double a0a14a7a9 = a0*a14*a7*a9;
	double a12a2a7a9 = a12*a2*a7*a9;
	double a11a14a5 = a11*a14*a5;
	double a10a15a5 = a10*a15*a5;
	double  a11a13a6 = a11*a13*a6;
	double a10a13a7 = a10*a13*a7;
	double a15a6a9 = a15*a6*a9;
	double a14a7a9 = a14*a7*a9;
	double a1a11a14 = a1*a11*a14;
	double a1a10a15 = a1*a10*a15;
	double a11a13a2 = a11*a13*a2;
	double a10a13a3 = a10*a13*a3;
	double a15a2a9 = a15*a2*a9;
	double a14a3a9 = a14*a3*a9;
	double a15a2a5 = a15*a2*a5;
	double a14a3a5 = a14*a3*a5;
	double a1a15a6 = a1*a15*a6;
	double a13a3a6 = a13*a3*a6;
	double a1a14a7 = a1*a14*a7;
	double a13a2a7 = a13*a2*a7;
	double a11a2a5 = a11*a2*a5;
	double a10a3a5 = a10*a3*a5;
	double a1a11a6 = a1*a11*a6;
	double a1a10a7 = a1*a10*a7;
	double a3a6a9 = a3*a6*a9;
	double a2a7a9 = a2*a7*a9;
	double a11a14a4 = a11*a14*a4;
	double a10a15a4 = a10*a15*a4;
	double a11a12a6 = a11*a12*a6;
	double a10a12a7 = a10*a12*a7;
	double a15a6a8 = a15*a6*a8;
    double a14a7a8 = a14*a7*a8;
    double a0a11a14 = a0*a11*a14;
    double a0a10a15 = a0*a10*a15;
    double a11a12a2 = a11*a12*a2;
    double a10a12a3 = a10*a12*a3;
    double a15a2a8 = a15*a2*a8;
    double a14a3a8 = a14*a3*a8;
    double a15a2a4 = a15*a2*a4;
	double a14a3a4 = a14*a3*a4;
    double a0a15a6 = a0*a15*a6;
    double a12a3a6 = a12*a3*a6;
    double a0a14a7 = a0*a14*a7;
    double a12a2a7 = a12*a2*a7;
    double a11a2a4 = a11*a2*a4;
    double a10a3a4 = a10*a3*a4;
    double a0a11a6 = a0*a11*a6;
    double a0a10a7 = a0*a10*a7;
    double a3a6a8 = a3*a6*a8;
    double a2a7a8 = a2*a7*a8;
    double a11a13a4 = a11*a13*a4;
    double a11a12a5 = a11*a12*a5;
    double a15a5a8 = a15*a5*a8;
    double a13a7a8 = a13*a7*a8;
    double a15a4a9 = a15*a4*a9;
    double a12a7a9 = a12*a7*a9;
    double a1a11a12 = a1*a11*a12;
    double a0a11a13 = a0*a11*a13;
    double a1a15a8 = a1*a15*a8;
    double a13a3a8 = a13*a3*a8;
    double a0a15a9 = a0*a15*a9;
    double a12a3a9 = a12*a3*a9;
    double a1a15a4 = a1*a15*a4;
    double a13a3a4 = a13*a3*a4;
    double a0a15a5 = a0*a15*a5;
    double a12a3a5 = a12*a3*a5;
    double a1a12a7 = a1*a12*a7;
    double a0a13a7 = a0*a13*a7;
    double a1a11a4 = a1*a11*a4;
    double a0a11a5 = a0*a11*a5;
    double a3a5a8 = a3*a5*a8;
    double a1a7a8 = a1*a7*a8;
    double a3a4a9 = a3*a4*a9;
    double a0a7a9 = a0*a7*a9;
    double a10a13a4 = a10*a13*a4;
    double a10a12a5 = a10*a12*a5;
    double a14a5a8 = a14*a5*a8;
    double a13a6a8 = a13*a6*a8;
    double a14a4a9 = a14*a4*a9;
    double a12a6a9 = a12*a6*a9;
    double a1a10a12 = a1*a10*a12;
    double a0a10a13 = a0*a10*a13;
    double a1a14a8 = a1*a14*a8;
    double a13a2a8 = a13*a2*a8;
    double a0a14a9 = a0*a14*a9;
    double a12a2a9 = a12*a2*a9;
    double a12a3a6a9 = a12*a3*a6*a9;
    double a1a14a4 = a1*a14*a4;
    double a13a2a4 = a13*a2*a4;
    double a0a14a5 = a0*a14*a5;
    double a12a2a5 = a12*a2*a5;
    double a1a12a6 = a1*a12*a6;
    double a0a13a6 = a0*a13*a6;
    double a1a10a4 = a1*a10*a4;
    double a0a10a5 = a0*a10*a5;
    double a2a5a8 = a2*a5*a8;
    double a1a6a8 = a1*a6*a8;
    double a2a4a9 = a2*a4*a9;
    double a0a6a9 = a0*a6*a9;

    double det =a1a11a14a4 -a1a10a15a4 -a11a13a2a4 +a10a13a3a4 -
           a0a11a14a5 +a0a10a15a5 +a11a12a2a5 -a10a12a3a5 -
           a1a11a12a6 +a0a11a13a6 +a1a10a12a7 -a0a10a13a7 -
           a15a2a5a8 +a14a3a5a8 +a1a15a6a8 -a13a3a6a8 -
           a1a14a7a8 +a13a2a7a8 +a15a2a4a9 -a14a3a4a9 -
           a0a15a6a9 +a12a3a6a9 +a0a14a7a9 -a12a2a7a9;

    mtx[0][0] = ( -a11a14a5 +a10a15a5 +a11a13a6
        -a10a13a7 -a15a6a9 +a14a7a9)/det;
    mtx[0][1] = ( a1a11a14 -a1a10a15 -a11a13a2
        +a10a13a3 + a15a2a9 -a14a3a9)/det;
    mtx[0][2] = ( -a15a2a5 +a14a3a5 +a1a15a6
        - a13a3a6 -a1a14a7 +a13a2a7)/det;
    mtx[0][3] = ( a11a2a5 -a10a3a5 -a1a11a6
        +a1a10a7 +a3a6a9 -a2a7a9)/det;
    mtx[1][0] = ( a11a14a4 -a10a15a4 -a11a12a6
        +a10a12a7 +a15a6a8 - a14a7a8)/det;
    mtx[1][1] = ( -a0a11a14 +a0a10a15 +a11a12a2
        -a10a12a3 - a15a2a8 +a14a3a8)/det;
    mtx[1][2] = ( a15a2a4 -a14a3a4 -a0a15a6
        +a12a3a6 + a0a14a7 -a12a2a7)/det;
    mtx[1][3] = ( -a11a2a4 +a10a3a4 +a0a11a6
        -a0a10a7 -a3a6a8 +a2a7a8)/det;
    mtx[2][0] = ( -a11a13a4 +a11a12a5 -a15a5a8 +a13a7a8 +a15a4a9 - a12a7a9)/det;
    mtx[2][1] = ( -a1a11a12 +a0a11a13 +a1a15a8
        -a13a3a8 - a0a15a9 +a12a3a9)/det;
    mtx[2][2] = ( -a1a15a4 +a13a3a4 +a0a15a5
        -a12a3a5 +a1a12a7 -a0a13a7)/det;
    mtx[2][3] = ( a1a11a4 -a0a11a5 +a3a5a8
        -a1a7a8 -a3a4a9 +a0a7a9)/det;
    mtx[3][0] = ( a10a13a4 -a10a12a5 +a14a5a8
        -a13a6a8 -a14a4a9 + a12a6a9)/det;
    mtx[3][1] = ( a1a10a12 -a0a10a13 -a1a14a8
        +a13a2a8 + a0a14a9 -a12a2a9)/det;
    mtx[3][2] = ( a1a14a4 -a13a2a4 -a0a14a5
        +a12a2a5 - a1a12a6 +a0a13a6)/det;
    mtx[3][3]= ( -a1a10a4 +a0a10a5 -a2a5a8
           +a1a6a8 +a2a4a9 -a0a6a9)/det;
           
	TlTransform tmp( mtx );
	return tmp;
}

Point3D 
operator*(const Point3D& p, const TlTransform& t )
{
	int j;
	double pt[4];
	Point3D tmp;
	
	
	for ( j = 0; j < 4; j++ )
	{
		pt[ j ] = t.mat[0][j] * p.x +
		           t.mat[1][j] * p.y +
		           t.mat[2][j] * p.z +
		           t.mat[3][j] ;
	}
	tmp.x = pt[0] / pt[3];
	tmp.y = pt[1] / pt[3];
	tmp.z = pt[2] / pt[3];
	
	return tmp;
}

Boolean
TlTransform::UnitTransform()
{
	int i, j;
	
	for ( i = 0; i < 4; i++ ) {
		for ( j = 0; j < 4; j++ ) {
			if ( i == j ) {
				if ( !AEQUAL( mat[ i ][ j ], 1.0 ) ) return FALSE;
			} else { 
				if ( !AEQUAL( mat[ i ][ j ], 0.0 ) ) return FALSE;
			}
		}
	}		
	return TRUE;
}

ostream& 
TlTransform::WriteSDF( ostream& os )
{
	int i;
	double x, y, z, w;
	
	char buf[100];
	sprintf( buf, "Transform {" );
	BEGIN_SDF( os, buf );

	for ( i = 0; i < 4; i++ )
	  {
	    /*sprintf( buf, "%0.5f %0.5f %0.5f %0.5f",
	      mat[0][i], mat[1][i],
	      mat[2][i], mat[3][i] ); */
	    x =  mat[i][0];
	    y =  mat[i][1];
	    z =  mat[i][2];
	    w =  mat[i][3];
	    sprintf( buf, "%g %g %g %g",x, y, z, w);
	    WRITE_SDF( os, buf );
	  } 
		
	END_SDF( os, "}" );

	return os;
}

