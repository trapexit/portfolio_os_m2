/*
 * @(#) Matrix_Destruct.c 96/05/11 1.2
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

void Matrix_Destruct( Matrix *matrix ) {

    assert( matrix );
    FreeMem( matrix, sizeof(Matrix) );

}
