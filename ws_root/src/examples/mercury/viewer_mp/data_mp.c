/*
 * Copyright (c) 1993-1996, an unpublished work by The 3DO Company.
 * All rights reserved. This material contains confidential
 * information that is the property of The 3DO Company. Any
 * unauthorized duplication, disclosure or use is prohibited.
 * 
 * @(#) data_mp.c 96/10/30 1.1
 * Distributed with Portfolio V3.1
 *
 * data_mp.c contains all the new routines required for the multiprocessor 
 * features of the example viewer_mp.
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
#include <kernel:cache.h>
#include <kernel:mem.h>
#include <kernel:io.h>
#include <kernel:operror.h>
#include <kernel:task.h>
#include <device:mp.h>
#include <string.h>
#include <stdio.h>
#else
#include <kernel/types.h>
#include <graphics/clt/clt.h>
#include <kernel/cache.h>
#include <kernel/mem.h>
#include <kernel/io.h>
#include <kernel/operror.h>
#include <kernel/task.h>
#include <device/mp.h>
#include <string.h>
#include <stdio.h>
#endif
#include "mercury.h"




float gHitherPlane = 10.0;



#define kFloatsPerVertex		6
#define kSizePerVertex			(kFloatsPerVertex*sizeof(float))
#define kVerticesPerSet			8
#define kSetsOfVertices			2	/* We double-buffer the vertex data which will be altered 
									by the slave function running on the 2nd processor. */
static float *gpCornerVertices[kSetsOfVertices];

/* The vertex table refered to by gCornerSharedVerts is now initialized in Init_MPStuff. 
Two copies are created for double buffering. */

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
    /* float fxmin, fymin, fzmin */				-100.0, -100.0, -100.0,
    /* float fxextent, fyextent, fzextent */	200.0, 200.0, 200.0,
    /* float *pvertex */ 						NULL,
    /* short *pshared */						gCornerSharedVerts,
    /* uint16 vertexcount */			kVerticesPerSet,
    /* uint16 sharedcount */			sizeof(gCornerSharedVerts)/(2 * sizeof(short)),
    /* short *pinxex */					gCornerIndices,
    /* float *puv */					NULL
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


static Err SlaveFunction(void *arg);	/*the function that runs 
										on the second processor*/
void ManageMP(void);	/* This function runs as a separate thread on 
					the first processor and manages the second processor. */


Item readyToCalcSignals[kSetsOfVertices];	/*signals sent whenever one of the data 
											sets has been drawn and can be recalculated*/

Item readyToDrawSignals[kSetsOfVertices];	/*signals sent whenever one of the data 
											sets has been recalculated and can be drawn*/
											
Item parentTaskItem;
Item childTaskItem;
  /*Item number of the thread which manages the second processor (note that this thread
   *runs on the first processor... the reason this extra thread is necessary is that
   *otherwise it's hard to synchronize with the second processor since it might finish what
   *it's doing right while the first processor is busy doing something important)
   */

/*counters which track which of the sets are being worked on by the two processors*/
int setBeingDrawn=0;
int setBeingCalced=1;

/*the second processor needs a stack*/
uint32 *stack;

