/******************************************************************************
**
**  @(#) sprite4.c 96/07/09 1.9
**
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include <graphics/graphics.h>
#include <graphics/view.h>

#include <graphics/frame2d/frame2d.h>
#include <graphics/frame2d/spriteobj.h>
#include <graphics/frame2d/gridobj.h>

#include <misc/event.h>
#include <kernel/debug.h>
#include  <kernel/time.h>

#define DBUG(x) {printf x;}
#define DBUG2(x) /* {printf x;} */
#define CKERR(x,y) {int32 _err=(x);if(_err<0){printf("Error - %s\n",y);PrintfSysErr(_err); exit(1);}}

#define VTX_FORMAT_SPR (GEO_TexCoords|GEO_Locations)

#define FBWIDTH 			320
#define FBHEIGHT 			240
#define DEPTH 				16
#define POSITION_INCREMENT 	( 1 )
#define SCALE_INCREMENT 	( 0.001 )




/************************  DefaultTabInit *************************/
/*
 * Sets some default texture attributes.
 */

void DefaultTabInit (SpriteObj *sp)
{
  Spr_SetTextureAttr (sp, TXA_ColorOut, TX_BlendOutSelectTex);
  Spr_SetTextureAttr (sp, TXA_AlphaOut, TX_AlphaSelectTexAlpha);
  Spr_SetTextureAttr (sp, TXA_TextureEnable, 1);

  if (Spr_GetPIP(sp))
    {
      Spr_SetTextureAttr (sp, TXA_PipColorSelect, RC_TXTPIPCNTL_PIPCOLORSELECT_PIP);	/* If we have a PIP, use the PIP color */
    }
  else
    {
      Spr_SetTextureAttr (sp, TXA_PipColorSelect, RC_TXTPIPCNTL_PIPCOLORSELECT_TEXTURE);	/* If no PIP, use texture color */
    }

  Spr_SetTextureAttr (sp, TXA_PipAlphaSelect, 0);	/* Default to using constant alpha */
  Spr_SetTextureAttr (sp, TXA_PipSSBSelect, 0);	/* Default to using constant SSB */
}


int32 cornertable[] =
{SPR_CENTER, SPR_TOPLEFT, SPR_TOPRIGHT, SPR_BOTTOMLEFT, SPR_BOTTOMRIGHT};

#define maxcorners (sizeof(cornertable)/sizeof(*cornertable))





/********************************  main *********************************/

