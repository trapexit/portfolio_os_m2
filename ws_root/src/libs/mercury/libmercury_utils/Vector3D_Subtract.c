#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

void Vector3D_Subtract( Vector3D *v, Vector3D *a, Vector3D *b ) {

    (v)->x = (a)->x - (b)->x;
    (v)->y = (a)->y - (b)->y;
    (v)->z = (a)->z - (b)->z;
}
