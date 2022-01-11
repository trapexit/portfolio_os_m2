#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

void Matrix_Copy( Matrix *m, Matrix *a ) {

    memcpy( (m), (a), sizeof(Matrix) );

}
