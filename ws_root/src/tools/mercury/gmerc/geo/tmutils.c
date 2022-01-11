/*
 *    @(#) tmutils.c 95/09/12 1.3
 *  Copyright 1994, The 3DO Company

 Utility functions to create trimesh data


 */

#define INTERFACE_CODE	1
#ifdef INTERFACE_CODE
extern int mercid;
extern int facetSurfaceFlag;
#endif

#include "gp.i"
#include "tmutils.h"
#include "texpage.h"
#include "geoitpr.h"
#include <math.h>

#define ROUND(x)	{ \
	uint32 *foo = (uint32 *)&(x); \
	*foo = (*foo + 3) & ~3; \
}

#define FEQL(x, y)	((fabs(x-y)) < 0.00001)

Err
TM_Init( TmTriMesh *tm )
{
	tm->nonLits.n = 0;
	tm->lits.n = 0;
	tm->texs.n = 0;
	tm->colors.n = 0;
	tm->Fstrips = tm->Lstrips = 0;
	tm->hint.n = 0;
	tm->mats.n = 0;
	tm->numElements = 0;
	tm->sizeElements = 0;
	tm->styleElements = 0;
	tm->styleTxbIndex = -3; /* An illegal value */
	tm->styleTexgen = 0;
	tm->totalStyles = 0;
	return GFX_OK;
}

Err
TM_NonLitVertices(TmTriMesh *tm, uint32 numVerts, Point3* verts)
{
	if (tm->nonLits.n != 0)
	  return GFX_MiscError;

	tm->nonLits.n = numVerts;
	tm->nonLits.verts = verts;

	return GFX_OK;
}

Err
TM_LitVertices(TmTriMesh *tm, uint32 numNormals, Point3 *litVerts, Vector3 *normals)
{
	if (tm->lits.n != 0)
	  return GFX_MiscError;
	  
	tm->lits.n = numNormals;
	tm->lits.verts = litVerts;
	tm->lits.normals = normals;

	return GFX_OK;
}
	
Err
TM_Materials(TmTriMesh *tm, uint32 numMats, TmMaterial *mat)
{
	if (tm->mats.n != 0)
	  return GFX_MiscError;

	tm->mats.n = numMats;
	tm->mats.matList = mat;

	return GFX_OK;
}	
	
Err
TM_TexCoords(TmTriMesh *tm, uint32 numCoords, TexCoord *coords)
{
	if (tm->texs.n != 0)
	  return GFX_MiscError;

	tm->texs.n = numCoords;
	tm->texs.coords = coords;

	return GFX_OK;
}	
	
Err
TM_Colors(TmTriMesh *tm, uint32 numColors, Color4 *pColors)
{
	if (tm->colors.n != 0)
	  return GFX_MiscError;

	tm->colors.n = numColors;
	tm->colors.colors = pColors;

	return GFX_OK;
}	
	
Err
TM_Hints(TmTriMesh *tm, uint32 numHints, uint32* hints)
{
	if (tm->hint.n != 0)
		return GFX_MiscError;

	tm->hint.n = numHints;
	tm->hint.hint = hints;
	return GFX_OK;
}

Err
TM_StripFan(TmTriMesh *tm, TmFormat format, uint32 n, uint16 *vertexIndices,
			uint16* textureIndices, uint16 *colorIndices,
			int32 txbIndex, uint16 colorIndex)
{
	TmStripFan	*clump = (TmStripFan*)Mem_Alloc(sizeof(TmStripFan));

	if (clump == NULL)
	  return GFX_ErrorNoMemory;

	clump->next = 0;

	clump->type	= format.type;
	clump->cont	= format.cont;
	clump->order = format.order;
	clump->planar = format.planar;
	clump->style = 0;
	clump->n = n;
	clump->normalIndex = -3; 	/* An illegal value, used either for facet color or facet normal index */
	clump->vertexIndices = vertexIndices;
	clump->txbIndex = txbIndex;
	if (format.vertex == TM_VERTEX_LIT) {
		clump->style |= OP_Normals;
	} else {
		switch (format.color) {
			case TM_COLOR_INDEX:
				if (format.opt & TM_INLINE_COLOR) {
					clump->style |= OP_Colors;
					clump->colorIndices = colorIndices;
				} else {
					printf("Format TM_COLOR_INDEX not supported without TM_INLINE_COLOR\n");
					return GFX_MiscError;
				}
				break;
/* <Reddy 9-27-95> - not needed
			case TM_COLOR_FACET:
				clump->style |= OP_PerFacet | OP_Colors;
				clump->rgba = *facetColor;
				goto fancheck;
*/
			case TM_COLOR_FACET_LIT:
				clump->style |= OP_PerFacet | OP_Normals;
				clump->normalIndex = colorIndex;
fancheck:
				if (format.type != TM_FAN) {
					printf("Format type TM_STRIP not supported with color style TM_COLOR_FACET_???\n");
					return GFX_MiscError;
				}
				break;

			case TM_COLOR_FACET_INDEX:
				if (format.opt & TM_INLINE_COLOR) {
					clump->style |= OP_PerFacet | OP_Colors;
				/* <Reddy 9-27-95> use normalIndex for the facet color index */
					clump->normalIndex = colorIndex;
					goto fancheck;
				} else {
					printf("Format TM_COLOR_FACET_INDEX not yet implemnted \n");
					return GFX_MiscError;
				}

			case TM_COLOR_NONE:
				break;
			default:
				printf("Illegal color format %d \n",format.color);
				return GFX_MiscError;
		}
	}
	switch (format.texCoord) {
		case TM_TEXCOORD_NONE:
			break;
		case TM_TEXCOORD_INDEX:
			if (format.opt & TM_INLINE_TEXCOORD) {
				clump->style |= OP_TexCoords;
				clump->textureIndices = textureIndices;
			} else {
				printf("Format TM_TEXCOORD_INDEX not supported without TM_INLINE_TEXCOORD\n");
				return GFX_MiscError;
			}
			break;
		default:
			printf("Illegal texture format %d \n",format.texCoord);
			return GFX_MiscError;
	}

	tm->totalStyles |= clump->style;
	if ((tm->styleElements != clump->style) ||
		(tm->styleElements & OP_TexCoords) &&
		(tm->styleTxbIndex != txbIndex)) {

		/* round up the size to be a multiple of 4 */
		ROUND(tm->sizeElements);

		/* start counting for a new elemnt */
		tm->numElements ++;
		/* Based on the style add in the overhead for this element */
		tm->sizeElements += 8;		/* nwords style */
		tm->styleElements = clump->style;
		tm->styleTxbIndex = txbIndex;
		if (tm->styleElements & OP_TexCoords) {
			tm->sizeElements += 4;  /* For TexBlendIndex */
		}
		tm->sizeElements += 4; /* nStripFans */
	}
	if (clump->style & OP_PerFacet) {
		tm->sizeElements += 2 + n*2; /* nIndices + n indices */
		if (tm->styleElements & OP_Colors) {
			tm->sizeElements += 16; /* For rgba */
		}
		if (tm->styleElements & OP_TexCoords) {
			tm->sizeElements += n*4;	/* For uv */
		}
		if (tm->styleElements & OP_Normals) {
			tm->sizeElements += 2;		/* For ivNorm */
		}
		if (tm->styleElements & OP_Colors) {
			/* The OP_Colors per facet type is aligned
			   for each strip !!! */
			/* round up the size to be a multiple of 4 */
			ROUND(tm->sizeElements);
		}
	} else {
		/* Now add in bytes for this strip */
		tm->sizeElements += 2 + n*2; /* nIndices + n indices */
		if (tm->styleElements & OP_Colors) {
			tm->sizeElements += n*4;	/* For rgba */
		}
		if (tm->styleElements & OP_TexCoords) {
			tm->sizeElements += n*4;	/* For uv */
		}
	}

	if (tm->Lstrips == 0) {
		tm->Fstrips = clump;
	} else {
		tm->Lstrips->next = clump;
	}
	tm->Lstrips = clump;
	return GFX_OK;
}

