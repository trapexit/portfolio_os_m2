#ifndef __GRAPHICS_FRAME2D_SPRITEOBJ_H
#define __GRAPHICS_FRAME2D_SPRITEOBJ_H


/******************************************************************************
**
**  @(#) spriteobj.h 96/02/22 1.26
**
**  Definitions for 2D Sprite objects.
**
******************************************************************************/


#include <kernel/types.h>
#include <graphics/frame2d/frame2d.h>
#include <graphics/clt/clt.h>
#include <graphics/clt/clttxdblend.h>
#include <graphics/pipe/col.h>

typedef struct TexBlendData {
  CltTxData		*tb_txData	;
  CltPipControlBlock	*tb_PCB;
  CltSnippet	tb_TAB;
  CltSnippet	tb_DAB;
} TexBlendData;

/*
 * The contents of the sprite structure really is private to the system
 * At some point, the structure will be hidden in an internal header file,
 * and the contents will not be exposed to the public, except for the
 * initial node field.
 */
typedef struct SpriteObj {
  Node		spr_Node;
  uint32	spr_Flags;
  TexBlendData	*spr_TexBlend;
  void	        *spr_Cache;  /* PRIVATE */
  int32		spr_Width, spr_Height;
  Point2	spr_Position;
  Point2	spr_Corners[4];
  int32		spr_HSlice;
  int32		spr_VSlice;
} SpriteObj;

typedef struct ExtSpriteObj {
  Node		spr_Node;
  uint32	spr_Flags;
  TexBlendData	*spr_TexBlend;
  void	        *spr_Cache;  /* PRIVATE */
  int32		spr_Width, spr_Height;
  Point2	spr_Position;
  Point2	spr_Corners[4];
  int32		spr_HSlice;
  int32		spr_VSlice;
  Color4	spr_Colors[4];
  gfloat	spr_ZValues[4];
} ExtSpriteObj;

typedef struct ShortSpriteObj {
  Node		spr_Node;
  uint32	spr_Flags;
  TexBlendData	*spr_TexBlend;
  void	        *spr_Cache;  /* PRIVATE */
} ShortSpriteObj;


/* define sprite flags */
#define SPR_SKIP		0x80000000
#define SPR_GEOMETRYENABLE	0x40000000
#define SPR_CALC_VERTICES       0x20000000
#define SPR_DONT_CLIP           0x10000000
#define SPR_CALC_TXLOAD         0x08000000
#define SPR_SYSTEMPIPDATA	0x00000010
#define SPR_SYSTEMTEXTUREDATA	0x00000008
#define SPR_OVERSIZETEXTURE	0x00000004
#define SPR_SYSTEMPIP		0x00000002
#define SPR_SYSTEMTXDATA	0x00000001


SpriteObj* Spr_Create (TagArg *t);
SpriteObj* Spr_CreateExtended (TagArg *t);
SpriteObj* Spr_CreateShort (TagArg *t);
Err Spr_Delete (SpriteObj *sp);

#define SPR_TAG_NAME	1
#define SPR_TAG_PRIORITY	2

#define SPR_TAG_TEXTUREDATA (TAG_ITEM_LAST+1)
#define SPR_TAG_PIP	(TAG_ITEM_LAST+2)
#define SPR_TAG_WIDTH	(TAG_ITEM_LAST+3)
#define SPR_TAG_HEIGHT	(TAG_ITEM_LAST+4)
#define SPR_TAG_XPOS	(TAG_ITEM_LAST+5)
#define SPR_TAG_YPOS	(TAG_ITEM_LAST+6)
#define SPR_TAG_HSLICE	(TAG_ITEM_LAST+7)
#define SPR_TAG_VSLICE	(TAG_ITEM_LAST+8)
#define SPR_TAG_SKIP	(TAG_ITEM_LAST+9)

Err Spr_SetWidth (SpriteObj *sp, const uint32 Width);
Err Spr_SetHeight (SpriteObj *sp, const uint32 Height);
Err Spr_SetPosition (SpriteObj *sp, const Point2 * position);
Err Spr_SetCorners (SpriteObj *sp, const Point2 *Corners);
Err Spr_SetHSlice (SpriteObj *sp, const uint32 hslice);
Err Spr_SetVSlice (SpriteObj *sp, const uint32 vslice);
Err Spr_SetSkip (SpriteObj *sp, const uint32 skip);
Err Spr_SetGeometryEnable (SpriteObj *sp, const uint32 value);
Err Spr_SetColors (SpriteObj *sp, const Color4 *c);
Err Spr_SetZValues (SpriteObj *sp, const gfloat *zv);

Err Spr_SetTextureAttr (SpriteObj *sp,
			const CltTxAttribute attr, const uint32 value);
