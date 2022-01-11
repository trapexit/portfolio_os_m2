#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

void Vector3D_Set( Vector3D *v, float x, float y, float z ) {
    (v)->x = x;
    (v)->y = y;
    (v)->z = z;
}
