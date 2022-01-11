/****
 *
 *	@(#) geo.c 95/09/08 1.55
 *	Copyright 1994, The 3DO Company
 *
 * Geometry, TriMesh, QuadMesh: Geometry Classes
 * This implementation is for the C binding. The C++ binding
 * is inlines on top of these functions.
 *
 ****/
#include "gp.i"

/*
 * Internal functions
 */
extern void tmesh_generate_normals(TriMesh* geo);
extern void tmesh_copy_data(TriMesh* dst, TriMesh* src);
extern uint32 tmesh_index_size(TriMesh* geo);
extern void tri_normal(Point3*, Point3*, Point3*, Vector3*, Vector3*, Vector3*);

static void quad_normal(Point3*, Point3*, Point3*, Point3*,
						Vector3*, Vector3*, Vector3*, Vector3*);
void tlist_generate_normals(Geometry* geo);
void tfan_generate_normals(Geometry* geo);
void tstrip_generate_normals(Geometry* geo);
void qmesh_generate_normals(QuadMesh* geo);
extern void geo_normalize(Geometry* geo);

/****
 *
 * hdr_size returns the byte size of the header of a primitive
 * structure based on the primitive type. It is based on the ordering
 * of the GEO_xxx defines for primitive types in k9geo.h.
 *
 ****/
#define	GEO_HdrMask	7

static int	hdr_size[] =
{
	0,
	sizeof(Geometry),	/* TriList */
	sizeof(Geometry),	/* TriStrip */
	sizeof(Geometry),	/* TriFan */
	sizeof(Geometry),	/* Points */
	sizeof(Geometry),	/* Lines */
	sizeof(QuadMesh),	/* QuadMesh */
	sizeof(TriMesh),	/* TriMesh */
	sizeof(TriMesh),	/* GLTriMesh */
};

/****
 *
 * vtx_size returns the vertex size (number of floats) based
 * on the vertex style (colors, normals, texcoords). It depends on
 * the following values:
 *	GEO_Colors = 1, GEO_Normals = 2, GEO_TexCoords = 4
 *
 ****/
static int	vtx_size[] =
   {
    3,				/* locations only */
    3 + 4,			/* GEO_Colors */
    3 + 3,			/* GEO_Normals */
    3 + 4 + 3,		/* GEO_Colors | GEO_Normals */
    3 + 2,			/* GEO_TexCoords */
    3 + 4 + 2,		/* GEO_Colors | GEO_TexCoords */
    3 + 3 + 2,		/* GEO_Normals | GEO_TexCoords */
    3 + 4 + 3 + 2,	/* GEO_Colors | GEO_Normals | GEO_TexCoords */
   };

/****
 *
 * vtx_style_name returns the vertex style as an ascii string using
 * the numeric vertex style as input. It depends on the following values:
 *	GEO_Colors = 1, GEO_Normals = 2, GEO_TexCoords = 4
 *
 ****/
static char* vtx_style_name[] =
   {
    "Locations",
    "(Locations | Colors)",
    "(Locations | Normals)",
    "(Locations | Colors | Normals)",
    "(Locations | TexCoords)",
    "(Locations | Colors | TexCoords)",
    "(Locations | Normals | TexCoords)",
    "(Locations | Colors | Normals | TexCoords)",
   };

/****
 *
 * prim_name returns the name of the primitive as an ascii string
 * using the primitive type as an index
 *
 ****/
static char* prim_name[] =
   {
	"ILLEGAL",
    "TriList",      /* these must follow the GEO_xxx ordering */
    "TriStrip",
    "TriFan",
    "Points",
    "Lines",
    "QuadMesh",
    "TriMesh",
    "TriMesh",
   };

uint32	tmesh_index_size(TriMesh* geo)
/****
 *
 * Return the total size of the TriMesh structure in bytes.
 * This includes the header and vertex data as well as
 * the size of index and length data. The index and
 * length sizes are rounded up to even numbers to be on a 32-bit
 * boundary. There are 2 length/index values for each 32 bit word
 * (lengths and indices are 16 bit values).
 *
 ****/
{
	uint32 size = 0;
	int32 words = (geo->LengthSize + 1) / 2;

	words += (geo->IndexSize + 1) / 2;
	words *= sizeof(int32);
	size += words;
	return size;
}

