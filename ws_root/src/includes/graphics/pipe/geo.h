#ifndef __GRAPHICS_PIPE_GEO_H
#define __GRAPHICS_PIPE_GEO_H


/******************************************************************************
**
**  @(#) geo.h 96/02/20 1.43
**
**  Geometry Classes
**  The Geometry classes encapsulate geometric primitives as arrays of vertices
**  which are editable at the vertex level. A vertex may contain a position,
**  color, normal and/or texture coordinates. The vertex style determines
**  which of these are included in the vertex. The primitive type determines
**  how the vertices are rendered to describe an image.
**
**  The structure fields of all of these classes are publicly accessible.
**
******************************************************************************/


/****
 *
 * GEO_Dynamic
 * The vertex data array can be allocated by the system or specified by
 * the user by setting GEO_Dynamic in the vertex style. If GEO_Dynamic is
 * set, Geo_Delete will attempt to free the memory in the vertex data
 * (even if the system did not allocate it).
 *
 * GEO_Offset
 * If you specify your own vertex data pointer, it can be an absolute
 * address or a byte offset relative to the top of the Geometry structure.
 * The latter is specified by setting GEO_Offset in the vertex style.
 * GEO_Offset also applies to triangle mesh index and length data.
 *
 ****/

/*
 * Primitive types. Note: these must remain in this order. Geometry
 * code depends on this ordering for table dispatch.
 */
#define	GEO_Null		0
#define	GEO_TriList		1
#define	GEO_TriStrip	2
#define	GEO_TriFan		3
#define	GEO_Points		4
#define	GEO_Lines		5
#define	GEO_QuadMesh	6
#define GEO_TriMesh		8
#define GEO_MaxPrim		8
#define GEO_MinPrim		0
/*
 * Vertex styles and options. Note: these must remain in this order.
 * Geometry code depends on this ordering for table dispatch.
 */
#define	GEO_Colors		1	/* vertices include colors */
#define	GEO_Normals		2	/* vertices include normals */
#define	GEO_TexCoords	4	/* vertices include texture coordinates */
#define	GEO_VertexMask	7	/* mask for vertex geometry style */
#define	GEO_Locations	8	/* vertices include locations (always true) */
#define	GEO_Dynamic		16	/* VertexData was allocated by system (not user) */

typedef struct TexCoord { gfloat u, v; } TexCoord;
struct TexGenProp;

#if defined(__cplusplus) && !defined(GFX_C_Bind)

/****
 *
 * Geometry
 * Linear array of vertices which may contain positions, colors, normals
 * or texture coordinates. The vertex style determines which of these
 * are included in the vertex. The primitive type determines how the
 * vertices are rendered to describe an image.
 *
 * Attribute	Type		Description
 * ---------------------------------------------
 * Type			int16		primtive type (points, lines, quadmesh, ...)
 * Style		int16		vertex style (colors, normals, ...)
 * Size			uint32		number of vertices
 * Locations	Point3*		vertex locations
 * Normals		Vector3*	vertex normals
 * Colors		Color4*		vertex colors
 * TexCoords	TexCoord*	vertex texture coordinates
 *
 ****/
