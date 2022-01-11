#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

/* MATRIX INVERSION
 *
 * Routines to invert a matrix
 *
 * IMPLEMENTATION
 *
 * The code is based on the code presented in "Graphics Gems"
 * (ed. Andrew Glassner), in "Matrix Inversion" (pg. 766).  The code
 * has been modified to work with the 3x4 matrices we use.  We don't
 * have a fourth column, so it is hardwired as (0, 0, 0, 1).
 *
 * WARNING - TRUE MATRIX INVERSION REQUIRES SQUARE MATRICES, the 3x4
 * matrix results may not be what is desired.
 */

#define INLINE_BUG

#ifndef INLINE_BUG
#pragma inline det2x2
#endif

#pragma inline det3x3, det4x4, adjoint
#pragma pure_function det2x2, det3x3, det4x4, adjoint

/* det2x2 - calulate the determinent of a 2x2 matrix passed in as
 * discrete numbers
 *
 * RETURN VALUE
 * The determinent of the matrix.
 */

#ifdef INLINE_BUG

#define det2x2(a1, a2, b1, b2)	((a1) * (b2) - (a2) * (b1))

#else

static float det2x2(float a1, float a2, float b1, float b2)

{
    return(a1 * b2 - a2 * b1);
}

#endif

/* det3x3 - calulate the determinent of a 3x3 matrix passed in as
 * discrete numbers
 *
 * RETURN VALUE
 * The determinent of the matrix.
 */

static float det3x3(float a1, float a2, float a3,
		    float b1, float b2, float b3,
		    float c1, float c2, float c3)

{
    float ans;

    ans = (  a1 * det2x2(b2, b3, c2, c3)
	   - b1 * det2x2(a2, a3, c2, c3)
	   + c1 * det2x2(a2, a3, b2, b3));

    return(ans);
}

/* det4x4 - calulate the determinent of a 4x4 matrix
 *
 * RETURN VALUE
 * The determinent of the matrix.
 */

static float det4x4(pMatrix matrix)

{
    float a1, a2, a3, a4, b1, b2, b3, b4, c1, c2, c3, c4;
    float ans;

    a1 = matrix->mat[0][0]; b1 = matrix->mat[0][1]; c1 = matrix->mat[0][2];
    a2 = matrix->mat[1][0]; b2 = matrix->mat[1][1]; c2 = matrix->mat[1][2];
    a3 = matrix->mat[2][0]; b3 = matrix->mat[2][1]; c3 = matrix->mat[2][2];
    a4 = matrix->mat[3][0]; b4 = matrix->mat[3][1]; c4 = matrix->mat[3][2];

    ans = (  a1 * det3x3(b2, b3, b4, c2, c3, c4, 0.0, 0.0, 1.0)
	   - b1 * det3x3(a2, a3, a4, c2, c3, c4, 0.0, 0.0, 1.0)
	   + c1 * det3x3(a2, a3, a4, b2, b3, b4, 0.0, 0.0, 1.0));

    return(ans);
}

/* adjoint - calculate the adjoint of a matrix
 *
 * Let a   denote the minor determinant of matrix A obtained by
 *      ij
 * deleting the ith row and jth column from A.
 *
 *               i+j
 * Let b   = (-1)   a
 *      ij           ji
 *
 * The matrix B = (b  ) is the adjoint of A.
 *                  ij
 */

static adjoint(pMatrix src, pMatrix dst)

{
    float a1, a2, a3, a4, b1, b2, b3, b4, c1, c2, c3, c4;

    a1 = src->mat[0][0]; b1 = src->mat[0][1]; c1 = src->mat[0][2];
    a2 = src->mat[1][0]; b2 = src->mat[1][1]; c2 = src->mat[1][2];
    a3 = src->mat[2][0]; b3 = src->mat[2][1]; c3 = src->mat[2][2];
    a4 = src->mat[3][0]; b4 = src->mat[3][1]; c4 = src->mat[3][2];

    dst->mat[0][0] =   det3x3(b2, b3, b4, c2, c3, c4, 0.0, 0.0, 1.0);
    dst->mat[1][0] = - det3x3(a2, a3, a4, c2, c3, c4, 0.0, 0.0, 1.0);
    dst->mat[2][0] =   det3x3(a2, a3, a4, b2, b3, b4, 0.0, 0.0, 1.0);
    dst->mat[3][0] = - det3x3(a2, a3, a4, b2, b3, b4, c2, c3, c4);

    dst->mat[0][1] = - det3x3(b1, b3, b4, c1, c3, c4, 0.0, 0.0, 1.0);
    dst->mat[1][1] =   det3x3(a1, a3, a4, c1, c3, c4, 0.0, 0.0, 1.0);
    dst->mat[2][1] = - det3x3(a1, a3, a4, b1, b3, b4, 0.0, 0.0, 1.0);
    dst->mat[3][1] =   det3x3(a1, a3, a4, b1, b3, b4, c1, c3, c4);

    dst->mat[0][2] =   det3x3(b1, b2, b4, c1, c2, c4, 0.0, 0.0, 1.0);
    dst->mat[1][2] = - det3x3(a1, a2, a4, c1, c2, c4, 0.0, 0.0, 1.0);
    dst->mat[2][2] =   det3x3(a1, a2, a4, b1, b2, b4, 0.0, 0.0, 1.0);
    dst->mat[3][2] = - det3x3(a1, a2, a4, b1, b2, b4, c1, c2, c4);
}

/* Invert src and place the result in dst.
 *
 * RETURN VALUE
 * -1               No value can be calculated
 * 0                Everything is OK
 */

Err Matrix_FullInvert(pMatrix dst, pMatrix src) {

    int i, j;
    float det;

    /* Calculate the adjoint matrix */
    adjoint(src, dst);

    /* Calculate the 4/4 determinent.  If the determinent is zero,
     * then the inverse matrix is not unique.
     */
    det = det4x4(dst);
    if (fabsf(det) < FLT_EPSILON) {
	return(-1);
    }

    /* Scale the adjoint matrix to get the inverse. */
    for (i = 0; i < 4; i++)
	for (j = 0; j < 3; j++)
	    dst->mat[i][j] /= det;

    return(0);
}
