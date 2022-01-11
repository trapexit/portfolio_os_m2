/****
 *
 *	@(#) box.h 95/08/31 1.18
 *	Copyright 1994, The 3DO Company
 *
 * Box3: 3D rectangular bounding volume
 * Box2: 2D rectangle
 *
 * Init(min, max)		Initialize from two corners
 * Width				Return width of box
 * Height				Return height of box
 * Depth				Return depth of box
 * Center				Return center of box as a point
 * Normalize			Validate minimum and maximum corners
 * Transform, *=		Transform box by matrix
 * Around				Initialize box to surround point or box
 * Extend				Extend box to surround point or box
 * Print				Print box on standard output
 *
 ****/
#ifndef _GPBOX
#define _GPBOX

typedef struct Box2
   {
	Point2 min;
	Point2 max;
   } Box2;


struct Box3
   {
	Point3 min;
	Point3 max;
   };

#if defined(__cplusplus) && defined(GFX_C_Bind)
extern "C" {
#endif

/*
 * PUBLIC C API
 */

void	Box3_Set(Box3* b, float xmin, float ymin, float zmin,
				 float xmax, float ymax, float zmax);
void	Box3_Center(const Box3*, Point3*);
void	Box3_Print(const Box3*);
void	Box3_Normalize(Box3*);
void	Box3_Around(Box3*, const Point3*, const Point3*);
void	Box3_ExtendPt(Box3*, const Point3*);
void	Box3_ExtendPts(Box3*, const Point3*, int32);
void	Box3_ExtendBox(Box3*, const Box3*);
void	Box3_Transform(Box3*, const Transform*);
bool	Box3_IsEmpty(Box3*);
bool	Box3_IsEqual(const Box3*, const Box3*);

#if defined(__cplusplus) && defined(GFX_C_Bind)
}
#endif

#define	Box3_Width(b)	((b)->max.x - (b)->min.x)
#define	Box3_Height(b)	((b)->max.y - (b)->min.y)
#define	Box3_Depth(b)	((b)->max.z - (b)->min.z)

#endif /* GPBOX */
