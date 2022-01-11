#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

void Vector3D_Zero( Vector3D *v ) {

    (v)->x = 0.0;
    (v)->y = 0.0;
    (v)->z = 0.0;
}
