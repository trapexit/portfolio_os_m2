/*
**	File:		bsdf_read.c	
**
**	Contains:	"Mercury" binary SDF data reader 
**              WARNING: This reader does not free the memory used for
**              temporary data gluing. It also copies the Pod structures
**              into articulated models. You can use originally allocated
**              Pod buffer ( instead of coping ) if the file contains a
**              single articulated model. For faster I/O it might be useful
**              to read the whole file into memory and parse the data.
**
**	Written by:	Ravindar Reddy
**
**	Copyright:	© 1996 by The 3DO Company. All rights reserved.
**	          	This material constitutes confidential and proprietary
**	          	information of the 3DO Company and shall not be used by
**	          	any Person or for any purpose except as expressly
**	          	authorized in writing by the 3DO Company.
**
**	Last Modified:	 96/05/10	version		1.40	
*/

#ifdef MACINTOSH
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kernel:mem.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kernel/mem.h>
#endif

#include "bsdf_read.h"
#ifdef ANIM
#include "AM_Model.h"
#include "AM_Anim.h"
#endif

/* 
** Let there be light !!
** Default lights.
*/

static LightDir LightDirData1 = 
{
	-0.866,	0.0,	-0.5,
	.9,	0.9,	0.7,
};
	
static LightDir LightDirData2 = 
{
	0.766,	-.1,	-0.4,
	0.3,	0.3,	0.25,
};
	
static uint32 podnolights[] =
{
    0,          
};              

static uint32 podlights[] = 
{
	(uint32)&M_LightDir,
	(uint32)&LightDirData1,
	(uint32)&M_LightDir,
	(uint32)&LightDirData2,
	0,
};

static uint32 podspeclights[] = 
{
	(uint32)&M_LightDirSpec,
	(uint32)&LightDirData1,
	(uint32)&M_LightDirSpec,
	/* (uint32)&M_LightDir,*/
	(uint32)&LightDirData2,
	0,
};

static uint32 podenvlights[] = 
{
	(uint32)&M_LightDirSpecTex,
	(uint32)&LightDirData1,
	(uint32)&M_LightEnv,
	0,
	0,
};

static uint32 podspectexlights[] = 
{
	(uint32)&M_LightDirSpecTex,
	(uint32)&LightDirData1,
	(uint32)&M_LightDirSpecTex,
	/*(uint32)&M_LightDir, */
	(uint32)&LightDirData2,
	0,
};


/*
** Array based dictionary functions 
** The dictionary is nothing but a array of void pointers to do a fast
** object ID look-up. Any data ( like temporary gluing structures ) in the
** dictionary can be freed at the end of file parsing.
*/
static bool 
BSDF_DictInit( 
	BSDF *sdf 
	)
{
    uint32 n, i;

    n = sdf->MaxID - sdf->FirstID + 1;
    if( n <= 0 ) 
	return FALSE;

    sdf->Dict = AllocMem( n*sizeof(pDict), MEMTYPE_NORMAL| MEMTYPE_FILL );
	for (i = 0; i < n; i++)
	{
		sdf->Dict[i].name = NULL;
		sdf->Dict[i].data = NULL;
	}

    return TRUE;
}

static bool 
BSDF_DictEnd( 
	BSDF *sdf 
	)
{
    uint32 n;

    n = sdf->MaxID - sdf->FirstID + 1;
    if( n <= 0 ) 
	return FALSE;

    FreeMem( sdf->Dict, n*sizeof(pDict) );
    return TRUE;
}

static void *
BSDF_DictLookUp( 
	BSDF *sdf, 
	uint32 objid 
	)
{
	void *data;
	
    if( objid < sdf->FirstID || objid > sdf->MaxID ) {
	printf("ERROR: Illegal objid in BSDF_DictLookUp\n");
	return NULL;
    }

	data  = sdf->Dict[objid-sdf->FirstID].data;

    return data;
}

static bool 
BSDF_DictInsert( 
	BSDF *sdf, 
	uint32 objid, 
	uint32 chunkid, 
	void *data 
	)
{
    if( objid < sdf->FirstID || objid > sdf->MaxID ) {
	printf("ERROR: Illegal objid in BSDF_DictInsert\n");
	return FALSE;
    }

    sdf->Dict[objid-sdf->FirstID].data = data;
    sdf->Dict[objid-sdf->FirstID].chunkid = chunkid;
    return TRUE;
}

/*
** functions to read the raw file
*/

static int32
BSDF_ReadData(
	BSDF *sdf,
	void    *outBuffer,
	int32	numBytes
	)
{
	int32 bytesRead;
	
	bytesRead = ReadRawFile(sdf->Stream, outBuffer, numBytes );
	if ( bytesRead == numBytes ) return bytesRead;
	else return 0;
}
	
