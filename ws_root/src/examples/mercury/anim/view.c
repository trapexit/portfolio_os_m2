/*
**	File:		view.c	
**
**	Contains:	Code to set up camera and viewing parameters 
**
**	Written by:	Ravindar Reddy
**
**	Copyright:	© 1996 by The 3DO Company. All rights reserved.
**	          	This material constitutes confidential and proprietary
**	          	information of the 3DO Company and shall not be used by
**	          	any Person or for any purpose except as expressly
**	          	authorized in writing by the 3DO Company.
**
**	Last Modified:	 96/04/18	version		1.23	
*/

#ifdef MACINTOSH
#include <stdio.h>
#include <stdlib.h>
#include <:kernel:mem.h>
#include <:kernel:types.h>
#include <:graphics:clt:clt.h>
#include <:graphics:clt:gstate.h>
#include <:graphics:graphics.h>
#include <:graphics:view.h>
#include <math.h>
#include <misc:event.h>
#include <:kernel:time.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <kernel/mem.h>
#include <kernel/types.h>
#include <graphics/clt/clt.h>
#include <graphics/clt/gstate.h>
#include <graphics/graphics.h>
#include <graphics/view.h>
#include <math.h>
#include <misc/event.h>
#include <kernel/time.h>
#endif
#include "mercury.h"
#include "bsdf_read.h"
#include "AM_Model.h"
#include "view.h"

#define CPUMUL 2
#define CPUSPEED 66000000

/* globals to control the camera movement */
extern float twist,radius,xangle,yangle;
extern float radius_inc;

/* globals to control the renderer */
extern GState *gs;
extern uint32 FBWIDTH;
extern uint32 FBHEIGHT;

/* variables local to this file */
static float hither = 1.0;
static float vp_hither = 1.2;
static float left = -1.0;
static float right = 1.0;
static BBox b_box;
static float view_wclose = 1.01;
static float view_wfar = 1000.0;
static Vector3D vrp;
static Matrix frustrumviewmat;

/* Used in mainloop, also */
Matrix cameramat;

static void
CalcPodListBbox(
	BBox *bbox,
	Pod *pod_list,
	uint32 num_pods
	)
{
	uint32 i;
	Pod *tmp;

	if( num_pods <= 0 ) return;

	tmp = pod_list;
 
	bbox->fxmin = tmp->pgeometry->fxmin;
	bbox->fymin = tmp->pgeometry->fymin;
	bbox->fzmin = tmp->pgeometry->fzmin;
	bbox->fxextent = tmp->pgeometry->fxextent;
	bbox->fyextent = tmp->pgeometry->fyextent;
	bbox->fzextent = tmp->pgeometry->fzextent;

	tmp = tmp->pnext;
	for( i = 1; i < num_pods; i++ )
	{
		if( bbox->fxmin > tmp->pgeometry->fxmin ) 
			bbox->fxmin = tmp->pgeometry->fxmin;
		if( bbox->fymin > tmp->pgeometry->fymin ) 
			bbox->fymin = tmp->pgeometry->fymin;
		if( bbox->fzmin > tmp->pgeometry->fzmin ) 
			bbox->fzmin = tmp->pgeometry->fzmin;
		if( bbox->fxextent < tmp->pgeometry->fxextent )
			bbox->fxextent = tmp->pgeometry->fxextent;
		if( bbox->fyextent < tmp->pgeometry->fyextent )
			bbox->fyextent = tmp->pgeometry->fyextent;
		if( bbox->fzextent < tmp->pgeometry->fzextent )
			bbox->fzextent = tmp->pgeometry->fzextent;

		tmp = tmp->pnext;
	}
}
		