class Geometry
   {
public:

//	CONSTRUCTORS and DESTRUCTORS
	Geometry() { };
	Geometry(int16 type, int16 style, int32 n, Point3* loc,
			 Vector3* nml = NULL, Color4* col = NULL, TexCoord* tc = NULL);
	Geometry(int16 type, int16 style, int32 n);
	Geometry(const Geometry&);
	~Geometry();

//	ACCESSORS
	uint32		GetTotalSize() const;
	uint32		GetHeaderSize() const;
	char*		GetStyleName() const;
	void		GetBound(Box3*);
	Point3&		GetLocation(int32 i);
	Vector3&	GetNormal(int32 i);
	Color4&		GetColor(int32 i);
	TexCoord&	GetTexCoord(int32 i);

	void		SetLocation(int32 i, gfloat x, gfloat y, gfloat z);
	void		SetLocation(int32 i, const Point3& p);
	void		SetLocation(int32 i, const Point3* p);
	void		SetNormal(int32 i, gfloat x, gfloat y, gfloat z);
	void		SetNormal(int32 i, const Vector3& n);
	void		SetNormal(int32 i, const Vector3* n);
	void		SetColor(int32 i, gfloat r, gfloat g, gfloat b, gfloat a);
	void		SetColor(int32 i, const Color4& c);
	void		SetColor(int32 i, const Color4* c);
	void		SetTexCoord(int32 i, gfloat u, gfloat v);
	void		SetTexCoord(int32 i, const TexCoord& c);
	void		SetTexCoord(int32 i, const TexCoord* c);


//	OTHER METHODS
	Geometry*	Copy();
	void		CopyData(const Geometry*);
	void		Print() const;
	void		Clear(int32 style);
	void		MakeNormals();
	void		MakeTexCoords(const struct TexGenProp*);

//	OPERATORS
	Geometry&	operator=(const Geometry&);
	Geometry&	operator*=(const Transform*);

//	STATICS
	static char*	GetStyleName(int32 style);

public:
	int16		Type;
	int16		Style;
	int32		Size;
	Point3*		Locations;
	Vector3*	Normals;
	Color4*		Colors;
	TexCoord*	TexCoords;
   };

/****
 *
 * TriStrip, TriFan, TriList, Points, Lines
 * Subclasses of Geometry which automatically choose a primitive type
 *
 ****/
class Points : public Geometry
   {
public:
	Points() { };
	Points(int16 style, int32 n, Point3* locs);
	Points(int16 style, int32 n);
	Points(const Geometry&);
   };

class Lines : public Geometry
   {
public:
	Lines() { };
	Lines(int16 style, int32 n, Point3* locs);
	Lines(int16 style, int32 n);
	Lines(const Geometry&);
   };

class TriStrip : public Geometry
   {
public:
	TriStrip() { };
	TriStrip(int16 style, int32 n, Point3* locs,
		   Vector3* nml = NULL, Color4* col = NULL, TexCoord* tc = NULL);
	TriStrip(int16 style, int32 n);
	TriStrip(const Geometry&);
   };

class TriFan : public Geometry
   {
public:
	TriFan() { };
	TriFan(int16 style, int32 n, Point3* locs,
		   Vector3* nml = NULL, Color4* col = NULL, TexCoord* tc = NULL);
	TriFan(int16 style, int32 n);
	TriFan(const Geometry&);
   };

class TriList : public Geometry
   {
public:
	TriList() { };
	TriList(int16 style, int32 n, Point3* locs,
			Vector3* nml = NULL, Color4* col = NULL, TexCoord* tc = NULL);
	TriList(int16 style, int32 n);
	TriList(const Geometry&);
   };

/****
 *
 * QuadMesh
 * Two dimensional array of vertices which may have positions, colors, normals
 * or texture coordinates. The vertex style determines which of these
 * are included in the vertex.
 *
 * Attribute	Type		Description
 * -------------------------------------
 * Type			int16		primtive type (always GEO_QuadMesh)
 * Style		int16		vertex style (colors, normals, ...)
 * Size			uint32		number of vertices
 * Locations	gfloat*		vertex locations
 * Normals		gfloat*		vertex normals
 * Colors		gfloat*		vertex colors
 * XSize		uint32		number of vertices in the X direction
 * YSize		uint32		number of vertices in the Y direction
 *
 ****/
