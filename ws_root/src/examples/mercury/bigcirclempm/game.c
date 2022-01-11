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
#include "mpm.h"
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
extern bool useapp;
extern float apppodratio,apptime;
/* Change to 1 to print stuff */
uint32 printtime=0;

uint32 TotalTriangleCount=0;

extern uint32 objecttype, usefacets;

extern void DumpState(void);
int whichobj=0; /* ==0 for all objects,
		   ==1 for plane only,
		   ==2 for icos only
		   ==3 for dunuts only */

uint32 podcountmax=600;

struct TimerBlock tb[2];

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

PodGeometry *geobuffer;
float *pcostable;
float *psintable;

Matrix frustrumviewmat;

float view_wclose = 1.01;
float view_wfar = 100000000.0;

void MoveObjects(Pod *ppodbuffer, Matrix *pmatrixbuffer);
void MoveObjectsInit(Pod *, Matrix *);
void InitializePodTextures(void);

uint32 ipointlightmat=-1;
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
	0.0,	0.0,	0.0,
	0.0,	0.0,	-1.0,
	500000.0 * 32.0 * 1.0,
	500000.0,
	0.5,				/* cos(60 degrees) */
	1.0/(.866-.5),			/* 1.0/(cos(30 degrees)-cos(60 degrees)) */
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
	0.5,	0.5,	0.5,		/* diffuse color	*/
	0.05,				/* shine		*/
	1.0,	1.0,	1.0,		/* specular color	*/
};

