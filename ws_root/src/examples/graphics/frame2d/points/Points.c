
/******************************************************************************
**
**  @(#) Points.c 96/07/09 1.8
**
******************************************************************************/

/**
|||	AUTODOC -class Examples -group Frame2D -name Points
|||	Illustrates how to draw points.
|||
|||	  Synopsis
|||
|||	    Points
|||
|||	  Description
|||
|||	    Demonstrates how to draw points with the 2D API.
|||
|||	  Associated Files
|||
|||	    Points.c
|||
|||	  Location
|||
|||	    Examples/Graphics/Frame2d
|||
**/


/************ INCLUDES ************/

#include <kernel/task.h>
#include <graphics/frame2d/f2d.h>
#include <graphics/view.h>
#include <misc/event.h>
#include <stdio.h>
#include <kernel/random.h>
#include <stdlib.h>


/************ DEFINES ************/

#define FBWIDTH 320
#define FBHEIGHT 240

#define NUMPOINTS 	( 100 )

#define CKERR(x,y) {Err ick=(x);if(ick<0){printf("Error - %s\n",y);PrintfSysErr(x); exit(1);}}


/********** PROTOTYPES ************/

void ChangeCoords( Point2 *coords );
void ChangeColors( Color4 *colors );
uint32 SnagBits( uint32 x, int p, int n );



/************** main **************/

void main(void)
{
	GState				*gs;
	ControlPadEventData cped;
	uint32				joybits;
	Err					err;
	Item      			bitmaps[3];
	int32				displaySignal;
	Item				viewItem;
	uint32				viewType = VIEWTYPE_16;
	uint32				bmType = BMTYPE_16;
	int32 				numpoints = NUMPOINTS;
	Point2 				points[NUMPOINTS];
	Color4 				colors[NUMPOINTS];
	int32				i;
	Point2				setcoords = { 30.0, 30.0 };
	Color4				setcolors = { 0.1, 0.3, 0.9, 1.0 };
	gfloat				alpha = 1.0;
	char				instructions[] = "\nPoints:\n"
										 "\tThis program takes no input.  Draws 100 points\n"
										 "\tat randomly generated positions with a randomly\n"
										 "\tgenerated color during each frame. Does not use\n"
										 "\tdouble-buffering.\n"
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
	CLT_SetSrcToCurrentDest (gs);

	/* Repeat buffer clearing procedure. */
	err = GS_SendList(gs);
	CKERR(err,"GS_SendList");

	err = GS_WaitIO(gs);
	CKERR(err,"GS_WaitIO");

	ModifyGraphicsItemVA(viewItem,
						VIEWTAG_BITMAP, bitmaps[0],
						TAG_END );

	err = GS_BeginFrame(gs);
	CKERR(err,"GS_BeginFrame");

	printf(instructions);

	while( 1 )
	{
		err = GetControlPad( 1, FALSE, &cped );
		CKERR(err, "GetControlPad" );

		joybits = cped.cped_ButtonBits;

		if( joybits & ControlX )
			break;


		for( i = 0; i < NUMPOINTS; i++ )
		{
			points[i].x = setcoords.x;
			points[i].y = setcoords.y;
			colors[i].r = setcolors.r;
			colors[i].g = setcolors.g;
			colors[i].b = setcolors.b;
			colors[i].a = alpha;

			ChangeCoords( &setcoords );
		}

		ChangeColors( &setcolors );

		/* A single point would be drawn with:
		 * 		F2_Point (GState *gs, gfloat x, gfloat y, Color4 *c);
		 * However, drawing a single point would have
		 * made a very boring example.  Instead, we'll
		 * draw 100 points per frame with the following call.
		 */
		F2_Points (gs, numpoints, points, colors);

		err = GS_SendList(gs);
		CKERR(err,"GS_SendList");

		err = GS_WaitIO(gs);
		CKERR(err,"GS_WaitIO");
	}

	/* Clean up */
	err = DeleteItem(viewItem);
	CKERR(err,"DeleteItem");

	GS_FreeBitmaps(bitmaps, 3);
	GS_FreeLists(gs);

	err = KillEventUtility ();
	CKERR(err,"KillEventUtility");

	err = FreeSignal(displaySignal);
	CKERR(err,"FreeSignal");

	err = CloseGraphicsFolio();
	CKERR(err,"CloseGraphicsFolio");


	printf("Exiting");
	exit(0);
}




/************** SnagBits() **************/
/* Populates the Point2 variable with somewhat random
 * x and y values.
 */

void ChangeCoords( Point2 *coords )
{
	coords->x = (gfloat)((rand()) % 320);
	coords->y = (gfloat)((rand()) % 240);
}



/************** SnagBits() **************/
/* Returns the value located in the bits of x starting at
 * position p and including n bits to the right.
 */

uint32 SnagBits( uint32 x, int p, int n )
{
	return (x >> (p+1-n)) & ~(~0 << n);
}


/********* ChangeColors() **************/
/* Populates the r, g, and b parts of a Color4
 * with somewhat randomly generated values.
 * Tries to keep the screen from going white by
 * rejecting combined r, g, b values over 2.0.
 */

void ChangeColors( Color4 *colors )
{
	uint32	random;

	do
	{
		random = rand();
		colors->r = ((gfloat)SnagBits(random, 24, 8)) / 255;
		colors->g = ((gfloat)SnagBits(random, 16, 8)) / 255;
		colors->b = ((gfloat)SnagBits(random, 8, 8)) / 255;
	} while ((colors->r + colors->g + colors->b) > 2.0 );
}
