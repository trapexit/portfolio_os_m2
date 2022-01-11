/*
 * @(#) mainloop.c 96/10/02 1.80
 *
 *	bigcircle, a test for the mercury pipeline
 *
 */
#include <stdio.h>
#include <stdlib.h>
#ifdef MACINTOSH
#include <graphics:view.h>
#include <kernel:io.h>
#include <kernel:time.h>
#include <kernel:mem.h>
#include <kernel:task.h>
#include <misc:event.h>
#else
#include <graphics/view.h>
#include <kernel/io.h>
#include <kernel/time.h>
#include <kernel/mem.h>
#include <kernel/task.h>
#include <misc/event.h>
#endif
#include <assert.h>
#include <strings.h>
#include "mercury.h"
#include "mpm.h"
#include "game.h"

#include <device/mp.h>
#include <kernel/cache.h>

#include "rendermodes.h"

extern TimerTicks Realstart, Realend;
extern uint32 printtime;
extern uint32 num_tex_loads;
extern uint32 podcountmax;
static Item	viewItem;

bool 		bufferstouse=3;
bool		waitforbda=0;
bool		dump=0;
bool		useapp=0;
float		apptime=0.;
bool   		done = 0;

uint32 FBWIDTH=320;
uint32 FBHEIGHT=240;
uint32 DEPTH=16;

uint32 usetranstexel = 0;
uint32 transtexel;
uint32 uselowlatency=0;
uint32 latency=4096;

Report rave[600]; 

/* Start of timing stuff */
uint32 automovecam = 0;
float initialcamx=0.;
float initialcamy=-400.;
float initialcamz=-600.;

uint32 goalfr = 0;
uint32 goal;

/* = 0 for all data
   = 1 for cubes */
uint32 objecttype = 0;
uint32 usefacets = 0;
uint32 rendermode;
uint32 dynpre=LIT_DYN;
uint32 fogspec=FS_NONE;
uint32 transp=TRANS_NONE;
uint32 texp=TEX_TRUE;
uint32 filtermode=POINT;

uint32 displayoff = 0;

uint32 doantialias = 0;

#define	USE_Z			1
#define NUM_FRAME_BUFFERS	3

static Item	bitmaps[NUM_FRAME_BUFFERS+USE_Z];

#define ControlMask (ControlX | ControlStart | ControlRightShift)

uint32 BestViewType(uint32 xres, uint32 yres, uint32 depth);

uint32 Render(uint32 bufferstouse, uint32 depth, uint32 doclear, uint32 zoff, uint32 *pcurWFBIndex,
       MpmContext *mc)
{
    uint32 ret;
    uint32 curWFBIndex = *pcurWFBIndex;

    SampleSystemTimeTT(&Realstart);

    if (uselowlatency) GS_LowLatency(mc->gs[0],1,latency);

    if (bufferstouse==1) {
	/* Only done for double buffering */
	GS_BeginFrame(mc->gs[0]);
	/* Don't BeginFrame for gs[1] because only gs[0] is the first list after the
	   vidSignal */
    }


    if (depth==16) {
	CLT_SetRegister(mc->gs[0]->gs_ListPtr, DBUSERCONTROL, CLT_Bits(DBUSERCONTROL,DITHEREN,1));
	CLT_DbDitherMatrix(GS_Ptr(mc->gs[0]), 0, 0);
    }

    if (doclear) {
	CLT_ClearFrameBuffer(mc->gs[0], .5, .5, .5, 1., 1, !zoff);
    }

    M_DBInit(GS_Ptr(mc->gs[0]), 0, 0, FBWIDTH, FBHEIGHT);

    if (zoff) {
	CLT_ClearRegister(mc->gs[0]->gs_ListPtr, DBUSERCONTROL,
			  CLT_Bits(DBUSERCONTROL,ZBUFFEN,1)|
			  CLT_Bits(DBUSERCONTROL,ZOUTEN,1));
    }

    if (depth==16) {
	static uint32 odddither=0;

	CLT_SetRegister(mc->gs[0]->gs_ListPtr, DBUSERCONTROL, CLT_Bits(DBUSERCONTROL,DITHEREN,1));
	if (odddither) {
	    CLT_DbDitherMatrix(GS_Ptr(mc->gs[0]), 0xC0D12E3F, 0xE1D0302F);
	} else {
	    CLT_DbDitherMatrix(GS_Ptr(mc->gs[0]), ~0xC0D12E3F, ~0xE1D0302F);
	}
	odddither = 1-odddither;
    }

    GameCode(mc);

    MPM_LinkSortSplit(mc);

    if (uselowlatency) {
	ret = MPM_DrawSendSwapLL(mc, bitmaps[curWFBIndex]);
    } else {
	ret = MPM_DrawSendSwap(mc, bitmaps[curWFBIndex]);
    }
    ret = ret & 0xffff;

    SampleSystemTimeTT(&Realend);

    MPM_NewFrame(mc);

    if (bufferstouse>0) {
	curWFBIndex++;
	if (curWFBIndex > bufferstouse) curWFBIndex = 0;
	*pcurWFBIndex = curWFBIndex;

    }

   return ret;
}

