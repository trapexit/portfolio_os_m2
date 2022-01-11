#ifndef __MERCURY_H
#define __MERCURY_H
#ifdef MACINTOSH
#include <graphics:clt:gstate.h>
#include <graphics:clt:clt.h>
#else
#include <graphics/clt/gstate.h>
#include <graphics/clt/clt.h>
#endif
#include "lighting.h"
#include "matrix.h"
#include "cltmacromacros.h"

typedef struct CloseData 
{
    /*	constants 	*/
    char tltable[32];				/* line aligned table */
    float fconstTINY;
    float fconst0pt0;
    float fconst0pt25;
    float fconst0pt5;
    float fconst0pt75;
    float fconst1pt0;
    float fconst2pt0;
    float fconst3pt0;
    float fconst8pt0;
    float fconst16pt0;
    float fconst255pt0;
    float fconst12million;
    float fconstasin0;
    float fconstasin1;
    float fconstasin2;
    float fconstasin3;

    /*	ALMOST constants */
    float fwclose;
    float fwfar;
    float fscreenwidth;
    float fscreenheight;

    /* filled in in advance */
    Matrix fcamskewmatrix;		/* takes 2 cache lines, not 3 */
    float *pxformbuffer;
    uint32 *ptexselsnippets;
    uint32 *pVIwrite;
    uint32 *pVIwritemax;
    GState *gstate;	
    uint32 watermark;			/* How much to leave in buffer */
    float frbase;
    float fgbase;
    float fbbase;
    float fabase;
    float frdiffuse;
    float fgdiffuse;
    float fbdiffuse;
    float fshine0;
    float frspecular;
    float fgspecular;
    float fbspecular;
    float fshine1;
    float fcamx;
    float fcamy;
    float fcamz;
    float flocalcamx;
    float flocalcamy;
    float flocalcamz;
    float flight0;
    float flight1;
    float flight2;
    float flight3;
    float flight4;
    float flight5;
    float flight6;
    float flight7;
    float flight8;
    float flight9;
    float flight10;
    float flight11;
    float flight12;
    float flight13;
    void (*plightreturn)(void);
    void (*pdrawroutine)(void);

    /* filled in inside routine */
    uint32 drawlrsave;
    uint32 drawlrsave1;
    uint32 drawlrsave2;
    uint32 ppodsave;
    float fxmat00;
    float fxmat01;
    float fxmat02;
    float fxmat10;
    float fxmat11;
    float fxmat12;
    float fxmat20;
    float fxmat21;
    float fxmat22;
    float fxmat30;
    float fxmat31;
    float fxmat32;

    /*	clip routine	*/
    float	float0;
    float	float1;
    float	float2;
    float	float3;
    float	float4;
    float	float5;
    float	float6;
    float	float7;
    float	float8;
    float	float9;
    float	float10;
    float	float11;
    float	float12;
    float	float13;
    float	float14;
    float	float15;
    float	float16;
    float	float17;
    uint32	cr;
    uint32	int17_31[15];

    /* converted light data! the more lights, the longer -- this
     * should support 8 lights for sure.
     */
    uint32 convlightdata[100];
    /*
     * clipping buffers
     * 2 swapping buffers of 8 pointers each
     * and space for up to 13 vertices at 10 words each
     */
    uint32 clipbuf1[8];
    uint32 clipbuf2[8];
    uint32 clipdata[130];

    /* Other data used during setup */
    uint32 fogcolor;
    uint32 srcaddr;
    uint32 depth;
    /* AA flags */
    uint32 aa;
#ifdef STATISTICS
    uint32 numpods_fast;
    uint32 numpods_slow;
    uint32 numtris_fast;
    uint32 numtris_slow;
    uint32 numtexloads;
    uint32 numtexbytes;
#endif    
    /* MP variables */
    Item mpioreq;
    uint32 nverts;
    struct CloseData *slaveclosedata;
    uint32 *slavestack;
    struct PodQueue *mp;
    uint32 maxpods;
    uint32 *clist[2];
    uint32 clistsize;
    uint32 whichlist;
    struct PodList *podlist;
    struct PodList *firstl, *lastl;
    uint32 numpods;
    uint32 numbytes;
    uint32 vertsplit;
    void (*lastcase)(struct CloseData*);
    /* MP statistics */
    uint32 totalpods;
} CloseData;

typedef struct PodList {
    uint16 podid;
    uint16 flags;
    uint32 *startlist;
    uint32 *endlist;
} PodList;

