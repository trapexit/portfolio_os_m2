/****
 *
 * Temporary surface compiler to test system, only handles one kind of 
 * element type and only handles some classes of source geometry.
 *
 ****/
/*  #include "fw.i" */
#include "gp.i"
#include "tmutils.h"
#include "texpage.h"
#include "geoitpr.h"
#include <math.h>

int facetSurfaceFlag = 0;
/* #define VERT_SEPARATE_UV 1 */

typedef union Token
   {
	struct { int16 op; int16 size; } cmd;
    int32        i;
    uint32       u;
    float       f;
   } Token;

#define	GFX_Surface			2

#define Obj_ClearFlags(o, f)    (((GfxObj*) (o))->m_Flags &= ~(f))

#define TE_VERSION 1 /* set this define to specify which TE silicon version
					  * to use (currently only 1 supported)
					  */

/*
 * If SDF_SaveTriMeshforGL is passed as the option to Surf_Compile
 * GEO_GLTriMesh primitives will be passed thru unscathed rather than
 * being compiled. The GL renderer still understands TriMesh so you can
 * still use it in SDF files and do testing on GL. These SDF files will
 * not render on the hardware (just like compiled surfaces won't
 * render under GL).
 */
#define EPS 0.000001
#define INC 1000
#define IDX_INC 2000
#define IDX_INC1 100
#define MAT_INC 20

#define MAX_VERTS	2048

static Color4	current_color = {1.0, 1.0, 1.0, 1.0};
static Vector3	current_normal = {1, 0, 0};

static int32	numVertices = 0;
static int32	maxVertices = 0 ;
static Point3	*verts = NULL;
static Color4	*colors = NULL;
static Vector3	*normals = NULL;
static TexCoord *texs = NULL;
static int32	*vert_mat = NULL;

static uint16	*idx_array = NULL;
static uint16	*idx_ptr;
static int32	num_idx = 0;
static int32	max_idx = 0;

#ifdef VERT_SEPARATE_UV
static int32	numUVVertices = 0;
static uint16	*idx_uv_array = NULL;
static uint16	*idx_uv_ptr;
static uint16	*new_uv_idx = NULL;
static int32	*uvList = NULL;
static int32	num_unique = 0;
static uint16	*unique_array = NULL;
static uint16	*snake_uv_array = NULL;
static uint16	*snake_array = NULL;
static int32	max_snake_idx = 0;
#endif

static int32	current_texture_index = -1;
static int32	prev_texture_index = -1;
static bool		texblend_update_pending;
static int32	in_element;
static int32	current_op;

static SurfaceData	*csurf = NULL;
static uint32 	surfStyle = 0;
static uint32 	current_style;
static bool		snakeOpt;

bool GM_CollapseUV = TRUE;
bool GM_CollapseGeo = TRUE;

static TmTriMesh tmtmesh;
static TmFormat tmformat;
static int32	*triList = NULL;
static int32	numTri = 0;
static int32	totalTri = 0;
static int32 	maxTri = 0;
static uint16	*new_idx = NULL;
static int32 	num_new_idx = 0;
static int32 	max_new_idx = 0;

static int32	current_material = 0;
static int32	previous_material = 0;
static int32	current_texgenkind = 0;
static int32	previous_texgenkind = 0;
static int32	normalsPerMaterial;
static uint32	*mat_ptr = NULL;
static int32	num_mat = 0;
static int32	max_mat = 0;

SurfaceData *process_Surface(GfxRef surf, uint32 options);

extern long *MakeSnake(long countoftriangles, long *ptrtotriangles, long twosided);
#ifdef VERT_SEPARATE_UV
extern void TmSnakeUV(TmTriMesh *tmesh, TmFormat format, long *pSnake, 
		    uint16 *snake_ptr, uint16 *snake_uv_ptr, int32 texIdx);
#endif
extern void TmSnake(TmTriMesh *tmesh, TmFormat format, long *pStrip, int32 texIdx);

char* Extend(SurfaceData *surf, int32 n);
void AddPrim(SurfaceData *surf, uint32 *ph);
Err AddGeometry(SurfaceData *surf, Geometry* src_geo, int32 texindex, int32 matindex, int32 texgenkind);

static uint32	Facet_Surface_Flag = 0;
uint32 GetFacetSurfaceFlag(void)
{
	return Facet_Surface_Flag;
}

void SetFacetSurfaceFlag(uint32 flag)
{
	Facet_Surface_Flag = flag;
}

char* Extend(SurfaceData *surf, int32 n)
{
	PrimHeader* h;

	h = (PrimHeader*) Mem_Alloc(n);			/* allocate new block */
	h->nextPrim = NULL;						/* make it tail of list */
	h->primSize = n;						/* byte size of the block */
	AddPrim(surf, (uint32*)h);
	return surf->m_LastPrim = (char*) h;	/* return new block addr */
}

void AddPrim(SurfaceData *surf, uint32 *ph)
{
	PrimHeader *h = (PrimHeader*)ph;
	if (surf->m_FirstPrim == NULL) 			/* is first block? */
		surf->m_FirstPrim = (char*) h;		/* start of data area */
	else									/* link to previous block */
		((PrimHeader*) surf->m_LastPrim)->nextPrim = h;

	surf->m_LastPrim = (char*) h;
	/* Obj_OrFlags(this, SURF_UpdBounds);	*/	/* mark bounds as changed */
}