BSDF* 
GameCodeInit(
	CloseData   **pclosedata,
	GState *gs, 
	int argc, 
	char **argv
	)
{
	BSDF 		*sdf;

	/* init the max vertex buffer count */
	*pclosedata = M_Init(2000, 400, gs);
	M_InitMP(*pclosedata, 2000, 50000, 650);

	(*pclosedata)->fogcolor = M_PackColor(0.0, 0.0, 0.0);
    (*pclosedata)->fwclose = view_wclose;
    (*pclosedata)->fwfar = view_wfar;
    (*pclosedata)->fscreenwidth = FBWIDTH;
    (*pclosedata)->fscreenheight = FBHEIGHT;
	(*pclosedata)->frbase = 0.2;
	(*pclosedata)->fgbase = 0.2;
	(*pclosedata)->fbbase = 0.2;
	(*pclosedata)->fabase = 1.0;

	sdf = MoveObjectsInit( argc, argv);
    MoveCameraInit(*pclosedata);

    return sdf;
}

/*
float ambientr = 1.0;
float ambientg = 1.0;
float ambientb = 1.0;
*/

PerfInfo perf;

void
InitPerfInfo(
	PerfInfo *p,
	uint32 tri_count
	)
{
	p->disableAnim = 0;
	p->printInfo = 0;
	p->numFrames = 0;
	p->scene3DTriCount = tri_count;
}
		

