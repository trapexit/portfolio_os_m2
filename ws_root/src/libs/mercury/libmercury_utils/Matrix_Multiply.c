#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

void Matrix_Multiply( Matrix *m, Matrix *a ) {

    Matrix      b;

    Matrix_Copy(&(b), (m));
    Matrix_Mult((m), &(b), (a));

}