static Err
TM_Bbox( TmTriMesh *tm, Box3 *bbox )
{	
	int i;

	/* for now deal with only lit case */
	if( tm->lits.n > 0 )
	{
		Box3_Around(bbox, tm->lits.verts, tm->lits.verts);
		Box3_ExtendPts(bbox, tm->lits.verts, tm->lits.n);
		
		return GFX_OK;
	}
	else if ( tm->nonLits.n > 0 )
	{
		Box3_Around(bbox, tm->nonLits.verts, tm->nonLits.verts);
		Box3_ExtendPts(bbox, tm->nonLits.verts, tm->nonLits.n);
		
		return GFX_OK;
	} else return GFX_BoundsEmpty;		
}

static Err
TM_HeaderInfo( 
	TmTriMesh *tm, 
	uint32 *vsize, 
	uint32 *visize, 
	uint32 *tcsize,
	uint32 *tri_count )
{	
	TmStripFan	*stripFan;
	uint32 style = tm->Fstrips->style;		/* start with 1st primitive style */
	int32 txbIndex = -3;					/* An illegal value */
	uint32		i;

	*tri_count = 0;
	*visize = 0;				
	*tcsize = 0;
					
	/* if the number of vertices are odd then duplicate last vertex */
	if (tm->nonLits.n > 0)
		*vsize = tm->nonLits.n;
	else
		*vsize = tm->lits.n;	
	
	for (stripFan = tm->Fstrips; stripFan != 0; stripFan = stripFan->next) 
	{
		/* style across all strips&fans should be same for now 
		   texture coordinates should exist for now */
		if ( style != stripFan->style )
		{
			fprintf( stdout, "Warning: Dissimilar styles ( %d != %d ) in TriMesh\n",
				style, stripFan->style );
			continue; 
		}
		
		/* one index for texture change */	
		if (txbIndex != stripFan->txbIndex)
		{
			*visize += 1;		
			txbIndex = stripFan->txbIndex;
		}
		
		/* number of vertex indices for each strip or fan */
		*visize += stripFan->n;
		if (stripFan->cont == 0) *tri_count += ( stripFan->n - 2 );
		else *tri_count += stripFan->n;	/* first tri has 3 vertices */
		
		/* size for the texture coordinates */
		if (style & OP_TexCoords) *tcsize += stripFan->n;
	}
	/* one last fake index and a dummy index -1 to flag the end */
	*visize += 2;
	
	/* in the number of vertex indices are odd then pad it with a half word */
	if( (*visize) % 2 ) {
		*visize += 1;
	}
	/*
	if( (*vsize) % 2 )  {
		*vsize += 1;
	}
	*/
	
#if 0
	fprintf( stdout, "Triangle Count in POD = %d\n", *tri_count );
#endif

	return GFX_OK;
}

#define ABSF( x ) ( ( x < 0 ) ? -1.0 * x : x )

static float
PODS_CalcMagicNum( Box3 bbox )
{
	float xyzmagic;
    float fv, pf;
    unsigned int *data = ( unsigned int *)&xyzmagic;
    unsigned int exp, mant, sign;

	/* calculate the XYZ magic number */
	xyzmagic = ABSF(bbox.min.x);
	if ( xyzmagic < ABSF( bbox.min.y ) ) xyzmagic = ABSF( bbox.min.y );
	if ( xyzmagic < ABSF( bbox.min.z ) ) xyzmagic = ABSF( bbox.min.z );
	if ( xyzmagic < ABSF( bbox.max.x ) ) xyzmagic = ABSF( bbox.max.x );
	if ( xyzmagic < ABSF( bbox.max.y ) ) xyzmagic = ABSF( bbox.max.y );
	if ( xyzmagic < ABSF( bbox.max.z ) ) xyzmagic = ABSF( bbox.max.z );

    exp = data[0] & 0x7f800000;
    mant = data[0] & 0x007fffff;
    sign = data[0] & 0x80000000;
	
    exp = ( (exp >> 23) + 1 ) << 23;

    data[0] = sign | exp;

    return( xyzmagic );
}

#define PCLK    0x8000
#define PFAN    0x4000
#define CFAN    0x2000
#define NEWS    0x1000
#define STXT    0x0800


