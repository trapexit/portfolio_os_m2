/******************************************************************************
**
**  @(#) clip.c 96/08/19 1.5
**
******************************************************************************/
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company.
**  All rights reserved. This material contains confidential
**  information that is the property of The 3DO Company. Any
**  unauthorized duplication, disclosure or use is prohibited.
**  
**
******************************************************************************/


#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/mem.h>
#include <graphics/graphics.h>
#include <graphics/bitmap.h>
#include <graphics/view.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "clip.h"


/* DumpScreenContext() prints out the contents of the entire Screen Context	*/
/* structure to the terminal for debugging puposes.							*/

void DumpScreenContext (ScreenContext *SCPtr)
{
	int32	ScreenEntry;

	printf ("\nScreenContext\n");
	
	for (ScreenEntry = 0; ScreenEntry < SCREENBUFFERS; ScreenEntry++)
	{
		printf ("  .sc_BitmapItems[%d] = %d\n", ScreenEntry,
			SCPtr->sc_BitmapItems[ScreenEntry]);
		printf ("  .sc_Bitmaps[%d] = %d\n", ScreenEntry,
			SCPtr->sc_Bitmaps[ScreenEntry]);
		printf ("    .bm = %d.\n", SCPtr->sc_Bitmaps[ScreenEntry]->bm);
		printf ("    .bm_Width = %d.\n",
			SCPtr->sc_Bitmaps[ScreenEntry]->bm_Width);
		printf ("    .bm_Height = %d.\n",
			SCPtr->sc_Bitmaps[ScreenEntry]->bm_Height);
		printf ("    .bm_XOrigin = %d.\n",
			SCPtr->sc_Bitmaps[ScreenEntry]->bm_XOrigin);
		printf ("    .bm_YOrigin = %d.\n",
			SCPtr->sc_Bitmaps[ScreenEntry]->bm_YOrigin);
		printf ("    .bm_ClipWidth = %d.\n",
			SCPtr->sc_Bitmaps[ScreenEntry]->bm_ClipWidth);
		printf ("    .bm_ClipHeight = %d.\n",
			SCPtr->sc_Bitmaps[ScreenEntry]->bm_ClipHeight);
		printf ("    .bm_BufferSize = %d.\n",
			SCPtr->sc_Bitmaps[ScreenEntry]->bm_BufferSize);
		printf ("    .bm_BufMemType = %d.\n",
			SCPtr->sc_Bitmaps[ScreenEntry]->bm_BufMemType);
		printf ("    .bm_BufMemCareBits = %d.\n",
			SCPtr->sc_Bitmaps[ScreenEntry]->bm_BufMemCareBits);
		printf ("    .bm_BufMemStateBits = %d.\n",
			SCPtr->sc_Bitmaps[ScreenEntry]->bm_BufMemStateBits);
		printf ("    .bm_Buffer = %d.\n",
			SCPtr->sc_Bitmaps[ScreenEntry]->bm_Buffer);
		printf ("    .bm_Type = %d.\n",
			SCPtr->sc_Bitmaps[ScreenEntry]->bm_Type);
	}

	printf ("  .sc_ZBufferItem = %d\n", SCPtr->sc_ZBufferItem);
	printf ("  .sc_ZBuffer = %d\n", SCPtr->sc_ZBuffer);

	/*if (SCPtr->sc_ZBufferItem != GFX_NO_Z_BUFFER)
	{
		printf ("    .bm = %d.\n", SCPtr->sc_ZBuffer->bm);
		printf ("    .bm_Width = %d.\n", SCPtr->sc_ZBuffer->bm_Width);
		printf ("    .bm_Height = %d.\n", SCPtr->sc_ZBuffer->bm_Height);
		printf ("    .bm_XOrigin = %d.\n", SCPtr->sc_ZBuffer->bm_XOrigin);
		printf ("    .bm_YOrigin = %d.\n", SCPtr->sc_ZBuffer->bm_YOrigin);
		printf ("    .bm_ClipWidth = %d.\n", SCPtr->sc_ZBuffer->bm_ClipWidth);
		printf ("    .bm_ClipHeight = %d.\n",
			SCPtr->sc_ZBuffer->bm_ClipHeight);
		printf ("    .bm_BufferSize = %d.\n",
			SCPtr->sc_ZBuffer->bm_BufferSize);
		printf ("    .bm_BufMemType = %d.\n",
			SCPtr->sc_ZBuffer->bm_BufMemType);
		printf ("    .bm_BufMemCareBits = %d.\n",
			SCPtr->sc_ZBuffer->bm_BufMemCareBits);
		printf ("    .bm_BufMemStateBits = %d.\n",
			SCPtr->sc_ZBuffer->bm_BufMemStateBits);
		printf ("    .bm_Buffer = %d.\n", SCPtr->sc_ZBuffer->bm_Buffer);
		printf ("    .bm_Type = %d.\n", SCPtr->sc_ZBuffer->bm_Type);
	}*/

	printf ("  .sc_ViewItem = %d\n", SCPtr->sc_ViewItem);
	printf ("  .sc_View = %d\n", SCPtr->sc_View);
	printf ("    .v = %d.\n", SCPtr->sc_View->v);
	printf ("    .v_LeftEdge = %d.\n", SCPtr->sc_View->v_LeftEdge);
	printf ("    .v_TopEdge = %d.\n", SCPtr->sc_View->v_TopEdge);
	printf ("    .v_Width = %d.\n", SCPtr->sc_View->v_Width);
	printf ("    .v_Height = %d.\n", SCPtr->sc_View->v_Height);
	printf ("    .v_PixWidth = %d.\n", SCPtr->sc_View->v_PixWidth);
	printf ("    .v_PixHeight = %d.\n", SCPtr->sc_View->v_PixHeight);
	printf ("    .v_WinLeft = %d.\n", SCPtr->sc_View->v_WinLeft);
	printf ("    .v_WinTop = %d.\n", SCPtr->sc_View->v_WinTop);
	printf ("    .v_Type = %d.\n", SCPtr->sc_View->v_Type);
	printf ("    .v_ViewFlags = %d.\n", SCPtr->sc_View->v_ViewFlags);
	printf ("    .v_Bitmap = %d.\n", SCPtr->sc_View->v_Bitmap);
	printf ("    .v_BitmapItem = %d.\n", SCPtr->sc_View->v_BitmapItem);

	printf ("  .sc_RenderSignal = %d\n", SCPtr->sc_RenderSignal);
	printf ("  .sc_DisplaySignal = %d\n", SCPtr->sc_DisplaySignal);

	printf ("  .sc_FreeZonePtr = %d\n", SCPtr->sc_FreeZonePtr);
	printf ("  .sc_FreeZoneSize = %d\n", SCPtr->sc_FreeZoneSize);

	printf ("  .sc_CurrentScreen = %d.\n\n", SCPtr->sc_CurrentScreen);
}


