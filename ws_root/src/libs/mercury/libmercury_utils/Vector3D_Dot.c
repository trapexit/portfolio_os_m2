#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

float Vector3D_Dot( Vector3D *v, Vector3D *a ) {

    return ( (v)->x * (a)->x +
             (v)->y * (a)->y +
             (v)->z * (a)->z );
}
