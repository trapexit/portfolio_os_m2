/******************************************
**
** @(#) portable_graphics.c 96/07/09 1.30
**
** Simple Device Independant Graphics System to simplify
** porting audio programs like Drumbox and PatchDemo.
**
** Author: Phil Burk
** Copyright 3DO 1995
******************************************/


#include <audiodemo/portable_graphics.h>

#include <graphics/font.h>

#include <graphics/graphics.h>
#include <graphics/view.h>

#include <graphics/frame2d/frame2d.h>
#include <graphics/frame2d/spriteobj.h>
#include <graphics/frame2d/gridobj.h>
#include <kernel/task.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PRT(x)  { printf x; }
#define ERR(x)  PRT(x)
#define DBUG(x) /* PRT(x) */

#define USE_HIGH_RES    (1)

#if USE_HIGH_RES
	#define FBWIDTH  (640)
	#define FBHEIGHT (480)
	#define VIEW_TYPE (VIEWTYPE_16_640_LACE)
#else
	#define FBWIDTH  (320)
	#define FBHEIGHT (240)
	#define VIEW_TYPE (VIEWTYPE_16)
#endif
#define FBDEPTH  (16)

/* #define SYSTEM_FONT ("/remote/Examples/Graphics/Fonts/example.font") */
#define SYSTEM_FONT ("default_14")

/*********************************************************************************/
Err   pgDeleteDisplay( PortableGraphicsContext **pgconPtr )
{
	PortableGraphicsContext *pgcon;

	pgcon = *pgconPtr;

	if( pgcon == NULL ) return -1;

/* Switch screens to make sure everything flushed out. */
	pgSwitchScreens( pgcon );

	if( pgcon->pgcon_Signal ) FreeSignal( pgcon->pgcon_Signal );

	if( pgcon->pgcon_Font ) CloseFont(pgcon->pgcon_Font);

/* Free GState first to make sure nothing in use. */
/* Frees command lists for us. */
	if( pgcon->pgcon_GS ) GS_Delete( pgcon->pgcon_GS );

DBUG(("Bitmap items: %d, %d, %d\n", pgcon->pgcon_Bitmaps[0], pgcon->pgcon_Bitmaps[1],
  pgcon->pgcon_Bitmaps[2]));

	GS_FreeBitmaps( pgcon->pgcon_Bitmaps, pgcon->pgcon_NumBuffers );

/* Free and mark as free in app. */
	free( pgcon );
	*pgconPtr = NULL;

	CloseFontFolio();
	CloseGraphicsFolio();

	DBUG(("pgDeleteDisplay finished.\n"));
#ifdef MEMDEBUG
	DumpMemDebug(NULL);
#endif
	return 0;
}

