/*
 *
 *
 */
#ifndef _POD_H_
#define _POD_H_

#include "mercury.h"
#include "misc.h"
#include "material.h"
#include "lights.h"

/*
 * Macros
 */
#define mPod_SetNext(s,n) (s)->pnext = (n)
#define mPod_GetNext(s) (s)->pnext

#define mPod_SetCase(s,c) (s)->pcase = (c)
#define mPod_GetCase(s) (s)->pcase

#define mPod_SetMaterial(s,m) (s)->pmaterial = (m)
#define mPod_GetMaterial(s) (s)->pmaterial

#define mPod_SetIndices(s,m) (s)->pgeometry->pindex = (m)
#define mPod_GetIndices(s) (s)->pgeometry->pindex

/*
 * Prototypes
 */
void Pod_SetFlags( Pod *pod, uint32 nPods, uint32 flags );
void Pod_ClearFlags( Pod *pod, uint32 nPods, uint32 flags );

void Pod_SetLight( Pod *pod, uint32 nPods, uint32* lights );
void Pod_SetCase( Pod *pod, uint32 nPods, void* pcase );

void Pod_ReplaceSetup( Pod *self, uint32 nPods, void *replace, void *with );

void Pod_CalcBoundingBox( Pod *pod, uint32 nPods, BBox *bbox );
void Pod_Scale( Pod* pod, uint32 nPods, float scale );

void Pod_Print(Pod *pod);


#endif
/* End of File */
