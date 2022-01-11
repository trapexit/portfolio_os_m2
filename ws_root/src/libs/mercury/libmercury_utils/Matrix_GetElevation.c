#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

float Matrix_GetElevation( Matrix *m ) {

	Vector3D	direction, axisY;
	float 		elevation;

	Vector3D_Set( &axisY, 0.0f, 1.0f, 0.0f );
	Vector3D_Set( &direction, m->mat[2][0], m->mat[2][1], m->mat[2][2] );
	Vector3D_Normalize( &direction );

	elevation = Vector3D_Dot( &axisY, &direction );

	elevation = ASIN( elevation );

	if ( m->mat[1][1] < 0.0f ) {
		elevation = RAD180 + elevation;
		if ( elevation >= RAD180 ) {
			elevation -= RAD360;
		}
	}
	elevation *= -1 ;

	return elevation;

}
