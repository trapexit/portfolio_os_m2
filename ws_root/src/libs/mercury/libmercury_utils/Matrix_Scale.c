#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

void Matrix_Scale( Matrix *m, float x, float y, float z ) {

    (m)->mat[0][0] *= x;
    (m)->mat[1][0] *= x;
    (m)->mat[2][0] *= x;
    (m)->mat[3][0] *= x;

    (m)->mat[0][1] *= y;
    (m)->mat[1][1] *= y;
    (m)->mat[2][1] *= y;
    (m)->mat[3][1] *= y;

    (m)->mat[0][2] *= z;
    (m)->mat[1][2] *= z;
    (m)->mat[2][2] *= z;
    (m)->mat[3][2] *= z;

}
