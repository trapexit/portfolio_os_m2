#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

void Matrix_Turn( Matrix *m, char axis, float r ) {

    switch (axis) {
        case 'x':
        case 'X':
            Matrix_TurnX((m),(r));
            break;

        case 'y':
        case 'Y':
            Matrix_TurnY((m),(r));
            break;

        case 'z':
        case 'Z':
            Matrix_TurnZ((m),r);
            break;

        default:
            break;
    }
}