typedef struct PodQueue {
    void	*inlock;
    int32 	inid;
    struct Pod	*curpod;
/* Separate insies from outsies */
    uint32	pad[5]; 
    void	*outlock;
    int32	outid;
    uint32	outflag;
    uint32	startaddr;
} PodQueue;

typedef struct AAData {
    uint16	edgecount;
    uint16	flags;
    Color3	color;
    uint8	*paaedgebuffer;
} AAData;

typedef struct Pod 
{
    uint32 flags;
    struct Pod *pnext;
    void (*pcase)(CloseData*);
    struct PodTexture *ptexture;
    struct PodGeometry *pgeometry;
    Matrix *pmatrix;
    uint32 *plights;
    uint32 *puserdata;
    Material *pmaterial;
    AAData *paadata;
} Pod;

#define M_STACK_SIZE		4096

/* flags bits in pod structure */
#define	samecaseFLAG		(1 << (31-8))
#define	sametextureFLAG		(1 << (31-9))

/* used in pploop, can be thrown away */
#define callatstartFLAG		(1 << (31-16))
#define casecodeisasmFLAG 	(1 << (31-17))
#define usercheckedclipFLAG 	(1 << (31-18))
#define preconcatFLAG	 	(1 << (31-19))

/* okay to overwrite these in per vertex Draw routines */
#define hithernocullFLAG	(1 << (31-23))

/* need to preserve these flags in per vertex Draw routines */
#define nocullFLAG		(1 << (31-28))
#define frontcullFLAG		(1 << (31-29))
#define clipFLAG		(1 << (31-30))
#define callatendFLAG		(1 << (31-31))

/* flags for antialias */

#define aa2ndpassFLAG		(1 << (31-24))
#define aapassbbtestFLAG	(1 << (31-25))
#define aasavenocullFLAG	(1 << (31-26))
#define aasaveclipFLAG		(1 << (31-27))

#define aanodrawFLAG		(1 << (31-24))
#define aafirsttriFLAG		(1 << (31-25))
#define aatriculledFLAG		(1 << (31-26))
#define aaalterprimFLAG		(1 << (31-27))

#define	AA2NDPASS	0x1
#define	AALINEDRAW	0x2


typedef struct PodGeometry 
{
    float fxmin,fymin,fzmin;
    float fxextent,fyextent,fzextent;
    float *pvertex;
    short *pshared;
    uint16 vertexcount;
    uint16 sharedcount;
    short *pindex;
    float *puv;
    uint16 *paaedge;
} PodGeometry;

typedef struct PodTexture
{
    void (*proutine)(void);
    struct TpageSnippets *ptpagesnippets;
    uint32 *ptexture;
    uint32 texturecount;
    uint32 texturebytes;
    uint32 *ppip;
    uint32 pipbytes;
} PodTexture;

typedef struct MSnippet
{
    CltSnippet  snippet;
    float      uscale, vscale;
} MSnippet;

typedef struct TpageSnippets
{
	uint32 loadcount;
	CltSnippet *ploadsnippets;
	MSnippet *pselectsnippets;
} TpageSnippets;

#define PCLK		0x8000
#define PFAN		0x4000
#define CFAN		0x2000
#define NEWS		0x1000
#define STXT		0x0800	

typedef struct BBox 
{
    float fxmin,fymin,fzmin;
    float fxextent,fyextent,fzextent;
} BBox;

typedef struct BBoxList 
{
    uint32 flags;
    struct BBoxList *pnext;
    struct BBox *pbbox;
    Matrix *pmatrix;
} BBoxList;

/*	BBox flags		*/

#define	xminFlag		(1 << (31-1))
#define	xmaxFlag		(1 << (31-5))
#define	yminFlag		(1 << (31-9))
#define	ymaxFlag		(1 << (31-13))
#define	hitherFlag		(1 << (31-17))
#define	yonFlag			(1 << (31-21))


