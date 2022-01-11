/*
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
#include <math.h>
#include "mercury.h"
#include "game.h"

#include "rendermodes.h"

extern TimerTicks Realstart, Realend;
extern uint32 printtime;
extern uint32 num_tex_loads;
extern uint32 podcount, podcountmax;

static bool	usemp=0;
static uint32	vertdispatch=0;
static bool 	bufferstouse=3;
static bool	dump=0;
static bool	useprofile=0;

bool	useaa=0;
bool	useaaline=0;
bool	useapp=0;
float	apptime=0.;

#ifdef ALLOW_EXTRA_LIST
static bool	waitforbda=0;
static bool	extralist=0;
static uint32	extratris=0;
static float bdawait=0.;
#endif /* ALLOW_EXTRA_LIST */

bool   done = 0;
uint32 FBWIDTH=320;
uint32 FBHEIGHT=240;
uint32 DEPTH=16;

static uint32 odddither=0;

static float backrgb[3] = {.5, .5, .5};

uint32 usetranstexel = 0;
uint32 transtexel;

static uint32 uselowlatency=0;
static uint32 latency=1024;

/* Start of timing stuff */
uint32 automovecam = 0;
float initialcamx=0.;
float initialcamy=-400.;
float initialcamz=-600.;
float initialinc=0.;

uint32 goalfr = 0;
static uint32 goal;
static uint32 numframes=10;
static float totalfr=0.;
static uint32 framecount=10;
static uint32 startpodcount;
static uint32 podcpu[2]={0,0};
static uint32 timescalled=0;
extern uint32 clippedtris,unclippedtris;
#ifdef STATISTICS
static uint32 totalunclippedtris=0;
static uint32 totalclippedtris=0;
#endif

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

/* Start of timing stuff */
uint32 displayoff = 0;
/* End */

#define NUM_FRAME_BUFFERS	3

#define ControlMask (ControlX | ControlStart | ControlRightShift)

uint32 BestViewType(uint32 xres, uint32 yres, uint32 depth);

static GState	*gs;

static Err errorSendList(GState *g)
{
    (void)g;
    printf("Command list too small for entire scene to fit in one buffer \n");
    exit(-1);
}

float bdatime=0.;
uint32 bdanumtimes=0;
float vdlwaittime=0.;
uint32 vdlwaitnumtimes=0;

void ProfileFunc(GState *g, int32 arg)
{

    static TimerTicks bdastart, bdaend;
    static TimerTicks vdlwaitstart, vdlwaitend;
    TimerTicks delta;
    TimeVal tv;
    TOUCH(g);

    switch (arg) {
    case 1:
	/* SendIO Started */
	SampleSystemTimeTT(&bdastart);
	break;
    case -1:
	/* SendIO finished */
	SampleSystemTimeTT(&bdaend);
	SubTimerTicks(&bdastart, &bdaend, &delta);
	ConvertTimerTicksToTimeVal(&delta, &tv);
	bdatime += tv.tv_sec + tv.tv_usec * .000001;
	bdanumtimes++;
	break;
    case 2:
	/* Started waiting for vdl */
	SampleSystemTimeTT(&vdlwaitstart);
	break;
    case -2:
	/* vdl wait ends */
	SampleSystemTimeTT(&vdlwaitend);
	SubTimerTicks(&vdlwaitstart, &vdlwaitend, &delta);
	ConvertTimerTicksToTimeVal(&delta, &tv);
	vdlwaittime += tv.tv_sec + tv.tv_usec * .000001;
	vdlwaitnumtimes++;
	break;
    }
}

