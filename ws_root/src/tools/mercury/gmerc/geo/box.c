/****
 *
 *	@(#) box.c 95/10/30 1.20
 *	Copyright 1994, The 3DO Company
 *
 *	Box3: Bounding Volume Class
 *
 ****/
#include "gp.i"

void Box3_Set(Box3* b, float xmin, float ymin, float zmin,
			  float xmax, float ymax, float zmax)
{

	b->min.x = xmin;
	b->max.x = xmax;
	b->min.y = ymin;
	b->max.y = ymax;
	b->min.z = zmin;
	b->max.z = zmax;

}

void Box3_Print(const Box3* b)
{
#ifdef _DEBUG
	GP_Printf(("{ %f %f %f   %f %f %f }",
				b->min.x, b->min.y, b->min.z,
				b->max.x, b->max.y, b->max.z));
#endif
}

#ifdef NOT_GEOMETRY_LIBRARY
void Box3_Normalize(Box3* b)
{
	float tmp;

	if (b->min.x > b->max.x)
	   { tmp = b->min.x; b->min.x = b->max.x; b->max.x = tmp; }
	if (b->min.y > b->max.y)
	   { tmp = b->min.y; b->min.y = b->max.y; b->max.y = tmp; }
	if (b->min.z > b->max.z)
	   { tmp = b->min.z; b->min.z = b->max.z; b->max.z = tmp; }

}
#endif

void Box3_Transform(Box3* b, const Transform* matrix)
{
  	float	xthick = Box3_Width(b);
	float	ythick = Box3_Height(b);
    Point3	pts[8];
/*
 * Get the box's 8 corners
 */

    pts[0] = b->min;
    pts[1].x = pts[0].x + xthick;
    pts[1].y = pts[0].y;
    pts[1].z = pts[0].z;
    pts[2].x = pts[1].x;
    pts[2].y = pts[0].y + ythick;
    pts[2].z = pts[0].z;
    pts[3].x = pts[0].x;
    pts[3].y = pts[2].y;
    pts[3].z = pts[0].z;

    pts[4] = b->max;
    pts[5].x = pts[4].x - xthick;
    pts[5].y = pts[4].y;
    pts[5].z = pts[4].z;
    pts[6].x = pts[5].x;
    pts[6].y = pts[4].y - ythick;
    pts[6].z = pts[4].z;
    pts[7].x = pts[4].x;
    pts[7].y = pts[6].y;
    pts[7].z = pts[4].z;
/*
 * Transform the 8 corners and compute a new box that encloses
 * the transformed points
 */
    Pt3_TransMany(pts, matrix, 8);
	/* <HPP> the points represent a new bounding so we should not use   *
	 * Box3_ExtendPts since this causes the scaling of the bounding box */
	{
 		int32	i;
		b->min = pts[0];	/* initialize them to a valid values */
		b->max = pts[0];		
		
		for(i=0;i<8;i++) {
			if (pts[i].x > b->max.x) b->max.x = pts[i].x;
			if (pts[i].x < b->min.x) b->min.x = pts[i].x;
			if (pts[i].y > b->max.y) b->max.y = pts[i].y;
			if (pts[i].y < b->min.y) b->min.y = pts[i].y;
			if (pts[i].z > b->max.z) b->max.z = pts[i].z;
			if (pts[i].z < b->min.z) b->min.z = pts[i].z;
	  	 }
	}
	/* Box3_ExtendPts(b, pts, 8); */
}

#ifdef NOT_GEOMETRY_LIBRARY
void Box3_Center(const Box3* b, Point3* center)
{
	center->x = (b->min.x + b->max.x) / 2;
	center->y = (b->min.y + b->max.y) / 2;
	center->z = (b->min.z + b->max.z) / 2;

}
#endif

#ifdef NOT_GEOMETRY_LIBRARY
bool Box3_IsEqual(const Box3* b1, const Box3* b2)
{
	return (Vec3_IsEqual(&(b1->min), &(b2->min)) &&
		   Vec3_IsEqual(&(b1->max), &(b2->max)));
}
#endif

void Box3_Around(Box3* b, const Point3 *p1, const Point3* p2)
/****
 *
 * Initialize this box as the smallest rectangular bounding
 * volume to surround the input points
 *
 ****/
{

    b->min = *p1;
    b->max = *p1;
	Box3_ExtendPt(b, p2);

}

void Box3_ExtendPt(Box3* b, const Point3 *p)
{

    if (p->x > b->max.x) b->max.x = p->x;
    if (p->x < b->min.x) b->min.x = p->x;
    if (p->y > b->max.y) b->max.y = p->y;
    if (p->y < b->min.y) b->min.y = p->y;
    if (p->z > b->max.z) b->max.z = p->z;
    if (p->z < b->min.z) b->min.z = p->z;

}


void Box3_ExtendPts(Box3* b, const Point3 *p, int32 n)
{

	while (--n >= 0)
	   {
		if (p->x > b->max.x) b->max.x = p->x;
		if (p->x < b->min.x) b->min.x = p->x;
		if (p->y > b->max.y) b->max.y = p->y;
		if (p->y < b->min.y) b->min.y = p->y;
		if (p->z > b->max.z) b->max.z = p->z;
		if (p->z < b->min.z) b->min.z = p->z;
		++p;
	   }

}

void Box3_ExtendBox(Box3* dst, const Box3* src)
{

    dst->min.x = MIN(dst->min.x, src->min.x);
    dst->min.y = MIN(dst->min.y, src->min.y);
    dst->min.z = MIN(dst->min.z, src->min.z);
    dst->max.x = MAX(dst->max.x, src->max.x);
    dst->max.y = MAX(dst->max.y, src->max.y);
    dst->max.z = MAX(dst->max.z, src->max.z);

}
