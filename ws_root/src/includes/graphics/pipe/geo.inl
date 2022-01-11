#ifndef __GRAPHICS_PIPE_GEO_INL
#define __GRAPHICS_PIPE_GEO_INL


/******************************************************************************
**
**  @(#) geo.inl 96/02/20 1.37
**
**  Inlines for Geometry classes
**
******************************************************************************/


#if defined(__cplusplus) && !defined(GFX_C_Bind)

/*
 * Geometry inlines
 */
inline uint32 Geometry::GetTotalSize() const
	{ return Geo_GetTotalSize(this); }

inline uint32 Geometry::GetHeaderSize() const
	{ return Geo_GetHeaderSize(this); }

inline void Geometry::GetBound(Box3* b)
	{ Geo_GetBound(this, b); }

inline void Geometry::Print() const
	{ Geo_Print(this); }

inline void Geometry::Clear(int32 style)
	{ Geo_Clear(this, style); }

inline Geometry& Geometry::operator*=(const Transform* trans)
	{ Geo_Transform(this, trans); return *this; }

inline void Geometry::MakeNormals()
	{ Geo_MakeNormals(this); }

inline void Geometry::MakeTexCoords(const TexGenProp* txg)
	{ Geo_MakeTexCoords(this, txg); }

inline Point3& Geometry::GetLocation(int32 i)
	{ return Locations[i]; }

inline Vector3& Geometry::GetNormal(int32 i)
	{ return Normals[i]; }

inline Color4& Geometry::GetColor(int32 i)
	{ return Colors[i]; }

inline TexCoord& Geometry::GetTexCoord(int32 i)
	{ return TexCoords[i]; }

inline void Geometry::SetLocation(int32 i, gfloat x, gfloat y, gfloat z)
{
	Locations[i].x = x;
	Locations[i].y = y;
	Locations[i].z = z;
}

inline void Geometry::SetLocation(int32 i, const Point3& p)
	{ Locations[i] = p; }

inline void Geometry::SetLocation(int32 i, const Point3* p)
	{ Locations[i] = *p; }

inline void Geometry::SetNormal(int32 i, gfloat x, gfloat y, gfloat z)
{
	Normals[i].x = x;
	Normals[i].y = y;
	Normals[i].z = z;
}

inline void Geometry::SetNormal(int32 i, const Vector3& n)
	{ Normals[i] = n; }

inline void Geometry::SetNormal(int32 i, const Vector3* n)
	{ Normals[i] = *n; }

inline void Geometry::SetColor(int32 i,
			gfloat r, gfloat g, gfloat b, gfloat a = 1)
{
	Colors[i].r = r;
	Colors[i].g = g;
	Colors[i].b = b;
	Colors[i].a = a;
}

inline void Geometry::SetColor(int32 i, const Color4& c)
	{ Colors[i] = c; }

inline void Geometry::SetColor(int32 i, const Color4* c)
	{ Colors[i] = *c; }

inline void Geometry::SetTexCoord(int32 i, gfloat u, gfloat v)
{
	TexCoords[i].u = u;
	TexCoords[i].v = v;
}

inline void Geometry::SetTexCoord(int32 i, const TexCoord& c)
	{ TexCoords[i] = c; }

inline void Geometry::SetTexCoord(int32 i, const TexCoord* c)
	{ TexCoords[i] = *c; }

/*
 * Constructors for subclasses
 */
inline Points::Points(int16 style, int32 n, Point3* loc)
	: Geometry(GEO_Points, style, n, loc) { }

inline Points::Points(int16 style, int32 n)
	: Geometry(GEO_Points, style, n) { }

inline Points::Points(const Geometry& geo)
	: Geometry(geo)
	{ Type = GEO_Points; }

inline Lines::Lines(int16 style, int32 n, Point3* loc)
	: Geometry(GEO_Lines, style, n, loc) { }

inline Lines::Lines(int16 style, int32 n)
	: Geometry(GEO_Lines, style, n) { }

inline Lines::Lines(const Geometry& geo)
	: Geometry(geo)
	{ Type = GEO_Lines; }

inline TriStrip::TriStrip(int16 style, int32 n, Point3* loc,
					Vector3* nml, Color4* col, TexCoord* tc)
	: Geometry(GEO_TriStrip, style, n, loc, nml, col, tc) { }

inline TriStrip::TriStrip(int16 style, int32 n)
	: Geometry(GEO_TriStrip, style, n) { }

inline TriStrip::TriStrip(const Geometry& geo)
	: Geometry(geo)
	{ Type = GEO_TriStrip; }

inline TriFan::TriFan(int16 style, int32 n, Point3* loc,
					Vector3* nml, Color4* col, TexCoord* tc)
	: Geometry(GEO_TriFan, style, n, loc, nml, col, tc) { }

inline TriFan::TriFan(int16 style, int32 n)
	: Geometry(GEO_TriFan, style, n) { }

inline TriFan::TriFan(const Geometry& geo)
	: Geometry(geo)
	{ Type = GEO_TriFan; }

inline TriList::TriList(int16 style, int32 n, Point3* loc,
					Vector3* nml, Color4* col, TexCoord* tc)
	: Geometry(GEO_TriList, style, n, loc, nml, col, tc) { }

inline TriList::TriList(int16 style, int32 n)
	: Geometry(GEO_TriList, style, n) { }

