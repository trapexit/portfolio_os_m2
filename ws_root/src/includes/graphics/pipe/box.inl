#ifndef __GRAPHICS_PIPE_BOX_INL
#define __GRAPHICS_PIPE_BOX_INL


/******************************************************************************
**
**  @(#) box.inl 96/02/20 1.15
**
**  Inlines for Box3 Classes
**
******************************************************************************/


#define	Box3_Width(b)	((b)->max.x - (b)->min.x)
#define	Box3_Height(b)	((b)->max.y - (b)->min.y)
#define	Box3_Depth(b)	((b)->max.z - (b)->min.z)

#define	Box3_IsEmpty(b)				\
	(((b)->min.x == (b)->max.x) &&	\
	 ((b)->min.y == (b)->max.y) &&	\
	 ((b)->min.z == (b)->max.z))

#if defined(__cplusplus) && !defined(GFX_C_Bind)

inline Box3::Box3(gfloat xmin, gfloat ymin, gfloat zmin,
				  gfloat xmax, gfloat ymax, gfloat zmax)
	{ min.x = xmin; max.x = xmax; min.y = ymin; max.y = ymax;
	  min.z = zmin; max.z = zmax; }

inline void Box3::Set(gfloat xmin = 0, gfloat ymin = 0, gfloat zmin = 0,
					  gfloat xmax = 0, gfloat ymax = 0, gfloat zmax = 0)
	{ min.x = xmin; max.x = xmax; min.y = ymin; max.y = ymax;
	  min.z = zmin; max.z = zmax; }

inline gfloat Box3::Width() const
	{ return max.x - min.x; }

inline gfloat Box3::Height() const
	{ return max.y - min.y; }

inline gfloat Box3::Depth() const
	{ return max.z - min.z; }

inline Box3::Box3(const Box3& b)
	{ min = b.min; max = b.max; }

inline Box3& Box3::operator=(const Box3& b)
	{ min = b.min; max = b.max; return *this; }

inline void Box3::Normalize()
	{ Box3_Normalize(this); }

inline void Box3::Print() const
	{ Box3_Print(this); }

inline Point3 Box3::Center() const
	{ Point3 p; Box3_Center(this, &p); return p; }

inline void Box3::Center(Point3* p) const
	{ Box3_Center(this, p); }

inline Box3& Box3::operator*=(const Transform& m)
	{ Box3_Transform(this, &m); return *this; }

inline void Box3::Around(const Point3& p1, const Point3& p2)
	{ Box3_Around(this, &p1, &p2); }

inline void Box3::Around(const Point3* p1, const Point3* p2)
	{ Box3_Around(this, p1, p2); }

inline void Box3::Extend(const Point3& p)
	{ Box3_ExtendPt(this, &p); }

inline void Box3::Extend(const Box3& b)
	{ Box3_ExtendBox(this, &b); }

inline void Box3::Extend(const Point3* p)
	{ Box3_ExtendPt(this, p); }

inline void Box3::Extend(const Box3* b)
	{ Box3_ExtendBox(this, b); }

inline bool Box3::IsEmpty() const
	{ return Box3_IsEmpty(this); }

inline bool Box3::operator==(const Box3& b) const
	{ return Box3_IsEqual(this, &b); }

inline bool Box3::operator!=(const Box3& b) const
	{ return !Box3_IsEqual(this, &b); }

#endif


#endif /* __GRAPHICS_PIPE_BOX_INL */
