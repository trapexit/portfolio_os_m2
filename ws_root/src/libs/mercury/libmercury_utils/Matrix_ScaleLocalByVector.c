#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

void Matrix_ScaleLocalByVector( Matrix *m, Vector3D *v ) {

    Matrix_ScaleLocal(m, (v)->x, (v)->y, (v)->z );
}
