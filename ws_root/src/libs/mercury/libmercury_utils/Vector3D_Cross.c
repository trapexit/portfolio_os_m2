#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

void Vector3D_Cross( Vector3D *v, Vector3D *a, Vector3D *b ) {

    (v)->x = (a)->y * (b)->z - (a)->z * (b)->y;
    (v)->y = (a)->z * (b)->x - (a)->x * (b)->z;
    (v)->z = (a)->x * (b)->y - (a)->y * (b)->x;

}
