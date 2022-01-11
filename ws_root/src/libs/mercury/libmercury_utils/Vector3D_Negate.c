#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

void Vector3D_Negate( Vector3D *v ) {

    (v)->x = -((v)->x);
    (v)->y = -((v)->y);
    (v)->z = -((v)->z);
}
