#include <stdio.h>
#include <stdlib.h>
#ifdef MACINTOSH
#include <kernel:mem.h>
#include <graphics:view.h>
#else
#include <kernel/mem.h>
#include <graphics/view.h>
#endif
#include <math.h>
#include <string.h>
#include "rendermodes.h"

#include "mercury.h"
#include "game.h"
#include "matrix.h"
#include "lighting.h"
#include "bsdf_read.h"

/*	Start stuff for timing */
#ifdef MACINTOSH
#include <kernel:time.h>
#else
#include <hardware/PPCasm.h>
#include <kernel/time.h>
#endif

extern uint32 FBWIDTH;
extern uint32 FBHEIGHT;

extern uint32 transp;
extern bool useaa, useaaline;
extern bool useapp;
extern float apptime;

#ifdef ALLOW_EXTRA_LIST
extern uint32 extralist;
#endif /* ALLOW_EXTRA_LIST */

extern void IcacheFlush(void);
/* Change to 1 to print stuff */
uint32 printtime=0;


extern uint32 objecttype, usefacets;

extern void DumpState(void);
int whichobj=0; /* ==0 for all objects,
		   ==1 for plane only,
		   ==2 for icos only
		   ==3 for dunuts only */

uint32 podcount, podcountmax;
uint32 lastpodcount=0;

uint32 interruptsoff = 0;
extern uint32 displayoff;

extern uint32 goalfr;

uint32 moveobjects=1;
uint32 num_tex_loads=32;

#define NUM_PLANE 	40
#define NUM_ICOS 	60
#define NUM_DONUT 	53
#define NUM_CUBE 	600

static float *prelitplanevtx, *preliticosvtx, *prelitdonutvtx;

extern CltSnippet ConstBrightnessSnippet;

void concatenate(float *psrca,float *psrcb,float *presult);

Pod *ppodbuffer;
PodGeometry *geobuffer;
Pod *firstpod;
float *pcostable;
float *psintable;

Matrix frustrumviewmat;

float view_wclose = 1.01;
float view_wfar = 100000000.0;

void MoveObjects(void);
void MoveObjectsInit(void);
void InitializePodTextures(void);

Matrix pointlightmat = 
{
	1.0, 0.0, 0.0,
	0.0, 1.0, 0.0,
	0.0, 0.0, 1.0,
	0.0, 1250.0, -300.0
};
	
LightDir LightDirData = 
{
	-0.866,	0.0,	-0.5,
	1.0,	1.0,	1.0,
};
LightPoint LightPointData = 
{
	-0.866,	0.0,	-0.5,
	250000.0 * 32.0 * 1.0,
	250000.0,
	1.0,	1.0,	1.0,
};
LightSoftSpot LightSoftSpotData = /* 30 to 60 degrees */
{
	-0.866,	0.0,	-0.5,
	0.0,	0.0,	-1.0,
	500000.0 * 32.0 * 1.0,
	500000.0,
	0.9397,				/* cos(20 degrees) */
	1.0/(.9848-.9397),			/* 1.0/(cos(10 degrees)-cos(20 degrees)) */
	1.0,	1.0,	1.0,
};
/*		fog0 = 1.0/(fogfar - fognear) 		*/
/*		fog1 = fognear/(fogfar - fognear)	*/
LightFog LightFogData = 
{
	1.0/(30.0 - 10.0),	10.0/(30.0 - 10.0)	/* fognear 10.0, fogfar 30.0 */
};
	
Material OpaqueMaterial = 
{
	0.05,	0.05,	0.05,	1.0,	/* base color 		*/
	0.8,	0.8,	0.8,		/* diffuse color	*/
	.5,				/* shine		*/
	1.0,	1.0,	.3,		/* specular color	*/
};

Material TransMaterial = 
{
	0.05,	0.05,	0.05,	.5,	/* base color 		*/
	.8,	0.8,	0.8,		/* diffuse color	*/
	.5,				/* shine		*/
	1.0,	1.0,	.3,		/* specular color	*/
};

uint32 lightstemplate[22];

uint32 numdefaultlights=3;
uint32 defaultlightstemplate[] = 
{
	(uint32)&M_LightFog,
	(uint32)&LightFogData,
	(uint32)&M_LightDir,
	(uint32)&LightDirData,
	(uint32)&M_LightPoint,
	(uint32)&LightPointData,
	(uint32)&M_LightSoftSpot,
	(uint32)&LightSoftSpotData,
	0
};

uint32 numlights=0;
uint32 translights[22];
uint32 lights[22];

float objectangle = 0.0;
float objectvelo = 0.02;
float lightx,lighty,lightz;

extern PodGeometry icos0;
extern PodGeometry icos1;
extern PodGeometry icos2;
extern PodGeometry icos3;
extern PodGeometry icos4;
extern PodGeometry miniicos;
extern PodGeometry faceticos0;
extern PodGeometry faceticos1;
extern PodGeometry faceticos2;
extern PodGeometry faceticos3;
extern PodGeometry faceticos4;
extern PodGeometry plane;
extern PodGeometry donut0;
extern PodGeometry donut1;
extern PodGeometry donut2;
extern PodGeometry donut3;
extern PodGeometry donut4;
extern PodGeometry cube;
extern PodGeometry facetcube;