typedef struct PodChunkHeader
{
	uint32	flags;
	uint16	caseType;		/* POD case type */
	uint16	texPageIndex;	/* POD texture page index */
	uint32	materialId;
	uint32	lightId;
	uint16	numSharedVerts;
	uint16	numVerts;
	uint32	numVertIndices;
	uint32	numTexCoords;
	uint32	numTriangles;

    float fxmin,fymin,fzmin;
    float fxextent,fyextent,fzextent;
} PodChunkHeader;

static void
POD_DumpGeometry( long *geom )
{
	float xyzmagic, fracxyz, fracnxyz;
	float *flt, x, y, z, nx, ny, nz;
	uint32 ix, iy, iz, inx, iny, inz;
	int16 *i16, voffset, uvoffset;
	uint16 *ui16;
	uint32 i;
	float *pvertex, *puv;
	int16 *pindex;
	SurfToken *tok;
	PodChunkHeader hd;
	
	/* Dump the Header */
	tok = (SurfToken *)geom;	
	memcpy( &hd, tok, sizeof( PodChunkHeader ) );

#if 0
	fprintf( stderr, "Flags = 0x%x, Case = %d\n", hd.flags, hd.caseType );
#endif

	fprintf( stdout, "Geometry Header :\n" );

	fprintf( stdout, "\tMin Point = (%f, %f, %f)\n", hd.fxmin, hd.fymin, hd.fzmin );
	fprintf( stdout, "\tExtents   = (%f, %f, %f)\n", hd.fxextent, hd.fyextent,
									hd.fzextent );

	tok = (SurfToken *)( (char *)tok + sizeof( PodChunkHeader ) );

	/* print the vertex and normal data */
	fprintf( stdout, "Vertex Data :\n" );
	fprintf( stdout, "\tNumber of vertices      %d\n", hd.numVerts );
	fprintf( stdout, "\tNumber of triangles      %d\n", hd.numTriangles);
	for( i = 0; i < hd.numVerts; i++ )
	{
		fprintf( stdout, "\t V[%d] = %f, %f, %f, %f, %f, %f\n",
			i, tok[0].f, tok[1].f, tok[2].f,	
			tok[3].f, tok[4].f, tok[5].f );
		tok += 6;
	}
    ui16 = (uint16*)tok;
    tok = (SurfToken *)( ui16 + hd.numVertIndices );

	fprintf( stdout, "Vertex Indices :\n" );
	fprintf( stdout, "\tNumber of indices       %d\n", hd.numVertIndices );
	for( i = 0; i < (hd.numVertIndices - 2); i++ )
	{	
		fprintf( stdout, "\t%d%s%s%s%s%s\n", (ui16[0] & 0x07ff),
		( (ui16[0]&PCLK)?" + PCLK":"" ),
		( (ui16[0]&PFAN)?" + PFAN":"" ),
		( (ui16[0]&NEWS)?" + NEWS":"" ),
		( (ui16[0]&CFAN)?" + CFAN":"" ),
		( (ui16[0]&STXT)?" + STXT":"" ) );
		
		ui16++;
	}
	/* write the last pair of indices */
	if( ((int16)ui16[0]) == -1 )
	{
		/* one last dummy texture index */
		fprintf( stdout, "\t%d\n", (int16)(ui16[0]) );
		/* next one is a pad halfword */
	} else {
		fprintf( stdout, "\t%d%s%s%s%s%s\n", (ui16[0] & 0x07ff),
		( (ui16[0]&PCLK)?" + PCLK":"" ),
		( (ui16[0]&PFAN)?" + PFAN":"" ),
		( (ui16[0]&NEWS)?" + NEWS":"" ),
		( (ui16[0]&CFAN)?" + CFAN":"" ),
		( (ui16[0]&STXT)?" + STXT":"" ) );
		fprintf( stdout, "\t%d\n", (int16)(ui16[1]) );
	}
	ui16 += 2; 
	/* one for fake index */
	
	fprintf( stdout, "Texture Coordinates :\n" );
	fprintf( stdout, "\tNumber of tex coords    %d\n", hd.numTexCoords );

	for( i = 0; i < hd.numTexCoords; i++ )
	{	
		fprintf( stdout, "\tUV = %f, %f\n", tok[0].f, tok[1].f );
		tok += 2;
	}
}