/*
** WARNING : use BSDF_ReadData instead of this function
*/
static uint32 
BSDF_ReadWord( 
	BSDF *sdf 
	)
{
    int32 nbytes;
    uint32 val;
    int32 size = 4;
	
    nbytes = ReadRawFile(sdf->Stream, &val, size);
    if( nbytes == size ) return val;
    return 0;
}

/* set the marker from the beginning of the data */
static int32
BSDF_SetMarker(
	BSDF *sdf,
	int32 offSet
	)
{
	int32 seek_size;	
	
	seek_size = SeekRawFile( sdf->Stream, offSet, FILESEEK_START );

	return( seek_size );
}

static int32
BSDF_GetMarker(
	BSDF *sdf
	)
{
	int32 seek_size;

	/*
	** WARNING: File folio  GetRawFileInfo function is not giving
	** fi_BytesInFile so this work around
	*/
	seek_size = SeekRawFile( sdf->Stream, 0, FILESEEK_CURRENT );
	
	return( seek_size );
}

static void
BSDF_SkipChunk(
	BSDF *sdf,
	BSDFChunk *chunk
	)
{
	BSDF_SetMarker( sdf, chunk->offset.end );
}

static void
BSDF_ScanChunk(
	BSDF *sdf,
	BSDFChunk *chunk
	)
{
	/* Scan the chunk's header */
	BSDF_ReadData( sdf, &chunk->header.ID, sizeof( BSDFChunkHeader ) );

	chunk->offset.start = BSDF_GetMarker( sdf );
	chunk->offset.end = chunk->offset.start + chunk->header.size;
}

static int32 
BSDF_SkipBytes( 
	BSDF *sdf, 
	int32 numbytes 
	)
{
	int32 seek_size;

    seek_size = SeekRawFile( sdf->Stream, numbytes,  FILESEEK_CURRENT );

	return( seek_size );
}

static bool 
BSDF_ReadHeaderChunk( 
	BSDF *sdf,
	BSDFChunk *chunk,
	AllocProcPtr memcall
	)
{
	(void)&chunk;
	(void)&memcall;

	/* printf("ID_MHDR\n" ); */
    BSDF_SkipBytes( sdf, 4 );
    sdf->FirstID = BSDF_ReadWord( sdf );
    sdf->MaxID = BSDF_ReadWord( sdf );
	sdf->numAmodels = BSDF_ReadWord( sdf );
	sdf->numTexPages = BSDF_ReadWord( sdf );
	sdf->numPods = BSDF_ReadWord( sdf );
	sdf->totalTriCount = 0;
	sdf->materials = NULL;

#ifdef VERBOSE
	printf( "numAmodels = %d, numTexPages = %d, numPods = %d\n",
			sdf->numAmodels, sdf->numTexPages, sdf->numPods );
#endif

   	BSDF_DictInit( sdf );

    return TRUE;
}

static bool 
BSDF_ReadMaterialsChunk( 
	BSDF *sdf,
	BSDFChunk *chunk,
	AllocProcPtr memcall
	)
{
    uint32 i;
	uint32 len;
	MaterialChunk *mat_chunk;
	Material *mats;
	uint32 num_mats;

	/* WARNING : only one material array is allowed for each file */

    len = chunk->offset.end - chunk->offset.start - 4;
	num_mats = (uint32)len / sizeof( MaterialChunk );

	/* read the ID of the material chunk */
    BSDF_ReadWord( sdf );

	mat_chunk = (MaterialChunk *)AllocMem( len, MEMTYPE_NORMAL );
    /* read the material array data */
    BSDF_ReadData( sdf, mat_chunk, len );

    mats = ( Material* )(*memcall)( num_mats*sizeof( Material ) , MEMTYPE_NORMAL );

	for( i = 0; i < num_mats; i++ )
	{
		/* 
		** base color of the material is calculated as 	:
		** base = mat_ambient * light_ambient_factor + mat_emmissive
		** assume light_ambient_factor = 1.0 here
		** transparency is stored in diffuse alpha
		*/
		mats[i].base.r = mat_chunk[i].emmissive.r + mat_chunk[i].ambient.r;
		mats[i].base.g = mat_chunk[i].emmissive.g + mat_chunk[i].ambient.g;
		mats[i].base.b = mat_chunk[i].emmissive.b + mat_chunk[i].ambient.b;
		mats[i].base.a = mat_chunk[i].diffuse.a;

		/* clamp the color component values */
		if( mats[i].base.r > 1.0 ) mats[i].base.r = 1.0;
		if( mats[i].base.g > 1.0 ) mats[i].base.g = 1.0;
		if( mats[i].base.b > 1.0 ) mats[i].base.b = 1.0;
		
		memcpy( &mats[i].diffuse, &mat_chunk[i].diffuse, sizeof( Color3 ) );
		memcpy( &mats[i].specular, &mat_chunk[i].specular, sizeof( Color3 ) );
		mats[i].shine = mat_chunk[i].shine;
		mats[i].flags = 0;
	}

	/* assign the material array data */
	sdf->materials = mats;

	/* free the memory */
	FreeMem( mat_chunk, len );

    return TRUE;
}