Material TransMaterial = 
{
	0.05,	0.05,	0.05,	.5,	/* base color 		*/
	0.5,	0.5,	0.5,		/* diffuse color	*/
	0.05,				/* shine		*/
	1.0,	1.0,	1.0,		/* specular color	*/
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
uint32 prelightstemplate[] = 
{
	(uint32)&M_LightDir,
	(uint32)&LightDirData,
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

void GameCodeInit(MpmContext *mpmc)
{
    Pod		*podbase;
    Matrix 	*matrixbase;
    CloseData	*pclosedata;

    tb[0].timescalled = 0;
    tb[1].timescalled = 0;
    
    if (objecttype == 0) {
	podcountmax = mpmc->podcount = 154;	
    } else {
	mpmc->podcount = podcountmax;
    }
    podbase = mpmc->pm[0].podbase = (Pod *)AllocMemAligned(2*podcountmax*sizeof(Pod), MEMTYPE_NORMAL, 32);
    matrixbase = mpmc->pm[0].matrixbase = (Matrix*)AllocMemAligned(2*podcountmax*sizeof(Matrix), MEMTYPE_NORMAL, 32);
    geobuffer = (PodGeometry *)AllocMemAligned(podcountmax*sizeof(PodGeometry), MEMTYPE_NORMAL, 32);
    if (!podbase || !matrixbase || !geobuffer) {
	    printf("Out of memory can't create pods/matrices/geos\n");
	    exit(1);
    }
    memset((char*)mpmc->pm[0].matrixbase, 0, 2*podcountmax*sizeof(Matrix));
    memset((char*)geobuffer, 0, podcountmax*sizeof(PodGeometry));

    mpmc->pm[1].podbase = podbase+podcountmax;
    mpmc->pm[1].matrixbase = matrixbase+podcountmax;

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

    if(!pcostable || !psintable)
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

    pclosedata = mpmc->pclosedata[0] = M_Init(200, 0, mpmc->gs[0]);
    mpmc->pclosedata[1] = M_Init(200, 0, mpmc->gs[1]);
    mpmc->pclosedata[2] = M_Init(200, 0, mpmc->gs[1]);

    pclosedata->fogcolor = M_PackColor(.5, .5, .5);

    InitializePodTextures();
    MoveCameraInit(pclosedata);
    MoveCamera(pclosedata);

    MoveObjectsInit(podbase,matrixbase);
    MoveObjects(podbase,matrixbase);

    /* Make sure the sort gets called again */ 
    mpmc->pm[0].needsort = 1;
    mpmc->pm[1].needsort = 1;

    if (dynpre == LIT_PRE) {
	Pod 	*firstpod;

	firstpod = M_Sort(podcountmax,mpmc->pm[0].podbase,0);
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

    /* Now pods have been positioned and possibly pre-lit, duplicate */
    MPM_DuplicatePods(mpmc);

}


void AnalyzeTime(TimerBlock *ptb,uint32 numtri,char* msg)
{
    TimerTicks delta;
    float runtime,elapsed;
    TimeVal tv;

    if(!ptb->timescalled) {
	ptb->accumtime = 0.0;
	ptb->totaltime = 0.0;
	ptb->totaltriangles = 0;
    }
    SubTimerTicks(&ptb->start, &ptb->end, &delta);
    ConvertTimerTicksToTimeVal(&delta, &tv);
    runtime = tv.tv_sec + tv.tv_usec * .000001;
    if (numtri <= 0)
	return;
    elapsed = runtime/(float)numtri;
    ptb->accumtime += elapsed;
    ptb->totaltime += runtime;

    ptb->timescalled++;
    ptb->totaltriangles+=numtri;
    if(ptb->timescalled == 50) {
	ptb->accumtime /= 50.;
	if (printtime) {
	    printf("Mercury only performance for %s:\n",msg);
	    printf("\t%d triangles to screen\n",(int)(ptb->totaltriangles/50.)); 
	    printf("\t%g triangles/sec to screen\n",1./ptb->accumtime);
	}
	ptb->timescalled = 0;
	ptb->totaltriangles = 0;
    }
}

uint32 Realtimescalled;
float Realaccumtime;
float Realtotaltime;
uint32 RealTotalTriangleCount;
uint32 Realtotaltriangles;
TimerTicks Realstart, Realend;
uint32 Realpodcount;

bool AnalyzeRealTime(Report *r, uint32 podcount, uint32 numtriangles)
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
	
	SubTimerTicks(&Realstart, &Realend, &delta);
	ConvertTimerTicksToTimeVal(&delta, &tv);

	cputime = tv.tv_sec + tv.tv_usec * .000001;
	Realtotaltime += cputime;

	if (numtriangles <= 0)
	    return 0;

	Realtotaltriangles += numtriangles;
	RealTotalTriangleCount += TotalTriangleCount;
	Realtimescalled++;
	Realpodcount += podcount;
	r->tps_screen = numtriangles/cputime;
	r->tps_scene = TotalTriangleCount/cputime;
	
	if (Realtimescalled >= 50) {

	    cputime = Realtotaltime/Realtimescalled;
	    fps = 1./cputime;
	    timepertri = Realtotaltime/Realtotaltriangles;
	    if (printtime) {
		extern bool done;
		uint32 screentri = Realtotaltriangles/Realtimescalled;
		printf("--------------\n");
		printf("%d objects\n",Realpodcount/Realtimescalled);
		printf("%g frames/sec ",fps);
		printf("%d triangles drawn %d to screen\n",RealTotalTriangleCount/Realtimescalled,
		       screentri); 
		if (timepertri < .0000001)
		    return 0;
		printf("%g triangles/sec to screen\n",1./timepertri);
		if (!goalfr) {
		    printf("%g triangles/sec in scene\n",((float)RealTotalTriangleCount)*fps/Realtimescalled);
		}
		printf("--------------\n");
		if (printtime == 2) done = 1;
	    }
	    Realtimescalled = 0;
	} else {
	    fps = 1./cputime;
	}
	r->fps = fps;
	return 1;
}

PodTexture* macrotexture;
PodTexture macrotexturecopies[29];
PodTexture* microtexture;
PodTexture brightmicrotexture;
uint32 numPages;

float cameraincline = 0;
Matrix cameramat;
float camerapos = 0, camerapi = 6.283185;

float hither = 256.;
float vp_hither = 1.2;
float left = -1.0;
float right = 1.0;

void MoveCameraInit(CloseData *pclosedata)
{
    float cosphi, sinphi;
    ViewPyramid vp;
    extern float initialcamx, initialcamy, initialcamz;

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

void MoveObjectsInit(Pod *ppodbuffer, Matrix *pmatrixbuffer)
{
    float 	*vtxstart;
    PodGeometry	*geo, *nextgeo;
    uint32 	i,temp;
    Pod 	*ppod;
    Matrix	*pmatrix;
    void (*casecode)(CloseData*)=0;
    void (*transcase)(CloseData*)=0;
    Material *material = &OpaqueMaterial;
    PodTexture	*tex;
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
    }

    if (transp) {
	material = &TransMaterial;
    } else {
	transcase = casecode;
    }
    
    if (texp) {
	tex = microtexture;
    } else {
	tex = 0;
    }

    if (fogspec == FS_SPEC) {
	OpaqueMaterial.base.a = 0.0;
    }

    ppod = ppodbuffer;
    pmatrix = pmatrixbuffer;
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
		ppod->pnext = ppod+1;
		ppod->pcase = casecode;
		if (texp) {
		    if((i == 0) || (i >= 30) || (texcount <= 3)) {
			ppod->ptexture = macrotexture;
		    } else {
			texcount--;
			ppod->ptexture = &macrotexturecopies[i-1];
		    }
		} else {
		    ppod->ptexture = 0;
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
		ppod->pmatrix = pmatrix;
		memcpy(pmatrix,&planemats[i],sizeof(Matrix));
		ppod->plights = lights;
		ppod->pmaterial = &OpaqueMaterial;
		ppod++;
		pmatrix++;
		TotalTriangleCount += 98;
	    }
	}

	if (whichobj==0 || whichobj==2) {
	    vtxstart = preliticosvtx;
	    for(i = 0; i < NUM_ICOS; i++) {
		ppod->flags =  hithernocullFLAG;
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
		ppod->pmatrix = pmatrix;
		ppod->plights = translights;
		ppod->pmaterial = material;
		ppod++;
		pmatrix++;
		TotalTriangleCount += 20;
	    }
	}

	if (whichobj==0 || whichobj==3) {
	    vtxstart = prelitdonutvtx;
	    for(i = 0; i < NUM_DONUT; i++) {
		ppod->flags = hithernocullFLAG;
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
		ppod->pmatrix = pmatrix;
		ppod->plights = translights;
		ppod->pmaterial = material;
		ppod++;
		pmatrix++;
		TotalTriangleCount += 128;
	    }
	}
	
	if (whichobj==0) {
	    ppod->flags = hithernocullFLAG;

	    ppod->pnext = ppod+1;
	    if (texp) {
		ppod->ptexture = &brightmicrotexture;
		ppod->pcase = &SetupOtherCase;
	    } else {
		ppod->ptexture = 0;
		ppod->pcase = casecode;
	    }
	    ppod->pgeometry = &miniicos;
	    ppod->pmatrix = pmatrix;
	    ipointlightmat = pmatrix-pmatrixbuffer;
	    memcpy(pmatrix,&pointlightmat,sizeof(Matrix));
	    ppod->plights = translights;
	    ppod->pmaterial = material;
	} else {
	    podcountmax = 2;
	    if (whichobj==1) {
		TotalTriangleCount = 98*podcountmax;
	    } else if (whichobj==2) {
		TotalTriangleCount = 20*podcountmax;
	    } else if (whichobj==3) {
		TotalTriangleCount = 128*podcountmax;
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
	for(i = 0; i < podcountmax; i++) {
	    ppod->flags = hithernocullFLAG;
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

	    ppod->pmatrix = pmatrix;
	    ppod->plights = translights;
	    ppod->pmaterial = material;
	    ppod++;
	    pmatrix++;
	}
	TotalTriangleCount = podcountmax*12;
    }


    if (printtime)
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

void MoveObjects(Pod *ppodbuffer, Matrix *pmatrixbuffer)
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

    ppod = ppodbuffer;
    pmat = pmatrixbuffer;
    if (objecttype==1) {
	npods = podcountmax;
    } else {
	ppod += NUM_PLANE;
	pmat += NUM_PLANE;
	npods = podcountmax - NUM_PLANE - 1;
    }
	
    for(i = 0; i < npods; i++) {
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

    if (ipointlightmat>=0) {
	float cosa,sina,mfact;
	Matrix *ppointlightmat = pmatrixbuffer+ipointlightmat;
		
	cosa = *(pcostable + ((pf_angle << 6) & 0x3ff));
	sina = *(psintable + ((pf_angle << 6) & 0x3ff));
		
	ppointlightmat->mat[1][1] = cosa;
	ppointlightmat->mat[2][1] = sina;
	ppointlightmat->mat[1][2] = -sina;
	ppointlightmat->mat[2][2] = cosa;
		
	if(pf_rad < 0x800)
	    mfact = (float)pf_rad/2048.0;
	else
	    mfact = 2.0 - (float)pf_rad/2048.0;
	if(mfact < 0.1)
	    mfact = 0.1;

	LightPointData.x = ppointlightmat->mat[3][0] =
            *(psintable + (((pf_angle * 37) >> 2) & 0x3ff)) * 500.0 * mfact;
	LightPointData.y = ppointlightmat->mat[3][1] =
           1250.0 + *(psintable + (((pf_angle * 53) >> 2) & 0x3ff)) * 500.0 * mfact;
	LightPointData.z = ppointlightmat->mat[3][2] =
            -575.0 + *(psintable + (((pf_angle * 17) >> 2) & 0x3ff)) * 300.0 * mfact;
    }

    if (numlights > 3 ) {
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
	lightz = 150.0 * sinf(objectangle/4) - 800.0;

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

void GameCode(MpmContext *mpmc)
{
    LightsInit();

    if (moveobjects) {
	MoveObjects(MPM_Getpodbase(mpmc),MPM_Getmatrixbase(mpmc));
    }
    MoveCamera(mpmc->pclosedata[0]);

    if (useapp) {
	BurnTime();
    }
}

