/****
 * @(#) PortInit.c 95/11/01 1.58
 * Creates a GP and sets draw mode based on command line arguments.
 *
 ****/

#include <graphics/gp.h>
#include <graphics/gfxutils/putils.h>
#ifndef EXTERNAL_RELEASE
#include <graphics/sl.h>
#endif
#include <kernel/mem.h>
#include <graphics/graphics.h>
#include <graphics/view.h>
#include <graphics/bitmap.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>


/* Constants */

#define		kMaxNumScreens					3
#define		kMaxCmdListBuffers				5
#define		kDefaultNumScreens				2
#define		kDefaultScreenWidth				640
#define		kDefaultScreenHeight			480
#define		kDefaultScreenDepth				16
#define		kDefaultNumCmdListBufs			2
#define		kDefaultCmdListBufSize			(32*1024)	/* in words */
#define		kDefaultTransparencyMode		FALSE
#define		kDefaultDitherMode				FALSE
#define		kDefaultVideoInterpMode			FALSE


/* Globals */

static GP*			gp;
static uint32		gNumScreens;
static Item			viewItem;
static Item			gBitmaps[kMaxNumScreens+1];
static int32        gCurBuffer = 0;


/* Prototypes */

/*
static Err	AllocAllBitmaps			(int32 numFBs, int32 wide, int32 high,
									 int32 type, Item frameBuffs[],
									 Bitmap** zBuff);
									 */
void		gfxDriverInit			(void);
Bitmap*		gfxDriverAllocBitmap	(uint32 bmWidth, uint32 bmHeight, int32 type );
Err			gfxDriverSendCmdList	(Bitmap *fb, Bitmap *zb, void* list );
int32		gfxDriverGetIOIndex		(void* list);
static void	ParseArgs_Usage			(void);


#ifdef GFX_DRIVER_DEBUG
#define GfxDriverPrintf(x)	printf x
#else
#define GfxDriverPrintf(x)
#endif


/* Code */

static void ParseArgs_Usage(void)
{
    printf("InitGP: Usage: [-d] [-b] [-s] [-h] [app params]\n");
	printf("  -?:    Display this help\n");
	printf("  -b:    Enable transparency via dest. blend\n");
	printf("  -d:    Enable dithering\n");
	printf("  -640:  Wide display (xMax = 640)\n");
	printf("  -480:  Tall display (yMax = 480)\n");
	printf("  -320:  Narrow display (xMax = 320)\n");
	printf("  -240:  Short display (yMax = 240)\n");
	printf("  -32:   True color (32 bits per pixel)\n");
	printf("  -16:   False color (16 bits per pixel)\n");
	printf("  -h:    Hi-res (640x480x32bpp) shortcut\n");
	printf("  -i:    Enable video interpolation (320x240 only)\n");
	printf("  -s#:   Use # frame buffers (default = -s2)\n");
    exit(1);
}


GP* InitGP( int* argc, char* argv[] )
{
	AppInitStruct appData;
	Err theErr;

	theErr = GfxUtil_ParseArgs( argc, argv, &appData );
	assert(theErr == 0);
#ifndef BUILD_PARANOIA
	TOUCH(theErr);
#endif
	return GfxUtil_SetupGraphics( &appData );
}