main(int argc, char **argv)
{
    int32	buffSignal;
    uint32 	viewType;
    uint32  	bmType;

    uint32	curWFBIndex = 1;

    ControlPadEventData cped;
    uint32	Buttons,ButtonsOld, ButtonsCur=0;

    uint32	buffersize=150000; /* =600K */

    int		ac;
    char	**av;
    char	*cp;
    bool	zoff=0;
    bool	doclear=1;
    extern	uint32 lightstemplate[], numlights;
    Err 	err;

    uint32 	i;
    Report 	r;
    uint32 	maxi=0;
    uint32	automeasure=1;
    uint32	cpu0podcount=0;
    uint32 	tcount;

    MpmContext	mc;

    ac = argc;
    av = argv;
    cp = *++av; /* point to first argument */

    FBWIDTH=320;
    FBHEIGHT=240;

    MPM_Init(&mc);

    mc.mpioreq = CreateMPIOReq(0);
    if (mc.mpioreq<0) {
      mc.mpioreq = CreateMPIOReq(1);
      if (mc.mpioreq < 0) {
	printf("Can't create MP item: \n");
	PrintfSysErr(mc.mpioreq);
	exit(-1);
      }
    }

    while ( ac >= 2 && *cp++ == '-')
	{
	    switch (*cp) {

	    case 'a':
		{
		    extern uint32 moveobjects;
		    moveobjects=0;
		}
		break;

	    case 'b':
		ac--;
		cp = *++av;
		buffersize = atoi(cp);
		break;

	    case 'c':
		ac--;
		cp = *++av;
		podcountmax = atoi(cp);
		objecttype = 1;
		break;

	    case 'd':
		ac--;
		cp = *++av;
		DEPTH = atoi(cp);
		break;
		
	    case 'e':
		uselowlatency=1;
		ac--;
		cp = *++av;
		latency = atoi(cp);
		break;

	    case 'f':
		usefacets = 1;
		break;

	    case 'g':
		goalfr = 1;
		ac--;
		cp = *++av;
		goal = atoi(cp);
		break;
		
	    case 'h':
		if (*++cp == 'x') {
		    FBWIDTH=640;
		} else if (*cp == 'y') {
		    FBHEIGHT=480;
		}
		break;

	    case 'i':
		ac--;
		cp = *++av;
		initialcamy = -atoi(cp);
		break;

	    case 'l':
		{
		    uint32 nextlight = numlights*2;
		    extern float *LightFogData;
		    extern float *LightDirData;
		    extern float *LightPointData;
		    extern float *LightSoftSpotData;
		    if (*++cp == 'f') {
			numlights++;
			fogspec = FS_FOG;
			lightstemplate[nextlight++] = (uint32)&M_LightFog;
			lightstemplate[nextlight] = (uint32)&LightFogData;
		    } else if (!strcmp(cp, "ds")) {
			numlights++;
			lightstemplate[nextlight++] = (uint32)&M_LightDirSpec;
			lightstemplate[nextlight] = (uint32)&LightDirData;
			fogspec = FS_SPEC;
		    } else if (*cp == 'd') {
			numlights++;
			lightstemplate[nextlight++] = (uint32)&M_LightDir;
			lightstemplate[nextlight] = (uint32)&LightDirData;
		    } else if (*cp == 'p') {
			numlights++;
			lightstemplate[nextlight++] = (uint32)&M_LightPoint;
			lightstemplate[nextlight] = (uint32)&LightPointData;
		    } else if (*cp == 's') {
			numlights++;
			lightstemplate[nextlight++] = (uint32)&M_LightSoftSpot;
			lightstemplate[nextlight] = (uint32)&LightSoftSpotData;
		    }
		    break;
		}

	    case 'm':
		if (*++cp == 'p') {
		    filtermode = POINT;
		} else if (*cp == 'b') {
		    filtermode = BILINEAR;
		} else if (*cp == 'l') {
		    filtermode = LINEAR;
		} else if (*cp == 't') {
		    filtermode = TRILINEAR;
		}
		break;

	    case 'r':
		cp++;
		if (!strcmp(cp,"dyn")) {
		    dynpre = LIT_DYN;
		} else if (!strcmp(cp,"pre")) {
		    dynpre = LIT_PRE;
		} else if (!strcmp(cp,"notex")) {
		    texp = TEX_NONE;
		} else if (!strcmp(cp,"trans")) {
		    transp = TRANS_TRUE;
		}

		break;

	    case 'o':
		ac--;
		cp = *++av;
		useapp=1;
		apptime = atoi(cp);
		break;

	    case 'q':
		ac--;
		cp = *++av;
		automeasure=0;
		cpu0podcount = atoi(cp);
		break;

	    case 'p':
		printtime = 52;
		break;

	    case 's':
		doclear=0;
		break;

	    case 't':
		ac--;
		cp = *++av;
		usetranstexel = 1;
		transtexel = atoi(cp);
		break;

	    case 'v':
		ac--;
		cp = *++av;
		bufferstouse = atoi(cp);
		if (bufferstouse<1) bufferstouse=1;
		if (bufferstouse>3) bufferstouse=3;
		break;

	    case 'w':
		waitforbda=1;
		break;


	    case 'y':
		ac--;
		cp = *++av;
		num_tex_loads = atoi(cp);
		if (num_tex_loads<3) num_tex_loads=3;
		break;
		
	    case 'z':
		zoff = 1;
		break;

	    default:
		{
		    printf("\t -a turns off object animation\n");
		    printf("\t -b buffersize set the size of the command list buffers\n");
		    printf("\t -c n display n cubes\n");
		    printf("\t -d 16|32  display depth\n");
		    printf("\t -e n  Use low latency gstate mode with latency of n words\n");
		    printf("\t -f renders faceted icos data\n");
		    printf("\t -g xx ensure xx frames/sec\n");
		    printf("\t -hx  width is 640\n");
		    printf("\t -hy  height is 480\n");
		    printf("\t -i xxx initial camera distance xxx\n");
		    printf("\t -lf  add a fog light\n");
		    printf("\t -ld  add a directional light\n");
		    printf("\t -lds add a specular directional light\n");
		    printf("\t -lp  add a point light\n");
		    printf("\t -ls  add a spot light\n");
		    printf("\t -m[bplt] for BILINEAR POINT LINEAR or TRILINEAR texture filter modes\n");
		    printf("\t -o m application consumes m ms per frame\n");
		    printf("\t -p print performance statistics and quit\n");
		    printf("\t -q p pods are sent to CPU0 turns off auto measurement\n");
		    printf("\t -r dyn|pre|trans|notex sets render mode\n");
		    printf("\t -s no screen clear\n");
		    printf("\t -t x  use the x PIP entry as a transparent texel in the microtexture\n");
		    printf("\t -v  i buffers to use (1,2 or 3)\n");
		    printf("\t -w  time BDA list separately\n");
		    printf("\t -y n do n texture loads\n");
		    printf("\t -z no z clear and no z buffering\n");
		    printf("Controls are:\n");
		    printf("Up, Down  Moves camera up and down\n");
		    printf("Left, Right Moves camera to left and right\n");
		    printf("Right Shift prints performance info every 50 frames\n");
		    printf("A + Left Shift Resets camera to original settings\n");
		    printf("A + Up, Down Moves camera in and out\n");
		    printf("B + Up, Down Tilts camera up and down\n");
		    printf("Left Shift + Up, Down, Left, Right changes the perspective\n");
		    printf("C + Up, Down changes the hither plane\n");
		    printf("Start/Pause  Starts and stops object motion\n");
		    printf("Stop Exits program\n");
		    printf("C  Turns fog on/off\n");
		    printf("\n");
		    exit(1);
		}
	    }
	    ac--; /* force one less argument */
	    cp = *++av; /* next argument */
	}

    {
	extern 	uint32 defaultlightstemplate[], numdefaultlights;

	uint32 i,j;

	if (numlights == 0 ) {
	    fogspec = FS_FOG;
	    numlights = numdefaultlights;
	    /* Copy the lights that were selected into the template */
	    for (i=0,j=0;i<numlights; i++,j++) {
		lightstemplate[j]=defaultlightstemplate[j];
		j++;
		lightstemplate[j]=defaultlightstemplate[j];
	    }
	    lightstemplate[j]=0;
	}
    
    }

    printf("Rendering on %d x %d at %d bits per pixel\n\n",
	   FBWIDTH,FBHEIGHT,DEPTH);
    rendermode = CASECODE(dynpre, fogspec, transp, texp);
    printf("Rendering mode %s LIT ",dynpre? "PRE":"DYN");
    switch (fogspec) {
    case FS_NONE:
	printf("NO FOG ");
	break;
    case FS_FOG:
	printf("FOG ");
	break;
    case FS_SPEC:
	printf("SPEC ");
	break;
    };
    if (transp) printf("TRANSPARENT ");
    if (texp) {
	printf("TEXTURED\n");
	printf("%d Texture loads per frame\n",num_tex_loads);
    }
    printf("\n\n");
    if (usetranstexel) printf("using transparent texel %d\n",transtexel);

    switch (rendermode) {
    case DYNFOGTRANSTEX:
    case DYNSPECTRANSTEX:
    case PREFOGTRANSTEX:
    case PRESPECTRANS:
    case PRESPECTRANSTEX:
    case PRESPEC:
    case PRESPECTEX:
	printf("rendering mode not supported \n");
	exit(1);
    }

    if (objecttype == 0) {
	printf("using all objects\n");
    } else if (objecttype == 1) {
	printf("using cubes only\n");
    }
    if (usefacets) printf("using faceted object types \n");
    printf("using 2 command list buffers %d words\n",buffersize);
    if (uselowlatency) printf("using latency of %d words\n",latency);
    if (goalfr) {
	printf("Frame rate goal is %d\n",goal);
    }
    printf("using %d lights\n",numlights);

    if (!doclear) printf("NO screen clear\n");
    if (zoff) printf("z buffer turned off\n");
    printf("Using %d bufferes\n",bufferstouse);
    bufferstouse--; /* Now in range 0,1,2 */
    
    if (texp) {
	switch (filtermode) {
	case POINT:
	    printf("POINT");
	    break;
	case BILINEAR:
	    printf("BILINEAR");
	    break;
	case LINEAR:
	    printf("LINEAR");
	    break;
	case TRILINEAR:
	    printf("TRILINEAR");
	    break;
	} 
	printf(" texture filter mode\n");
    }

    if (useapp) {
	printf("%f ms of CPU0 used for game\n",apptime);
	apptime *= .001;
    }

    bmType = (DEPTH==16 ? BMTYPE_16 : BMTYPE_32);

    OpenGraphicsFolio ();

    InitEventUtility(1, 0, 0);

    buffSignal = AllocSignal(0);

    mc.gs[0] = GS_Create();
    mc.slavedata.latency = latency;

    if (err = GS_AllocBitmaps(bitmaps, FBWIDTH, FBHEIGHT, bmType, bufferstouse+1, 1)) {
	PrintfSysErr(err);
	exit(1);
    }
    if (bufferstouse==0) {
	curWFBIndex = 0;
    }

    if (GS_AllocLists(mc.gs[0], 2, buffersize) < 0) {
	printf("Can't alloc gstate command lists\n");
	exit(1);
    }

    GS_SetZBuffer(mc.gs[0], bitmaps[bufferstouse+1]);
    GS_SetDestBuffer(mc.gs[0], bitmaps[curWFBIndex]);

    /* Only gs[0] has to wait for signal */
    GS_SetVidSignal(mc.gs[0], buffSignal);
    viewType = BestViewType(FBWIDTH, FBHEIGHT, DEPTH);
    viewItem = CreateItemVA(MKNODEID(NST_GRAPHICS, GFX_VIEW_NODE),
			    VIEWTAG_VIEWTYPE, viewType,
			    VIEWTAG_RENDERSIGNAL, buffSignal,
			    VIEWTAG_BITMAP, bitmaps[0],
			    TAG_END );
    if (viewItem < 0) { PrintfSysErr(viewItem); exit(1); }
    AddViewToViewList( viewItem, 0 );
    GS_SetView (mc.gs[0], viewItem);

    mc.gs[1] = GS_Clone(mc.gs[0]);

    GameCodeInit(&mc);

    mc.pclosedata[0]->depth = DEPTH;
    if (transp) {
	/* If we are doing transparent objects we must place the address of the new FB
	   as the src address to blend with */

	Bitmap * newDestBM = (Bitmap*)LookupItem(bitmaps[curWFBIndex]);
	mc.pclosedata[0]->srcaddr = (uint32)newDestBM->bm_Buffer;
    }

    if (automeasure) {
	uint32 j,k;
	float maxfps=0;
	uint32 numtimes;

	printf("Measuring performance profile....\n");
	printf("CPU0 pods  frames/sec  BDAtriangles/sec scenetriangles/sec\n");
	for (i=0; i<podcountmax; i++) {
	    /* Analyze each configuration w/ 20 frames and take
	       the average */
	    rave[i].podcount0 = i;
	    r.fps = 0;
	    r.tps_screen = 0;
	    r.tps_scene = 0;
	    numtimes=0;
	    MPM_Setpodcount0(&mc,i);

	    for (j=0; j<5; j++) {
		for (k=0; k<2; k++) {
		    tcount = Render(bufferstouse, DEPTH, doclear, zoff, &curWFBIndex, &mc);

		    if (AnalyzeRealTime(&r,podcountmax,tcount)) {
			rave[i].fps += r.fps;
			rave[i].tps_screen += r.tps_screen;
			rave[i].tps_scene += r.tps_scene;
			numtimes++;
		    }
		}
	    }
	    rave[i].fps /= numtimes;
	    rave[i].tps_screen /= numtimes;
	    rave[i].tps_scene /= numtimes;
	    if (maxfps<rave[i].fps) {
		maxfps = rave[i].fps;
		maxi = i;
	    }
	    printf("   %d      %g  %g  %g\n",rave[i].podcount0,rave[i].fps,
		   rave[i].tps_screen,rave[i].tps_scene);
	}

	/* Choose best configuration */

	printf("Best performance is :\n");
	printf("   %d      %g  %g  %g\n",rave[maxi].podcount0,rave[maxi].fps,
	       rave[maxi].tps_screen,rave[maxi].tps_scene);
	MPM_Setpodcount0(&mc,rave[maxi].podcount0);
    } else {
	MPM_Setpodcount0(&mc,cpu0podcount);
    }

    while(!done) {
	
	tcount = Render(bufferstouse, DEPTH, doclear, zoff, &curWFBIndex, &mc);

	if (printtime>2) {
	    printtime--;
	} else {
	    if (AnalyzeRealTime(&r,MPM_Getpodcount(&mc),tcount)) {
		if (goalfr) {
		    float frame_error = goal-r.fps;
		    if (frame_error > .1) {
			if (mc.podcount > 1) {
			    mc.podcount--;
			    mc.pm[0].needsort=1;
			    mc.pm[1].needsort=1;
			} else if (frame_error < .1) {
			    if (mc.podcount < podcountmax) {
				mc.podcount++;
				mc.pm[0].needsort=1;
				mc.pm[1].needsort=1;
			    }
			}
		    }
		    if (mc.pm[0].podcount0 > mc.podcount) {
			mc.pm[0].podcount0 = mc.podcount;
		    }
		    if (mc.pm[1].podcount0 > mc.podcount) {
			mc.pm[1].podcount0 = mc.podcount;
		    }
		}
	    }
	}
	
	GetControlPad(1, FALSE, &cped);
	{
	    CloseData *pclosedata = mc.pclosedata[0];
	    extern float cameraincline;
	    extern float hither, vp_hither, left, right;
	    ButtonsOld = ButtonsCur & ControlMask;
	    ButtonsCur = cped.cped_ButtonBits;
	    Buttons = ButtonsCur & ((ButtonsCur ^ ButtonsOld) | ~ControlMask);

	    if( Buttons & ControlX ) {
		printtime = 2;
	    } else if( Buttons & ControlStart ) {
		moveobjects = 1-moveobjects;
	    } else if ((Buttons & ControlUp) &&
		       (Buttons & ControlC)) {
		hither *= 2.;
		printf("hither == %g\n",hither);
		MoveCameraInit(pclosedata);
	    } else if ((Buttons & ControlDown) &&
		       (Buttons & ControlC)) {
		hither /= 2.;
		if (hither < .0001) {
		    hither = .0001;
		}
		printf("hither == %g\n",hither);
		MoveCameraInit(pclosedata);
	    } else if ((Buttons & ControlUp) &&
		       (Buttons & ControlLeftShift)) {
		vp_hither += .01;
		printf("vp_hither == %g\n",vp_hither);
		MoveCameraInit(pclosedata);
	    } else if ((Buttons & ControlDown) &&
		       (Buttons & ControlLeftShift)) {
		vp_hither -= .01;
		if (vp_hither < .0001) {
		    vp_hither = .0001;
		}
		printf("vp_hither == %g\n",vp_hither);
		MoveCameraInit(pclosedata);
	    } else if ((Buttons & ControlRight) &&
		       (Buttons & ControlLeftShift)) {
		left += .01;
		right += .01;
		printf("left == %g\n",left);
		MoveCameraInit(pclosedata);
	    } else if ((Buttons & ControlLeft) &&
		       (Buttons & ControlLeftShift)) {
		left -= .01;
		right -= .01;
		printf("left == %g\n",left);
		MoveCameraInit(pclosedata);
	    } else if((Buttons & ControlLeftShift) && (Buttons & ControlA) ) {
		automovecam = 0;
		pclosedata->fcamx = initialcamx;
		pclosedata->fcamy = initialcamy;
		pclosedata->fcamz = initialcamz;
		cameraincline = 0.;
		hither = 256.;
		vp_hither = 1.2;
		left = -1.;
		right = 1.;
		MoveCameraInit(pclosedata);
	    } else if ((Buttons & ControlUp) &&
		(Buttons & ControlA)) {
		pclosedata->fcamy += 20;
	    } else if ((Buttons & ControlDown) &&
		(Buttons & ControlA)) {
		pclosedata->fcamy -= 20;
	    } else if ((Buttons & ControlUp) &&
		(Buttons & ControlB)) {
		cameraincline += 0.05;
		MoveCameraInit(pclosedata);
	    } else if ((Buttons & ControlDown) &&
		(Buttons & ControlB)) {
		cameraincline -= 0.05;
		MoveCameraInit(pclosedata);
	    } else if ((Buttons & ControlC) && (Buttons & ControlA)) {
		extern float fogfar, fogspeed, fogfarmax;
		extern uint32 fogging;
		if (fogging) {
		    fogfar = 100000.;
		    fogspeed=0.;
		} else {
		    fogfar = fogfarmax;
		    fogspeed=10.;
		}
		fogging = 1-fogging;
	    } else if (Buttons & ControlUp) {
		pclosedata->fcamz -= 20;
	    } else if (Buttons & ControlDown) {
		pclosedata->fcamz += 20;
	    } else if (Buttons & ControlLeft) {
		pclosedata->fcamx += 20;
	    } else if (Buttons & ControlRight) {
		pclosedata->fcamx -= 20;
	    } else if (Buttons & ControlRightShift) {
		printtime = 1-printtime;
	    }
	}

    }
    /* Clean up */
    GS_WaitIO(mc.gs[0]);
    GS_WaitIO(mc.gs[1]);
    GS_FreeBitmaps(bitmaps,bufferstouse+USE_Z);
    GS_Delete(mc.gs[0]);
    GS_Delete(mc.gs[1]);
    DeleteItem(viewItem);
    DeleteMPIOReq(mc.mpioreq);

    CloseGraphicsFolio ();
}

uint32 BestViewType(uint32 xres, uint32 yres, uint32 depth)
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
