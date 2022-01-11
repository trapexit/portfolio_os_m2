/******************************************************************************
**
**  @(#) xfade.c 96/07/09 1.4
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
#include <ctype.h>
#include <stdlib.h>
#include <misc/event.h>
#include <kernel/debug.h>
#include <kernel/mem.h>

#define DBUG(x) /* printf x */
#define DBUG2(x) /* printf x */
#define CKERR(x,y) {int32 err=(x);if(err<0){printf("Error - %s\n",y);PrintfSysErr(x); exit(1);}}

#define FBWIDTH 320
#define FBHEIGHT 240
#define DEPTH 16



GState*		gs;		/* -> current graphics pipeline */
Item		bitmaps[3];
SpriteObj	*sp, *sp2;
int		i, j, step=1;
Point2		center;
ControlPadEventData cped;
uint32		memdebugflag=0;

int32		buffSignal;
Item		viewItem;
uint32		viewType;
uint32		bmType;
uint32		depth=DEPTH, fbwidth=FBWIDTH, fbheight=FBHEIGHT;

int32		curFB = 0;

char		*fname[2]={"leaf.utf", "test2.utf"};

gfloat		r=0., g=0., b=0.;




void
parse (int argc, char *argv[])
{
  j = 0;

  for (i=1; i<argc; i++) {
    if (*argv[i]=='-') {
      switch (tolower(argv[i][1])) {
      case 'm':
	memdebugflag = true;
	break;
      case 'd':
	depth = strtol(argv[i]+2, 0, 0);
	switch (depth) {
	case 16:
	case 32:
	  break;
	case 1:
	  depth=16;
	  break;
	case 2:
	case 3:
	case 24:
	  depth=32;
	  break;
	default:
	  printf ("Unrecognized depth option '%s' (use 16 or 32)\n", argv[i]);
	  depth=DEPTH;
	}
	printf ("Depth set to %d\n", depth);
	break;
      case 'w':
	fbwidth = strtol(argv[i]+2, 0, 0);
	switch (fbwidth) {
	case 320:
	case 640:
	  break;
	case 3:
	case 32:
	  fbwidth=320;
	  break;
	case 6:
	case 64:
	  fbwidth=640;
	  break;
	default:
	  printf ("Unrecognized width option '%s' (use 320 or 640)\n",argv[i]);
	  fbwidth=FBWIDTH;
	}
	printf ("Width set to %d\n", fbwidth);
	break;
      case 'h':
	fbheight = strtol(argv[i]+2, 0, 0);
	switch (fbheight) {
	case 240:
	case 480:
	  break;
	case 2:
	case 24:
	  fbheight=240;
	  break;
	case 4:
	case 48:
	  fbheight=480;
	  break;
	default:
	  printf ("Unrecognized height option '%s' (use 320 or 640)\n",argv[i]);
	  fbheight=FBHEIGHT;
	}
	printf ("Height set to %d\n", fbheight);
	break;
      case 's':
	step = strtol(argv[i]+2, 0, 0);
	if (step<1) {
	  printf ("Unrecognized step option '%s' (use values > 0)\n", argv[i]);
	  step = 1;
	}
	printf ("Step size set to %d\n", step);
	break;
      case 'b':
	{
	  char *ptr;
	  r = strtof (argv[i]+2, &ptr);
	  if (ptr) g = strtof (ptr+1, &ptr);
	  if (ptr) b = strtof (ptr+1, &ptr);
	  if (!ptr) {
	    printf("Unrecognized background option '%s' (use -b<r>,<g>,<b>)\n",
		    argv[i]);
	    r = g = b = 0.;
	  }
	  if (r>1. || g>1. || b>1.) {
	    r /= 255.;
	    g /= 255.;
	    b /= 255.;
	  }
	}
	printf ("Background color set to %f,%f,%f\n", r, g, b);
	break;
      default:
	printf ("Unrecognized option '%s'\n", argv[i]);
      }
    } else {
      if (j>=2) {
	printf ("Extra parameter '%s' ignored\n");
      }
      fname[j] = argv[i];
      j++;
    }
  }
}


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


void AlphaLevel (SpriteObj *sp, uint32 alpha)
{
  uint32 alphacomplement;

  alpha = (alpha&0xff)*0x00010101;
  alphacomplement = alpha^0x00ffffff;
  Spr_SetDBlendAttr (sp, DBLA_AInputSelect, DBL_ASelectTexColor);
  Spr_SetDBlendAttr (sp, DBLA_AMultCoefSelect, DBL_MASelectConst);
  Spr_SetDBlendAttr (sp, DBLA_AMultConstSSB0, alpha);
  Spr_SetDBlendAttr (sp, DBLA_AMultConstSSB1, alpha);
  Spr_SetDBlendAttr (sp, DBLA_BInputSelect, DBL_BSelectSrcColor);
  Spr_SetDBlendAttr (sp, DBLA_BMultCoefSelect, DBL_MBSelectConst);
  Spr_SetDBlendAttr (sp, DBLA_BMultConstSSB0, alphacomplement);
  Spr_SetDBlendAttr (sp, DBLA_BMultConstSSB1, alphacomplement);
  Spr_SetDBlendAttr (sp, DBLA_ALUOperation, DBL_AddClamp);
}


void NoBlend (SpriteObj *sp)
{
  Spr_SetDBlendAttr (sp, DBLA_EnableAttrs, DBL_AlphaDestOut|DBL_RGBDestOut);
}



