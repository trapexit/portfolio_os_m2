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
#include "filepod.h"

BSDF*               gBSDF = NULL;
BSDF*               gBSDF2 = NULL;
BSDF*               gBSDF3 = NULL;
BSDF*               gBSDF4 = NULL;
BSDF*               gBSDF5 = NULL;

#define NUM_DUST_VERTS 4

#ifdef RENDER_SHADOW
BSDF  *shadow0BSDF;
BSDF  *shadow1BSDF;

#ifdef SINGLE_SHADOW
Pod    shadowPods[SHADOW_SEG];
struct PodTexture  shadowPTextures[SHADOW_SEG];
static PodGeometry shadowPodsGeo[SHADOW_SEG];   /* Pod Geometry */

float              shadowVertex[SHADOW_SEG*NUM_DUST_VERTS*6]; 
static float       shadowUV[SHADOW_SEG*(NUM_DUST_VERTS-2)*6];
#else
Pod    shadowPods[ALLOC_SHADOW_SEG];
struct PodTexture  shadowPTextures[ALLOC_SHADOW_SEG];
static PodGeometry shadowPodsGeo[ALLOC_SHADOW_SEG];   /* Pod Geometry */

float              shadowVertex[ALLOC_SHADOW_SEG*NUM_DUST_VERTS*6]; 
static float       shadowUV[ALLOC_SHADOW_SEG*(NUM_DUST_VERTS-2)*6];
#endif




#endif

Pod                 dustPods[MAX_DUST];      /* Dust */
static PodGeometry         dustPodsGeo[MAX_DUST];   /* Pod Geometry */
static float               dustVertex[MAX_DUST*NUM_DUST_VERTS*6];
static float               dustUV[MAX_DUST*(NUM_DUST_VERTS-2)*6];

Pod                        poofPods[MAX_POOFS];      /* Effect Poofs */
static PodGeometry         poofPodsGeo[MAX_POOFS];   /* Pod Geometry */
static float               poofVertex[MAX_POOFS*NUM_DUST_VERTS*6];
static float               poofUV[MAX_POOFS*(NUM_DUST_VERTS-2)*6];

static LightPoint   gPoint = {
  /* float x, y, z */ 
  -60.0, 60.0, 60.0,
  /* float maxdist */
  /*  25000.0 * 256.0 * 0.9, */
  25000.0 * 1024.0 * 0.9,
  /* float intensity */
  /*  25000.0, */
  250000.0,
  /* Color3 lightColor */
  { 0.7, 0.7, 0.9  }
};
static LightDir     gDir = {
  /* float nx, ny, nz */                    0.3, -0.7, 0.648,
					    /* Color3 lightcolor */                   0.2, 0.2, 0.2
};

static uint32              gModelLightList[] = {
  /*  (uint32)&M_LightDir,
      (uint32)&gDir, */
  (uint32)&M_LightPoint,
  (uint32)&gPoint,
  0
};

static uint32              gNullLightList[] = {
  0
};


Err Model_LoadFromDisk(char* filename, uint32* numPods, uint32* maxPodVerts)
{
  char    textureName[48], modelName[48];
  char    shadowName[48];
  int32   i;
  Pod*    curPod;

  strcpy(textureName, filename);
  strcat(textureName, ".utf");

  strcpy(modelName, filename);
  strcat(modelName, ".bsf");

  /* Now read in the texture(s) and pod(s) */

  gBSDF = ReadInMercuryData(modelName, NULL, textureName, AllocMem);
  if (gBSDF == NULL) 
    {
      printf("err shadowModel model!\n");
      *numPods = 0;
      return -1;
    } 
  else
    {
      printf("Successfully read in %d Pods.\n", gBSDF->numPods);
      *numPods = gBSDF->numPods;
    }
  
#ifdef RENDER_SHADOW
  strcpy(shadowName, filename);
  strcat(shadowName, ".sdw");
  shadow0BSDF =  ReadInMercuryData(shadowName, NULL, NULL, AllocMem);
  if (shadow0BSDF == NULL)
    {
      printf("err loading shadow model \"%s\"!\n", shadowName);
    }
  else
    {
      printf("Successfully read in shadow %d Pods.\n", shadow0BSDF->numPods);
    }

  curPod = shadow0BSDF->pods;
  for (i=0; i<shadow0BSDF->numPods; i++) 
    {
     curPod->plights =  gNullLightList;
    }
#endif

  /* Scale each pod in the model, and assign our light list to each pod.
   * Also figure out how many vertices we need to reserve space for in our
   * transform buffer */

  *maxPodVerts = 0;
  curPod = gBSDF->pods;
  for (i=0; i<gBSDF->numPods; i++) {
    if (curPod->pgeometry->vertexcount > *maxPodVerts)
      *maxPodVerts = curPod->pgeometry->vertexcount;

    /*	curPod->plights = gModelLightList; */
    curPod = curPod->pnext;
  }
  return 0;
}

