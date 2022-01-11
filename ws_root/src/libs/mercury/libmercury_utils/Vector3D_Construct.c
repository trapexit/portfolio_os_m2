/*
 * @(#) Vector3D_Construct.c 96/05/11 1.3
 *
 * Copyright (c) 1996, The 3DO Company.  All rights reserved.
 */

#ifdef MACINTOSH
#include <kernel:mem.h>
#else
#include <kernel/mem.h>
#endif
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

Vector3D* Vector3D_Construct( void ) {

    Vector3D *v;

    v = AllocMem(sizeof(Vector3D), MEMTYPE_NORMAL);
    Vector3D_Zero( v );

    return v;

}