void 
AnalyzeTime(
	PerfInfo *p
	)
{
	TimerTicks fDelta, mDelta, aDelta;
	gfloat fTime, mTime, aTime, eTime;
	uint32 fPerc, mPerc, aPerc;

	/* initialize the data */
	if( p->numFrames == 0 )
	{
		p->fInfo.minTime = 1000000000.0;
		p->fInfo.maxTime = 0.0;
		p->fInfo.accumTime = 0.0;
		p->fInfo.totalTime = 0.0;

		p->mInfo.minTime = 1000000000.0;
		p->mInfo.maxTime = 0.0;
		p->mInfo.accumTime = 0.0;
		p->mInfo.totalTime = 0.0;

		p->aInfo.minTime = 1000000000.0;
		p->aInfo.maxTime = 0.0;
		p->aInfo.accumTime = 0.0;
		p->aInfo.totalTime = 0.0;

		p->total3DTriCount  = 0;
		p->total2DTriCount  = 0;
	}

	/* no triangles rendered for this frame */
	if ( p->scene2DTriCount <= 0 ) return;

	/* number of frames rendered and total triangle count */
	p->numFrames++;
	p->total2DTriCount += p->scene2DTriCount;
	p->total3DTriCount += p->scene3DTriCount;

	/* get the time duration for each module */
	SubTimerTicks( &p->fInfo.sTime, &p->fInfo.eTime, &fDelta );
	fTime = (gfloat)fDelta.tt_Lo*4*CPUMUL;	
	eTime = fTime/(gfloat)p->scene2DTriCount;
	if( eTime > p->fInfo.maxTime ) p->fInfo.maxTime = eTime;
	if( eTime < p->fInfo.minTime ) p->fInfo.minTime = eTime;
	p->fInfo.accumTime += eTime;
	p->fInfo.totalTime += fTime;

	SubTimerTicks( &p->mInfo.sTime, &p->mInfo.eTime, &mDelta );
	mTime = (gfloat)mDelta.tt_Lo*4*CPUMUL;	
	eTime = mTime/(gfloat)p->scene2DTriCount;
	if( eTime > p->mInfo.maxTime ) p->mInfo.maxTime = eTime;
	if( eTime < p->mInfo.minTime ) p->mInfo.minTime = eTime;
	p->mInfo.accumTime += eTime;
	p->mInfo.totalTime += mTime;


	SubTimerTicks( &p->aInfo.sTime, &p->aInfo.eTime, &aDelta );
	aTime = (gfloat)aDelta.tt_Lo*4*CPUMUL;	
	eTime = aTime/(gfloat)p->scene2DTriCount;
	if( eTime > p->aInfo.maxTime ) p->aInfo.maxTime = eTime;
	if( eTime < p->aInfo.minTime ) p->aInfo.minTime = eTime;
	p->aInfo.accumTime += eTime;
	p->aInfo.totalTime += aTime;

	if( p->numFrames == 50 )
	{
		p->fInfo.totalTime /= (gfloat)CPUSPEED; /* From cycles to seconds */
		p->fInfo.accumTime /= (gfloat)p->numFrames;
		p->fInfo.maxTime = ( p->fInfo.maxTime - p->fInfo.accumTime ) / 
							p->fInfo.accumTime;
		p->fInfo.minTime = ( p->fInfo.accumTime - p->fInfo.minTime ) / 
							p->fInfo.accumTime;
		if ( p->fInfo.maxTime > p->fInfo.minTime ) 
			fPerc = (uint32)(p->fInfo.maxTime * 1000.0);
		else fPerc = (uint32)( p->fInfo.minTime * 1000.0);	

		p->mInfo.totalTime /= (gfloat)CPUSPEED; /* From cycles to seconds */
		p->mInfo.accumTime /= (gfloat)p->numFrames;
		p->mInfo.maxTime = ( p->mInfo.maxTime - p->mInfo.accumTime ) / 
							p->mInfo.accumTime;
		p->mInfo.minTime = ( p->mInfo.accumTime - p->mInfo.minTime ) / 
							p->mInfo.accumTime;

		p->aInfo.totalTime /= (gfloat)CPUSPEED; /* From cycles to seconds */
		p->aInfo.accumTime /= (gfloat)p->numFrames;
		p->aInfo.maxTime = ( p->aInfo.maxTime - p->aInfo.accumTime ) / 
							p->aInfo.accumTime;
		p->aInfo.minTime = ( p->aInfo.accumTime - p->aInfo.minTime ) / 
							p->aInfo.accumTime;

		if ( !p->disableAnim )
		printf("Animation Engine + Mercury performance:\n");
		else printf("Mercury only performance:\n");
		printf("\t%d triangles to screen",
			(int)(p->total2DTriCount / p->numFrames)); 
		printf("\t %g (+/- %d.%d percent) cycles/tri\n",
			p->fInfo.accumTime,fPerc/10,fPerc % 10);
		printf("\t%g triangles/sec\n",
			(gfloat)p->total3DTriCount / p->fInfo.totalTime);
		printf("\t%g triangles/sec to screen\n",
			( (gfloat)CPUSPEED)/p->fInfo.accumTime );
		printf("\t%g frames/sec\n",
			p->numFrames / p->fInfo.totalTime );

		mPerc = p->mInfo.totalTime * 100.0 / p->fInfo.totalTime;
		aPerc = p->aInfo.totalTime * 100.0 / p->fInfo.totalTime;
		fPerc = 100 - mPerc - aPerc;
		printf("\tPercentages :\n" );
		printf("\t\tMercury %d (%f sec)\n", mPerc, p->mInfo.totalTime );
		printf("\t\tAnimation %d (%f sec)\n", aPerc, p->aInfo.totalTime );
		printf("\t\tMisc %d (Total Time = %f sec)\n", fPerc, p->fInfo.totalTime );	

		p->numFrames = 0;
		p->total2DTriCount = 0;
		p->total3DTriCount = 0;
	}
}

uint32 
GameCode( 
	BSDF *sdf,
	CloseData *pclosedata 
	)
{
	Pod *PodList;
	uint32 PodCount;
	uint32 podpiperet;
	static uint32 lastpodcount = 0;
	static Pod *firstpod;

    MoveCamera(pclosedata);

	PodCount=0;

#ifdef ANIM
	/* Only sort if number of objects has changed */
	if (lastpodcount != sdf->numAmodels) {
		PodList = AM_GetPodList( sdf->amdls, sdf->numAmodels, &PodCount );

		firstpod = M_Sort(PodCount,PodList, 0);
		lastpodcount = sdf->numAmodels;
	}
#else
	/* Only sort if number of objects has changed */
	if (lastpodcount != sdf->numPods) {
		PodList = sdf->pods;
		PodCount = sdf->numPods;

		firstpod = M_Sort(PodCount,PodList, 0);
		lastpodcount = sdf->numPods;
	}
#endif
	podpiperet = M_Draw(firstpod,pclosedata);
	podpiperet = podpiperet & 0xffff;

	return podpiperet;
}

