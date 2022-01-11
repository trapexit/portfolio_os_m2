#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

/* Computes a perspective matrix that is concatenated on to the camera matrix */

void Matrix_Perspective(pMatrix matrix, pViewPyramid p,
		    float screen_xmin, float screen_xmax,
		    float screen_ymin, float screen_ymax, float wscale)
{
    float dx = screen_xmax-screen_xmin;
    float dy = screen_ymax-screen_ymin;
    float rl = p->right-p->left;
    float tb = p->top-p->bottom;

    matrix->mat[0][0] = p->hither * dx * wscale/rl;
    matrix->mat[0][1] = 0.0;
    matrix->mat[0][2] = 0.0;

	matrix->mat[1][0] = 0.0;
	matrix->mat[1][1] = - p->hither * dy * wscale/tb;
    matrix->mat[1][2] = 0.0;

    matrix->mat[2][0] = ((p->right+p->left)*dx/(2.*rl) - (screen_xmax+screen_xmin)/2.)*wscale;
    matrix->mat[2][1] =  (-(p->top+p->bottom)*dy/(2.*tb) - (screen_ymax+screen_ymin)/2.)*wscale;
    matrix->mat[2][2] = -wscale;

    matrix->mat[3][0] = 0.;
    matrix->mat[3][1] = 0.;
    matrix->mat[3][2] = 0.;
}
