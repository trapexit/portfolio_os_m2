/*
 * @(#) Matrix_Print.c 96/05/11 1.2
 *
 * Copyright (c) 1996, The 3DO Company.  All rights reserved.
 */

#include <stdio.h>
#include "matrix.h"

void Matrix_Print( Matrix *m ) {

    printf("Matrix_Print:\n");
    printf("[ %3.3f, %3.3f, %3.3f ]\n",
        (m)->mat[0][0], (m)->mat[0][1], (m)->mat[0][2] );
    printf("[ %3.3f, %3.3f, %3.3f ]\n",
        (m)->mat[1][0], (m)->mat[1][1], (m)->mat[1][2] );
    printf("[ %3.3f, %3.3f, %3.3f ]\n",
        (m)->mat[2][0], (m)->mat[2][1], (m)->mat[2][2] );
    printf("[ %3.3f, %3.3f, %3.3f ]\n",
        (m)->mat[3][0], (m)->mat[3][1], (m)->mat[3][2] );
}
