#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

void Matrix_MoveByVector( Matrix *m, Vector3D *v ) {

	Vector3D	t;

	Vector3D_Copy(&t, v);
    Vector3D_OrientateByMatrix(&t, m);
    Matrix_TranslateByVector(m, &t);

}
