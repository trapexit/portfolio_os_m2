/* @(#) scalemodel.c 96/06/11 1.3 */

#include <math.h>

#include <stdio.h>
#include "mercury.h"
#include "scalemodel.h"


/* This is the "better" way to scale an object.  Point lights and spotlights
 * will appear to move closer or further from an object when it's scale factor
 * is not 1.0.  So, we just scale the geometry instead.  It's only done once
 * at file load time, so speed isn't a big concern here.
 */
void Model_Scale(Pod* pod, float scale)
{
    uint32        i;
    PodGeometry*  pgeo;
    float*        pvtx;

    pgeo = pod->pgeometry;
    pvtx = pgeo->pvertex;
    for (i=0; i<pgeo->vertexcount; i++) {
	pvtx[i*6 + 0] *= scale;      /* X */
	pvtx[i*6 + 1] *= scale;      /* Y */
	pvtx[i*6 + 2] *= scale;      /* Z */
    }
    /* Now scale the bounding box */
    pgeo->fxmin *= scale;
    pgeo->fymin *= scale;
    pgeo->fzmin *= scale;
    pgeo->fxextent *= scale;
    pgeo->fyextent *= scale;
    pgeo->fzextent *= scale;
}


/* Transform a bounding box from a pod's local coords to world coords */
void BBox_LocalToWorld(const BBox* local, const Matrix* mtx, BBox* world)
{
    Vector3D fmax;

    world->fxmin = (local->fxmin * mtx->mat[0][0] +
		    local->fymin * mtx->mat[1][0] +
		    local->fzmin * mtx->mat[2][0] +
		    mtx->mat[3][0]);
    world->fymin = (local->fxmin * mtx->mat[0][1] +
		    local->fymin * mtx->mat[1][1] +
		    local->fzmin * mtx->mat[2][1] +
		    mtx->mat[3][1]);
    world->fzmin = (local->fxmin * mtx->mat[0][2] +
		    local->fymin * mtx->mat[1][2] +
		    local->fzmin * mtx->mat[2][2] +
		    mtx->mat[3][2]);
    fmax.x = ((local->fxmin + local->fxextent) * mtx->mat[0][0] +
	      (local->fymin + local->fyextent) * mtx->mat[1][0] +
	      (local->fzmin + local->fzextent) * mtx->mat[2][0] +
	      mtx->mat[3][0]);
    fmax.y = ((local->fxmin + local->fxextent) * mtx->mat[0][1] +
	      (local->fymin + local->fyextent) * mtx->mat[1][1] +
	      (local->fzmin + local->fzextent) * mtx->mat[2][1] +
	      mtx->mat[3][1]);
    fmax.z = ((local->fxmin + local->fxextent) * mtx->mat[0][2] +
	      (local->fymin + local->fyextent) * mtx->mat[1][2] +
	      (local->fzmin + local->fzextent) * mtx->mat[2][2] +
	      mtx->mat[3][2]);
    world->fxextent = fmax.x - world->fxmin;
    world->fyextent = fmax.y - world->fymin;
    world->fzextent = fmax.z - world->fzmin;
}


void Model_ComputeTotalBoundingBox(const Pod* firstPod, int32 podCount, BBox* bbox)
{
    BBox     curBBox;
    Vector3D curMax, bboxMax;
    Pod*     curPod;
    uint32   i;

    curPod = (Pod*)firstPod;
    BBox_LocalToWorld((BBox*)&(curPod->pgeometry->fxmin),
		      curPod->pmatrix, bbox);
    bboxMax.x = bbox->fxmin + bbox->fxextent;
    bboxMax.y = bbox->fymin + bbox->fyextent;
    bboxMax.z = bbox->fzmin + bbox->fzextent;

    for (i=1; i<podCount; i++) {
	curPod = curPod->pnext;
	BBox_LocalToWorld((BBox*)&(curPod->pgeometry->fxmin),
			  curPod->pmatrix, &curBBox);
	curMax.x = curBBox.fxmin + curBBox.fxextent;
	curMax.y = curBBox.fymin + curBBox.fyextent;
	curMax.z = curBBox.fzmin + curBBox.fzextent;

	if (curBBox.fxmin < bbox->fxmin)
	    bbox->fxmin = curBBox.fxmin;
	if (curMax.x > bboxMax.x)
	    bboxMax.x = curMax.x;
	if (curBBox.fymin < bbox->fymin)
	    bbox->fymin = curBBox.fymin;
	if (curMax.y > bboxMax.y)
	    bboxMax.y = curMax.y;
	if (curBBox.fzmin < bbox->fzmin)

	    bbox->fzmin = curBBox.fzmin;
	if (curMax.z > bboxMax.z)
	    bboxMax.z = curMax.z;
    }
    bbox->fxextent = bboxMax.x - bbox->fxmin;
    bbox->fyextent = bboxMax.y - bbox->fymin;
    bbox->fzextent = bboxMax.z - bbox->fzmin;
}

