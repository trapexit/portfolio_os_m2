#ifndef __GRAPHICS_FRAME2D_GRIDOBJ_H
#define __GRAPHICS_FRAME2D_GRIDOBJ_H


/******************************************************************************
**
**  @(#) gridobj.h 96/02/20 1.11
**
**  Public definitions for the 2D Grid object.
**
******************************************************************************/


/* definition of Grid Object  */
typedef struct GridObj {
  Node		gro_Node;
  bool		gro_Skip;
  uint32	gro_Width, gro_Height;
  Point2	gro_Position;
  Vector2	gro_HDelta;
  Vector2	gro_VDelta;
  SpriteObj	**gro_SpriteArray;	/* Ptr to array of ptrs to sprite objects */
} GridObj;

#ifdef __cplusplus
extern "C" {
#endif

GridObj *Gro_Create (TagArg *t);
Err Gro_Delete (GridObj *gr);

#define GRO_TAG_NAME	1
#define GRO_TAG_PRIORITY	2

#define GRO_TAG_SPRITEARRAY	(TAG_ITEM_LAST+1)
#define GRO_TAG_WIDTH	(TAG_ITEM_LAST+2)
#define GRO_TAG_HEIGHT	(TAG_ITEM_LAST+3)
#define GRO_TAG_XPOS	(TAG_ITEM_LAST+4)
#define GRO_TAG_YPOS	(TAG_ITEM_LAST+5)
#define GRO_TAG_HDELTAX	(TAG_ITEM_LAST+6)
#define GRO_TAG_HDELTAY	(TAG_ITEM_LAST+7)
#define GRO_TAG_VDELTAX	(TAG_ITEM_LAST+8)
#define GRO_TAG_VDELTAY	(TAG_ITEM_LAST+9)

Err Gro_SetSpriteArray (GridObj *gr, SpriteObj **sp);
Err Gro_SetWidth (GridObj *gr, uint32 width);
Err Gro_SetHeight (GridObj *gr, uint32 height);
Err Gro_SetPosition (GridObj *gr, Point2 *position);
Err Gro_SetHDelta (GridObj *gr, Vector2 *hdelta);
Err Gro_SetVDelta (GridObj *gr, Vector2 *vdelta);

SpriteObj **Gro_GetSpriteArray (GridObj *gr);
uint32 Gro_GetWidth (const GridObj *gr);
uint32 Gro_GetHeight (const GridObj *gr);
void Gro_GetPosition (const GridObj *gr, Point2 *position);
void Gro_GetHDelta (const GridObj *gr, Vector2 *hdelta);
void Gro_GetVDelta (const GridObj *gr, Vector2 *vdelta);

#ifdef __cplusplus
}
#endif


#endif /* __GRAPHICS_FRAME2D_GRIDOBJ_H */