class QuadMesh : public Geometry
   {
public:
	QuadMesh() { };
	QuadMesh(int16 style, int32 nx, int32 ny, Point3* locs,
			 Vector3* nml = NULL, Color4* col = NULL, TexCoord* tc = NULL);
	QuadMesh(int16 style, int32 nx, int32 ny);
	QuadMesh(const Geometry&);
	QuadMesh(const QuadMesh&);

	Point3&		GetLocation(int32 i, int32 j);
	Vector3&	GetNormal(int32 i, int32 j);
	Color4&		GetColor(int32 i, int32 j);
	TexCoord&	GetTexCoord(int32 i, int32 j);
	void		SetLocation(int32 i, int32 j, gfloat x, gfloat y, gfloat z);
	void		SetLocation(int32 i, int32 j, const Point3& p);
	void		SetLocation(int32 i, int32 j, const Point3* p);
	void		SetNormal(int32 i, int32 j, gfloat x, gfloat y, gfloat z);
	void		SetNormal(int32 i, int32 j, const Vector3& n);
	void		SetNormal(int32 i, int32 j, const Vector3* n);
	void		SetColor(int32 i, int32 j,
						 gfloat r, gfloat g, gfloat b, gfloat a);
	void		SetColor(int32 i, int32 j, const Color4& c);
	void		SetColor(int32 i, int32 j, const Color4* c);
	void		SetTexCoord(int32 i, int32 j, gfloat u, gfloat v);
	void		SetTexCoord(int32 i, int32 j, const TexCoord& c);
	void		SetTexCoord(int32 i, int32 j, const TexCoord* c);
public:
	int32	XSize;
	int32	YSize;
   };

#else
/*
 * Public C Data Structures
 */
struct Geometry
   {
	int16		Type;
	int16		Style;
	uint32		Size;
	Point3*		Locations;
	Vector3*	Normals;
	Color4*		Colors;
	TexCoord*	TexCoords;
   };

typedef struct Geometry Points;
typedef struct Geometry Lines;
typedef struct Geometry TriStrip;
typedef struct Geometry TriFan;
typedef struct Geometry TriList;

struct QuadMesh
   {
	int16		Type;
	int16		Style;
	int32		Size;
	Point3*		Locations;
	Vector3*	Normals;
	Color4*		Colors;
	TexCoord*	TexCoords;
	int32		XSize;
	int32		YSize;
   };

#endif

#if defined(__cplusplus) && defined(GFX_C_Bind)
extern "C" {
#endif

/****
 *
 * PUBLIC C API
 *
 ****/

/*
 *	Construction and Initialization
 *
 *	Geo_Create		Allocates a Geometry object and allocates room
 *					for vertex data
 *	Geo_Init		Initializes a Geometry object, caller supplies
 *					vertex data array
 *  Geo_CopyData	Copy vertex data from one Geometry object into another.
 *					Only copies	successfully if both geometry objects have
 *					the same number of vertices. If they are TriMesh objects,
 *					index data is copied as well.
 *
 *	QMesh_Create	Allocates a QuadMesh object and allocates room
 *					for vertex data
 *	QMesh_Init		Initializes a QuadMesh object, caller supplies
 *					vertex data array
 */
Geometry*	Geo_Create(int32 type, int32 style, int32 n);
void		Geo_Init(Geometry* g, int32 prim, int32 style, int32 n,
					 Point3* loc, Vector3* nml, Color4* col, TexCoord* tc);
void		Geo_CreateData(Geometry* g, int32 prim, int32 style, int32 n);
QuadMesh*	QMesh_Create(int32 style, int32 x, int32 y);
void		QMesh_CreateData(QuadMesh* m, int32 style, int32 x, int32 y);
void		QMesh_Init(QuadMesh* m, int32 style, int32 x, int32 y,
					 Point3* loc, Vector3* nml, Color4* col, TexCoord* tc);

/*
 * Other Operations
 */
void	Geo_Clear(Geometry* src, int32 style);
void	Geo_Transform(Geometry* geo, const Transform* trans);
Err		Geo_MakeNormals(Geometry* src);
Err		Geo_MakeTexCoords(Geometry* src, const struct TexGenProp* txg);
void	Geo_CopyData(Geometry* dst, const Geometry* src);
void	Geo_VertexCopy(Geometry* dst, const Geometry* src);
void	Geo_Delete(Geometry* geo);
void	Geo_DeleteData(Geometry* geo);
void	Geo_Print(const Geometry* geo);

/*
 * General Accessors
 */
uint32	Geo_GetTotalSize(const Geometry*);
uint32	Geo_GetHeaderSize(const Geometry*);
void	Geo_GetBound(Geometry*, Box3*);
char*	Geo_GetStyleName(int32);

#if defined(__cplusplus) && defined(GFX_C_Bind)
}
#endif

#include <graphics/pipe/geo.inl>


#endif	/* __GRAPHICS_PIPE_GEO_H */