Err AddGeometry(SurfaceData *surf, Geometry* src_geo, int32 texindex, int32 matindex, int32 texgenkind)
/****
 *
 * Adds a geometric primitive to the surface. A primitive is stored
 * as a Geometry structure:
 *
 *		int16	Type		GEO_QuadMesh
 *		int16	Style		(GEO_Colors, GEO_Normals, ...)
 *		uint32	Size		total number of vertices
 *		float*	Locations	-> location data (0 based, in tokens)
 *		float*	Normals		-> normal data (0 based, in tokens)
 *		float*	Colors		-> color data (0 based, in tokens)
 *		float*	TexCoords	-> texcoord data (0 based, in tokens)
 *		+	uint32	XSize		number of X vertices	QUADMESH ONLY
 *		+	uint32	YSize		number of Y vertices
 *		float	...			floats for locations, normals, colors, texcoords
 *
 *
 ****/
{
	Geometry*	dst_geo;		/* -> new geometry structure */
	GeoPrim*	geo_hdr;		/* -> Geometry primitive block header */
	char*		cur;
	uint32		total;
	uint32		header;
	int32		style;

/*
 * Check to see if we need to inherit material index
 * values from the previous block. SURF_None and SURF_UseLast
 * are handled at run-time for textures
 */
	if (surf->m_LastPrim)
	   {
		if (texindex == ((GeoPrim*) (surf->m_LastPrim))->texIndex)
			texindex = SURF_UseLast;
		if (matindex < 0)
			matindex = ((GeoPrim*) (surf->m_LastPrim))->matIndex;
	   }
/*
 * If the input geometry was NULL, just add texture and material index
 */
	if (src_geo == NULL)					/* no geometry? */
	   {
		geo_hdr = (GeoPrim*) Extend(surf, sizeof(GeoPrim));
		if (!geo_hdr)
			return GFX_ErrorNoMemory;		/* out of memory */
		geo_hdr->primType = GEO_Null;
		geo_hdr->matIndex = matindex;			/* specify material index */
		geo_hdr->texIndex = texindex;			/* and texture index */
		geo_hdr->texgenkind = texgenkind;			/* and texture index */
		return GFX_OK;
	   }
/*
 * Add another block to the Surface and copy over the Geometry header
 * and texture/material indices
 */
	total = Geo_GetTotalSize(src_geo);
	header = Geo_GetHeaderSize(src_geo);
	style = src_geo->Style & GEO_VertexMask;
	geo_hdr = (GeoPrim*) Extend(surf,
				total + sizeof(GeoPrim) - sizeof(Geometry));
	if (!geo_hdr)
		return GFX_ErrorNoMemory;				/* out of memory */
	geo_hdr->primType = src_geo->Type;			/* primitive type */
	geo_hdr->matIndex = matindex;				/* specify material index */
	geo_hdr->texIndex = texindex;				/* and texture index */
	geo_hdr->texgenkind = texgenkind;				/* and texgen index */
	dst_geo = &(geo_hdr->geoData);				/* -> Geometry area */
	bcopy(src_geo, dst_geo, header);			/* copy input header */
	cur = ((char*) dst_geo) + header;
#ifdef K9_USE_GL
/*
 * If triangle mesh, set the pointers for length and index data.
 * We round up the length and index data to 32-bit boundaries
 * in the destination.
 */
	if (dst_geo->Type == GEO_GLTriMesh)			/* trimesh special case */
	   {
		((TriMesh*) dst_geo)->LengthData = (int16*) cur;
		((TriMesh*) dst_geo)->LengthSize = ((TriMesh*) src_geo)->LengthSize;
		cur += sizeof(int16) * ((((TriMesh*) src_geo)->LengthSize + 1) & ~1);
		((TriMesh*) dst_geo)->IndexData = (int16*) cur;
		((TriMesh*) dst_geo)->IndexSize = ((TriMesh*) src_geo)->IndexSize;
		cur += sizeof(int16) * ((((TriMesh*) src_geo)->IndexSize + 1) & ~1);
	   }
#endif
/*
 * Set location, normal and color data if necessary. If the source
 * geometry structure has a non-null pointer for a data area or has
 * the corresponding bit set in the style, space will be allocated
 * for it in the surface.
 */
	dst_geo->Locations = (Point3*) cur;			/* -> location data */
	cur += src_geo->Size * sizeof(Point3);		/* after locs */
	if ((style & GEO_Normals) || src_geo->Normals)
	   {										/* have normals? */
		dst_geo->Normals = (Vector3*) cur;		/* -> normal data */
		cur += src_geo->Size * sizeof(Vector3);	/* after normals */
	   }
	else dst_geo->Normals = NULL;
	if ((style & GEO_Colors) || src_geo->Colors)
	   {										/* have colors? */
		dst_geo->Colors = (Color4*) cur;		/* -> color data */
		cur += src_geo->Size * sizeof(Color4);	/* after colors */
	   }
	else dst_geo->Colors = NULL;
	if ((style & GEO_TexCoords) || src_geo->TexCoords)
	   {										/* have tex coords? */
		dst_geo->TexCoords = (TexCoord*) cur;	/* -> tex coord data */
		cur += src_geo->Size * sizeof(TexCoord);
	   }
	else dst_geo->TexCoords = NULL;
	assert(cur >= (((char*) dst_geo) + total));
	Geo_CopyData(dst_geo, src_geo);				/* copy data areas */
	return GFX_OK;
}

#ifdef VERT_SEPARATE_UV
static void
add_snake(void)
{
  int32	*pTri;
  long	*pSnake;
  long i;
	
  long *pUniquePtrTri;

  if (numTri > 0) {
    pTri = triList+totalTri*3;
    pSnake = (long *)MakeSnake((long)numTri, (long *)pTri, 0);
    if (pSnake) {
      if (current_op & OP_TexCoords)
	{
	  /*	  fprintf(stderr,"TmSnakeUV\n"); */
	  TmSnakeUV(&tmtmesh, tmformat, pSnake, snake_array, snake_uv_array,
		    current_texture_index);
	  num_unique = 0;
	}
      else
	{
	  /*	  fprintf(stderr,"TmSnake\n");  */
	  TmSnake(&tmtmesh, tmformat, pSnake, current_texture_index);
	  num_unique = 0;
	}
    } else {
      GLIB_ERROR(("add_snake: no snake generated !!!\n"));
    }
    totalTri += numTri;
    numTri = 0;
  }
}
#else
static void
add_snake(void)
{
	int32	*pTri;
	long	*pSnake;

	if (numTri > 0) {
		pTri = triList+totalTri*3;
		pSnake = (long *)MakeSnake((long)numTri, (long *)pTri, 0);
		if (pSnake) {
			TmSnake(&tmtmesh, tmformat, pSnake, current_texture_index);
		} else {
			GLIB_ERROR(("add_snake: no snake generated !!!\n"));
		}
		totalTri += numTri;
		numTri = 0;
	}
}
#endif

static void 
set_texture_index(int32 tex)
{
	if (tex == -2)
		tex = current_texture_index;
	prev_texture_index = current_texture_index;
	if ( current_texture_index != tex ) {
		if (snakeOpt) {
			add_snake();
		}
		current_texture_index = tex;
		texblend_update_pending = TRUE;
	}
}

static void
add_material(void)
{
	uint16 *mat;

	if (normalsPerMaterial > 0) {
		if ( (num_mat+1) > max_mat ) {
			max_mat = num_mat+1+MAT_INC;
			if ( mat_ptr ) {
				mat_ptr = (uint32 *)Mem_Realloc(mat_ptr,max_mat*sizeof(uint32));
			} else {
				mat_ptr = (uint32 *)Mem_Alloc(max_mat*sizeof(uint32));
			}
			if ( mat_ptr == NULL ) {
   	        	GLIB_ERROR(("add_material: out of memory\n"));
			}
		}
		mat = (uint16*)(mat_ptr+num_mat);
		*mat = current_material;
		*(mat+1) = normalsPerMaterial;
		normalsPerMaterial = 0;
		num_mat++;
	}
}

static void
set_texgenkind(int32 texgenkind)
{
	previous_texgenkind = current_texgenkind;
	if (current_texgenkind != texgenkind) {
		current_texgenkind = texgenkind;
	}
	tmtmesh.styleTexgen = current_texgenkind;
}

static void
set_material(int32 matidx)
{
	previous_material = current_material;
	if (current_material != matidx) {
		add_material();
		current_material = matidx;
	}
}

