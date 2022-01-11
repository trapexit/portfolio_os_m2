
/******************************************************************************
**
**  @(#) gsquare.c 96/08/03 1.16
**
******************************************************************************/

/*  This program displays a simple square to illustrate
 *  How to create a command list and send it to the triangle engine.
 *	The square toggles between colored vertices and texture mapped vertices.
 *
 *  Disclaimer: An actual program would not use some techniques
 *  used here such as:
 *		waiting for the TE to complete after sending a command list
 *		(better to double buffer the command lists)
 *
 *		knowing that all the commands fit in allocated command list
 *		(usually due to memory constraints a smaller buffer is used
 *		 which must be sent to the TE when filled)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <kernel/types.h>
#include <kernel/task.h>
#include <graphics/clt/clt.h>
#include <graphics/clt/gstate.h>
#include <graphics/graphics.h>
#include <graphics/view.h>
#include <kernel/io.h>
#include <kernel/time.h>
#include <misc/event.h>
#include <assert.h>
#include <strings.h>
#include "texload.h"

#define DBGX(x) /* printf x */


void DestBlendInit(GState* gs);


/* Returned Error code
   = 0 success
   = 1 failure
*/
/* Utility */

#define FBWIDTH				320
#define FBHEIGHT			240
#define DEPTH				32
#define NUM_FRAME_BUFFERS	2
#define	USE_Z				1
#define SIZE				80.0

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

main(int argc, char **argv)
{
	Item	bitmaps[NUM_FRAME_BUFFERS+USE_Z];
	gfloat	br, bg, bb;
	GState*	gs;
	gfloat	xpos, ypos;
	gfloat	deltax, deltay;
	gfloat	rndx, rndy;
	int32	buffSignal;
	Item	viewItem;
	uint32 	viewType = BestViewType(FBWIDTH, FBHEIGHT, DEPTH);
	uint32	bmType = (DEPTH==32 ? BMTYPE_32 : BMTYPE_16);
	uint32	curFBIndex = 0;
	bool	done = 0;
	bool	texturing = 1;
	TextureSnippets snips;		/* Will hold cmd lists to load the texture */
	float	U, V, W = 0.5;
	ControlPadEventData cped;

	(void)argc;
	(void)argv;

	OpenGraphicsFolio();

	InitEventUtility(1, 0, FALSE);

	buffSignal = AllocSignal(0);

	gs = GS_Create();
	GS_AllocLists(gs, 2, 1024);		/* 1024 words = 4096 bytes */
	GS_AllocBitmaps(bitmaps, FBWIDTH, FBHEIGHT, bmType, 2, 1);
	GS_SetDestBuffer(gs, bitmaps[0]);
	GS_SetZBuffer(gs, bitmaps[NUM_FRAME_BUFFERS]);
	GS_SetVidSignal(gs, buffSignal);

	viewItem = CreateItemVA(MKNODEID(NST_GRAPHICS, GFX_VIEW_NODE),
							VIEWTAG_VIEWTYPE, viewType,
							VIEWTAG_RENDERSIGNAL, buffSignal,
							VIEWTAG_BITMAP, bitmaps[0],
							TAG_END );
	if (viewItem < 0) { PrintfSysErr(viewItem); exit(1); }
	AddViewToViewList( viewItem, 0 );

	DestBlendInit(gs);

	/* first 2 triangles to clear the frame buffer to a background color */

	xpos = 0.0;
	ypos = 0.0;
	deltax = deltay = 1.0;
	rndx = rndy = 1.0;

	br = bg = 0.2;
	bb = 0.8;

	/* Load texture from disk into TRAM, including its PIP if there is one */
	CLT_InitSnippet( &snips.dab );
	CLT_InitSnippet( &snips.tab );
	snips.lcb = NULL;
	snips.pip = NULL;
	snips.txdata = NULL;

	LoadTexture( &snips, (argc > 1 ? argv[1] : "cow.utf") );
	UseTxLoadCmdLists( gs, &snips );
	U = (float)(snips.lcb->XSize << (snips.txdata->maxLOD - 1)) * W;
	V = (float)(snips.lcb->YSize << (snips.txdata->maxLOD - 1)) * W;
	while(!done) {
		GS_BeginFrame(gs);
		CLT_CopySnippetData(GS_Ptr(gs), &CltNoTextureSnippet);
		CLT_ClearFrameBuffer (gs, br, bg, bb, 0., TRUE, TRUE);
		CLT_Sync(GS_Ptr(gs));

		/* Now a colorful square */
		if (texturing) {
			CLT_ClearRegister(gs->gs_ListPtr, TXTTABCNTL,
							  FV_TXTTABCNTL_COLOROUT_MASK |
							  FV_TXTTABCNTL_ALPHAOUT_MASK);
			CLT_SetRegister(gs->gs_ListPtr, TXTTABCNTL,
							CLT_SetConst(TXTTABCNTL,COLOROUT,TEXCOLOR) |
							CLT_SetConst(TXTTABCNTL,ALPHAOUT,TEXALPHA));
			CLT_SetRegister(gs->gs_ListPtr, TXTADDRCNTL,
							FV_TXTADDRCNTL_TEXTUREENABLE_MASK);
			CLT_TRIANGLE(GS_Ptr(gs), 1, RC_STRIP, 1, 1, 0, 4);
			CLT_VertexUvW(GS_Ptr(gs), xpos,      ypos,      0.0, 0.0, W);
			CLT_VertexUvW(GS_Ptr(gs), xpos,      ypos+SIZE, 0.0, V,   W);
			CLT_VertexUvW(GS_Ptr(gs), xpos+SIZE, ypos,      U,   0.0, W);
			CLT_VertexUvW(GS_Ptr(gs), xpos+SIZE, ypos+SIZE, U,   V,   W);
		} else {
			CLT_ClearRegister(gs->gs_ListPtr, TXTTABCNTL,
							  FV_TXTTABCNTL_COLOROUT_MASK |
							  FV_TXTTABCNTL_ALPHAOUT_MASK);
			CLT_SetRegister(gs->gs_ListPtr, TXTTABCNTL,
							CLT_SetConst(TXTTABCNTL,COLOROUT,PRIMCOLOR) |
							CLT_SetConst(TXTTABCNTL,ALPHAOUT,PRIMALPHA));
			CLT_ClearRegister(gs->gs_ListPtr, TXTADDRCNTL,
							  FV_TXTADDRCNTL_TEXTUREENABLE_MASK);
			CLT_TRIANGLE(GS_Ptr(gs), 1, RC_STRIP, 0, 0, 1, 4);
			CLT_VertexRgba(GS_Ptr(gs), xpos,      ypos,      1.0, 0.2, 0.2, 1.0);
			CLT_VertexRgba(GS_Ptr(gs), xpos,      ypos+SIZE, 0.2, 1.0, 0.2, 1.0);
			CLT_VertexRgba(GS_Ptr(gs), xpos+SIZE, ypos,      0.2, 0.2, 1.0, 1.0);
			CLT_VertexRgba(GS_Ptr(gs), xpos+SIZE, ypos+SIZE, 1.0, 0.2, 1.0, 1.0);
		}
		CLT_Pause(GS_Ptr(gs));

		gs->gs_SendList(gs);

		/* This is the swap buffer stuff */
		/* wait for all TE io to complete in case we are rendering too fast */
		GS_WaitIO(gs);
		ModifyGraphicsItemVA(viewItem,
							 VIEWTAG_BITMAP, bitmaps[curFBIndex],
							 TAG_END );

		curFBIndex = 1 - curFBIndex;
		GS_SetDestBuffer(gs, bitmaps[curFBIndex]);

		if (((xpos+SIZE) >= 320.0) || (xpos <= 0.)) {
			texturing = 1 - texturing;
			rndx += 1.0;
			if (rndx>10.0) rndx = 1.0;
			deltax = -deltax ;
			xpos += deltax*rndx;
		}

		xpos += deltax;

		if (((ypos + SIZE) >= 240.0) || (ypos <= 0.)) {
			texturing = 1 - texturing;
			rndy += 1.0;
			if (rndy > 10.0) rndy = 1.0;
			deltay = - deltay;
			ypos += deltay*rndy;
		}

		ypos += deltay;

		if (GetControlPad(1, FALSE, &cped))
			done = TRUE;
	}
	CloseGraphicsFolio ();
}

