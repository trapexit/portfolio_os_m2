#ifndef __GRAPHICS_PIPE_BOX_H
#define __GRAPHICS_PIPE_BOX_H


/******************************************************************************
**
**  @(#) box.h 96/02/20 1.19
**
**  Box3: 3D rectangular bounding volume
**  Box2: 2D rectangle
**
******************************************************************************/


/****
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

typedef struct Box2
   {
	Point2 min;
	Point2 max;
   } Box2;

#if defined(__cplusplus) && !defined(GFX_C_Bind)
/*
 * PUBLIC C++ API
 */
struct Box3
   {
	Point3	min, max;

	Box3() { };
	Box3(const Box3&);
	Box3(gfloat xmin, gfloat ymin, gfloat zmin,
		 gfloat xmax, gfloat ymax, gfloat zmax);
	void	Set(gfloat xmin, gfloat ymin, gfloat zmin,
				gfloat xmax, gfloat ymax, gfloat zmax);
	void	Set(const Point3&, const Point3&);
	Box3&	operator=(const Box3&);
	Box3&	operator*=(const Transform&);
	bool	operator==(const Box3&) const;
	bool	operator!=(const Box3&) const;
	void	Normalize();
	gfloat	Width() const;
	gfloat	Height() const;
	gfloat	Depth() const;
	Point3	Center() const;
	void	Center(Point3*) const;
	bool	IsEmpty() const;
	void	Around(const Point3&, const Point3&);
	void	Around(const Point3*, const Point3*);
	void	Extend(const Point3&);
	void	Extend(const Point3*);
	void	Extend(const Box3&);
	void	Extend(const Box3*);
	void	Print() const;
   };

#else

struct Box3
   {
	Point3 min;
	Point3 max;
   };

#endif

#if defined(__cplusplus) && defined(GFX_C_Bind)
extern "C" {
#endif

/*
 * PUBLIC C API
 */

void	Box3_Set(Box3* b, gfloat xmin, gfloat ymin, gfloat zmin,
				 gfloat xmax, gfloat ymax, gfloat zmax);
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

#include <graphics/pipe/box.inl>

#endif /* __GRAPHICS_PIPE_BOX_H */
