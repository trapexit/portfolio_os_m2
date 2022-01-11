/****
 *
 *	@(#) vec.h 95/08/31 1.19
 *	Copyright 1994, The 3DO Company
 *
 * Point3, Point2, Point4 and Vector3 Classes
 *
 ****/
#ifndef _GPVEC
#define _GPVEC


struct Vector3 { float x, y, z; };
struct Point4 { float x, y, z, w; };
typedef	struct Point2 { float x, y; } Point2;

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * PUBLIC C API
 *
 * Point3: 3D point class represents an absolute location
 * Point4: 4D point class represents a homogeneous coordinate
 * Vector3: 3D vector class represents a direction
 *
 * Except for their behavior when transformed by a matrix,
 * Point3 and Vector3 are the same.
 *
 * Vec3_Negate			Negate this vector
 * Vec3_Normalize		Make this vector a unit vector
 * Vec3_Add				Add one vector to another
 * Vec3_Subtract		Subtract one vector from another
 * Vec3_Transform		Transform vector by matrix
 * Pt3_Transform 		Transform point by matrix
 * Pt4_Transform		Transform homogeneous point by matrix
 * Vec3_Dot				Return dot product of vector
 * Vec3_Cross			Compute cross product of vector
 * Vec3_Length			Return length of vector
 * Vec3_LengthSquared 	Return square of length of vector
 * Vec3_IsEqual			Return TRUE if two vectors are equal
 * Pt3_Distance			Return distance between two points
 *
 ****/
void	Vec3_Negate(Vector3*);
void	Vec3_Add(Vector3* dst, const Vector3* src);
void	Vec3_Subtract(Vector3* dst, const Vector3* src);
float	Vec3_Dot(const Vector3*, const Vector3*);
float	Vec3_LengthSquared(const Vector3*);
float	Vec3_Normalize(Vector3*);
void	Vec3_Transform(Vector3*, const Transform*);
void	Vec3_TransMany(Vector3*, const Transform*, int32);
void	Pt3_Transform(Point3*, const Transform*);
void	Pt3_TransMany(Point3*, const Transform*, int32);
void	Pt4_Transform(Point4*, const Transform*);
void	Pt4_TransMany(Point4*, const Transform*, int32);
void	Vec3_Cross(Vector3* dst, const Vector3* src); 
float	Vec3_Length(const Vector3*);
float	Pt3_Distance(const Vector3*, const Vector3*);
void	Pt3_Print(const Point3 *p);
void	Vec3_Print(const Vector3 *p);
void	Pt4_Print(const Point4 *p);
bool	Vec3_IsEqual(const Vector3*, const Vector3*);

/* #if defined(__cplusplus) && defined(GFX_C_Bind) */
#if defined(__cplusplus)
}
#endif

#define Pt2_Set(p, X, Y) \
    ((p)->x = ((float)X), (p)->y = ((float)Y))

#define Pt3_Set(p, X, Y, Z) \
    ((p)->x = ((float)X), (p)->y = ((float)Y), (p)->z = ((float)Z))

#define Pt4_Set(p, X, Y, Z, W) \
    ((p)->x = ((float)X), (p)->y = ((float)Y), (p)->z = ((float)Z), (p)->w = ((float)W))

#define Vec3_Set Pt3_Set

#define	Vec3_IsEqual(a, b)	\
	(((a)->x == (b)->x) && ((a)->y == (b)->y) && ((a)->z == (b)->z))

#endif /* GPVEC */
