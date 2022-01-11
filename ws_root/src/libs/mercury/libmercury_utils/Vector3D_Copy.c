#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

void Vector3D_Copy( Vector3D *v, Vector3D *a ) {

    (v)->x = (a)->x;
    (v)->y = (a)->y;
    (v)->z = (a)->z;
}
