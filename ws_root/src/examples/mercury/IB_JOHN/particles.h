/*
 *
 *
 */
#ifndef _particles_h_
#define _particles_h_

#ifdef MACINTOSH
#include <kernel:types.h>
#include <kernel:mem.h>
#include <kernel:random.h>
#include <graphics:graphics.h>
#include <graphics:view.h>
#include <graphics:clt:gstate.h>
#include <misc:event.h>
#else
#include <kernel/types.h>
#include <kernel/mem.h>
#include <kernel/random.h>
#include <graphics/graphics.h>
#include <graphics/view.h>
#include <graphics/clt/gstate.h>
#include <misc/event.h>
#endif
#include <stdio.h>
#include <assert.h>
#include "mercury.h"
#include "matrix.h"

#include "misc.h"
#include "pod.h"
#include "material.h"

/*
 *
 */
typedef struct {

	uint32		type;

	Vector3D	position;
	Vector3D	previousPosition;
	Vector3D	velocity;
	float		age;

	void		*nextFree;
	void		*nextUse;

} Particle, *pParticle;

/*
 *
 */
typedef struct {

	uint32		nParticles;
	Particle	*particle;

	Particle	*firstFree;
	Particle	*firstUse;

	Vector3D	delta;

	Pod			*pod;

	float		*uvs;

	uint32		nVertices;
	uint32		nIndices;

	char		textureFilename[64];

} Particles, *pParticles;

/*
 * Constants
 */
#define kDefaultParticleTex "particle.utf"

enum { 	kParticle_Hide = 0,
		kParticle_Blood,
		kParticle_WeaponHit,
		kParticle_ForceField,
		kParticle_Last
	 };

/*
 *
 */
#define mParticles_GetPod(s) (s)->pod

/*
 *
 */
Particles* Particles_Construct( void );
void Particles_Destruct( Particles *self );

void Particles_SetCount( Particles *self, uint32 newCount );
void Particles_SetDeltaRange( Particles *self, float x, float y, float z );
void Particles_SetTexture( Particles *self, char *filename );

Particle* Particles_GetAFreeParticle( Particles *self );
void Particles_FreeAParticle( Particles *self, Particle *freeme );

void Particles_Update( Particles *self );
void Particles_Fire( Particles *self, uint32 type, float x, float y, float z );

Particle* Particles_CreateParticles( Particles *self );
void Particles_DeleteParticles( Particles *self );

Pod* Particles_CreatePod( Particles *self );
void Particles_DeletePod( Particles *self );
void Particles_UpdatePod( Particles *self, Matrix *matrixCamera );

#endif
/* End of File */
