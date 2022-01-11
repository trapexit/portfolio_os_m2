#ifndef __AUDIODEMO_PORTABLE_GRAPHICS_H
#define __AUDIODEMO_PORTABLE_GRAPHICS_H

/******************************************
**
** @(#) portable_graphics.h 96/07/09 1.11
**
** Simple Device Independant Graphics System to simplify
** porting audio programs like Drumbox and PatchDemo.
**
** Author: Phil Burk
** Copyright 3DO 1995
******************************************/

#include <kernel/types.h>
#include <graphics/graphics.h>
#include <graphics/view.h>
#include <graphics/frame2d/frame2d.h>
#include <graphics/pipe/col.h>

#define MAX_FRAMEBUFFERS		3        /* 2 frame bufs, 1 z-buffer */

typedef struct PortableGraphicsContext
{
	GState   *pgcon_GS;
	Point2    pgcon_Points[2];
	Color4    pgcon_BgColor;
	Color4    pgcon_FgColor;
	Item      pgcon_Font;
	Item      pgcon_View;
	Item      pgcon_Bitmaps[MAX_FRAMEBUFFERS];
	int32     pgcon_NumBuffers;   /* 2 when double buffered. */
	int32     pgcon_Signal;       /* Signal sent when display switches. */
} PortableGraphicsContext;

/************ Prototypes *************************/


Err   pgCreateDisplay( PortableGraphicsContext **pgconPtr, int32 NumBuffers );
Err   pgSetColor( PortableGraphicsContext *pgcon, float32 Red, float32 Green, float32 Blue );
Err   pgSetBackgroundColor( PortableGraphicsContext *pgcon, float32 Red, float32 Green, float32 Blue );
Err   pgClearBackgroundColor( PortableGraphicsContext *pgcon );
Err   pgDrawRect( PortableGraphicsContext *pgcon, float32 LeftX, float32 Topy, float32 RightX, float32 BottomY );
Err   pgMoveTo( PortableGraphicsContext *pgcon, float32 X, float32 Y );
Err   pgDrawTo( PortableGraphicsContext *pgcon, float32 X, float32 Y );
Err   pgDrawText( PortableGraphicsContext *pgcon, const char *Text );
Err   pgSwitchScreens( PortableGraphicsContext *pgcon );
Err   pgClearDisplay( PortableGraphicsContext *pgcon, float32 Red, float32 Green, float32 Blue );
Err   pgDeleteDisplay( PortableGraphicsContext **pgconPtr );

/* Lower level pg routines */
Err   pgDisplayScreen( PortableGraphicsContext *pgcon, int32 ScreenIndex );
Err   pgDrawToScreen( PortableGraphicsContext *pgcon, int32 ScreenIndex );
int32 pgQueryCurrentDrawScreen( PortableGraphicsContext *pgcon );

#endif /*
#define __AUDIODEMO_PORTABLE_GRAPHICS_H */
