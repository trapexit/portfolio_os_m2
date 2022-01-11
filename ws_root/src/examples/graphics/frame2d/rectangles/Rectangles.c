
/******************************************************************************
**
**  @(#) Rectangles.c 96/07/09 1.5
**
******************************************************************************/

/**
|||	AUTODOC -class Examples -group Frame2D -name Rectangles
|||	Illustrates how to move and map the corners of a sprite.
|||
|||	  Synopsis
|||
|||	    Rectangles
|||
|||	  Description
|||
|||	    Shows two methods of drawing rectangles with the 2D API.
|||	    The first simple sets coordinates and a color.  The
|||	    second draws whatever is found at a given area in memory.
|||
|||	  Associated Files
|||
|||	    Rectangles.c
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
#include <kernel/item.h>
#include <stdio.h>
#include <stdlib.h>


/*************************  defines  *************************/

#define CKERR(x,y) {Err ick=(x);if(ick<0){printf("Error - %s\n",y);PrintfSysErr(x); exit(1);}}

#define FBWIDTH 			( 320 )
#define FBHEIGHT 			( 240 )

/*************************  main  *************************/

void main(void)
{
	GState				*gs;
	ControlPadEventData cped;
	uint32				joybits;
	Bitmap				*bm_ptr;
	Item      			bitmaps[3];
	int32				curFB = 0;
	int32				displaySignal;
	Item				viewItem;
	uint32				viewType = VIEWTYPE_16;
	uint32				bmType = BMTYPE_16;
	Err					err;
	Color4				color = { 0.4, 0.2, 0.8, 1.0 };
	gfloat				x1 = 50.0;
	gfloat				y1 = 50.0;
	gfloat				x2 = 200.0;
	gfloat				y2 = 100.0;
 	void				*sptr;
	int32				width = 320;
	int32				type = BMTYPE_16;
	Point2				src = { 50, 50 };
	Point2				dest = { 50, 150 };
	Point2				size = { 150, 50 };
	char				instructions[] = "\nRectangles:\n"
										 "\tShows two methods of drawing rectanges. The top rectangle\n"
										 "\tis drawn based on a set of coordinates and a given color.\n"
										 "\tThe second is drawn based on an area of memory.\n"
										 "\n\tThe joypad will move the upper left coordinate of the\n"
										 "\tfirst rectange.\n"
										 "\n\tPress Stop to quit.\n";


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


	printf(instructions);

	/* Main loop */
	while( 1 )
	{
		err = GetControlPad (1, FALSE, &cped);
		CKERR( err, "GetControlPad" );

		joybits = cped.cped_ButtonBits;

		if( joybits & ControlX )
			break;

		if( joybits & ControlUp )
			y1 -= .5;

		if ( joybits & ControlDown )
			y1 += .5;

		if ( joybits & ControlLeft )
			x1 -= .5;

		if ( joybits & ControlRight )
			x1 += .5;


		/* Draw a rectangle at the given coordinates with the
		 * given color.
		 */
		F2_FillRect( gs, x1, y1, x2, y2, &color );


		/* Grab a pointer to the present bitmap. */
		bm_ptr = (Bitmap *)(LookupItem(bitmaps[curFB]));

		/* Grab a pointer to the buffer of the present bitmap. */
		sptr = (void *)(bm_ptr->bm_Buffer);

		/* Draw a rectangle based upon an area of the present bitmap. */
		F2_CopyRect (gs, sptr, width, type, &src, &dest, &size);

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
		CLT_SetSrcToCurrentDest (gs);
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

	err = CloseGraphicsFolio();
	CKERR(err,"CloseGraphicsFolio");


	printf( "\nExiting.\n");
	exit (0);
}