PodGeometry *faceticostab[5] = 
{
	&faceticos0,&faceticos1,&faceticos2,&faceticos3,&faceticos4
};

PodGeometry *icostab[5] = 
{
	&icos0,&icos1,&icos2,&icos3,&icos4
};

PodGeometry addtlicosdata[32];

PodGeometry *donuttab[5] = 
{
	&donut0,&donut1,&donut2,&donut3,&donut4
};
PodGeometry addtldonutdata[32];

extern Matrix planemats[40];
Matrix icosmats[60];
Matrix donutmats[53];
Matrix cubemats[600];

static void LightsInit(void)
{
    long i,j;
    for (i=0,j=0;i<numlights; i++,j++) {
	lights[j]=lightstemplate[j];
	translights[j] = lightstemplate[j];
	if (transp && (lightstemplate[j] == (uint32)M_LightFog))
	    translights[j] = (uint32)M_LightFogTrans;
	j++;
	lights[j]=lightstemplate[j];
	translights[j]=lightstemplate[j];
    }
    lights[j]=0;
    translights[j]=0;
}

CloseData *GameCodeInit(GState* gs)
{
    CloseData 	*pclosedata;

    if (objecttype == 0) {
	podcountmax = podcount = 154;	
    } else {
	podcountmax = podcount = 600;
    }
    ppodbuffer = (Pod *)AllocMemAligned(podcount*sizeof(Pod), MEMTYPE_NORMAL, 32);
    geobuffer = (PodGeometry *)AllocMemAligned(podcount*sizeof(PodGeometry), MEMTYPE_NORMAL, 32);
    if (!ppodbuffer || !geobuffer) {
	    printf("Out of memory can't create pods and geos\n");
	    exit(1);
    }

    if (dynpre == LIT_PRE) {
	
	prelitplanevtx = (float *)AllocMem(NUM_PLANE*plane.vertexcount*6*sizeof(float), MEMTYPE_NORMAL);
	preliticosvtx = (float *)AllocMem(NUM_ICOS*(faceticos0.vertexcount*6*sizeof(float)+
						     faceticos0.sharedcount*2*sizeof(short)),
					   MEMTYPE_NORMAL);
	prelitdonutvtx = (float *)AllocMem(NUM_DONUT*donut0.vertexcount*6*sizeof(float), MEMTYPE_NORMAL);
	if (!prelitplanevtx  || !preliticosvtx || !prelitdonutvtx) {
	    printf("Out of memory can't create pre-lit vertices\n");
	    exit(1);
	}
    }

    pcostable = (float *)AllocMem(1024 * sizeof(float), MEMTYPE_ANY);
    psintable = (float *)AllocMem(1024 * sizeof(float), MEMTYPE_ANY);

    if(!ppodbuffer || !pcostable || !psintable)
	{
	    printf("Out of memory in InitializeGameCode\n");
	    exit(1);
	}

    {
	uint32 i;
	for(i = 0; i < 1024; i++)
	    {
		*(pcostable + i) = cosf((PI/512) * (float)i);
		*(psintable + i) = sinf((PI/512) * (float)i);
	    }
    }

    pclosedata = M_Init(200, 0, gs);
    pclosedata->fogcolor = M_PackColor(.5, .5, .5);

    InitializePodTextures();
    MoveCameraInit(pclosedata);
    MoveObjectsInit();
    MoveObjects();
    firstpod = M_Sort(podcount,ppodbuffer,0);
    lastpodcount = podcount;
    if (texp == TEX_ENV) {
	/* Bump the intensity of base material color */
	float col=(fogspec==FS_SPEC)?.4:.75;
	OpaqueMaterial.base.r = col;
	OpaqueMaterial.base.g = col;
	OpaqueMaterial.base.b = col;
	TransMaterial.base.r = col;
	TransMaterial.base.g = col;
	TransMaterial.base.b = col;
    }

    if (dynpre == LIT_PRE) {
	LightsInit();
	M_PreLight(firstpod,firstpod,pclosedata);
	if (fogspec == FS_FOG) {
	    numlights = 1;
	    lightstemplate[0] = (uint32)M_LightFog;
	    lightstemplate[1] = (uint32)&LightFogData;
	    lightstemplate[2] = 0;
	} else {
	    numlights = 0;
	    lightstemplate[0] = 0;
	}
    }

    return pclosedata;
}

uint32 timescalled = 0;
uint32 TotalTriangleCount=0;
uint32 totaltriangles;
float accumtime;
float totaltime;
uint32 unclippedtris;
uint32 clippedtris;
TimerTicks start, end;