static void
ResetIndexPtr(TmTriMesh *tm, uint16 *old, uint16 *new)
{
	TmStripFan *stripfan = tm->Fstrips;
	int32 offset;

	offset = new - old;
	while(stripfan) {
		stripfan->vertexIndices += offset;
		if (current_op & OP_Colors) {
			stripfan->colorIndices += offset;
		}
#ifndef VERT_SEPARATE_UV
		if (current_op & OP_TexCoords) {
			stripfan->textureIndices += offset;
		}
#endif
		stripfan = stripfan->next;
	}
}

#ifdef VERT_SEPARATE_UV
ResetUVIndexPtr(TmTriMesh *tm, uint16 *old, uint16 *new)
{
	TmStripFan *stripfan = tm->Fstrips;
	int32 offset;

	offset = new - old;
	while(stripfan) {
		if (current_op & OP_TexCoords) {
			stripfan->textureIndices += offset;
		}
		stripfan = stripfan->next;
	}
}

#endif

#ifdef VERT_SEPARATE_UV
static long
UniquePtList_Construct(uint16 *uTC, long count)
{
  uint32  i, numGeo, numUV, numPt;
  uint16  common, geo, uv;
  
  /* 
     Given a list to remap idx_data to a list of unique XYZ and Normal 
     and a list to remap idx_data to a list of unique UV,
     construct a list of unique Vertices (XYZ, Normal, and UV)
     */
  numGeo = 0;  /* # of unique Geometric Points so far */
  numUV = 0;   /* # of unique UVs so far */
  numPt = num_unique;   /* # of unique Geometric + UV Points so far */
  
  if (count+num_unique>max_snake_idx)
    {
      max_snake_idx = num_unique+count+IDX_INC1;
      if ( snake_array )
	{
	  snake_array = 
	    (uint16*)Mem_Realloc(snake_array,max_snake_idx*sizeof(uint16));
	} 
      else
	{
	  snake_array = (uint16*)Mem_Alloc(max_snake_idx*sizeof(uint16));
	}
      if ( snake_array == NULL )
	{
	  GLIB_ERROR(("add_vertices: out of memory\n"));
	}
      if ( snake_uv_array )
	{
	  snake_uv_array = 
	    (uint16*)Mem_Realloc(snake_uv_array,max_snake_idx*sizeof(uint16));
	} 
      else
	{
	  snake_uv_array = (uint16*)Mem_Alloc(max_snake_idx*sizeof(uint16));
	}
      if ( snake_uv_array == NULL )
	{
	  GLIB_ERROR(("add_vertices: out of memory\n"));
	}
    }
  
  for (i=0; i<count; i++)
    {
      geo = idx_array[i];
      uv  = idx_uv_array[i];
      if ((geo<numGeo) && (uv<numUV))
	{
	  if (geo > uv)
	    common = geo;
	  else
	    common = uv;
	  if((idx_array[common] == idx_array[i]) &&
	     (idx_uv_array[common] == idx_uv_array[i]))
	    {
	      uTC[i] = common+num_unique;
	    }
	  else
	    {
	      snake_array[numPt] = idx_array[i];
	      snake_uv_array[numPt] = idx_uv_array[i];
	      uTC[i] =numPt++;
	    }
	}
      else
	{ /* Update number of unique Geos and UVs up to this point */
	  if (geo==numGeo)
	    numGeo++;
	  if (uv==numUV)
	    numUV++;
	  snake_array[numPt] = idx_array[i];
	  snake_uv_array[numPt] = idx_uv_array[i];
	  uTC[i] = numPt++;
	}
      /*
      fprintf(stderr,"idx[%d]=%d\tidx_uv[%d]=%d\tUTC[%d]=%d\tNumPt=%d\n", i, 
	      idx_array[i], i, idx_uv_array[i], i, uTC[i], numPt);
	      */
    }
  
 return(numPt);
}
#endif

