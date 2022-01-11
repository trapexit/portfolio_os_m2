
/******************************************************************************
**
**  @(#) RenderOrder.c 96/07/09 1.4
**
******************************************************************************/

/**
|||	AUTODOC -class Examples -group Frame2D -name RenderOrder
|||	Illustrates how to link sprites and control their rendering order.
|||
|||	  Synopsis
|||
|||	    RenderOrder <filename.utf> [ <filename.utf> <filename.utf> ... ]
|||
|||	  Description
|||
|||	    This program loads up to ten textures, puts them into
|||	    a list, and draws them all with a call to F2_DrawList.
|||
|||	    The A button cycles control of the sprites. The directional
|||	    pad moves the sprite. The B and C buttons move the sprite
|||	    forward and backward through the list.
|||
|||	  Associated Files
|||
|||	    RenderOrder.c
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
#define DEPTH 				( 16 )
#define POSITION_INCREMENT 	( 1 )

#define MAX_NUM_SPRITES		( 10 )

/*************************  main  *************************/

void main(int argc, char **argv)
{
	GState				*gs;
	SpriteObj			*sp[MAX_NUM_SPRITES];
	ControlPadEventData cped;
	Item      			bitmaps[3];
	int32				curFB = 0;
	uint32				joybits, oldbits;
	int32				displaySignal;
	Item				viewItem;
	uint32				viewType = VIEWTYPE_16;
	uint32				bmType = BMTYPE_16;
	char 				*fname[MAX_NUM_SPRITES];
	Err					err;
	int32				which = 0;
	gfloat				x_delta, y_delta;
	int32				i;
	int32				texcount;
	Point2				points[MAX_NUM_SPRITES];
	List				sp_list;
	Node				*nodeptr;
	char				instructions[] = "\nRenderOrder:\n"
										 "\tDemonstrates how to link sprites and control\n"
										 "\tthe order in which they are drawn.\n"
										 "\tMove sprites with control pad.\n"
										 "\tA toggles which sprite can be moved.\n"
										 "\tB moves sprite forward through the list.\n"
										 "\tC moves sprite backward through the list.\n"
										 "\n\tPress Stop to quit.\n";

	/* Check for filename */
	if (argc<2)
	{
		printf("RenderOrder - Filename(s) required.\n");
		printf("     usage:  RenderOrder <filename.utf> [ <filename.utf> <filename.utf> ... ]\n");
		exit (1);
	}
	else
	{
		for( i = 0, texcount = 0;  (i < MAX_NUM_SPRITES) && (i < (argc - 1)); i++ )
		{
			texcount++;
			fname[i] = argv[i+1];
			printf("Loading %s...\n", fname[i] );

			/* Gimme a sprite!  Create a sprite, and then load the utf file into it. */
			sp[i] = Spr_Create(0);
			Spr_LoadUTF (sp[i], fname[i]);

			/* Set the sprite's corners. */
			Spr_ResetCorners (sp[i], SPR_TOPLEFT);

			points[i].x = 30 + (10 * i);
			points[i].y = 30 + (10 * i);

			/* Move the sprites to the created postion. */
			Spr_SetPosition (sp[i], &(points[i]));

			/* Set the color output of the texture to the texture color. */
			Spr_SetTextureAttr (sp[i], TXA_ColorOut, TX_BlendOutSelectTex );
		}

		if( i == (argc - 2) )
			printf("The last texture was not loaded.\n");
		else if( i < (argc - 1) )
			printf("The last %i textures were not loaded.\n", (argc - 1 - i));
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
							VIEWTAG_BITMAP, bitmaps[0],
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


	/* List stuff */
	PrepList(&sp_list);
	for( i = 0; i < texcount; i++ )
	{
		InsertNodeFromHead(&sp_list, &(sp[i]->spr_Node));
	}


	printf(instructions);

	joybits = 0;

	/* Main loop */
	while( 1 )
	{
		oldbits = joybits;

		err = GetControlPad (1, FALSE, &cped);
		CKERR( err, "GetControlPad" );

		joybits = cped.cped_ButtonBits;

		if( joybits & ControlX )
		{
			break;
		}

		if( (joybits & ControlA) && (oldbits != joybits)  )
		{
			/* Rotate through list of sprites */
			if( which == (texcount - 1))
				which = 0;
			else
				which++;
		}
		else if( (joybits & ControlB)  && (oldbits != joybits) )
		{
			/* Moving present texture forward through list */
			if( (Node *)(sp[which]) == LastNode(&sp_list) )
			{
				RemNode((Node *)(sp[which]) );
				AddHead(&sp_list, (Node *)(sp[which]));
			}
			else
			{
				nodeptr = NextNode( (Node *)(sp[which]) );
				RemNode((Node *)(sp[which]) );
				InsertNodeAfter(nodeptr, (Node *)(sp[which]));
			}
		}
		else if( (joybits & ControlC) && (oldbits != joybits) )
		{
			/* Moving present texture backward through list */
			if( ((Node *)(sp[which])) == FirstNode(&sp_list)  )
			{
				RemNode((Node *)(sp[which]) );
				AddTail(&sp_list, (Node *)(sp[which]));
			}
			else
			{
				nodeptr = PrevNode( (Node *)(sp[which])  );
				RemNode((Node *)(sp[which]) );
				InsertNodeBefore(nodeptr, (Node *)(sp[which]));
			}
		}

		x_delta = y_delta = 0;

		if( joybits & ControlUp )
			y_delta = -1;
		if( joybits & ControlDown )
			y_delta = 1;
		if( joybits & ControlLeft )
			x_delta = -1;
		if( joybits & ControlRight )
			x_delta = 1;

		err = Spr_Translate (sp[which], x_delta, y_delta);
		CKERR( err, "Spr_Translate" );


		/* Draw the textures in the list */
		err = F2_DrawList (gs, &sp_list);
		CKERR( err, "F2_DrawList" );

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
		CLT_ClearFrameBuffer (gs, 0.4, 0.2, 0.8, 0., TRUE, TRUE);

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

	for( i = 0; i < texcount; i++ )
	{
		err = Spr_Delete(sp[i]);
		CKERR(err,"Spr_Delete");
	}

	err = CloseGraphicsFolio();
	CKERR(err,"CloseGraphicsFolio");

	printf( "Exiting.\n");
	exit (0);


}




