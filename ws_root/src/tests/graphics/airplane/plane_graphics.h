#ifndef _PLANE_GRAPHICS_H
#define _PLANE_GRAPHICS_H
/******************************************************************************
**
**  @(#) plane_graphics.h 95/07/07 1.1
**
******************************************************************************/


/*
** Rendering of terrain.
**
** Author: phil Burk
** Copyright 1995 3DO
** All Rights Reserved
*/

#include <kernel/types.h>
#include "stdio.h"
#include "math.h"

#include <graphics/gp.h>
#include <graphics/graphics.h>
#include <graphics/view.h>
#include <graphics/frame2d/frame2d.h>
#include <graphics/font.h>

#include <graphics/frame2d/spriteobj.h>
#include <graphics/frame2d/gridobj.h>
#include <graphics/gfxutils/putils.h>

#include "plane_physics.h"
#include "terrain.h"

typedef struct DemoGraphicsContext
{
	GP        *dgc_GP;
	Item       dgc_Font;
	Item       dgc_View;
	Transform *dgc_ModelView;
	TextState *dgc_TextState;
	TextStencil *dgc_TextStencil;
	float32    dgc_FrameRate;
} DemoGraphicsContext;

Err InitDemoGraphics( DemoGraphicsContext *dgc, int argc, char **argv );
Err TermDemoGraphics( DemoGraphicsContext *dgc );
Err DrawWorld( DemoGraphicsContext *dgc, PlaneStatus *plst, Terrain *tran );

#define SYSTEM_FONT ("default_14")

#define FBWIDTH  (320)
#define FBHEIGHT (240)

#endif /* _PLANE_GRAPHICS_H */