/* CreateClipDisplay() is a very specialized routine to create an oversized	*/
/* pair of Frame Buffers plus a Z Buffer in a small region of memory.  The	*/
/* purpose is to generate Bitmap Items for 'virtual' screens of size 960	*/
/* pixels wide by 880 pixels tall, but in an interleaved manner to keep		*/
/* memory usage very low.  By overlapping, the entire memory for these		*/
/* screens can be allocated in the space of a single 960x902 pixel region.	*/
/* In addition, only a small portion equivalent to three 320x262 Bitmaps is	*/
/* actual rendered into by the Triangle Engine, so the majority of the		*/
/* allocated memory is free for general purpose use.  The reason for		*/
/* generating such large displays is so that a Triangle Engine clipping		*/
/* window can be offset into the 'virtual' bitmaps by 640 pixels in both	*/
/* the x and y directions thus providing true hardware clipping!			*/

void CreateClipDisplay (ScreenContext *SCPtr)
{
	uint8	*Buffer;
	int32	ReturnVal;
	
	/* Open the Graphics Folio. */
	if ((ReturnVal = OpenGraphicsFolio()) < 0)
		ShutDownClip (OPENGRAPHICS, ReturnVal);

	/* The first step is to generate a Bitmap Item for Buffer 0 that covers	*/
	/* the entire region of memory utilized.  The memory is allocated		*/
	/* safely via the method of creating a Bitmap Item first then			*/
	/* calling AllocMemMasked() based on the Graphics Folio's suggestions.	*/
	if ((SCPtr->sc_BitmapItems[0] =
			CreateItemVA (MKNODEID (NST_GRAPHICS, GFX_BITMAP_NODE),
						BMTAG_WIDTH, (DEFAULT_WIDTH * 3),
						BMTAG_HEIGHT,
							(Y_OFFSET + DEFAULT_HEIGHT + EXTRA_LINES),
						BMTAG_TYPE, DEFAULT_BMTYPE,
						BMTAG_CLIPWIDTH, (DEFAULT_WIDTH * 3),
						BMTAG_CLIPHEIGHT,
							(Y_OFFSET + DEFAULT_HEIGHT + EXTRA_LINES),
						BMTAG_XORIGIN, 0,
						BMTAG_YORIGIN, 0,
						BMTAG_DISPLAYABLE, TRUE,
						BMTAG_RENDERABLE, TRUE,
						BMTAG_BUMPDIMS, TRUE,
						TAG_END)) < 0)
		ShutDownClip (CREATEBITMAP, SCPtr->sc_BitmapItems[0]);

	/* Retrieve the values in the Bitmap structure selected */
	/* by the Graphics Folio.								*/ 
	if ((SCPtr->sc_Bitmaps[0] = (Bitmap *)
			LookupItem (SCPtr->sc_BitmapItems[0])) == NULL)
		ShutDownClip (LOOKUPITEM, SCPtr->sc_BitmapItems[0]);

	/* Use the system-chosen Bitmap info to allocate memory for the	*/
	/* buffer.  AllocMemMasked() insures that the buffer is aligned	*/
	/* according to system requirements.							*/
	if ((Buffer =
			AllocMemMasked (SCPtr->sc_Bitmaps[0]->bm_BufferSize,
			SCPtr->sc_Bitmaps[0]->bm_BufMemType,
	  	 	SCPtr->sc_Bitmaps[0]->bm_BufMemCareBits,
			SCPtr->sc_Bitmaps[0]->bm_BufMemStateBits)) == NULL)
		ShutDownClip (ALLOCMEM, SCPtr->sc_Bitmaps[0]->bm_BufferSize);

	/* Attach Bitmap to BitmapItem. */
	if ((ReturnVal = ModifyGraphicsItemVA (SCPtr->sc_BitmapItems[0],
			BMTAG_BUFFER, Buffer,
			TAG_END)) < 0) ShutDownClip (MODIFYGFX, ReturnVal);

	/* Repeat Bitmap Item creation for Buffer 1. */
	if ((SCPtr->sc_BitmapItems[1] =
			CreateItemVA (MKNODEID (NST_GRAPHICS, GFX_BITMAP_NODE),
						BMTAG_WIDTH, (DEFAULT_WIDTH * 3),
						BMTAG_HEIGHT, (Y_OFFSET + DEFAULT_HEIGHT),
						BMTAG_TYPE, DEFAULT_BMTYPE,
						BMTAG_CLIPWIDTH, (DEFAULT_WIDTH * 3),
						BMTAG_CLIPHEIGHT, (Y_OFFSET + DEFAULT_HEIGHT),
						BMTAG_XORIGIN, 0,
						BMTAG_YORIGIN, 0,
						BMTAG_DISPLAYABLE, TRUE,
						BMTAG_RENDERABLE, TRUE,
						BMTAG_BUMPDIMS, TRUE,
						TAG_END)) < 0)
		ShutDownClip (CREATEBITMAP, SCPtr->sc_BitmapItems[1]);

	/* Retrieve the values in the Bitmap structure selected */
	/* by the Graphics Folio.								*/ 
	if ((SCPtr->sc_Bitmaps[1] = (Bitmap *)
			LookupItem (SCPtr->sc_BitmapItems[1])) == NULL)
		ShutDownClip (LOOKUPITEM, SCPtr->sc_BitmapItems[1]);

	/* Attach Bitmap to BitmapItem.  In this case, the memory has already	*/
	/* been allocated for Bitmap 0, so a pointer is offset into this same	*/
	/* region by a carefully selected number of pages.  This page offset	*/
	/* is very important, because this is what keeps alignments correct		*/
	/* for the Z Buffer and also allows the 'virtual' bitmaps to overlap.	*/
	if ((ReturnVal = ModifyGraphicsItemVA (SCPtr->sc_BitmapItems[1],
			BMTAG_BUFFER, (Buffer + (PAGE_OFFSET1 * PAGESIZE)),
			TAG_END)) < 0) ShutDownClip (MODIFYGFX, ReturnVal);

	/* Create a Bitmap Item for the Z-Buffer. */
	if ((SCPtr->sc_ZBufferItem =
			CreateItemVA (MKNODEID (NST_GRAPHICS, GFX_BITMAP_NODE),
						BMTAG_WIDTH, (DEFAULT_WIDTH * 3),
						BMTAG_HEIGHT, (Y_OFFSET + DEFAULT_HEIGHT),
						BMTAG_TYPE, BMTYPE_16_ZBUFFER,
						BMTAG_CLIPWIDTH, (DEFAULT_WIDTH * 3),
						BMTAG_CLIPHEIGHT, (Y_OFFSET + DEFAULT_HEIGHT),
						BMTAG_XORIGIN, 0,
						BMTAG_YORIGIN, 0,
						BMTAG_RENDERABLE, TRUE,
						BMTAG_BUMPDIMS, TRUE,
						TAG_END)) < 0)
		ShutDownClip (CREATEBITMAP, SCPtr->sc_ZBufferItem);

	/* Retrieve the values in the Bitmap structure selected */
	/* by the Graphics Folio.								*/ 
	if ((SCPtr->sc_ZBuffer = (Bitmap *)
			LookupItem (SCPtr->sc_ZBufferItem)) == NULL)
		ShutDownClip (LOOKUPITEM, SCPtr->sc_ZBufferItem);

	/* Attach Bitmap to BitmapItem.  Again, the memory was already			*/
	/* allocated for Bitmap 0, so a pointer is offset into this region to	*/
	/* generate the Z-Buffer.  The page offset was carefully selected to	*/
	/* keep the Z-Buffer properly aligned with the other Bitmaps.			*/
	if ((ReturnVal = ModifyGraphicsItemVA (SCPtr->sc_ZBufferItem,
				BMTAG_BUFFER, (Buffer + (PAGE_OFFSETZ * PAGESIZE)),
				TAG_END)) < 0) ShutDownClip (MODIFYGFX, ReturnVal);

	/* Allocate a Render Signal for VBL syncronization. */
	if ((SCPtr->sc_RenderSignal = AllocSignal (0)) <= 0)
		ShutDownClip (ALLOCSIGNAL, SCPtr->sc_RenderSignal);
		
	/* Create a View through which all Bitmaps will be made visible.  To	*/
	/* get the offset clip region to show up, this view is shifted into the	*/
	/* 'virtual' Bitmaps by an x and y offset of 640 pixels in both dirs.	*/
	if ((SCPtr->sc_ViewItem =
			CreateItemVA (MKNODEID (NST_GRAPHICS, GFX_VIEW_NODE),
						VIEWTAG_VIEWTYPE, DEFAULT_MODE,
						VIEWTAG_TOPEDGE, 0,
						VIEWTAG_LEFTEDGE, 0,
						VIEWTAG_WINTOPEDGE, Y_OFFSET,
						VIEWTAG_WINLEFTEDGE, X_OFFSET,
						VIEWTAG_BITMAP, SCPtr->sc_BitmapItems[0],
						VIEWTAG_PIXELWIDTH,	DEFAULT_WIDTH,
						VIEWTAG_PIXELHEIGHT, DEFAULT_HEIGHT,
						VIEWTAG_AVGMODE, DEFAULT_AVG,
						VIEWTAG_RENDERSIGNAL, SCPtr->sc_RenderSignal,
						VIEWTAG_BESILENT, TRUE, /* No signal when creating. */
						TAG_END)) < 0)
		ShutDownClip (CREATEVIEW, SCPtr->sc_ViewItem);

	/* Retrieve the pointer to the created View structure.	*/
	if ((SCPtr->sc_View = LookupItem (SCPtr->sc_ViewItem)) == NULL)
		ShutDownClip (LOOKUPITEM, SCPtr->sc_ViewItem);

	/* Attach the standard view to the display screen. */
	AddViewToViewList (SCPtr->sc_ViewItem, 0);
	
	/* Let the user know where the safe free memory region is. */
	SCPtr->sc_FreeZonePtr = Buffer;
	SCPtr->sc_FreeZoneSize = (DEFAULT_WIDTH * 3) * Y_OFFSET * 2;						/* Size in bytes of free region.		*/
	
	SCPtr->sc_CurrentScreen = 0;
}