/*********************************************************************************/
Err   pgCreateDisplay( PortableGraphicsContext **pgconPtr, int32 NumBuffers )
{
	Err       result;
	PortableGraphicsContext *pgcon;

/* Allocate context. */
	pgcon = (PortableGraphicsContext * ) malloc( sizeof( PortableGraphicsContext ) );
	if( pgcon == NULL ) return -1;
	memset( (char *) pgcon, 0,  sizeof( PortableGraphicsContext ) );
#ifdef MEMDEBUG
	DBUG(("Allocated PortableGraphicsContext...\n"));
	DumpMemDebug(NULL);
#endif

	result = OpenGraphicsFolio();
	if (result < 0)
	{
		ERR(("pgOpenDisplay: OpenGraphicsFolio() failed: "));
		PrintfSysErr(result);
		return result;
	}
#ifdef MEMDEBUG
	DBUG(("Opened graphics folio...\n"));
	DumpMemDebug(NULL);
#endif


	result = OpenFontFolio();
	if (result < 0)
	{
		ERR(("pgOpenDisplay: OpenFontFolio() failed: "));
		PrintfSysErr(result);
		return result;
	}

#ifdef MEMDEBUG
	DBUG(("Opened font folio...\n"));
	DumpMemDebug(NULL);
#endif

	if ((pgcon->pgcon_Font = OpenFont(SYSTEM_FONT)) < 0)
		ERR(("pgOpenDisplay: Couldn't open font('%s'): err=0x%x\n", "font.file", pgcon->pgcon_Font ));

	pgcon->pgcon_GS = GS_Create();
	if (result < 0)
	{
		ERR(("pgOpenDisplay: GS_Create failed.\n"));
		return -1;
	}

/* ALlocate signal to be sent when display switches. */
	pgcon->pgcon_Signal = AllocSignal( 0 );
	if( pgcon->pgcon_Signal < 0 )
	{
		ERR(("pgOpenDisplay: AllocSignal failed.\n"));
		return pgDeleteDisplay( &pgcon );
	}

	GS_AllocLists(pgcon->pgcon_GS, NumBuffers, 2048);
#ifdef MEMDEBUG
	DBUG(("GSAllocLists...\n"));
	DumpMemDebug(NULL);
#endif

	GS_AllocBitmaps(pgcon->pgcon_Bitmaps, FBWIDTH, FBHEIGHT, BMTYPE_16, NumBuffers, FALSE);
#ifdef MEMDEBUG
	DBUG(("GSAllocBitmaps...\n"));
	DumpMemDebug(NULL);
#endif

DBUG(("Bitmap items: %d, %d, %d\n", pgcon->pgcon_Bitmaps[0], pgcon->pgcon_Bitmaps[1],
  pgcon->pgcon_Bitmaps[2]));

	GS_SetDestBuffer(pgcon->pgcon_GS, pgcon->pgcon_Bitmaps[0]);

	/* GS_SetZBuffer(pgcon->pgcon_GS, pgcon->pgcon_Bitmaps[NumBuffers]); */
	/* We're not using z-buffering, we're 2-D */

	GS_SetVidSignal(pgcon->pgcon_GS, pgcon->pgcon_Signal);
	pgcon->pgcon_View = CreateItemVA(MKNODEID(NST_GRAPHICS, GFX_VIEW_NODE),
			  VIEWTAG_VIEWTYPE, VIEW_TYPE,
			  VIEWTAG_DISPLAYSIGNAL, pgcon->pgcon_Signal,
			  VIEWTAG_BITMAP, GS_GetDestBuffer(pgcon->pgcon_GS),
			  TAG_END );
	if (pgcon->pgcon_View < 0)
	{
		ERR(("pgOpenDisplay: Creating View failed: "));
		PrintfSysErr(pgcon->pgcon_View);
		return result;
	}
	AddViewToViewList( pgcon->pgcon_View, 0 );

	pgcon->pgcon_NumBuffers = NumBuffers;

	pgSetColor( pgcon, 1.0, 1.0, 1.0 );
	pgClearBackgroundColor( pgcon );
	*pgconPtr = pgcon;
#ifdef MEMDEBUG
	DBUG(("Returning from pgCreate.\n"));
	DumpMemDebug(NULL);
#endif
	return result;
}
/*********************************************************************************/
Err   pgSetColor( PortableGraphicsContext *pgcon, float32 Red, float32 Green, float32 Blue )
{
	Err  result = 0;
	DBUG(("pgSetColor: pgcon = 0x%x, r,g,b = %g, %g, %g\n",
		pgcon, Red, Green, Blue ));
	pgcon->pgcon_FgColor.r = Red;
	pgcon->pgcon_FgColor.g = Green;
	pgcon->pgcon_FgColor.b = Blue;
	pgcon->pgcon_FgColor.a = 1.0;
	return result;
}
/*********************************************************************************/
Err   pgSetBackgroundColor( PortableGraphicsContext *pgcon, float32 Red, float32 Green, float32 Blue )
{
	Err  result = 0;
	DBUG(("pgSetBackgroundColor: pgcon = 0x%x, r,g,b = %g, %g, %g\n",
		pgcon, Red, Green, Blue ));
	pgcon->pgcon_BgColor.r = Red;
	pgcon->pgcon_BgColor.g = Green;
	pgcon->pgcon_BgColor.b = Blue;
	pgcon->pgcon_BgColor.a = 1.0;
	return result;
}
/*********************************************************************************/
Err   pgClearBackgroundColor( PortableGraphicsContext *pgcon )
{
	Err  result = 0;
	DBUG(("pgSetBackgroundColor: pgcon = 0x%x, r,g,b = %g, %g, %g\n",
		pgcon, Red, Green, Blue ));
	pgcon->pgcon_BgColor.r = 0.0;
	pgcon->pgcon_BgColor.g = 0.0;
	pgcon->pgcon_BgColor.b = 0.0;
	pgcon->pgcon_BgColor.a = 0.0;
	return result;
}
/*********************************************************************************/
Err   pgDrawRect( PortableGraphicsContext *pgcon, float32 LeftX, float32 TopY, float32 RightX, float32 BottomY )
{
	DBUG(("pgDrawRect: pgcon = 0x%x, x1,y1 = %g, %g; x2,y2 = %g, %g\n",
		pgcon, LeftX, TopY, RightX, BottomY ));
	F2_FillRect( pgcon->pgcon_GS,
		LeftX*FBWIDTH, TopY*FBHEIGHT, RightX*FBWIDTH, BottomY*FBHEIGHT,
		&pgcon->pgcon_FgColor );

	pgcon->pgcon_GS->gs_SendList(pgcon->pgcon_GS);
	GS_WaitIO(pgcon->pgcon_GS);

	return 0;
}
/*********************************************************************************/
Err   pgMoveTo( PortableGraphicsContext *pgcon, float32 X, float32 Y )
{
	Err  result = 0;
	DBUG(("pgMoveTo: pgcon = 0x%x, x,y = %g, %g\n", pgcon, X, Y ));
	pgcon->pgcon_Points[0].x = X*FBWIDTH;
	pgcon->pgcon_Points[0].y = Y*FBHEIGHT;
	return result;
}
/*********************************************************************************/
Err   pgDrawTo( PortableGraphicsContext *pgcon, float32 X, float32 Y )
{

	DBUG(("pgDrawTo: pgcon = 0x%x, x,y = %g, %g\n", pgcon, X, Y ));
/* Set end of line. */
	pgcon->pgcon_Points[1].x = X*FBWIDTH;
	pgcon->pgcon_Points[1].y = Y*FBHEIGHT;
/* Draw line. */
	F2_DrawLine( pgcon->pgcon_GS,
		&pgcon->pgcon_Points[0], &pgcon->pgcon_Points[1],
		&pgcon->pgcon_FgColor, &pgcon->pgcon_FgColor );

	pgcon->pgcon_GS->gs_SendList(pgcon->pgcon_GS);
	GS_WaitIO(pgcon->pgcon_GS);

/* Move current position to end of last line. */
	pgcon->pgcon_Points[0].x = pgcon->pgcon_Points[1].x;
	pgcon->pgcon_Points[0].y = pgcon->pgcon_Points[1].y;
	return 0;
}

