/*
 * @(#) Vector3D_MultiplyByMatrix.c 96/05/10 1.2
 *
 * Copyright (c) 1996, The 3DO Company.  All rights reserved
 */

#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

void Vector3D_MultiplyByMatrix( Vector3D *v, Matrix *m ) {

    Vector3D    a;

    Vector3D_Copy((&a), (v));

	(v)->x = (m)->mat[0][0] * (&a)->x +
             (m)->mat[1][0] * (&a)->y +
             (m)->mat[2][0] * (&a)->z +
			 (m)->mat[3][0];

    (v)->y = (m)->mat[0][1] * (&a)->x +
             (m)->mat[1][1] * (&a)->y +
             (m)->mat[2][1] * (&a)->z +
			 (m)->mat[3][1];

    (v)->z = (m)->mat[0][2] * (&a)->x +
             (m)->mat[1][2] * (&a)->y +
             (m)->mat[2][2] * (&a)->z +
			 (m)->mat[3][2];

}