void AnalyzeTime(void)
{
	TimerTicks delta;
	float runtime,elapsed;
	TimeVal tv;

	if(!timescalled) {
	    accumtime = 0.0;
	    totaltime = 0.0;
	    totaltriangles = 0;
	}
	SubTimerTicks(&start, &end, &delta);
	ConvertTimerTicksToTimeVal(&delta, &tv);
	runtime = tv.tv_sec + tv.tv_usec * .000001;
	if ((unclippedtris+clippedtris) <= 0)
	    return;
	elapsed = runtime/(float)(unclippedtris+clippedtris);
	accumtime += elapsed;
	totaltime += runtime;

	timescalled++;
	totaltriangles+=unclippedtris+clippedtris;
	if(timescalled == 50) {
	    accumtime /= (float)timescalled;

	    printf("Mercury only performance:\n");
	    printf("\t%d triangles to screen\n",(int)(totaltriangles/timescalled)); 
	    printf("\t%g triangles/sec to screen\n",1./accumtime);
	    if (!goalfr) {
		printf("\t%g triangles/sec in scene\n",(((float)TotalTriangleCount)*timescalled/totaltime));
	    }
	    timescalled = 0;
	    totaltriangles = 0;
	}
}

uint32 Realtimescalled;
float Realaccumtime;
float Realtotaltime;
uint32 RealTotalTriangleCount;
uint32 Realtotaltriangles;
TimerTicks Realstart, Realend;
uint32 Realpodcount;

float AnalyzeRealTime(void)
{
	TimerTicks delta;
	float cputime,timepertri;
	float fps;
	TimeVal tv;

	if (!Realtimescalled) {
	    Realtotaltime = 0;
	    Realpodcount = 0;
	    Realtotaltriangles = 0;
	    RealTotalTriangleCount = 0;
	}
	
	ConvertTimerTicksToTimeVal(&delta, &tv);

	SubTimerTicks(&Realstart, &Realend, &delta);
	ConvertTimerTicksToTimeVal(&delta, &tv);

	cputime = tv.tv_sec + tv.tv_usec * .000001;
	Realtotaltime += cputime;

	if ((unclippedtris+clippedtris) <= 0)
	    return 0;

	Realtotaltriangles += unclippedtris+clippedtris;
	RealTotalTriangleCount += TotalTriangleCount;
	Realtimescalled++;
	Realpodcount += podcount;

	if (Realtimescalled >= 50) {
	    cputime = Realtotaltime/Realtimescalled;
	    fps = 1./cputime;
	    timepertri = Realtotaltime/Realtotaltriangles;
	    if (printtime==1 || printtime==2) {
		extern bool done;
		extern float bdatime;
		extern uint32 bdanumtimes;
		extern float vdlwaittime;
		extern uint32 vdlwaitnumtimes;
		printf("--------------\n");
		printf("%d objects\n",Realpodcount/Realtimescalled);
		printf("%g frames/sec ",fps);
		printf("%d triangles drawn\n",RealTotalTriangleCount/Realtimescalled); 
		if (timepertri < .0000001)
		    return 0;
		printf("%g triangles/sec to screen\n",1./timepertri);
		if (!goalfr) {
		    printf("%g triangles/sec in scene\n",((float)RealTotalTriangleCount)*fps/Realtimescalled);
		}
		if (bdanumtimes > 0 ){
		    bdatime /= bdanumtimes;
		    printf("BDA busy %g percent\n",100.*bdatime*fps);
		    bdatime = 0;
		    bdanumtimes = 0;
		}
		if (vdlwaitnumtimes > 0) {
		    vdlwaittime /= vdlwaitnumtimes;
		    printf("Wait for VDL %g percent\n",100.*vdlwaittime*fps);
		    vdlwaittime = 0;
		    vdlwaitnumtimes = 0;
		}

#ifdef ALLOW_EXTRA_LIST
		if (extralist) {
		    extern float bdawait;
		    printf("Waiting for BDA %g percent\n",(100.*bdawait)/Realtotaltime);
		    bdawait = 0.;
		}
#endif /* ALLOW_EXTRA_LIST */
		printf("--------------\n");
		if (printtime == 2) done = 1;
	    }
	    Realtimescalled = 0;
	} else {
	    fps = 1./cputime;
	}
	return fps;
}

PodTexture* macrotexture;
PodTexture macrotexturecopies[29];
PodTexture* microtexture;
PodTexture* environment;
PodTexture brightmicrotexture;
uint32 numPages;

float cameraincline;
Matrix cameramat;
float camerapos = 0, camerapi = M_PI*2.;

float hither = 256.;
float vp_hither = 1.2;
float left = -1.0;
float right = 1.0;

void MoveCameraInit(CloseData *pclosedata)
{
    float cosphi, sinphi;
    ViewPyramid vp;
    extern float initialcamx, initialcamy, initialcamz;
    extern float initialinc;

    vp.left = left;
    vp.right = right;
    vp.top = 0.75;
    vp.bottom = -0.75;
    vp.hither = vp_hither;

    Matrix_Perspective(&frustrumviewmat, &vp, 0, FBWIDTH, 0, FBHEIGHT, 1./hither);

    pclosedata->fcamx = initialcamx;
    pclosedata->fcamy = initialcamy;
    pclosedata->fcamz = initialcamz;

    pclosedata->fwclose = view_wclose;
    pclosedata->fwfar = view_wfar;
    pclosedata->fscreenwidth = FBWIDTH;
    pclosedata->fscreenheight = FBHEIGHT;

    cameraincline = initialinc;
    cosphi = cosf(cameraincline);
    sinphi = sinf(cameraincline);

    cameramat.mat[0][0] = 1.0;
    cameramat.mat[1][0] = 0.0;
    cameramat.mat[2][0] = 0.0;
    cameramat.mat[0][1] = 0.0;
    cameramat.mat[1][1] = sinphi;
    cameramat.mat[2][1] = cosphi;
    cameramat.mat[0][2] = 0.0;
    cameramat.mat[1][2] = -cosphi;
    cameramat.mat[2][2] = sinphi;
    
    MoveCamera(pclosedata);
}