/* Destroy all Bitmaps and free the associated buffers and close the	*/
/* Graphics Folio.  Works in conjunction with CreateClipDisplay().		*/

void DestroyClipDisplay (ScreenContext *SCPtr)
{
	int32	ScreenEntry, ReturnVal;
	
	if (SCPtr->sc_ViewItem)
		if ((ReturnVal = DeleteItem (SCPtr->sc_ViewItem)) < 0)
			ShutDownClip (DELETEITEM, ReturnVal);

	if (SCPtr->sc_ZBufferItem)
	{
		FreeMem (SCPtr->sc_ZBuffer->bm_Buffer,
			SCPtr->sc_ZBuffer->bm_BufferSize);
		if ((ReturnVal = DeleteItem (SCPtr->sc_ZBufferItem)) < 0)
			ShutDownClip (DELETEITEM, ReturnVal);
	}
	
	for (ScreenEntry = 0; ScreenEntry < SCREENBUFFERS; ScreenEntry++)
	{
		if (SCPtr->sc_BitmapItems[ScreenEntry])
		{
			FreeMem (SCPtr->sc_Bitmaps[ScreenEntry]->bm_Buffer,
				SCPtr->sc_Bitmaps[ScreenEntry]->bm_BufferSize);
			if ((ReturnVal =
					DeleteItem (SCPtr->sc_BitmapItems[ScreenEntry])) < 0)
				ShutDownClip (DELETEITEM, ReturnVal);
		}
	}

    printf("about to free render and display signals\n");

    if ((ReturnVal = FreeSignal (SCPtr->sc_RenderSignal)) < 0)
		ShutDownClip (FREESIGNAL, ReturnVal);
	
	printf("finished freeing render and display signals\n");
		
	CloseGraphicsFolio();
}


