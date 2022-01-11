#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

void Matrix_RotateYLocal( Matrix *m, float r ) {

    register float cos, sin, tmp;

    if (r != 0.0f) {

        cos = COS(r);
        sin = SIN(r);

        tmp = (m)->mat[0][0];
        (m)->mat[0][0] = cos * tmp + sin * (m)->mat[0][2];
        (m)->mat[0][2] = -sin * tmp + cos * (m)->mat[0][2];

		tmp = (m)->mat[1][0];
        (m)->mat[1][0] = cos * tmp + sin * (m)->mat[1][2];
        (m)->mat[1][2] = -sin * tmp + cos * (m)->mat[1][2];

		tmp = (m)->mat[2][0];
        (m)->mat[2][0] = cos * tmp + sin * (m)->mat[2][2];
        (m)->mat[2][2] = -sin * tmp + cos * (m)->mat[2][2];
    }
}