void 
MoveCameraInit( 
	CloseData *pclosedata 
	)
{
    ViewPyramid vp;

    vp.left = left;
    vp.right = right;
    vp.top = 0.75;
    vp.bottom = -0.75;
    vp.hither = vp_hither;

    Matrix_Perspective(&frustrumviewmat, &vp, 0, FBWIDTH, 0, FBHEIGHT, 1./hither);

	/* view reference point at the center of the bbox */
	vrp.x = b_box.fxmin + b_box.fxextent / 2.0;
	vrp.y = b_box.fymin + b_box.fyextent / 2.0;
	vrp.z = b_box.fzmin + b_box.fzextent / 2.0;

	/* set the view radius */
	if( b_box.fxextent > b_box.fyextent )
		if( b_box.fxextent > b_box.fzextent )
			radius = b_box.fxextent;
		else radius = b_box.fzextent;
	else {
		if( b_box.fyextent > b_box.fzextent )
			radius = b_box.fyextent;
		else radius = b_box.fzextent;
	}
	radius *= 1.5;
	radius_inc = radius / 100.0;

	/* set the yon plane */
	pclosedata->fwfar = 10.0 * radius;

	printf( "View radius = %f\n", radius );

#ifdef VERBOSE
	printf( "Scene bounding box:\n" );
	printf( "\t min point = %f, %f, %f,\n",
		b_box.fxmin, b_box.fymin, b_box.fzmin );
	printf( "\t extents   = %f, %f, %f,\n",
		b_box.fxextent, b_box.fyextent, b_box.fzextent );
#endif
}

void 
MoveCamera( 
	CloseData *pclosedata 
	)
{

    Vector3D	camLocation;

    extern long automovecam;
    
    Matrix_GetTranslation( &cameramat, &camLocation );
    Matrix_LookAt( &cameramat, &vrp, &camLocation, twist );
    
    M_SetCamera( pclosedata, &cameramat, &frustrumviewmat );

}

typedef struct MemList {
	struct MemList *next;
}MemList;

MemList **MemCurrent=0;

void * 
MyAlloc(
	int32 memSize, 
	uint32 memFlags
	)
{
	MemList * result;
	
	result = AllocMemTrackWithOptions(memSize+sizeof(MemList),memFlags);
	if( result == NULL )
	{
		printf( "ERROR : Out of memory, requested %d bytes\n", memSize );
		exit( 1 );
	}

#if 0
	printf("MyAlloc %d 0x%x => 0x%x\n",
		memSize,memFlags,result );
#endif

	if (MemCurrent)
	{
		result->next = *MemCurrent;
		*MemCurrent = result;
	}
	return (void*)(result+1);
}

void 
MyFreeList(
	MemList *list
	)
{
	MemList *next;
	while (list)
	{
		printf("MyFreeList 0x%x \n",list);
		next = list->next;
		FreeMemTrack(list);
		list = next;
	}
}

BSDF *
MoveObjectsInit(
	int argc, 
	char **argv
	)
{
	MemList *testlist=0;
	BSDF *sdf;
	char *geomFile, *texFile, *animFile;

	(void)&argc;

	MemCurrent = &testlist;
	
	geomFile = argv[0];
	texFile = argv[1];
	if( argc > 2 ) animFile = argv[2];
	else animFile = NULL;

	sdf = ReadInMercuryData( geomFile, animFile, texFile, MyAlloc );

	CalcPodListBbox( &b_box, sdf->pods, sdf->numPods );
	/*
	MyFreeList(testlist);
	exit (0);
	*/

	return (sdf);
}
