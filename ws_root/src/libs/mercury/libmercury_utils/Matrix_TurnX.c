#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

void Matrix_TurnX( Matrix *m, float r ) {

    Matrix  a, b;

    Matrix_Identity( &a );
    Matrix_RotateX( &a, (r));

	Matrix_Copy( &b, m );
    Matrix_Mult( (m), &a, &b );
}