static bool 
BSDF_ReadSurfChunk( 
	BSDF *sdf,
	BSDFChunk *chunk,
	AllocProcPtr memcall
	)
{
    uint32 id, pod_count;
    surface *s;

	(void)&chunk;

	/* printf( "ID_SURF\n" ); */
    id = BSDF_ReadWord( sdf );
    pod_count = BSDF_ReadWord( sdf );
	/* if there is no pod count, the default is 20 */
	if (pod_count == 0)
		pod_count = 20;

    if( BSDF_DictLookUp( sdf, id ) ) {
		printf("ERROR: BSDF_ReadSurfChunk: ID %d already exists\n",id);
		return FALSE;
    }
    s = ( surface * )(*memcall)( sizeof( surface ) + 4*(pod_count-1) , MEMTYPE_NORMAL );
	s->num_pods = 0;
    BSDF_DictInsert( sdf, id, ID_SURF, (void *)s );

    /* flags are not currently used */

    return TRUE;
}

static bool 
BSDF_ReadPodChunk( 
	BSDF *sdf, 
	BSDFChunk *chunk,
	Pod *pods ,
	AllocProcPtr memcall
	)
{
    int32 id;
	uint32 len, chunk_len;
	uint16 *utok16;
    uint32 *ph;
	surface *s;
	gfloat *flt;
	PodGeometry *pgeom;
	PodChunkHeader *hd;
	int32 slen;

	/* printf( "ID_SATM\n" ); */

    chunk_len = chunk->offset.end - chunk->offset.start - 8;
    BSDF_ReadWord( sdf );
    id = BSDF_ReadWord( sdf );

    if( (s = ( surface *)BSDF_DictLookUp( sdf, id )) == NULL ) {
		printf("ERROR: BSDF_ReadPodChunk: surface id %d not found\n",id);
		return FALSE;
    }

	ph = (uint32 *)AllocMem( chunk_len, MEMTYPE_NORMAL );
    if( ph == NULL ) {
        printf("ERROR: Couldn't allocate memory.\n");
        exit( 1 );
    }

    /* read in the pod chunk data */
    if( BSDF_ReadData( sdf, (char *)ph, chunk_len) != chunk_len ) {
		printf("BSDF_ReadPodChunk:  error reading Pod chunk data\n");
		return FALSE;
    }
	hd = ( PodChunkHeader *)ph;
	sdf->totalTriCount += hd->numTriangles;

	pgeom = (PodGeometry *)(*memcall)( sizeof(PodGeometry), MEMTYPE_NORMAL );
	if( pgeom == NULL ) {
		printf("ERROR: Couldn't allocate memory.\n");
		exit( 1 );
	}
	pgeom->vertexcount = hd->numVerts;
	pgeom->sharedcount = hd->numSharedVerts;
	pgeom->pshared = NULL;
	pgeom->fxmin = hd->fxmin; 
	pgeom->fymin = hd->fymin;
	pgeom->fzmin = hd->fzmin;
	pgeom->fxextent = hd->fxextent;
	pgeom->fyextent = hd->fyextent;
	pgeom->fzextent = hd->fzextent;

	flt = ( float *)( (char *)ph + sizeof ( PodChunkHeader ) );

	len = hd->numVerts * 6 * sizeof(float);
	pgeom->pvertex = (*memcall)( len, MEMTYPE_NORMAL );

	if( pgeom->pvertex == NULL ) {
		printf("ERROR: Couldn't allocate memory.\n");
		exit( 1 );
	}
	memcpy( pgeom->pvertex, flt, len );
	if (pgeom->sharedcount > 0) {
	  slen = hd->numSharedVerts * 2 * sizeof(uint16);
	  pgeom->pshared = (*memcall)( slen, MEMTYPE_NORMAL);
	  if (pgeom->pshared == NULL){
	    printf("ERROR: Couldn't allocate memeory.\n");
	    exit(1);
	  }
	  memcpy(pgeom->pshared, ((char *)flt + len), slen);
	  utok16 = (uint16 *) ((char *)flt + len + slen);
	}
	else
	  utok16 = (uint16 *) (flt + hd->numVerts*6);

	len = hd->numVertIndices*sizeof(uint16);
	pgeom->pindex = (*memcall)( len, MEMTYPE_NORMAL );
	if( pgeom->pindex == NULL ) {
		printf("ERROR: Couldn't allocate memory.\n");
		exit( 1 );
	}
	memcpy( pgeom->pindex, utok16, len );
	flt = (gfloat *) (utok16 + hd->numVertIndices);

	/* lit textured or just lit case */
	if (( hd->numTexCoords != 0 ) || (hd->flags & environFLAG))
	{
		if (hd->flags & environFLAG) 
			len = hd->numVertIndices * 2 * sizeof(float);
		else
			len = hd->numTexCoords*2*sizeof(gfloat);
		pgeom->puv = (*memcall)( len, MEMTYPE_NORMAL );
		if( pgeom->puv == NULL ) {
			printf("ERROR: Couldn't allocate memory.\n");
			exit( 1 );
		}
		if (!(hd->flags & environFLAG))
			memcpy( pgeom->puv, flt, len );
	} else pgeom->puv = NULL;
	
	/* set the Pod property data */
	pods->pgeometry = pgeom;
	pods->flags = hd->flags;

	/* based rendering case type set the ptexture data */
	pods->ptexture = NULL;
	pods->plights = podlights;

	pods->pmaterial = sdf->materials + hd->materialId;
	
	switch ( hd->caseType )
	{
        case PreLitCase:
            pods->pcase = &M_SetupPreLit;
            pods->plights = podnolights;
            break;
        case PreLitTexCase:
            pods->pcase = &M_SetupPreLitTex;
            pods->ptexture = sdf->textures + hd->texPageIndex;
            pods->plights = podnolights;
            break;
        case PreLitTransCase:
            pods->pcase = &M_SetupPreLitTrans;
            pods->plights = podnolights;
            break;
        case PreLitTransTexCase:
            pods->pcase = &M_SetupPreLitTransTex;
            pods->ptexture = sdf->textures + hd->texPageIndex;
            pods->plights = podnolights;
            break;
		case DynLitCase: 
			pods->pcase = &M_SetupDynLit;
			break;
		case DynLitTexCase : 
			pods->pcase = &M_SetupDynLitTex;
			pods->ptexture = sdf->textures + hd->texPageIndex;
			break;
		case DynLitTransCase : 
			pods->pcase = &M_SetupDynLitTrans;
			break;
		case DynLitTransTexCase : 
			pods->pcase = &M_SetupDynLitTransTex;
			pods->ptexture = sdf->textures + hd->texPageIndex;
			break;
		case DynLitSpecCase : 
			pods->pcase = &M_SetupDynLit;
			pods->plights = podspeclights;
			break;
		case DynLitSpecTexCase : 
			pods->pcase = &M_SetupDynLitSpecTex;
			pods->plights = podspectexlights;
			pods->ptexture = sdf->textures + hd->texPageIndex;
			break;
		case DynLitTransSpecCase:
			pods->pcase = &M_SetupDynLitTrans;
			pods->plights = podspeclights;
			break;
	        case DynLitEnvCase:
		        pods->pcase = &M_SetupDynLitEnv;
		        pods->plights = podenvlights;
			pods->ptexture = sdf->textures + hd->texPageIndex;
		        break;
	        case DynLitTransEnvCase:
		        pods->pcase = &M_SetupDynLitTransEnv;
		        pods->plights = podenvlights;
			pods->ptexture = sdf->textures + hd->texPageIndex;
		        break;
	        case DynLitSpecEnvCase:
		        pods->pcase = &M_SetupDynLitSpecEnv;
		        pods->plights = podenvlights;
			pods->ptexture = sdf->textures + hd->texPageIndex;
		        break;
		default :
			printf( "ERROR: unknown geometry case %d\n", hd->caseType );
			break;
	}

	/* free the the temp chunk data */
	FreeMem( ph, chunk_len  );

	s->ppods[s->num_pods] = pods;

	s->num_pods++;

    return TRUE;
}