void tmesh_copy_data(TriMesh* dst, TriMesh* src)
/****
 *
 * Copy the vertex data from one geometry object to another. Will only
 * do the copy if both source and destination have the same number of
 * vertices and the vertices are the same style. For a triangle mesh,
 * the index and length data is copied as well.
 *
 ****/
{
	char*			dst_ptr;
	const char*		src_ptr;

	if (src_ptr = (char*) src->IndexData)
	   {
		dst_ptr = (char*) dst->IndexData;
		assert(dst_ptr);
		bcopy(src_ptr, dst_ptr, src->IndexSize * sizeof(int16));
	   }
	if (src_ptr = (char*) src->LengthData)
	   {
		dst_ptr = (char*) dst->LengthData;
		assert(dst_ptr);
		bcopy(src_ptr, dst_ptr, src->LengthSize * sizeof(int16));
	   }
}

void
tmesh_generate_normals(TriMesh* geo)
/****
 *
 * Generate the normals for a triangle mesh.
 *
 ****/
{
	int16*	len_data = geo->LengthData;			/* length of each index */
	int16*	idx_data = geo->IndexData;			/* mesh indices */
	Point3	*loc, *v0, *v1, *v2;					/* location pointers */
	Vector3	*nml, *n0, *n1, *n2;					/* normal pointers */
	int		i, l, length;

	assert(geo);
	assert(geo->Type == GEO_GLTriMesh);
	assert(geo->Style & GEO_Normals);
	loc = geo->Locations;
	nml = geo->Normals;
	for (l = 0;								/* for each index array */
		 l < geo->LengthSize;
		 ++l, ++len_data) {
		if (*len_data > 0)					/* triangle strip */
		   {
			int32 even = TRUE;
			length = *len_data;
			for (i = 2; i < length; i++) {
				v0 = loc + idx_data[i - 2];
				v1 = loc + idx_data[i - 1];
				v2 = loc + idx_data[i];
				n0 = nml + idx_data[i - 2];
				n1 = nml + idx_data[i - 1];
				n2 = nml + idx_data[i];
				if (even) tri_normal(v0, v1, v2, n0, n1, n2);
				else tri_normal(v1, v0, v2, n0, n1, n2);
				even = !even;
			   }
		   }
		else									/* triangle fan*/
		   {
			length = -*len_data;
			v0 = loc + idx_data[0];				/* central point for fan */
			n0 = nml + idx_data[0];
			for (i = 2; i < length; i++) {
				v1 = loc + idx_data[i-1];
				v2 = loc + idx_data[i];
				n1 = nml + idx_data[i-1];
				n2 = nml + idx_data[i];
				tri_normal(v0, v1, v2, n0, n1, n2);
			   }
		   }
		idx_data += length;
	}
	geo_normalize((Geometry*) geo);
}

void Geo_Init(Geometry* geo, int32 type, int32 style, int32 n,
			  Point3 *loc, Vector3* nml, Color4* col, TexCoord* tc)
/****
 *
 * Initialize a Geometry structure. We do not allocate vertex data. If
 * the user has set GEO_Dynamic in the style, we will garbage collect
 * their vertex data when Geo_Delete is called.
 *
 ****/
{

	geo->Style = style;
	geo->Type = type;
	geo->Size = n;
    geo->Locations = loc;
    geo->Normals = nml;
    geo->Colors = col;
    geo->TexCoords = tc;
}
	
Geometry* Geo_Create(int32 prim_type, int32 style, int32 size)
/****
 *
 * Allocate a new geometry object. Does not work for TriMesh or QuadMesh.
 *
 ****/
{
	Geometry* dst;

	assert(prim_type != GEO_QuadMesh);
	if (dst = New_Object(Geometry)) {
		Geo_CreateData(dst, prim_type, style, size);
	}
	return dst;
}

void Geo_CreateData(Geometry* geo, int32 type, int32 style, int32 n)
/****
 *
 * Initialize a Geometry structure and dynamically allocate a vertex data area
 * large enough to accomodate the given number of vertices. If we allocate
 * the data array, we set the GEO_Dynamic bit in the style to indicate this.
 * If this bit is set, Geo_Delete will garbage collect the vertex data.
 *
 ****/
{

	geo->Style = style;
	geo->Type = type;
	geo->Size = n;
	geo->Locations = NULL;
	geo->Normals = NULL;
	geo->Colors = NULL;
	geo->TexCoords = NULL;
	if (n > 0)
	   {
		geo->Style |= GEO_Dynamic;
		geo->Locations = New_Array(Point3, n);
		assert(geo->Locations);
		if (style & GEO_Normals)
		   {
			geo->Normals = New_Array(Vector3, n);
			assert(geo->Normals);
	 	   }
		if (style & GEO_Colors)
		   {
			geo->Colors = New_Array(Color4, n);
			assert(geo->Colors);
		   }
		if (style & GEO_TexCoords)
		   {
			geo->TexCoords = New_Array(TexCoord, n);
			assert(geo->TexCoords);
		   }
	   }
}

