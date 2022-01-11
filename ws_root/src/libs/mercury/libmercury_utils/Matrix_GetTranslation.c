#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

void Matrix_GetTranslation( Matrix *m, Vector3D *v ) {

    (v)->x = (m)->mat[3][0];
    (v)->y = (m)->mat[3][1];
    (v)->z = (m)->mat[3][2];
}
