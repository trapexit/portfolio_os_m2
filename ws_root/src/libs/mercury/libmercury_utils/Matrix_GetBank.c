#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

float Matrix_GetBank( Matrix *m ) {

	Vector3D	direction, axisY;
	float 		bank;

	Vector3D_Set( &axisY, 0.0f, 1.0f, 0.0f );
	Vector3D_Set( &direction, m->mat[0][0], m->mat[0][1], m->mat[0][2] );
	Vector3D_Normalize( &direction );

	bank = Vector3D_Dot( &axisY, &direction );

	bank = ASIN( bank );

	if ( m->mat[1][1] < 0.0f ) {
		bank = RAD180 - bank;
		if ( bank >= RAD180 ) {
			bank -= RAD360;
		}
	}

	return bank;

}
