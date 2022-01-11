#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

void Matrix_Zero( Matrix *m ) {

    memset( (m), 0, sizeof( Matrix ) );

}
