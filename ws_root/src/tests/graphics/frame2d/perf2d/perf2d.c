/******************************************************************************
**
**  @(#) perf2d.c 96/07/09 1.4
**
******************************************************************************/

/*
 * Graphics test program to exercise the 2D API
 */

#include <graphics/graphics.h>
#include <graphics/bitmap.h>
#include <graphics/view.h>
#include <graphics/frame2d/f2d.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <misc/event.h>
#include <kernel/debug.h>
#include <kernel/mem.h>

#define DBUG(x) {printf x;}
#define DBUG2(x) /* {printf x;} */
#define CKERR(x,y) {int32 err=(x);if(err<0){printf("Error - %s\n",y);PrintfSysErr(x); exit(1);}}


#define VTX_FORMAT_SPR (GEO_TexCoords|GEO_Locations)


#define FBWIDTH 640
#define FBHEIGHT 480
#define DEPTH 32

#define W2 (FBWIDTH*2)
#define WD2 (FBWIDTH/2)
#define H2 (FBHEIGHT*2)
#define HD2 (FBHEIGHT/2)

#define NUMPASSES 25000

Point2 corners[4] = {
  {10., 10.},
  {210., 10.},
  {10., 210.},
  {270., 180.},
};




gfloat sizetable[] = {
  1., .75, .5, .4, .3, .2, .15, .10, .05,
};

#define NUMSIZES (sizeof(sizetable)/sizeof(*sizetable))




  GState*	gs;			/* -> current graphics pipeline */
  Item      bitmaps[3];
  SpriteObj	*sp, *sp2, *sparray[6];
  GridObj	*gr;
  int		i, width, height, rw, rh;
  Point2	p;
  ControlPadEventData cped;
  uint32	memdebugflag=0;
  TimeVal	timein, timeout;
  gfloat	dt;

  int32			buffSignal;
  Item			viewItem;
  uint32		viewType;
  uint32		bmType = (DEPTH==16)?BMTYPE_16:BMTYPE_32;

  char *fname="leaf.utf";
  gfloat pi;








uint32
BestViewType(uint32 xres, uint32 yres, uint32 depth)
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


void
DefaultTabInit (SpriteObj *sp)
{
  Spr_SetTextureAttr (sp, TXA_ColorOut, 1);
  Spr_SetTextureAttr (sp, TXA_BlendOp, 1);
  Spr_SetTextureAttr (sp, TXA_FirstColor, 1);
  Spr_SetTextureAttr (sp, TXA_SecondColor, 3);
  Spr_SetTextureAttr (sp, TXA_ThirdColor, 2);
  Spr_SetTextureAttr (sp, TXA_AlphaOut, 1);
  Spr_SetTextureAttr (sp, TXA_FirstAlpha, 2);
  Spr_SetTextureAttr (sp, TXA_SecondAlpha, 1);
  Spr_SetTextureAttr (sp, TXA_TextureEnable, 1);
  Spr_SetTextureAttr (sp, TXA_BlendColorSSB0, 0xffffffff);
  Spr_SetTextureAttr (sp, TXA_BlendColorSSB1, 0xffffffff);
  if (Spr_GetPIP(sp)) {
    Spr_SetTextureAttr (sp, TXA_PipColorSelect, 2);	/* If we have a PIP, use the PIP color */
  } else {
    Spr_SetTextureAttr (sp, TXA_PipColorSelect, 1);	/* If no PIP, use texture color */
  }
  Spr_SetTextureAttr (sp, TXA_PipAlphaSelect, 0);	/* Default to using constant alpha */
  Spr_SetTextureAttr (sp, TXA_PipSSBSelect, 0);	/* Default to using constant SSB */
}