static bool 
BSDF_ReadCharacterChunk( 
	BSDF *sdf,
	BSDFChunk *chunk,
	AllocProcPtr memcall
	)
{
	uint32 i, j;
    uint32 id, flags;
	character *m;

	(void)&chunk;

    id = BSDF_ReadWord( sdf );

	/* fprintf( stderr, "ID_CHAR\n" ); */

	m = ( character * )(*memcall)( sizeof( character ) , MEMTYPE_NORMAL );
	m->type = Class_Character;

	if( m == NULL ) {
		printf("ERROR: Couldn't allocate memory.\n");
		exit( 1 );
	}

    BSDF_DictInsert( sdf, id, ID_CHAR, (void *)m );

	BSDF_ReadWord( sdf );
    flags = BSDF_ReadWord( sdf );

    if( flags & SDFB_Char_HasBBox ) {
		float box[3][3];
		if( BSDF_ReadData( sdf, (char *)box, 24L ) != 24 ) {
	    	printf("BSDF:  error reading bbox data\n");
	    	return FALSE;
		}
	}
    if( flags & SDFB_Char_HasTransform ) {
		float mat[4][4];

		if( BSDF_ReadData( sdf, (char *)mat, 64L ) != 64 ) {
	    	printf("BSDF:  error reading transform data\n");
	    	return FALSE;
		}
		for( i= 0; i < 4; i++ )
			for( j= 0; j < 3; j++ )
				m->mat.mat[i][j] = mat[i][j];	
    }

    return TRUE;
}

