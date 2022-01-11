/*
 * @(#) Matrix_MultiplyOrientation.c 96/05/10 1.3
 *
 * Copyright (c) 1996, The 3DO Company.  All rights reserved.
 */

#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

void Matrix_MultiplyOrientation( Matrix *m, Matrix *a ) {

    Matrix      b;

    Matrix_Copy(&(b), (m));
    Matrix_MultOrientation((m), &(b), (a));

}