static void
add_vertices(Geometry *geo, bool reset_index)
{
	int32 count = geo->Size;
	Point3 *svtx = geo->Locations;
	Color4 *scolor = geo->Colors;
	Vector3 *snrm = geo->Normals;
	TexCoord *stex = geo->TexCoords;

    int32 i, j, k;
    Point3 *dvtx;
	Color4 *dcolor;
	Vector3 *dnrm;
	TexCoord *dtex;
	uint16 *didx;
#ifdef VERT_SEPARATE_UV
  uint16 *didx_uv;
#endif
	bool m1, m2;

	if ( (numVertices+count) > maxVertices ) {
		maxVertices = numVertices+count+INC;
		if ( verts ) {
			verts = (Point3 *)Mem_Realloc(verts, 
					  maxVertices*sizeof(Point3));
		} else {
			verts = (Point3 *)Mem_Alloc(maxVertices*sizeof(Point3));
		}
		if ( verts == NULL ) {
            GLIB_ERROR(("add_vertices: out of memory\n"));
		}
		if ( vert_mat ) {
			vert_mat = (int32*)Mem_Realloc(vert_mat,
					     maxVertices*sizeof(int32));
		} else {
			vert_mat = (int32*)Mem_Alloc(maxVertices*sizeof(int32));
		}
		if ( vert_mat == NULL ) {
			GLIB_ERROR(("add_vertices: out of memory\n"));
		}
		if ( texs ) {
			texs = (TexCoord *)Mem_Realloc(texs, maxVertices*sizeof(TexCoord));
		} else {
			texs = (TexCoord *)Mem_Alloc(maxVertices*sizeof(TexCoord));
		}
		if ( texs == NULL ) {
   	        GLIB_ERROR(("add_vertices: out of memory\n"));
		}
		if ( normals ) {
			normals = (Vector3 *)Mem_Realloc(normals, maxVertices*sizeof(Vector3));
		} else {
			normals = (Vector3 *)Mem_Alloc(maxVertices*sizeof(Vector3));
		}
		if ( normals == NULL ) {
			GLIB_ERROR(("add_lit_vertices: out of memory\n"));
		}
		if ( colors ) {
			colors = (Color4 *)Mem_Realloc(colors, maxVertices*sizeof(Color4));
		} else {
			colors = (Color4 *)Mem_Alloc(maxVertices*sizeof(Color4));
		}
		if ( colors == NULL ) {
  	        	GLIB_ERROR(("add_vertices: out of memory\n"));
		}
	}
	if (reset_index) {	/* for prim trifan, tristrip, trilist */
		if ((num_idx+count) > max_idx) {
			max_idx = num_idx+count+IDX_INC;
			if ( idx_array ) {
				idx_ptr = idx_array;
				idx_array = (uint16*)Mem_Realloc(idx_array,
							max_idx*sizeof(uint16));
				ResetIndexPtr(&tmtmesh,idx_ptr,idx_array);
			} else {
				idx_array = (uint16*)Mem_Alloc(max_idx*sizeof(uint16));
			}
			if ( idx_array == NULL ) {
				GLIB_ERROR(("add_vertices: out of memory\n"));
			}
#ifdef VERT_SEPARATE_UV
	  if ( idx_uv_array )
	    {
	      idx_uv_ptr = idx_uv_array;
	      idx_uv_array = (uint16*)Mem_Realloc(idx_uv_array,
						  max_idx*sizeof(uint16));
	      ResetUVIndexPtr(&tmtmesh,idx_uv_ptr,idx_uv_array);
	    } 
	  else 
	    {
	      idx_uv_array = (uint16*)Mem_Alloc(max_idx*sizeof(uint16));
	    }
	  if ( idx_uv_array == NULL ) {
	    GLIB_ERROR(("add_vertices: out of memory\n"));
	  }
	  if ( unique_array )
	    {
	      unique_array = (uint16*)Mem_Realloc(unique_array,
						  max_idx*sizeof(uint16));
	    } 
	  else
	    {
	      unique_array = (uint16*)Mem_Alloc(max_idx*sizeof(uint16));
	    }
	  if ( unique_array == NULL ) {
	    GLIB_ERROR(("add_vertices: out of memory\n"));
	  }
#endif
		}
		didx = idx_ptr = idx_array+num_idx;
#ifdef VERT_SEPARATE_UV
      didx_uv = idx_uv_ptr = idx_uv_array+num_idx;
#endif
		num_idx+=count;
	} else {		/* for prim trimesh, quadmesh */
		if (count>max_idx) {
			max_idx = count+IDX_INC1;
			if ( idx_array ) {
				idx_array = (uint16*)Mem_Realloc(idx_array,max_idx*sizeof(uint16));
			} else {
				idx_array = (uint16*)Mem_Alloc(max_idx*sizeof(uint16));
			}
			if ( idx_array == NULL ) {
				GLIB_ERROR(("add_vertices: out of memory\n"));
			}
#ifdef VERT_SEPARATE_UV
	if ( idx_uv_array ) {
	  idx_uv_array = (uint16*)Mem_Realloc(idx_uv_array,max_idx*sizeof(uint16));
	} else {
	  idx_uv_array = (uint16*)Mem_Alloc(max_idx*sizeof(uint16));
	}
	if ( idx_uv_array == NULL ) {
	  GLIB_ERROR(("add_vertices: out of memory\n"));
	}
	if ( unique_array ) {
	  unique_array = (uint16*)Mem_Realloc(unique_array,max_idx*sizeof(uint16));
	} else {
	  unique_array = (uint16*)Mem_Alloc(max_idx*sizeof(uint16));
	}
	if ( unique_array == NULL ) {
	  GLIB_ERROR(("add_vertices: out of memory\n"));
	}
#endif
		}
		didx = idx_ptr = idx_array;
#ifdef VERT_SEPARATE_UV
      didx_uv = idx_uv_ptr = idx_uv_array;
#endif
	}
    dvtx = verts+numVertices;
	if (current_op & OP_Normals) {
	    dnrm = normals+numVertices;
	} else {
		if (current_op & OP_Colors) {
		    dcolor = colors+numVertices;
		}
	}
	if (current_op & OP_TexCoords) {
#ifdef VERT_SEPARATE_UV
    /*
    fprintf(stderr,"numUVVertices=%d texs=%x\n", numUVVertices,texs);
    */
    dtex = texs+numUVVertices;
#else
    dtex = texs+numVertices;
#endif
	}
	k = numVertices;
	for (i=0; i<count; i++) {
      if (GM_CollapseGeo)
      {
		for (j=0; j<k; j++) {
			if ((vert_mat[j]==current_material) && (verts[j].x==svtx->x)
			    && (verts[j].y==svtx->y) && (verts[j].z==svtx->z)) {
				m1 = m2 = 1;
#ifndef VERT_SEPARATE_UV
				if ((current_op & OP_TexCoords) &&
					((texs[j].u != stex->u) || (texs[j].v != stex->v))) {
					m1 = 0;
				}
#endif
				if (current_op & OP_Normals) {
					if ((fabs(normals[j].x-snrm->x)>EPS) ||
						(fabs(normals[j].y-snrm->y)>EPS) ||
						(fabs(normals[j].z-snrm->z)>EPS)) {
						m2 = 0;
					}
				} else {
					if (current_op & OP_Colors) {
						if ((colors[j].r != scolor->r) || (colors[j].g != scolor->g) ||
 							(colors[j].b != scolor->b) || (colors[j].a != scolor->a)) {
							m2 = 0;
						}
					}
				}
				if (m1 && m2) {
					svtx++;
					if (current_op & OP_Normals) {
						snrm++;
					} else {
						if (current_op & OP_Colors) {
							scolor++;
						}
					}
#ifndef VERT_SEPARATE_UV
					if (current_op & OP_TexCoords) {
						stex++;
					}
#endif
					*didx++ = j;
					break;
				}
			}
		}
		}
      	if ((j>=k) || (!GM_CollapseGeo)) {
			*dvtx++ = *svtx++;
			if (current_op & OP_Normals) {
				*dnrm++ = *snrm++;
			} else {
				if (current_op & OP_Colors) {
					*dcolor++ = *scolor++;
				}
			}
#ifndef VERT_SEPARATE_UV
			if (current_op & OP_TexCoords) {
				*dtex++ = *stex++;
			}
#endif
			vert_mat[k] = current_material;
			*didx++ = k++;
		}
    }
	if (current_op & OP_Normals) {
		normalsPerMaterial += k-numVertices;
	}
    numVertices = k;

#ifdef VERT_SEPARATE_UV 
  stex = geo->TexCoords;
  k = numUVVertices;
  if (current_op & OP_TexCoords)
    {
      for (i=0; i<count; i++) 
	{
	  if (GM_CollapseUV)
	    {
	      for (j=0; j<k; j++) 
		{
		  m1 = m2 = 1;
		  if ((current_op & OP_TexCoords) &&
		      ((texs[j].u != stex->u) || (texs[j].v != stex->v)))
		    {
		      m1 = 0;
		    }
		  if (m1 && m2)
		    {
		      if (current_op & OP_TexCoords)
			{
			  stex++;
			}
		      *didx_uv++ = j;
		      break;
		    }
		}
	    }
	  if ((j>=k) || (!GM_CollapseUV))
	    {
	      *dtex++ = *stex++;
	      *didx_uv++ = k++;
	    }
	}
      numUVVertices=k;
      /*
      fprintf(stderr,"Num_idx=%d Num_unique=%d count=%d\n", num_idx, num_unique,
	      count);
	      */
      num_unique = UniquePtList_Construct(unique_array, count);
    }
#endif
}