static bool 
BSDF_ReadModelChunk( 
	BSDF *sdf,
	BSDFChunk *chunk,
	AllocProcPtr memcall
	)
{
	uint32 i, j;
    uint32 id, flags;
	model *m;
	surface *s;

	(void)&chunk;

    id = BSDF_ReadWord( sdf );

	m = ( model * )(*memcall)( sizeof( model ) , MEMTYPE_NORMAL );
	m->obj.type = Class_Model;

	if( m == NULL ) {
		printf("ERROR: Couldn't allocate memory.\n");
		exit( 1 );
	}

	m->surf = NULL;
    BSDF_DictInsert( sdf, id, ID_MODL, (void *)m );

    BSDF_ReadWord( sdf );
    flags = BSDF_ReadWord( sdf );

    if( flags & SDFB_Char_HasBBox ) {
		float box[3][3];
		if( BSDF_ReadData( sdf, (char *)box, 24L ) != 24 ) {
	    	printf("BSDF:  error reading bbox data\n");
	    	return FALSE;
		}
	}
    if( flags & SDFB_Char_HasTransform ) {
		float mat[4][4];

		if( BSDF_ReadData( sdf, (char *)mat, 64L ) != 64 ) {
	    	printf("BSDF:  error reading transform data\n");
	    	return FALSE;
		}
		for( i= 0; i < 4; i++ )
			for( j= 0; j < 3; j++ )
				m->obj.mat.mat[i][j] = mat[i][j];	
    }

    id = BSDF_ReadWord( sdf );
	if (id==0) 
	{
		m->surf = NULL;
		printf( "WARNING: Model with no geometry exist !\n" );
	} else {
    	if( (s = (surface *)BSDF_DictLookUp(sdf, id)) == NULL ) {
			printf("BSDF_ReadModelChunk: surface id %d not found\n",id);
			return FALSE;
		} else {
			m->surf = s;

			/* set the Pod property data */
			for( i = 0; i < m->surf->num_pods; i++ )
			{
				m->surf->ppods[i]->pmatrix = &m->obj.mat;
			}
		}
	}

	/* material array id */
    BSDF_ReadWord( sdf );

	/* texture array id - not used */
    BSDF_ReadWord( sdf );
	
    return TRUE;
}

#ifdef ANIM
static bool 
BSDF_ReadAnimChunk( 
	BSDF *sdf,
	BSDFChunk *chunk,
	AllocProcPtr memcall
	)
{
    uint32 id, len;
	uint32 anim_type;
	uint32 size[2];
	AmAnim	*m;

	(void)&chunk;

    id = BSDF_ReadWord( sdf );

	anim_type = BSDF_ReadWord( sdf );
	size[0] = BSDF_ReadWord( sdf );
	size[1] = BSDF_ReadWord( sdf );

	len = 4 + size[0] + size[1];
	m = ( AmAnim * )(*memcall)( sizeof( AmAnim ) , MEMTYPE_NORMAL );
	m->animData = (uint32 *)(*memcall)( len , MEMTYPE_NORMAL );

	if( ( m == NULL ) || ( m->animData == NULL ) ) {
		printf("ERROR: Couldn't allocate memory.\n");
		exit( 1 );
	}

    BSDF_DictInsert( sdf, id, ID_ANIM, (void *)m );

	/* read and set the animation data */
	m->animData[0] = anim_type;
	
	/* set the animation case routine here */	
	m->proutine = AM_EvalSpline;

	/* read the spline data */
	len = size[0] + size[1];
	if( BSDF_ReadData( sdf, &m->animData[1], len ) != len ) {
    	printf("BSDF:  error reading animation spline data\n");
    	return FALSE;
	}

    return TRUE;
}
#endif  /* ANIM */