void
AlphaInit (SpriteObj *sp)
{
  Spr_SetDBlendAttr (sp, DBLA_EnableAttrs,
		     DBL_BlendEnable|DBL_SrcInputEnable|DBL_AlphaDestOut|
		     DBL_RGBDestOut);
  /* Spr_SetDBlendAttr (sp, DBLA_Discard, DBL_DiscardAlpha0); */
  Spr_SetDBlendAttr (sp, DBLA_DSBSelect, DBL_DSBSelectObjSSB);
  Spr_SetDBlendAttr (sp, DBLA_AInputSelect, DBL_ASelectTexColor);
  Spr_SetDBlendAttr (sp, DBLA_AMultCoefSelect, DBL_MASelectTexAlpha);
  Spr_SetDBlendAttr (sp, DBLA_AMultRtJustify, 0);
  Spr_SetDBlendAttr (sp, DBLA_BInputSelect, DBL_BSelectSrcColor);
  Spr_SetDBlendAttr (sp, DBLA_BMultCoefSelect, DBL_MASelectTexAlphaComplement);
  Spr_SetDBlendAttr (sp, DBLA_BMultRtJustify, 0);
  Spr_SetDBlendAttr (sp, DBLA_ALUOperation, DBL_AddClamp);

  if (sp->spr_TexBlend && sp->spr_TexBlend->tb_txData &&
      (sp->spr_TexBlend->tb_txData->expansionFormat&EXP_HASALPHA)) {
    DBUG (("texture alpha\n"));
    /* Use texture alpha if we have texture alpha */
    Spr_SetTextureAttr (sp, TXA_PipAlphaSelect, 1);
  } else if (Spr_GetPIP(sp)) {
    DBUG (("PIP alpha\n"));
    /* If we have a PIP, use the PIP alpha */
    Spr_SetTextureAttr (sp, TXA_PipAlphaSelect, 2);
  } else {
    DBUG (("Constant alpha\n"));
    /* If no PIP, use constant alpha */
    Spr_SetTextureAttr (sp, TXA_PipAlphaSelect, 0);
  }
}


void Halftone (SpriteObj *sp)
{
  Spr_SetDBlendAttr (sp, DBLA_AInputSelect, 0);
  Spr_SetDBlendAttr (sp, DBLA_AMultCoefSelect, DBL_MASelectConst);
  Spr_SetDBlendAttr (sp, DBLA_AMultConstSSB0, 0x808080);
  Spr_SetDBlendAttr (sp, DBLA_AMultConstSSB1, 0x808080);
  Spr_SetDBlendAttr (sp, DBLA_BInputSelect, 0);
  Spr_SetDBlendAttr (sp, DBLA_BMultCoefSelect, DBL_MBSelectConst);
  Spr_SetDBlendAttr (sp, DBLA_BMultConstSSB0, 0x808080);
  Spr_SetDBlendAttr (sp, DBLA_BMultConstSSB1, 0x808080);
  Spr_SetDBlendAttr (sp, DBLA_ALUOperation, 0x00);
}


void NoBlend (SpriteObj *sp)
{
  Spr_SetDBlendAttr (sp, DBLA_EnableAttrs, DBL_AlphaDestOut|DBL_RGBDestOut);
}


void DoPerf (char *str, gfloat scale)
{
  printf ("%s at scale %f\n", str, scale);

  /* Clear the frame buffer */
  CLT_ClearFrameBuffer (gs, .2, .2, .8, 0., TRUE, TRUE);
  CLT_SetSrcToCurrentDest (gs);

  /* Send the command list to be drawn */
  gs->gs_SendList(gs);
  GS_WaitIO(gs);

  Spr_ResetCorners (sp, SPR_TOPLEFT);
  Spr_Scale (sp, scale, scale);

  width = Spr_GetWidth(sp)*scale;
  height = Spr_GetHeight(sp)*scale;
  rw = FBWIDTH-width;
  rh = FBHEIGHT-height;

  /* get time in */
  SampleSystemTimeTV (&timein);
  for (i=0; i<NUMPASSES; i++) {
    p.x = rand()%(rw-10) + 5; p.y = rand()%(rh-10) + 5;
    Spr_SetPosition (sp, &p);
    F2_Draw (gs, sp);
  }
  /* Send the command list to be drawn */
  gs->gs_SendList(gs);
  GS_WaitIO(gs);
  /* get time out */
  SampleSystemTimeTV (&timeout);
  SubTimes (&timein, &timeout, &timeout);
  dt = timeout.tv_sec + (timeout.tv_usec*.000001);
  printf ("Elapsed time = %f secs., # sprites/sec = %f\n", dt, NUMPASSES/dt);
  printf ("Sprite output dimensions: %dx%d, total pixels: %d, Mpixels/sec = %f\n",
	  width, height, width*height, width*height*NUMPASSES/(dt*1000000.));


  /* Wait for exit */
  while( !GetControlPad (1, FALSE, &cped) ) ;
  while( !GetControlPad (1, FALSE, &cped) ) ;

}



