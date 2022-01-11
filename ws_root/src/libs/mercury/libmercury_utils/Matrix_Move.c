#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

void Matrix_Move( Matrix *m, float x, float y, float z ) {

    Vector3D    d;

    Vector3D_Set(&d, (x), (y), (z));
    Matrix_MoveByVector((m), &d);

}
