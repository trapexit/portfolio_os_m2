
/******************************************************************************
**
**  @(#) SimpleSprite.c 96/07/09 1.4
**
******************************************************************************/

/**
|||	AUTODOC -class Examples -group Frame2D -name SimpleSprite
|||	Illustrates how to display a single sprite.
|||
|||	  Synopsis
|||
|||	    SimpleSprite <filename.utf>
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
|||	    SimpleSprite.c
|||
|||	  Location
|||
|||	    Examples/Graphics/Frame2D
|||
**/


/*************************  includes  *************************/

#include <kernel/task.h>
#include <graphics/frame2d/f2d.h>
#include <graphics/view.h>
#include <misc/event.h>
#include <stdio.h>
#include <stdlib.h>

/*************************  defines  *************************/

#define FBWIDTH 	( 320 )
#define FBHEIGHT 	( 240 )

#define CKERR(x,y) {Err ick=(x);if(ick<0){printf("Error - %s\n",y);PrintfSysErr(x); exit(1);}}


/*************************  main  *************************/
void main(int argc, char **argv)
{
	GState				*gs;
	SpriteObj			*sp;
	ControlPadEventData cped;
	Item      			bitmaps[3];
	int32				displaySignal;
	Item				viewItem;
	uint32				viewType = VIEWTYPE_16;
	uint32				bmType = BMTYPE_16;
	char 				*fname;
	Err					err;

	char				instructions[] = "\SimpleSprite:\n"
										 "\tThis program simply creates a sprite and loads the\n"
										 "\tgiven texture into it.\n"
										 "\n\tPress any button to quit.\n";


	/* Check for filename */
	if (argc<2)
	{
		printf("SimpleSprite - Filename required.\n");
		printf("\tusage:  SimpleSprite <filename.utf>\n");
		exit (1);
	}
	else
	{
		fname = argv[1];
	}

 	/* Initialize the EventBroker for handling the ControlPad input. */
	err = InitEventUtility(1, 0, TRUE);
	CKERR( err, "InitEventUtility");

  	/* Set up some basic graphics stuff like gaining access to the
	 * GraphicsFolio, allocating a signal to use as a display signal,
	 * creating a GState object, and allocating lists and bitmaps for
	 * that GState object.
	 */
	err = OpenGraphicsFolio ();
	CKERR( err, "OpenGraphicsFolio");

	displaySignal = AllocSignal (0);
	if(!displaySignal)
	{
		printf("Couldn't allocate signal.");
		PrintfSysErr(displaySignal);
		exit(1);
	}

	gs = GS_Create ();
  	if(!gs)
	{
		printf("Couldn't create GState.");
		exit(1);
	}

	err = GS_AllocLists(gs, 2, 2048);
	CKERR(err,"GS_AllocLists");

	err = GS_AllocBitmaps(bitmaps, FBWIDTH, FBHEIGHT, bmType, 2, 1);
	CKERR(err,"GS_AllocBitmaps");

	GS_SetDestBuffer(gs, bitmaps[0]);
	GS_SetZBuffer(gs, bitmaps[2]);


	/* Create a viewItem and add it to the ViewList.  */
	viewItem = CreateItemVA(MKNODEID(NST_GRAPHICS, GFX_VIEW_NODE),
							VIEWTAG_VIEWTYPE, viewType,
							VIEWTAG_DISPLAYSIGNAL, displaySignal,
							VIEWTAG_BITMAP,  bitmaps[0],
							TAG_END );
	CKERR(viewItem,"can't create view item");

	err = AddViewToViewList( viewItem, 0 );
	CKERR(err,"AddViewToViewList");

	/* Associate a signal bit with a GState object.  This signal will
     * allow a GState to stay synchronized with the video in a double-
     * buffering scheme.
	 */
	err = GS_SetVidSignal( gs, displaySignal );
	CKERR(err,"GS_SetVidSignal");

	/* Clearing the second buffer ensures that no garbage will
	 * appear on the screen.
	 */
	CLT_ClearFrameBuffer (gs, 0., 0., 0., 0., TRUE, TRUE);

	/* Send the list to the Triangle Engine. */
	err = GS_SendList(gs);
	CKERR(err,"GS_SendList");

	/* Wait for the list to be processed. */
	err = GS_WaitIO(gs);
	CKERR(err,"GS_WaitIO");

	/* Swap buffers. */
	ModifyGraphicsItemVA(viewItem,
						VIEWTAG_BITMAP, bitmaps[0],
						TAG_END );

	/* Notify the GState that the next command list buffer is the first
     * to be rendered to a Bitmap.
	 */
	err = GS_BeginFrame(gs);
	CKERR(err,"GS_BeginFrame");

	CLT_ClearFrameBuffer (gs, 0., 0., 0., 0., TRUE, TRUE);
	CLT_SetSrcToCurrentDest (gs);


	/* Create and load the main sprite */
	sp = Spr_Create(0);
	if (!sp)
	{
    	printf ("Error creating main sprite.\n");
    	exit (1);
	}

	/* Load the given texture into the sprite. */
	err = Spr_LoadUTF (sp, fname);
	CKERR( err,"Spr_LoadUTF");

	/* Set the color output of the texture to the texture color. */
	err = Spr_SetTextureAttr (sp, TXA_ColorOut, TX_BlendOutSelectTex);
	CKERR( err,"Spr_SetTextureAttr");

	/* Set the sprite's corners. */
	Spr_ResetCorners (sp, SPR_TOPLEFT);

	/* Move the sprite so it's not stuck in a corner. */
	Spr_Translate (sp, 120, 80);

	/* Draw the sprite into the GState. */
	F2_Draw (gs, sp);

	/* Send the command list to be drawn */
	GS_SendList(gs);

	/* Wait for the list to finish drawing. */
	GS_WaitIO(gs);


	printf ("Waiting to exit.\n");
	while( !GetControlPad (1, FALSE, &cped) ) ;


	/* Clean up */
	err = KillEventUtility ();
	CKERR(err,"KillEventUtility");

	err = DeleteItem(viewItem);
	CKERR(err,"DeleteItem");

	GS_FreeBitmaps(bitmaps, 3);
	GS_FreeLists(gs);

	err = FreeSignal(displaySignal);
	CKERR(err,"FreeSignal");

	err = Spr_Delete(sp);
	CKERR(err,"Spr_Delete");

	err = CloseGraphicsFolio();
	CKERR(err,"CloseGraphicsFolio");


	printf( "Exiting.\n");
	exit (0);
}

