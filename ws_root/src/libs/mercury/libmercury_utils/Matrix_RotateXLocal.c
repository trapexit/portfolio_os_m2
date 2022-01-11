#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

void Matrix_RotateXLocal( Matrix *m, float r ) {

    register float cos, sin, tmp;

    if (r != 0.0f) {

        cos = COS(r);
        sin = SIN(r);

        tmp = (m)->mat[0][1];
        (m)->mat[0][1] = cos * tmp - sin * (m)->mat[0][2];
        (m)->mat[0][2] = cos * (m)->mat[0][2] + sin * tmp;
        tmp = (m)->mat[1][1];
        (m)->mat[1][1] = cos * tmp - sin * (m)->mat[1][2];
        (m)->mat[1][2] = cos * (m)->mat[1][2] + sin * tmp;
        tmp = (m)->mat[2][1];
        (m)->mat[2][1] = cos * tmp - sin * (m)->mat[2][2];
        (m)->mat[2][2] = cos * (m)->mat[2][2] + sin * tmp;
    }
}
