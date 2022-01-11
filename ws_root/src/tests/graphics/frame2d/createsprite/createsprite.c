/******************************************************************************
**
**  @(#) createsprite.c 96/07/09 1.6
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


#define FBWIDTH 	( 320 )
#define FBHEIGHT 	( 240 )
#define DEPTH 		( 16 )

#define CKERR(x,y) {int32 _err=(x);if(_err<0){printf("Error - %s\n",y);PrintfSysErr(_err); exit(1);}}


/*
 * Simple 2 bits/pixel texture, checkerboard pattern using PIP entries 0 and 3
 * for most of the checkerboard, use colors 1 and 2 for the center squares.
 */
uint32 txtr[] = {
  0xf0f0f0f0, 0xf0f0f0f0,
  0x0f0f0f0f, 0x0f0f0f0f,
  0xf0f0f0f0, 0xf0f0f0f0,
  0x0f05af0f, 0x0f05af0f,
  0xf0fa50f0, 0xf0fa50f0,
  0x0f0f0f0f, 0x0f0f0f0f,
  0xf0f0f0f0, 0xf0f0f0f0,
  0x0f0f0f0f, 0x0f0f0f0f,
};


/* Square with diamond center texture */
uint32 txtr2[] = {
  0x0003c000, 0x000ff000,
  0x003ffc00, 0x00ffff00,
  0x03ffffc0, 0x0ffffff0,
  0x3ffffffc, 0xffffffff,
  0xffffffff, 0x3ffffffc,
  0x0ffffff0, 0x03ffffc0,
  0x00ffff00, 0x003ffc00,
  0x000ff000, 0x0003c000,
};


/* PIP table with 4 entries - white, black, grey, red */
uint32 pipdata[] = {
  0xffffffff, 0xff000000, 0xff808080, 0xffff0000,
};

/* 4 level grey scale PIP table */
uint32 pipdata2[] = {
  0xff000000, 0xff555555, 0xffaaaaaa, 0xffffffff,
};

