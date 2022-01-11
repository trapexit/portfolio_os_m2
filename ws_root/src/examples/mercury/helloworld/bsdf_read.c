/*
**	@(#) bsdf_read.c 96/09/30 1.14
**
**	Contains:	"Mercury" binary SDF data reader 
**              WARNING: This reader does not free the memory used for
**              temporary data gluing. It also copies the Pod structures
**              into articulated models. You can use originally allocated
**              Pod buffer ( instead of coping ) if the file contains a
**              single articulated model. For faster I/O it might be useful
**              to read the whole file into memory and parse the data.
**
**	Copyright:	© 1996 by The 3DO Company. All rights reserved.
**	          	This material constitutes confidential and proprietary
**	          	information of the 3DO Company and shall not be used by
**	          	any Person or for any purpose except as expressly
**	          	authorized in writing by the 3DO Company.
**
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

/* 
** Let there be light !!
** Default lights.
*/

static LightDir LightDirData1 = 
{
	-0.866,	0.0,	-0.5,
	1.0,	1.0,	1.0,
};
	
static LightDir LightDirData2 = 
{
	0.866,	0.0,	-0.5,
	1.0,	1.0,	1.0,
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
	/* (uint32)&M_LightDirSpec,
	(uint32)&M_LightDir,
	(uint32)&LightDirData2, */
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
	/* (uint32)&M_LightDirSpecTex,
	(uint32)&M_LightDir,
	(uint32)&LightDirData2, */
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
#if 0
    printf("Number of verts is %d, shared verts is %d\n", hd->numVerts, hd->numSharedVerts);
#endif
    pgeom->fxmin = hd->fxmin; 
    pgeom->fymin = hd->fymin;
    pgeom->fzmin = hd->fzmin;
    pgeom->fxextent = hd->fxextent;
    pgeom->fyextent = hd->fyextent;
    pgeom->fzextent = hd->fzextent;

    flt = ( float *)( (char *)ph + sizeof ( PodChunkHeader ) );

   	len = hd->numVerts*6*sizeof(gfloat);
    pgeom->pvertex = (*memcall)( len, MEMTYPE_NORMAL );

    if( pgeom->pvertex == NULL ) {
	printf("ERROR: Couldn't allocate memory.\n");
	exit( 1 );
    }
#if 0
    printf("len= %d\n", len);
#endif
    memcpy( pgeom->pvertex, flt, len );
    if (pgeom->sharedcount > 0)
	{
        slen = hd->numSharedVerts * 2 * sizeof(uint16);  
		pgeom->pshared = (*memcall)( slen, MEMTYPE_NORMAL );
    	if( pgeom->pshared == NULL ) {
			printf("ERROR: Couldn't allocate memory.\n");
			exit( 1 );
    	}
    	memcpy( pgeom->pshared, ((char *)flt + len), slen );
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
#if 0
    if (pgeom->sharedcount > 0)
    {
        for (i = 0; i < pgeom->vertexcount; i++)
            printf("i = %d, x = %f y = %f z = %f, nx = %f, ny = %f, nz = %f\n",
                i,
                *(pgeom->pvertex + i * 6),
                *(pgeom->pvertex + i * 6 + 1),
                *(pgeom->pvertex + i * 6 + 2),
                *(pgeom->pvertex + i * 6 + 3),
                *(pgeom->pvertex + i * 6 + 4),
                *(pgeom->pvertex + i * 6 + 5));
        for (i = 0; i < pgeom->sharedcount; i++)
            printf("i = %d, vert = %d, normal = %d\n", i,
            *(pgeom->pshared + 2 * i),
            *(pgeom->pshared + 2 * i+1));
        ui16 = (uint16*)pgeom->pindex;
   
        fprintf( stdout, "\tNumber of indices       %d\n", hd->numVertIndices );        for( i = 0; i < (hd->numVertIndices - 2); i++ )
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
    }
#endif
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

static int32 
BSDF_ParseData(
    BSDF *sdf,
	AllocProcPtr memcall
    )
{
	Pod    *pod_ptr = NULL;
	uint32	num_pods = 0;
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
				sdf->pods = (Pod *)(*memcall)(sdf->numPods*sizeof(Pod), 
				                              MEMTYPE_NORMAL ); 
				/* get the pointer to the current Pod data buffer */
				pod_ptr = sdf->pods;
			    break;
			case ID_MATA:			/* material array chunk */
		    	BSDF_ReadMaterialsChunk( sdf, &chunk, memcall);
		    	break;
			case ID_PODG:			/* pod chunk */
			    BSDF_ReadPodChunk( sdf, &chunk, pod_ptr ,memcall);
	
				/* fill the next net pod in the buffer */
				pod_ptr->pnext = pod_ptr + 1;
				pod_ptr++; num_pods++;
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


BSDF*
ReadInMercuryData(
    char *sdfFile,
    char *texFile,
	AllocProcPtr memcall
    )
{
	BSDF *sdf;

	printf( "Geometry File = %s\n", sdfFile );
	printf( "Texture File = %s\n", texFile );

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

    /* now go ahead and parse the file */
	BSDF_ParseData( sdf, memcall );

    CloseRawFile( sdf->Stream );

	return sdf;
}

