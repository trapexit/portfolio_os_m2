#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

void Matrix_Identity( Matrix *m ) {

    Matrix_Copy( (m), &matrixIdentity );

}
