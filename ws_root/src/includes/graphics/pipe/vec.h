#ifndef __GRAPHICS_PIPE_VEC_H
#define __GRAPHICS_PIPE_VEC_H


/******************************************************************************
**
**  @(#) vec.h 96/02/20 1.20
**
**  Point3, Point2, Point4 and Vector3 Classes
**
******************************************************************************/


#if defined(__cplusplus) && !defined(GFX_C_Bind)

/**** C++ binding
 *
 * Point3: 3D point class represents an absolute location
 * Point4: 4D point class represents a homogeneous coordinate
 * Vector3: 3D vector class represents a direction
 *
 * Except for their behavior when transformed by a matrix,
 * Point3 and Vector3 are the same.
 *
 * Vector3(x,y,z)		Initialize a vector from x,y,z components
 * Point3(x,y,z)		Initialize a point from x,y,z components
 * Point4(x,y,z,w)		Initialize a 4D point from x,y,z, components
 *
 * Normalize()			Make this vector a unit vector
 * -Vector3				Negate this vector
 * Vector3(Vector3&)	Copy one vector to another
 * Vector3 = Vector3
 * Vector3 += Vector3	Add vectors
 * Vector3 + Vector3
 * Vector3 -= Vector3	Subtract vectors
 * Vector3 - Vector3
 * Vector3 *= Transform	Transform vector by matrix
 * Vector3 * Transform
 * Vector3 == Vector3	Compare vectors
 * Vector3 != Vector3	Compare vectors
 * Transform * Vector3
 * Dot()				Return dot product of vector
 * Cross(Vector3&)		Compute cross product of vector
 * Length()				Return length of vector
 * LengthSquared()		Return square of length of vector
 * Distance(Point3&)	Distance between two points
 * Vector3[int n]		Return nth component (0=x, 1=y, 2=z)
 *
 ****/
struct Vector3
   {
	gfloat x, y, z;

	Vector3() { };
	Vector3(gfloat X, gfloat Y, gfloat Z);
	inline Vector3(const Vector3&);

	inline Vector3& operator=(const Vector3&);
	Vector3		operator-() const;
	void    	Set(gfloat X, gfloat Y, gfloat Z);
	gfloat		Normalize();
	Vector3&	operator+=(const Vector3&);
	Vector3		operator+(const Vector3&) const;
	Vector3&	operator-=(const Vector3&);
	Vector3		operator-(const Vector3&) const;
	Vector3&	operator*=(const Transform&);
	Vector3		operator*(const Transform&) const;
	bool		operator==(const Vector3&) const;
	bool		operator!=(const Vector3&) const;
	gfloat		operator[](int) const;
	gfloat		Dot(const Vector3&) const;
	Vector3		Cross(const Vector3&) const;
	gfloat		Length() const;
	gfloat		LengthSquared() const;
	void    	Print() const;
   };

struct Point3 : public Vector3
   {
	Point3() { };
	Point3(gfloat X, gfloat Y, gfloat Z);
	Point3(const Vector3&);
	Vector3&	operator*=(const Transform&);
	Vector3		operator*(const Transform&) const;
	gfloat		Distance(const Point3&) const;
   };

struct Point4
   {
	gfloat x, y, z, w;

	Point4() { };
	Point4(gfloat X, gfloat Y, gfloat Z, gfloat W);
	Point4(const Vector3&);
	Point4(const Point4&);
	Point4& operator=(const Point4&);
	Point4&	operator*=(const Transform&);
	Point4	operator*(const Transform&) const;
	gfloat	operator[](int) const;
	Point4 operator-() const;
	void    Set(gfloat X, gfloat Y, gfloat Z, gfloat W);
	void	Normalize();
	Point4&	operator+=(const Point4&);
	Point4	operator+(const Point4&) const;
	Point4&	operator-=(const Point4&);
	Point4	operator-(const Point4&) const;
	bool	operator==(const Point4&) const;
	bool	operator!=(const Point4&) const;
	gfloat	Dot(const Point4&) const;
	gfloat	Length() const;
	gfloat	LengthSquared() const;
	void    Print() const;
   };

struct Point2
   {
	gfloat x, y;

	Point2() { };
	Point2(gfloat x, gfloat y);
	void   Set(gfloat x, gfloat y);
   };

#else	/* C Binding */

struct Vector3 { gfloat x, y, z; };
struct Point4 { gfloat x, y, z, w; };
typedef	struct Point2 { gfloat x, y; } Point2;

#endif

#if defined(__cplusplus) && defined(GFX_C_Bind)
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
gfloat	Vec3_Dot(const Vector3*, const Vector3*);
gfloat	Vec3_LengthSquared(const Vector3*);
gfloat	Vec3_Normalize(Vector3*);
void	Vec3_Transform(Vector3*, const Transform*);
void	Vec3_TransMany(Vector3*, const Transform*, int32);
void	Pt3_Transform(Point3*, const Transform*);
void	Pt3_TransMany(Point3*, const Transform*, int32);
void	Pt4_Transform(Point4*, const Transform*);
void	Pt4_TransMany(Point4*, const Transform*, int32);
void	Vec3_Cross(Vector3* dst, const Vector3* src);
gfloat	Vec3_Length(const Vector3*);
gfloat	Pt3_Distance(const Vector3*, const Vector3*);
void	Pt3_Print(const Point3 *p);
void	Vec3_Print(const Vector3 *p);
void	Pt4_Print(const Point4 *p);
bool	Vec3_IsEqual(const Vector3*, const Vector3*);

#if defined(__cplusplus) && defined(GFX_C_Bind)
}
#endif

#include <graphics/pipe/vec.inl>


#endif /* __GRAPHICS_PIPE_VEC_H */