void MoveCamera(CloseData *pclosedata)
{

    extern uint32 automovecam;

    cameramat.mat[3][0] = - pclosedata->fcamx * cameramat.mat[0][0] - pclosedata->fcamy * cameramat.mat[1][0] 
	- pclosedata->fcamz * cameramat.mat[2][0];
    cameramat.mat[3][1] = - pclosedata->fcamx * cameramat.mat[0][1] - pclosedata->fcamy * cameramat.mat[1][1] 
	- pclosedata->fcamz * cameramat.mat[2][1];
    cameramat.mat[3][2] = - pclosedata->fcamx * cameramat.mat[0][2] - pclosedata->fcamy * cameramat.mat[1][2] 
	- pclosedata->fcamz * cameramat.mat[2][2];
		
    Matrix_Mult((pMatrix)&pclosedata->fcamskewmatrix, &cameramat, &frustrumviewmat);

    if (automovecam) {
	
	/* Put move code in here */
	camerapos += 8;
	if(camerapos >= 1024.) {
	    camerapos = 0;
	}
	if (camerapos < 512. ) {
	    pclosedata->fcamx = 400.0 * *(pcostable+(int)camerapos);
	    pclosedata->fcamy = 400.0 * *(psintable+(int)camerapos);
	} else {
	    pclosedata->fcamx = - 400.0 * *(pcostable+(int)(camerapos-512.));
	    pclosedata->fcamy = - 400.0 * *(psintable+(int)(camerapos-512.));
	}			

    }
}


/* This is done on purpose to see what a non-fogged object looks like
   when fog is present for the other objects in a scene */
void SetupOtherCase(CloseData *pc)
{
    pc->pdrawroutine = &M_DrawPreLitTex;
    pc->plightreturn = &M_LightReturnPreLitTex;

    CLT_Sync(&pc->pVIwrite);
    M_TBTex(&pc->pVIwrite);
    M_DBFog(&pc->pVIwrite, pc->fogcolor);
}

