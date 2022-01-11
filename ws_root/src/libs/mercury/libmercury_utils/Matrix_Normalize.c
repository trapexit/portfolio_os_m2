#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

void Matrix_Normalize( Matrix *m ) {

    float normlen;

    /*
     * Normalise X-Axis
     */
    normlen = sqrtf( (m)->mat[0][0] * (m)->mat[0][0] +
                     (m)->mat[1][0] * (m)->mat[1][0] +
                     (m)->mat[2][0] * (m)->mat[2][0] );

    if (normlen != 0.0) {
        (m)->mat[0][0] = (m)->mat[0][0] / normlen;
        (m)->mat[1][0] = (m)->mat[1][0] / normlen;
        (m)->mat[2][0] = (m)->mat[2][0] / normlen;
    }

    /*
     * Normalise Y-Axis
     */
    normlen = sqrtf( (m)->mat[0][1] * (m)->mat[0][1] +
                     (m)->mat[1][1] * (m)->mat[1][1] +
                     (m)->mat[2][1] * (m)->mat[2][1] );

    if (normlen != 0.0) {
        (m)->mat[0][1] = (m)->mat[0][1] / normlen;
        (m)->mat[1][1] = (m)->mat[1][1] / normlen;
        (m)->mat[2][1] = (m)->mat[2][1] / normlen;
    }

    /*
     * Normalise Z-Axis
     */
    normlen = sqrtf( (m)->mat[0][2] * (m)->mat[0][2] +
                     (m)->mat[1][2] * (m)->mat[1][2] +
                     (m)->mat[2][2] * (m)->mat[2][2] );

    if (normlen != 0.0) {
        (m)->mat[0][2] = (m)->mat[0][2] / normlen;
        (m)->mat[1][2] = (m)->mat[1][2] / normlen;
        (m)->mat[2][2] = (m)->mat[2][2] / normlen;
    }
}