main(int argc, char *argv[])
{
  parse (argc, argv);

  bmType = (depth==16)?BMTYPE_16:BMTYPE_32;
  center.x = fbwidth/2;
  center.y = fbheight/2;

  if (memdebugflag) {
    CreateMemDebug(NULL);
    ControlMemDebug(MEMDEBUGF_ALLOC_PATTERNS |
		    MEMDEBUGF_FREE_PATTERNS |
		    MEMDEBUGF_PAD_COOKIES);
    sp2 = Spr_Create(NULL);
    Spr_LoadUTF (sp2, fname[0]);
    Spr_Delete (sp2);
    DumpMemDebug (NULL);
    DeleteMemDebug ();
  }

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

  viewType = BestViewType(fbwidth, fbheight, depth);

  printf ("Alloc bitmaps\n");
  GS_AllocLists(gs, 2, 2048);
  GS_AllocBitmaps(bitmaps, fbwidth, fbheight, bmType, 2, 1);
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
  /* Initially clear the frame buffer and make it visible */
  CLT_ClearFrameBuffer (gs, r, g, b, 0., TRUE, TRUE);
  gs->gs_SendList(gs);
  GS_WaitIO(gs);
  printf ("Bring view to front\n");
  ModifyGraphicsItemVA(viewItem,
		       VIEWTAG_BITMAP, bitmaps[0],
		       TAG_END );



  printf ("Load sprite file %s\n", fname[0]);
  sp = Spr_Create(0);
  Spr_LoadUTF (sp, fname[0]);

  printf ("Load sprite file %s\n", fname[1]);
  sp2 = Spr_Create(0);
  Spr_LoadUTF (sp2, fname[1]);

  printf ("Set default attributes\n");
  DefaultTabInit (sp);
  DefaultTabInit (sp2);
  NoBlend (sp);
  AlphaInit (sp2);
  Spr_ResetCorners (sp, SPR_CENTER);
  Spr_ResetCorners (sp2, SPR_CENTER);
  Spr_SetPosition (sp, &center);
  Spr_SetPosition (sp2, &center);

  while (1) {
    /* Fade in */
    for (i=0; i<=255; i+=step) {
      if ((GetControlPad (1, FALSE, &cped)) < 0) {
	printf("GetControlPad failed\n");
	break;
      }
      if( cped.cped_ButtonBits & ControlX ) goto out;

      /* Swap the buffers */
      ModifyGraphicsItemVA(viewItem,
			   VIEWTAG_BITMAP, bitmaps[curFB],
			   TAG_END );
      curFB = 1-curFB;
      GS_SetDestBuffer(gs, bitmaps[curFB]);

      AlphaLevel (sp2, i);
      CLT_ClearFrameBuffer (gs, r, g, b, 0., TRUE, TRUE);
      CLT_SetSrcToCurrentDest (gs);
      F2_Draw (gs, sp);
      F2_Draw (gs, sp2);
      WaitSignal(buffSignal);
      gs->gs_SendList (gs);
      GS_WaitIO(gs);
    }

    AlphaLevel (sp2, 255);
    /* Pause */
    for (i=0; i<30; i++) {
      if ((GetControlPad (1, FALSE, &cped)) < 0) {
	printf("GetControlPad failed\n");
	break;
      }
      if( cped.cped_ButtonBits & ControlX ) goto out;

      /* Swap the buffers */
      ModifyGraphicsItemVA(viewItem,
			   VIEWTAG_BITMAP, bitmaps[curFB],
			   TAG_END );
      curFB = 1-curFB;
      GS_SetDestBuffer(gs, bitmaps[curFB]);

      CLT_ClearFrameBuffer (gs, r, g, b, 0., TRUE, TRUE);
      CLT_SetSrcToCurrentDest (gs);
      F2_Draw (gs, sp);
      F2_Draw (gs, sp2);
      WaitSignal(buffSignal);
      gs->gs_SendList (gs);
      GS_WaitIO(gs);
    }

    /* Fade out */
    for (i=255; i>=0; i-=step) {
      if ((GetControlPad (1, FALSE, &cped)) < 0) {
	printf("GetControlPad failed\n");
	break;
      }
      if( cped.cped_ButtonBits & ControlX ) goto out;

      /* Swap the buffers */
      ModifyGraphicsItemVA(viewItem,
			   VIEWTAG_BITMAP, bitmaps[curFB],
			   TAG_END );
      curFB = 1-curFB;
      GS_SetDestBuffer(gs, bitmaps[curFB]);

      AlphaLevel (sp2, i);
      CLT_ClearFrameBuffer (gs, r, g, b, 0., TRUE, TRUE);
      CLT_SetSrcToCurrentDest (gs);
      F2_Draw (gs, sp);
      F2_Draw (gs, sp2);
      WaitSignal(buffSignal);
      gs->gs_SendList (gs);
      GS_WaitIO(gs);
    }

    AlphaLevel (sp2, 0);
    /* Pause */
    for (i=0; i<30; i++) {
      if ((GetControlPad (1, FALSE, &cped)) < 0) {
	printf("GetControlPad failed\n");
	break;
      }
      if( cped.cped_ButtonBits & ControlX ) goto out;

      /* Swap the buffers */
      ModifyGraphicsItemVA(viewItem,
			   VIEWTAG_BITMAP, bitmaps[curFB],
			   TAG_END );
      curFB = 1-curFB;
      GS_SetDestBuffer(gs, bitmaps[curFB]);

      CLT_ClearFrameBuffer (gs, r, g, b, 0., TRUE, TRUE);
      CLT_SetSrcToCurrentDest (gs);
      F2_Draw (gs, sp);
      F2_Draw (gs, sp2);
      WaitSignal(buffSignal);
      gs->gs_SendList (gs);
      GS_WaitIO(gs);
    }
  }

out:
  KillEventUtility();

  printf ("All done\n");
  exit (0);
}



