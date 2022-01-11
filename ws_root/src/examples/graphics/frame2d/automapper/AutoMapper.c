
/******************************************************************************
**
**  @(#) AutoMapper.c 96/07/09 1.4
**
******************************************************************************/

/**
|||	AUTODOC -class Examples -name AutoMapper
|||	Illustrates how to move and map the corners of a sprite.
|||
|||	  Synopsis
|||
|||	    AutoMapper <filename.utf>
|||
|||	  Description
|||
|||	    This progam loads a given utf file into a sprite and
|||	    allows the user to manipulate each corner of the sprite.
|||	    If no control pad input is received after about five
|||	    seconds, the program enters an automatic mode, moving
|||	    the sprite around the screen.
|||
|||	  Associated Files
|||
|||	    AutoMapper.c
|||
|||	  Location
|||
|||	    TBD
|||
**/



/*************************  INCLUDES  ***************************/

#include <kernel/task.h>
#include <graphics/frame2d/f2d.h>
#include <graphics/view.h>
#include <misc/event.h>
#include <stdio.h>
#include <stdlib.h>

/*************************  DEFINES  ***************************/

#define CKERR(x,y) {Err ick=(x);if(ick<0){printf("Error - %s\n",y);PrintfSysErr(x); exit(1);}}

#define FBWIDTH 320
#define FBHEIGHT 240

#define TIMETOWAIT	250


/*************************  PROTOTYPES  ***************************/

int32 HandleMarkerMovement( Point2 *marker, Point2 *points, long joybits );


/*************************  HandleMarkerMovement  ***************************/
/*
 *  Lets the user move a given corner of the texture around the screen.
 */

#define MAX_X 		( 310.0 )
#define MIN_X		( 10.0 )
#define MAX_Y		( 230.0 )
#define	MIN_Y		( 10.0 )
#define INCREMENT	( 2.0 )

int32 HandleMarkerMovement( Point2 *marker, Point2 *points, long joybits )
{
	int32 movement = 0;

	if( (joybits & ControlUp) &&  (marker->y > MIN_Y) )
	{
		marker->y -= INCREMENT;
		movement = 1;
	}

	if( (joybits & ControlDown) && (marker->y < MAX_Y) )
	{
		marker->y += INCREMENT;
		movement = 1;
	}

	if( (joybits & ControlLeft) && (marker->x > MIN_X) )
	{
		marker->x -= INCREMENT;
		movement = 1;
	}

	if( (joybits & ControlRight) && (marker->x < MAX_X) )
	{
		marker->x += INCREMENT;
		movement = 1;
	}

	if( movement )
	{
		points->x = marker->x;
		points->y = marker->y;
	}

	return movement;
}

/********************************  main *********************************/

void main(int argc, char **argv)
{
	GState				*gs;
	SpriteObj			*sp;
	int					i;
	ControlPadEventData cped;
	Item      			bitmaps[3];
	int32				curFB = 0;
	int32				displaySignal;
	Item				viewItem;
	uint32				viewType = VIEWTYPE_16;
	uint32				bmType = BMTYPE_16;
	char 				*fname;
	Color4				color = { 0.0, 0.7, 0.2, 1.0 };
	Err					err;
	Point2				marker[2] = 0;
	uint32				joybits, oldjoybits;
	int32				movement;
	int32				noinput = 0;
	int32				which = 0;
	Point2				points[4] = { 0 };
	Point2				delta[4] = { { 1, 1},
									 { -1, 1},
									 { -1, -1},
									 { 1, 1} };
	char				instructions[] = "\nAutoMapper:\n"
										 "\tMove corners with the control pad.\n"
										 "\tSelect corner to control by pressing A.\n"
										 "\tIf no input is received, program enters auto mode\n"
										 "\tafter five seconds.\n"
										 "\n\tPress Stop to quit.\n";


	/* Check for filename */
	if (argc<2)
	{
		printf("AutoMapper - Filename required.\n");
		printf("     usage:  AutoMapper <filename.utf>\n");
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


	/* Starting position for the marker rectangle */
	marker[0].x = 100.0;
	marker[0].y = 90.0;
	marker[1].x = 104.0;
	marker[1].y = 94.0;


	/* Populate points array with corners of the sprite */
	points[0].x = 100.0;
	points[0].y = 90.0;
	points[1].x = 100 + Spr_GetWidth(sp);
	points[1].y = 90.0;
	points[2].x = 100 + Spr_GetWidth(sp);
	points[2].y = 90.0 + Spr_GetHeight(sp);
	points[3].x = 100.0;
	points[3].y = 90.0 + Spr_GetHeight(sp);


	err = Spr_ResetCorners (sp, SPR_TOPLEFT);
	CKERR( err, "Spr_ResetCorners");

	/* Slicing the sprite horizontally and vertically with the
	 * next two calls helps smooth the appearance of the sprite
	 * when it is being mapped and warped.
	 */
	err = Spr_SetHSlice (sp, 8);
	CKERR( err, "Spr_SetHSlice");

	Spr_SetVSlice (sp, 8);
	CKERR( err, "Spr_SetVSlice");


	printf(instructions);

	joybits = 0;

	while( 1 )
	{
		oldjoybits = joybits;

		err = GetControlPad (1, FALSE, &cped);
		CKERR( err, "GetControlPad");

		joybits = cped.cped_ButtonBits;

		if( joybits & ControlX )
		{
			break;
		}
		else if( (joybits & ControlA) &&  !(oldjoybits & ControlA) )
		{
			/* Rotate control of the maker. */

			if( which < 3 )
				which++;
			else
				which = 0;

			marker[0].x = points[which].x;
			marker[0].y = points[which].y ;

			noinput = 0;
		}
		else
		{
			movement = HandleMarkerMovement( &marker[0], &points[which], joybits );

			if (movement)
				noinput = 0;
			else
				noinput++;

			if( noinput > TIMETOWAIT )
			{
				for( i = 0; i < 4; i++ )
				{
					points[i].x += delta[i].x;
					points[i].y += delta[i].y;

					if ((points[i].x >= MAX_X) || (points[i].x <= MIN_X))
						delta[i].x = -delta[i].x;
					if ((points[i].y >= MAX_Y) || (points[i].y <= MIN_Y))
						delta[i].y = -delta[i].y;
				}
			}
		}

		err = Spr_MapCorners (sp, points, SPR_CENTER);
		CKERR( err, "Spr_MapCorners");

		F2_Draw (gs, sp);
		CKERR( err, "F2_Draw");

		if( noinput <= TIMETOWAIT )
		{
			/* If we're not in automatic mode, set the marker's
			 * position and draw it.
			 */
			marker[1].x = marker[0].x + 4;
			marker[1].y = marker[0].y + 4;

			F2_FillRect( gs, marker[0].x,  marker[0].y,  marker[1].x,  marker[1].y, &color );
		}

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

	err = Spr_Delete(sp);
	CKERR(err,"Spr_Delete");

	err = CloseGraphicsFolio();
	CKERR(err,"CloseGraphicsFolio");

	printf( "Exiting.\n");
	exit (0);


}

