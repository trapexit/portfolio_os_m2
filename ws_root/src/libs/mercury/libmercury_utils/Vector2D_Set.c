#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

void Vector2D_Set( Vector2D *v, float x, float y ) {
    (v)->x = x;
    (v)->y = y;
}