void MoveObjectsInit(void)
{
    float 	*vtxstart;
    PodGeometry	*geo, *nextgeo;
    uint32 	i,j,k,temp;
    Pod 	*ppod;
    void (*casecode)(CloseData*)=0;
    void (*transcase)(CloseData*)=0;
    Material *material = &OpaqueMaterial;
    PodTexture	*tex=0;
    uint32	texcount=num_tex_loads;

    switch (rendermode) {
    case DYN:
	casecode = &M_SetupDynLit;
	break;
	    
    case DYNTEX:
	casecode = &M_SetupDynLitTex;
	break;
	
    case DYNTRANS:
	casecode = M_SetupDynLit;
	transcase = M_SetupDynLitTrans;
	break;

    case DYNTRANSTEX:
	casecode = M_SetupDynLitTex;
	transcase = M_SetupDynLitTransTex;
	break;
	
    case DYNFOG:
	casecode = &M_SetupDynLitFog;
	break;

    case DYNFOGTEX:
	casecode = &M_SetupDynLitFogTex;
	break;

    case DYNFOGTRANS:
	casecode = M_SetupDynLitFog;
	transcase = M_SetupDynLitFogTrans;
	break;

    case DYNSPEC:
	casecode = &M_SetupDynLit;
	break;

    case DYNSPECTRANS:
	casecode = &M_SetupDynLit;
	transcase = &M_SetupDynLitTrans;
	break;

    case DYNSPECTEX:
	casecode = &M_SetupDynLitSpecTex;
	break;
	
    case PRE:
	casecode = &M_SetupPreLit;
	break;
	    
    case PRETEX:
	casecode = &M_SetupPreLitTex;
	break;
	
    case PRETRANS:
	casecode = M_SetupPreLit;
	transcase = M_SetupPreLitTrans;
	break;

    case PRETRANSTEX:
	casecode = M_SetupPreLitTex;
	transcase = M_SetupPreLitTransTex;
	break;
	
    case PREFOG:
	casecode = &M_SetupPreLitFog;
	break;

    case PREFOGTEX:
	casecode = &M_SetupPreLitFogTex;
	break;

    case PREFOGTRANS:
	casecode = M_SetupPreLitFog;
	transcase = M_SetupPreLitFogTrans;
	break;
	    
    case DYNENV:
	casecode = &M_SetupDynLitEnv;
	break;
	
    case DYNTRANSENV:
	casecode = M_SetupDynLitEnv;
	transcase = M_SetupDynLitTransEnv;
	break;

    case DYNFOGENV:
	casecode = &M_SetupDynLitFogEnv;
	break;

    case DYNSPECENV:
	casecode = &M_SetupDynLitSpecEnv;
	break;
	
    }

    if (transp) {
	material = &TransMaterial;
    } else {
	transcase = casecode;
    }
    
    switch (texp) {
    case TEX_TRUE:
	tex = microtexture;
	break;
    case TEX_NONE:
	tex = 0;
	break;
    case TEX_ENV:
	tex = environment;
	break;
    }

    if (fogspec == FS_SPEC) {
	OpaqueMaterial.base.a = 0.0;
    }

    ppod = ppodbuffer;
    nextgeo = geobuffer;
    
    if (objecttype==0) {
	for(i = 0; i < 32; i++) {
	    if (usefacets) {
		addtlicosdata[i] = *faceticostab[i % 5];
	    } else {
		addtlicosdata[i] = *icostab[i % 5];
	    }
	}
	for(i = 0; i < 32; i++)
	    addtldonutdata[i] = *donuttab[i % 5];


	if (whichobj==0 || whichobj==1) {
	    vtxstart = prelitplanevtx;
	    for(i = 0; i < NUM_PLANE; i++) {
		ppod->flags = 0;
		ppod->paadata = 0;
		ppod->pnext = ppod+1;
		ppod->pcase = casecode;
		switch (texp) {
		case TEX_TRUE:
		    if((i == 0) || (i >= 30) || (texcount <= 3)) {
			ppod->ptexture = macrotexture;
		    } else {
			texcount--;
			ppod->ptexture = &macrotexturecopies[i-1];
		    }
		    break;
		case TEX_NONE:
		    ppod->ptexture = 0;
		    break;
		case TEX_ENV:
		    ppod->ptexture = environment;
		}

		ppod->pgeometry = &plane;
		if (dynpre == LIT_PRE) {
		    geo = ppod->pgeometry;
		    memcpy(nextgeo, geo, sizeof(PodGeometry));
		    memcpy(vtxstart, geo->pvertex, 6*sizeof(float)*geo->vertexcount);
		    nextgeo->pvertex = vtxstart;
		    ppod->pgeometry = nextgeo;
		    nextgeo++;
		    vtxstart += 6*geo->vertexcount;
		} 
		ppod->pmatrix = &planemats[i];
		ppod->plights = lights;
		ppod->pmaterial = &OpaqueMaterial;
		ppod++;
		TotalTriangleCount += 98;
	    }
	}

	if (whichobj==0 || whichobj==2) {
	    vtxstart = preliticosvtx;
	    for(i = 0; i < NUM_ICOS; i++) {
		ppod->flags =  hithernocullFLAG;
		ppod->paadata = 0;
		ppod->pnext = ppod+1;
		ppod->pcase = transcase;
		ppod->ptexture = tex;

		temp = i % 37;
		if(temp < 5) {
		    if (usefacets) {
			ppod->pgeometry = faceticostab[temp];
		    } else {
			ppod->pgeometry = icostab[temp];
		    }
		} else {
		    ppod->pgeometry = &addtlicosdata[temp - 5];
		}
			
		if (dynpre == LIT_PRE) {
		    geo = ppod->pgeometry;
		    memcpy(nextgeo, geo, sizeof(PodGeometry));
		    memcpy(vtxstart, geo->pvertex, (6*sizeof(float)*geo->vertexcount)+(2*sizeof(short)*geo->sharedcount));
		    nextgeo->pvertex = vtxstart;
		    ppod->pgeometry = nextgeo;
		    nextgeo++;
		    vtxstart += (6*geo->vertexcount)+geo->sharedcount;
		} 
		ppod->pmatrix = &icosmats[i];
		for(j = 0; j < 2; j++)
		    for(k = 0; k < 2; k++)
		        icosmats[i].mat[j][k] = 0.0;
		ppod->plights = translights;
		ppod->pmaterial = material;
		if (useaa) {
		    M_BuildAAEdgeTable(ppod);
		    if (useaaline) {
			AAData *aadat = ppod->paadata;
			aadat->color.r = 0.;
			aadat->color.g = 0.;
			aadat->color.b = 0.;
			aadat->flags = AALINEDRAW;
		    }
		    switch (rendermode) {
		    case DYN:
			ppod->pcase =  M_SetupDynLitAA;
			break;
		    case DYNTEX:
			ppod->pcase =  M_SetupDynLitTexAA;
			break;
		    case PRE:
			ppod->pcase =  M_SetupPreLitAA;
			break;
		    case PRETEX:
			ppod->pcase =  M_SetupPreLitTexAA;
			break;
		    }
		}
		ppod++;
		TotalTriangleCount += 20;
	    }
	}

	if (whichobj==0 || whichobj==3) {
	    vtxstart = prelitdonutvtx;
	    for(i = 0; i < NUM_DONUT; i++) {
		ppod->flags = hithernocullFLAG;
		ppod->paadata = 0;
		ppod->pnext = ppod+1;
		ppod->pcase = transcase;
		ppod->ptexture = tex;

		temp = i % 37;
		if(temp < 5)
		    ppod->pgeometry = donuttab[temp];
		else
		    ppod->pgeometry = &addtldonutdata[temp - 5];

		if (dynpre == LIT_PRE) {
		    geo = ppod->pgeometry;
		    memcpy(nextgeo, geo, sizeof(PodGeometry));
		    memcpy(vtxstart, geo->pvertex, 6*sizeof(float)*geo->vertexcount);
		    nextgeo->pvertex = vtxstart;
		    ppod->pgeometry = nextgeo;
		    nextgeo++;
		    vtxstart += 6*geo->vertexcount;
		} 
		ppod->pmatrix = &donutmats[i];
		for(j = 0; j < 2; j++)
		    for(k = 0; k < 2; k++)
		        donutmats[i].mat[j][k] = 0.0;
		ppod->plights = translights;
		ppod->pmaterial = material;
		ppod++;
		TotalTriangleCount += 128;
	    }
	}
	
	if (whichobj==0) {
	    ppod->flags = hithernocullFLAG;
	    ppod->paadata = 0;

	    ppod->pnext = ppod+1;
	    switch (texp) {
	    case TEX_TRUE:
		ppod->ptexture = &brightmicrotexture;
		ppod->pcase = &SetupOtherCase;
		break;
	    case TEX_NONE:
		ppod->ptexture = 0;
		ppod->pcase = casecode;
		break;
	    case TEX_ENV:
		ppod->ptexture = environment;
		ppod->pcase = casecode;
		break;
	    }
	    ppod->pgeometry = &miniicos;
	    ppod->pmatrix = &pointlightmat;
	    ppod->plights = translights;
	    ppod->pmaterial = material;
	} else {
	    podcount = 2;
	    if (whichobj==1) {
		TotalTriangleCount = 98*podcount;
	    } else if (whichobj==2) {
		TotalTriangleCount = 20*podcount;
	    } else if (whichobj==3) {
		TotalTriangleCount = 128*podcount;
	    }
	}
    } else {
	gfloat *cubevtx;

	cubevtx = (float *)AllocMem(NUM_CUBE*(facetcube.vertexcount*6*sizeof(float)+
					      facetcube.sharedcount*2*sizeof(short)), MEMTYPE_NORMAL);
	if (!cubevtx) {
	    printf("Out of memory can't create cube vertices\n");
	    exit(1);
	}
	vtxstart = cubevtx;
	for(i = 0; i < podcount; i++) {
	    ppod->flags = hithernocullFLAG;
	    ppod->paadata = 0;
	    ppod->pnext = ppod+1;
	    ppod->pcase = transcase;
	    ppod->ptexture = tex;

	    if (usefacets) {
		ppod->pgeometry = &facetcube;
	    } else {
		ppod->pgeometry = &cube;
	    }
			
	    geo = ppod->pgeometry;
	    memcpy(nextgeo, geo, sizeof(PodGeometry));
	    memcpy(vtxstart, geo->pvertex, (6*sizeof(float)*geo->vertexcount)+(2*sizeof(short)*geo->sharedcount));
	    nextgeo->pvertex = vtxstart;
	    ppod->pgeometry = nextgeo;
	    nextgeo++;
	    vtxstart += (6*geo->vertexcount)+geo->sharedcount;

	    ppod->pmatrix = &cubemats[i];
		for(j = 0; j < 2; j++)
		    for(k = 0; k < 2; k++)
		        cubemats[i].mat[j][k] = 0.0;
	    ppod->plights = translights;
	    ppod->pmaterial = material;
	    ppod++;
	}
	TotalTriangleCount = podcount*12;
    }

#ifdef ALLOW_EXTRA_LIST
    if (extralist) {
	TotalTriangleCount *= 3;
    }
#endif /* ALLOW_EXTRA_LIST */

    printf("Number of triangles in scene = %d\n",TotalTriangleCount);
}