#ifdef ANIM
static bool 
BSDF_ReadAmodelChunk( 
	BSDF *sdf,
	BSDFChunk *chunk,
	AmModel *m,
	AllocProcPtr memcall
	)
{
	uint32 i, j;
	uint32 chunk_len, chunk_id;
	uint32 hier_len;
	uint32	numNodes, treeDepth, numPods;
	uint32 	*ui32;
	AmNodeChunk *hier;
	AmAnim *anim;
	model	*mdl;
	character *chr;
	AmControl *cntrl;
	char	*data;
	uint32 *anims = NULL;

	/* fprintf( stderr, "ID_AMDL\n" ); */
    chunk_len = chunk->offset.end - chunk->offset.start;

	data = (char *)AllocMem( chunk_len, MEMTYPE_NORMAL );
	if( BSDF_ReadData( sdf, data, chunk_len ) != chunk_len ) 
	{
    	printf("ERROR: Can not read articulated model data\n");
    	return FALSE;
	}

	ui32 = ( uint32 *)data;
	
	/* HIER chunk data */
	chunk_id = ui32[0];
	hier_len = ui32[1] + 8;
	if ( chunk_id != ID_HIER ) 
	{
		printf("ERROR: Expecting HIER chunk, got %d\n", chunk_id );
		return FALSE;
	}

	numNodes = ui32[2];
	treeDepth = ui32[3];
	numPods = ui32[4];

	/* initialize control and other default data */
	m->numNodes = numNodes;
	m->numPods = numPods;
	m->textures = sdf->textures;
	m->pods = NULL;
	m->userData = NULL;
	m->next = NULL;

	hier = ( AmNodeChunk *)( (char *)(ui32 + 5) );
	ui32 = (uint32 *) ((char *)ui32 + hier_len);

	/* ANMA chunk data */
	/* ANMA chunk is an optional chunk */
	if ( chunk_len > hier_len )
	{
		chunk_id = ui32[0];
		if ( chunk_id != ID_ANMA ) {
			printf("BSDF:  expecting ANMA chunk, got %d\n", chunk_id );
			return FALSE;
		}
		cntrl = ( AmControl *)(ui32 + 2);
		memcpy( &m->control, cntrl, sizeof( AmControl ) );

		anims = (uint32 *)( (char *)( ui32 + 2 ) + sizeof( AmControl ) ) ;
	}

	/* 
	** allocate data for pod list, matrix buffer, flattened hierarchy 
	** and animation data
	*/
	m->matrices = ( Matrix *)(*memcall)( sizeof( Matrix ) * m->numNodes, 
	                                     MEMTYPE_NORMAL );
	m->hierarchy = ( AmNode *)(*memcall)( sizeof( AmNode ) * m->numNodes, 
	                                      MEMTYPE_NORMAL );
	m->pods = (Pod *)(*memcall)( sizeof(Pod) * m->numPods, MEMTYPE_NORMAL );

	/* KLUDGE : even if anim chunk exist use the data from anim file */
	if ( sdf->animData.animArray == NULL )
		m->animation = ( AmAnim *)(*memcall)( sizeof( AmAnim ) * m->numNodes, 
	                                      MEMTYPE_NORMAL );
	else {
		if( sdf->animData.animLen != m->numNodes )
		{
			printf( "ERROR : %d object animations from file ( expected %d )\n",
			         sdf->animData.animLen, m->numNodes );
			exit( 0 );
		}
		m->animation = sdf->animData.animArray;
		m->control = sdf->animData.control;
	}

#ifdef VERBOSE
	printf( "beginFrame = %f, endFrame = %f\n", 
		m->control.beginFrame, m->control.endFrame );
	printf( "Total Nodes in AmModel = %d, Tree Depth = %d\n", 
		numNodes, treeDepth );
#endif

	/* fill in the data for AmModel */
	numPods = 0;
	for( i = 0; i < m->numNodes; i++ )
	{
		/* assign hierarchy node data */
		m->hierarchy[i].type = hier[i].type;	
		m->hierarchy[i].flags = hier[i].flags;	
		m->hierarchy[i].n = hier[i].n;	
		m->hierarchy[i].pivot = hier[i].pivot;	

#if 0
		printf( "pivot x = %f, y = %f, z = %f\n",
				m->hierarchy[i].pivot.x,	
				m->hierarchy[i].pivot.y,	
				m->hierarchy[i].pivot.z );
#endif

		/* make sure all the model PODs point to AmModel matrix buffer */
		if( hier[i].mdl_id == 0 )
		{
			printf("ERROR: Model does not exist\n" );
			exit( 1 );
		}
		chr = ( character *)BSDF_DictLookUp( sdf, hier[i].mdl_id  );
		if( chr  == NULL ) 
		{
			printf("ERROR: Character %d not found\n", hier[i].mdl_id );
			return FALSE;
		}

		/* copy the original matrix */
		memcpy( m->matrices[i].mat, chr->mat.mat, sizeof( Matrix ) );

		/* 
		**	Make all model PODs point to the the transform. 
		**	For each model set the pod offset and number of pods
		**	in the pod buffer. This can be used to hide the models
		*/
		if( chr->type == Class_Model ) 
		{
			mdl = (model *)chr;

			m->hierarchy[i].flags = 0;

			m->hierarchy[i].podCount = mdl->surf->num_pods;
			m->hierarchy[i].podOffset = numPods;

			/* WARNING : free the memory allocated for data gluing */
			for( j = 0; j < mdl->surf->num_pods; j++ )
			{
				m->pods[numPods] = *(mdl->surf->ppods[j]);
				m->pods[numPods].pmatrix = ( Matrix *) &m->matrices[i];

				numPods++;
			}
		} else {
			m->hierarchy[i].flags = 0;
			m->hierarchy[i].podCount = 0;
			m->hierarchy[i].podOffset = 0;
		}

		/* assign animation node data */
		/* KLUDGE : even if anim chunk exist use the data from anim file */
		if ( sdf->animData.animArray == NULL )
		{
			if( ( anims != NULL ) && ( anims[i] != 0 ) )
			{
				if( ( anim = (AmAnim *)BSDF_DictLookUp( sdf, anims[i] )) == NULL ) 
				{
					printf("ERROR: Animation %d not found\n", anims[i] );
					return FALSE;
				}
				m->animation[i] = *anim;
			} else {
				m->animation[i].proutine = AM_EvalTransform;	
				m->animation[i].animData = (*memcall)( sizeof( Matrix ), 
				                                       MEMTYPE_NORMAL );
				memcpy( m->animation[i].animData, &chr->mat, sizeof( Matrix ) );
			}
		} else {
			if( m->animation[i].animData == NULL )
			{
				m->animation[i].proutine = AM_EvalTransform;	
				m->animation[i].animData = (*memcall)( sizeof( Matrix ), 
				                                       MEMTYPE_NORMAL );
				memcpy( m->animation[i].animData, &chr->mat, sizeof( Matrix ) );
			}
		}
	}

	/* free the memory allocated for reading the chunk data */
	FreeMem( data, chunk_len  );	

	return TRUE;
}
#endif  /* ANIM */

