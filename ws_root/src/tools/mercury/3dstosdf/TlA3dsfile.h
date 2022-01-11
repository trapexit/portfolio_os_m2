/*
	File:		A3dsfile.h

	Contains:	 

	Written by:	Ravindar Reddy	 

	Copyright:	© 1994 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):


**		 <7>	  9/1/95	RRR		do not write texture coordinates when there         is no
**									texture reference
**		 <3>	 12/2/94	RRR		Fix 4-byte alignment for UNIX
	To Do:
*/

#pragma once

#include <iostream.h>
#include <stdlib.h>
#include <stdio.h>
#include "TlModel.h"
#include "TlLight.h"
#include "TlGroup.h"

#define NAME_SIZE_3DS 50 /* 17 */

// Useful macros

typedef union
{
	Int16			i;
	Byte			b[ 2 ];
}	B16;

typedef union
{
	Int32			i;
	Byte			b[ 4 ];
}	B32;

void inline SwapInt16( Int16 *swap )
{
	B16				spaw, *paws;
	
#ifndef INTEL
	spaw.i = *swap;
	paws = (B16 *)swap;
	
	paws->b[ 0 ] = spaw.b[ 1 ];
	paws->b[ 1 ] = spaw.b[ 0 ];
#endif
};

void inline SwapInt32( Int32 *swap )
{
	B32				spaw, *paws;
	
#ifndef INTEL
	spaw.i = *swap;
	paws = (B32 *)swap;
	
	paws->b[ 0 ] = spaw.b[ 3 ];
	paws->b[ 1 ] = spaw.b[ 2 ];
	paws->b[ 2 ] = spaw.b[ 1 ];
	paws->b[ 3 ] = spaw.b[ 0 ];
#endif

};

// Chunk and subchunk IDs

const Int16 chunkid_3DSHeader = 0x4D4D;
const Int16 chunkid_3DSProlog = 0x0002;
const Int16 chunkid_StartModel = 0x3D3D;

const Int16 chunkid_Material = 0xAFFF;
const Int16 chunkid_NamedObject = 0x4000;

const Int16 chunkid_VertexList = 0x4110;
const Int16 chunkid_FacetList = 0x4120;
const Int16 chunkid_MaterialList = 0x4130;
const Int16 chunkid_SmoothList = 0x4150;
const Int16	chunkid_4by3Matrix = 0x4160;

const Int16 chunkid_ColorFloat = 0x0010;
const Int16 chunkid_ColorByte = 0x0011;

const Int16 chunkid_MatName = 0xA000;
const Int16 chunkid_MatAmbient = 0xA010;
const Int16 chunkid_MatDiffuse = 0xA020;
const Int16 chunkid_MatSpecular = 0xA030;
const Int16 chunkid_MatShininess = 0xA040;
const Int16 chunkid_MatShinePct = 0xA041;
const Int16 chunkid_MatTransparency = 0xA050;
const Int16 chunkid_MatShading = 0xA100;
const Int16 chunkid_MatTexMap = 0xA200;
const Int16 chunkid_MatTex2Map = 0xA33A;
const Int16 chunkid_MatMapName = 0xA300;
const Int16 chunkid_IntPercentage = 0x0030;
const Int16 chunkid_MatTexTiling = 0xA351;
const Int16 chunkid_MatTwoSided = 0xA081;
const Int16 chunkid_MatDecal = 0xA082;
const Int16 chunkid_MatAdditive = 0xA083;


const Int16 chunkid_Trimesh = 0x4100;
const Int16 chunkid_TexVerts = 0x4140;
const Int16 chunkid_MeshTextureInfo = 0x4170;

const Int16 chunkid_Light = 0x4600;
const Int16 chunkid_LightOff = 0x4620;
const Int16 chunkid_SoftLight = 0x4610;

// Keyframe data
const Int16 chunkid_KFData = 0xB000;
const Int16 chunkid_KFHeader = 0xB00A;
const Int16 chunkid_KFNodeID = 0xB030;
const Int16 chunkid_KFNodeHdr = 0xB010;
const Int16 chunkid_KFObject = 0xB002;
const Int16 chunkid_KFInstName = 0xB011;
const Int16 chunkid_KFObjPivot = 0xB013;
const Int16 chunkid_KFObjBbox = 0xB014;