/* Brain-dead error handling and shutdown by death of Task. */

void ShutDownClip (int32 ErrorCode, int32 ErrorVal)
{
	if (ErrorCode > 0)
	{
		printf ("Could not ");

		switch (ErrorCode)
		{
			case OPENGRAPHICS:  
				printf ("open graphics folio.  ");
				break;
			case CREATEBITMAP:  
				printf ("create Bitmap Item.  ");
				break;
			case CREATEVIEW:  
				printf ("create View Item.  ");
				break;
			case LOOKUPITEM:  
				printf ("find [returned] Item via LookupItem().  ");
				break;
			case DELETEITEM:
				printf ("delete Item.  ");
				break;
			case ALLOCSIGNAL:  
				printf ("allocate signal bits.  ");
				break;
			case FREESIGNAL:  
				printf ("free signal bits.  ");
				break;
			case ALLOCMEM:  
				printf ("allocate [returned] bytes.  ");
				break;
			case REALLOCMEM:  
				printf ("reallocate [returned] bytes.  ");
				break;
			case MODIFYGFX:
				printf ("modify graphics Item.  ");
				break;
			default:
				break;
		}
		
		printf ("Returned %d.\n", ErrorVal);
		if (ErrorVal < 0) PrintfSysErr (ErrorVal);
	}
	
	printf ("\nHardware Clip Test Over.\n");
	exit (1);
}