static int32 
BSDF_ParseData(
    BSDF *sdf,
	AllocProcPtr memcall
    )
{
	Pod    *pod_ptr = NULL;
	uint32	num_pods = 0;
#ifdef ANIM
	AmModel *amdl_ptr = NULL;
	uint32 num_amdls = 0;
#endif
	BSDFChunk headmaster, chunk;

	/* first get initial FORM */
	BSDF_ScanChunk( sdf, &headmaster );
	if( headmaster.header.ID != ID_FORM )
	{
		printf("ERROR: Not an IFF binary SDF file\n" );
		return( GFX_ErrorInternal );
	}
	BSDF_SkipBytes( sdf, 4 );
	
	/* keep reading the chunks embedded in this chunk */
	do 
	{
		BSDF_ScanChunk( sdf, &chunk );

		if ( chunk.offset.end <= headmaster.offset.end )
		{
			switch( chunk.header.ID ) 
			{
			case ID_MHDR:			/* header chunk */
				BSDF_ReadHeaderChunk( sdf, &chunk, memcall );

				/* allocate memory for reading Pod data */
				printf("Allocated %d pods\n", sdf->numPods);
				sdf->pods = (Pod *)(*memcall)(sdf->numPods*sizeof(Pod), 
				                              MEMTYPE_NORMAL ); 
				/* get the pointer to the current Pod data buffer */
				pod_ptr = sdf->pods;
#ifdef ANIM
	            sdf->amdls = (AmModel *)(*memcall)(sdf->numAmodels*sizeof(AmModel), 
				                                   MEMTYPE_NORMAL ); 
				amdl_ptr = sdf->amdls;
#endif
			    break;
			case ID_MATA:			/* material array chunk */
			  BSDF_ReadMaterialsChunk( sdf, &chunk, memcall);
			  break;
			case ID_PODG:			/* pod chunk */
			  BSDF_ReadPodChunk( sdf, &chunk, pod_ptr ,memcall);
			  /* Todd's Change */
			  num_pods++;
			  pod_ptr->puserdata = NULL;
			  pod_ptr->paadata = NULL;
			  /* fill the next net pod in the buffer */
			  if (num_pods < (sdf->numPods))
			    {
			      pod_ptr->pnext = pod_ptr + 1;
			      pod_ptr++;
			    }
			  else
			    {
			      printf("NULL\n");
			      pod_ptr->pnext = NULL;
			    }
			  break;
			case ID_SURF:			/* surface chunk */
			  BSDF_ReadSurfChunk( sdf, &chunk, memcall);
		    	break;
			case ID_MODL:			/* model chunk */
		   		BSDF_ReadModelChunk( sdf, &chunk, memcall);
		    	break;
			case ID_CHAR:			/* character chunk */
		   		BSDF_ReadCharacterChunk( sdf, &chunk, memcall);
		    	break;
#ifdef ANIM
			case ID_ANIM:			/* animation chunk */
		   		BSDF_ReadAnimChunk( sdf, &chunk, memcall);
		    	break;
			case ID_AMDL:			/* articulated model chunk */
		   		 BSDF_ReadAmodelChunk( sdf, &chunk, amdl_ptr, memcall);
	
				/* fill the next next  articulated model in the buffer */
				amdl_ptr->next = amdl_ptr + 1;
				amdl_ptr++; num_amdls++;
		    	break;
#endif
			default:
				break;
			}
		}
		BSDF_SkipChunk( sdf, &chunk );

    } while ( chunk.offset.end < headmaster.offset.end );

	/* clean up the dictionary array */
	BSDF_DictEnd( sdf );

    return GFX_OK;
}

#ifdef ANIM