Err GfxUtil_ParseArgs(int* argc, char *argv[], AppInitStruct* appData)
{
	char 	*cp, opt;
	int32	ArgC;
	char	**ArgV;
	char	**ArgOut;

#ifndef EXTERNAL_RELEASE
	SL_INIT();
	SL_PARSE(argc, argv, "++", "--");

	TRACEP("GfxUtil_ParseArgs", printf("(argc=%ld)\n", *argc));

#endif
	appData->numScreens = kDefaultNumScreens;
	appData->screenWidth = kDefaultScreenWidth;
	appData->screenHeight = kDefaultScreenHeight;
	appData->screenDepth = kDefaultScreenDepth;
	appData->numCmdListBufs = kDefaultNumCmdListBufs;
	appData->cmdListBufSize = kDefaultCmdListBufSize;
	appData->autoTransparency = kDefaultTransparencyMode;
	appData->enableDither = kDefaultDitherMode;
	appData->enableVidInterp = kDefaultVideoInterpMode;

	ArgC = *argc;
	ArgV = argv + 1;
	ArgOut = argv + 1;
	cp = *ArgV;							/* point to first argument */

	while (--ArgC >= 1) {				/* scan all args */
		if (*cp++ != '-') {				/* not an option? */
			*ArgOut++ = *ArgV++;		/* copy arg to output */
			continue;
		}

		opt = *cp++;					/* get the option letter */
		--(*argc);						/* eliminate this arg */

		switch (tolower(opt)) {
		case 'b':		/* Transparency enabled (via Dest Blending) */
			appData->autoTransparency = TRUE;
			break;

		case 'd':		/* Transparency enabled (via Dest Blending) */
			appData->enableDither = TRUE;
			break;

		case 's':					/* Num screens */
			if ((*cp >= '1') && (*cp <= (kMaxNumScreens+'0')))
				appData->numScreens = *cp - '0';
			printf("Using %d frame buffers\n", appData->numScreens);
			break;

		case '6':
			appData->screenWidth = 640;
			break;

		case '4':
			appData->screenHeight = 480;
			break;

		case '3':
			if (*cp++ == '2')
			{
				if (*cp == '0')
					appData->screenWidth = 320;
				else
					appData->screenDepth = 32;
			}
			break;
			
		case '2':
			appData->screenHeight = 240;
			break;

		case '1':
			appData->screenDepth = 16;
			break;

		case 'h':
			appData->screenWidth = 640;
			appData->screenHeight = 480;
			appData->screenDepth = 32;
			break;

		case 'i':
			appData->enableVidInterp = TRUE;
			break;

		case '?':
			--(*argc);					/* skip this arg */
			ParseArgs_Usage();
			break;

		default:
			*ArgOut++ = *ArgV;			/* copy this arg */
			++(*argc);                  /* and don't eliminate */
		}
		cp = *++ArgV;					/* next argument */
	}
	return GFX_OK;
}

