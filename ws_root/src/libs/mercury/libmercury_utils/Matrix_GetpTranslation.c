#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

Vector3D* Matrix_GetpTranslation( Matrix *m )
{
	return (Vector3D*)(&((m)->mat[3][0]));
}