/* 
** primitive for Mercury PODS - <Reddy 1-8-96> 
** snake rules : Starts with a Strip oriented CCW
**               Strip ( last tri ) -> Fan   = same orientation
**               Fan ( last tri )   -> Strip = opposite orientation
** Pod format :
**			Num Vertices	= 1 uint32 
**			Num Indices 	= 1 uint32 
**			Num Tex Coords	= 1 uint32 
**			Min Point		= 3 float32 s
**			BBox extents	= 3 float32 s
**			Vertex Data		= 6 float32 s * Num Vertices
**			Index  Data		= 1 int16 * Num Indices 
**			Tex UV Data		= 2 float32 s * Num Tex Coords 
*/
struct PrimHeader *
TM_Compile( TmTriMesh *tm )
{
	struct PrimHeader 	*ph;
	SurfToken	*tok, *stok, *ttok;
	SurfToken	*elementHeader;
	SurfToken	*elementNstrip=0;
	TmStripFan	*stripFan, *pstripFan;
	uint32		size, numStripFans, i;
	int16		*tok16;
	uint16		*utok16, *tcutok16, flags;
	Box3 		bbox;
	Err			result;
	uint32 style = tm->Fstrips->style;		/* start with 1st primitive style */
	int32 		txbIndex = -3;		/* An illegal value */
	uint32 pfan = 0, cfan;
	uint16 pclk = 0, clk;
	uint16 news, stxt;
	PodChunkHeader hd;	
	int32 nonlit_transflag = 0;
#ifdef INTERFACE_CODE
	int32 podsize;
	char *podptr;
	int32 facetSurface = 0;
	int32 match;
	int32 fvertcount = 0, tmpfv;
	int16 *fvertidx = NULL;
	int16 *invidx = NULL;
	int32 j;
	uint16 *svtok;
	int32 odd = 0;
	uint32 numverts;
	gTexBlend *txb;
#endif

	if (facetSurfaceFlag)
		facetSurface = 1;
	/* get the data size */
	result = TM_HeaderInfo( tm, 
			&numverts, 
			&hd.numVertIndices, 
			&hd.numTexCoords, 
			&hd.numTriangles );
	if (numverts % 2)
		hd.numVerts = numverts+1;
	else
		hd.numVerts = numverts;

	hd.numSharedVerts = 0;
	if ( result != GFX_OK ) return NULL;

	result = TM_Bbox( tm, &bbox );

	/* initialize rest of the data */
	/* only two case types are written out */
    if (tm->nonLits.n > 0)
	{
		if( hd.numTexCoords == 0 ) hd.caseType = PreLitCase;
		else hd.caseType = PreLitTexCase;
	}
	else
	{
		if( hd.numTexCoords == 0 ) hd.caseType = DynLitCase;
		else hd.caseType = DynLitTexCase;
	}

	hd.flags = 0;
	hd.texPageIndex = 0;
	hd.lightId = 0;
	hd.fxmin = bbox.min.x;
	hd.fymin = bbox.min.y;
	hd.fzmin = bbox.min.z;
	hd.fxextent = bbox.max.x - bbox.min.x;
	hd.fyextent = bbox.max.y - bbox.min.y;
	hd.fzextent = bbox.max.z - bbox.min.z;
	/* set the material index */
	if( tm->mats.n > 0 )
	{
		if( tm->mats.n > 1 )
			fprintf( stderr, "WARNING: Using  material[0] ( %d )\n", tm->mats.n );
		hd.materialId = tm->mats.matList[0].material;
	} else hd.materialId = -1;

#if 0
	fprintf( stderr, "Material Index = %d\n", hd.materialId );
	fprintf( stderr, "**Flags = 0x%x, Case = %d\n", hd.flags, hd.caseType );
#endif

	/* Figure out how big the thing is and allocate it */
	/* header will contain : Pmin, Pextent, nverts, nindices, ntcoords */
	/* word align the index data */
	
	if (facetSurface) {
	  fvertidx = (int16 *)malloc((hd.numVerts+2) * sizeof(int16));
	  invidx = (int16 *)malloc((hd.numVerts+2) * sizeof(int16));
	  for (i = 0; i < hd.numVerts; i++) {
		/* printf("x = %f, y = %f, z = %f\n",
			tm->lits.verts[i].x, tm->lits.verts[i].y, tm->lits.verts[i].z); */
		fvertidx[i] = 0;
		invidx[i] = 0;
	  }
	  /* select vertex which has unique locations and normals */
	  for (i = 0; i < tm->lits.n; i+=3) {
		  match = 1;
		  for (j = 0; j < fvertcount; j++) {
		    if (FEQL(tm->lits.verts[fvertidx[j]].x, tm->lits.verts[i].x) &&
			  FEQL(tm->lits.verts[fvertidx[j]].y, tm->lits.verts[i].y) &&
			  FEQL(tm->lits.verts[fvertidx[j]].z, tm->lits.verts[i].z) &&
			  FEQL(tm->lits.normals[fvertidx[j]].x, tm->lits.normals[i].x) &&
			  FEQL(tm->lits.normals[fvertidx[j]].y, tm->lits.normals[i].y) &&
			  FEQL(tm->lits.normals[fvertidx[j]].z, tm->lits.normals[i].z))
		      match = 0;
		  }
		  if (match)
		  {
		    fvertidx[fvertcount] = i;
	        invidx[i] = fvertcount;
		    fvertcount++;
		  }
		  else
		  {
		    match = 1;
		    for (j = 0; j < fvertcount; j++) {
		      if (FEQL(tm->lits.verts[fvertidx[j]].x, tm->lits.verts[i+1].x) &&
			    FEQL(tm->lits.verts[fvertidx[j]].y, tm->lits.verts[i+1].y) &&
			    FEQL(tm->lits.verts[fvertidx[j]].z, tm->lits.verts[i+1].z) &&
			    FEQL(tm->lits.normals[fvertidx[j]].x, tm->lits.normals[i+1].x) &&
			    FEQL(tm->lits.normals[fvertidx[j]].y, tm->lits.normals[i+1].y) &&
			    FEQL(tm->lits.normals[fvertidx[j]].z, tm->lits.normals[i+1].z))
		        match = 0;
		    }
		    if (match)
		    {
		      fvertidx[fvertcount] = i+1;
	          invidx[i+1] = fvertcount;
		      fvertcount++;
		    }
			else
			{
		      match = 1;
		      for (j = 0; j < fvertcount; j++) {
		        if (FEQL(tm->lits.verts[fvertidx[j]].x, tm->lits.verts[i+2].x) &&
			      FEQL(tm->lits.verts[fvertidx[j]].y, tm->lits.verts[i+2].y) &&
			      FEQL(tm->lits.verts[fvertidx[j]].z, tm->lits.verts[i+2].z) &&
			      FEQL(tm->lits.normals[fvertidx[j]].x, tm->lits.normals[i+2].x) &&
			      FEQL(tm->lits.normals[fvertidx[j]].y, tm->lits.normals[i+2].y) &&
			      FEQL(tm->lits.normals[fvertidx[j]].z, tm->lits.normals[i+2].z))
		          match = 0;
		      }
		      if (match)
		      {
		        fvertidx[fvertcount] = i+2;
	            invidx[i+2] = fvertcount;
		        fvertcount++;
		      }
			}
		  }
	  }
	  /* select remaining vertex which has unique locations */
	  for (i = 0; i < tm->lits.n; i++) {
		match = 1;
		for (j = 0; j < fvertcount; j++) {
		  if (FEQL(tm->lits.verts[fvertidx[j]].x, tm->lits.verts[i].x) &&
			FEQL(tm->lits.verts[fvertidx[j]].y, tm->lits.verts[i].y) &&
			FEQL(tm->lits.verts[fvertidx[j]].z, tm->lits.verts[i].z)) {
		  /*
		  if ((tm->lits.verts[fvertidx[j]].x == tm->lits.verts[i].x) &&
			(tm->lits.verts[fvertidx[j]].y == tm->lits.verts[i].y) &&
			(tm->lits.verts[fvertidx[j]].z == tm->lits.verts[i].z)) {
		  */
			  match = 0;
			  break;
		  }
	    }
	    if (match) {
		  /* printf("round 2:index = %d\n", i); */
		  fvertidx[fvertcount] = i;
	      invidx[i] = fvertcount;
		  fvertcount++;
	    }
	  }
	  if (fvertcount % 2)
		odd = 1;
	  hd.numSharedVerts = hd.numVerts - fvertcount - odd;
	  hd.numVerts = fvertcount + odd;
	  /* two short to represent those data */
	  for (i = 0; i < hd.numVerts + hd.numSharedVerts; i++) {
		match = 1;
		for (j = 0; j < fvertcount; j++) {
		  if (fvertidx[j] == i)
		  {
			  match = 0;
			  break;
		  }
	    }
	    if (match) {
		  /* printf("round 3:index = %d\n", i); */
		  fvertidx[fvertcount] = i;
	      invidx[i] = fvertcount;
		  fvertcount++;
	    }
	  }
	  /* hd.numTexCoords = hd.numVerts + hd.numSharedVerts; */
	  size = sizeof(PrimHeader) 
		   + sizeof( PodChunkHeader)
		   + hd.numVerts * 24		    
		   + hd.numSharedVerts * 4
		   + hd.numVertIndices * 2				  
		   + hd.numTexCoords * 8;
	}
	else { 
	  size = sizeof(PrimHeader) 
		   + sizeof( PodChunkHeader)		/* POD header size */
		   + hd.numVerts * 24				/* 24 bytes per vertex */
		   + hd.numVertIndices * 2				/* 2 bytes per index */
		   + hd.numTexCoords * 8;				/* 8 bytes for each UV pair */
	}

	/* ph = (PrimHeader*) Mem_Alloc(size); */
	ph = (PrimHeader*) malloc(size);
	if (ph == NULL) return ph;
	
	ph->nextPrim = 0;
	ph->primType = GEO_TriMesh;
	ph->primSize = size;
	ph->texgenkind = tm->styleTexgen;
	stok = (SurfToken*)(ph); 
	tok = (SurfToken*)(ph+1);
#ifdef INTERFACE_CODE
	ph->texpage = -1;
	ph->subpage = -1;
	ph->podindex = mercid;
	mercid++;
#endif

	/* copy POD header info */
	memcpy( tok, &hd, sizeof( PodChunkHeader ) );

	tok = (SurfToken *)( (char *)tok + sizeof( PodChunkHeader ) );
	
	/* encode the vertex and normal data */
    if (tm->nonLits.n > 0)
	{
		for( i = 0; i < tm->nonLits.n; i++ )
		{
			tok[0].f = tm->nonLits.verts[i].x;
			tok[1].f = tm->nonLits.verts[i].y;
			tok[2].f = tm->nonLits.verts[i].z;
			tok[3].f = tm->colors.colors[i].r;
			tok[4].f = tm->colors.colors[i].g;
			tok[5].f = tm->colors.colors[i].b;
			if (tm->colors.colors[i].a < 1.0)
				nonlit_transflag = 1;
			tok += 6;
		}
   	 /* vertices should be in pairs - duplicate the last one to make even number */
		if ( tm->nonLits.n % 2 )
		{
			tok[0].f = tm->nonLits.verts[tm->nonLits.n-1].x;
			tok[1].f = tm->nonLits.verts[tm->nonLits.n-1].y;
			tok[2].f = tm->nonLits.verts[tm->nonLits.n-1].z;
			tok[3].f = tm->colors.colors[tm->nonLits.n-1].r;
			tok[4].f = tm->colors.colors[tm->nonLits.n-1].g;
			tok[5].f = tm->colors.colors[tm->nonLits.n-1].b;
			tok += 6;
		}
	}
	else
	{
	  if (facetSurface) {
		for( i = 0; i < hd.numVerts-odd; i++ )
		{
			tok[0].f = tm->lits.verts[fvertidx[i]].x;
			tok[1].f = tm->lits.verts[fvertidx[i]].y;
			tok[2].f = tm->lits.verts[fvertidx[i]].z;
			tok[3].f = tm->lits.normals[fvertidx[i]].x;
			tok[4].f = tm->lits.normals[fvertidx[i]].y;
			tok[5].f = tm->lits.normals[fvertidx[i]].z;
			tok += 6;
		}
		if (odd)
		{
			if ((fvertidx[hd.numVerts-1] > 0) && (fvertidx[hd.numVerts-1] < hd.numVerts))
				i = fvertidx[hd.numVerts-1];
			else
				i = hd.numVerts-odd-1;
			tok[0].f = tm->lits.verts[i].x;
			tok[1].f = tm->lits.verts[i].y;
			tok[2].f = tm->lits.verts[i].z;
			tok[3].f = tm->lits.normals[i].x;
			tok[4].f = tm->lits.normals[i].y;
			tok[5].f = tm->lits.normals[i].z;
			/* printf("%f %f %f %f %f %f\n", tok[0].f, tok[1].f, tok[2].f,
					tok[3].f, tok[4].f, tok[5].f); */
			tok += 6;
		}
		svtok = (uint16 *)tok;
		for( i = hd.numVerts; i < hd.numVerts+hd.numSharedVerts; i++ )
		{
		  for (j = 0; j < hd.numVerts; j++) {
			if ((tm->lits.verts[fvertidx[j]].x == tm->lits.verts[fvertidx[i]].x) &&
				(tm->lits.verts[fvertidx[j]].y == tm->lits.verts[fvertidx[i]].y) &&
				(tm->lits.verts[fvertidx[j]].z == tm->lits.verts[fvertidx[i]].z)) {
			  *svtok = j;
			  break;
			}
		  }
		  svtok++;
		  *svtok = fvertidx[i]/3;
		  svtok++;
#if 0
		  for (j = 0; j < hd.numVerts; j++) {
			/*
			if ((tm->lits.normals[fvertidx[j]].x == tm->lits.normals[fvertidx[i]].x) &&
				(tm->lits.normals[fvertidx[j]].y == tm->lits.normals[fvertidx[i]].y) &&
				(tm->lits.normals[fvertidx[j]].z == tm->lits.normals[fvertidx[i]].z)) {
			*/
			if (FEQL(tm->lits.normals[fvertidx[j]].x, tm->lits.normals[fvertidx[i]].x) &&
				FEQL(tm->lits.normals[fvertidx[j]].y, tm->lits.normals[fvertidx[i]].y) &&
				FEQL(tm->lits.normals[fvertidx[j]].z, tm->lits.normals[fvertidx[i]].z)) {
			  *svtok = j;
			  /* printf("match normal i = %d, j = %d\n", i, j); */
			  break;
			}
		  }
		  svtok++;
#endif
		}		
	  }
	  else {
		for( i = 0; i < tm->lits.n; i++ )
		{
			tok[0].f = tm->lits.verts[i].x;
			tok[1].f = tm->lits.verts[i].y;
			tok[2].f = tm->lits.verts[i].z;
			tok[3].f = tm->lits.normals[i].x;
			tok[4].f = tm->lits.normals[i].y;
			tok[5].f = tm->lits.normals[i].z;
			tok += 6;
		}
   	 /* vertices should be in pairs - duplicate the last one to make even number */
		if ( tm->lits.n % 2 )
		{
			tok[0].f = tm->lits.verts[tm->lits.n-1].x;
			tok[1].f = tm->lits.verts[tm->lits.n-1].y;
			tok[2].f = tm->lits.verts[tm->lits.n-1].z;
			tok[3].f = tm->lits.normals[tm->lits.n-1].x;
			tok[4].f = tm->lits.normals[tm->lits.n-1].y;
			tok[5].f = tm->lits.normals[tm->lits.n-1].z;
			tok += 6;
		}
	  }
	}

	if ((nonlit_transflag) && (hd.caseType == PreLitCase))
		hd.caseType = PreLitTransCase;
	else if ((nonlit_transflag) && (hd.caseType == PreLitTexCase))
		hd.caseType = PreLitTransTexCase;

	/* vertex indices, and texture coordinates */
	if (hd.numSharedVerts)
	  utok16 = (uint16*)tok + hd.numSharedVerts * 2;
	else
	  utok16 = (uint16*)tok;

	if( hd.numTexCoords != 0 )
		tok = (SurfToken *)( (char *)(ph + 1) + 
				( sizeof( PodChunkHeader) + hd.numVerts * 24 + 
				hd.numSharedVerts * 4 +
				hd.numVertIndices * 2 ) );
	
	/* first prim is a STRIP and the first tri is CCW */
	pfan = 0; pclk = 0;
	
	for (pstripFan = stripFan = tm->Fstrips; stripFan != 0; stripFan = stripFan->next) 
	{
		/* this is part of snake primitive */
		if ( ( stripFan != tm->Fstrips ) && (stripFan->cont == 0) ) news = NEWS;
		else news = 0;
		
		/* style across all strips&fans should be same for now 
		   texture coordinates should exist for now */
		if ((style != stripFan->style) && !(style & OP_TexCoords) )
			continue; 
		
		for ( i = 0; i < stripFan->n; i++) 
		{
			flags = 0; stxt = 0; clk = 0; cfan = 0;
		
			/* figure out the five index flags for each vertex  */
			clk = 0;						
			if( stripFan->type == TM_FAN ) cfan = CFAN;
			
			if ( !((i <= 2) && ( stripFan->cont == 0 )) ) 
			{
				/* triangle direction simply alternates from the previous one */
				if ( pclk == PCLK ) clk = 0;
				else clk = PCLK;
			}
		
			flags |= pclk;					/* previous tri direction */
						
			pclk = clk;
			flags |= ( cfan | pfan );		/* previous and current prim type */
			if( news ) {
				flags |= news;				/* this is part of snake primitive */
				flags |= CFAN;				/* whenever NEWS is set you need to set CFAN */
				news = 0;
			}
			if( cfan ) pfan = PFAN;
			else pfan = 0;
			
			/* vertex index is > 12 bit precision */
			if( stripFan->vertexIndices[i] > 2048 )
				fprintf( stderr, "Error: vertex index ( %d ) is > 2048\n",
					stripFan->vertexIndices[i] );
			/* one index for texture change */
			if (txbIndex != stripFan->txbIndex)
			{
#ifdef INTERFACE_CODE
				int16 subindex = -1;
				char name[64];
				int texpage = 0, subpage = 0;
#endif
				stxt = STXT;
				flags |= stxt;
						
				/* vertex index with encoded flages */
				if (facetSurface)
				{
			      /* printf("old index = %d, new index = %d\n",
					stripFan->vertexIndices[i],
					invidx[stripFan->vertexIndices[i]]); */
				  *utok16++ = ((int16)invidx[stripFan->vertexIndices[i]]) | flags;
				}
				else
				  *utok16++ = ((int16)stripFan->vertexIndices[i]) | flags;

#ifndef INTERFACE_CODE
				if( ((int16)stripFan->txbIndex) < 0 ) 
				{
					if ( ((int16)stripFan->txbIndex) == -2 ) 
						*utok16++ = txbIndex;
					else *utok16++ = 0;
				} else *utok16++ = (int16)stripFan->txbIndex;

#endif
#ifdef INTERFACE_CODE
				/* intercept the texture index here */
				if( ((int16)stripFan->txbIndex) < 0 )
				{
					if ( ((int16)stripFan->txbIndex) == -2 ) 
						subindex = txbIndex;
					/* else if ( ((int16)stripFan->txbIndex) == -1 ) 
						subindex = -1; */
					else subindex = 0;
				} 
				else 
					subindex = (int16)stripFan->txbIndex;

				txb = NULL;
				if (HasTexPage())
				{
					GetTexEntry(subindex, name, &texpage, &subpage);
					if (subindex >= 0)
						txb = Glib_GetTexBlend(subindex);
					if (txb && (txb->pageindex >= 0))
					{
						texpage = txb->pageindex;
						subpage = txb->subindex;
						Glib_MarkTexBlendUsed(i);
					}
						subindex = (int16) subpage;
				}
				else
				{
					if (subindex >= 0)
						txb = Glib_GetTexBlend(subindex);
					if (txb && (txb->pageindex >= 0))
					{
						texpage = txb->pageindex;
						subpage = txb->subindex;
						Glib_MarkTexBlendUsed(i);
					}
						subindex = (int16) subpage;
				}

				*utok16++ = subindex;
				if (ph->texpage == -1)
					ph->texpage = texpage;
				else if ((ph->texpage != -1) && (ph->texpage != texpage))
					printf("Pod's textures are in different texture pages\n");
				else
					ph->texpage = texpage;
				ph->subpage = subpage;
#endif
				txbIndex = stripFan->txbIndex;
			} else {		
				/* vertex index with encoded flages */
			  if (facetSurface)
			  {
			      /* printf("old index = %d, new index = %d\n",
					stripFan->vertexIndices[i],
					invidx[stripFan->vertexIndices[i]]); */
				*utok16++ = ((int16)invidx[stripFan->vertexIndices[i]]) | flags;
			  }
			  else
				*utok16++ = ((int16)stripFan->vertexIndices[i]) | flags;
			}
			/* UV texture coordinates, encoded to be 12.4 */
			if( hd.numTexCoords != 0 )
			{	
	  			if (facetSurface)
				{
					int32 txidx;
					txidx = invidx[stripFan->textureIndices[i]];
					(tok++)->f = tm->texs.coords[stripFan->textureIndices[i]].u;
					(tok++)->f = tm->texs.coords[stripFan->textureIndices[i]].v;
				}
				else
				{
					(tok++)->f = tm->texs.coords[stripFan->textureIndices[i]].u;
					(tok++)->f = tm->texs.coords[stripFan->textureIndices[i]].v;
				}
			}
		}
		pstripFan = stripFan;
	}
	/* end the last one with fake index 0 + STXT + NEWS + CFAN */
	*(utok16++) = STXT | NEWS | CFAN | pclk | pfan;
	
	/* xformoffset(uint16) and flags(uint16). set to 0 */
	/* (tok++)->u = 0; */

	/* a dummy index -1 to flag the end */
	tok16 = (int16 *)utok16;
	*tok16 = -1;
	
	if (GetMessageFlag())
		POD_DumpGeometry( (long *)( ph + 1 ) );

	if (fvertidx)
		free(fvertidx);
	if (invidx)
		free(invidx);
	return ph;
}

