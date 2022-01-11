
/******************************************************************************
**
**  @(#) DrawLines.c 96/07/09 1.5
**
******************************************************************************/

/**
|||	AUTODOC -class Examples -group Frame2D -name DrawLines
|||	Illustrates three methods of drawing lines.
|||
|||	  Synopsis
|||
|||	    DrawLines
|||
|||	  Description
|||
|||	    This program demonstrates how to draw single and multiple
|||	    lines and how to call F2_DrawLine(), F2_ColoredLines(), and
|||	    F2_ShadedLines().
|||
|||	  Associated Files
|||
|||	    DrawLines.c
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


/*************************  main  *************************/

void main(void)
{
	GState				*gs;
	ControlPadEventData cped;
	uint32				joybits;
	Item      			bitmaps[3];
	int32				curFB = 0;
	int32				displaySignal;
	Item				viewItem;
	uint32				viewType = VIEWTYPE_16;
	uint32				bmType = BMTYPE_16;
	Err					err;
	int32				numpoints = 5;
	Point2				reg_points[] = {{50, 50},
								   		{50,100},
								   		{100,100},
								   		{100, 50},
								   		{50, 50} };
	Point2				colored_points[] = {{100,50},
								   			{100,100},
								   			{150,100},
								   			{150, 50},
								   			{100, 50} };
	Point2				shaded_points[] = { {200,50},
								   			{200,100},
								   			{250,100},
								   			{250, 50},
								   			{200, 50} };
	Color4				colors[] = { { 0.8, 0.0, 0.1, 1.0 },
									 { 0.0, 0.8, 0.1, 1.0 },
									 { 0.0, 0.1, 0.8, 1.0 },
									 { 0.5, 0.0, 0.5, 1.0 },
									 { 0.8, 0.0, 0.1, 1.0 } };
  	char				instructions[] = "\nDrawLines:\n"
										 "\tThis program takes no input. It simply demonstrates\n"
										 "\tthree methods of drawing lines. The fact that this looks\n"
										 "\tlike the number 100 is completely coincidental.\n"
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

	printf(instructions);

	/* Main loop */
	while( 1 )
	{
		err = GetControlPad (1, FALSE, &cped);
		CKERR( err, "GetControlPad");

		joybits = cped.cped_ButtonBits;

		if( joybits & ControlX )
		{
			break;
		}


		F2_DrawLine(gs, &reg_points[0], &reg_points[1], &colors[0], &colors[1] );
		F2_ColoredLines (gs, numpoints, colored_points, colors);
		F2_ShadedLines (gs, numpoints, shaded_points, colors);

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

	err = CloseGraphicsFolio();
	CKERR(err,"CloseGraphicsFolio");

	printf( "Exiting.\n");
	exit (0);
}
