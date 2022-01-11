/*
 * @(#) Matrix_MultOrientation.c 96/05/10 1.2
 *
 * Copyright (c) 1996, The 3DO Company.  All rights reserved.
 */

#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

void Matrix_MultOrientation( Matrix *m, Matrix *a, Matrix *b ) {

    int i, j;

    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            (m)->mat[i][j] = (a)->mat[i][0] * (b)->mat[0][j] +
                             (a)->mat[i][1] * (b)->mat[1][j] +
                             (a)->mat[i][2] * (b)->mat[2][j];
        }
    }
}