void
TM_Free( TmTriMesh *tm )
{
  TmStripFan *sf;

  while (tm->Fstrips != 0) {
	sf = tm->Fstrips->next;
	Mem_Free(tm->Fstrips);
	tm->Fstrips = sf;
  }
  tm->Lstrips = 0;
}

#ifdef _DEBUG
const float i64 = 1./64.;

static SurfToken*
AlignToken(int16* t16)
{
    uint32 it = (uint32)t16;

	if (it & 3) it += 4 - (it & 3);
    return (SurfToken*)it;
}

static void
printStyle(char*s, uint32 style)
{
	printf("%s",s);
	if (style & OP_PerFacet) printf("OP_PerFacet");
	if (style & OP_Colors) printf(" OP_Colors");
	if (style & OP_Normals) printf(" OP_Normals");
	if (style & OP_TexCoords) printf(" OP_TexCoords");
	printf("\n");
}

static SurfToken *tok;

static int
PrintPerVertex(int style, int nNonLit, int nLit)
{
	int16 nStripFans, textureIndex;
	int16  *tok16;
	uint16 *utok16;
	uint16 unum16;
	int32 vcnt, j, n;
	TmFormat fmt;

	printStyle("style =",style);
	if (style & OP_TexCoords) {
		textureIndex = tok++->u;
		printf("textureIndex = %d, ",textureIndex);
	}
	nStripFans = tok++->u;
	printf("nStripFans = %d\n",nStripFans);

	utok16 = (uint16*)tok;

	n = 0;
	printf("{\n");
    while (n < nStripFans ) {
		unum16 = (uint16)*utok16++;
		fmt.cont = unum16 >> 15;
		fmt.order = unum16 >> 14;
		fmt.type = unum16 >> 13;
        vcnt = unum16 & 0x1FFF;
		if ( fmt.type == TM_STRIP ) {
			 printf("TriStrip {");
		} else {
			printf("TriFan {");
		}
		printf(" \nContinuity = %d, Order = %d\n", fmt.cont, fmt.order );

/*
		if (vcnt < 3) {
			printf("<Illegal size strip/fan %d vertices>\n",vcnt);
			return 1;
		}
		utok16 = (uint16 *)tok16;
*/
        for (j = 0; j < vcnt; j++) {
			if (style & OP_Normals) {
				if (utok16[0] >= nLit) {
					printf("<Illegal lit vertex index %d>\n",utok16[0]);
					return 1;
				}
			} else if (utok16[0] >= (nNonLit + nLit)) {
				printf("<Illegal non lit vertex index %d>\n",utok16[0]);
				return 1;
			}
			printf(" %d",utok16[0]);
			utok16++;

			if (style & OP_TexCoords) {
				printf(" {%f %f}", (float)utok16[0]*i64, (float)utok16[1]*i64);
				utok16 += 2;
			}

			if (style & OP_Colors) {
				uint8 *r = (uint8*)utok16;
				printf(" {%d %d %d %d}",r[0], r[1], r[2], r[3]);
				utok16 += 2;
			}
        }
        printf("}\n");
		n++;
		tok16 = (int16 *)utok16;
    }
	printf("}\n");

	tok = AlignToken(tok16);
	return 0;
}

