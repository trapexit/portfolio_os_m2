#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

void Matrix_RotateLocal( Matrix *m, char axis, float r ) {

    switch (axis) {

        case 'x':
        case 'X':
            Matrix_RotateXLocal((m), (r));
            break;

        case 'y':
        case 'Y':
            Matrix_RotateYLocal((m), (r));
            break;

        case 'z':
        case 'Z':
            Matrix_RotateZLocal((m), (r));
            break;

        default:
            break;
    }
}
