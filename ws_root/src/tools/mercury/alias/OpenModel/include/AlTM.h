//-
//	Copyright (C) 1995, Alias|Wavefront
//
//  These coded instructions,  statements and  computer programs contain
//  unpublished information proprietary to Alias|Wavefront  and are
//  protected by the Canadian and US Federal copyright law. They may not
//  be disclosed to third parties  or copied  or duplicated, in whole or
//  in part,  without the prior written consent of Alias|Wavefront
//
//  Unpublished-rights reserved under the Copyright Laws of the United States.
//
//+

//
//	.NAME AlTM - Basic Interface to Alias transformation matrix.
//
//  .SECTION Description
//
//      This class encapsulates the functionality for creating,
//      manipulating and deleting Alias 4x4 transformation matrices.
//
//		Tranformation matrices are used to perform linear transformations
//		of a vector.  Composite transformations are performed by multiplying
//		the individual transformations together.
//
//		The matrix is ordered as AlTM[y][x]:
//		.nf
// %@		0,0    0,1   0,2   0,3
// %@		1,0    1,1   1,2   1,3
// %@		2,0    2,1   2,2   2,3
// %@		3,0    3,1   3,2   3,3
//
//		The transformation matrix usually takes the form:
//
// %@ TM = %@ | r00 r01 r02 0 |
// %@%@%@%@%@ | r10 r11 r12 0 |
// %@%@%@%@%@ | r20 r21 r22 0 |
// %@%@%@%@%@ | tx0 ty0 tz0 1 |
//		.fi
//		where tx0, ty0, tz0 are the aggregrate translations and
//		r(xy) is the aggregate product of rotations, scales and shears.
//
//		A point is transformed by considering the point to be a row vector of
//		4 elements and then post multiplying by the transformation matrix.
//	
//		.nf
// %@ [x' y' z' w']
// %@= [x y z w] %@  | 00 01 02 03 |
// %@%@%@%@%@%@%@ | 10 11 12 13 |
// %@%@%@%@%@%@%@ | 20 21 22 23 |
// %@%@%@%@%@%@%@ | 30 31 32 33 |	
//		.fi
//		
//		If the w component is not specified, it is assumed to have a value of 1.
//
//		The static functions at the end of the class are used to create
//		typical tranformation matricies. 
//
//		Note: the [] operator has been defined for the AlTM class.  This means
//		that the individual matrix elements can be addressed as if the AlTM
//		were a standard 4x4 array of doubles.
//		This eliminates the need to create a temporary 4x4 array to retrieve
//		the values.
//	.br
//	e.g.	AlTM matrix;
//			matrix[3][2] = 10.0;
//

#ifndef _AlTM
#define _AlTM

typedef double _AlMatrix[4][4];

class AlTM {
	friend 		class AlFriend;
public:
	inline		AlTM() {};
	inline		~AlTM() {};

				AlTM( const AlTM& );
				AlTM( const double[4][4] );
				AlTM( const double d0,const double d1,const double d2,const double d3 );

	inline double* operator []( int row )
				{
					return fTM[row];
				}
	inline const double* operator []( int row ) const
				{
					return fTM[row];
				}

	statusCode	getTM( double[4][4] ) const;
	statusCode	setTM( const double[4][4] );

	AlTM&		operator =( const AlTM& );
	AlTM&		operator =( const double d );

	int 		operator ==( const AlTM& ) const;
	int			operator !=( const AlTM& ) const;

	AlTM		operator +( const AlTM& ) const;
	AlTM		operator -( const AlTM& ) const;
	AlTM		operator *( const AlTM& ) const;
	AlTM		operator *( const double ) const;

	friend AlTM	operator *( const double d, const AlTM& tm );

	AlTM&		operator +=( const AlTM& );
	AlTM&		operator -=( const AlTM& );
	AlTM&		operator *=( const AlTM& );
	AlTM&		operator *=( const double );

	statusCode	transPoint( double d[4] ) const;
	statusCode	transPoint( const double pt[4], double transPt[4] ) const;

	statusCode	transPoint( double& x, double &y, double &z, double &w ) const;
	statusCode	transPoint( double& x, double &y, double &z ) const; // assumes w = 1.0
	statusCode	transVector( double& x, double& y, double& z ) const;
	statusCode	transNormal( double& x, double& y, double& z ) const;

	AlTM		inverse( void ) const;
	AlTM		transpose( void ) const;

	static AlTM	identity( void );
	static AlTM	zero( void );
	static AlTM	diagonal( const double d0, const double d1, const double d2, const double d3 );

	// standard transformations
	static AlTM	scale( const double );
	static AlTM	scale_nonp( const double, const double, const double );
	static AlTM	translate( const double, const double, const double );
	static AlTM	rotateX( const double );
	static AlTM	rotateY( const double );
	static AlTM	rotateZ( const double );
	static AlTM rotate( double x, double y, double z, double angle );

protected:
	_AlMatrix	fTM;
};

#endif