void main(int argc, char **argv)
{
	GState				*gs;
	SpriteObj			*sp;
	ControlPadEventData cped;
	int32				buffSignal;
	Item				viewItem;
	uint32				viewType = VIEWTYPE_16;
	uint32				bmType = BMTYPE_16;
	char 				*fname;
	Err					err;
	uint32				joybits=0, oldjoybits;
	Point2				points = { 160,120 };
	float32				angle = 0.0;
	float32				Xscale = 1;
	float32				Yscale = 1;
	int32				curFB = 0;
	int32				cornerpos = 0;
	Item      bitmaps[3];

	/* Check for filename */
	if (argc<2)
	{
		printf("SpriteEffects - Filename required.\n");
		printf("     usage:  SpriteEffects <filename.utf>\n");
		exit (1);
	}
	else

	{
		fname = argv[1];
	}

 	OpenGraphicsFolio ();
	buffSignal = AllocSignal (0);
	gs = GS_Create ();
	if (!gs)
	{
    	printf ("Error creating GState\n");
    	exit (1);
	}

	/* Initialize the EventBroker. */
	if( InitEventUtility(1, 0, LC_ISFOCUSED) < 0 )
	{
		printf("Error in InitEventUtility\n");
		exit(1);
	}

	/* Request memory */
	GS_AllocLists(gs, 2, 2048);
	err = GS_AllocBitmaps(bitmaps, FBWIDTH, FBHEIGHT, bmType, 2, 1);
	CKERR(err,"can't alloc bitmaps");
	GS_SetDestBuffer(gs, bitmaps[0]);
	GS_SetZBuffer(gs, bitmaps[2]);






	/* Create the view */
	viewItem = CreateItemVA(MKNODEID(NST_GRAPHICS, GFX_VIEW_NODE),
				VIEWTAG_VIEWTYPE, viewType,
				VIEWTAG_DISPLAYSIGNAL, buffSignal,
				VIEWTAG_BITMAP, bitmaps[0],
				TAG_END );
	CKERR(viewItem,"can't create view item");
	AddViewToViewList( viewItem, 0 );

	/* Initially clear the frame buffer to black and make it visible */
	CLT_ClearFrameBuffer (gs, 0., 0., 0., 0., TRUE, TRUE);
	GS_SendList(gs);
	GS_WaitIO(gs);
	ModifyGraphicsItemVA(viewItem,
			     VIEWTAG_BITMAP, bitmaps[0],
			     TAG_END );

	CLT_ClearFrameBuffer (gs, 0.3, 0.2, 0.5, 0.0, TRUE, TRUE);	/* clear the frame buffer */
	CLT_SetSrcToCurrentDest (gs);

	/* Create and load the main sprite */
	sp = Spr_Create(0);
	Spr_LoadUTF (sp, fname);
	DefaultTabInit (sp);


	Spr_SetPosition (sp, &points);
	Spr_ResetCorners( sp, SPR_TOPLEFT );

	Spr_SetHSlice (sp, 8);
	Spr_SetVSlice (sp, 8);

	while( 1 )
	{
		if ((GetControlPad (1, FALSE, &cped)) < 0)
		{
			printf("GetControlPad failed\n");
			break;
		}

		oldjoybits = joybits;
		joybits = cped.cped_ButtonBits;

		if( joybits & ControlX )
		{
			break;
		}
		else if( joybits & ControlRightShift )
		{
			angle += .1;
		}
		else if( joybits & ControlLeftShift )
		{
			angle -= .1;
		}
		else if( joybits & ControlUp )
		{
			points.y -= POSITION_INCREMENT;
		}
		else if( joybits & ControlDown )
		{
			points.y += POSITION_INCREMENT;
		}
		else if( joybits & ControlLeft )
		{
			points.x -= POSITION_INCREMENT;
		}
		else if( joybits & ControlRight )
		{
			points.x += POSITION_INCREMENT;
		}
		else if( joybits & ControlA )
		{
			if ( Xscale < 1 )
			{
				/* Reset the scaling variables to 1 */
				Xscale = Yscale = 1;
			}
			else
			{
				Xscale += SCALE_INCREMENT;
				Yscale += SCALE_INCREMENT;
			}

			Spr_Scale (sp, Xscale, Yscale);
		}
		else if( joybits & ControlB )
		{
			if ( Xscale > 1 )
			{
				/* Reset the scaling variables to 1 */
				Xscale = Yscale = 1;
			}
			else
			{
				Xscale -= SCALE_INCREMENT;
				Yscale -= SCALE_INCREMENT;
			}

			Spr_Scale (sp, Xscale, Yscale);
		}
		else if( joybits & ControlC )
		{
			points.x = 160;
			points.y = 120;

			if (!(oldjoybits&ControlC)) {
			  if (++cornerpos >= maxcorners) cornerpos=0;
			}
			Spr_ResetCorners( sp, cornertable[cornerpos] );

			Xscale = 1;
			Yscale = 1;
			Spr_Scale (sp, Xscale, Yscale);

			angle = 0;
		}



		Spr_SetPosition (sp, &points);
		Spr_Rotate (sp, angle);
		F2_Draw (gs, sp);


		/* Wait until the currently displayed buffer has finished displaying. */
		WaitSignal(buffSignal);

		/* Send the command list to be drawn */
		GS_SendList(gs);

		/* Make sure it's finished. */
		GS_WaitIO(gs);

		/* Swap the buffers */
		ModifyGraphicsItemVA(viewItem,
				     VIEWTAG_BITMAP, bitmaps[curFB],
				     TAG_END );
		curFB = 1-curFB;
		GS_SetDestBuffer(gs, bitmaps[curFB]);
		CLT_ClearFrameBuffer (gs, 0.3, 0.2, 0.5, 0., TRUE, TRUE);
	}

	/* Clean up */
	KillEventUtility ();
	Spr_Delete (sp);
	GS_FreeBitmaps(bitmaps, 3);
	GS_FreeLists(gs);
	CloseGraphicsFolio ();



	printf( "Exiting.\n");
	exit (0);


}




