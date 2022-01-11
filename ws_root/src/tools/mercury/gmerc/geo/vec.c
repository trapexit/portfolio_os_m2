/****
 *
 *	@(#) vec.c 95/03/17 1.19
 *	Copyright 1994, The 3DO Company
 *
 *
 *	Point3, Point4, Vector3 Classes
 *
 ****/
#include "gp.i"
#include <math.h>

#define Trans_GetData(a)		&((Transform *) (a))->data

void Pt3_Print(const Point3 *p) 
{
#ifdef _DEBUG
    GP_Printf(("{ %f %f %f }", (p)->x, (p)->y, (p)->z));
#endif
}

void Vec3_Print(const Vector3 *p) 
{
#ifdef _DEBUG
    GP_Printf(("{ %f %f %f }", (p)->x, (p)->y, (p)->z));
#endif
}

void Pt4_Print(const Point4 *p)
{
#ifdef _DEBUG
    GP_Printf(("{ %f %f %f %f}", (p)->x, (p)->y, (p)->z, (p)->w));
#endif
}


#ifdef NOT_GEOMETRY_LIBRARY
void Vec3_Negate(Vector3* v)
{
	v->x = -v->x; v->y = -v->y; v->z = -v->z;
}
#endif

void Vec3_Add(Vector3* dst, const Vector3* src)
{
	dst->x += src->x;
	dst->y += src->y;
	dst->z += src->z;
}

void Vec3_Subtract(Vector3* dst, const Vector3* src)
{
	dst->x -= src->x;
	dst->y -= src->y;
	dst->z -= src->z;
}


float Vec3_Dot(const Vector3* a, const Vector3* b)
{
	return (a->x * b->x + a->y * b->y + a->z * b->z);
}


void Vec3_Cross(Vector3* dst, const Vector3* src)
/****
 *
 * Returns the cross product of two vectors in this vector.
 *
 ****/
{
	float x, y;

	x = dst->y * src->z - dst->z * src->y;
	y = dst->z * src->x - dst->x * src->z;
	dst->z = dst->x * src->y - dst->y * src->x;
	dst->x = x; dst->y = y;
}

float Vec3_Length(const Vector3* v)
{
	return (sqrt(v->x * v->x + v->y * v->y + v->z * v->z));
}

float Vec3_LengthSquared(const Vector3* v)
{
	return (v->x * v->x + v->y * v->y + v->z * v->z);
}


float Pt3_Distance(const Vector3* a, const Vector3* b)
{
	float s;

	s = sqrt(SQ(a->x - b->x) + SQ(a->y - b->y) + SQ(a->z - b->z));
	return s;
}

void Vec3_Transform(Vector3* v, const Transform* mtx)
{
	float x, y;
	const MatrixData* m = Trans_GetData(mtx);

    x = v->x * (*m)[0][0] + v->y * (*m)[1][0] + v->z * (*m)[2][0];
    y = v->x * (*m)[0][1] + v->y * (*m)[1][1] + v->z * (*m)[2][1];
    v->z = v->x * (*m)[0][2] + v->y * (*m)[1][2] + v->z * (*m)[2][2];
	v->x = x; v->y = y;
}


void Vec3_TransMany(Vector3* v, const Transform* mtx, int32 n)
{
	float x, y;
	const MatrixData* m = Trans_GetData(mtx);

	while (--n >= 0)
	   {
    	x = v->x * (*m)[0][0] + v->y * (*m)[1][0] + v->z * (*m)[2][0];
    	y = v->x * (*m)[0][1] + v->y * (*m)[1][1] + v->z * (*m)[2][1];
    	v->z = v->x * (*m)[0][2] + v->y * (*m)[1][2] + v->z * (*m)[2][2];
		v->x = x; v->y = y;
		++v;
	   }
}


void Pt3_Transform(Point3* p, const Transform* mtx)
{
	float x, y;
	const MatrixData* m = Trans_GetData(mtx);

    x = p->x * (*m)[0][0] + p->y * (*m)[1][0] + p->z * (*m)[2][0] + (*m)[3][0];
    y = p->x * (*m)[0][1] + p->y * (*m)[1][1] + p->z * (*m)[2][1] + (*m)[3][1];
    p->z = p->x * (*m)[0][2] + p->y * (*m)[1][2] + p->z * (*m)[2][2] + (*m)[3][2];
	p->x = x; p->y = y;
}

void Pt3_TransMany(Point3* p, const Transform* mtx, int32 n)
{
	register float x, y, z;
	const MatrixData* m = Trans_GetData(mtx);
	
	while (--n >= 0) {
		x = p->x;
		y = p->y;
		z = p->z;
	    p->x = (x * (*m)[0][0] + y * (*m)[1][0] + z * (*m)[2][0]) + (*m)[3][0];
		p->y = (x * (*m)[0][1] + y * (*m)[1][1] + z * (*m)[2][1]) + (*m)[3][1];
		p->z = (x * (*m)[0][2] + y * (*m)[1][2] + z * (*m)[2][2]) + (*m)[3][2];
		++p;
	}
}


void Pt4_Transform(Point4* p, const Transform* mtx)
{
	float x, y, z;
	const MatrixData* m = Trans_GetData(mtx);

    x = p->x * (*m)[0][0] + p->y * (*m)[1][0] + p->z * (*m)[2][0] + p->w *(*m)[3][0];
    y = p->x * (*m)[0][1] + p->y * (*m)[1][1] + p->z * (*m)[2][1] + p->w *(*m)[3][1];
    z = p->x * (*m)[0][2] + p->y * (*m)[1][2] + p->z * (*m)[2][2] + p->w *(*m)[3][2];
    p->w = p->x * (*m)[0][3] + p->y * (*m)[1][3] + p->z * (*m)[2][3] + p->w *(*m)[3][3];
	p->x = x; p->y = y; p->z = z;
}


void Pt4_TransMany(Point4* p, const Transform* mtx, int32 n)
{
	float x, y, z;
	const MatrixData* m = Trans_GetData(mtx);

	while (--n >= 0)
	   {
	    x = p->x * (*m)[0][0] + p->y * (*m)[1][0] +
			p->z * (*m)[2][0] + p->w *(*m)[3][0];
	    y = p->x * (*m)[0][1] + p->y * (*m)[1][1] +
			p->z * (*m)[2][1] + p->w *(*m)[3][1];
	    z = p->x * (*m)[0][2] + p->y * (*m)[1][2] +
			p->z * (*m)[2][2] + p->w *(*m)[3][2];
	    p->w = p->x * (*m)[0][3] + p->y * (*m)[1][3] +
			p->z * (*m)[2][3] + p->w *(*m)[3][3];
		p->x = x; p->y = y; p->z = z;
		++p;
	    }
}

float Vec3_Normalize(Vector3* v)
{
    float divisor;

    divisor = Vec3_Length(v);
    if (divisor > 0.0)
	   {
        v->x /= divisor;
        v->y /= divisor;
        v->z /= divisor;
	   }
    return divisor;
}