long fi_angle = 0;
long fi_speed = -32;
long do_angle = 0;
long do_speed = 6;
float fogfar = 5000.0;
float fognear = 900.0;
float fogspeed = 10.;
float fogfarmin = 1000.0;
float fogfarmax = 5000.0;
long fogging=1;
long fogmovingout = 0;
long pf_angle = 0;
long pf_rad = 0;
long pf_radspeed = 16;

void MoveObjects(void)
{
    long i,temp,npods;
    Pod *ppod;
    Matrix *pmat;
    float rad;
	
    if(!displayoff) {
	if(fogmovingout) {
	    fogfar += fogspeed;
	    if(fogfar > fogfarmax) {
		fogfar = fogfarmax;
		fogmovingout = 0;
	    }
	} else {
	    fogfar -= fogspeed;
	    if(fogfar < fogfarmin)
		{
		    fogfar = fogfarmin;
		    fogmovingout = 1;
		}
	}
	
	LightFogData.dist1 = hither/(fogfar - fognear);
	LightFogData.dist2 = fognear/(fogfar - fognear);
    }

    if(!displayoff) {
	do_angle += do_speed;
	fi_angle += fi_speed;
    }

    if (objecttype==1) {
	ppod = ppodbuffer;
	pmat = &cubemats[0];
	npods = podcount;
    } else {
	ppod = ppodbuffer + 40;
	pmat = &icosmats[0];
	npods = 113;
    }
	
    for(i = 0; i < npods; i++) {
	if(i == 60 && objecttype != 1)
	    pmat = &donutmats[0];

	temp = (do_angle + (i * 151)) & 0x3ff;

	if(i < 60) {
	    pmat->mat[1][1] = pmat->mat[0][0] = *(pcostable + (fi_angle & 0x3ff));
	    pmat->mat[1][0] = -(*(psintable + (fi_angle & 0x3ff)));
	    pmat->mat[0][1] = *(psintable + (fi_angle & 0x3ff));
	    pmat->mat[2][2] = 1.0;
	} else {
	    float cosa,sina,cosb,sinb;
	    cosa = *(pcostable + (temp & 0x3ff));
	    sina = *(psintable + (temp & 0x3ff));
	    cosb = *(pcostable + ((temp << 2) & 0x3ff));
	    sinb = *(psintable + ((temp << 2) & 0x3ff));
	    pmat->mat[0][0] = cosa;
	    pmat->mat[0][1] = sina;
	    pmat->mat[0][2] = 0.0;

	    pmat->mat[1][0] = -sina * cosb;
	    pmat->mat[1][1] = cosa * cosb;
	    pmat->mat[1][2] = -sinb;

	    pmat->mat[2][0] = -sina * sinb;
	    pmat->mat[2][1] = cosa * sinb;
	    pmat->mat[2][2] = cosb;
	}
			
	rad = 350.0 + (400.0/113.0) * (float)((i * 59) % 113);
	pmat->mat[3][0] = rad * *(pcostable + temp);
	pmat->mat[3][1] = 1250.0 + rad * *(psintable + temp);
	pmat->mat[3][2] = -900.0 + (650.0/113.0) * (float)((i * 31) % 113);
	pmat++;
	ppod++;
    }

    if(!displayoff) {
	pf_angle++;
	pf_rad = (pf_rad + pf_radspeed) & 0xfff;
    }

    {
	float cosa,sina,mfact;
		
	cosa = *(pcostable + ((pf_angle << 6) & 0x3ff));
	sina = *(psintable + ((pf_angle << 6) & 0x3ff));
		
	pointlightmat.mat[1][1] = cosa;
	pointlightmat.mat[2][1] = sina;
	pointlightmat.mat[1][2] = -sina;
	pointlightmat.mat[2][2] = cosa;
		
	if(pf_rad < 0x800)
	    mfact = (float)pf_rad/2048.0;
	else
	    mfact = 2.0 - (float)pf_rad/2048.0;
	if(mfact < 0.1)
	    mfact = 0.1;

	LightPointData.x = pointlightmat.mat[3][0] =
            *(psintable + (((pf_angle * 37) >> 2) & 0x3ff)) * 500.0 * mfact;
	LightPointData.y = pointlightmat.mat[3][1] =
           1250.0 + *(psintable + (((pf_angle * 53) >> 2) & 0x3ff)) * 500.0 * mfact;
	LightPointData.z = pointlightmat.mat[3][2] =
            -575.0 + *(psintable + (((pf_angle * 17) >> 2) & 0x3ff)) * 300.0 * mfact;
    }

    {
	float lighttilt = 0;
	float lighttheta = 0;
	float lightrotatespeed = -0.08;
	static long lightinorout = 0;
	float lightmaxtilt = PI/2;
	float lightmintilt = 0.0;
	float lightinoutspeed = 0.0025;

	objectangle += objectvelo;
	if(objectangle > (8 * PI))
	    objectangle -= (8 * PI);
	else if (objectangle < -(8 * PI))
	    objectangle += 8 * PI;


	lightx = cosf(objectangle) * 400.0;
	lighty = 725.0 + sinf(objectangle) * 400.0;
	lightz = 150.0 * sinf(objectangle/4) ;

	LightSoftSpotData.x = lightx;
	LightSoftSpotData.y = lighty;
	LightSoftSpotData.z = lightz;

	lighttheta += lightrotatespeed;
	if(lighttheta > (2 * PI))
	    lighttheta -= (2 * PI);
	else if (lighttheta < -(2 * PI))
	    lighttheta += 2 * PI;

	if(lightinorout) {
	    lighttilt -= lightinoutspeed;
	    if(lighttilt < lightmintilt) {
		lighttilt = lightmintilt;
		lightinorout = 0;
	    }
	} else {
	    lighttilt += lightinoutspeed;
	    if(lighttilt > lightmaxtilt) {
		lighttilt = lightmaxtilt;
		lightinorout = 1;
	    }
	}
	LightSoftSpotData.nx = sinf(lighttilt) * cosf(lighttheta);
	LightSoftSpotData.ny = sinf(lighttilt) * sinf(lighttheta);
	LightSoftSpotData.nz = -cosf(lighttilt);

    }
}	

