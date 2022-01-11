#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

void Matrix_Mult( Matrix *m, Matrix *a, Matrix *b ) {

	/*
	 *
	 */
	(m)->mat[0][0] = (a)->mat[0][0] * (b)->mat[0][0] +
			  		 (a)->mat[0][1] * (b)->mat[1][0] +
			  		 (a)->mat[0][2] * (b)->mat[2][0];

	(m)->mat[0][1] = (a)->mat[0][0] * (b)->mat[0][1] +
			  		 (a)->mat[0][1] * (b)->mat[1][1] +
			  		 (a)->mat[0][2] * (b)->mat[2][1];

	(m)->mat[0][2] = (a)->mat[0][0] * (b)->mat[0][2] +
			  		 (a)->mat[0][1] * (b)->mat[1][2] +
			  		 (a)->mat[0][2] * (b)->mat[2][2];

	/*
	 *
	 */
	(m)->mat[1][0] = (a)->mat[1][0] * (b)->mat[0][0] +
			  		 (a)->mat[1][1] * (b)->mat[1][0] +
			  		 (a)->mat[1][2] * (b)->mat[2][0];

	(m)->mat[1][1] = (a)->mat[1][0] * (b)->mat[0][1] +
			  		 (a)->mat[1][1] * (b)->mat[1][1] +
			  		 (a)->mat[1][2] * (b)->mat[2][1];

	(m)->mat[1][2] = (a)->mat[1][0] * (b)->mat[0][2] +
			  		 (a)->mat[1][1] * (b)->mat[1][2] +
			  		 (a)->mat[1][2] * (b)->mat[2][2];

	/*
	 *
	 */
	(m)->mat[2][0] = (a)->mat[2][0] * (b)->mat[0][0] +
			  		 (a)->mat[2][1] * (b)->mat[1][0] +
			  		 (a)->mat[2][2] * (b)->mat[2][0];

	(m)->mat[2][1] = (a)->mat[2][0] * (b)->mat[0][1] +
			  		 (a)->mat[2][1] * (b)->mat[1][1] +
			  		 (a)->mat[2][2] * (b)->mat[2][1];

	(m)->mat[2][2] = (a)->mat[2][0] * (b)->mat[0][2] +
			  		 (a)->mat[2][1] * (b)->mat[1][2] +
			  		 (a)->mat[2][2] * (b)->mat[2][2];

	/*
	 *
	 */
	(m)->mat[3][0] = (a)->mat[3][0] * (b)->mat[0][0] +
			  		 (a)->mat[3][1] * (b)->mat[1][0] +
			  		 (a)->mat[3][2] * (b)->mat[2][0] +
					                  (b)->mat[3][0];

	(m)->mat[3][1] = (a)->mat[3][0] * (b)->mat[0][1] +
			  		 (a)->mat[3][1] * (b)->mat[1][1] +
			  		 (a)->mat[3][2] * (b)->mat[2][1] +
					                  (b)->mat[3][1];

	(m)->mat[3][2] = (a)->mat[3][0] * (b)->mat[0][2] +
			  		 (a)->mat[3][1] * (b)->mat[1][2] +
			  		 (a)->mat[3][2] * (b)->mat[2][2] +
					                  (b)->mat[3][2];
}