GP* GfxUtil_SetupGraphics(const AppInitStruct* appData)
{
	int32			type;
	int32			xres, yres, depth;
	int32			viewTypeTagArg=0, avgModeTagArg0, avgModeTagArg1;
	Err				err;
	GState*			gs;
	int32			sigMask;

#ifndef EXTERNAL_RELEASE
	TRACEP("GfxUtil_SetupGraphics", );
#endif

	err = OpenGraphicsFolio();
	if (err < 0) {
		PrintfSysErr(err);
		return NULL;
	}

	xres = appData->screenWidth;
	yres = appData->screenHeight;
	depth = appData->screenDepth;


	gNumScreens = appData->numScreens;
	type = ( depth == 16 ) ?  BMTYPE_16 : BMTYPE_32;

	if ((xres==320) && (yres==240) && (depth==16)) viewTypeTagArg = VIEWTYPE_16;
	if ((xres==320) && (yres==480) && (depth==16)) viewTypeTagArg = VIEWTYPE_16_LACE;
	if ((xres==320) && (yres==240) && (depth==32)) viewTypeTagArg = VIEWTYPE_32;
	if ((xres==320) && (yres==480) && (depth==32)) viewTypeTagArg = VIEWTYPE_32_LACE;
	if ((xres==640) && (yres==240) && (depth==16)) viewTypeTagArg = VIEWTYPE_16_640;
	if ((xres==640) && (yres==480) && (depth==16)) viewTypeTagArg = VIEWTYPE_16_640_LACE;
	if ((xres==640) && (yres==240) && (depth==32)) viewTypeTagArg = VIEWTYPE_32_640;
	if ((xres==640) && (yres==480) && (depth==32)) viewTypeTagArg = VIEWTYPE_32_640_LACE;

	/* create graphics pipeline */

	gp = GP_Create(GP_SoftRender); /* use soft for rendering */

	/* create the command list buffers and render bitmaps */

	gs = GS_Create();
	err = GP_SetGState(gp,gs);
	if (err < 0) {
		PrintfSysErr(err);
		return NULL;
	}

	err = GS_AllocLists(gs, appData->numCmdListBufs, appData->cmdListBufSize);
	if (err < 0) {
		PrintfSysErr(err);
		return NULL;
	}

	err = GS_AllocBitmaps(gBitmaps, xres, yres, type, gNumScreens, 1);
	assert (err == 0);
#ifndef BUILD_PARANOIA
	TOUCH(err);
#endif
	GS_SetDestBuffer(gs, gBitmaps[0]);
	GS_SetZBuffer(gs, gBitmaps[gNumScreens]);

	avgModeTagArg0 = 0;
	avgModeTagArg1 = 0;
	if (appData->enableVidInterp) {
		avgModeTagArg0 |= AVG_DSB_0;
		avgModeTagArg1 |= AVG_DSB_1;
		printf("Using pixel interpolation\n");
		if (xres == 320) avgModeTagArg0 |= AVGMODE_H;
		if (xres == 320) avgModeTagArg1 |= AVGMODE_H;
		if (yres == 240) avgModeTagArg0 |= AVGMODE_V;
		if (yres == 240) avgModeTagArg1 |= AVGMODE_V;
	}


	sigMask = AllocSignal(0);
	GS_SetVidSignal(gs, sigMask);

	viewItem = CreateItemVA( MKNODEID(NST_GRAPHICS, GFX_VIEW_NODE),
							 VIEWTAG_BITMAP, gBitmaps[0],
							 VIEWTAG_VIEWTYPE, viewTypeTagArg,
							 VIEWTAG_AVGMODE, avgModeTagArg0,
							 VIEWTAG_AVGMODE, avgModeTagArg1,
							 VIEWTAG_DISPLAYSIGNAL, sigMask,
							 TAG_END );

	assert(viewItem >= 0);
	AddViewToViewList( viewItem, 0 );

	/* Setup GP */

	{
		Box2 b;

		printf("Setting display to %dx%d, %d bpp\n", xres, yres, depth);
		b.min.x = 0; b.min.y = 0; b.max.x = xres; b.max.y = yres;
		GP_SetViewport( gp, &b );
	}

	/* turn lighting on */

	GP_Enable(gp, GP_Lighting );

	/* Z buffer hidden surface elimination */

	GP_SetHiddenSurf(gp, GP_ZBuffer);

	if (appData->autoTransparency) {
		TexBlend*	txbDefault;
				
		txbDefault = GP_GetTxbDefault(gp);
		assert(txbDefault);
		Txb_SetDblEnableAttrs( txbDefault,
			DBL_ZBuffEnable | DBL_ZBuffOutEnable |
			DBL_BlendEnable | DBL_SrcInputEnable |
			DBL_AlphaDestOut | DBL_RGBDestOut );
		Txb_SetDblDiscard( txbDefault, DBL_DiscardAlpha0 | DBL_DiscardZClipped );
		Txb_SetDblZCompareControl( txbDefault, DBL_PixOutOnSmallerZ |
			DBL_ZUpdateOnSmallerZ | DBL_PixOutOnEqualZ );
		Txb_SetDblAInputSelect( txbDefault, DBL_ASelectTexColor );
		Txb_SetDblAMultCoefSelect( txbDefault, DBL_MASelectTexAlpha );
		Txb_SetDblAMultRtJustify( txbDefault, 0 );
		Txb_SetDblBInputSelect( txbDefault, DBL_BSelectSrcColor );
		Txb_SetDblBMultCoefSelect( txbDefault, DBL_MASelectTexAlphaComplement );
		Txb_SetDblBMultRtJustify( txbDefault, 0 );
		Txb_SetDblALUOperation( txbDefault, DBL_AddClamp );
		Txb_SetDblFinalDivide( txbDefault, 0 );
		Txb_SetDblSrcBaseAddr( txbDefault, (int32)GP_GetDestBuffer(gp) );
		Txb_SetDblSrcPixels32Bit( txbDefault, (depth==32) );
		Txb_SetDblSrcXStride( txbDefault, xres );
		Txb_SetDblSrcXOffset( txbDefault, 0 );
		Txb_SetDblSrcYOffset( txbDefault, 0 );
		printf("Destination blending enabled\n");
	}

	if (appData->enableDither && (appData->screenDepth<32)) {
		TexBlend* txbDefault;
		int32 attrs;

		/* 
		   Using a dither matrix which looks like:
		   		-4	 0	-3	 1		= 0xC0D1
				 2	-2	 3	-1		= 0x2E3F
				-2	 1	-3	 0		= 0xE1D0
				 3	 0	 2	-1		= 0x302F
		   Each of these numbers are packed into a 4-bit signed int,
		   then the top 8 are placed in MatrixA, the bottom 8 in MatrixB.
		*/
		txbDefault = GP_GetTxbDefault(gp);
		attrs = Txb_GetDblEnableAttrs( txbDefault );
		Txb_SetDblEnableAttrs( txbDefault, attrs | DBL_DitherEnable );
		Txb_SetDblDitherMatrixA( txbDefault, 0xC0D12E3F );
		Txb_SetDblDitherMatrixB( txbDefault, 0xE1D0302F );
		GP_Enable( gp, GP_Dithering );
	} else {
		GP_Disable( gp, GP_Dithering );
	}

	return gp;
}