static void
add_tri_prim(Geometry *geo)
{
	int32	i, j;
	int32 	triCnt = geo->Size-2;
	int32	triSize = sizeof(int32)*3;
	int32 	*pTri;
	uint16 	*idx;
	bool 	fan = (geo->Type == GEO_TriFan) ? 1 : 0;

	if (snakeOpt) {
		if ( ((totalTri+numTri)+triCnt) > maxTri ) {
			maxTri = (totalTri+numTri)+triCnt+IDX_INC;
			if ( triList ) {
				triList = (int32 *)Mem_Realloc(triList,maxTri*triSize);
			} else {
				triList = (int32 *)Mem_Alloc(maxTri*triSize);
			}
			if ( triList == NULL ) {
				GLIB_ERROR(("add_tri_prim: out of memory\n"));
			}
		}
		pTri = triList + (totalTri+numTri)*3;

		idx = idx_ptr;
#ifdef VERT_SEPARATE_UV
		if (current_style & OP_TexCoords)
		  idx = unique_array;
#endif
		if (fan) {
			for ( i = 0; i < triCnt; i++ ) {
				*pTri++ = idx[0];
				*pTri++ = idx[i+1];
				*pTri++ = idx[i+2];
			}
		} else {
			for ( i = 0; i < triCnt; i++ ) {
				*pTri++ = idx[i];
				if (i % 2) {
					*pTri++ = idx[i+2];
					*pTri++ = idx[i+1];
				} else {
					*pTri++ = idx[i+1];
					*pTri++ = idx[i+2];
				}
			}
		}
		numTri+=triCnt;
	} else {
		tmformat.type = (fan) ? TM_FAN : TM_STRIP;
		tmformat.order = TM_COUNTERCLOCKWISE;
		tmformat.planar = TM_NONPLANAR;
		tmformat.cont = TM_NEW;
		if (GFX_OK != TM_StripFan(&tmtmesh, tmformat, geo->Size,
						idx_ptr, 
#ifdef VERT_SEPARATE_UV
						((current_style & OP_TexCoords) ? idx_uv_ptr : NULL), 
#else
						((current_style & OP_TexCoords) ? idx_ptr : NULL), 
#endif
						((current_style & OP_Colors) ? idx_ptr : NULL), 
						current_texture_index, 0)) {
			exit(-1);
		}
	}
}

static void
add_triangles(Geometry *geo)
{
	int32 	i;
	int32 	triCnt;
	int32	triSize = sizeof(int32)*3;
	int32 	*pTri;
	uint16 	*idx;
#ifdef VERT_SEPARATE_UV
  uint16 	*idx_uv;
#endif

	triCnt = geo->Size/3;
	if (triCnt*3 != geo->Size ) {
		GLIB_ERROR(("add_triangles: invalid list size %d\n", triCnt));
	}
	if (snakeOpt) {
		if ( ((totalTri+numTri)+triCnt) > maxTri ) {
			maxTri = (totalTri+numTri)+triCnt+IDX_INC;
			if ( triList ) {
				triList = (int32 *)Mem_Realloc(triList,maxTri*triSize);
			} else {
				triList = (int32 *)Mem_Alloc(maxTri*triSize);
			}
			if ( triList == NULL ) {
				GLIB_ERROR(("add_tri_prim: out of memory\n"));
			}
		}
		pTri = triList + (totalTri+numTri)*3;

		idx = idx_ptr;
#ifdef VERT_SEPARATE_UV
    if (current_style & OP_TexCoords)
      idx = unique_array;
#endif
		for ( i = 0; i < geo->Size; i+=3 ) {
			*pTri++ = idx[i];
			*pTri++ = idx[i+1];
			*pTri++ = idx[i+2];
		}
		numTri+=triCnt;
	} else {
		tmformat.type = TM_FAN;
		tmformat.order = TM_COUNTERCLOCKWISE;
		tmformat.planar = TM_NONPLANAR;
		tmformat.cont = TM_NEW;
		idx = idx_ptr;
#ifdef VERT_SEPARATE_UV
    idx_uv = idx_uv_ptr;
#endif
		for (i = 0; i < geo->Size; i+=3, idx+=3) {
			if (GFX_OK != TM_StripFan(&tmtmesh, tmformat, 3,
							idx, 
#ifdef VERT_SEPARATE_UV
				    ((current_style & OP_TexCoords) ? idx_uv : NULL), 
#else
				    ((current_style & OP_TexCoords) ? idx : NULL), 
#endif
							((current_style & OP_Colors) ? idx : NULL), 
							current_texture_index, 0)) {
				exit(-1);
			}
		}
	}

}

static void
add_trimesh_prim(Geometry *geo)
{
	int32 i, j, l, length;
	TriMesh *tmesh = (TriMesh *)geo;
	bool fan ;
	int16*  len_data = tmesh->LengthData;         /* length of each index */
    int16*  idx_data = tmesh->IndexData;          /* mesh indices */
	bool	coplanar;
	int32 	triCnt;
	int32	triSize = sizeof(int32)*3;
	int32 	*pTri;
	uint16 	*idx, *idx1;
#ifdef VERT_SEPARATE_UV
  uint16 	*idx_uv, *idx1_uv;
#endif

	/* for each index array */

	for (l = 0; l < tmesh->LengthSize; ++l, ++len_data) {
		if (*len_data > 0) {                /* triangle strip */
			length = *len_data;
			fan = 0;
		} else { 							/* triangle fan */
			length = -*len_data;
			fan = 1;
        }
		if (snakeOpt) {
			triCnt = length-2;
		    if ( ((totalTri+numTri)+triCnt) > maxTri ) {
				maxTri = (totalTri+numTri)+triCnt+IDX_INC;
				if ( triList ) {
					triList = (int32 *)Mem_Realloc(triList,maxTri*triSize);
				} else {
					triList = (int32 *)Mem_Alloc(maxTri*triSize);
				}
				if ( triList == NULL ) {
					GLIB_ERROR(("add_tri_prim: out of memory\n"));
				}
			}
			pTri = triList + (totalTri+numTri)*3;

			idx = idx_ptr;
#ifdef VERT_SEPARATE_UV
	if (current_style & OP_TexCoords)
	  idx = unique_array;
#endif
			if (fan) {
				for ( i = 0; i < triCnt; i++ ) {
					*pTri++ = idx[idx_data[0]];
					*pTri++ = idx[idx_data[i+1]];
					*pTri++ = idx[idx_data[i+2]];
				}
			} else {
				for ( i = 0; i < triCnt; i++ ) {
					*pTri++ = idx[idx_data[i]];
					if (i%2) {
						*pTri++ = idx[idx_data[i+2]];
						*pTri++ = idx[idx_data[i+1]];
					} else {
						*pTri++ = idx[idx_data[i+1]];
						*pTri++ = idx[idx_data[i+2]];
					}
				}
			}
			numTri+=triCnt;
		} else {
			tmformat.type = (fan) ? TM_FAN : TM_STRIP;
			tmformat.order = TM_COUNTERCLOCKWISE;
			tmformat.planar = TM_NONPLANAR;
			tmformat.cont = TM_NEW;

			if ((num_new_idx+length) > max_new_idx ) {
				max_new_idx = num_new_idx+length+IDX_INC;
				if ( new_idx ) {
					idx = new_idx;
					new_idx = (uint16 *)Mem_Realloc(new_idx,
									max_new_idx*sizeof(uint16));
					ResetIndexPtr(&tmtmesh,idx,new_idx);
				} else {
					new_idx = (uint16 *)Mem_Alloc(max_new_idx*sizeof(uint16));
				}
				if ( new_idx == NULL ) {
					GLIB_ERROR(("add_trimesh: out of memory\n"));
				}
#ifdef VERT_SEPARATE_UV
	  if ( new_uv_idx ) {
	    idx_uv = new_uv_idx;
	    new_uv_idx = (uint16 *)Mem_Realloc(new_uv_idx,
					    max_new_idx*sizeof(uint16));
	    ResetUVIndexPtr(&tmtmesh,idx_uv,new_uv_idx);
	  } else {
	    new_uv_idx = (uint16 *)Mem_Alloc(max_new_idx*sizeof(uint16));
	  }
	  if ( new_uv_idx == NULL ) {
	    GLIB_ERROR(("add_trimesh: out of memory\n"));
	  }
#endif
			}
			idx = new_idx+num_new_idx;
#ifdef VERT_SEPARATE_UV
	idx_uv = new_uv_idx+num_new_idx;
#endif
			num_new_idx+=length;
			idx1 = idx;
#ifdef VERT_SEPARATE_UV
	idx1_uv = idx_uv;
	for (i = 0; i < length; i++) {
	  *idx1++ = idx_ptr[idx_data[i]];
	  *idx1_uv++ = idx_uv_ptr[idx_data[i]];
	}
#else
			for (i = 0; i < length; i++) {
				*idx1++ = idx_ptr[idx_data[i]];
			}
#endif
			if (GFX_OK != TM_StripFan(&tmtmesh, tmformat, length,
							idx, 
#ifdef VERT_SEPARATE_UV
				  ((current_style & OP_TexCoords) ? idx_uv : NULL), 
#else
				  ((current_style & OP_TexCoords) ? idx : NULL), 
#endif
							((current_style & OP_Colors) ? idx : NULL), 
							current_texture_index, 0)) {
				exit(-1);
			}
		}
		idx_data += length;
	}
}

