/*
**	File:		bsdf_read.h	
**
**	Contains:	Header file for "Mercury" binary SDF data reader 
**
**	Written by:	Ravindar Reddy
**
**	Copyright:	© 1996 by The 3DO Company. All rights reserved.
**	          	This material constitutes confidential and proprietary
**	          	information of the 3DO Company and shall not be used by
**	          	any Person or for any purpose except as expressly
**	          	authorized in writing by the 3DO Company.
**
**	Last Modified:	 96/09/17	version		1.18	
*/

#ifndef _H_PBSDF
#define _H_PBSDF

/* include the animation engine and reader portion */
#define ANIM 1

#include "mercury.h"
#ifdef ANIM
#include "AM_Model.h"
#endif

/* Error Codes */
#define GFX_OK 0
#define GFX_ErrorInternal (-3)

#ifndef MAKE_ID
#define MAKE_ID(a,b,c,d) (((uint32)(a))<<24 | ((uint32)(b))<<16 | \
							((uint32)(c))<<8 | ((uint32)(d)))
#endif /* MAKE_ID */

/* data chunk ids */
#define ID_FORM MAKE_ID('F','O','R','M')
#define ID_MHDR MAKE_ID('M','H','D','R')
#define ID_SCEN MAKE_ID('S','C','E','N')
#define ID_CHAR MAKE_ID('C','H','A','R')
#define ID_SURF MAKE_ID('S','U','R','F')
#define ID_PODG MAKE_ID('P','O','D','G')
#define ID_MATA MAKE_ID('M','A','T','A')
#define ID_MODL MAKE_ID('M','O','D','L')

#ifdef ANIM
/* model, hierarchy and animation array chunks */
#define ID_AMDL MAKE_ID('A','M','D','{')    /* articulated model enclosing */
                                            /* HIER, MDLA, ANMA chunks */
#define ID_MDLA MAKE_ID('M','D','L','A')    /* model array chunk */
#define ID_HIER MAKE_ID('H','I','E','R')    /* hierarchy array chunk */
#define ID_ANMA MAKE_ID('A','N','M','A')    /* animation array chunk */
#define ID_ANIM MAKE_ID('A','N','I','M')    /* animation node chunk */
#endif

/* character flags */
#define SDFB_Char_HasTransform	0x01
#define SDFB_Char_Invisible		0x02
#define SDFB_Char_Culling		0x04
#define SDFB_Char_HasBBox		0x08

/* material flags */
#define SDFB_MAT_Ambient	0x01
#define SDFB_MAT_Diffuse	0x02
#define SDFB_MAT_Specular	0x04
#define SDFB_MAT_Emissive	0x08

/* Built-in class IDs */
#define Class_Scene			1
#define Class_Character		2
#define Class_Model			3
#define Class_Camera		4
#define Class_Light			5
#define Class_Surface		6
#define Class_Texture		7
#define Class_TexBlend		8
#define Class_Transform		9
	
typedef struct pDict 
{
    void	*data;
    char	*name;
	uint32	chunkid;
} pDict;

#ifdef ANIM

typedef struct AnimSequence 
{
	uint32 animLen;
	AmControl control;
	AmAnim    *animArray;
} AnimSequence;
#endif

/* structure to hold data from binary scene description file */
typedef struct BSDF
{
    RawFile	         *Stream;			/* open file stream */
    pDict	         *Dict;				/* dictionary */
    uint32	         FirstID;			/* first OBJID in bsdf file */
    uint32	         MaxID;				/* last OBJID in bsdf file */
	uint32           totalTriCount;		/* total triangle count in file */
	uint32           numPods;			/* number of geometry pods from a file */
	uint32           numAmodels;		/* number of AmModels in file */
	uint32           numTexPages;		/* number of texture pages in file */
	Pod              *pods;				/* geometry data read from a file */
#ifdef ANIM
	AnimSequence     animData;		    /* animation read from a file */
	AmModel          *amdls;			/* AmModels in a file */
#endif
	PodTexture       *textures;			/* PodTextures in a file */
	Material         *materials;		/* materials in a file */
} BSDF;

/* structures created for reader convenience */

/* Pod case types */
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

#define environFLAG				(1 << (31-21))

/* Pod chunk header */
typedef struct PodChunkHeader
{
	uint32	flags;
    uint16  caseType;       /* POD case type */
    uint16  texPageIndex;   /* POD texture page index */
    uint32  materialId;
    uint32  lightId;
    uint16  numSharedVerts;
    uint16  numVerts;
    uint32  numVertIndices;
    uint32  numTexCoords;
    uint32  numTriangles;

    float fxmin,fymin,fzmin;
    float fxextent,fyextent,fzextent;
} PodChunkHeader;

typedef struct MaterialChunk
{
	uint32 flags;
	Color4 diffuse;
	Color4 specular;
	Color4 emmissive;
	Color4 ambient;
	float  shine;
} MaterialChunk;

typedef struct surface
{
	uint32 num_pods;
	Pod	*ppods[1];
} surface;

typedef struct character
{
	uint32 type;
	Matrix mat;
} character;

typedef struct model
{
	character obj;
	surface *surf;
} model;

typedef struct AmNodeChunk
{
    unsigned int    type      : 1; 
    unsigned int    flags     : 15;
    unsigned int    n         : 16;
    Vector3D        pivot;
    uint32          mdl_id;
} AmNodeChunk;

/* structures to hold chunk information at read time */
typedef struct BSDFChunkHeader
{
    uint32           ID;
    uint32           size;
}   BSDFChunkHeader;

typedef struct BSDFChunkOffsets
{
    int32           start;
    int32           end;
}   BSDFChunkOffsets;

typedef struct BSDFChunk
{
    BSDFChunkHeader       header;
    BSDFChunkOffsets      offset;
}   BSDFChunk;

typedef void *(*AllocProcPtr)(int32, uint32);

/* function prototypes */
void 
ReadInTextureFile(
	PodTexture **ptext,
	uint32     *num_pages,
	char       *texFile,
	AllocProcPtr memcall
	);

#ifdef ANIM

void 
ReadInAnimFile(
	AmAnim     **anims,
	uint32     *num_anims,
	AmControl  *control,
	char       *animFile,
	AllocProcPtr memcall
	);

#endif

BSDF*
ReadInMercuryData(
	char       *sdfFile,
	char       *animFile,
	char       *texFile,
	AllocProcPtr memcall
	);
#endif