/*********************************************************************************/
Err   pgDrawText( PortableGraphicsContext *pgcon, const char *Text )
{
	PenInfo pen;
	StringExtent se;

	DBUG(("pgDrawText: pgcon = 0x%x, Text = %s\n", pgcon,Text ));

		/* set up pen */
	memset(&pen, 0, sizeof(PenInfo));
	pen.pen_XScale = pen.pen_YScale = 1.0;
	pen.pen_X = pgcon->pgcon_Points[0].x;
	pen.pen_Y = pgcon->pgcon_Points[0].y;       /* top edge of text */
	pen.pen_BgColor =
	    (((int32) (pgcon->pgcon_BgColor.a * 255)) << 24) |      /* determines amount of background to apply */
		(((int32) (pgcon->pgcon_BgColor.r * 255)) << 16) |
		(((int32) (pgcon->pgcon_BgColor.g * 255)) << 8) |
		(((int32) (pgcon->pgcon_BgColor.b * 255)) << 0);
	pen.pen_FgColor =
		(((int32) (pgcon->pgcon_FgColor.r * 255)) << 16) |
		(((int32) (pgcon->pgcon_FgColor.g * 255)) << 8) |
		(((int32) (pgcon->pgcon_FgColor.b * 255)) << 0);

		/* translate top edge to baseline, which is what's necessary for rendering text */
	GetStringExtent (&se, pgcon->pgcon_Font, &pen, Text, strlen(Text));
	pen.pen_Y += TO_BASELINE(&se);

		/* draw */
	DrawString (pgcon->pgcon_GS, pgcon->pgcon_Font, &pen, Text, strlen(Text));

		/* pick up pen position after drawing text */
	pgcon->pgcon_Points[0].x = pen.pen_X;
	pgcon->pgcon_Points[0].y = pen.pen_Y - TO_BASELINE(&se);

	return 0;
}

/*********************************************************************************/
Err   pgSwitchScreens( PortableGraphicsContext *pgcon )
{
	int32 drawScreenIndex;

/* Display the recently drawn screen */
	drawScreenIndex = pgQueryCurrentDrawScreen( pgcon );
	pgDisplayScreen( pgcon, drawScreenIndex );

/* Now draw to the next screen. */
	drawScreenIndex++;
	if( drawScreenIndex >= pgcon->pgcon_NumBuffers ) drawScreenIndex = 0;
	pgDrawToScreen( pgcon, drawScreenIndex );
	return 0;
}

/*********************************************************************************/
Err   pgClearDisplay( PortableGraphicsContext *pgcon, float32 Red, float32 Green, float32 Blue )
{
	CLT_ClearFrameBuffer(pgcon->pgcon_GS, Red, Green, Blue, 0.0, TRUE, TRUE);
	pgcon->pgcon_GS->gs_SendList(pgcon->pgcon_GS);
	GS_WaitIO(pgcon->pgcon_GS);
	return 0;
}

/*********************************************************************************/

/* Lower level pg routines */
Err   pgDisplayScreen( PortableGraphicsContext *pgcon, int32 ScreenIndex )
{
	Err  result = 0;

/* Clear any old signals so we know we have a switched screen. */
	if( GetCurrentSignals() & pgcon->pgcon_Signal ) WaitSignal( pgcon->pgcon_Signal );

/* Switch display on next VBL */
	ModifyGraphicsItemVA(pgcon->pgcon_View,
		VIEWTAG_BITMAP, pgcon->pgcon_Bitmaps[ScreenIndex],
		TAG_END );

/* A signal is sent when the switch occurs. GState will wait on it later. */
	GS_BeginFrame(pgcon->pgcon_GS);

	return result;
}
/*********************************************************************************/
Err   pgDrawToScreen( PortableGraphicsContext *pgcon, int32 ScreenIndex )
{
	return GS_SetDestBuffer(pgcon->pgcon_GS,pgcon->pgcon_Bitmaps[ScreenIndex]);
}
/*********************************************************************************/
int32 pgQueryCurrentDrawScreen( PortableGraphicsContext *pgcon )
{
	Item curBMItem = GS_GetDestBuffer(pgcon->pgcon_GS);
	if (curBMItem == pgcon->pgcon_Bitmaps[0])
		return 0;
	else
		return 1;
}
/*********************************************************************************/