inline TriList::TriList(const Geometry& geo)
	: Geometry(geo)
	{ Type = GEO_TriList; }

inline QuadMesh::QuadMesh(int16 style, int32 x, int32 y,
			Point3* loc, Vector3* nml, Color4* col, TexCoord* tc)
	: Geometry(GEO_QuadMesh, style, x * y, loc, nml, col, tc)
	{ XSize = x; YSize = y; }

inline QuadMesh::QuadMesh(int16 style, int32 x, int32 y)
	: Geometry(GEO_QuadMesh, style, x * y)
	{ XSize = x; YSize = y; }

inline Point3& QuadMesh::GetLocation(int32 i, int32 j)
	{ return Locations[i + XSize * j]; }

inline Vector3& QuadMesh::GetNormal(int32 i, int32 j)
	{ return Normals[i + XSize * j]; }

inline Color4& QuadMesh::GetColor(int32 i, int32 j)
	{ return Colors[i + XSize * j]; }

inline TexCoord& QuadMesh::GetTexCoord(int32 i, int32 j)
	{ return TexCoords[i + XSize * j]; }

inline void QuadMesh::SetLocation(int32 i, int32 j, gfloat x, gfloat y, gfloat z)
{
Point3& p = Locations[i + XSize * j];

	p.x = x;
	p.y = y;
	p.z = z;
}

inline void QuadMesh::SetLocation(int32 i, int32 j, const Point3& p)
	{ Locations[i + Xsize * j] = p; }

inline void QuadMesh::SetLocation(int32 i, int32 j, const Point3* p)
	{ Locations[i + Xsize * j] = *p; }

inline void QuadMesh::SetNormal(int32 i, int32 j, gfloat x, gfloat y, gfloat z)
{
Vector3& p = Normals[i + Xsize * j];

	p.x = x;
	p.y = y;
	p.z = z;
}

inline void QuadMesh::SetNormal(int32 i, int32 j, const Vector3& n)
	{ Normals[i + Xsize * j] = n; }

inline void QuadMesh::SetNormal(int32 i, int32 j, const Vector3* n)
	{ Normals[i + Xsize * j] = *n; }

inline void QuadMesh::SetColor(int32 i, int32 j,
			gfloat r, gfloat g, gfloat b, gfloat a = 1)
{
Color4& c = Colors[i + Xsize * j];

	c.r = r;
	c.g = g;
	c.b = b;
	c.a = a;
}

inline void QuadMesh::SetColor(int32 i, int32 j, const Color4& c)
	{ Colors[i + Xsize * j] = c; }

inline void QuadMesh::SetColor(int32 i, int32 j, const Color4* c)
	{ Colors[i + Xsize * j] = *c; }

inline void QuadMesh::SetTexCoord(int32 i, int32 j, gfloat u, gfloat v)
{
TexCoord& c = TexCoords[i + Xsize * j];
	c.u = u;
	c.v = v;
}

inline void QuadMesh::SetTexCoord(int32 i, int32 j, const TexCoord& c)
	{ TexCoords[i + Xsize * j] = c; }

inline void QuadMesh::SetTexCoord(int32 i, int32 j, const TexCoord* c)
	{ TexCoords[i + Xsize * j] = *c; }

#endif

#define	Geo_GetLocation(g, i)		(g)->Locations[i]
#define	Geo_GetNormal(g, i)			(g)->Normals[i]
#define	Geo_GetColor(g, i)			(g)->Colors[i]
#define	Geo_GetTexCoord(g, i)		(g)->TexCoords[i]

#define	Geo_SetLocation(g, i, X, Y, Z) \
	Pt3_Set(&(g)->Locations[i], X, Y, Z)

#define	Geo_SetNormal(g, i, X, Y, Z) \
	Vec3_Set(&(g)->Normals[i], X, Y, Z)

#define	Geo_SetColor(g, i, R, G, B, A) \
	Col_Set(&(g)->Colors[i], R, G, B, A)

#define	Geo_SetTexCoord(g, i, U, V) \
	((g)->TexCoords[i].u = (U), (g)->TexCoords[i].v = (V))

#define	QMesh_GetLocation(g, i, j) \
	(g)->Locations[(i) + (g)->XSize * (j)]

#define	QMesh_GetNormal(g, i, j) \
	(g)->Normals[(i) + (g)->XSize * (j)]

#define	QMesh_GetColor(g, i, j) \
	((g)->Colors[(i) + (g)->XSize * (j)])

#define	QMesh_GetTexCoord(g, i, j) \
	(g)->TexCoords[(i) + (g)->XSize * (j)]

#define	QMesh_SetLocation(g, i, j, X, Y, Z) \
	Pt3_Set(&(g)->Locations[(i) + (g)->XSize * (j)], X, Y, Z)

#define	QMesh_SetNormal(g, i, j, X, Y, Z) \
	Vec3_Set(&(g)->Normals[(i) + (g)->XSize * (j)], X, Y, Z)

#define	QMesh_SetColor(g, i, j, R, G, B, A) \
	Col_Set(&((g)->Colors[(i) + (g)->XSize * (j)]), R, G, B, A)

#define	QMesh_SetTexCoord(g, i, j, U, V) \
	((g)->TexCoords[(i) + (g)->XSize * (j)].u = (U), \
	 (g)->TexCoords[(i) + (g)->XSize * (j)].v = (V))


#endif /* __GRAPHICS_PIPE_GEO_INL */
