/*
 * @(#) Vector3D_Print.c 96/05/11 1.2
 *
 * Copyright (c) 1996, The 3DO Company.  All rights reserved.
 */

#include <stdio.h>
#include "matrix.h"

void Vector3D_Print( Vector3D *v ) {

    printf("Vector3D_Print: ");
    printf("( %3.3f %3.3f %3.3f )\n", (v)->x, (v)->y, (v)->z );
}