void
BSDF_ReadAnimArrayChunk( 
	BSDF *sdf, 
	BSDFChunk *chunk,
	AmAnim **anima,
	uint32 *num_anims, 
	AmControl  *control,
	AllocProcPtr memcall
	)
{
	uint32 i, len;
	uint32 chunk_len;
	uint32 *anims, *ptr;
	AmAnim *anim;
	AmControl *cntrl;

    chunk_len = chunk->offset.end - chunk->offset.start;

	len = chunk_len - sizeof( AmControl );
	*num_anims = len / sizeof( uint32 );

	ptr = anims = (uint32 *)AllocMem( chunk_len, MEMTYPE_NORMAL );
	if( BSDF_ReadData( sdf, anims, chunk_len ) != chunk_len ) 
	{
    	printf("ERROR: Can not read animation data\n");
    	return;
	}

	/* copy the animation control data */
	cntrl = ( AmControl *)anims;
	memcpy( control, cntrl, sizeof( AmControl ) );

	anims = ( uint32 *)( ((char *)anims) + sizeof( AmControl ) );

	/* allocate animation array */
	len = *num_anims * sizeof( AmAnim );
	*anima = ( AmAnim *)(*memcall)( len, MEMTYPE_NORMAL );
	
	for( i = 0; i < (*num_anims); i++ )
	{
		if( anims[i] != 0 )
		{
			if( (  anim = (AmAnim *)BSDF_DictLookUp( sdf, anims[i] )) == NULL ) 
			{
				printf("ERROR: Animation %d not found\n", anims[i] );
				return;
			} else (*anima)[i] = *anim;
		} else {
			/* if there is no animation then copy the 
			** local transform from the model later 
			*/
			(*anima)[i].proutine = NULL;	
			(*anima)[i].animData = NULL;
		}
	}

	FreeMem( ptr, chunk_len );	
}

void 
ReadInAnimFile(
	AmAnim     **anims,
	uint32     *num_anims,
	AmControl  *control,
	char       *animFile,
	AllocProcPtr memcall
	)
{
	BSDF sdf;
	BSDFChunk headmaster, chunk;

	*num_anims = 0;
	*anims = NULL;

	if( animFile == NULL ) return;

	if ( OpenRawFile( &sdf.Stream, animFile, FILEOPEN_READ ) < 0 ) 
	{
		printf("ERROR: Couldn't open animation file.\n");
	}

	/* first get initial FORM */
	BSDF_ScanChunk( &sdf, &headmaster );
	if( headmaster.header.ID != ID_FORM )
	{
		printf("ERROR: Not an IFF binary SDF file\n" );
		return;
	}
	BSDF_SkipBytes( &sdf, 4 );
	
	/* keep reading the chunks embedded in this chunk */
	do 
	{
		BSDF_ScanChunk( &sdf, &chunk );

		if ( chunk.offset.end <= headmaster.offset.end )
		{
			switch( chunk.header.ID ) 
			{
			case ID_MHDR:			/* header chunk */
				BSDF_ReadHeaderChunk( &sdf, &chunk, memcall );
			    break;
			case ID_ANIM:			/* animation chunk */
		   		BSDF_ReadAnimChunk( &sdf, &chunk, memcall);
		    	break;
			case ID_ANMA:			/* animation array chunk */
		   		 BSDF_ReadAnimArrayChunk( &sdf, &chunk, 
				                          anims, num_anims, 
				                          control, memcall);
		    	break;
			default:
				printf( "default \n" );
				break;
			}
		}
		BSDF_SkipChunk( &sdf, &chunk );

    } while ( chunk.offset.end < headmaster.offset.end );

	/* clean up the dictionary array */
	BSDF_DictEnd( &sdf );

	CloseRawFile( sdf.Stream );
}

#endif

BSDF*
ReadInMercuryData(
    char *sdfFile,
    char *animFile,
    char *texFile,
	AllocProcPtr memcall
    )
{
	BSDF *sdf;

	printf( "Geometry File = %s\n", sdfFile );
	printf( "Texture File = %s\n", texFile );
	printf( "Animation File = %s\n", animFile );

	sdf = (*memcall)( sizeof(BSDF), MEMTYPE_NORMAL | MEMTYPE_FILL);
	if( sdf == NULL ) 
	{
		printf("ERROR: Couldn't allocate memory.\n");
		exit( 1 );
	}

   	if (OpenRawFile(&sdf->Stream,sdfFile,FILEOPEN_READ) < 0) 
	{
        printf("ERROR: Couldn't open Model file.\n");
        exit(1);
    }

	/* read the texture page file */
	ReadInTextureFile( &sdf->textures, 
	                   &sdf->numTexPages, 
	                   texFile, memcall );

#ifdef ANIM
	/* read in animation file */
	ReadInAnimFile( &(sdf->animData.animArray), 
	                &(sdf->animData.animLen),
	                &(sdf->animData.control),
	                animFile, memcall ); 
#endif

    /* now go ahead and parse the file */
	BSDF_ParseData( sdf, memcall );

    CloseRawFile( sdf->Stream );

	return sdf;
}