static void
add_quadmesh_prim(Geometry *geo)
{
	uint32 i, j, k, k1, k2;
	int32 xsize, ysize;
	int32 verts_per_strip;
	QuadMesh* q = (QuadMesh*)geo;
	int32 	triCnt;
	int32	triSize = sizeof(int32)*3;
	int32 	*pTri;
	uint16 	*idx, *idx1;
#ifdef VERT_SEPARATE_UV
  uint16 	*idx_uv, *idx1_uv;
#endif

	xsize = q->XSize;
	ysize = q->YSize;
	verts_per_strip = 2*xsize;

	for ( i = 0; i < ysize-1; i++ ) {
		/* generate a tri strip per row */
		if (snakeOpt) {
			triCnt = verts_per_strip-2;
		    if ( ((totalTri+numTri)+triCnt) > maxTri ) {
				maxTri = (totalTri+numTri)+triCnt+IDX_INC;
				if ( triList ) {
					triList = (int32 *)Mem_Realloc(triList,maxTri*triSize);
				} else {
					triList = (int32 *)Mem_Alloc(maxTri*triSize);
				}
				if ( triList == NULL ) {
					GLIB_ERROR(("add_tri_prim: out of memory\n"));
				}
			}
			pTri = triList + (totalTri+numTri)*3;

			idx = idx_ptr;
#ifdef VERT_SEPARATE_UV
	if (current_style & OP_TexCoords)
	  idx = unique_array;
#endif
			k = i*xsize;
			k1 = (i+1)*xsize;
			for ( j = 2; j < verts_per_strip; j++ ) {
				k2 = ( j & 0x1 ) ? (i+1)*xsize+(j>>1): i*xsize+(j>>1);
				*pTri++ = idx[k];
				if (j%2) {
					*pTri++ = idx[k2];
					*pTri++ = idx[k1];
				} else {
					*pTri++ = idx[k1];
					*pTri++ = idx[k2];
				}
				k = k1;
				k1 = k2;
			}
			numTri+=triCnt;
		} else {
			tmformat.type = TM_STRIP;
			tmformat.order = TM_COUNTERCLOCKWISE;
			tmformat.planar = TM_NONPLANAR;
			tmformat.cont = TM_NEW;

		    if ((num_new_idx+verts_per_strip) > max_new_idx ) {
				max_new_idx = num_new_idx+verts_per_strip+IDX_INC;
				if ( new_idx ) {
					idx = new_idx;
					new_idx = (uint16 *)Mem_Realloc(new_idx,
									max_new_idx*sizeof(uint16));
					ResetIndexPtr(&tmtmesh,idx,new_idx);
				} else {
					new_idx = (uint16 *)Mem_Alloc(max_new_idx*sizeof(uint16));
				}
				if ( new_idx == NULL ) {
					GLIB_ERROR(("add_trimesh: out of memory\n"));
				}
#ifdef VERT_SEPARATE_UV
	if ( new_uv_idx ) {
	  idx_uv = new_uv_idx;
	  new_uv_idx = (uint16 *)Mem_Realloc(new_uv_idx,
					  max_new_idx*sizeof(uint16));
	  ResetUVIndexPtr(&tmtmesh,idx_uv,new_uv_idx);
	} else {
	  new_uv_idx = (uint16 *)Mem_Alloc(max_new_idx*sizeof(uint16));
	}
	if ( new_uv_idx == NULL ) {
	  GLIB_ERROR(("add_trimesh: out of memory\n"));
	}
#endif
			}
			idx = new_idx+num_new_idx;
#ifdef VERT_SEPARATE_UV
      idx_uv = new_uv_idx+num_new_idx;
#endif
			num_new_idx+=verts_per_strip;
			idx1 = idx;
#ifdef VERT_SEPARATE_UV
      idx1_uv = idx_uv;
      for (j = 0; j < verts_per_strip; j++) {
	k = ( j & 0x1 ) ? (i+1)*xsize+(j>>1): i*xsize+(j>>1);
	*idx1++ = idx_ptr[k];
	*idx1_uv++ = idx_uv_ptr[k];
      }
#else
			for (j = 0; j < verts_per_strip; j++) {
				k = ( j & 0x1 ) ? (i+1)*xsize+(j>>1): i*xsize+(j>>1);
				*idx1++ = idx_ptr[k];
			}
#endif
			if (GFX_OK != TM_StripFan(&tmtmesh, tmformat, verts_per_strip,
							idx, 
#ifdef VERT_SEPARATE_UV
				((current_style & OP_TexCoords) ? idx_uv : NULL), 
#else
				((current_style & OP_TexCoords) ? idx : NULL), 
#endif
							((current_style & OP_Colors) ? idx : NULL), 
							current_texture_index, 0)) {
				exit(-1);
			}
		}
	}
}

void
new_element(uint32 new_op)
{
	uint32 *p;

	assert(!in_element);

	current_op = new_op;
	surfStyle |= new_op;
	in_element = TRUE;

	tmformat.opt = 0;
	if (new_op & OP_Normals) {
		tmformat.vertex = TM_VERTEX_LIT;
		tmformat.color = TM_COLOR_NONE;
	} else {
		tmformat.vertex = TM_VERTEX_NONLIT;
		if (new_op & OP_Colors) {
			tmformat.color = TM_COLOR_INDEX;
			tmformat.opt |= (TM_COLOR_EQ_VERTEX | TM_INLINE_COLOR); 
		} else {
			tmformat.color = TM_COLOR_NONE;
		}
	}
	if (new_op & OP_TexCoords) {
		tmformat.texCoord = TM_TEXCOORD_INDEX;
		tmformat.opt |= (TM_TEXCOORD_EQ_VERTEX | TM_INLINE_TEXCOORD);
	} else {
		tmformat.texCoord = TM_TEXCOORD_NONE;
	}
}