void Geo_Delete(Geometry* geo)
/****
 *
 * Delete the given geometry object.
 *
 ****/
{

	Geo_DeleteData(geo);
	Del_Object(geo);
}

void Geo_DeleteData(Geometry* geo)
/****
 *
 * Delete the vertex data for a geometry primtive. We only explicitly
 * delete the vertex data if we have allocated it (GEO_Dynamic)
 *
 ****/
{

	if (!(geo->Style & GEO_Dynamic))
		return;
	if (geo->Locations)
		Del_Object(geo->Locations);
	if (geo->Normals)
		Del_Object(geo->Normals);
	if (geo->Colors)
		Del_Object(geo->Colors);
	if (geo->TexCoords)
		Del_Object(geo->TexCoords);
	geo->Locations = NULL;
	geo->Normals = NULL;
	geo->Colors = NULL;
	geo->TexCoords = NULL;
	geo->Style &= ~GEO_Dynamic;
}

void QMesh_Init(QuadMesh* geo, int32 style, int32 x, int32 y,
				Point3 *loc, Vector3* nml, Color4* col, TexCoord* tc)
{

	geo->XSize = x;
	geo->YSize = y;
	Geo_Init((Geometry*) geo, GEO_QuadMesh, style, x * y, loc, nml, col, tc);
}
	
QuadMesh* QMesh_Create(int32 style, int32 x, int32 y)
/****
 *
 * Allocate a new QuadMesh object.
 *
 ****/
{
	QuadMesh* dst;


	if (dst = New_Object(QuadMesh))
		QMesh_CreateData(dst, style, x, y);
	return dst;
}

void QMesh_CreateData(QuadMesh* geo, int32 style, int32 x, int32 y)
/****
 *
 * Initialize a QuqdMesh primitive and dynamically create a data
 * area to hold the vertices.
 *
 ****/
{

	geo->XSize = x;
	geo->YSize = y;
	Geo_CreateData((Geometry*) geo, GEO_QuadMesh, style, x * y);
}

char*  Geo_GetStyleName(int32 style)
/****
 *
 * Returns a string containing the name of the given vertex style.
 * The vertex style is one or more of:
 *		GEO_Locations, GEO_Colors, GEO_Normals, GEO_TexCoords
 * Names will look like "Locations | Colors | Normals | TexCoords"
 *	
 ****/
{
	return vtx_style_name[style & GEO_VertexMask];
}

uint32	Geo_GetHeaderSize(const Geometry* geo)
/****
 *
 * Return the size of the Geometry structure header in bytes
 *
 ****/
{
	assert(geo);
	return hdr_size[geo->Type & GEO_HdrMask];
}

uint32	Geo_GetTotalSize(const Geometry* geo)
/****
 *
 * Return the total size of the Geometry structure in bytes.
 * This includes the header and vertex data. For a triangle mesh,
 * it also includes the size of index and length data. The index and
 * length sizes are rounded up to even numbers to be on a 32-bit
 * boundary. There are 2 length/index values for each 32 bit word
 * (lengths and indices are 16 bit values).
 *
 ****/
{
	uint32 size = hdr_size[geo->Type & GEO_HdrMask];

	size += geo->Size * vtx_size[geo->Style & GEO_VertexMask] * sizeof(float);
	if (geo->Type == GEO_GLTriMesh)
		size += tmesh_index_size((TriMesh*) geo);
	return size;
}

void Geo_GetBound(Geometry* geo, Box3* bound)
{
	const Point3*	loc = (const Point3*) geo->Locations;
	int32			n = geo->Size;

	assert(loc);
	Box3_Around(bound, loc, loc);
	while (--n > 0)							/* for each vertex */
		Box3_ExtendPt(bound, ++loc);
}