static int
PrintPerFacet(int style, int nNonLit, int nLit)
{
	int16 nStripFans, textureIndex;
	int16  *tok16;
	int32 vcnt, j, n;

	printStyle("style =",style);
	if (style & OP_TexCoords) {
		textureIndex = tok++->u;
		printf("textureIndex = %d, ",textureIndex);
	}
	nStripFans = tok++->u;
	printf("nStripFans = %d\n",nStripFans);


	n = 0;
	printf("{\n");
    while (n < nStripFans ) {
		tok16 = (int16*)tok;
		printf("TriFan ");
		if (style & OP_Colors) {
			int icol;
			printf("{");
			for (icol = 0; icol<4; icol++) {
				if (tok->f < 0. ||
					tok->f > 1. ) {
					printf("<Illegal color component %f \n>",tok->f);
					return 1;
				}
				printf(" %5.4f", tok->f);
				tok++;
			}
			printf("}");
			tok16 = (int16*)tok;
		} else if (style & OP_Normals) {
			if (tok16[0] >= nLit ||
				tok16[0] < 0) {
				printf("<Illegal normal index %d>\n",tok16[0]);
				return 1;
			}
			printf(" {%d}",tok16[0]);
			tok16++;
		} 
        vcnt = *tok16++;
		if ( vcnt < 0 ) {
			 printf("Illegal number of indices %d",vcnt);
			 return 1;
		} 
/*
		if (vcnt < 3) {
			printf("<Illegal size strip/fan %d vertices>\n",vcnt);
			return 1;
		}
*/
		printf("{");
        for (j = 0; j < vcnt; j++) {

			if (tok16[0] >= nNonLit ||
				tok16[0] < 0) {
				printf("<Illegal vertex index %d>\n",tok16[0]);
				return 1;
			}
			printf(" %d",*tok16++);

			if (style & OP_TexCoords) {
				printf(" {%f %f}", (float)tok16[0]*i64, (float)tok16[1]*i64);
				tok16 += 2;
			}
        }
		if ((style & (OP_Colors | OP_PerFacet)) ==
			(OP_Colors | OP_PerFacet)) {
			tok = AlignToken(tok16);
		} else {
			tok = (SurfToken*)tok16;
		}
        printf("}\n");
		n++;
    }
	printf("}\n");
	tok = AlignToken((int16*)tok);
	return 0;
}

