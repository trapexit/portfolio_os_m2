#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

void Matrix_RotateZLocal( Matrix *m, float r ) {

    register float cos, sin, tmp;

    if (r != 0.0f) {

        cos = COS(r);
        sin = SIN(r);

        tmp = (m)->mat[0][0];
        (m)->mat[0][0] = cos * tmp - sin * (m)->mat[0][1];
        (m)->mat[0][1] = cos * (m)->mat[0][1] + sin * tmp;
        tmp = (m)->mat[1][0];
        (m)->mat[1][0] = cos * tmp - sin * (m)->mat[1][1];
        (m)->mat[1][1] = cos * (m)->mat[1][1] + sin * tmp;
        tmp = (m)->mat[2][0];
        (m)->mat[2][0] = cos * tmp - sin * (m)->mat[2][1];
        (m)->mat[2][1] = cos * (m)->mat[2][1] + sin * tmp;
    }
}
