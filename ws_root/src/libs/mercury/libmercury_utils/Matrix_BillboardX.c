#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

void Matrix_BillboardX( Matrix *m, Vector3D *v ) {

	float deltay,deltaz;
	float sin,cos;
	float len;

	deltay = v->y - m->mat[3][1];
	deltaz = v->z - m->mat[3][2];

	len = rsqrtff((deltay*deltay)+(deltaz*deltaz)+.000001);

	sin = deltaz * len;
	cos = deltay * len;

	m->mat[0][0] = 1.0;
	m->mat[0][1] = 0.0;
	m->mat[0][2] = 0.0;

	m->mat[1][0] = 0.0;
	m->mat[1][1] = cos;
	m->mat[1][2] = sin;

	m->mat[2][0] = 0.0;
	m->mat[2][1] = -sin;
	m->mat[2][2] = cos;
}