void
end_element(void)
{
	assert(in_element);

	in_element = FALSE;

	if (snakeOpt) {
		add_snake();
	}
}

static void
begin_block(void)
{
	numVertices = 0;
	num_idx = 0;
#ifdef VERT_SEPARATE_UV
	numUVVertices = 0;
#endif
	num_new_idx = 0;
	numTri = 0;
	totalTri = 0;
	surfStyle = 0;
	in_element = FALSE;

	num_mat = 0;
	normalsPerMaterial = 0;

	if (TM_Init(&tmtmesh) != GFX_OK) {
		printf("Can't initialize TM\n");
		exit(-1);
	}
}

static void
end_block(void)
{
	if (in_element)
		end_element();
}

static SurfToken*
AlignToken(int16* t16)
{
    uint32 it = (uint32)t16;
	if (it & 3) it += 4 - (it & 3);
    return (SurfToken*)it;
}

static void
add_indexed_prim(SurfaceData *csurf, TmTriMesh *tm)
{
	SurfToken   *tokens;
	uint32		nTriangles, nVerts;
    Token*		t;
	uint32  	numTokensVert, numTokensNorm, numTokensNonLitVert;
	char       *pdata;
	int32		sz;
	PrimHeader	*pHeader;
	BlockHeader *bHeader;

	if (current_op & OP_Normals) {
		TM_LitVertices(tm, numVertices, verts, normals);
		add_material();
		TM_Materials(tm, num_mat, (TmMaterial *)(mat_ptr));
	} else {
		TM_NonLitVertices(tm, numVertices, verts);
		if (current_op & OP_Colors) {
			TM_Colors(tm, numVertices, colors);
		}
	}
	if (current_op & OP_TexCoords) {
		TM_TexCoords(tm, numVertices, texs);
	}
	pHeader = (PrimHeader *)TM_Compile(tm);
	if( pHeader == NULL ) return;
	
	AddPrim((SurfaceData *)csurf, (uint32 *)pHeader);
}

static bool
add_geometry(Geometry * geo)
{
	uint32 new_op;
	bool start_element;
	bool reset_index;
	int texpage1 = 0, texpage2 = 0, subpage1, subpage2;
    gTexBlend *p_txb, *c_txb;

    assert(geo);
	current_style = geo->Style & GEO_VertexMask;

	switch ( current_style ) {
	case GEO_Colors:
		new_op = OP_Colors;
		break;
	case ( GEO_Colors | GEO_TexCoords ):
		new_op = OP_Colors|OP_TexCoords;
		break;
	case ( GEO_Normals | GEO_TexCoords ) :
		new_op = OP_Normals|OP_TexCoords;
		break;
	case GEO_Normals:
		new_op = OP_Normals;
		break;
	case GEO_TexCoords:
		new_op = OP_TexCoords;
		break;
	default:
        GLIB_ERROR(("add_geometry: unsupported attribute combination 0x%lx\n",
                    current_style));
    }
	start_element = FALSE;
	if ( in_element ) {
		if ( new_op != current_op ) {
			start_element = TRUE;
		}
	} else start_element = TRUE;

	if (( geo->Style & GEO_TexCoords ) && texblend_update_pending ) {
		start_element = TRUE;
	}
	if (previous_texgenkind ^ current_texgenkind)
		start_element = TRUE;

	if (previous_material ^ current_material)
		start_element = TRUE;

	if (HasTexPage())
	{
		GetTexEntry(prev_texture_index, NULL, &texpage1, &subpage1);
		GetTexEntry(current_texture_index, NULL, &texpage2, &subpage2);
	}
    else
    {
		p_txb = NULL;
		c_txb = NULL;
		if (prev_texture_index > -1)
			p_txb = Glib_GetTexBlend(prev_texture_index);
		if (p_txb && (p_txb->pageindex >= 0))
		{
			texpage1 = p_txb->pageindex;
			subpage1 = p_txb->subindex;
		}
		else
		{
			texpage1 = -1;
			subpage1 = -1;
		}
		if (current_texture_index > -1)
			c_txb = Glib_GetTexBlend(current_texture_index);
		if (c_txb && (c_txb->pageindex >= 0))
		{
			texpage2 = c_txb->pageindex;
			subpage2 = c_txb->subindex;
		}
		else
		{
			texpage2 = -1;
			subpage2 = -1;
		}
    }
	if (texpage1 ^ texpage2)
		start_element = TRUE;

	if ((geo->Type == GEO_TriList) && (!facetSurfaceFlag) && (GetFacetSurfaceFlag()))
		start_element = TRUE;

	if ((geo->Type != GEO_TriList) && (facetSurfaceFlag) && (GetFacetSurfaceFlag()))
		start_element = TRUE;

	if ((numVertices+geo->Size) > MAX_VERTS)
		start_element = TRUE;

	if ( start_element ) {
		if ( in_element ) {
			end_element();

			if (((geo->Type == GEO_TriList) && (!facetSurfaceFlag) && (GetFacetSurfaceFlag())) ||
			    ((geo->Type != GEO_TriList) && (facetSurfaceFlag) && (GetFacetSurfaceFlag())) ||
			    (texpage1 ^ texpage2) ||
				(previous_texgenkind ^ current_texgenkind) ||
				(previous_material ^ current_material) ||
				((current_op & OP_Normals) ^ (new_op & OP_Normals)) ||
				((current_op & OP_TexCoords) ^ (new_op & OP_TexCoords)) ||
				((numVertices+geo->Size) > MAX_VERTS)) {
				add_indexed_prim(csurf, &tmtmesh);
				TM_Free(&tmtmesh);
				TM_Init(&tmtmesh);
				begin_block();
			}
		}
		new_element(new_op);
	}
	if (!snakeOpt && ((geo->Type == GEO_TriFan) || 
		(geo->Type == GEO_TriStrip) || (geo->Type == GEO_TriList))) {
		reset_index = 1;
	} else {
		reset_index = 0;
	}
	if ((geo->Type == GEO_TriList) && (GetFacetSurfaceFlag()))
	{
		facetSurfaceFlag = 1;
		GM_CollapseGeo = FALSE;
		
	}
	else
		facetSurfaceFlag = 0;

	add_vertices(geo, reset_index);
	GM_CollapseGeo = TRUE;

	switch (geo->Type) {
		case GEO_TriFan:
        	add_tri_prim(geo);
        	break;
		case GEO_TriStrip:
        	add_tri_prim(geo);
        	break;
		case GEO_GLTriMesh:
			add_trimesh_prim(geo);
			break;
		case GEO_TriList:
			add_triangles(geo);
			break;
		case GEO_QuadMesh:
			add_quadmesh_prim(geo);
			break;
	default:
		GLIB_ERROR(("add_geometry: unsupported primitive\n"));
	}

	return TRUE;
}

