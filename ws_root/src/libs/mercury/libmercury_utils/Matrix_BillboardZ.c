#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

void Matrix_BillboardZ( Matrix *m, Vector3D *v ) {

	float deltax,deltay;
	float sin,cos;
	float len;

	deltax = v->x - m->mat[3][0];
	deltay = v->y - m->mat[3][1];

	len = rsqrtff((deltax*deltax)+(deltay*deltay)+.000001);

	sin = deltay * len;
	cos = deltax * len;

	m->mat[0][0] = cos;
	m->mat[0][1] = sin;
	m->mat[0][2] = 0.0;

	m->mat[1][0] = -sin;
	m->mat[1][1] = cos;
	m->mat[1][2] = 0.0;

	m->mat[2][0] = 0.0;
	m->mat[2][1] = 0.0;
	m->mat[2][2] = 1.0;
}
