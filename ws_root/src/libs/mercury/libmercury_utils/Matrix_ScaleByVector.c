#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

void Matrix_ScaleByVector( Matrix *m, Vector3D *v ) {

    Matrix_Scale( m, (v)->x, (v)->y, (v)->z );
}