main(int argc, char **argv)
{
  /* Initialization */
  for (i=1; i<argc; i++) {
    if (*argv[i]=='-') {
      switch (tolower(argv[i][1])) {
      case 'm':
	memdebugflag = true;
	break;
      default:
	printf ("Unrecognized option '%s'\n", argv[i]);
      }
    } else {
      fname = argv[i];
    }
  }

  if (memdebugflag) {
    CreateMemDebug(NULL);
    ControlMemDebug(MEMDEBUGF_ALLOC_PATTERNS |
		    MEMDEBUGF_FREE_PATTERNS |
		    MEMDEBUGF_PAD_COOKIES);
    sp2 = Spr_Create(NULL);
    Spr_LoadUTF (sp2, fname);
    Spr_Delete (sp2);
    DumpMemDebug (NULL);
    DeleteMemDebug ();
  }

  pi = 4.*atanf(1.);

  printf ("Create GState\n");
  OpenGraphicsFolio ();
  buffSignal = AllocSignal (0);
  gs = GS_Create ();
  if (!gs) {
    printf ("Error creating GState\n");
    exit (1);
  }

  /* Initialize the EventBroker. */
  if( InitEventUtility(1, 0, LC_ISFOCUSED) < 0 ) {
      printf("Error in InitEventUtility\n");
      exit(1);
  }

  viewType = BestViewType(FBWIDTH, FBHEIGHT, DEPTH);

  printf ("Alloc bitmaps\n");
  GS_AllocLists(gs, 2, 2048);
  GS_AllocBitmaps(bitmaps, FBWIDTH, FBHEIGHT, bmType, 2, 1);
  GS_SetDestBuffer(gs, bitmaps[0]);
  GS_SetZBuffer(gs, bitmaps[2]);
  viewItem = CreateItemVA(MKNODEID(NST_GRAPHICS, GFX_VIEW_NODE),
			  VIEWTAG_VIEWTYPE, viewType,
			  VIEWTAG_DISPLAYSIGNAL, buffSignal,
			  VIEWTAG_BITMAP, bitmaps[0],
			  TAG_END );
  CKERR(viewItem,"can't create view item");
  AddViewToViewList( viewItem, 0 );

  printf ("Clear frame buffer\n");
  /* Initially clear the frame buffer to black and make it visible */
  CLT_ClearFrameBuffer (gs, 0., 0., 0., 0., TRUE, TRUE);
  gs->gs_SendList(gs);
  GS_WaitIO(gs);
  printf ("Bring view to front\n");
  ModifyGraphicsItemVA(viewItem,
		       VIEWTAG_BITMAP, bitmaps[0],
		       TAG_END );


  /* Now start performance analysis */

  printf ("Load sprite file %s\n", fname);
  sp = Spr_Create(0);
  Spr_LoadUTF (sp, fname);

  printf ("Set default attributes\n");
  DefaultTabInit (sp);

  {
    int j;
    for (j=0; j<NUMSIZES; j++) {
      DoPerf ("Test with dblend off", sizetable[j]);
    }
  }

  AlphaInit (sp);
  Halftone (sp);

  {
    int j;
    for (j=0; j<NUMSIZES; j++) {
      DoPerf ("Test with dblend on", sizetable[j]);
    }
  }

  KillEventUtility();

  printf ("All done\n");
  exit (0);
}