/*	This function initializes the thread used to control the second processor 
	and the signals used to communicate with that thread. It also allocates buffers 
	shared between the two processors.
*/
void Init_MPStuff(void)
{
	int i;
	CacheInfo ci;

	parentTaskItem=CURRENTTASKITEM;
	
	GetCacheInfo(&ci, sizeof(ci));
	
	/*allocate a stack for the second processor*/	
	stack=(uint32*)	AllocMemAligned(ALLOC_ROUND(4096, ci.cinfo_DCacheLineSize),
                            MEMTYPE_FILL,
                            ci.cinfo_DCacheLineSize);
							
	
	gpCornerVertices[0]=(gfloat*) AllocMemAligned(ALLOC_ROUND(kSizePerVertex*kVerticesPerSet*kSetsOfVertices, 
											ci.cinfo_DCacheLineSize),
                            				MEMTYPE_FILL,
                            				ci.cinfo_DCacheLineSize);
	for(i=1; i<kSetsOfVertices; i++)
	{
		gpCornerVertices[i]=gpCornerVertices[0] + i*kFloatsPerVertex*kVerticesPerSet;
	}
	/*gCornerGeometry.pvertex=gpCornerVertices[0];*/
	
	for(i=0; i<kSetsOfVertices; i++)
	{
		/* Initialize ith set of vertices */
		/* Vertex 0 */
		gpCornerVertices[i][0] = -100.00; /* x */
		gpCornerVertices[i][1] = -100.00; /* y */
		gpCornerVertices[i][2] = -100.00; /* z */
		gpCornerVertices[i][3] = 0.8; /* r */
		gpCornerVertices[i][4] = 0.8; /* g */
		gpCornerVertices[i][5] = 0.8; /* b */
		/* Vertex 1 */
		gpCornerVertices[i][6] = 100.00; /* x */
		gpCornerVertices[i][7] = -100.00; /* y */
		gpCornerVertices[i][8] = -100.00; /* z */
		gpCornerVertices[i][9] = 0.8; /* r */
		gpCornerVertices[i][10] = 0.8; /* g */
		gpCornerVertices[i][11] = 0.8; /* b */
		/* Vertex 2 */
		gpCornerVertices[i][12] = 100.00; /* x */
		gpCornerVertices[i][13] = 100.00; /* y */
		gpCornerVertices[i][14] = -100.00; /* z */
		gpCornerVertices[i][15] = 0.6; /* r */
		gpCornerVertices[i][16] = 0.6; /* g */
		gpCornerVertices[i][17] = 0.6; /* b */
		/* Vertex 3 */
		gpCornerVertices[i][18] = -100.00; /* x */
		gpCornerVertices[i][19] = 100.00; /* y */
		gpCornerVertices[i][20] = -100.00; /* z */
		gpCornerVertices[i][21] = 0.6; /* r */
		gpCornerVertices[i][22] = 0.6; /* g */
		gpCornerVertices[i][23] = 0.6; /* b */
		/* Vertex 4 */
		gpCornerVertices[i][24] = -100.00; /* x */
		gpCornerVertices[i][25] = 100.00; /* y */
		gpCornerVertices[i][26] = 100.00; /* z */
		gpCornerVertices[i][27] = 0.3; /* r */
		gpCornerVertices[i][28] = 0.3; /* g */
		gpCornerVertices[i][29] = 0.3; /* b */
		/* Vertex 5 */
		gpCornerVertices[i][30] = -100.00; /* x */
		gpCornerVertices[i][31] = -100.00; /* y */
		gpCornerVertices[i][32] = 100.00; /* z */
		gpCornerVertices[i][33] = 0.8; /* r */
		gpCornerVertices[i][34] = 0.8; /* g */
		gpCornerVertices[i][35] = 0.8; /* b */
		/* Vertex 6 */
		gpCornerVertices[i][36] = 100.00; /* x */
		gpCornerVertices[i][37] = -100.00; /* y */
		gpCornerVertices[i][38] = 100.00; /* z */
		gpCornerVertices[i][39] = 0.8; /* r */
		gpCornerVertices[i][40] = 0.8; /* g */
		gpCornerVertices[i][41] = 0.8; /* b */
		/* Dummy VertexÑmust be even */
		gpCornerVertices[i][42] = 0.0; /* x */
		gpCornerVertices[i][43] = 0.0; /* y */
		gpCornerVertices[i][44] = 0.0; /* z */
		gpCornerVertices[i][45] = 0.0; /* r */
		gpCornerVertices[i][46] = 0.0; /* g */
		gpCornerVertices[i][47] = 0.0; /* b */
	}

	
	/*intialize the signals that we'll get from the child task*/
	for (i=0;i<kSetsOfVertices;i++)
		readyToDrawSignals[i]=AllocSignal(0);
	  
	childTaskItem=CreateThread(ManageMP, "mp thread", CURRENTTASK->t.n_Priority+10, 2048, NULL);
  
	/*we get this signal when the child task is ready to go*/
	WaitSignal(readyToDrawSignals[setBeingDrawn]);
	
	/*tell the child task that the second set of vertex data is ready to be filled with calculated data */
	/*SendSignal(childTaskItem, readyToCalcSignals[setBeingCalced]);*/
}