void Geo_CopyData(Geometry* dst, const Geometry* src)
/****
 *
 * Copy the vertex data from one geometry object to another. Will only
 * do the copy if both source and destination have the same number of
 * vertices and the vertices are the same style. For a triangle mesh,
 * the index and length data is copied as well.
 *
 ****/
{
	int32			style;
	char*			dst_ptr;
	const char*		src_ptr;

/*
 * If the destination is dynamically allocated, enlarge the vertex
 * data area if necessary. If possible, we re-use the existing area.
 */

	style = dst->Style & GEO_VertexMask;
	if (dst->Style & GEO_Dynamic)
	   {
		if ((src->Size > dst->Size) ||
			((src->Style & GEO_VertexMask) | style) != style)
		   {
			Geo_DeleteData(dst);
			Geo_CreateData(dst, dst->Type, style, src->Size);
		   }
	   }
	else
	   {
		assert(src->Size == dst->Size);
		assert(style == (src->Style & GEO_VertexMask));
	   }
/*
 * Copy location data
 */
	if (src_ptr = (char*) src->Locations)
	   {
		dst_ptr = (char*) dst->Locations;
		assert(dst_ptr);
		bcopy(src_ptr, dst_ptr, src->Size * sizeof(Point3));
	   }
/*
 * Copy normal data
 */
	if (src_ptr = (char*) src->Normals)
	   {
		dst_ptr = (char*) dst->Normals;
		assert(dst_ptr);
		bcopy(src_ptr, dst_ptr, src->Size * sizeof(Vector3));
	   }
/*
 * Copy color data
 */
	if (src_ptr = (char*) src->Colors)
	   {
		dst_ptr = (char*) dst->Colors;
		assert(dst_ptr);
		bcopy(src_ptr, dst_ptr, src->Size * sizeof(Color4));
	   }
/*
 * Copy texcoord data
 */
	if (src_ptr = (char*) src->TexCoords)
	   {
		dst_ptr = (char*) dst->TexCoords;
		assert(dst_ptr);
		bcopy(src_ptr, dst_ptr, src->Size * sizeof(TexCoord));
	   }
	if (src->Type == GEO_GLTriMesh)
		tmesh_copy_data((TriMesh*) dst, (TriMesh*) src);
}

void Geo_Clear(Geometry* geo, int32 style)
/****
 *
 * Zero the components of the vertices in this primitive based
 * on the input style (GEO_Locations, GEO_Colors, GEO_Normals, GEO_TexCoords)
 *
 ****/
{
	char*	data;

	assert(geo);
	assert((geo->Type > 0) && (geo->Type <= 7));
	style &= ((geo->Style & GEO_VertexMask) | GEO_Locations);
	if (style == 0)								/* don't have any to zero */
		return;
	if ((style & GEO_Locations) && (data = (char*) geo->Locations))
		bzero(data, geo->Size * sizeof(Point3));
	if ((style & GEO_Normals) && (data = (char*) geo->Normals))
		bzero(data, geo->Size * sizeof(Vector3));
	if ((style & GEO_Colors) && (data = (char*) geo->Colors))
		bzero(data, geo->Size * sizeof(Color4));
	if ((style & GEO_TexCoords) && (data = (char*) geo->TexCoords))
		bzero(data, geo->Size * sizeof(TexCoord));
}

Err 
Geo_MakeNormals(Geometry* geo)
/****
 *
 * Automatically generate normals for * the geometric primitive.
 * We will only generate these if there is space for them in the data structure.
 *
 ****/
{
	int32 style;
	assert(geo);
	assert((geo->Type > 0) && (geo->Type <= 7));
	style = geo->Style & GEO_Normals;
	if (style == 0)						/* don't have room for normals */
		return GFX_OK;
	Geo_Clear(geo, GEO_Normals);		/* clear out normals first */
	switch (geo->Type)
	   {
		case GEO_TriStrip:	tstrip_generate_normals(geo); break;
		case GEO_TriFan:	tfan_generate_normals(geo); break;
		case GEO_TriList:	tlist_generate_normals(geo); break;
		case GEO_GLTriMesh:	tmesh_generate_normals((TriMesh*) geo); break;
		case GEO_QuadMesh:	qmesh_generate_normals((QuadMesh*) geo); break;
		default:
		GLIB_ERROR(("Geo_MakeNormals: type %d not implemented\n", geo->Type));
		return GFX_ErrorNotImplemented;
	   }
	return GFX_OK;
}

