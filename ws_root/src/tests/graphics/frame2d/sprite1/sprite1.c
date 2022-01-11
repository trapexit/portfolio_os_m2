/******************************************************************************
**
**  @(#) sprite1.c 96/07/09 1.39
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


#define FBWIDTH 320
#define FBHEIGHT 240
#define DEPTH 32

#define W2 (FBWIDTH*2)
#define WD2 (FBWIDTH/2)
#define H2 (FBHEIGHT*2)
#define HD2 (FBHEIGHT/2)

Point2 corners[4] = {
  {10., 10.},
  {210., 10.},
  {10., 210.},
  {270., 180.},
};

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
  Spr_SetDBlendAttr (sp, DBLA_Discard, DBL_DiscardAlpha0);
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



main(int argc, char **argv)
{
  GState*	gs;			/* -> current graphics pipeline */
  Item      bitmaps[3];
  SpriteObj	*sp, *sp2, *sparray[6];
  GridObj	*gr;
  int		i;
  ControlPadEventData cped;
  uint32	memdebugflag=0;

  int32			buffSignal;
  Item			viewItem;
  uint32		viewType = BestViewType(FBWIDTH, FBHEIGHT, DEPTH);
  uint32		bmType = (DEPTH==16)?BMTYPE_16:BMTYPE_32;

  char *fname="leaf.utf";
  gfloat pi;

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



  printf ("Clear frame buffer\n");
  CLT_ClearFrameBuffer (gs, .2, .2, .8, 0., TRUE, TRUE);	/* clear the frame buffer */
  printf ("Set src to current dest\n");
  CLT_SetSrcToCurrentDest (gs);

  printf ("Create sprite and grid objects\n");
  /* create sprite and grid objects */
  sp = Spr_Create(0);
  sp2 = Spr_CreateShort (0);

  printf ("Load sprites\n");
  Spr_LoadUTF (sp, fname);
  Spr_LoadUTF (sp2, fname);

  printf ("Set default attributes\n");
  DefaultTabInit (sp);

  Spr_ResetCorners (sp, SPR_TOPLEFT);

  printf ("(%g,%g) (%g,%g) \n(%g,%g) (%g,%g)\n",
	  sp->spr_Corners[0].x, sp->spr_Corners[0].y,
	  sp->spr_Corners[1].x, sp->spr_Corners[1].y,
	  sp->spr_Corners[2].x, sp->spr_Corners[2].y,
	  sp->spr_Corners[3].x, sp->spr_Corners[3].y);

  printf ("sprite @ %lx\n", sp);

  Spr_Translate (sp, 50, 50);

  printf ("Draw the sprite\n");

  F2_Draw (gs, sp);

  /* Now draw the sprite again using alpha blend */
  Spr_Translate (sp, 100, 0);
  AlphaInit (sp);
  F2_Draw (gs, sp);

  printf ("Rotate sprite 90 degrees\n");
  Spr_Rotate (sp, -90.0);
  printf ("Set for halftone drawing\n");
  Halftone (sp);
  F2_Draw (gs, sp);

  printf ("Send command list\n");
  /* Send the command list to be drawn */
  gs->gs_SendList(gs);
  printf ("Wait on command list\n");
  GS_WaitIO(gs);


  printf ("Wait for button press and release\n");
  /* wait for an event */
  while( !GetControlPad (1, FALSE, &cped) ) ;
  while( !GetControlPad (1, FALSE, &cped) ) ;


  {
    Bitmap* bm = (Bitmap*)LookupItem(GS_GetDestBuffer(gs));
    Point2 p1, p2, p3;
    p1.x = 40.;  p1.y = 40.;
    p2.x = 220.; p2.y = 120.;
    p3.x = 80.;  p3.y = 80.;
    F2_CopyRect (gs, bm->bm_Buffer, bm->bm_Width, bm->bm_Type, &p1, &p2, &p3);
    gs->gs_SendList(gs);
    GS_WaitIO(gs);
  }

  /* wait for an event */
  while( !GetControlPad (1, FALSE, &cped) ) ;
  while( !GetControlPad (1, FALSE, &cped) ) ;

  {
    Point2 p2;
    CLT_ClearFrameBuffer (gs, .2, .2, .8, 0., TRUE, TRUE);	/* clear the frame buffer */
    CLT_SetSrcToCurrentDest (gs);

    NoBlend (sp2);
    sparray[0] = sp;
    sparray[1] = sp2;
    sparray[2] = sp;
    sparray[3] = sp2;
    sparray[4] = sp;
    sparray[5] = sp2;
    gr = Gro_Create (0);
    Gro_SetWidth (gr, 3);
    Gro_SetHeight (gr, 2);
    p2.x = 20., p2.y = 20., Gro_SetPosition (gr, &p2);
    p2.x = 90., p2.y = 0., Gro_SetHDelta (gr, &p2);
    p2.x = 0., p2.y = 70., Gro_SetVDelta (gr, &p2);
    Gro_SetSpriteArray (gr, sparray);
    F2_Draw (gs, gr);

    /* Send the command list to be drawn */
    gs->gs_SendList(gs);
    GS_WaitIO(gs);
  }

  /* wait for an event */
  while( !GetControlPad (1, FALSE, &cped) ) ;
  while( !GetControlPad (1, FALSE, &cped) ) ;

  {
    Point2 p2;
    p2.x = 10.;
    p2.y = 10.;
    Spr_SetPosition (sp,&p2);
    Spr_ResetCorners (sp,SPR_TOPLEFT);
    Spr_Scale (sp, 2.0, 2.0);
    for (i=0; i<21; i++) {
      CLT_ClearFrameBuffer (gs, .2, .8, .2, 0., TRUE, TRUE);	/* clear fb to green */
      CLT_SetSrcToCurrentDest (gs);

      F2_Draw (gs, sp);
      Spr_Translate (sp,10.,10.);
      Spr_Rotate (sp,-18.);
      /*       Spr_Scale (sp,.9,.9); */
      gs->gs_SendList(gs);
      GS_WaitIO(gs);
      /* wait for an event */
      while( !GetControlPad (1, FALSE, &cped) ) ;
      while( !GetControlPad (1, FALSE, &cped) ) ;
    }
  }


  AlphaInit (sp);
  Spr_SetHSlice (sp, 8);
  Spr_SetVSlice (sp, 8);
  Spr_MapCorners (sp, corners, SPR_CENTER);

  for (i=0; i<10; i++) {
    CLT_ClearFrameBuffer (gs, .8, .2, .2, 0., TRUE, TRUE);	/* clear frame buffer to red */
    CLT_SetSrcToCurrentDest (gs);
    F2_Draw (gs, sp);
    /* Send the command list to be drawn */
    GS_SendList (gs);
    GS_WaitIO(gs);
    /* wait for an event */
    while( !GetControlPad (1, FALSE, &cped) ) ;
    while( !GetControlPad (1, FALSE, &cped) ) ;
    Spr_Scale (sp, 1.1, 1.1);
  }

  {
    Point2 p1, p2;
    Color4 c0, c1;
    p1.x = 100.;
    p1.y = 100.;
    p2.x = 200.;
    p2.y = 200.;
    c0.r = 0.; c0.g = 0.; c0.b = 0.; c0.a = 1.0;
    c1.r = 1.; c1.g = 1.; c1.b = 1.; c1.a = 1.0;
    DBUG2 (("line (%g,%g), (%g,%g)\n", p1.x, p1.y, p2.x, p2.y));
    F2_DrawLine (gs, &p1, &p2, &c0, &c1);
    gs->gs_SendList(gs);
    GS_WaitIO(gs);
  }

  /* wait for an event */
  while( !GetControlPad (1, FALSE, &cped) ) ;
  while( !GetControlPad (1, FALSE, &cped) ) ;

  {
    Point2 p[2];
    Color4 c[2];
    int32 i;

    CLT_ClearFrameBuffer (gs, .2, .8, .2, 0., TRUE, TRUE);	/* clear fb to green */
    CLT_SetSrcToCurrentDest (gs);
    p[0].x = 160.;
    p[0].y = 120.;
    c[0].r = 0.; c[0].g = 0.; c[0].b = 0.; c[0].a = 1.0;
    c[1].r = 1.; c[1].g = 1.; c[1].b = 1.; c[1].a = 0.0;
    for (i=0; i<40; i++) {
      p[1].x = p[0].x + 100.*cosf(i*pi/20);
      p[1].y = p[0].y + 100.*sinf(i*pi/20);
      DBUG2 (("line (%g,%g), (%g,%g)\n", p[0].x, p[0].y, p[1].x, p[1].y));
      F2_ShadedLines (gs, 2, p, c);
    }
    gs->gs_SendList(gs);
    GS_WaitIO(gs);
  }

  /* wait for an event */
  while( !GetControlPad (1, FALSE, &cped) ) ;
  while( !GetControlPad (1, FALSE, &cped) ) ;

  {
    Point2 p1;
    Color4 c1;
    p1.x = 100.;
    p1.y = 100.;
    c1.r = 1.; c1.g = 1.; c1.b = 1.; c1.a = 1.0;
    CLT_ClearFrameBuffer (gs, .2, .8, .2, 0., TRUE, TRUE);	/* clear fb to green */
    F2_Point (gs, p1.x, p1.y, &c1);
    gs->gs_SendList(gs);
    GS_WaitIO(gs);
  }

  /* wait for an event */
  while( !GetControlPad (1, FALSE, &cped) ) ;
  while( !GetControlPad (1, FALSE, &cped) ) ;

  while( !GetControlPad (1, FALSE, &cped) ) {
    for (i=0; i<10; i++) {
      Point2 p1;
      Color4 c0;
      p1.x = (gfloat)((rand()%(W2))-WD2);
      p1.y = (gfloat)((rand()%H2)-HD2);
      c0.r = (gfloat)(rand()&255)/256.;
      c0.g = (gfloat)(rand()&255)/256.;
      c0.b = (gfloat)(rand()&255)/256.;
      c0.a = 1.0;
      F2_Point (gs, p1.x, p1.y, &c0);
    }
  }


  /* wait for an event */
  while( !GetControlPad (1, FALSE, &cped) ) ;
  DBUG2 (("2.."));
  while( !GetControlPad (1, FALSE, &cped) ) {
    DBUG2 (("+"));
    for (i=0; i<10; i++) {
      Point2 p1, p2;
      Color4 c0, c1;
      p1.x = (gfloat)((int32)(rand()%W2)-WD2);
      p1.y = (gfloat)((int32)(rand()%H2)-HD2);
      p2.x = (gfloat)((int32)(rand()%W2)-WD2);
      p2.y = (gfloat)((int32)(rand()%H2)-HD2);
      c0.r = 0.; c0.g = 0.; c0.b = 0.; c0.a = 1.0;
      c1.r = 1.; c1.g = 1.; c1.b = 1.; c1.a = 1.0;
      DBUG2 (("line (%g,%g), (%g,%g)\n", p1.x, p1.y, p2.x, p2.y));
      F2_DrawLine (gs, &p1, &p2, &c0, &c1);
    }
    gs->gs_SendList(gs);
    GS_WaitIO(gs);
  }
  while( !GetControlPad (1, FALSE, &cped) ) ;

  while( !GetControlPad (1, FALSE, &cped) ) {
    gfloat x1, y1, x2, y2;
    Color4 c;
    x1 = (gfloat)((int32)(rand()%W2)-WD2);
    y1 = (gfloat)((int32)(rand()%H2)-HD2);
    x2 = (gfloat)((int32)(rand()%W2)-WD2);
    y2 = (gfloat)((int32)(rand()%H2)-HD2);
    c.r = (gfloat)((uint32)rand()*2+(rand()&1))/(65536.*65536.);
    c.g = (gfloat)((uint32)rand()*2+(rand()&1))/(65536.*65536.);
    c.b = (gfloat)((uint32)rand()*2+(rand()&1))/(65536.*65536.);
    c.a = 1.;
    F2_FillRect (gs, x1, y1, x2, y2, &c);
    gs->gs_SendList(gs);
    GS_WaitIO(gs);
#if 0
    while( !GetControlPad (1, FALSE, &cped) ) ;
    while( !GetControlPad (1, FALSE, &cped) ) ;
#endif
  }
  while( !GetControlPad (1, FALSE, &cped) ) ;


  {
    Point2 p[4]; Color4 c[4];
    p[0].x = 100; p[0].y = 100;
    p[1].x = 200; p[1].y = 100;
    p[2].x = 200; p[2].y = 200;
    p[3].x = 100; p[3].y = 200;
    c[0].r = 1., c[0].g = 0., c[0].b = 0., c[0].a = 1.;
    c[1].r = 0., c[1].g = 1., c[1].b = 0., c[1].a = 1.;
    c[2].r = 0., c[2].g = 0., c[2].b = 1., c[2].a = 1.;
    c[3].r = 1., c[3].g = 0., c[3].b = 0., c[3].a = 1.;

    F2_TriFan (gs, GEOF2_COLORS, 4, p, c, 0);
    gs->gs_SendList(gs);
    GS_WaitIO(gs);
  }
  while( !GetControlPad (1, FALSE, &cped) ) ;
  while( !GetControlPad (1, FALSE, &cped) ) ;

  while( !GetControlPad (1, FALSE, &cped) ) {
    Point2 p[3]; Color4 c[3];
    p[0].x = (gfloat)((int32)(rand()%W2)-WD2);
    p[0].y = (gfloat)((int32)(rand()%H2)-HD2);
    p[1].x = (gfloat)((int32)(rand()%W2)-WD2);
    p[1].y = (gfloat)((int32)(rand()%H2)-HD2);
    p[2].x = (gfloat)((int32)(rand()%W2)-WD2);
    p[2].y = (gfloat)((int32)(rand()%H2)-HD2);
    c[0].r = (gfloat)((uint32)rand()*2+(rand()&1))/(65536.*65536.);
    c[0].g = (gfloat)((uint32)rand()*2+(rand()&1))/(65536.*65536.);
    c[0].b = (gfloat)((uint32)rand()*2+(rand()&1))/(65536.*65536.);
    c[0].a = 1.;
    c[1].r = (gfloat)((uint32)rand()*2+(rand()&1))/(65536.*65536.);
    c[1].g = (gfloat)((uint32)rand()*2+(rand()&1))/(65536.*65536.);
    c[1].b = (gfloat)((uint32)rand()*2+(rand()&1))/(65536.*65536.);
    c[1].a = 1.;
    c[2].r = (gfloat)((uint32)rand()*2+(rand()&1))/(65536.*65536.);
    c[2].g = (gfloat)((uint32)rand()*2+(rand()&1))/(65536.*65536.);
    c[2].b = (gfloat)((uint32)rand()*2+(rand()&1))/(65536.*65536.);
    c[2].a = 1.;
    F2_TriStrip (gs, GEOF2_COLORS, 3, p, c, 0);
    gs->gs_SendList(gs);
    GS_WaitIO(gs);
  }
  while( !GetControlPad (1, FALSE, &cped) ) ;


  KillEventUtility();

  printf ("All done\n");
  exit (0);
}

