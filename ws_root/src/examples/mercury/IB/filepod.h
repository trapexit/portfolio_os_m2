/*
 * @(#) filepod.h 96/05/09 1.3
 */

#define MAX_POOFS 	 20
#define MAX_DUST 	 20
#define USE_SINGLE_DUST
#define DUST_DELTAY  	 0.6
#define FRAME_DISPLAY


#define DRAW_SHADOWS
#undef  FAKE_SHADOW
#define  BLINN_SHADOW
#undef  SINGLE_SHADOW



#ifdef DRAW_SHADOWS

#ifdef  BLINN_SHADOW
#undef  RENDER_SHADOW
#else
#define RENDER_SHADOW      /* RENDER_SHADOW refers to shadows rendered onto a quad */
#endif

#ifdef  FAKE_SHADOW

#undef  SINGLE_SHADOW
#define SHADOW_SEG   1

#else

#define SHADOW_SEG   4
#define POD_SHADOW        /* Refers to shadows that use Shadow Objects for shadow generation */

#endif

#define SHADOW_SIZE  400    /* Initial Shadow Size */
#define SHADOW_Y     0.5

#ifndef SINGLE_SHADOW
#define ALLOC_SHADOW_SEG 2*SHADOW_SEG
#endif

#ifdef POD_SHADOW
extern BSDF* shadow0BSDF;
extern BSDF* shadow1BSDF;
#endif

#ifdef RENDER_SHADOW
extern Pod   shadowPods[];
#endif

#endif /* DRAW_SHADOWS */

#ifdef BLINN_SHADOW
typedef struct BlinnShadow {
  int     numNodes;
  Matrix  *Original;
  Matrix  Squash;
} BlinnShadow;

extern BlinnShadow shadowTrans[2][20];

#endif

extern BSDF* gBSDF;
extern BSDF* gBSDF2;
extern BSDF* gBSDF3;
extern BSDF* gBSDF4;
extern BSDF* gBSDF5;
extern Pod  dustPods[];
extern Pod  poofPods[];

extern Err Model_LoadFromDisk(char*, uint32* numPods, uint32* maxPodVerts);
extern Err Model_LoadFromDisk2(char*, uint32* numPods, uint32* maxPodVerts);
extern Err Model_LoadFromDisk3(char*, uint32* numPods, uint32* maxPodVerts);
extern Err Model_LoadFromDisk4(char*, uint32* numPods, uint32* maxPodVerts);
extern Err Model_LoadFromDisk5(char*, uint32* numPods, uint32* maxPodVerts);

