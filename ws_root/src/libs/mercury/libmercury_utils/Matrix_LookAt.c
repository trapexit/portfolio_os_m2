#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include "matrix.h"

void Matrix_LookAt( Matrix *m, Vector3D *pWhere, Vector3D *pMe, float twist ) {

    float   deltax,deltay,deltaz;
    float   sx,cx,sy,cy,sz,cz,sysx,cysx;
    float   lenxyz,lenxz,sqrtxz;

    deltax = pWhere->x - pMe->x;
    deltay = pWhere->y - pMe->y;
    deltaz = pWhere->z - pMe->z;

    sqrtxz = (deltax*deltax)+(deltaz*deltaz)+.000001;
    lenxz = rsqrtff(sqrtxz);
    lenxyz = rsqrtff(sqrtxz+(deltay*deltay));
    sqrtxz = sqrtf(sqrtxz);

    sx = deltay * lenxyz;
    cx = sqrtxz * lenxyz;
    sy = -deltax * lenxz;
    cy = -deltaz * lenxz;

    sz = sinf(twist);
    cz = cosf(twist);

    sysx = sy*sx;
    cysx = cy*sx;

    m->mat[0][0] = (cy*cz)+(sysx*sz);
    m->mat[0][1] = cx*sz;
    m->mat[0][2] = -(sy*cz)+(cysx*sz);

    m->mat[1][0] = -(cy*sz)+(sysx*cz);
    m->mat[1][1] = cx*cz;
    m->mat[1][2] = (sy*sz)+(cysx*cz);

    m->mat[2][0] = sy*cx;
    m->mat[2][1] = -sx;
    m->mat[2][2] = cy*cx;
}
