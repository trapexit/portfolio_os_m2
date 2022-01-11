/******************************************************************************
**
**  @(#) sprite3.c 96/07/09 1.7
**
******************************************************************************/

/**
|||	AUTODOC -internal -class Examples -name SpriteExample_1
|||	Illustrates how to display a single sprite.
|||
|||	  Synopsis
|||
|||	    SpriteExample_1 <filename.utf>
|||
|||	  Description
|||
|||	    This program demonstrates creation of a GState, allocation of
|||	    bitmaps, sprite creation, loading a .utf file into a sprite,
|||	    setting simple texture attributes for a sprite, positioning a
|||	    sprite, translating a sprite, and drawing a sprite.
|||
|||	    Waits for a change in the ControlPad, and then cleans up and
|||	    exits when a change is detected.
|||
|||	  Associated Files
|||
|||	    SpriteExample_1.c
|||
|||	  Location
|||
|||	    TBD
|||
**/




#include <stdio.h>
#include <stdlib.h>

#include <graphics/graphics.h>
#include <graphics/view.h>

#include <graphics/frame2d/frame2d.h>
#include <graphics/frame2d/spriteobj.h>
#include <graphics/frame2d/gridobj.h>

#include <misc/event.h>
#include <kernel/debug.h>


#define FBWIDTH 	( 320 )
#define FBHEIGHT 	( 240 )
#define DEPTH 		( 16 )

#define CKERR(x,y) {int32 _err=(x);if(_err<0){printf("Error - %s\n",y);PrintfSysErr(_err); exit(1);}}

/*************************  main  *************************/
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
	Item				bitmaps[3];

	/* Check for the UTF filename */
	if (argc<2)
	{
		printf("\nSpriteExample_1 - UTF filename required.\n");
		printf("usage:  SpriteExample_1 <filename.utf>\n");
		exit (1);
	}
	else
	{
		fname = argv[1];
	}



	/* Set up some basic graphics stuff like gaining access to the
	 * GraphicsFolio, allocating a signal to use as a display signal,
	 * creating a GState object, and allocating lists and bitmaps for
	 * that GState object.
	 */
	OpenGraphicsFolio ();
	buffSignal = AllocSignal (0);
	gs = GS_Create ();
	if (!gs)
	{
    	printf ("Error creating GState\n");
    	exit (1);
	}

	/* Initialize the EventBroker for handling the ControlPad input. */
	if( InitEventUtility(1, 0, LC_ISFOCUSED) < 0 ) {
		printf("Error in InitEventUtility\n");
		exit(1);
	}


	GS_AllocLists(gs, 2, 2048);
	err = GS_AllocBitmaps(bitmaps, FBWIDTH, FBHEIGHT, bmType, 2, 1);
	CKERR(err,"can't alloc bitmaps");
	GS_SetDestBuffer(gs, bitmaps[0]);
	GS_SetZBuffer(gs, bitmaps[2]);

	/* Create a viewItem and add it to the ViewList.  */
	viewItem = CreateItemVA(MKNODEID(NST_GRAPHICS, GFX_VIEW_NODE),
				VIEWTAG_VIEWTYPE, viewType,
				VIEWTAG_DISPLAYSIGNAL, buffSignal,
				VIEWTAG_BITMAP, bitmaps[0],
				TAG_END );
	CKERR(viewItem,"can't create view item");
	AddViewToViewList( viewItem, 0 );

	/* Initially clear the frame buffer to black and make it visible.
	 * This ensures that no garbage will be made visible.
	 */
	CLT_ClearFrameBuffer (gs, 0., 0., 0., 0., TRUE, TRUE);
	GS_SendList(gs);
	GS_WaitIO(gs);
	ModifyGraphicsItemVA(viewItem,
			     VIEWTAG_BITMAP, bitmaps[0],
			     TAG_END );



	CLT_ClearFrameBuffer (gs, .2, .2, .8, 0., TRUE, TRUE);	/* clear the frame buffer and set it to blue */

	/* Gimme a sprite!  Create a sprite, and then load the utf file into it. */
	sp = Spr_Create(0);
	Spr_LoadUTF (sp, fname);

	/* These attributes are the minimum requirements for displaying a
	 * sprite.  They will not alter how a texture appears from its
	 * original form.
	 */
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

	/* Set the sprite's corners. */
	Spr_ResetCorners (sp, SPR_TOPLEFT);

	/* Move the sprite so it's not stuck in a corner. */
	Spr_Translate (sp, 50, 50);

	/* Draw the sprite into the GState. */
	F2_Draw (gs, sp);

	/* Send the command list to be drawn */
	GS_SendList(gs);

	/* Wait for the list to finish drawing. */
	GS_WaitIO(gs);


	printf ("Waiting to exit.\n");
	while( !GetControlPad (1, FALSE, &cped) ) ;


	/* Clean up */
	KillEventUtility ();
	Spr_Delete (sp);
	GS_FreeBitmaps(bitmaps, 3);
	GS_FreeLists(gs);
	CloseGraphicsFolio ();


	printf( "Exiting.\n");
	exit (0);


}

