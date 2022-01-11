#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

void Matrix_BillboardY( Matrix *m, Vector3D *v ) {

	float deltax,deltaz;
	float sin,cos;
	float len;

	deltax = v->x - m->mat[3][0];
	deltaz = v->z - m->mat[3][2];

	len = rsqrtff((deltax*deltax)+(deltaz*deltaz)+.000001);

	sin = deltax * len;
	cos = deltaz * len;

	m->mat[0][0] = cos;
	m->mat[0][1] = 0.0;
	m->mat[0][2] = -sin;

	m->mat[1][0] = 0.0;
	m->mat[1][1] = 1.0;
	m->mat[1][2] = 0.0;

	m->mat[2][0] = sin;
	m->mat[2][1] = 0.0;
	m->mat[2][2] = cos;
}
