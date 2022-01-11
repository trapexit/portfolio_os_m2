#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

/*
 * MATRIX INVERSION
 * assumes different source and destination matrices
 */
void Matrix_Invert( Matrix *dst, Matrix *src ) {

	dst->mat[0][0] = src->mat[0][0];
	dst->mat[0][1] = src->mat[1][0];
	dst->mat[0][2] = src->mat[2][0];
	dst->mat[1][0] = src->mat[0][1];
	dst->mat[1][1] = src->mat[1][1];
	dst->mat[1][2] = src->mat[2][1];
	dst->mat[2][0] = src->mat[0][2];
	dst->mat[2][1] = src->mat[1][2];
	dst->mat[2][2] = src->mat[2][2];

	dst->mat[3][0] = -(src->mat[3][0] * src->mat[0][0])
				-(src->mat[3][1] * src->mat[0][1])
				-(src->mat[3][2] * src->mat[0][2]);
	dst->mat[3][1] = -(src->mat[3][0] * src->mat[1][0])
				-(src->mat[3][1] * src->mat[1][1])
				-(src->mat[3][2] * src->mat[1][2]);
	dst->mat[3][2] = -(src->mat[3][0] * src->mat[2][0])
				-(src->mat[3][1] * src->mat[2][1])
				-(src->mat[3][2] * src->mat[2][2]);
}
