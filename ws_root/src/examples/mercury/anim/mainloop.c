/*
**	@(#) mainloop.c 96/09/17 1.45
**
**	File:		mainloop.c	
**
**	Contains:	main loop for animation viewer 
**
**	Written by:	Ravindar Reddy
**
**	Copyright:	© 1996 by The 3DO Company. All rights reserved.
**	          	This material constitutes confidential and proprietary
**	          	information of the 3DO Company and shall not be used by
**	          	any Person or for any purpose except as expressly
**	          	authorized in writing by the 3DO Company.
**
**	Last Modified:	 96/04/22	version		1.31	
*/

#ifdef MACINTOSH
#include <stdio.h>
#include <stdlib.h>
#include <:graphics:view.h>
#include <:kernel:io.h>
#include <:kernel:time.h>
#include <:kernel:mem.h>
#include <:kernel:task.h>
#include <:misc:event.h>
#include <assert.h>
#include <strings.h>
#include <math.h>
#include "mercury.h"
#else
#include <stdio.h>
#include <stdlib.h>
#include <graphics/view.h>
#include <graphics/frame2d/f2d.h> /* gratuitously included for testing */
#include <kernel/io.h>
#include <kernel/time.h>
#include <kernel/mem.h>
#include <kernel/task.h>
#include <misc/event.h>
#include <assert.h>
#include <strings.h>
#include <math.h>
#include "mercury.h"
#endif

#include "bsdf_read.h"
#include "AM_Model.h"
#include "AM_Anim.h"
#include "view.h"

#define ControlMask (ControlX | ControlStart | ControlRightShift | ControlLeftShift)
#define USE_Z           1
#define NUM_FRAME_BUFFERS   3
#define ANGLEINC 3.1415/180		

/* globals to control the camera movement */
float twist = 0.0;
float radius = 300.0;
float radius_inc = 5.0;
float xangle = 0.0;
float yangle = 0.0;

/* globals to control the renderer */
GState	*gs;
uint32 FBWIDTH=320;
uint32 FBHEIGHT=240;
uint32 DEPTH=32;

/* local variables to this file */
Item     teIORequest;
IOInfo   teIOInfo;

extern Matrix cameramat;



uint32 
BestViewType(
	uint32 xres, 
	uint32 yres, 
	uint32 depth
	)
{
	uint32 viewType;

	viewType = VIEWTYPE_INVALID;

	if ((xres==320) && (yres==240) && (depth==16)) viewType = VIEWTYPE_16;
	if ((xres==320) && (yres==480) && (depth==16)) viewType = VIEWTYPE_16_LACE;
	if ((xres==320) && (yres==240) && (depth==32)) viewType = VIEWTYPE_32;
	if ((xres==320) && (yres==480) && (depth==32)) viewType = VIEWTYPE_32_LACE;
	if ((xres==640) && (yres==240) && (depth==16)) viewType = VIEWTYPE_16_640;
	if ((xres==640) && (yres==480) && (depth==16)) viewType = VIEWTYPE_16_640_LACE;
	if ((xres==640) && (yres==240) && (depth==32)) viewType = VIEWTYPE_32_640;
	if ((xres==640) && (yres==480) && (depth==32)) viewType = VIEWTYPE_32_640_LACE;

	return viewType;
}

static void
Usage( 
	char* prog 
	)
{
    printf("Usage: %s [flags] <rushsktn.bsf> <rushpage.utf> [rushsktn.anim.bsf]\n", prog );
    printf("Flags are:\n");
    printf("\t-n <numDuplicates> : Duplicate the first articulated model \n");
    printf("\t-h                 : Set FB hieght to 480\n");
    printf("\t-w                 : Set FB width to 640\n");
    printf("\t-d                 : Disable triangle engine display\n");
    printf("\t-s <startTime>       : Starting time of the animation \n");
    printf("\t-e <endTime>       : Ending time of the animation \n");
	
    printf("\nControls are:\n");
    printf("\tRight Shift    : Disable/Enable Animation Engine\n");
    printf("\tLeft Shift     : Prints performance info every 50 frames\n");
    printf("\n\tUp, Down     : Moves camera up and down\n");
    printf("\tLeft, Right    : Moves camera to left and right\n");
    printf("\tA + Up, Down   : Moves camera in and out\n");
    printf("\tB + Up, Down   : Rotate the camera left and right\n");
    printf("\tC + Up, Down   : Vary the locked FPS to slow/speed the animation\n");
    printf("\tStop           : Exits program\n");
    printf("\n");
}