/*
 *	Load two textures.  Artificially replicate.  Test code only!
 */
void InitializePodTextures(void)
{
    long i,j;
    uint32 *plong,*plong2, temp;

    ReadInTextureFile(&macrotexture,&numPages,"three.utf", AllocMem);
    ReadInTextureFile(&microtexture,&numPages,"twelve.utf",AllocMem);
    ReadInTextureFile(&environment,&numPages,"sky.utf",AllocMem);

    /*	give the colors an SSB they don't have! */
    for(i = 0; i < (microtexture->pipbytes >> 2); i++)
	*(microtexture->ppip + i) |= 0x80000000;

    {
	extern uint32 usetranstexel;
	extern uint32 transtexel;
	/* User selects which texel to make translucent in command line */

	if (usetranstexel) {
	    *(microtexture->ppip+transtexel) &= ~0x80000000;
	}
    }

    /*	Make 29 copies of the macro texture */
    for(i = 0; i < 29; i++)
	{
	    macrotexturecopies[i] = *macrotexture;

	    plong = (uint32 *) AllocMem(macrotexture->texturebytes,MEMTYPE_ANY);
	    if(!plong)
		{
		    printf("Out of memory, can't make texture copies!\n");
		    exit(1);
		}
	    macrotexturecopies[i].ptexture = plong;
	    plong2 = macrotexture->ptexture;
	    for(j = 0; j < (macrotexture->texturebytes >> 2); j++)
	    	*plong++ = *plong2++;

	    plong = (uint32 *) AllocMem(macrotexture->pipbytes,MEMTYPE_ANY);
	    if(!plong)
		{
		    printf("Out of memory, can't make pip copies!\n");
		    exit(1);
		}
	    macrotexturecopies[i].ppip = plong;
	    plong2 = macrotexture->ppip;


	    {
		long bright,r,g,b;
		/*			bright = 256 - (i * 8);	*/
		bright = 256;
		for(j = 0; j < (macrotexture->pipbytes >> 2); j++)
		    {
		    	temp = *plong2++;
		    	r = (temp >> 16) & 0xff;
		    	g = (temp >> 8) & 0xff;
		    	b = temp & 0xff;
		    	r = ((bright * r) >> 8) & 0xff;
		    	g = ((bright * g) >> 8) & 0xff;
		    	b = ((bright * b) >> 8) & 0xff;
		    	temp = (temp & 0xff000000) + (r << 16) + (g << 8) + b;
		    	*plong++ = temp;
		    }
	    }
	}

    /*	Make one copy of the micro texture */
    brightmicrotexture = *microtexture;

    plong = (uint32 *) AllocMem(microtexture->texturebytes,MEMTYPE_ANY);
    if(!plong)
	{
	    printf("Out of memory, can't make texture copies!\n");
	    exit(1);
	}
    brightmicrotexture.ptexture = plong;
    plong2 = microtexture->ptexture;
    for(j = 0; j < (microtexture->texturebytes >> 2); j++)
    	*plong++ = *plong2++;

    plong = (uint32 *) AllocMem(microtexture->pipbytes,MEMTYPE_ANY);
    if(!plong)
	{
	    printf("Out of memory, can't make pip copies!\n");
	    exit(1);
	}
    brightmicrotexture.ppip = plong;
    plong2 = microtexture->ppip;

    {
	long r,g,b;
	for(j = 0; j < (microtexture->pipbytes >> 2); j++)
	    {
	    	temp = *plong2++;
	    	r = (temp >> 16) & 0xff;
	    	g = (temp >> 8) & 0xff;
	    	b = temp & 0xff;
	    	r = (r + 256) >> 1;
	    	g = (g + 256) >> 1;
	    	b = (b + 256) >> 1;
	    	temp = (temp & 0xff000000) + (r << 16) + (g << 8) + b;
	    	*plong++ = temp;
	    }
    }

}