Err Model_LoadFromDisk2(char* filename, uint32* numPods, uint32* maxPodVerts)
{
  char    textureName[48], modelName[48];
  char    shadowName[48];
  int32   i;
  Pod*    curPod;

  strcpy(textureName, filename);
  strcat(textureName, ".utf");

  strcpy(modelName, filename);
  strcat(modelName, ".bsf");

  /* Now read in the texture(s) and pod(s) */

  gBSDF2 = ReadInMercuryData(modelName, NULL, textureName, AllocMem);
  if (gBSDF2 == NULL) {
    printf("err loading model!\n");
    *numPods = 0;
    return -1;
  } else {
    printf("Successfully read in %d Pods.\n", gBSDF2->numPods);
    *numPods = gBSDF2->numPods;
  }

#ifdef RENDER_SHADOW
  strcpy(shadowName, filename);
  strcat(shadowName, ".sdw");
  shadow1BSDF =  ReadInMercuryData(shadowName, NULL, NULL, AllocMem);
  if (shadow1BSDF == NULL)
    {
      printf("err loading shadow model \"%s\"!\n", shadowName);
    }
  else
    {
      printf("Successfully read in shadow %d Pods.\n", shadow1BSDF->numPods);
    }
  curPod = shadow1BSDF->pods;
  for (i=0; i<shadow1BSDF->numPods; i++) 
    {
     curPod->plights =  gNullLightList;
    }
#endif



  /* Scale each pod in the model, and assign our light list to each pod.
   * Also figure out how many vertices we need to reserve space for in our
   * transform buffer */

  *maxPodVerts = 0;
  curPod = gBSDF2->pods;
  for (i=0; i<gBSDF2->numPods; i++) {
    if (curPod->pgeometry->vertexcount > *maxPodVerts)
      *maxPodVerts = curPod->pgeometry->vertexcount;
    /*	curPod->plights = gModelLightList; */
    curPod = curPod->pnext;
  }
  return 0;
}

Err Model_LoadFromDisk3(char* filename, uint32* numPods, uint32* maxPodVerts)
{
  char    textureName[48], modelName[48];
  int32   i;
  Pod*    curPod;

  strcpy(textureName, filename);
  strcat(textureName, ".utf");

  strcpy(modelName, filename);
  strcat(modelName, ".bsf");

  /* Now read in the texture(s) and pod(s) */

  gBSDF3 = ReadInMercuryData(modelName, NULL, textureName, AllocMem);
  if (gBSDF3 == NULL) {
    printf("err loading model!\n");
    *numPods = 0;
    return -1;
  } else {
    printf("Successfully read in %d Pods.\n", gBSDF3->numPods);
    *numPods = gBSDF3->numPods;
  }

  /* Scale each pod in the model, and assign our light list to each pod.
   * Also figure out how many vertices we need to reserve space for in our
   * transform buffer */

  *maxPodVerts = 0;
  curPod = gBSDF3->pods;
  for (i=0; i<gBSDF3->numPods; i++) {
    if (curPod->pgeometry->vertexcount > *maxPodVerts)
      *maxPodVerts = curPod->pgeometry->vertexcount;
    curPod->plights = gModelLightList;
    curPod = curPod->pnext;
  }
  return 0;
}

