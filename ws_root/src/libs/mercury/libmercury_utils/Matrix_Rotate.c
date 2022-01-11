#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

void Matrix_Rotate( Matrix *m, char axis, float r ) {

    switch (axis) {

        case 'x':
        case 'X':
            Matrix_RotateX((m), (r));
            break;

        case 'y':
        case 'Y':
            Matrix_RotateY((m), (r));
            break;

        case 'z':
        case 'Z':
            Matrix_RotateZ((m), (r));
            break;

        default:
            break;
    }
}
