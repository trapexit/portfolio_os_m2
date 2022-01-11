#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

void Matrix_TranslateByVector( Matrix *m, Vector3D *v ) {

    (m)->mat[3][0] += (v)->x;
    (m)->mat[3][1] += (v)->y;
    (m)->mat[3][2] += (v)->z;

}