Err Model_LoadFromDisk4(char* filename, uint32* numPods, uint32* maxPodVerts)
{
  char    textureName[48], modelName[48];
  int32   i;
  Pod*    curPod;

  strcpy(textureName, filename);
  strcat(textureName, ".utf");

  strcpy(modelName, filename);
  strcat(modelName, ".bsf");

  /* Now read in the texture(s) and pod(s) */

  gBSDF4 = ReadInMercuryData(modelName, NULL, textureName, AllocMem);
  if (gBSDF4 == NULL) {
    printf("err loading model!\n");
    *numPods = 0;
    return -1;
  } else {
    printf("Successfully read in %d Pods.\n", gBSDF4->numPods);
    *numPods = gBSDF4->numPods;
  }

  /* Scale each pod in the model, and assign our light list to each pod.
   * Also figure out how many vertices we need to reserve space for in our
   * transform buffer */

  *maxPodVerts = 0;
  curPod = gBSDF4->pods;
  for (i=0; i<gBSDF4->numPods; i++) {
    if (curPod->pgeometry->vertexcount > *maxPodVerts)
      *maxPodVerts = curPod->pgeometry->vertexcount;
    curPod->plights = gModelLightList;
    curPod = curPod->pnext;
  }
  return 0;
}

Err Model_LoadFromDisk5(char* filename, uint32* numPods, uint32* maxPodVerts)
{
  char    textureName[48], modelName[48];
  int32   i;
  Pod*    curPod;
  PodGeometry *pg1, *pg2;
  float   *tmp, start_seg, delta_seg;

  strcpy(textureName, filename);
  strcat(textureName, ".utf");

  strcpy(modelName, filename);
  strcat(modelName, ".bsf");

  /* Now read in the texture(s) and pod(s) */

  gBSDF5 = ReadInMercuryData(modelName, NULL, textureName, AllocMem);
  if (gBSDF5 == NULL) {
    printf("err loading model!\n");
    *numPods = 0;
    return -1;
  } else {
    printf("Successfully read in %d Pods.\n", gBSDF5->numPods);
    *numPods = gBSDF5->numPods;
  }

  /* Scale each pod in the model, and assign our light list to each pod.
   * Also figure out how many vertices we need to reserve space for in our
   * transform buffer */

  *maxPodVerts = 0;
  curPod = gBSDF5->pods;
  printf("Rune/Dust NumPods=%d\n", gBSDF5->numPods);
  for (i=0; i<gBSDF5->numPods; i++) {
    if (curPod->pgeometry->vertexcount > *maxPodVerts)
      *maxPodVerts = curPod->pgeometry->vertexcount;
    /*   curPod->plights = gModelLightList; */
    curPod->plights = gNullLightList;
    curPod->flags |= nocullFLAG;   /* These are 2D objects that always face the camera */
    curPod = curPod->pnext;
 }

  /* This is just a template for all "flat" objects, copy it to make pods for dust */
  
 
 pg1 = gBSDF5->pods->pgeometry;
  for (i=0; i<MAX_DUST; i++)
    {
      memcpy(&(dustPods[i]), gBSDF5->pods, sizeof(Pod));
      dustPods[i].ptexture = &(gBSDF5->textures[1]);
      dustPods[i].plights = gNullLightList; /* Not really necessary */
      /* Patch Some Pointers */
      pg2 = dustPods[i].pgeometry = &dustPodsGeo[i];
      memcpy(pg2, pg1, sizeof(PodGeometry));
      pg2->pvertex = &dustVertex[i*NUM_DUST_VERTS*6];
      pg2->puv  = &dustUV[i*(NUM_DUST_VERTS-2)*6];
      pg2->sharedcount = 0;
    }

  for (i=0; i<MAX_POOFS; i++)
    {
      memcpy(&(poofPods[i]), gBSDF5->pods, sizeof(Pod));
      poofPods[i].ptexture = &(gBSDF5->textures[0]);   /* Not Really Necessary */
      poofPods[i].plights = gNullLightList; /* Not really necessary */
      /* Patch Some Pointers */
      pg2 = poofPods[i].pgeometry = &poofPodsGeo[i];
      memcpy(pg2, pg1, sizeof(PodGeometry));
      pg2->pvertex = &poofVertex[i*NUM_DUST_VERTS*6];
      pg2->puv  = &poofUV[i*(NUM_DUST_VERTS-2)*6];
      pg2->sharedcount = 0;
    }

#ifdef RENDER_SHADOW
  start_seg = -1.0;
  delta_seg = 2.0/SHADOW_SEG;
  for (i=0; i<SHADOW_SEG; i++, start_seg += delta_seg)
    {
      if (i>0)
	{
	  shadowPods[i-1].pnext = &(shadowPods[i]);
	}
      memcpy(&(shadowPods[i]), gBSDF5->pods, sizeof(Pod));
      shadowPods[i].ptexture = &(shadowPTextures[i]);   /* Really Necessary, Get Shadow Template */
      memcpy(shadowPods[i].ptexture, &(gBSDF5->textures[4]), sizeof(PodTexture));
      shadowPods[i].plights = gNullLightList; /* Don't light the damn thing! */
      shadowPods[i].pnext = NULL;
      /* Patch Some Pointers */
      pg2 = shadowPods[i].pgeometry = &shadowPodsGeo[i];
      memcpy(pg2, pg1, sizeof(PodGeometry));
      pg2->pvertex = &shadowVertex[i*NUM_DUST_VERTS*6];
      pg2->puv  = &shadowUV[i*(NUM_DUST_VERTS-2)*6];
      pg2->sharedcount = 0;

      tmp = pg2->pvertex;
      tmp[3] = tmp[5] = tmp[9] = tmp[11] = tmp[15] = tmp[17] = tmp[21] = tmp[23] = 0.0; 
      tmp[4] = tmp[10] = tmp[16] = tmp[22] = 1.0; 

      tmp[1] = tmp[7] = tmp[13] = tmp[19] = SHADOW_Y;
      tmp[0] = tmp[12] = -1*SHADOW_SIZE;
      tmp[6] = tmp [18] = SHADOW_SIZE;      

      tmp[2] = tmp[8] = SHADOW_SIZE * start_seg;
      tmp[14] = tmp[20] = SHADOW_SIZE *(start_seg+delta_seg);
      
      tmp = pg2->puv;
      tmp[0] = tmp[1] = tmp[3] = tmp[4] = tmp[6] = tmp[11] = 0.0;
      tmp[2] = tmp[5] = tmp[7] = tmp[8] = tmp[9] = tmp[10] = 1.0;

    }

#ifndef SINGLE_SHADOW
  start_seg = -1.0;
  for (i=SHADOW_SEG; i<ALLOC_SHADOW_SEG; i++, start_seg += delta_seg)
    {
      if (i>0)
	{
	  shadowPods[i-1].pnext = &(shadowPods[i]);
	}
      memcpy(&(shadowPods[i]), gBSDF5->pods, sizeof(Pod));
      shadowPods[i].ptexture = &(shadowPTextures[i]);   /* Really Necessary, Get Shadow Template */
      memcpy(shadowPods[i].ptexture, &(gBSDF5->textures[4]), sizeof(PodTexture));
      shadowPods[i].plights = gNullLightList; /* Don't light the damn thing! */
      shadowPods[i].pnext = NULL;
      /* Patch Some Pointers */
      pg2 = shadowPods[i].pgeometry = &shadowPodsGeo[i];
      memcpy(pg2, pg1, sizeof(PodGeometry));
      pg2->pvertex = &shadowVertex[i*NUM_DUST_VERTS*6];
      pg2->puv  = &shadowUV[i*(NUM_DUST_VERTS-2)*6];
      pg2->sharedcount = 0;

      tmp = pg2->pvertex;
      tmp[3] = tmp[5] = tmp[9] = tmp[11] = tmp[15] = tmp[17] = tmp[21] = tmp[23] = 0.0; 
      tmp[4] = tmp[10] = tmp[16] = tmp[22] = 1.0; 

      tmp[1] = tmp[7] = tmp[13] = tmp[19] = SHADOW_Y;
      tmp[0] = tmp[12] = -1*SHADOW_SIZE;
      tmp[6] = tmp [18] = SHADOW_SIZE;      

      tmp[2] = tmp[8] = SHADOW_SIZE * start_seg;
      tmp[14] = tmp[20] = SHADOW_SIZE *(start_seg+delta_seg);
      
      tmp = pg2->puv;
      tmp[0] = tmp[1] = tmp[3] = tmp[4] = tmp[6] = tmp[11] = 0.0;
      tmp[2] = tmp[5] = tmp[7] = tmp[8] = tmp[9] = tmp[10] = 1.0;
    }

#endif

#endif
  return 0;
}









