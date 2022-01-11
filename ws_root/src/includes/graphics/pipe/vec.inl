#ifndef __GRAPHICS_PIPE_VEC_INL
#define __GRAPHICS_PIPE_VEC_INL


/******************************************************************************
**
**  @(#) vec.inl 96/02/20 1.19
**
**  Inlines for Point3 and Vector3 Classes
**
******************************************************************************/


#define Pt2_Set(p, X, Y) \
    ((p)->x = ((gfloat)X), (p)->y = ((gfloat)Y))

#define Pt3_Set(p, X, Y, Z) \
    ((p)->x = ((gfloat)X), (p)->y = ((gfloat)Y), (p)->z = ((gfloat)Z))

#define Pt4_Set(p, X, Y, Z, W) \
    ((p)->x = ((gfloat)X), (p)->y = ((gfloat)Y), (p)->z = ((gfloat)Z), (p)->w = ((gfloat)W))

#define Vec3_Set Pt3_Set

#define	Vec3_IsEqual(a, b)	\
	(((a)->x == (b)->x) && ((a)->y == (b)->y) && ((a)->z == (b)->z))

#if defined(__cplusplus) && !defined(GFX_C_Bind)

/*
 * Vector3 inlines
 */
inline Vector3::Vector3(gfloat X, gfloat Y, gfloat Z = 0)
	{ x = X; y = Y; z = Z; }

inline Vector3::Vector3(const Vector3& v)
	{ x = v.x; y = v.y; z = v.z; }

inline Vector3& Vector3::operator=(const Vector3& v)
	{ x = v.x; y = v.y; z = v.z; return *this; }

inline void Vector3::Set(gfloat X = 0, gfloat Y = 0, gfloat Z = 0)
	{ x = X; y = Y; z = Z; }

inline Vector3 Vector3::operator-() const
	{ return Vector3(-x, -y, -z); }

inline Vector3&	Vector3::operator+=(const Vector3& v)
	{ x += v.x; y += v.y; z += v.z; return *this; }

inline Vector3	Vector3::operator+(const Vector3& v) const
	{ return Vector3(x + v.x, y + v.y, z + v.z); }

inline Vector3&	Vector3::operator-=(const Vector3& v)
	{ x -= v.x; y -= v.y; z -= v.z; return *this; }

inline Vector3	Vector3::operator-(const Vector3& v) const
	{ return Vector3(x - v.x, y - v.y, z - v.z); }

inline Vector3	Vector3::operator*(const Transform& m) const
	{ Vector3 v(*this); return v *= m; }

inline gfloat	Vector3::Dot(const Vector3& v) const
	{ return x * v.x + y * v.y + z * v.z; }

inline Vector3	Vector3::Cross(const Vector3& src) const
	{ Vector3 dst = *this; Vec3_Cross(&dst, &src); return dst; }

inline gfloat	Vector3::LengthSquared() const
	{ return x * x + y * y + z * z; }

inline gfloat	Vector3::Length() const
	{ return sqrt(LengthSquared()); }

inline gfloat	Point3::Distance(const Point3& v) const
	{ return sqrt(x * v.x + y * v.y + z * v.z); }

inline void		Vector3::Print() const
	{ Vec3_Print(this); }

inline bool		Vector3::operator==(const Vector3& v) const
	{ return (x == v.x) && (y == v.y) && (z == v.z); }

inline bool		Vector3::operator!=(const Vector3& v) const
	{ return (x != v.x) || (y != v.y) || (z != v.z); }

/*
 * Point2 inlines
 */
inline Point2::Point2(gfloat X, gfloat Y)
	{ x = X; y = Y; }

inline void Point2::Set(gfloat X = 0, gfloat Y = 0)
	{ x = X; y = Y; }

/*
 * Point3 inlines
 */
inline Point3::Point3(gfloat X, gfloat Y, gfloat Z = 0)
	{ x = X; y = Y; z = Z; }

inline Point3::Point3(const Vector3& v)
	{ x = v.x; y = v.y; z = v.z; }

/*
 * Point4 inlines
 */
inline Point4::Point4(gfloat X, gfloat Y, gfloat Z = 0, gfloat W = 1)
	{ x = X; y = Y; z = Z; w = W; }

inline Point4::Point4(const Vector3& v)
	{ x = v.x; y = v.y; z = v.z; w = 1; }

inline Point4::Point4(const Point4& p)
	{ x = p.x; y = p.y; z = p.z; w = p.w; }

inline void Point4::Set(gfloat X = 0, gfloat Y = 0, gfloat Z = 0, gfloat W = 0)
	{ x = X; y = Y; z = Z; w = W; }

inline Point4& Point4::operator=(const Point4& v)
	{ x = v.x; y = v.y; z = v.z; w = v.w; return *this; }

inline Point4 Point4::operator-() const
	{ return Point4(-x, -y, -z, -w); }

inline Point4&	Point4::operator+=(const Point4& v)
	{ x += v.x; y += v.y; z += v.z; w += v.w; return *this; }

inline Point4	Point4::operator+(const Point4& v) const
	{ return Point4(x + v.x, y + v.y, z + v.z, w + v.w); }

inline Point4&	Point4::operator-=(const Point4& v)
	{ x -= v.x; y -= v.y; z -= v.z; w -= v.w; return *this; }

inline Point4	Point4::operator-(const Point4& v) const
	{ return Point4(x - v.x, y - v.y, z - v.z, w - v.w); }

inline Point4	Point4::operator*(const Transform& m) const
	{ Point4 v(*this); return v *= m; }

inline gfloat	Point4::Dot(const Point4& v) const
	{ return x * v.x + y * v.y + z * v.z + w * v.w; }

inline gfloat	Point4::LengthSquared() const
	{ return x * x + y * y + z * z + w * w; }

inline gfloat	Point4::Length() const
	{ return sqrt(LengthSquared()); }

inline void	Point4::Print() const
	{ Pt4_Print(this); }

inline bool		Point4::operator==(const Point4& v) const
	{ return (x == v.x) && (y == v.y) && (z == v.z); }

inline bool		Point4::operator!=(const Point4& v) const
	{ return (x != v.x) || (y != v.y) || (z != v.z); }

#endif


#endif /* __GRAPHICS_PIPE_VEC_INL */
