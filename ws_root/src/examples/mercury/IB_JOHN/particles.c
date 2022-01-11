/*
 *
 *
 */
#include "particles.h"

/*
 *
 */
Particles* Particles_Construct( void )
{
	Particles		*self;
	Err				err;

	self = AllocMem( sizeof( Particles ), MEMTYPE_NORMAL );
	assert( self );

	self->pod = NULL;
	self->particle = NULL;
	Particles_SetCount( self, 100 );
	Particles_SetDeltaRange( self, 1.0f, 1.0f, 1.0f );
	Particles_SetTexture( self, kDefaultParticleTex );

	self->firstUse = NULL;
	self->firstFree = NULL;

	return self;
}

/*
 *
 */
void Particles_Destruct( Particles *self )
{
	assert( self );

	FreeMem( self, sizeof( Particles ) );
}

/*
 *
 */
void Particles_SetCount( Particles *self, uint32 newCount )
{
	self->nParticles = newCount;
}

/*
 *
 */
void Particles_SetDeltaRange( Particles *self, float x, float y, float z )
{
	self->delta.x = x;
	self->delta.y = y;
	self->delta.z = z;
}

/*
 *
 */
void Particles_SetTexture( Particles *self, char *filename )
{
    strcpy( self->textureFilename, filename );
}


/*-----------------------------------------------------------------------------------------------
 *
 */
Particle* Particles_CreateParticles( Particles *self )
{
	uint32			i;
	Particle		*particle;

	printf( "Particles_CreateParticles\n" );

	Particles_DeleteParticles( self );

	particle = (Particle*)AllocMem( sizeof( Particle )* self->nParticles, MEMTYPE_NORMAL );
	assert( particle );

	self->particle = particle;

	self->firstFree = particle;
	self->firstUse = NULL;

	for ( i=0; i<self->nParticles; i++ ) {
		if ( i == self->nParticles-1 ) {
			particle[i].nextFree = NULL;
		} else {
			particle[i].nextFree = &particle[i+1];
		}
		particle[i].nextUse = NULL;
		particle[i].type = kParticle_Hide;
	}

	return self->particle;
}

/*
 *
 */
void Particles_DeleteParticles( Particles *self )
{
	if ( self->particle != NULL ) {
		printf("Particles_DeletePod: --- Needs to be implemented ---\n");
	}
}

/*
 *
 */
void Particles_UpdatePod( Particles *self, Matrix *matrixCamera )
{
	uint32			i;
	Particle		*p;
	float			*vertices;
	Vector3D		vector;
	Vector3D		topPosition;
	float			red, green, blue;
	float			size;

	/*
	 * Align particles approximately with the camera
	 */
	Vector3D_Set( &vector, 1.0f, 0.0f, 0.0f );
	Vector3D_OrientateByMatrix( &vector, matrixCamera );
	vector.x *= self->delta.x;
	vector.y *= self->delta.y;
	vector.z *= self->delta.z;

	/*
	 * Update the pod geometry
	 */
	vertices = self->pod->pgeometry->pvertex;

	/*
	 * Active particles
	 */
	p = self->firstUse;
	while ( p != NULL ) {

		switch ( p->type ) {
		case kParticle_Blood:
			red = 0.5f;
			green = 0.1f;
			blue = 0.05f;
			topPosition.x = p->previousPosition.x;
			topPosition.y = p->previousPosition.y;
			topPosition.z = p->previousPosition.z;
			break;
		case kParticle_WeaponHit:
			red = 1.0f;
			green = 0.9f;
			blue = 0.4f;
			topPosition.x = p->position.x - 3.0f * (p->position.x - p->previousPosition.x);
			topPosition.y = p->position.y - 3.0f * (p->position.y - p->previousPosition.y);
			topPosition.z = p->position.z - 3.0f * (p->position.z - p->previousPosition.z);
			break;
		case kParticle_ForceField:
			red = 1.0f;
			green = 1.0f;
			blue = 1.0f;
			topPosition.x = p->position.x - 5.0f * (p->position.x - p->previousPosition.x);
			topPosition.y = p->position.y - 5.0f * (p->position.y - p->previousPosition.y);
			topPosition.z = p->position.z - 5.0f * (p->position.z - p->previousPosition.z);
			break;
		default:
			red = 0.8f;
			green = 0.8f;
			blue = 0.3f;
			topPosition.x = p->previousPosition.x;
			topPosition.y = p->previousPosition.y;
			topPosition.z = p->previousPosition.z;
			break;
		}

		vertices[0] = p->position.x;
		vertices[1] = p->position.y;
		vertices[2] = p->position.z;

		vertices[3] = red;
		vertices[4] = green;
		vertices[5] = blue;

		vertices +=6;

		vertices[0] = p->position.x + vector.x;
		vertices[1] = p->position.y + vector.y;
		vertices[2] = p->position.z + vector.z;

		vertices[3] = red;
		vertices[4] = green;
		vertices[5] = blue;

		vertices +=6;

		vertices[0] = topPosition.x;
		vertices[1] = topPosition.y;
		vertices[2] = topPosition.z;

		vertices[3] = red;
		vertices[4] = green;
		vertices[5] = blue;

		vertices +=6;

		p = p->nextUse;
	}

	/*
	 * In-active particles
	 */
	p = self->firstFree;
	while ( p != NULL ) {

		switch ( p->type ) {
		case kParticle_Blood:
			red = 0.5f;
			green = 0.1f;
			blue = 0.05f;
			size = 1.5f;
			break;
		default:
			red = 0.3f;
			green = 0.3f;
			blue = 0.3f;
			size = 1.0f;
			break;
		}

		vertices[0] = p->position.x;
		vertices[1] = 1.0f;
		vertices[2] = p->position.z;

		vertices[3] = red;
		vertices[4] = green;
		vertices[5] = blue;

		vertices +=6;

		vertices[0] = p->position.x + size * self->delta.x;
		vertices[1] = 1.0f;
		vertices[2] = p->position.z;

		vertices[3] = red;
		vertices[4] = green;
		vertices[5] = blue;

		vertices +=6;

		vertices[0] = p->position.x;
		vertices[1] = 1.0f;
		vertices[2] = p->position.z + size * self->delta.z;

		vertices[3] = red;
		vertices[4] = green;
		vertices[5] = blue;

		vertices +=6;

		p = p->nextFree;
	}
}


