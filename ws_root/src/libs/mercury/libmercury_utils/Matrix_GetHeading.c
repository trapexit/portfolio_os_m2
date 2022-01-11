#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

float Matrix_GetHeading( Matrix *m ) {

	Vector3D	direction;
	float 		heading;

	Vector3D_Set( &direction, m->mat[2][0], 0.0f, m->mat[2][2] );
	Vector3D_Normalize( &direction );

	heading = ASIN( direction.x );
	if ( direction.z >= 0.0f ) {
		if ( direction.x < 0.0f ) {
			heading = -RAD180 - heading;
		} else {
			heading = RAD180 - heading;
		}
	}
	heading += RAD180;
	if ( heading > RAD180 ) {
		heading -= RAD360;
	}

	return heading;

}
