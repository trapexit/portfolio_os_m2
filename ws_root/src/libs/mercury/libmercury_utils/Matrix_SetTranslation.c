#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

void Matrix_SetTranslation( Matrix *m, float x, float y, float z ) {

    (m)->mat[3][0] = x;
    (m)->mat[3][1] = y;
    (m)->mat[3][2] = z;
}