Err
Geo_MakeTexCoords(Geometry* geo, const TexGenProp* tx)
/****
 *
 * Automatically generate texture coordinates for the geometric primitive.
 * We will only generate these if there is space for them in the data structure.
 *
 ****/
{
	Point3* loc;
	TexCoord* tc;
	int32	style = geo->Style & GEO_VertexMask;
	Point3	p;
	int32	i;
	float	u, v, d;
	float  s_min = 0, t_min = 0;

	assert(geo);
	assert(tx);
	assert((geo->Type > 0) && (geo->Type <= 7));
	style &= GEO_TexCoords;

	if (style == 0)			/* nothing to do, no room for texture coordinates */
		return GFX_OK;
	loc = geo->Locations;
	assert(loc);
	tc = geo->TexCoords;
	assert(tc);

	if (tx->Kind != TXG_User)				/* not user generated? */
	   {	
		Geo_Clear(geo, style);				/* clear them out first */
		for (i = 0; i < geo->Size; ++i)		/* for each vertex */
		   {
			p = *loc;						/* transform the vertex */
			Pt3_Transform(&p, &(tx->ObjMap));
			switch (tx->Kind)				/* what kind of mapping to do? */
			   {
				case TXG_Cylindrical:			/* map to cylindrical object */
				u = (atan2(p.x, p.z) + PI) / (2 * PI);
				v = (p.y + 1.0 / 2) / 1.0;
				break;
	
				case TXG_Spherical:			/* map to spherical object */
				d = sqrt(p.x * p.x + p.z * p.z);
				u = (atan2(p.x, p.z) + PI) / (2 * PI);
				v = (atan2(p.y, d) + PI) / (2 * PI);
				break;

				case TXG_XY:				/* map to planar object */
				u = p.x;
				v = p.y;
				break;

				case TXG_XZ:				/* map to planar object */
				u = p.x;
				v = p.z;
				break;

				case TXG_YZ:				/* map to planar object */
				u = p.y;
				v = p.z;
				break;
			   }
			tc->u = (tx->Offset.x + u * tx->Scale.x) * tx->uRepeat;
			tc->v = (tx->Offset.y + v * tx->Scale.y) * tx->vRepeat;
			++loc;
			++tc;
		   }
	   }

	/* normalize_coordinates: M2 requires all texture coordinates 
	 * are positive.
	 */

	tc = geo->TexCoords;
	for (i = 0; i < geo->Size; ++i)			/* for each texcoord */
	   {
		s_min = MIN(s_min, tc->u);
		t_min = MIN(t_min, tc->v);
		tc += 2;
	   }
	s_min = floor(s_min);
	t_min = floor(t_min);
	for (i = 0; i < geo->Size; ++i)			/* for each texcoord */
	   {
		tc->u = tc->u - s_min;
		tc->v = tc->v - t_min;
		++tc;
	   }
	return GFX_OK;
}

void
Geo_Transform(Geometry* geo, const Transform* trans)
/****
 *
 * Transform the vertices of the primitive by the given matrix
 *
 ****/
{
	Point3* loc;
	Vector3* nml;
	int32	style = geo->Style & GEO_VertexMask;
	int		i;

	assert(geo);
	assert(trans);
	assert((geo->Type > 0) && (geo->Type <= 7));
	loc = geo->Locations;
	assert(loc);
	for (i = 0; i < geo->Size; ++i)
		Pt3_Transform(loc++, trans);
	if ((style & GEO_Normals) && (nml = geo->Normals))
		for (i = 0; i < geo->Size; ++i)
			Vec3_Transform(nml++, trans);
}

void
tstrip_generate_normals(Geometry* geo)
/****
 *
 * Generate the normals for a triangle strip.
 *
 ****/
{
	Point3*		loc;
	Vector3*	nml;
	bool		even = TRUE;
	int			i;

	assert(geo);
	assert(geo->Type == GEO_TriStrip);
	assert(geo->Style & GEO_Normals);
	loc = geo->Locations;
	nml = geo->Normals;
	assert(loc);
	assert(nml);
	for (i = 0; i < geo->Size - 2; ++i)
	   {
		if (even) tri_normal(loc, loc + 1, loc + 2, nml, nml + 1, nml + 2);
		else tri_normal(loc + 1, loc, loc + 2, nml + 1, nml, nml + 2);
		++loc;
		++nml;
		even = !even;
	   }
	geo_normalize(geo);
}

void
tfan_generate_normals(Geometry* geo)
/****
 *
 * Generate the normals for a triangle strip.
 *
 ****/
{
	Point3*		loc;
	Vector3*	nml;
	Point3		*v0;
	Vector3		*n0;
	bool		even = TRUE;
	int			i;

	assert(geo);
	assert(geo->Type == GEO_TriFan);
	assert(geo->Style & GEO_Normals);
	loc = geo->Locations;
	nml = geo->Normals;
	assert(loc);
	assert(nml);
	v0 = loc++;
	n0 = nml++;
	for (i = 1; i < geo->Size - 1; ++i)
	   {
		tri_normal(v0, loc, loc + 1, n0, nml, nml + 1);
		++loc;
		++nml;
	   }
	geo_normalize(geo);
}

