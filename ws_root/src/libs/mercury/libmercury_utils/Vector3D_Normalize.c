#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

void Vector3D_Normalize( Vector3D *v ) {

    float normlen = Vector3D_Length(v);

    if (normlen != 0.0) {
        (v)->x = (v)->x / normlen;
        (v)->y = (v)->y / normlen;
        (v)->z = (v)->z / normlen;
    }
}
