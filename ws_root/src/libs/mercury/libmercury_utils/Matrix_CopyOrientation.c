#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

void Matrix_CopyOrientation( Matrix *m, Matrix *b ) {

	m->mat[0][0] = b->mat[0][0];
	m->mat[1][0] = b->mat[1][0];
	m->mat[2][0] = b->mat[2][0];

	m->mat[0][1] = b->mat[0][1];
	m->mat[1][1] = b->mat[1][1];
	m->mat[2][1] = b->mat[2][1];

	m->mat[0][2] = b->mat[0][2];
	m->mat[1][2] = b->mat[1][2];
	m->mat[2][2] = b->mat[2][2];
}
