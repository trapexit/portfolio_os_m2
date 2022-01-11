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

static float gBillXVertices[] = {
    /* Format: x, y, z, r, g, b */
    100.0, 0.0, -50.0, 1.0, 1.0, 1.0, /* 0th Index*/
    100.0, 0.0,  50.0, 1.0, 0.0, 0.0, /* 1 */
      0.0, 0.0, -50.0, 1.0, 0.0, 0.0, /* 2 */
      0.0, 0.0,  50.0, 0.0, 0.0, 0.0, /* 3 */
};

static float gBillYVertices[] = {
    /* Format: x, y, z, r, g, b */
     -50.0, 100.0, 0.0, 1.0, 1.0, 1.0, /* 0th Index*/
      50.0, 100.0, 0.0, 0.0, 1.0, 0.0, /* 1 */
     -50.0,   0.0, 0.0, 0.0, 1.0, 0.0, /* 2 */
      50.0,   0.0, 0.0, 0.0, 0.0, 0.0, /* 3 */
};

static float gBillZVertices[] = {
    /* Format: x, y, z, r, g, b */
    0.0, -50.0, 100.0, 1.0, 1.0, 1.0, /* 0th Index*/
    0.0,  50.0, 100.0, 0.0, 0.0, 1.0, /* 1 */
    0.0, -50.0,   0.0, 0.0, 0.0, 1.0, /* 2 */
    0.0,  50.0,   0.0, 0.0, 0.0, 0.0, /* 3 */
};

static short gBillboardIndices[] = {
    0,
    1,
    2,
    3,
    /* Terminate the list */
    0 + PCLK + NEWS + CFAN + STXT, -1
};


static PodGeometry gBillXGeometry = {
    /* float fxmin, fymin, fzmin */           0.0, -50.0, 0.0,
    /* float fxextent, fyextent, fzextent */  0.0, 100.0, 100.0,
    /* float *pvertex */                      gBillXVertices,
    /* short *pshared */		      NULL,
    /* uint16 vertexcount */                  sizeof(gBillXVertices)/
					      kSizePerVertex,
    /* uint16 sharedcount */                  0,
    /* short *pinxex */                       gBillboardIndices,
    /* float *puv */                          NULL
};

static PodGeometry gBillYGeometry = {
    /* float fxmin, fymin, fzmin */             0.0, 0.0, -50.0,
    /* float fxextent, fyextent, fzextent */  100.0, 0.0, 100.0,
    /* float *pvertex */                      gBillYVertices,
    /* short *pshared */		      NULL,
    /* uint16 vertexcount */                  sizeof(gBillYVertices)/
					      kSizePerVertex,
    /* uint16 sharedcount */                  0,
    /* short *pinxex */                       gBillboardIndices,
    /* float *puv */                          NULL
};

static PodGeometry gBillZGeometry = {
    /* float fxmin, fymin, fzmin */           -50.0,   0.0, 0.0,
    /* float fxextent, fyextent, fzextent */  100.0, 100.0, 0.0,
    /* float *pvertex */                      gBillZVertices,
    /* short *pshared */		      NULL,
    /* uint16 vertexcount */                  sizeof(gBillZVertices)/
					      kSizePerVertex,
    /* uint16 sharedcount */                  0,
    /* short *pinxex */                       gBillboardIndices,
    /* float *puv */                          NULL
};


static uint32 gBillboardLightList[] = {
    0
};

Material gBillboardMaterial = {
    /* base color */                          { 0.0, 0.0, 0.0, 1.0 },
    /* diffuse color */                       { 0.0, 0.0, 0.0 },
    /* shine */                               0.0,
    /* specular color */                      { 1.0, 1.0, 1.0 }
};

static Matrix gBillXMatrix = {
    1.0, 0.0, 0.0,
    0.0, 1.0, 0.0,
    0.0, 0.0, 1.0,
    50.0, 0.0, 0.0
};

static Matrix gBillYMatrix = {
    1.0, 0.0, 0.0,
    0.0, 1.0, 0.0,
    0.0, 0.0, 1.0,
    0.0, 50.0, 0.0
};

static Matrix gBillZMatrix = {
    1.0, 0.0, 0.0,
    0.0, 1.0, 0.0,
    0.0, 0.0, 1.0,
    0.0, 0.0, 50.0
};

Pod gBillZPod = {
    /* uint32 flags */                        nocullFLAG,
    /* struct Pod *pnext */                   NULL,
    /* void (*pcase)(CloseData*) */           M_SetupPreLit,
    /* struct PodTexture *ptexture */         NULL,
    /* struct PodGeometry *pgeometry */       &gBillZGeometry,
    /* Matrix *pmatrix */                     &gBillZMatrix,
    /* uint32 *plights */                     gBillboardLightList,
    /* uint32 *puserdata */                   NULL,
    /* Material *pmaterial */                 &gBillboardMaterial
};

Pod gBillYPod = {
    /* uint32 flags */                        nocullFLAG,
    /* struct Pod *pnext */                   &gBillZPod,
    /* void (*pcase)(CloseData*) */           M_SetupPreLit,
    /* struct PodTexture *ptexture */         NULL,
    /* struct PodGeometry *pgeometry */       &gBillYGeometry,
    /* Matrix *pmatrix */                     &gBillYMatrix,
    /* uint32 *plights */                     gBillboardLightList,
    /* uint32 *puserdata */                   NULL,
    /* Material *pmaterial */                 &gBillboardMaterial
};

Pod gBillXPod = {
    /* uint32 flags */                        nocullFLAG,
    /* struct Pod *pnext */                   &gBillYPod,
    /* void (*pcase)(CloseData*) */           M_SetupPreLit,
    /* struct PodTexture *ptexture */         NULL,
    /* struct PodGeometry *pgeometry */       &gBillXGeometry,
    /* Matrix *pmatrix */                     &gBillXMatrix,
    /* uint32 *plights */                     gBillboardLightList,
    /* uint32 *puserdata */                   NULL,
    /* Material *pmaterial */                 &gBillboardMaterial
};