Err Spr_SetDBlendAttr (SpriteObj *sp,
		       const CltDblAttribute attr, const uint32 value);
Err Spr_SetPIP (SpriteObj *sp, const CltPipControlBlock *p);
Err Spr_SetTextureData (SpriteObj *sp, const CltTxData *t);

Err Spr_RemoveTextureAttr (SpriteObj *sp, const CltTxAttribute attr);
Err Spr_RemoveDBlendAttr (SpriteObj *sp, const CltDblAttribute attr);
Err Spr_RemovePIP (SpriteObj *sp);
Err Spr_RemoveTextureData (SpriteObj *sp);



uint32 Spr_GetWidth (const SpriteObj *sp);
uint32 Spr_GetHeight (const SpriteObj *sp);
Err Spr_GetPosition (const SpriteObj *sp, Point2 *position);
Err Spr_GetCorners (const SpriteObj *sp, Point2 *Corners);
uint32 Spr_GetHSlice (const SpriteObj *sp);
uint32 Spr_GetVSlice (const SpriteObj *sp);
uint32 Spr_GetSkip (const SpriteObj *sp);
uint32 Spr_GetGeometryEnable (const SpriteObj *sp);
Err Spr_GetColors (const SpriteObj *sp, Color4 *c);
Err Spr_GetZValues (const SpriteObj *sp, gfloat *zv);

Err Spr_GetTextureAttr (const SpriteObj *sp,
			const CltTxAttribute attr, uint32 *value);
Err Spr_GetDBlendAttr (const SpriteObj *sp,
		       const CltDblAttribute attr, uint32 *value);
CltPipControlBlock* Spr_GetPIP (const SpriteObj *sp);
CltTxData* Spr_GetTextureData (const SpriteObj *sp);

Err Spr_DisableClipping(void* f2obj);
Err Spr_EnableClipping(void* f2obj);

Err Spr_Translate (SpriteObj *sp, const gfloat x, const gfloat y);
Err Spr_Rotate (SpriteObj *sp, const gfloat angle);
Err Spr_Scale (SpriteObj *sp, const gfloat xscale, const gfloat yscale);
Err Spr_ResetCorners (SpriteObj *sp, const uint32 where);
Err Spr_MapCorners (SpriteObj *sp, const Point2 *Corners, const uint32 where);

#define SPR_TOPLEFT	0
#define SPR_TOPRIGHT	1
#define SPR_BOTTOMLEFT	2
#define SPR_BOTTOMRIGHT	3
#define SPR_CENTER	4


CltTxData* CltTxData_Create (const void *texeldata,
			     const uint32 width, const uint32 height,
			     const uint32 bpp, const uint32 expformat);

Err CltTxData_Delete (CltTxData* t);

CltPipControlBlock*
CltPipControlBlock_Create (const void *pipdata,
			   const uint32 pipindex, const uint32 numentries);

Err CltPipControlBlock_Delete (CltPipControlBlock *pcb);

Err Spr_CreateTxData (SpriteObj *sp, const void *texeldata,
		      const uint32 width, const uint32 height,
		      const uint32 bpp, const uint32 expformat);

Err Spr_CreatePipControlBlock (SpriteObj *sp, const void *pipdata,
			       const uint32 pipindex, const uint32 numentries);

Err Spr_LoadUTF (SpriteObj *sp, const char *fname);



/* expansion format definition macros */
#define MAKEEXPFORMAT(lit,hasalpha,hascolor,hasssb,xparent,adepth,cdepth) \
	CLA_TXTEXPTYPE(cdepth, adepth,xparent, hasssb, hascolor, hasalpha, lit)

#define EXP_LITERAL		CLT_Mask(TXTEXPTYPE,ISLITERAL)
#define EXP_HASALPHA	CLT_Mask(TXTEXPTYPE,HASALPHA)
#define EXP_HASCOLOR	CLT_Mask(TXTEXPTYPE,HASCOLOR)
#define EXP_HASSSB		CLT_Mask(TXTEXPTYPE,HASSSB)
#define EXP_TRANSPARENT	CLT_Mask(TXTEXPTYPE,ISTRANS)
#define EXP_ADEPTHMASK	CLT_Mask(TXTEXPTYPE,ADEPTH)
#define EXP_CDEPTHMASK	CLT_Mask(TXTEXPTYPE,CDEPTH)

#define EXP_ADEPTHSHIFT	CLT_Shift(TXTEXPTYPE,ADEPTH)
#define EXP_CDEPTHSHIFT	CLT_Shift(TXTEXPTYPE,CDEPTH)

#endif /* __GRAPHICS_FRAME2D_SPRITEOBJ_H */