void
tlist_generate_normals(Geometry* geo)
/****
 *
 * Generate the normals for a triangle list.
 *
 ****/
{
	Point3*		loc;
	Vector3*	nml;
	int			i;

	assert(geo);
	assert(geo->Type == GEO_TriList);
	assert(geo->Style & GEO_Normals);
	loc = geo->Locations;
	nml = geo->Normals;
	assert(loc);
	assert(nml);
	for (i = 0; i < geo->Size; i += 3)
	   {
		tri_normal(loc, loc + 1, loc + 2, nml, nml + 1, nml + 2);
		loc += 3;
		nml += 3;
	   }
	geo_normalize(geo);
}

void
qmesh_generate_normals(QuadMesh* geo)
/****
 *
 * Generate the normals for a quad mesh.
 *
 ****/
{
	Point3*		loc;
	Vector3*	nml;
	int32		style = geo->Style & GEO_VertexMask;
	int32		yinc = geo->XSize;
	int			i, j;

	assert(geo);
	assert(geo->Type == GEO_QuadMesh);
	assert(geo->Style & GEO_Normals);
	loc = geo->Locations;
	nml = geo->Normals;
	assert(loc);
	assert(nml);
	for (j = 0; j < geo->YSize - 1; ++j)
	   {
		for (i = 0; i < geo->XSize - 1; ++i)
		   {
			quad_normal(loc, loc + 1, loc + yinc + 1, loc + yinc,
						nml, nml + 1, nml + yinc + 1, nml + yinc);
			loc++;
			nml++;
		   }
		loc++;
		nml++;
	   }
	geo_normalize((Geometry*) geo);
}

void
tri_normal(Point3* v0, Point3* v1, Point3* v2,
		   Vector3* n0, Vector3* n1, Vector3* n2)
/****
 *
 * Compute vertex normals for a triangle.
 *	norm_ofs	offset of the vertex normal
 *	v0, v1, v2	pointers to vertices of triangle
 *	n0, n1, n2	pointers to normals of triangle
 *
 ****/
{
	Vector3	a, b;
	Vec3_Set(&a, v1->x - v0->x, v1->y - v0->y, v1->z - v0->z);
	Vec3_Set(&b, v2->x - v1->x, v2->y - v1->y, v2->z - v1->z);
	Vec3_Cross(&a, &b);
    Vec3_Normalize(&a);
    n0->x += a.x; n1->x += a.x; n2->x += a.x;
    n0->y += a.y; n1->y += a.y; n2->y += a.y;
    n0->z += a.z; n1->z += a.z; n2->z += a.z;
}

static void
quad_normal(Point3* v0, Point3* v1, Point3* v2, Point3* v3,
			Vector3* n0, Vector3* n1, Vector3* n2, Vector3* n3)
{
	Vector3	a, b;
/*
 * Compute the normal vector for the first triangle (v0, v1, v2)
 */
	Vec3_Set(&a, v1->x - v0->x, v1->y - v0->y, v1->z - v0->z);
	Vec3_Set(&b, v2->x - v1->x, v2->y - v1->y, v2->z - v1->z);
	Vec3_Cross(&a, &b);
	Vec3_Normalize(&a);
/*
 * Add this face normal to the first three vertices
 */
	Vec3_Add(n0, &a);
	Vec3_Add(n1, &a);
	Vec3_Add(n2, &a);
/*
 * Compute the normal vector for the second triangle (v0, v2, v3)
 */
	Vec3_Set(&a, v2->x - v0->x, v2->y - v0->y, v2->z - v0->z);
	Vec3_Set(&b, v3->x - v2->x, v3->y - v2->y, v3->z - v2->z);
	Vec3_Cross(&a, &b);
	Vec3_Normalize(&a);
/*
 * Add this face normal to the vertices of the second triangle
 */
	Vec3_Add(n0, &a);
	Vec3_Add(n2, &a);
	Vec3_Add(n3, &a);
}

void geo_normalize(Geometry* geo)
/****
 *
 * Normalize the normal vectors in a vertex list
 *
 ****/
{
	Vector3*	v;
	int32		i;

	v = geo->Normals;
	assert(v);
	for (i = 0; i < geo->Size; ++i)
		if (Vec3_Normalize(v++) == 0)
			printf("Geo_MakeNormals: zero normal vector i = %d\n", i);
}