main(int argc, char **argv)
{
    Item	bitmaps[NUM_FRAME_BUFFERS+1];
    int32	buffSignal;
    Item	viewItem;
    uint32 	viewType;
    uint32  	bmType;

    uint32	curWFBIndex = 1;

    ControlPadEventData cped;
    uint32	Buttons,ButtonsOld, ButtonsCur=0;

    uint32	buffersize=150000; /* =600K */

    uint32	*pVIsave,displayoffthisframe;
    int		ac;
    char	**av;
    char	*cp;
    CloseData	*pclosedata;
    bool	zoff=0;
    bool	doclear=1;
    extern	uint32 lightstemplate[], numlights;
#ifdef ALLOW_EXTRA_LIST
    Item     	extraioReqItem=0;
    IOInfo   	extraioInfo;
    uint32 	*extrabuffer;
    Err 	err;
#endif /* ALLOW_EXTRA_LIST */
	
    ac = argc;
    av = argv;
    cp = *++av; /* point to first argument */

    FBWIDTH=320;
    FBHEIGHT=240;

    while ( ac >= 2 && *cp++ == '-')
	{
	    switch (*cp) {

	    case '2':
		usemp = 1;
		ac--;
		cp = *++av;
		vertdispatch = atoi(cp);
		break;
		
	    case 'a':
		if (!strcmp(cp,"aa")) {
		    useaa = 1;
		} else if (!strcmp(cp,"aaline")) {
		    useaa = 1;
		    useaaline = 1;
		} else if (!strcmp(cp,"a")) {
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
		objecttype = 1;
		ac--;
		cp = *++av;
		startpodcount = atoi(cp);
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
		    } else if (!strcmp(cp, "dst")) {
			numlights++;
			lightstemplate[nextlight++] = (uint32)&M_LightDirSpecTex;
			lightstemplate[nextlight] = (uint32)&LightDirData;
			fogspec = FS_SPEC;
		    } else if (*cp == 'd') {
			numlights++;
			lightstemplate[nextlight++] = (uint32)&M_LightDir;
			lightstemplate[nextlight] = (uint32)&LightDirData;
		    } else if (!strcmp(cp,"env")) {
			uint32 nextlight = numlights*2;
			numlights++;
			lightstemplate[nextlight++] = (uint32)&M_LightEnv;
			lightstemplate[nextlight] = 0;
			texp = TEX_ENV;
			initialcamy=1380.;
			initialcamz=1320.;
			initialinc=M_PI_2;
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

	    case 'p':
		if (!strcmp(cp,"profile")) {
		    useprofile=1;
		} else if (!strcmp(cp,"p")) {
		    printtime = 52;
		}
		break;

	    case 'q':
		dump = 1;
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

#ifdef ALLOW_EXTRA_LIST
	    case 'w':
		waitforbda=1;
		break;

	    case 'x':
		extralist=1;
		break;
#endif /* ALLOW_EXTRA_LIST */

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
		    printf("\t -2 uses 2 cpus\n");
		    printf("\t -a turns off object animation\n");
		    printf("\t -aa turns on antialiasing\n");
		    printf("\t -aaline draws silhouette lines instead of antialiasing\n");
		    printf("\t -b buffersize set the size of the command list buffers\n");
		    printf("\t -c x display cubes starting with x cubes\n");
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
		    printf("\t -lenv add an environment map light\n");
		    printf("\t -lp  add a point light\n");
		    printf("\t -ls  add a spot light\n");
		    printf("\t -o m application consumes m ms per frame\n");
		    printf("\t -p print performance statistics and quit\n");
		    printf("\t -profile use GState profile to see bda and vdl busy/wait times\n");
		    printf("\t -m[bplt] for BILINEAR POINT LINEAR or TRILINEAR texture filter modes\n");
		    printf("\t -r dyn|pre|trans|notex sets render mode\n");
		    printf("\t -s no screen clear\n");
		    printf("\t -t x  use the x PIP entry as a transparent texel in the microtexture\n");
		    printf("\t -v  i buffers to use (1,2 or 3)\n");
#ifdef ALLOW_EXTRA_LIST
		    printf("\t -w  time BDA list separately\n");
		    printf("\t -x  extra command list added\n");
#endif /* ALLOW_EXTRA_LIST */
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

	if (numlights == 0) {
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
    switch (texp) {
    case TEX_TRUE:
	printf("TEXTURED\n");
	printf("%d Texture loads per frame\n",
#ifdef ALLOW_EXTRA_LIST	       
	       extralist?3*num_tex_loads:
#endif /* ALLOW_EXTRA_LIST */
	       num_tex_loads);
	if (usetranstexel) printf("using transparent texel %d\n",transtexel);
	break;
    case TEX_ENV:
	printf("ENVIRONMENT\n");
	break;
    }
    printf("\n\n");
    if (usemp) {
	printf("Rendering on 2 cpus\nDispatching %d vertices\n",vertdispatch);
    }
    if (useaa) {
	if ((fogspec != FS_NONE) || transp) {
	    printf("Rendering mode not compatible with antialiasing\n");
	    exit(1);
	}
    }
    switch (rendermode) {
    case DYNFOGTRANSTEX:
    case DYNSPECTRANSTEX:
    case PREFOGTRANSTEX:
    case PRESPECTRANS:
    case PRESPECTRANSTEX:
    case PRESPEC:
    case PRESPECTEX:
    case DYNFOGTRANSENV:
    case DYNSPECTRANSENV:
    case PREFOGTRANSENV:
    case PRESPECTRANSENV:
    case PRESPECENV:
    case PREENV:
    case PRETRANSENV:
    case PREFOGENV:
	printf("rendering mode not supported \n");
	exit(1);
    }

    if (useaaline) {
	printf("Drawing silhouette lines\n");
    } else if (useaa) {
	printf("Antialiasing on\n");
    }
    if (objecttype == 0) {
	printf("using all objects\n");
    } else if (objecttype == 1) {
	printf("Using cubes only. Starting with %d cubes\n",startpodcount);
	if (printtime) printtime = 200;
    }
    if (usefacets) printf("using faceted object types \n");
    printf("using 2 command list buffers %d words\n",buffersize);
    if (uselowlatency) printf("using latency of %d words\n",latency);
#ifdef ALLOW_EXTRA_LIST
    if (extralist) printf("and 2 extra command lists per frame\n");
#endif /*ALLOW_EXTRA_LIST*/
    if (goalfr) {
	printf("Frame rate goal is %d\n",goal);
    }
    printf("using %d lights\n",numlights);

    if (!doclear) printf("NO screen clear\n");
    if (zoff) printf("z buffer turned off\n");
    printf("Using %d bufferes\n",bufferstouse);
    
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

    gs = GS_Create();
    gs->gs_SendList = errorSendList;

    if (useprofile) GS_EnableProfiling(gs, ProfileFunc);

    if (GS_AllocLists(gs, 2, usemp ? 2200 : buffersize) < 0) {
	printf("Can't alloc gstate command lists\n");
	exit(1);
    }

    if (bufferstouse==1) {
	curWFBIndex = 0;
    }

    GS_AllocBitmaps(bitmaps, FBWIDTH, FBHEIGHT, bmType, bufferstouse, 1);

    GS_SetDestBuffer(gs, bitmaps[curWFBIndex]);
    GS_SetZBuffer(gs, bitmaps[bufferstouse]);
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

    pclosedata = GameCodeInit(gs);

    if (objecttype == 1) {
	if (startpodcount <= podcountmax) podcount = startpodcount;
    }

    pclosedata->depth = DEPTH;

#ifdef ALLOW_EXTRA_LIST
    if (extralist) { 
	Err 		(*saveSendList)(struct GState*) = gs->gs_SendList;
	CmdListP	saveListPtr = gs->gs_ListPtr;
	uint32		extratris;
	uint32		bytes1;

	extrabuffer = (uint32 *)AllocMemAligned(2*buffersize*4, MEMTYPE_NORMAL|MEMTYPE_TRACKSIZE, 32);

	/* Change the sendlist function to make sure that sendlist never is called */
	gs->gs_SendList = errorSendList;
	initialcamx = 700;
	MoveCameraInit(pclosedata);
	if (transp || useaa) {
	    Bitmap * newDestBM = (Bitmap*)LookupItem(bitmaps[curWFBIndex]);
	    pclosedata->srcaddr = (uint32)newDestBM->bm_Buffer;
	}
	if (DEPTH==16) {
	    CLT_SetRegister(gs->gs_ListPtr, DBUSERCONTROL, CLT_Bits(DBUSERCONTROL,DITHEREN,1));
	    CLT_DbDitherMatrix(GS_Ptr(gs), 0, 0);
	}

	if (doclear) {
	    CLT_ClearFrameBuffer(gs, backrgb[0], backrgb[1], backrgb[2], 1., 1, !zoff);
	}

	M_DBInit(GS_Ptr(gs), 0, 0, FBWIDTH, FBHEIGHT);

	if (zoff) {
	    CLT_ClearRegister(gs->gs_ListPtr, DBUSERCONTROL,
			      CLT_Bits(DBUSERCONTROL,ZBUFFEN,1)|
			      CLT_Bits(DBUSERCONTROL,ZOUTEN,1));
	}

	GameCode(pclosedata);
	extratris1 = clippedtris+unclippedtris;
	bytes1 = ((char*)gs->gs_ListPtr)-((char*)saveListPtr);
	memcpy(extrabuffer, saveListPtr, bytes1);
	gs->gs_ListPtr = saveListPtr;
	initialcamx = 0;
	initialcamz = -1200;
	MoveCameraInit(pclosedata);
	GameCode(pclosedata);
	CLT_Pause (GS_Ptr(gs));		/* Put pause command at end of list */
	extratris = extratris1 + clippedtris + unclippedtris;
	memcpy(((char*)extrabuffer)+bytes1, saveListPtr, ((char*)gs->gs_ListPtr)-((char*)saveListPtr));
	gs->gs_ListPtr = saveListPtr;
	gs->gs_SendList = saveSendList;
	initialcamx = -700.;
	initialcamz = -600;
	MoveCameraInit(pclosedata);
	bzero( (void *)&extraioInfo, sizeof(IOInfo));
	extraioInfo.ioi_Command = TE_CMD_EXECUTELIST;
	extraioInfo.ioi_Send.iob_Buffer = (uchar*)extrabuffer;
	extraioInfo.ioi_Send.iob_Len = bytes1;
	extraioReqItem = CreateIOReq (NULL, 0, gs->gs_TEDev, 0);
	if (extraioReqItem<0) {
	  PrintfSysErr (extraioReqItem);
	  exit (-1);
	}
    }
#endif /* ALLOW_EXTRA_LIST */

    displayoffthisframe = 0;

    if (usemp) {
	if (M_InitMP(pclosedata, 200, buffersize, vertdispatch))
	    exit(-1);
    }

#ifdef STATISTICS
    pclosedata->numpods_fast = 0;
    pclosedata->numpods_slow = 0;
    pclosedata->numtris_fast = 0;
    pclosedata->numtris_slow = 0;
    pclosedata->numtexloads = 0;
    pclosedata->numtexbytes = 0;
#endif

    while(!done) {

	
	if (transp || useaa) {
	    /* If we are doing transparent objects we must place the address of the new FB
	       as the src address to blend with */

	    Bitmap * newDestBM = (Bitmap*)LookupItem(bitmaps[curWFBIndex]);
	    pclosedata->srcaddr = (uint32)newDestBM->bm_Buffer;
	}
	SampleSystemTimeTT(&Realstart);
	if (bufferstouse==2){
	    /* Double buffering stuff */
#ifdef ALLOW_EXTRA_LIST
	    if (extralist) {
		/* Must wait for vid signal because extra list comes first */
		WaitSignal(buffSignal);
	    } else
#endif /* ALLOW_EXTRA_LIST */
		GS_BeginFrame(gs);
	}

	if (uselowlatency)
	    GS_LowLatency(gs,1,latency);
	pVIsave = *(uint32 **)&gs->gs_ListPtr;

#ifdef ALLOW_EXTRA_LIST
	if (extralist) {
	    {
		TimerTicks BDAstart, BDAend;
		TimerTicks delta;
		TimeVal tv;

		SampleSystemTimeTT(&BDAstart); 
		/* Wait for BDA to complete the last request */
		err = WaitIO(extraioReqItem);
		if (err < 0) {
		    PrintfSysErr(err);
		    exit(-1);
		}
		SampleSystemTimeTT(&BDAend); 
		SubTimerTicks(&BDAstart, &BDAend, &delta);
		ConvertTimerTicksToTimeVal(&delta, &tv);
		bdawait += tv.tv_sec + tv.tv_usec * .000001;
	    }

	    err = SendIO(extraioReqItem, &extraioInfo);
	    if (err<0) {
		PrintfSysErr(err);
		exit(-1);
	    }
	} else 
#endif /* ALLOW_EXTRA_LIST */
	  {
	    if (DEPTH==16) {
		CLT_SetRegister(gs->gs_ListPtr, DBUSERCONTROL, CLT_Bits(DBUSERCONTROL,DITHEREN,1));
		CLT_DbDitherMatrix(GS_Ptr(gs), 0, 0);
	    }

	    if (doclear) {
		/* Don't use GS clear it's too slow */
		CLT_ClearFrameBuffer(gs, backrgb[0], backrgb[1], backrgb[2], 1., 1, !zoff);
	    }

	    M_DBInit(GS_Ptr(gs), 0, 0, FBWIDTH, FBHEIGHT);

	    if (zoff) {
		CLT_ClearRegister(gs->gs_ListPtr, DBUSERCONTROL,
				  CLT_Bits(DBUSERCONTROL,ZBUFFEN,1)|
				  CLT_Bits(DBUSERCONTROL,ZOUTEN,1));
	    }
	}
	if (DEPTH==16) {
	    CLT_SetRegister(gs->gs_ListPtr, DBUSERCONTROL, CLT_Bits(DBUSERCONTROL,DITHEREN,1));
	    if (odddither) {
		CLT_DbDitherMatrix(GS_Ptr(gs), 0xC0D12E3F, 0xE1D0302F);
	    } else {
		CLT_DbDitherMatrix(GS_Ptr(gs), ~0xC0D12E3F, ~0xE1D0302F);
	    }
	    odddither = 1-odddither;
	}

	GameCode(pclosedata);

	if(displayoffthisframe) {
	    *(uint32 **)&gs->gs_ListPtr = pVIsave;
	} else {
	    
#ifdef ALLOW_EXTRA_LIST
	    if (waitforbda && extralist) {
		int i;
		TimerTicks bdastart, bdaend;		
		TimerTicks delta;
		float time;
		TimeVal tv;
		SampleSystemTimeTT(&bdastart);
		for (i=0; i<50; i++) {

		    err = WaitIO(extraioReqItem);
		    if (err < 0) {
			PrintfSysErr(err);
			exit(-1);
		    }

		    err = SendIO(extraioReqItem, &extraioInfo);
		    if (err<0) {
			PrintfSysErr(err);
			exit(-1);
		    }
		}
		err = WaitIO(extraioReqItem);
		if (err < 0) {
		    PrintfSysErr(err);
		    exit(-1);
		}
		SampleSystemTimeTT(&bdaend);
		SubTimerTicks(&bdastart, &bdaend, &delta);
		ConvertTimerTicksToTimeVal(&delta, &tv);
		time = tv.tv_sec + tv.tv_usec * .000001;
		printf("\nBDA performance for %d extra triangles:\n",extratris);
		printf("%g triangles/sec\n",(50*extratris)/time);
		waitforbda=0;
	    }
#endif /* ALLOW_EXTRA_LIST */

	    GS_SendLastList(gs);

	    if (bufferstouse>1) {
		/* inform the TE driver to swap buffers when it completes rendering */
	        GS_EndFrame (gs);

		curWFBIndex++;
		if (curWFBIndex>=bufferstouse) curWFBIndex = 0;
		GS_SetDestBuffer(gs, bitmaps[curWFBIndex]);
	    }

	    if (usemp) {
		podcpu[0] += pclosedata->totalpods;
		podcpu[1] += pclosedata->slaveclosedata->totalpods;
	    }

	    M_DrawEnd(pclosedata);

	    if (dump) {
		int notdone=1;

		printf("Waiting for dump of command list\n");
		while (notdone) {
		    GetControlPad(1, FALSE, &cped);
		    ButtonsOld = ButtonsCur & ControlMask;
		    ButtonsCur = cped.cped_ButtonBits;
		    Buttons = ButtonsCur & ((ButtonsCur ^ ButtonsOld) | ~ControlMask);
		    if( Buttons & ControlX ) {
			notdone = 0;
		    }
		}
		dump = 0;
	    }
	}

	displayoffthisframe = displayoff;

	GetControlPad(1, FALSE, &cped);
	{
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
		initialcamx=0.;
		initialcamy=-400.;
		initialcamz=-600.;
		initialinc=0.;
		pclosedata->fcamx = initialcamx;
		pclosedata->fcamy = initialcamy;
		pclosedata->fcamz = initialcamz;
		cameraincline = initialinc;
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
		initialinc += 0.05;
		MoveCameraInit(pclosedata);
	    } else if ((Buttons & ControlDown) &&
		(Buttons & ControlB)) {
		initialinc -= 0.05;
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

	SampleSystemTimeTT(&Realend);

	timescalled++;

	if (usemp) {
	    if (timescalled == 50) {
		if (printtime==1 || printtime==2) {
		    printf("Ave. pods for master CPU %d\n",podcpu[0]/timescalled);
		    printf("Ave. pods for slave CPU %d\n",podcpu[1]/timescalled);
		}
		podcpu[0] = 0;
		podcpu[1] = 0;
	    }
	}
#ifdef STATISTICS
	totalunclippedtris += unclippedtris;
	totalclippedtris += clippedtris;
	if (timescalled == 50) {
	    if (printtime==1 || printtime==2) {
		printf("Statistics averages:\n");
		printf("Number of unclipped pods %d\n",pclosedata->numpods_fast/50);
		printf("Number of clipped pods %d\n",pclosedata->numpods_slow/50);
		printf("Number of unclipped triangles %d\n",pclosedata->numtris_fast/50);
		printf("Number of triangles drawn in clipped pods %d\n",pclosedata->numtris_slow/50);
		if ((totalunclippedtris+totalclippedtris) != (pclosedata->numtris_fast+pclosedata->numtris_slow)) {
		    printf("Clipped and un-clipped triangles don't agree\n");
		}
		printf("Number of texture loads %d\n",pclosedata->numtexloads/50);
		printf("Number of texture bytes loaded %d\n",pclosedata->numtexbytes/50);
	    }

	    pclosedata->numpods_fast = 0;
	    pclosedata->numpods_slow = 0;
	    pclosedata->numtris_fast = 0;
	    pclosedata->numtris_slow = 0;
	    pclosedata->numtexloads = 0;
	    pclosedata->numtexbytes = 0;
	    totalunclippedtris = 0;
	    totalclippedtris = 0;

	}
#endif /*STATISTICS*/

	if (timescalled == 50){
	    timescalled = 0;
	}

	if (printtime==1 || printtime==2)
	    AnalyzeTime();

	if (printtime>2 && (objecttype != 1)) {
	    printtime--;
#ifdef ALLOW_EXTRA_LIST
	    bdawait=0.;
#endif /* ALLOW_EXTRA_LIST */
	} else {
	    float fr;
	    fr = AnalyzeRealTime();
	    if (goalfr) {
		if (--framecount == 0) {
		    fr = (fr + totalfr)/numframes;
		    if (fr < goal) {
			if ((goal-fr) < 4.) {
			    numframes=50;
			}
			if (podcount > 1)
			    podcount--;
		    } else {
			if (podcount < podcountmax)
			    podcount++;
		    }
		    totalfr = 0.;
		    framecount = numframes;
		} else {
		    totalfr += fr;
		}
	    }
	    if (printtime > 2) {
		printtime--;
	    }
	}

    }
    /* Clean up */
    GS_WaitIO(gs);
    if (useprofile) GS_DisableProfiling(gs);
    GS_FreeBitmaps(bitmaps,bufferstouse+1);
    GS_Delete(gs);
    DeleteItem(viewItem);
    M_End(pclosedata);
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



