#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

float Vector3D_Length( Vector3D *v ) {

    return ( sqrtf(Vector3D_Dot((v), (v))) );
}
