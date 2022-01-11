/*
 * @(#) data.c 96/05/09 1.8
 *
 * This data shows how to create 3 intersecting perpendicular walls (such as
 * in a corner of a room & a floor) with data created at runtime.  For this
 * example, the corner is pre-lit, so location and color data are supplied.
 * The object is faceted so that it looks somewhat 3D.
 *
 * This object looks like this:
 *
 *              3             2
 *             +-------------+
 *            /|             |
 *           / |             |
 *          /  |             |
 *       4 /   |             |
 *        +    |             |
 *        |    |0            |1
 *        |    +-------------+
 *        |   /             /
 *        |  /             /
 *        | /             /
 *       5|/           6 /
 *        +-------------+
 */

#ifdef MACINTOSH
#include <kernel:types.h>
#include <graphics:clt:clt.h>
#else
#include <kernel/types.h>
#include <graphics/clt/clt.h>
#endif
#include "mercury.h"


#define kFloatsPerVertex       6
#define kSizePerVertex         (kFloatsPerVertex*sizeof(float))


float gHitherPlane = 10.0;


static float gCornerVertices[] = {
    /* Format: x, y, z, r, g, b */
    -100.0, -100.0, -100.0, 0.8, 0.8, 0.8, /* 0th Index*/
     100.0, -100.0, -100.0, 0.8, 0.8, 0.8, /* 1 */
     100.0,  100.0, -100.0, 0.6, 0.6, 0.6, /* 2 */
    -100.0,  100.0, -100.0, 0.6, 0.6, 0.6, /* 3 */
    -100.0,  100.0,  100.0, 0.3, 0.3, 0.3, /* 4 */
    -100.0, -100.0,  100.0, 0.8, 0.8, 0.8, /* 5 */
     100.0, -100.0,  100.0, 0.8, 0.8, 0.8, /* 6 */
       0.0,    0.0,    0.0, 0.0, 0.0, 0.0  /* Dummy vertex.  Must be even # */
};


static short gCornerSharedVerts[] = {
    /* Format: Index in vertex table (above) to share X,Y,Z, followed by an
               Index in vertex table (above) to share r,g,b or nx,ny,nz.  These
               vertices can then be used in the index table, below */
    0, 2,                                  /* 8th Index (cont. from above) */
    0, 4,                                  /* 9 */
    1, 2,                                  /* 10 */
    3, 4,                                  /* 11 */
    5, 4,                                  /* 12 */
};


static short gCornerIndices[] = {
    /* Back face of faceted cube cutaway */
    10,
    2,
    8,
    3,
    /* Left face of faceted cube cutaway */
    9 + NEWS + CFAN + PCLK,
    11,
    12,
    4,
    /* Botom face of faceted cube cutaway */
    0 + NEWS + CFAN + PCLK,
    5,
    1,
    6,
    /* Terminate the list */
    0 + PCLK + NEWS + CFAN + STXT, -1
};


static PodGeometry gCornerGeometry = {
    /* float fxmin, fymin, fzmin */           -100.0, -100.0, -100.0,
    /* float fxextent, fyextent, fzextent */  200.0, 200.0, 200.0,
    /* float *pvertex */                      gCornerVertices,
    /* short *pshared */		      gCornerSharedVerts,
    /* uint16 vertexcount */                  sizeof(gCornerVertices)/
					      kSizePerVertex,
    /* uint16 sharedcount */                  sizeof(gCornerSharedVerts)/
					      (2 * sizeof(short)),
    /* short *pinxex */                       gCornerIndices,
    /* float *puv */                          NULL
};


static uint32 gCornerLightList[] = {
    0
};

Material gCornerMaterial = {
    /* base color */                          { 0.5, 0.5, 0.5, 1.0 },
    /* diffuse color */                       { 0.5, 0.5, 0.5 },
    /* shine */                               0.0,
    /* specular color */                      { 1.0, 1.0, 1.0 }
};

static Matrix gCornerMatrix = {
    1.0, 0.0, 0.0,
    0.0, 1.0, 0.0,
    0.0, 0.0, 1.0,
    0.0, 0.0, 0.0
};

Pod gCornerPod = {
    /* uint32 flags */                        0,
    /* struct Pod *pnext */                   NULL,
    /* void (*pcase)(CloseData*) */           M_SetupPreLit,
    /* struct PodTexture *ptexture */         NULL,
    /* struct PodGeometry *pgeometry */       &gCornerGeometry,
    /* Matrix *pmatrix */                     &gCornerMatrix,
    /* uint32 *plights */                     gCornerLightList,
    /* uint32 *puserdata */                   NULL,
    /* Material *pmaterial */                 &gCornerMaterial
};