const Int16 chunkid_KFMorphTrack = 0xB026;
const Int16 chunkid_KFPosTrack = 0xB020;
const Int16 chunkid_KFRotTrack = 0xB021;
const Int16 chunkid_KFSclTrack = 0xB022;

// Data types to stuff keyframe data

typedef struct
{
	Int16	mUseFlag;		// tells which property is used
	float	mTension;		// relative tangent magnitude
	float	mBias;			// relative tangent direction 
	float	mContinuity;	// tangential continuiety at this key
	float	mEaseIn;		// relative approching velocity
	float	mEaseOut;		// relative leaving velocity
} SplineProps;

typedef struct KFPosKey
{
	Int32 frame;
	Point3D pnt;
} KFPosKey;

typedef struct KFRotKey
{
	Int32 frame;
	double rot[4];
} KFRotKey;

typedef struct KFMorphKey
{
	Int32 frame;
	char obj[NAME_SIZE_3DS];
} KFMorphKey;

typedef struct KFMorphTrack
{
	Int32 		nKeys;
	KFMorphKey	*morphObjs;
} KFMorphTrack;

typedef struct KFPosTrack
{
	Int32 nKeys;
	KFPosKey *pnts;
	SplineProps	*spl;	
} KFPosTrack;

typedef struct KFRotTrack
{
	Int32 nKeys;
	KFRotKey *rots;
	SplineProps	*spl;	
} KFRotTrack;

typedef struct KFSclTrack
{
	Int32 nKeys;
	KFPosKey *scls;
	SplineProps	*spl;	
} KFSclTrack;

typedef struct KFObject
{
	Int16 nodeID;
	Int16 parentID;
	char meshObject[NAME_SIZE_3DS];
	Point3D pivot;
	char *instName;
	KFMorphTrack *mt;
	KFPosTrack *pt;
	KFRotTrack *rt;
	KFSclTrack *st;
	// TlGroup character for this KFObject
	TlGroup *grp;
	
	struct KFObject *next;
} KFObject;
	
// General chunk types

typedef struct a3dChunkHeader
{
	Int16		 	padding; // to avoid the problem of
					 // 4-byte alignment 
	Int16		 	ID;
	Int32			size;
}	a3dChunkHeader;

typedef struct a3dOffsets
{
	Int32			start;
	Int32			end;
}	a3dOffsets;

typedef struct a3dChunk
{
	a3dChunkHeader	header;
	a3dOffsets		offset;
}	a3dChunk;

typedef struct a3dTexInfo
{
	Int16			type;
	float			tileX, tileY;
	Point3DS		position;
	float			scale;
	float			transform[3][4];
	float			planeHeight, planeWidth;
	float			cylHeight;
} a3dTexInfo;

// 0, 1, 2 are vertex indices and 3 is for flags
typedef struct a3dFacet
{	
	Int16 vtx[4];
} a3dFacet;

// TlVertex with POSITION
typedef struct VData1 {
	Int32 indx;
        Int32 dummy;
	Point3D pnt;
} Vdata1;

// TlVertex with POSITION & TEXCOORDS
typedef struct VData2 {
	Int32 indx;
        Int32 dummy;
	Point3D pnt;
	UVPoint uvp;
} Vdata2;

typedef struct a3dColorFloat
{
	float			r, g, b;
}	a3dColorFloat;

const Int32		a3dHeaderSize = 6; // Int16 + Int32

typedef char a3dmdlName[NAME_SIZE_3DS];

void SetFileName3DS( const char *nm );
const char *GetFileName3DS();

// 3D Studio Chunk class definition
class A3dsfile
{
  public:
	// Constructors and destructor
					A3dsfile( char *fileName );
	virtual			~A3dsfile( void );

	ErrCode			ReadModel( TlGroup *prnt );
	virtual void	ScanChunk( a3dChunk *chunk );
	virtual void	SkipChunk( a3dChunk *chunk );
	Int16			ScanInt16();
	Int32			ScanInt32();
	void			ScanFloat32( float *flt );
	Byte			ScanByte();
	void			ScanPoint( Point3D *pnt );
	void			ScanSplineKey( SplineProps *spl );
	ostream& 		PrintKFObjects( ostream& os );
	ostream& 		SDF_WriteKfData( ostream& os );
	ostream& 		PrintDefineAttributes( ostream& os, char *fileName, Boolean newFile,
	                               char *scrFile, int scrType );
	void			SetPrefix( const char * );
	void			SetTexRefOutput( Boolean );
	void			SetLightsOutput( Boolean );
	void			SetSmootGroupsOutput( Boolean );
	void			SetTexStitchOutput( Boolean );
	