/* Entry points into Mercury */
extern CloseData* M_Init(uint32 nverts, uint32 clistwords, GState *gs);
extern Err	M_InitMP(CloseData *pclosedata, uint32 maxpods, uint32 clistwords, uint32 numverts);
extern void	M_EndMP(CloseData *pclosedata);
extern void	M_End(CloseData *pclosedata);
extern Pod*	M_Sort(uint32 podcount, Pod *ppod, Pod **plast);
extern void 	M_BuildAAEdgeTable(Pod *ppod);
extern void 	M_FreeAAEdgeTable(Pod *ppod);
extern uint32	M_Draw(Pod *pfirstpod, CloseData *pclosedata);
extern void	M_DrawEnd(CloseData *pclosedata);
extern uint32	M_DrawPod(Pod *, CloseData *pclosedata);
extern void	M_PreLight(Pod *psrcpod,Pod *pdestpod, CloseData *pclosedata);
extern void	M_CopySnippetData(CloseData *pclosedata, CltSnippet *);
extern void	M_BoundsTest(BBoxList*, CloseData*);

/* Utilities */
extern void 	M_LoadPodTexture(void);
extern void	M_SetCamera(CloseData *close, Matrix *world, Matrix *skew);

/* Utility macros */
#define 	M_FloatToByte(f) 	((uint32)((f)*255))
#define 	M_PackColor(r,g,b)	M_FloatToByte(r)<<16 | M_FloatToByte(g)<<8  | M_FloatToByte(b)

/* Case functions and data */
extern void	M_ClistManagerC(CloseData *);
extern void 	M_SetupDynLit(CloseData *);
extern void 	M_SetupDynLitTex(CloseData *);
extern void 	M_SetupDynLitTrans(CloseData *);
extern void 	M_SetupDynLitFog(CloseData *);
extern void 	M_SetupDynLitFogTex(CloseData *);
extern void 	M_SetupDynLitTransTex(CloseData *);
extern void 	M_SetupDynLitFogTrans(CloseData *);
extern void 	M_SetupDynLitSpecTex(CloseData *);
extern void 	M_SetupDynLitEnv(CloseData *);
extern void 	M_SetupDynLitFogEnv(CloseData *);
extern void 	M_SetupDynLitTransEnv(CloseData *);
extern void 	M_SetupDynLitSpecEnv(CloseData *);
extern void 	M_SetupPreLit(CloseData *);
extern void 	M_SetupPreLitTex(CloseData *);
extern void 	M_SetupPreLitTrans(CloseData *);
extern void 	M_SetupPreLitTransTex(CloseData *);
extern void 	M_SetupPreLitFog(CloseData *);
extern void 	M_SetupPreLitFogTex(CloseData *);
extern void 	M_SetupPreLitFogTrans(CloseData *);
extern void 	M_SetupPreLitEnv(CloseData *);
extern void 	M_SetupPreLitTransEnv(CloseData *);
extern void 	M_SetupPreLitFogEnv(CloseData *);

extern void 	M_DrawDynLitTex(void);
extern void 	M_LightReturnDynLitTex(void);
extern void 	M_DrawDynLit(void);
extern void 	M_LightReturnDynLit(void);
extern void 	M_DrawPreLitTex(void);
extern void 	M_LightReturnPreLitTex(void);
extern void 	M_DrawPreLit(void);
extern void 	M_LightReturnPreLit(void);
extern void 	M_DrawDynLitTrans(void);
extern void 	M_LightReturnDynLitTrans(void);
extern void 	M_DrawPreLitTrans(void);
extern void 	M_LightReturnPreLitTrans(void);
extern void 	M_DrawDynLitEnv(void);
extern void 	M_LightReturnDynLitEnv(void);
extern void 	M_DrawPreLitEnv(void);
extern void 	M_LightReturnPreLitEnv(void);

extern void 	M_SetupDynLitAA(CloseData *);
extern void 	M_SetupDynLitTexAA(CloseData *);
extern void 	M_SetupPreLitAA(CloseData *);
extern void 	M_SetupPreLitTexAA(CloseData *);

extern void 	M_DrawDynLitTexAA(void);
extern void 	M_LightReturnDynLitTexAA(void);
extern void 	M_DrawDynLitAA(void);
extern void 	M_LightReturnDynLitAA(void);
extern void 	M_DrawPreLitTexAA(void);
extern void 	M_LightReturnPreLitTexAA(void);
extern void 	M_DrawPreLitAA(void);
extern void 	M_LightReturnPreLitAA(void);

extern void	M_LightDir(void);
extern void	M_LightDirSpec(void);
extern void	M_LightDirSpecTex(void);
extern void	M_LightFog(void);
extern void	M_LightPoint(void);
extern void	M_LightSoftSpot(void);
extern void	M_LightFogTrans(void);
extern void	M_LightEnv(void);

#endif /* __MERCURY_H */