static void
init_surface(void)
{
	current_texgenkind = 0;
	previous_texgenkind = 0;
	current_material = 0;
	previous_material = 0;
	current_texture_index = -1;
	prev_texture_index = -1;
	texblend_update_pending = FALSE;
}

/* These functions are only defined to instrument compiled surfaces */
#if 0
typedef struct SurfIter
   {
    Token*  next;
    Token*  last;
   } SurfIter;
typedef GfxObj GPSurface;

typedef struct GPSData
   {
	SurfaceData	m_Surf;
	int32		m_Flags;
    int32		m_Size;				/* current size of data area */
    int32		m_MaxSize;			/* maximum size of data area */
   } GPSData;

uint32		GPSurf_MakeClass(void);
GPSurface*	GPSurf_Create(void);
Err			Surf_AddColor(GPSurface*, Color4*);
Err			Surf_AddNormal(GPSurface*, Vector3*);
Err			Surf_AddVertexOrder(GPSurface*, int32);
Err 		Surf_ForAll(GPSurface*, struct SurfIter*);
Token*		Surf_Next(SurfIter*, int16*, int16*);
Surface*	Surf_Compile(GPSurface*, uint32);

Surface* Surf_Compile(GPSurface *this, uint32 options)
{
    Token*		t;
	int16		cmd;
	int32		vsize;
	Box3 		bound, box;
	bool		first_bound = TRUE;
	SurfIter	iter;
	bool		indexed = 0;

	snakeOpt = (options & SDF_SnakePrimitives) ? 1 : 0;

	csurf = (SurfaceData *)Obj_Create(GFX_Surface);
	init_surface();

	begin_block();
	Surf_ForAll((GPSurface*) this, &iter);
    while (t = Surf_Next(&iter, &cmd, NULL))
        switch (cmd)
	   {
/*
 *	SURF_TriStrip / SURF_TriFan / SURF_TriList / SURF_Points / SURF_Lines
 *	0	uint32	Style			vertex style (determines vertex size)
 *	1	uint32	Size			number of vertices
 *	2	float*	Data			OFFSET of data
 *		... other stuff ...
 *	3	float	vertices...		vertex data
 */
        case SURF_TriStrip:
        case SURF_TriList:
		case SURF_QuadMesh:
        case SURF_TriFan:
        case SURF_TriMesh:
		if (((Geometry*)t)->Size <= 0) {
			GLIB_ERROR(("Surf_Compile: geo size <= 0, compiler does not handle the case\n"));
			return NULL;
		}
		Geo_GetBound((Geometry*) t, &box);

		if ( first_bound ) {
		 	bound = box;
			first_bound = FALSE;
		}
		else Box3_ExtendBox(&bound, &box);

#ifndef K9_USE_GL
		if (( options & SDF_CompileSurfaces ) || (cmd == SURF_TriMesh))
		   {
			indexed = 1;
			if ( !add_geometry((Geometry*) t) )
			   {
				GLIB_WARNING(("surface compilation failed\n"));
				return NULL;
			   }
			}
		else
#endif
		   {
			Geometry *geo = (Geometry*) t;

			Surf_AddGeometry((Surface *)csurf, geo, current_texture_index, 
							 current_material);
		   }
		texblend_update_pending = FALSE;
		break;

        case GEO_Null:
			Surf_AddGeometry((Surface *)csurf, NULL, current_texture_index, 
							 current_material);
			texblend_update_pending = FALSE;
			break;
/*
 * SURF_Color	Color4
 */
		case SURF_Color:
		current_color = *((Color4*) t);
		break;
/*
 * SURF_Normal	Vector3
 */
		case SURF_Normal:
		current_normal = *((Vector3*) t);
		break;
/*
 * SURF_MatIndex int32
 */
        case SURF_MatIndex:
		if (t->i >= 0) {
			set_material(t->i);	
		}
        break;

/*
 * SURF_TexIndex int32 (1 token)
 * If the texture has not been output with a primitive yet we must output
 * the texture to establish the texture state implied by having two texture
 * indices following each other in an SDF file. (e.g. Use modulate, TexIndex 1).
 */
        case SURF_TexIndex:
			if ( texblend_update_pending )
				Surf_AddGeometry((Surface *)csurf, NULL, current_texture_index, 
							 current_material);
	        set_texture_index(t->i);
        break;
	   }
	if (texblend_update_pending)
		Surf_AddGeometry((Surface *)csurf, NULL, current_texture_index, 
							 current_material);
	if (indexed) {
		end_block();
		add_indexed_prim(csurf, &tmtmesh);
		TM_Free(&tmtmesh);
	}
    if ( first_bound ) {
        printf("no primitives in surface\n");
    } 

	csurf->m_Bound = bound;
	Obj_ClearFlags(csurf, SURF_UpdBounds | SURF_BoundsEmpty);

	return (Surface *)csurf;
}
#endif

/* These functions use new data structure to compile surface */

SurfaceData *process_Surface(GfxRef surf, uint32 options)
{
    Token*		t;
	int16		cmd;
	int32		vsize;
	Box3 		box;
	bool		indexed = 0;
	GeoPrim		*geo;


	snakeOpt = (options & SDF_SnakePrimitives) ? 1 : 0;

	/* csurf = (SurfaceData *)Obj_Create(GFX_Surface); */
	csurf = (SurfaceData *)Surf_Create();
	init_surface();

	begin_block();
	geo = (GeoPrim *)((SurfaceData *)surf)->m_FirstPrim;
    while (geo)
    {
		/* if (geo->texgenkind >= 0)
			set_texgenkind(geo->texgenkind);	*/
		if (geo->matIndex >= 0)
			set_material(geo->matIndex);	
		if (geo->texIndex >= 0)
		{
			if ( texblend_update_pending )
				AddGeometry((SurfaceData *)csurf, NULL, current_texture_index, 
							 current_material, current_texgenkind);
	        set_texture_index(geo->texIndex);
		}
		else
	        set_texture_index(geo->texIndex);
		if (options & SDF_CompileSurfaces)
		{
			indexed = 1;
			t = (Token *) &geo->geoData;
			if ( !add_geometry((Geometry*) t) )
				return NULL;
			if (geo->texgenkind >= 0)
				set_texgenkind(geo->texgenkind);
			texblend_update_pending = FALSE;
		}

		geo = (GeoPrim *)geo->nextPrim;
	}
	if (texblend_update_pending)
		AddGeometry((SurfaceData *)csurf, NULL, current_texture_index, 
							 current_material, current_texgenkind);
	if (indexed) {
		end_block();
		add_indexed_prim(csurf, &tmtmesh);
		TM_Free(&tmtmesh);
	}

	Obj_ClearFlags(csurf, SURF_UpdBounds | SURF_BoundsEmpty);

	return (SurfaceData *)csurf;
}