  private:
	// Private member functions
	ErrCode			LoadModel( TlGroup *prnt );
	
	void			Parse3DS( a3dChunk *chunk, TlGroup *prnt );
	void			ParseModel( a3dChunk *chunk, TlGroup *prnt );
	void			ParseColor( TlColor *color );
	void			ParseTransparency( TlMaterial *mat );
	void			ParsePercentage( float *percent );
	
	// Add and lookup functions for TlMaterial and TlTexture info
	void			AddMaterial( TlMaterial *mat );
	void			AddTexture( TlTexture *mat );
	void			AddKFObject( KFObject *obj );

	TlMaterial*		GetMaterial( const char* name );
	TlTexture*		GetTexture( const char* name );
	KFObject*		GetKFObject( Int16 ID );
	
	// Read model geometry
	void			ParseNamedObject( a3dChunk *chunk, TlGroup *prnt );
	void			ParseTrimesh( a3dChunk *chunk, TlGroup *prnt, const char *name );
	void			ParseLight( a3dChunk *chunk, TlGroup *prnt, const char *name );
	void 			ParseSpotLight( a3dChunk *headmaster, TlLight *lgt );
	void			ParseMeshMatrix( TlTransform *tf );
	void			ParseFacetList( a3dChunk *chunk, TlSurface *surf );
	void			ParseVertexList( a3dChunk *chunk, TlSurface *surf );
	void			ParseTextureVertices( a3dChunk *chunk, TlSurface *surf );

	void			ParseMeshTextureInfo( a3dChunk *chunk, a3dTexInfo *tex_info );
	void			ParseTextureMap( a3dChunk *chunk, const char* name );
	void			ParseMaterialEntry( a3dChunk *chunk );
	void			ParseFaceMaterials( TlSurface *surf, TlSurface **surfs );
	void			ParseSmoothingGroups( TlSurface *surf, TlSurface **surfs, Int16 nThings );
	void			ParseMaterialList( a3dChunk *chunk, TlSurface *surf, 
	                                   TlSurface **surfs, Int16 nThings );
	const char			*ScanString();
	
	// Parse Keyframe data
	void 			ParseKFData( a3dChunk *chunk );
	void 			ParseKFObject( a3dChunk *chunk );
	void 			ParseKFNodeHdr( a3dChunk *chunk, KFObject *obj );
	void 			ParseKFMorphData( a3dChunk *chunk, KFMorphTrack **mt );
	void 			ParseKFPosTrack( a3dChunk *chunk, KFPosTrack **pt);
	void 			ParseKFRotTrack( a3dChunk *chunk, KFRotTrack **rt );
	void 			ParseKFSclTrack( a3dChunk *chunk, KFSclTrack **st );
	void			BuildKFObjHierarchy( TlGroup *parent );
	
	// ifstream functions
	Int32			ReadData( void	*outBuffer, Int32	numBytes );
	void			SetMarker( Int32 offSet, int fromWhere);
	Int32			GetMarker();	

	char*			AddModelName( const char *mname );
	char*			GetModelName( char *mname );
	void			SetTlSurfaceWrapMode( TlSurface *surf );
	void 			DeleteTextureVertices( TlSurface *surf );
			  
  private:				
	// private data

	FILE *fp;
	// TlMaterial and TlTexture lists in this file. These will be 
	// read into and later shared by Named Objects. The names
	// these attributes are unique
	TlMaterial *materials;
	TlTexture *textures;
	// KeyFrame objects data temporarily stored here
	KFObject *kfObjs;
	
	a3dmdlName *mdlNames;
	unsigned int mdlNum;
	unsigned int maxMdlNum;
	
	// prefix to be attached to all object names
	char		*prefix;
	
	// flags to enable and disable certain features in the 3DS reader
	Boolean		writeTexRefs;	
	Boolean		writeLights;	
	Boolean		writeSmoothingGroups;	
	Boolean		writeTextureStitiching;	
};
