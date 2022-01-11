/****
 *
 *	@(#) surf.h 95/09/21 1.60
 *	Copyright 1994, The 3DO Company
 *
 * Surface Class
 * Recording engine for the 3D graphics pipeline
 *
 ****/
#ifndef _GPSURF
#define _GPSURF

/*
 * Material and Texture index options
 */
#define	SURF_None		-1		/* turn material/texture off */
#define	SURF_UseLast	-2		/* use material/texture from last primitve */

/*
 * Shading Enables for materials 
 */
#define MAT_Ambient		0x0001   /* ambient lighting computations */
#define MAT_Diffuse		0x0002   /* diffuse lighting computations */
#define MAT_Specular	0x0004   /* specular lighting computations */
#define MAT_Emissive	0x0008   /* emissive lighting computations */
#define MAT_TwoSided	0x0010   /* two-sided lighting computations */

/*
 * Material Properties
 */
typedef struct MatProp
{
  uint32	ShadeEnable;
  Color4	Diffuse;
  Color4	Specular;
  Color4	Emission;
  Color4	Ambient;
  float  Shine;
} MatProp;

/*
 * SurfaceData is a structure that represents the INTERNAL FORMAT of
 * class Surface in the C binding. DO NOT RELY ON THE FIELDS IN THIS
 * STRUCTURE - THEY ARE SUBJECT TO CHANGE WITHOUT NOTICE! GPSurface
 * attributes should only be accessed by the Surf_XXX functions.
 */
typedef struct SurfaceData
{
  GfxObj		m_Obj;
  char*		m_FirstPrim;		/* -> first primitive added */
  char*		m_LastPrim;			/* -> last primitive added */
  Box3		m_Bound;    		/* bounding box */

  uint32  m_NumTriangles;         /* Computed for statistical purposes only */
  uint32  m_NumVerts;             /* Computed for statistical purposes only */
} SurfaceData;

/*
 * Surface Flags
 */
#define SURF_UpdBounds          	1
#define SURF_BoundsEmpty        	2
#define SURF_NormalizedTexCoords 	4

/****
 *
 * Surface command codes
 * These are the opcodes which describe the commands stored in the surface.
 * The opcodes 1 - 8 are reserved for geometric primitives
 *
 ****/
#define	SURF_TriStrip	GEO_TriStrip	/* triangle strip */
#define	SURF_TriList	GEO_TriList		/* triangle list */
#define	SURF_TriFan		GEO_TriFan		/* triangle fan */
#define	SURF_TriMesh	GEO_GLTriMesh	/* triangle mesh */
#define	SURF_QuadMesh	GEO_QuadMesh	/* quadrilateral mesh */
#define	SURF_Lines		GEO_Lines		/* draw lines */
#define	SURF_Points		GEO_Points		/* draw points */
#define	SURF_Geometry	8				/* last geometric primitive */

#define	SURF_Color		9		/* surface color */
#define	SURF_Normal		10		/* surface normal */
#define	SURF_MatIndex	11		/* material index */
#define	SURF_TexIndex	12		/* texture index */
#define	SURF_Last		16		/* last allowable command */

typedef struct BlockHeader
{
  uint16	nLitVerts;
  uint16	nNonLitVerts;
  uint16	nElements;
  uint16	style;
  void*	colorPtr;
} BlockHeader;

typedef	struct PrimHeader *PrimHeaderPtr;

typedef struct PrimHeader
{
  PrimHeaderPtr nextPrim;
  uint32	primType;
  uint32	primSize;
  uint32	texpage;
  uint32	subpage;
  uint32	podindex;
  uint32	texgenkind;
} PrimHeader;

/*
 * TriMesh has been removed from the Graphics API but is still used
 * internally by GPSurface. GEO_TriMesh is the identifier for a compiled
 * surface block. GEO_GLTriMesh is the identifier for the Geometry
 * structure described below (used by GPSurface)
 */

typedef struct GeoPrim
{
  PrimHeader*	nextPrim;
  int32		primType;
  int32		primSize;
  int16		matIndex;
  int16		texIndex;
  int16		texgenkind;
  Geometry	geoData;
} GeoPrim;

#define OP_Colors       0x00000001
#define OP_Normals      0x00000002
#define OP_TexCoords    0x00000004
#define OP_PerFacet     0x00000008

/****
 *
 * SurfToken describes a token in the surface. A token is the
 * smallest unit of storage in a surface. It is a fixed-size (32 bit)
 * quantity that can store an integer or a floating point number.
 *
 ****/
typedef union SurfToken
{
  struct { int16 op; int16 size; } cmd;
  struct { int16 h0; int16 h1; } tok16;
  int32        i;
  uint32       u;
  float       f;
} SurfToken;

#endif /* GPSURF */