static void BurnTime(void)
{
    TimerTicks gamestart, gameend;
    TimerTicks delta;
    TimeVal tv;
    float cputime;
    float junk;
    long i;
    junk = 0.0;
    SampleSystemTimeTT(&gamestart);
    while(1) {
	for(i = 0; i < 1024; i+=8) {
	    SampleSystemTimeTT(&gameend);
	    SubTimerTicks(&gamestart, &gameend, &delta);
	    ConvertTimerTicksToTimeVal(&delta, &tv);
	    cputime = tv.tv_sec + tv.tv_usec * .000001;
	    if (cputime>=apptime) {
		return;
	    }
	    junk += *(pcostable + i);
	}
	for(i = 0; i < 1024; i+=8) {
	    SampleSystemTimeTT(&gameend);
	    SubTimerTicks(&gamestart, &gameend, &delta);
	    ConvertTimerTicksToTimeVal(&delta, &tv);
	    cputime = tv.tv_sec + tv.tv_usec * .000001;
	    if (cputime>=apptime) {
		return;
	    }
	    junk += *(psintable + i);
	}
    }
}

void GameCode(CloseData *pclosedata)
{
    uint32 podpiperet;

    LightsInit();

    if (moveobjects) {
	MoveObjects();
    }

    MoveCamera(pclosedata);

    if (useapp) {
	BurnTime();
    }


    SampleSystemTimeTT(&start);

    /* Only sort if number of objects has changed */
    if (lastpodcount != podcount) {
	Pod *ppod;
	long i;

	/* Re-link the list and re-sort */
	ppod = ppodbuffer;
	for(i = 0; i < podcount; i++) {
	    ppod->pnext = ppod+1;
	    ppod++;
	}
	firstpod = M_Sort(podcount,ppodbuffer,0);
	lastpodcount = podcount;
    }
    podpiperet = M_Draw(firstpod,pclosedata);
    unclippedtris = podpiperet & 0xffff;
    clippedtris = (podpiperet & 0xffff0000)>>16;
#ifdef ALLOW_EXTRA_LIST
    if (extralist){
 	extern uint32 extratris;
 	unclippedtris += extratris;
    }
#endif /* ALLOW_EXTRA_LIST */
    SampleSystemTimeTT(&end);

}
