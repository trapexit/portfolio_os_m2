#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

void Vector3D_Average( Vector3D *v, Vector3D *a, Vector3D *b) {

    (v)->x = (((a)->x + (b)->x) / 2.0f);
    (v)->y = (((a)->y + (b)->y) / 2.0f);
    (v)->z = (((a)->z + (b)->z) / 2.0f);
}
