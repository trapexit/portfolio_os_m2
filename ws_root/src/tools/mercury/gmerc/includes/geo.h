/****
 *
 *	@(#) geo.h 95/07/10 1.41
 *	Copyright 1994, The 3DO Company
 *
 * Geometry Classes
 * The Geometry classes encapsulate geometric primitives as arrays of vertices
 * which are editable at the vertex level. A vertex may contain a position,
 * color, normal and/or texture coordinates. The vertex style determines
 * which of these are included in the vertex. The primitive type determines
 * how the vertices are rendered to describe an image.
 *
 * The structure fields of all of these classes are publicly accessible.
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
 * The C++ bindings are implemented as wrappers on top of the C functions
 *
 ****/
#ifndef _GPGEO
#define _GPGEO

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

typedef struct TexCoord { float u, v; } TexCoord;

#define TXG_User        0
#define TXG_Cylindrical 1
#define TXG_Spherical   2
#define TXG_XY          3
#define TXG_XZ          4
#define TXG_YZ          5

/*
 * Texture coordinate generation properties
 */
typedef struct TexGenProp
   {
    int32       Kind;
    Point2      Scale;
    Point2      Offset;
    int32       uRepeat, vRepeat;
    Transform   ObjMap;
   } TexGenProp;

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

#define	GEO_GLTriMesh	7
#define	GEO_TriMesh		8

typedef struct TriMesh
   {
    int16       Type;
    int16       Style;
    int32       Size;
    Point3*     Locations;
    Vector3*    Normals;
    Color4*     Colors;
    TexCoord*   TexCoords;
    int32       LengthSize;
    int32       IndexSize;
    int16*      LengthData;
    int16*      IndexData;
   } TriMesh;

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

void tmesh_generate_normals(TriMesh* geo);
void tfan_generate_normals(Geometry* geo);
void tstrip_generate_normals(Geometry* geo);
void qmesh_generate_normals(QuadMesh* geo);
void tlist_generate_normals(Geometry* geo);

#if defined(__cplusplus) && defined(GFX_C_Bind)
}
#endif

/* #include <geo.inl> */

#endif	/* GPGEO */
