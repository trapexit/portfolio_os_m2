/*
 * @(#) Matrix_CopyTranslation.c 96/05/10 1.2
 *
 * Copyright (c) 1996, The 3DO Company.  All rights reserved.
 */

#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

void Matrix_CopyTranslation( Matrix *m, Matrix *b ) {

	m->mat[3][0] = b->mat[3][0];
	m->mat[3][1] = b->mat[3][1];
	m->mat[3][2] = b->mat[3][2];

}