/*this function is called right before MDraw... it sets up the pod with the latest data*/
void AddNewDataToPod(void)
{
  
	/*Signal the ManageMP() thread that we're done with the last set of vertices we were using
	 *(although technically it would be more efficient to have a separate function make this call
	 *as soon as M_Draw was done... probably doesn't matter in practice)
	 */
	SendSignal(childTaskItem, readyToCalcSignals[setBeingDrawn]);
	 
	setBeingDrawn=(setBeingDrawn+1) % kSetsOfVertices;
    
	/* wait for the new set of data to be ready */ 
	WaitSignal(readyToDrawSignals[setBeingDrawn]); 
	
	/* when the data's ready, attach it to the pod */
	gCornerGeometry.pvertex=gpCornerVertices[setBeingDrawn];

}


/*this function runs as a separate thread on the first processor, and manages the second
 *processor
 */
void ManageMP(void)
{  
	int i, moveCounter=0;
	int32 moveValue=-1;
	Err funcResult;
	Item ioreq;
   
   
   
	ioreq=CreateMPIOReq(FALSE);

	/*allocate signals that data set is ready to be processed*/
	for (i=0;i<kSetsOfVertices;i++)
		readyToCalcSignals[i]=AllocSignal(0);

	/*signal the parent task that the thread is ready to go */
	SendSignal(parentTaskItem, readyToDrawSignals[setBeingDrawn]);
	
	do {
		if(moveCounter++ > 50) {
			moveCounter = 0;
			moveValue = -moveValue;
		}
		
		/* 
		Invalidate the cache lines for the part of memory about to be written to by the 2nd processor.
		This is done to ensure that the 1st processor recognizes the changes to memory about to be
		made by the 2nd processor.
		*/
		FlushDCache(0, (void*)gpCornerVertices[setBeingCalced], kSizePerVertex*kVerticesPerSet);
  
  		/* Start the second processor */
		if (DispatchMPFunc(ioreq, SlaveFunction, (void*) moveValue, &stack[1024], 4096, &funcResult)<0) {
			printf("ERROR in DispatchMPFunc\n");
			exit(-1);
		};
		/* Wait for the processor to finish */
		WaitIO(ioreq);

		/* signal the main task that this set of vertex data is ready to be drawn */
		SendSignal(parentTaskItem, readyToDrawSignals[setBeingCalced]);
   
		setBeingCalced=(setBeingCalced+1) % kSetsOfVertices;
   
		/* wait for the next set of vertices to be finished drawing*/   
		WaitSignal(readyToCalcSignals[setBeingCalced]);
   
	} while (true);
}


/*this is the function that actually runs on the second processor. Note that
 *it makes no system calls other than doing a writeback from the data cache
 */
static Err SlaveFunction(void *arg)
{
	int i, offset, moveValue= (int32)arg;
	
	for(i=0; i<4; i++)
	{
		offset = 2+i*kFloatsPerVertex;
		*(gpCornerVertices[setBeingCalced] + offset) 
				= *(gpCornerVertices[setBeingDrawn] + offset) + moveValue;
	}
	
		  
	/* Then we do a writeback from the data cache for the area of memory just written to. */
	WriteBackDCache(0, (void*)gpCornerVertices[setBeingCalced], kSizePerVertex*kVerticesPerSet);
    
	return 0;
	
}	
	
 
