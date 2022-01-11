#ifndef __GRAPHICS_FRAME2D_FRAME2D_H
#define __GRAPHICS_FRAME2D_FRAME2D_H


/******************************************************************************
**
**  @(#) frame2d.h 96/02/20 1.19
**
**  Definitions for 2D framework.
**
******************************************************************************/


#include <kernel/types.h>
#include <kernel/list.h>
#include <graphics/clt/gstate.h>
#include <graphics/pipe/gfxtypes.h>
#include <graphics/pipe/vec.h>
#include <graphics/pipe/geo.h>


/* for now, an arbitrary number */
#define NODE_2D	32

#define SPRMASK		0xfffffff0
#define SPRITEOBJNODE	0x01
#define EXTSPRITEOBJNODE	0x02
#define SHORTSPRITEOBJNODE	0x03

#define GRIDOBJNODE	0x11

typedef Point2 Vector2;

typedef Point2 Quad2[4];

/* generic frame2d object template */
typedef struct Frame2Obj {
  Node	Node;
  uint32	Flags;
} Frame2Obj;

#define F2_SKIP		0x80000000


/*
 * Styles for the triangle commands
 * The style bits are separate and can be OR'ed together
 */

/* just has X and Y coordinates */
#define GEOF2_NULL	0
/* has RGBA color components */
#define GEOF2_COLORS	1
/* has texture coordinates */
#define GEOF2_TEXCOORDS	2


#ifdef __cplusplus
extern "C" {
#endif

/*
 * Immediate mode primitives
 */

Err F2_Draw (GState *gs, const void* f2obj);

Err F2_DrawList (GState *gs, const List* l);

void F2_DrawLine(GState *gs, const Point2 *v0, const Point2 *v1, 
		 const Color4 *c0, const Color4 *c1);

void F2_ColoredLines (GState *gs, int32 numpoints, 
		      const Point2 *points, const Color4 *colors);

void F2_ShadedLines (GState *gs, int32 numpoints, 
		     const Point2 *points, const Color4 *colors);

void F2_FillRect (GState *gs, gfloat x1, gfloat y1, gfloat x2, gfloat y2, 
		  const Color4 *c);

void F2_Point (GState *gs, const gfloat x, const gfloat y, const Color4 *c);

void F2_Points (GState *gs, int32 numpoints, const Point2 *points, 
		const Color4 *colors);

void F2_CopyRect (GState *gs, const void *sptr, const int32 width, 
		  const int32 type, const Point2 *src, const Point2 *dest, 
		  const Point2 *size);

Err F2_TriFan (GState *gs, const int32 style, int32 numpoints,
	       const Point2 *p, const Color4 *c, const TexCoord *tx);

Err F2_TriStrip (GState *gs, const int32 style, int32 numpoints,
		 const Point2 *p, const Color4 *c, const TexCoord *tx);

Err F2_Triangles (GState *gs, const int32 style, int32 numpoints,
		  const Point2 *p, const Color4 *c, const TexCoord *tx);

#ifdef __cplusplus
}
#endif

#endif /* __GRAPHICS_FRAME2D_FRAME2D_H */
