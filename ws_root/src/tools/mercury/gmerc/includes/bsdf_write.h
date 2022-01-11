#ifndef BSDF_Write_INC
#define BSDF_Write_INC 1

#include "kf_eng.h"

/* Binary SDF chunk IDs */

#define	ID_SDFB	MAKE_ID('S','D','F','B')	/* binary sdf file */
#define	ID_SHDR	MAKE_ID('S','D','F','H')	/* binary sdf file header */
#define	ID_OBJN	MAKE_ID('O','B','J','N')	/* object name */
#define	ID_SCEN MAKE_ID('S','C','E','N')	/* scene object */
#define	ID_CHAR	MAKE_ID('C','H','A','R')	/* character */
#define	ID_MODL	MAKE_ID('M','O','D','L')	/* model */
#define	ID_PCAM MAKE_ID('P','C','A','M')	/* perspective camera */
#define	ID_OCAM MAKE_ID('O','C','A','M')	/* orthographic camera */
#define	ID_DRLT MAKE_ID('D','R','L','T')	/* directional light */
#define	ID_PTLT MAKE_ID('P','T','L','T')	/* point light */
#define	ID_SPLT MAKE_ID('S','P','L','T')	/* spot light */
#define	ID_SSLT MAKE_ID('S','S','L','T')	/* soft spot light */
#define	ID_MATA	MAKE_ID('M','A','T','A')	/* material arrays */
#define	ID_TEXO	MAKE_ID('T','E','X','O')	/* texture object */
#define	ID_PIPO	MAKE_ID('P','I','P','O')	/* PIP tables */
#define	ID_M2LR	MAKE_ID('M','2','L','R')	/* Texture load rect */
#define	ID_TEXB	MAKE_ID('T','E','X','B')	/* texblend object */
#define	ID_TXBA	MAKE_ID('T','X','B','A')	/* texblend arrays */
#define	ID_SURF MAKE_ID('S','U','R','F')	/* surface object */
#define	ID_SAGE MAKE_ID('S','A','G','E')	/* primitive object */
#define	ID_SATM MAKE_ID('S','A','T','M')	/* trimesh object */
#define	ID_SAQM MAKE_ID('S','A','Q','M')	/* quadmesh object */
#define	ID_DARA	MAKE_ID('D','A','R','A')	/* fixed length array chunk */
#define	ID_TRFM	MAKE_ID('T','R','F','M')	/* transform chunk */
#define	ID_ENGN	MAKE_ID('E','N','G','N')	/* engine chunk */
#define	ID_LNKA	MAKE_ID('L','N','K','A')	/* link array chunk */

#define ID_PODT MAKE_ID('P','O','D','T')
#define ID_HIER MAKE_ID('H','I','E','R')
#define ID_AMDL	MAKE_ID('A','M','D','{')
#define ID_MDLA MAKE_ID('M','D','L','A')
#define ID_ANIM MAKE_ID('A','N','I','M')
#define	ID_ANMA	MAKE_ID('A','N','M','A')
#define	ID_PODG	MAKE_ID('P','O','D','G')
#define	ID_MHDR	MAKE_ID('M','H','D','R')
#define ID_GANM MAKE_ID('G','A','N','M')

/* these chunks are not in binary SDF but are used as object tags */
#if 0
#define	ID_CAMR MAKE_ID('C','A','M','R')	/* camera object */
#define	ID_LIGT MAKE_ID('L','I','G','T')	/* camera object */
#endif
/* Version numbers for SDF files */

#define BSDF_VERSION ((uint32)1)

/* Flag defines */
#define SDFB_Char_HasTransform 0x01
#define SDFB_Char_Invisible    0x02
#define SDFB_Char_Culling      0x04
#define SDFB_Char_HasBbox      0x08

#define SDFB_Scene_Visible     0x01
#define SDFB_Scene_AutoClip    0x02
#define SDFB_Scene_AutoAdjust  0x04
#define SDFB_Scene_Transparent 0x08

#define SDFB_Geo_UseColors     0x01
#define SDFB_Geo_UseNormals    0x02
#define SDFB_Geo_UseTexCoords  0x04
#define SDFB_Geo_HasColors     0x08
#define SDFB_Geo_HasNormals    0x10
#define SDFB_Geo_HasTexCoords  0x20

/* Global data needed for the parser */

#define MAX_ROOT_NODES 50
#define MAX_ANIM_NODES 100

struct  _SDFBObject
{
	uint32		compileOptions;
	uint32		hdrChunkOffset;	/* store header chunk offset to patch object count */
	uint32		objIdRange[2];	/* First ID, Max ID */
	uint32		userIdRange[2];	/* First ID, Max ID */
	IFFParser	*iff;			/* IFF chunk file currently being written out */
	
	/* information needed for writing animation data in mercury */
	uint32		numRootNodes;
	uint32		numAnimNodes;
	KfEngine	*animNodes[MAX_ANIM_NODES]; /* collect all the anim nodes */
};

typedef struct _SDFBObject SDFBObject;


/* function prototypes */
	
Err
ANIM_WriteChunk(
	SDFBObject* pData, 
	GfxObj* pObj
	);
	
#endif