main(int argc, char **argv)
{
    Item	bitmaps[NUM_FRAME_BUFFERS+USE_Z];
	Bitmap * newDestBM;
    int32	buffSignal;
    Item	viewItem;
    uint32 	viewType;
    uint32  	bmType;
	uint32 	i;
	uint32  *pVIsave;
	uint32  displayoff = 0;
    uint32	curWFBIndex = 1;

    bool	done = 0;
    ControlPadEventData cped;
    uint32	ButtonsOld;
    uint32	ButtonsCur = 0;
    uint32	Buttons;
	uint32  numDuplicates = 0;
	float cx, cy, sx, sy;

    CloseData	*pclosedata;
    AmCloseData	*amclosedata;
	AmModel *tmp_mdl;
	PerfInfo perf;
	float startTime = -1.0;
	float endTime = -1.0;
	float curFPS = 30.0;
	uint32 ac = argc;
	char **av = argv;
	char *cp, opt;
	BSDF *sdf;

	Usage( argv[0] );

	cp = *++av; --ac; /* point to first argument */
	while( ac >= 2 && *cp++ == '-' )
	{
		opt = *cp;
		switch( opt )
		{
	  		case 'n':   /* duplicate AmModels */
				numDuplicates = atoi( *++av ); ac--;
				printf( "Number of duplicate characters = %d\n", numDuplicates );
	    		break;
	  		case 's':   /* duplicate AmModels */
				startTime = (float )atoi( *++av ); ac--;
				printf( "startTime = %f\n", startTime );
	    		break;
	  		case 'e':   /* duplicate AmModels */
				endTime = (float )atoi( *++av ); ac--;
				printf( "endTime = %f\n", startTime );
	    		break;
	  		case 'h':   /* FBHEIGHT */
				FBHEIGHT = 480;
	    		break;
	  		case 'w':   /* FBWIDTH */
				FBWIDTH = 640;
	    		break;
			case 'd':
				displayoff = 1;
				break;	
	  		default: 
				exit( 1 );
				break;
		}
		ac--; /* force one less argument */
		cp = *++av; /* next argument */
 	}

	/* atleast two arguments should be left for input files */
	if ( ac < 2 ) exit( 1 );

	printf( "Screen Width = %d, Height = %d\n", FBWIDTH, FBHEIGHT );

    bmType = (DEPTH==16 ? BMTYPE_16 : BMTYPE_32);

    OpenGraphicsFolio ();

    InitEventUtility(1, 0, 0);

    buffSignal = AllocSignal(0);

    gs = GS_Create();

    if (GS_AllocLists(gs, 2, 4000) < 0) {
	printf("Can't alloc gstate command lists\n");
	exit(1);
    }

    GS_AllocBitmaps(bitmaps, FBWIDTH, FBHEIGHT, bmType, NUM_FRAME_BUFFERS, 1);

    GS_SetDestBuffer(gs, bitmaps[curWFBIndex]);
    GS_SetZBuffer(gs, bitmaps[NUM_FRAME_BUFFERS]);
    GS_SetVidSignal(gs, buffSignal);
    viewType = BestViewType(FBWIDTH, FBHEIGHT, DEPTH);
    viewItem = CreateItemVA(MKNODEID(NST_GRAPHICS, GFX_VIEW_NODE),
                VIEWTAG_VIEWTYPE, viewType,
                VIEWTAG_RENDERSIGNAL, buffSignal,
                VIEWTAG_BITMAP, bitmaps[0],
                TAG_END );
    if (viewItem < 0) { PrintfSysErr(viewItem); exit(1); }
    AddViewToViewList( viewItem, 0 );
    GS_SetView (gs, viewItem);

    sdf = GameCodeInit(&pclosedata, gs, ac, av);

	pclosedata->depth = DEPTH;

	/* Animation engine code */
	amclosedata = AM_Init( 100 );

#ifdef ANIM
	/* set the control parameters extracted fron command line args */
	tmp_mdl = sdf->amdls;
	for( i = 0; i < sdf->numAmodels; i++ )
	{
		/* over ride the default values from the commandline data */
		if( startTime > 0.0 )
		{
			tmp_mdl->control.timeStart = startTime;
			tmp_mdl->control.curTime = startTime;
		}

		if( endTime > 0.0 )
			tmp_mdl->control.timeEnd = endTime;

		tmp_mdl = tmp_mdl->next;
	}
    /* Duplicate and reposition the AmModel */
    AM_Duplicate( sdf->amdls, &sdf->numAmodels, sdf->amdls, numDuplicates );

	/* init the performance data */
	sdf->totalTriCount = sdf->totalTriCount * ( numDuplicates + 1 );
#endif

	printf( "Total triangle count in the scene = %d\n", sdf->totalTriCount );

	InitPerfInfo( &perf, sdf->totalTriCount );

    while(!done) {

	/* 
	** The following two lines enable the transparency set in the
	** material base color alpha. To disable transparency comment
	** the two lines
	*/
	newDestBM = (Bitmap*)LookupItem(bitmaps[curWFBIndex]);
	pclosedata->srcaddr = (uint32)newDestBM->bm_Buffer;

	SampleSystemTimeTT( &perf.fInfo.sTime );
	if(!displayoff) GS_BeginFrame(gs);

	pVIsave = *(uint32 **)&gs->gs_ListPtr;

	/* Animation Code */

	SampleSystemTimeTT( &perf.aInfo.sTime );

#ifdef ANIM
	/* traverse and update articulated model positions */
	if ( !perf.disableAnim )
	{
		AM_Evaluate( amclosedata, sdf->amdls, sdf->numAmodels, curFPS );
	}
#endif

	SampleSystemTimeTT( &perf.aInfo.eTime );

    CLT_ClearFrameBuffer(gs, .2, .2, 0.2, 1., 1, 1);
    M_DBInit(GS_Ptr(gs), 0, 0, FBWIDTH, FBHEIGHT);

	cx = cosf(xangle);
	sx = sinf(xangle);
	cy = cosf(yangle);
	sy = sinf(yangle);

	Matrix_SetTranslation( &cameramat, radius*-sy, radius*cy*sx, radius*cy*cx );

	SampleSystemTimeTT( &perf.mInfo.sTime );
	perf.scene2DTriCount = GameCode(sdf, pclosedata);
	SampleSystemTimeTT( &perf.mInfo.eTime );

    if(displayoff) {
        *(uint32 **)&gs->gs_ListPtr = pVIsave;
    } else {
	GS_SendLastList(gs);
	
	    
	/* inform the TE driver to swap buffers when it completes rendering */
	GS_EndFrame (gs);

	curWFBIndex++;
	if (curWFBIndex>=NUM_FRAME_BUFFERS) curWFBIndex = 0;
	GS_SetDestBuffer(gs, bitmaps[curWFBIndex]);
	M_DrawEnd(pclosedata);
    }

	GetControlPad(1, FALSE, &cped);
	
    ButtonsOld = ButtonsCur & ControlMask;
    ButtonsCur = cped.cped_ButtonBits;
    Buttons = ButtonsCur & ((ButtonsCur ^ ButtonsOld) | ~ControlMask);

	if( Buttons & ControlX ) {
		done = TRUE;
	}
	if( Buttons & ControlRightShift ) {
		perf.disableAnim = 1 - perf.disableAnim;
	} else if( Buttons & ControlLeftShift ) {
		 perf.printInfo = 1 - perf.printInfo;
	} else if ((Buttons & ControlUp) &&
		(Buttons & ControlB)) {
		twist  += 0.05;
	} else if ((Buttons & ControlDown) &&
		(Buttons & ControlB)) {
		twist -= 0.05;
	} else if ((Buttons & ControlUp) &&
		(Buttons & ControlA)) {
		radius += radius_inc;
	} else if ((Buttons & ControlDown) &&
		(Buttons & ControlA)) {
		radius -= radius_inc;
	} else if ((Buttons & ControlUp) &&
		(Buttons & ControlC)) {
		/* do not let the FPS fall below 1.0 */
		if( curFPS > 1.0 ) curFPS -= 0.4;
	} else if ((Buttons & ControlDown ) &&
		(Buttons & ControlC)) {
		curFPS += 0.4;
	} else {
		if (Buttons & ControlUp) {
			xangle += ANGLEINC;
		}
		if (Buttons & ControlDown) {
			xangle -= ANGLEINC;
		}
		if (Buttons & ControlLeft) {
			yangle += ANGLEINC;
		}
		if (Buttons & ControlRight) {
			yangle -= ANGLEINC;
		}
	}
	SampleSystemTimeTT( &perf.fInfo.eTime );
	if ( perf.printInfo ) AnalyzeTime( &perf );

	}

	/* Animation engine code */
	AM_End( amclosedata );

	M_End( pclosedata );

    /* Clean up */
    GS_WaitIO(gs);
	GS_FreeBitmaps(bitmaps,NUM_FRAME_BUFFERS+USE_Z);
    GS_Delete(gs);
	DeleteItem(viewItem);

	CloseGraphicsFolio ();
}

