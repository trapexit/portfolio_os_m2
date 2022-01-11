#ifndef TMUTILS_H
#define TMUTILS_H

typedef struct TmMaterial {
	uint16 material, nNorms;
} TmMaterial;

typedef struct TmFormat {
  unsigned int reserved : 2;
  unsigned int cont:1;
  unsigned int order:1;
  unsigned int planar:1;
  unsigned int type:1;
  unsigned int vertex:1;
  unsigned int color:3;
  unsigned int texCoord :1;
  unsigned int opt:5;
} TmFormat;

typedef struct TmNonLit {
	uint32	n;
	Point3	*verts;
} TmNonLit;

typedef struct TmLit {
	uint32	n;
	Point3	*verts;
	Vector3	*normals;
} TmLit;

typedef struct TmMat {
	uint32 		n;
	TmMaterial *matList;
} TmMat;

typedef struct TmTex {
	uint32		n;
	TexCoord 	*coords;
} TmTex;

typedef struct TmColor {
	uint32	n;
	Color4	*colors;
} TmColor;

typedef struct TmHint {
	uint32 n;
	uint32 *hint;
} TmHint;

typedef struct TmStripFan {
	struct		TmStripFan *next;
	bool		type;			/* strip/fan */
	bool		cont;			/* new/continue */
	bool		order;			/* counterclockwise/clockwise */
	bool		planar;			/* planar/nonplanar */
	uint32		style;
	uint32		n;
	uint32		normalIndex;
	uint32		txbIndex;
#if 0 
	/* <Reddy 9-28-95> - do not need this, instead use 'normalIndex'
	   for both facet color and facet normal indices. At compile time
	   look this color or normal in respective arrays */
	Color4		rgba; /* for the facet color r, g, b, a */
#endif 
	uint16*		vertexIndices;
	uint16*		textureIndices;
	uint16*		colorIndices;
} TmStripFan;

typedef struct TmTriMesh {
	/* counts */
	uint32		numElements;
	uint32		sizeElements;
	uint32		styleElements;
	int32		styleTxbIndex;
	int32		styleTexgen;
	uint32		totalStyles;

	/* trimesh data */
	TmNonLit	nonLits;
	TmLit		lits;
	TmMat		mats;
	TmTex		texs;
	TmColor		colors;
	TmHint		hint ;
	TmStripFan	*Fstrips;
	TmStripFan	*Lstrips;
} TmTriMesh;

#define TM_STRIP 				0
#define TM_FAN					1
#define TM_NEW					0
#define TM_CONTINUE				1
#define TM_CLOCKWISE			0
#define TM_COUNTERCLOCKWISE		1
#define TM_NONPLANAR			0
#define TM_PLANAR				1

#define TM_VERTEX_NONLIT		0
#define TM_VERTEX_LIT			1
#define TM_COLOR_NONE			0
#define TM_COLOR_INDEX			1
#define TM_COLOR_FACET_INDEX	2
#define TM_COLOR_FACET			3
#define TM_COLOR_FACET_LIT		4
#define TM_TEXCOORD_NONE		0
#define TM_TEXCOORD_INDEX		1
#define TM_COLOR_EQ_VERTEX		1
#define TM_TEXCOORD_EQ_COLOR	2
#define TM_TEXCOORD_EQ_VERTEX	4
#define TM_INLINE_COLOR			8
#define TM_INLINE_TEXCOORD		16


/* define the POD cases */
#define DynLitCase              0
#define DynLitTexCase           1
#define DynLitTransCase         2
#define DynLitTransTexCase      3
#define DynLitFogCase           4
#define DynLitFogTexCase        5  
#define DynLitFogTransCase      6
#define DynLitSpecCase          7
#define DynLitSpecTexCase       8
#define PreLitCase              9
#define PreLitTexCase           10
#define PreLitTransCase         11
#define PreLitTransTexCase      12
#define DynLitTransSpecCase     13

#define DynLitEnvCase           14
#define DynLitTransEnvCase      15
#define DynLitSpecEnvCase       16
#define PreLitEnvCase           17
#define PreLitTransEnvCase      18

/* flags bits in pod structure */
#define samecaseFLAG        (1 << (31-8))
#define sametextureFLAG     (1 << (31-9))
/* used in pploop, can be thrown away */
#define callatstartFLAG     (1 << (31-16))
#define casecodeisasmFLAG   (1 << (31-17))
#define usercheckedclipFLAG     (1 << (31-18))

/* okay to overwrite these in per vertex Draw routines */
#define environFLAG         (1 << (31-21))
#define specularFLAG        (1 << (31-22))
#define hithernocullFLAG    (1 << (31-23))

/* need to preserve these flags in per vertex Draw routines */
#define nocullFLAG      (1 << (31-28))
#define frontcullFLAG       (1 << (31-29))
#define clipFLAG        (1 << (31-30))
#define callatendFLAG       (1 << (31-31))


extern Err
TM_Init( TmTriMesh *tm );

extern void
TM_Free( TmTriMesh *tm );

extern struct PrimHeader*
TM_Compile( TmTriMesh *tm ); 

extern Err
TM_StripFan(TmTriMesh *tm, TmFormat format, uint32 n, uint16 *vertexIndices,
			uint16* textureIndices, uint16 *colorIndices,
			int32 txbIndex, uint16 colorIndex);

extern Err
TM_Hints(TmTriMesh *tm, uint32 numHints, uint32* hints);

Err
TM_Materials(TmTriMesh *tm, uint32 numMats, TmMaterial *mat);

extern Err
TM_Colors(TmTriMesh *tm, uint32 numColors, Color4 *colors);

extern Err
TM_TexCoords(TmTriMesh *tm, uint32 numCoords, TexCoord *coords);


extern Err
TM_LitVertices(TmTriMesh *tm, uint32 numNormals, Point3 *litVerts, Vector3 *normals);

extern Err
TM_NonLitVertices(TmTriMesh *tm, uint32 numVerts, Point3* verts);

extern void
TM_Print(struct PrimHeader*);

#endif /* TMUTILS_H */
