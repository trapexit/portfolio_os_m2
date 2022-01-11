/*
 *
 *
 */
#include "shadows.h"

/*
 *
 */
Shadows* Shadows_Construct( void )
{
	Shadows		*self;
	Err			err;

	self = AllocMem( sizeof( Shadows ), MEMTYPE_NORMAL );
	assert( self );

	self->pod = NULL;
	self->shadow = NULL;
	Shadows_SetTexture( self, kDefaultShadowTex );

	return self;
}

/*
 *
 */
void Shadows_Destruct( Shadows *self )
{
	assert( self );

	FreeMem( self, sizeof( Shadows ) );
}

/*
 *
 */
void Shadows_SetTexture( Shadows *self, char *filename )
{
    strcpy( self->textureFilename, filename );
}

/*
 *
 */
void Shadows_Update( Shadows *self )
{
}

/*-----------------------------------------------------------------------------------------------
 *
 */
Pod* Shadows_CreatePod( Shadows *self )
{
	uint32			nVertices, nIndices, nUVs;
	uint32			i, j, k;
	uint32			x, z, ci;
	Pod				*pod;
	PodGeometry		*geo;
	PodTexture		*texture;

	Material		*material;
	Matrix			*matrix;
	float			*vertices;
	float			*uvs;
	short			*indices, index1, index2;
	uint32			*lights;
	uint32			numPages = 1;
	uint16			*aaedge;

	printf( "Shadows_CreatePod\n" );

	Shadows_DeletePod(self);

	nVertices = self->nShadows * 3;
	nIndices = self->nShadows * 4 + 2;
	nUVs = self->nShadows * 6;

	/*
	 * Must be an even number of vertices
	 */
	if ( nVertices&1 ) {
		nVertices++;
		vertices = (float*)AllocMem( nVertices*6*sizeof(float), MEMTYPE_ANY );
		assert( vertices );
		vertices[(nVertices-1)*6+0] = 0.0f;
		vertices[(nVertices-1)*6+1] = 0.0f;
		vertices[(nVertices-1)*6+2] = 0.0f;
		vertices[(nVertices-1)*6+3] = 0.0f;
		vertices[(nVertices-1)*6+4] = 0.0f;
		vertices[(nVertices-1)*6+5] = 0.0f;
	} else {
		vertices = (float*)AllocMem( nVertices*6*sizeof(float), MEMTYPE_ANY );
		assert( vertices );
	}

	i = 0;
	for ( j=0; j<(self->nShadows); j++ ) {
		for ( k=0; k<3; k++ ) {

			vertices[i+0] = 0.0f+j*10.0f;
			vertices[i+1] = 0+(k&2)*30.0f;
			vertices[i+2] = 0.0f;

			vertices[i+3] = 1.0f*(k&1);
			vertices[i+4] = 1.0f*(k&1);
			vertices[i+5] = 1.0f*(k&1);

			i += 6;
		}
	}

	uvs = (float*)AllocMem( nUVs*sizeof(float) , MEMTYPE_ANY );
	assert( uvs );

	indices = (short*)AllocMem( nIndices*sizeof(short), MEMTYPE_ANY );
	assert( indices );

	i = 0;	/* Pointer to new Index reference */
	j = 0;  /* Pointer to new Vertex reference */
	k = 0;  /* Pointer to new UV reference */


	for ( z=0; z<(self->nShadows); z++ ) {

		/*
		 *
		 */
		indices[i+0] = j + STXT + NEWS + CFAN + PCLK;
		indices[i+1] = 0;

		indices[i+2] = j+1;
		indices[i+3] = j+2;

		uvs[k+0] = 0.0f;
		uvs[k+1] = 1.0f;

		uvs[k+2] = 1.0f;
		uvs[k+3] = 1.0f;

		uvs[k+4] = 0.5f;
		uvs[k+5] = 0.0f;

		i += 4;
		j += 3;
		k += 6;
	}

	/*
	 * Flag end of indice list
	 */
	indices[i+0] = PCLK + NEWS + CFAN + STXT;
	indices[i+1] = -1;


	material = (Material*)AllocMem( sizeof(Material), MEMTYPE_ANY );
	assert( material );
	material->base.r = 0.0f;
	material->base.g = 0.0f;
	material->base.b = 0.0f;
	material->base.a = 0.4f;
	material->diffuse.r = 0.5f;
	material->diffuse.g = 0.5f;
	material->diffuse.b = 0.5f;
	material->shine = 0.4f;
	material->specular.r = 0.75f;
	material->specular.g = 0.75f;
	material->specular.b = 0.75f;

#if 1
	printf( "Shadows: Using %s texture.\n",self->textureFilename );
	ReadInTextureFile( &texture, &numPages, self->textureFilename, AllocMem );
#endif

	aaedge = (uint16*)AllocMem( sizeof(uint16)*1000, MEMTYPE_ANY );
	assert( aaedge );

	geo = (PodGeometry*)AllocMem( sizeof(PodGeometry), MEMTYPE_ANY );
	assert( geo );
	geo->fxmin = -10000.0f;
	geo->fymin = -10000.0f;
	geo->fzmin = -10000.0f;
	geo->fxextent = 20000.0f;
	geo->fyextent = 20000.0f;
	geo->fzextent = 20000.0f;
	geo->pvertex = vertices;
	geo->pshared = NULL;
	geo->vertexcount = nVertices;
	geo->sharedcount = 0;
	geo->pindex = indices;
	geo->puv = uvs;
	geo->paaedge = aaedge;

	matrix = Matrix_Construct();
	assert(matrix);

	lights = (uint32*)AllocMem( sizeof(uint32), MEMTYPE_ANY );
	assert(lights);
	*lights = 0;

	pod = (Pod*)AllocMem( sizeof(Pod), MEMTYPE_ANY );
	assert(pod);
	pod->flags = nocullFLAG;
	pod->pnext = NULL;
	pod->pcase = M_SetupPreLitTex;
	pod->ptexture = texture;
	pod->pgeometry = geo;
	pod->pmatrix = matrix;
	pod->plights = lights;
	pod->puserdata = NULL;
	pod->pmaterial = material;

	self->pod = pod;
	self->nVertices = nVertices;
	self->nIndices = nVertices;
	self->uvs = uvs;

	return pod;
}

/*
 *
 */
void Shadows_DeletePod( Shadows *self )
{
	if ( self->pod != NULL ) {
		printf("Shadows_DeletePod: --- Needs to be implemented ---\n");
	}
}

/*
 *
 */
void Shadows_UpdatePod( Shadows *self, Matrix *matrixCamera )
{
}

/* End of File */