void DestBlendInit(GState* gs)
{
	uint32	c0;

	CLT_DBUSERCONTROL(GS_Ptr(gs), 0, 0, 0, 0, 0, 0, 0,
					  CLT_Const(DBUSERCONTROL, DESTOUTMASK, ALPHA) |
					  CLT_Const(DBUSERCONTROL, DESTOUTMASK, RED) |
					  CLT_Const(DBUSERCONTROL, DESTOUTMASK, GREEN) |
					  CLT_Const(DBUSERCONTROL, DESTOUTMASK, BLUE) );
	CLT_DBDISCARDCONTROL(GS_Ptr(gs), 0, 0, 0, 0);

	CLT_DBSRCBASEADDR(GS_Ptr(gs), 0);
	CLT_DBSRCXSTRIDE(GS_Ptr(gs), 320);
	CLT_DBSRCOFFSET(GS_Ptr(gs), 0, 0);
	CLT_DBXYWINCLIP(GS_Ptr(gs), 0, 320, 0, 240);
	CLT_DBZCNTL(GS_Ptr(gs), 0, 0, 0, 0, 1, 1);
	CLT_DBZOFFSET(GS_Ptr(gs), 0, 0);
	CLT_DBCONSTIN(GS_Ptr(gs), 0xff, 0xff, 0xff);
	CLT_DBAMULTCNTL(GS_Ptr(gs),
					CLT_Const(DBAMULTCNTL, AINPUTSELECT, TEXCOLOR),
					CLT_Const(DBAMULTCNTL, AMULTCOEFSELECT, CONST),
					CLT_Const(DBAMULTCNTL, AMULTCONSTCONTROL, TEXSSB),
					0);
	c0 = CLA_DBAMULTCONSTSSB1(0x40, 0x40, 0x40);
	CLT_DBAMULTCONST(GS_Ptr(gs), c0, c0);
	CLT_DBDESTALPHACNTL(GS_Ptr(gs),
						CLT_Const(DBDESTALPHACNTL,DESTCONSTSELECT,TEXALPHA));
	CLT_DBDESTALPHACONST(GS_Ptr(gs), 0, 0x3f);
	CLT_DBALUCNTL(GS_Ptr(gs), CLT_Const(DBALUCNTL,ALUOPERATION,A_PLUS_B), 0);
	CLT_DBSSBDSBCNTL(GS_Ptr(gs), 1, CLT_Const(DBSSBDSBCNTL,DSBSELECT,CONST));
}
