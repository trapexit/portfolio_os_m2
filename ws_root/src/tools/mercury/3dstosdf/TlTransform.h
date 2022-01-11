/*
	File:		TlTransform.h

	Contains:	TlTransform class header

	Written by:	Ravindar Reddy

	Change History (most recent first):

		<2+>	 11/8/94	RRR		To concatenate rotation from previous key
		<1+>	 10/4/94	Reddy To fix transform concatanation order

	To Do:
*/

#include "TlBasicTypes.h"

#ifndef TRANSFORM
#define TRANSFORM


class TlTransform {

	Matrix4x4 mat;     

public:

	// Constructors

	TlTransform();
	TlTransform( Matrix4x4  mtx );
	// Create transformation out of rotation around an axis (in degrees) 
	TlTransform( Vector3D const& vec, double ang );
	// Create transformation out of position and rotation around an axis
	TlTransform( Point3D const& po, Point3D const& pi, Point3D const& ppi, 
	           Point3D const& ps, double rots[] );
	// Initializer            
	TlTransform( TlTransform const& transf ); 
	// Assignment               
	TlTransform& operator=( TlTransform const & transf );
	
	void AxisRotation( Vector3D *axis, double *ang );     

	TlTransform Invert() const;
	Boolean UnitTransform();
	friend TlTransform operator*(const TlTransform& t1, const TlTransform& t2 );
	friend Point3D operator*(const Point3D& p, const TlTransform& t );
	void SetVal( int i, int j, float val)
		{ mat[i][j] = val; }
	float GetVal( int i, int j )
		{ return mat[i][j]; }
	ostream& WriteSDF( ostream& os );
};

#endif // TRANSFORM