static int
PrintIllegalElement(int a, int b, int c)
{
	UNUSED(a);
	UNUSED(b);
	UNUSED(c);
	printf("<Illegal element in trimesh>\n");
	return 1;
}

static int (*PrintElementDispatch[])(int, int, int) = {
    &PrintIllegalElement,   /* No input not allowed */
    &PrintPerVertex,		/* Colors */
    &PrintPerVertex,		/* Normals */
    &PrintIllegalElement,   /* Colors and Normals not allowed */
    &PrintPerVertex,		/* TexCoords */
    &PrintPerVertex,		/* TexCoords Colors */
    &PrintPerVertex,		/* TexCoords Normals */
    &PrintIllegalElement,   /* TexCoords Colors and Normals not allowed */
    &PrintIllegalElement,   /* PerFacet and nothing */
    &PrintPerFacet,			/* PerFacet Colors */
    &PrintPerFacet,			/* PerFacet Normals */
    &PrintIllegalElement,	/* PerFacet Normals Colors */
    &PrintIllegalElement,	/* PerFacet Texture not allowed, same as per vertex */
    &PrintPerFacet,			/* PerFacet Texture/Colors */
    &PrintPerFacet,			/* PerFacet Texture/Normals */
    &PrintIllegalElement	/* Per facet textre, normals and tm->colors not allowed */
  };
