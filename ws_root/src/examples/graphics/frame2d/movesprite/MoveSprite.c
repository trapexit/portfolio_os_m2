
/******************************************************************************
**
**  @(#) MoveSprite.c 96/07/09 1.4
**
******************************************************************************/

/**
|||	AUTODOC -class Examples -group Frame2D -name MoveSprite
|||	Illustrates how to display, move, rotate and scale a single sprite.
|||
|||	  Synopsis
|||
|||	    MoveSprite <filename.utf>
|||
|||	  Description
|||
|||	    This program loads a sprite and lets you move, scale, and rotate it.
|||	    Also demonstrates how to do GState double-buffering.
|||
|||	    The joypad moves the sprite.  A enlarges the sprite, B shrinks
|||	    it.  Left and right shift rotate the sprite.  C restores the
|||	    sprite to its original size and location.  Stop quits the
|||	    program.
|||
|||	  Associated Files
|||
|||	    MoveSprite.c
|||
|||	  Location
|||
|||	    Examples/Graphics/Frame2d
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

#define CKERR(x,y) {Err ick=(x);if(ick<0){printf("Error - %s\n",y);PrintfSysErr(x); exit(1);}}

#define FBWIDTH 			( 320 )
#define FBHEIGHT 			( 240 )

#define POSITION_INCREMENT 	( 1 )
#define SCALE_INCREMENT 	( 0.001 )
#define	ANGLE_INCREMENT		( 0.1 )


/*************************  main  *************************/

void main(int32 argc, char **argv)
{
	GState				*gs;
	SpriteObj			*sp;
	ControlPadEventData cped;
	Item      			bitmaps[3];
	int32				curFB = 0;
	int32				displaySignal;
	Item				viewItem;
	uint32				viewType = VIEWTYPE_16;
	uint32				bmType = BMTYPE_16;
	char 				*fname;
	Err					err;
	uint32				joybits;
	Point2				points = { 100,100 };
	float32				angle = 0.0;
	float32				Xscale = 1;
	float32				Yscale = 1;
	char				instructions[] = "\nMoveSprite:\n"
										 "\tMove the sprite with the directional pad.\n"
										 "\tRotate the sprite with the shift keys.\n"
										 "\tScale sprite up and down with A and B buttons.\n"
										 "\tReset sprite to original size and position with C button.\n"
										 "\n\tPress Stop to quit.\n";


	/* Check for filename */
	if (argc<2)
	{
		printf("MoveSprite - Filename required.\n");
		printf("\tusage:  MoveSprite <filename.utf>\n");
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
	Spr_SetTextureAttr (sp, TXA_ColorOut, TX_BlendOutSelectTex);

	/* Set the sprite's position. */
	Spr_SetPosition (sp, &points);

	/* Specify which corner of the sprite is to be its positional anchor. */
	Spr_ResetCorners( sp, SPR_TOPLEFT );

	printf(instructions);

	/* Main loop */
	while( 1 )
	{
		err = GetControlPad (1, FALSE, &cped);
		CKERR( err, "GetControlPad" );

		joybits = cped.cped_ButtonBits;

		if( joybits & ControlX )
		{
			break;
		}
		else if( joybits & ControlC )
		{
			/* The C button will return the sprite to it's
			 * original state.
			 */
			points.x = 100;
			points.y = 100;
			Spr_SetPosition (sp, &points);

			Spr_ResetCorners( sp, SPR_TOPLEFT );

			Xscale = 1;
			Yscale = 1;
			Spr_Scale (sp, Xscale, Yscale);

			angle = 0;
		}
		else
		{
			/* If we're not quitting or resizing, move,
			 * rotate, and scale sprite however the user
			 * chooses.
			 */
			if( joybits & ControlRightShift )
			{
				angle += ANGLE_INCREMENT;
			}

			if( joybits & ControlLeftShift )
			{
				angle -= ANGLE_INCREMENT;
			}

			if( joybits & ControlUp )
			{
				points.y -= POSITION_INCREMENT;
			}

			if( joybits & ControlDown )
			{
				points.y += POSITION_INCREMENT;
			}

			if( joybits & ControlLeft )
			{
				points.x -= POSITION_INCREMENT;
			}

			if( joybits & ControlRight )
			{
				points.x += POSITION_INCREMENT;
			}
			if( joybits & ControlA )
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

			if( joybits & ControlB )
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
		}

		Spr_SetPosition (sp, &points);
		Spr_Rotate (sp, angle);
		F2_Draw (gs, sp);


		/* Send the command list to be drawn */
		err = GS_SendList(gs);
		CKERR( err, "GS_SendList");

		/* Make sure it's finished. */
		err = GS_WaitIO(gs);
		CKERR( err, "GS_WaitIO");

		/* Swap the buffers */
		ModifyGraphicsItemVA(viewItem,
						VIEWTAG_BITMAP, bitmaps[curFB],
						TAG_END );

		curFB = 1-curFB;

		err = GS_BeginFrame(gs);
		CKERR(err,"GS_BeginFrame");

		GS_SetDestBuffer( gs, bitmaps[curFB]);
		CLT_ClearFrameBuffer (gs, 0.0, 0.0, 0.0, 0., TRUE, TRUE);
	}


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



	printf( "\nExiting.\n");
	exit (0);


}
