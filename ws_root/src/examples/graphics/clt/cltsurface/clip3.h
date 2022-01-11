/******************************************************************************
**
**  @(#) clip3.h 96/08/20 1.4
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


/* ClipDisplay Error Codes. */

#define QUIT			0
#define OPENGRAPHICS	1
#define CREATEBITMAP	2
#define CREATEVIEW		3
#define LOOKUPITEM		4
#define DELETEITEM		5
#define ALLOCSIGNAL		6
#define FREESIGNAL		7
#define ALLOCMEM		8
#define REALLOCMEM		9
#define MODIFYGFX		10


/* Type definitions and constants used in ClipDisplay routines. */

#define SCREENBUFFERS	3				/* Number of screen buffers.  Double buffered = 2.		*/
#define DEFAULT_WIDTH	320
#define DEFAULT_HEIGHT	240
#define	DEFAULT_BMTYPE	BMTYPE_16		/* Choose:  BMTYPE_16, BMTYPE_32, BMTYPE_16_ZBUFFER.	*/

/* Choose:  VIEWTYPE_16, VIEWTYPE_16_LACE, VIEWTYPE_32,VIEWTYPE_32_LACE, VIEWTYPE_16_640,		*/
/*			VIEWTYPE_16_640_LACE, VIEWTYPE_32_640, VIEWTYPE_32_640_LACE, VIEWTYPE_YUV_16,		*/
/*			VIEWTYPE_YUV_16_LACE, VIEWTYPE_YUV_32, VIEWTYPE_YUV_32_LACE, VIEWTYPE_YUV_16_640,	*/
/*			VIEWTYPE_YUV_16_640_LACE, VIEWTYPE_YUV_32_640,VIEWTYPE_YUV_32_640_LACE.				*/

#define DEFAULT_MODE	VIEWTYPE_16
#define DEFAULT_AVG		(AVGMODE_H | AVGMODE_V | AVG_DSB_0 | AVG_DSB_1)	/* All averaging on.	*/

/* Sizes selected to work with Bitmap and Z Buffer page alignment to allow the hardware			*/
/* clipping trickery to work.  Don't mess with these values unless you know what you are doing.	*/

#define PAGESIZE		4096	/* Size in bytes of graphics pages for alignment calculations.	*/
#define VIRTUAL_WIDTH	1312	/* Width in pixels of the oversized 'virtual' bitmap.			*/
#define X_OFFSET		960		/* Number of pixels offset in x direction to start clip region.	*/
#define Y_OFFSET		640		/* Number of pixels offset in y direction to start clip region.	*/
#define	PAGE_OFFSET		4		/* Relative number of pages offset into Buffer 0 for buffers.	*/
#define PAGE_OFFSETZ	37		/* Relative number of pages offset into Buffer 0 for Z Buffer.	*/
#define	EXTRA_LINES		58		/* Number of lines extra to cover the largest offset from 0.	*/


/* A structure to hold useful common screen information.	*/
typedef struct ScreenContext
{
	/* The Bitmap structure holds the following useful info:  bm_Width, bm_Height, bm_XOrigin, bm_YOrigin,	*/
	/* bm_ClipWidth, bm_ClipHeight, bm_BufferSize, bm_BufMemType, bm_BufMemCareBits, bm_BufMemStateBits,	*/
	/* bm_Buffer, and bm_Type.																				*/

	Item		sc_BitmapItems[SCREENBUFFERS];			/* Bitmap Items for the screen buffers.	*/
	Bitmap		*sc_Bitmaps[SCREENBUFFERS];				/* Pointers to Bitmap structures.		*/
	Item		sc_ZBufferItem;							/* Bitmap Item for associated ZBuffer.	*/
	Bitmap		*sc_ZBuffer;							/* ZBuffer Bitmap struct pointer.		*/

	/* The View structure holds the following useful info:  v_LeftEdge, v_TopEdge, v_Width, v_Height,		*/
	/* v_PixWidth, v_PixHeight, v_WinLeft, v_WinTop, v_Type, v_ViewFlags, v_Bitmap, and v_BitmapItem.		*/

	Item		sc_ViewItem;							/* View Item for displaying Bitmaps.	*/
	View		*sc_View;								/* Pointer to View structure.			*/

	int32		sc_RenderSignal;						/* Signal indicating safe to render.	*/
	int32		sc_DisplaySignal;						/* View Signal when safe to display.	*/
	
    uint32		sc_CurrentScreen;						/* Currently displayed Screen.			*/
	
	/* Information specific to the hardware clipping routines.  A pointer and a size in	bytes of the free	*/
	/* region that was allocated but is untouched by the Triangle Engine, and may be safely utilized.		*/
	
	uint8		*sc_FreeZonePtr;						/* Pointer to free region of memory.	*/
	uint32		sc_FreeZoneSize;						/* Size in bytes of free region.		*/
} ScreenContext;


/* ClipDisplay Prototypes. */

void CreateClip3Display (ScreenContext *SCPtr);
void DestroyClipDisplay (ScreenContext *SCPtr);
void ShutDownClip (int32 ErrorCode, int32 ErrorVal);
void DumpScreenContext (ScreenContext *SCPtr);
