/*
 * @(#) filepod.c 96/11/27 1.18
 *
 * Routines to load and manage a Pod read in from disk.
 *
 * Formatted for 8 space Tab stops
 */

#ifdef MACINTOSH
#include <kernel:types.h>
#include <kernel:mem.h>
#include <file:fileio.h>
#include <graphics:clt:clt.h>
#else
#include <kernel/types.h>
#include <kernel/mem.h>
#include <file/fileio.h>
#include <graphics/clt/clt.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mercury.h"
#include "bsdf_read.h"
#include "scalemodel.h"
#include "data.h"
#include "filepod.h"


/* Scale model to be this much of the size of the cube */
#define kRelativeSize    0.8


BSDF*               gBSDF = NULL;
static LightPoint   gPoint = {
    /* float x, y, z */                       -60.0, 60.0, 60.0,
    /* float maxdist */                       25000.0 * 256.0 * 0.9,
    /* float intensity */                     25000.0,
    /* Color3 lightColor */                   { 0.7, 0.7, 0.9  }
};
static LightDir     gDir = {
    /* float nx, ny, nz */                    0.3, -0.7, 0.648,
    /* Color3 lightcolor */                   0.2, 0.2, 0.2
};
uint32              gModelLightList[] = {
    (uint32)&M_LightDir,
    (uint32)&gDir,
    (uint32)&M_LightPoint,
    (uint32)&gPoint,
    0
};


Err Model_LoadFromDisk(char* filename, uint32* numPods, uint32* maxPodVerts)
{
    char    textureName[32], modelName[32];
    int32   i;
    BBox    bbox;
    Pod*    curPod;
    float   maxExtent, scaleFactor;

    strcpy(textureName, filename);
    strcat(textureName, ".utf");

    strcpy(modelName, filename);
    strcat(modelName, ".bsf");

    /* Now read in the texture(s) and pod(s) */

    gBSDF = ReadInMercuryData(modelName, textureName, AllocMem);
    if (gBSDF == NULL) {
	printf("err loading model!\n");
	*numPods = 0;
	return -1;
    } else {
	printf("Successfully read in %d Pods.\n", gBSDF->numPods);
	*numPods = gBSDF->numPods;
    }

    /* Compute the scale factor such that the largest dimension is 80% the
     * size of the box */

    Model_ComputeTotalBoundingBox(gBSDF->pods, gBSDF->numPods, &bbox);
    maxExtent = bbox.fxextent;
    if (bbox.fyextent > maxExtent)  maxExtent = bbox.fyextent;
    if (bbox.fzextent > maxExtent)  maxExtent = bbox.fzextent;
    if (maxExtent == 0.0) {
	printf("Empty bounding box.  Using default scale\n");
	scaleFactor = 0.0;
    } else {
	scaleFactor = 
	  (kRelativeSize * gCornerPod.pgeometry->fxextent) / maxExtent;
	printf("Scaling model to %g%% of its original size.\n",
	       scaleFactor*100.0);
    }

    /* Scale each pod in the model, and assign our light list to each pod.
     * Also figure out how many vertices we need to reserve space for in our
     * transform buffer */

    *maxPodVerts = 0;
    curPod = gBSDF->pods;
    for (i=0; i<gBSDF->numPods; i++) {
	if ((curPod->pgeometry->vertexcount + curPod->pgeometry->sharedcount) > *maxPodVerts)
	    *maxPodVerts = curPod->pgeometry->vertexcount + curPod->pgeometry->sharedcount;
	if (scaleFactor != 0.0)
	    Model_Scale(curPod, scaleFactor);
	/*	curPod->plights = gModelLightList; */
	curPod = curPod->pnext;
    }

    return 0;
}