#endif

void
TM_Print(struct PrimHeader *prim)
{
#ifdef _DEBUG
  Point3 *verts ;
  int32 i, num_verts, num_nonLitVerts, num_normals;
  uint32 styles;
  uint32  nNorms, nMaterial;
  Vector3 *norm;
  int16  *tok16;
  uint16 mat;
  SurfToken *lastToken;

  printf("Compiled TriMesh \n");

  lastToken = (SurfToken*)(((char*)prim) + prim->primSize);
  tok = (SurfToken*) (prim+1);

  num_normals = tok[0].tok16.h0;
  num_nonLitVerts = tok[0].tok16.h1;
  num_verts =  num_normals + num_nonLitVerts;
  styles = tok[1].tok16.h1;
  printf("#Lit vertices %d, #Non-lit vertices %d\n",
		 num_normals, num_nonLitVerts);
  printf("#elements = %d\n",tok[1].tok16.h0);
  printStyle("All Styles = ",styles);
  if (tok[2].u != 0) {
	Color4 *cols = (Color4*)tok[2].u;
	printf("Colors:\n");
	for (i=0; i<num_normals; i++) {
	  printf("%f %f %f\n",cols[i].r, cols[i].g, cols[i].b);
	}
  }
  if (((styles & OP_Normals) &&
	   (num_normals <= 0) ) ||
	  (!(styles & OP_Normals) &&
	   (num_normals > 0 ) ) ) {
	printf("<Inconsistent styles and normals >\n");
	return;
  }
  tok+=3;
		
  verts = (Point3 *)tok;

  printf("Vertices:\n");
		
  for (i = 0; i < num_verts; i++ ) {
	printf("{ %f %f %f}\n", verts[i].x, verts[i].y, verts[i].z);
  }

  tok += 3*num_verts;

  /* print out normals if present */

  if (styles & OP_Normals) {
	norm = (Vector3*)tok;
	printf("Normals:\n");
	for (i = 0; i < num_normals; i++ ) {
	  printf("{ %f %f %f}\n", norm->x, norm->y, norm->z);
	  norm++;
	}
	tok += 3*num_normals;
	nMaterial = tok++->u;
	printf("nMaterial = %d\n", nMaterial);
	tok16 = (int16 *)tok;
	for (; nMaterial>0; nMaterial--) {
	  mat = *tok16++;
	  nNorms = *tok16++;
	  printf("material 0x%lx, nNorms = %d\n", mat, nNorms);
	}

	tok = (SurfToken *)tok16;
  }

  /* Now go through all elements */

  while (tok < lastToken) {
	int 	nTokens = tok->u;
	SurfToken 	*firstTokenElement = tok++;
	uint32	style;

	style = tok++->u;
	i = style & 0x0f;   /* style look up the dispatch */
	if ((*PrintElementDispatch[i])(style, num_verts, num_normals)) return;
	if ((tok-firstTokenElement) != nTokens) {
	  printf("<Inconsistent nToken = %d should be %d>\n",
			 tok-firstTokenElement, nTokens);
	  return;
	}
  }
#endif /* _DEBUG*/
}