/*************************  main  *************************/
void main(int argc, char **argv)
{
	GState				*gs;
	SpriteObj			*sp, *sp2, *sparray[9];
	GridObj				*gr;
	ControlPadEventData cped;
	int32				buffSignal;
	Item				viewItem;
	uint32				viewType = VIEWTYPE_16;
	uint32				bmType = BMTYPE_16;
	Err					err;
	Item				bitmaps[3];
	CltPipControlBlock		*pcb;
	CltTxData			*td;
	Point2				p2, p3;

	TOUCH(argc);
	TOUCH(argv);
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

	/* Gimme a sprite!  Create a sprite, attach predefined texture and PIP data */
	sp = Spr_Create(0);
	Spr_CreateTxData (sp, txtr, 16, 16, 2,
			  MAKEEXPFORMAT (0, 0, 1, 0, 0, 0, 2));
	Spr_CreatePipControlBlock (sp, pipdata, 0,
				   sizeof(pipdata)/sizeof(pipdata[0]));

	/* These attributes are the minimum requirements for displaying a
	 * sprite.  They will not alter how a texture appears from its
	 * original form.
	 */
	Spr_SetTextureAttr (sp, TXA_ColorOut, TX_BlendOutSelectTex);
	Spr_SetTextureAttr (sp, TXA_AlphaOut, TX_AlphaSelectTexAlpha);
	Spr_SetTextureAttr (sp, TXA_TextureEnable, 1);
	/* Use point sampling */
	Spr_SetTextureAttr (sp, TXA_MinFilter, TX_Nearest);
	Spr_SetTextureAttr (sp, TXA_MagFilter, TX_Nearest);
	Spr_SetTextureAttr (sp, TXA_InterFilter, TX_Nearest);

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

	/* Make the sprite big so we can see it */
	Spr_Scale (sp, 5., 5.);

	/* Draw the sprite into the GState. */
	F2_Draw (gs, sp);

	/* Send the command list to be drawn */
	GS_SendList(gs);

	/* Wait for the list to finish drawing. */
	GS_WaitIO(gs);


	printf ("Wait for a button to be pressed and released\n");
	while( !GetControlPad (1, FALSE, &cped) ) ;
	while( !GetControlPad (1, FALSE, &cped) ) ;




	CLT_ClearFrameBuffer (gs, .2, .8, .2, 0., TRUE, TRUE);	/* clear the frame buffer and set it to green */

	/* Gimme a sprite!  Create a sprite, attach predefined texture and PIP data */
	sp2 = Spr_Create (0);
	td = CltTxData_Create (txtr2, 16, 16, 2,
			    MAKEEXPFORMAT (0, 0, 1, 0, 0, 0, 2));
	Spr_SetTextureData (sp2, td);
	pcb = CltPipControlBlock_Create (pipdata2, 0,
					 sizeof(pipdata)/sizeof(pipdata[0]));
	Spr_SetPIP (sp2, pcb);

	/* These attributes are the minimum requirements for displaying a
	 * sprite.  They will not alter how a texture appears from its
	 * original form.
	 */
	Spr_SetTextureAttr (sp2, TXA_ColorOut, TX_BlendOutSelectTex);
	Spr_SetTextureAttr (sp2, TXA_AlphaOut, TX_AlphaSelectTexAlpha);
	Spr_SetTextureAttr (sp2, TXA_TextureEnable, 1);
	/* Use point sampling */
	Spr_SetTextureAttr (sp2, TXA_MinFilter, TX_Nearest);
	Spr_SetTextureAttr (sp2, TXA_MagFilter, TX_Nearest);
	Spr_SetTextureAttr (sp2, TXA_InterFilter, TX_Nearest);

	if (Spr_GetPIP(sp2))
	{
	  Spr_SetTextureAttr (sp2, TXA_PipColorSelect, RC_TXTPIPCNTL_PIPCOLORSELECT_PIP);	/* If we have a PIP, use the PIP color */
	}
	else
	{
	  Spr_SetTextureAttr (sp2, TXA_PipColorSelect, RC_TXTPIPCNTL_PIPCOLORSELECT_TEXTURE);	/* If no PIP, use texture color */
	}

	Spr_SetTextureAttr (sp2, TXA_PipAlphaSelect, 0);	/* Default to using constant alpha */
	Spr_SetTextureAttr (sp2, TXA_PipSSBSelect, 0);	/* Default to using constant SSB */

	/* Set the sprite's corners. */
	Spr_ResetCorners (sp2, SPR_TOPLEFT);

	/* Move the sprite so it's not stuck in a corner. */
	Spr_Translate (sp2, 50, 50);

	/* Make the sprite big so we can see it */
	Spr_Scale (sp2, 5., 5.);

	/* Draw the sprite into the GState. */
	F2_Draw (gs, sp2);

	/* Send the command list to be drawn */
	GS_SendList(gs);

	/* Wait for the list to finish drawing. */
	GS_WaitIO(gs);


	printf ("Wait for a button to be pressed and released\n");
	while( !GetControlPad (1, FALSE, &cped) ) ;
	while( !GetControlPad (1, FALSE, &cped) ) ;



	gr = Gro_Create (0);
	Gro_SetWidth (gr, 3);
	Gro_SetHeight (gr, 3);
	p2.x = 20., p2.y = 20., Gro_SetPosition (gr, &p2);
	p2.x = 70., p2.y = 0., Gro_SetHDelta (gr, &p2);
	p2.x = 0., p2.y = 70., Gro_SetVDelta (gr, &p2);
	Gro_GetHDelta (gr, &p3);
	printf ("HDelta out = %f,%f\n", p3.x, p3.y);
	Gro_GetVDelta (gr, &p3);
	printf ("VDelta out = %f,%f\n", p3.x, p3.y);
	sparray[0] = sparray[2] = sparray[4] = sparray[6] = sparray[8] = sp;
	sparray[1] = sparray[3] = sparray[5] = sparray[7] = sp2;
	Gro_SetSpriteArray (gr, sparray);
	printf ("sparray @ %lx, gr->gro_SpriteArray @%lx\n",
		(int32)sparray, (int32)Gro_GetSpriteArray(gr));

	CLT_ClearFrameBuffer (gs, .2, .2, .8, 0., TRUE, TRUE);	/* clear the frame buffer and set it to blue */

	/* Gimme a sprite!  Create a sprite, attach predefined texture and PIP data */

	/* Draw the sprite into the GState. */
	F2_Draw (gs, gr);

	/* Send the command list to be drawn */
	GS_SendList(gs);

	/* Wait for the list to finish drawing. */
	GS_WaitIO(gs);


	printf ("Wait for a button to be pressed and released\n");
	while( !GetControlPad (1, FALSE, &cped) ) ;
	while( !GetControlPad (1, FALSE, &cped) ) ;


	/* Clean up */
	KillEventUtility ();
	Spr_Delete (sp);
	Spr_Delete (sp2);
	CltPipControlBlock_Delete (pcb);
	CltTxData_Delete (td);
	GS_FreeBitmaps(bitmaps, 3);
	GS_FreeLists(gs);
	CloseGraphicsFolio ();


	printf( "Exiting.\n");
	exit (0);


}





