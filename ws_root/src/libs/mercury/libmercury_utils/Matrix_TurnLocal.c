#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

void Matrix_TurnLocal( Matrix *m, char axis, float r ) {

    switch (axis) {
        case 'x':
        case 'X':
            Matrix_TurnXLocal((m),(r));
            break;

        case 'y':
        case 'Y':
            Matrix_TurnYLocal((m),(r));
            break;

        case 'z':
        case 'Z':
            Matrix_TurnZLocal((m),r);
            break;

        default:
            break;
    }
}
