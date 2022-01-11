/*
 *
 *
 */
#ifndef _shadows_h_
#define _shadows_h_

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

	Pod			*pod;

	float		*uvs;
	uint32		nVertices;
	uint32		nIndices;

	char		textureFilename[64];

} Shadows, *pShadows;

/*
 * Constants
 */
#define kDefaultShadowTex "shadow.utf"

/*
 *
 */
#define mShadows_GetPod(s) (s)->pod

/*
 *
 */
Shadows* Shadows_Construct( void );
void Shadows_Destruct( Shadows *self );

void Shadows_SetTexture( Shadows *self, char *filename );

void Shadows_Update( Shadows *self );

Pod* Shadows_CreatePod( Shadows *self );
void Shadows_DeletePod( Shadows *self );
void Shadows_UpdatePod( Shadows *self, Matrix *matrixCamera );

#endif
/* End of File */