/*
 *
 */
void Particles_Update( Particles *self )
{
	uint32			i;
	Particle		*p;

	/*
	 * Move the particles
	 */
	p = self->firstUse;
	while ( p != NULL ) {

		p->age--;
		if ( p->age < 0.0f ) {
			Particles_FreeAParticle( self, p );
		} else {

			p->previousPosition.x = p->position.x;
			p->previousPosition.y = p->position.y;
			p->previousPosition.z = p->position.z;

			p->velocity.y += -1.0f;

			p->position.x += p->velocity.x;
			p->position.y += p->velocity.y;
			p->position.z += p->velocity.z;

			if ( p->position.y < 1.0f ) {
				p->velocity.y = -0.50f*p->velocity.y;
				p->position.y = 1.0f;

				p->velocity.x = 0.75f*p->velocity.x;
				p->velocity.z = 0.75f*p->velocity.z;
			}
		}
		p = p->nextUse;
	}
}

/*
 *
 */
Particle* Particles_GetAFreeParticle( Particles *self )
{
	Particle	*p;

	/*
	 * Get next free particle
	 */
	p = self->firstFree;

	if ( p != NULL ) {
		/*
		 * Remove from free list
		 */
		self->firstFree = p->nextFree;
		/*
		 * Add to in use list
		 */
		p->nextUse = self->firstUse;
		self->firstUse = p;
	}

	return p;
}

/*
 *
 */
void Particles_FreeAParticle( Particles *self, Particle *freeme )
{
	Particle	*p;

	p = self->firstUse;

	/*
	 * Remove from in use list
	 */
	if ( p == freeme ) {
		self->firstUse = freeme->nextUse;
	} else {
		while ( freeme != p->nextUse ) {
			p = p->nextUse;
		}
		p->nextUse = freeme->nextUse;
	}

	/*
	 * Add to free list
	 */
	freeme->nextFree = self->firstFree;
	self->firstFree = freeme;
}


/*
 *
 */
void Particles_Fire( Particles *self, uint32 type, float x, float y, float z )
{
	uint32		i;
	Particle	*p;

	p = Particles_GetAFreeParticle( self );

	if ( p != NULL ) {

		p->type = type;

		switch ( type ) {

			case kParticle_Blood:

				p->position.x = x;
				p->position.y = y;
				p->position.z = z;

				p->velocity.x = mRandom( 2.0f ) - 1.0f;
				p->velocity.y = mRandom( 3.0f ) - 1.0f;
				p->velocity.z = mRandom( 2.0f ) - 1.0f;

				p->age = 10.0f;

				break;

			case kParticle_WeaponHit:

				p->position.x = x;
				p->position.y = y;
				p->position.z = z;

				p->velocity.x = mRandom( 6.0f ) - 3.0f;
				p->velocity.y = mRandom( 8.0f ) + 3.0f;
				p->velocity.z = mRandom( 6.0f ) - 3.0f;

				p->age = 100.0f;

				break;

			case kParticle_ForceField:

				p->position.x = x;
				p->position.y = y;
				p->position.z = z;

				p->velocity.x = mRandom( 18.0f ) - 9.0f;
				p->velocity.y = mRandom( 16.0f ) + 4.0f;
				p->velocity.z = mRandom( 18.0f ) - 9.0f;

				p->age = 100.0f;

				break;

			default :
				p->position.x = x;
				p->position.y = y;
				p->position.z = z;

				p->velocity.x = 0.0f;
				p->velocity.y = 5.0f;
				p->velocity.z = 0.0f;

				p->age = 60.0f;

				break;

		}
	}
}

/*-----------------------------------------------------------------------------------------------
 *
 */
Pod* Particles_CreatePod( Particles *self )
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

	printf( "Particles_CreatePod\n" );

	Particles_DeletePod(self);

	nVertices = self->nParticles * 3;
	nIndices = self->nParticles * 4 + 2;
	nUVs = self->nParticles * 6;

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
	for ( j=0; j<(self->nParticles); j++ ) {
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


	for ( z=0; z<(self->nParticles); z++ ) {

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
	printf( "Particles: Using %s texture.\n",self->textureFilename );
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
void Particles_DeletePod( Particles *self )
{
	if ( self->pod != NULL ) {
		printf("Particles_DeletePod: --- Needs to be implemented ---\n");
	}
}

/* End of File */