void SwapBuffers(void)
{
	TexBlend* defaultTxb;
	BitmapPtr newDestBM;
	GState* gs = GP_GetGState(gp);

#ifndef EXTERNAL_RELEASE
	TRACEP("SwapBuffers", printf("()\n") );
#endif
	GP_Flush(gp);
	if (gNumScreens > 1) {
		GS_WaitIO(gs);
		ModifyGraphicsItemVA( viewItem,
							  VIEWTAG_BITMAP, gBitmaps[gCurBuffer],
							  TAG_END );
		gCurBuffer = (gCurBuffer + 1) % gNumScreens;
		newDestBM = (BitmapPtr)LookupItem(gBitmaps[gCurBuffer]);
		GS_SetDestBuffer(gs, gBitmaps[gCurBuffer]);
		GS_BeginFrame(gs);
		defaultTxb = GP_GetTxbDefault(gp);
		if ( defaultTxb )
			Txb_SetDblSrcBaseAddr( defaultTxb, (int32)(newDestBM->bm_Buffer) );
	}
}

void NextRendBuffer(void)
{
	TexBlend* defaultTxb;
	BitmapPtr newDestBM;
	GState* gs = GP_GetGState(gp);

	if (gNumScreens > 1) {

		newDestBM = (BitmapPtr)LookupItem(gBitmaps[gCurBuffer]);
		GS_SetDestBuffer(gs, gBitmaps[gCurBuffer]);
		GS_BeginFrame(gs);
		defaultTxb = GP_GetTxbDefault(gp);
		if ( defaultTxb )
			Txb_SetDblSrcBaseAddr( defaultTxb, (int32)(newDestBM->bm_Buffer) );
	}
}

void NextViewBuffer(void)
{
	if (gNumScreens > 1) {

		ModifyGraphicsItemVA( viewItem,
							  VIEWTAG_BITMAP, gBitmaps[gCurBuffer],
							  TAG_END );
	}
}

void WaitTE(void)
{
	GState* gs = GP_GetGState(gp);

	GS_WaitIO(gs);
}

void IncCurBuffer(void)
{
	gCurBuffer = (gCurBuffer + 1) % gNumScreens;
}

void SetGP(int32 rm)
{
	(void)&rm;
	/* nothing to do only 1 GP */
}

GP* GetGP(int32 rm)
{
	(void)&rm;
	return gp;
}

GP* GetCurrentGP(void)
{
	return gp;
}

