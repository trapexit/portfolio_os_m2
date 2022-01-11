/*
 *	@(#) spriteobj.c 96/07/09 1.35
 *
 * 2D Framework for M2 graphics
 *
 */

/*
 * Sprite object creation and accessor functions
 */

/* #define DBUG(x) printf x; */
#define DBUG(x)

#include <kernel/types.h>
#include <kernel/tags.h>
#include <math.h>
#include "frame2.i"



SpriteObj*
_spr_create_core (TagArg *t, SpriteObj *sp, int32 type, int32 size)
{
  TagArg *t1, *t2;
  char *name=0;
  uint32 width=0, height=0;

  /* Initialize contents of sprite object to zero */
  bzero (sp, sizeof(SpriteObj));

  /* Set up node fields in sprite object */
  sp->spr_Node.n_SubsysType = NODE_2D;
  sp->spr_Node.n_Type = type;
  sp->spr_Node.n_Size = size;

  /* set slice factors to 1 */
  if (type != SHORTSPRITEOBJNODE) {
    sp->spr_HSlice = 1;
    sp->spr_VSlice = 1;
  }

  t1 = t;
  while ((t2 = NextTagArg(&t1)) != NULL) {
    switch (t2->ta_Tag) {
    case SPR_TAG_NAME:
      FreeMem(name, TRACKED_SIZE);
      name = AllocMem( strlen((char*)(t2->ta_Arg)) + 1, MEMTYPE_NORMAL | MEMTYPE_TRACKSIZE );
      if (!name) goto error;
      strcpy (name, (char*)(t->ta_Arg));
      sp->spr_Node.n_Name = name;
      break;
    case SPR_TAG_PRIORITY:
      sp->spr_Node.n_Priority = (uint8)(t2->ta_Arg);
      break;
    case SPR_TAG_TEXTUREDATA:
      if (Spr_SetTextureData(sp,(CltTxData*)(t2->ta_Arg)) < 0) goto error;
      break;
    case SPR_TAG_PIP:
      if (Spr_SetPIP(sp,(CltPipControlBlock*)(t2->ta_Arg)) < 0) goto error;
      break;
    case SPR_TAG_WIDTH:
      width = (uint32)(t2->ta_Arg);
      break;
    case SPR_TAG_HEIGHT:
      height = (uint32)(t2->ta_Arg);
      break;
    case SPR_TAG_XPOS:
      if (type==SHORTSPRITEOBJNODE) goto error;
      sp->spr_Position.x = *(gfloat*)(&t2->ta_Arg);
      break;
    case SPR_TAG_YPOS:
      if (type==SHORTSPRITEOBJNODE) goto error;
      sp->spr_Position.y = *(gfloat*)(&t2->ta_Arg);
      break;
    case SPR_TAG_HSLICE:
      if (type==SHORTSPRITEOBJNODE) goto error;
      sp->spr_HSlice = (uint32)(t2->ta_Arg);
      break;
    case SPR_TAG_VSLICE:
      if (type==SHORTSPRITEOBJNODE) goto error;
      sp->spr_VSlice = (uint32)(t2->ta_Arg);
      break;
    case SPR_TAG_SKIP:
      sp->spr_Flags |= SPR_SKIP;
      break;
    default:
      goto error;
    }
  }

  if (type != SHORTSPRITEOBJNODE) {
    if (width) sp->spr_Width = width;
    if (height) sp->spr_Height = height;

    if (Spr_ResetCorners (sp, SPR_TOPLEFT) < 0) goto error;
    sp->spr_Flags |= SPR_GEOMETRYENABLE;
  }

  return sp;

 error:
  FreeMem(name, TRACKED_SIZE);
  FreeMem(sp, sizeof(SpriteObj));
  return 0;
}


/**
|||	AUTODOC -class frame2d -name Spr_Create
|||	Create a sprite object.
|||
|||	  Synopsis
|||
|||	    SpriteObj *Spr_Create (TagArg *t);
|||
|||	  Description
|||
|||	    This routine creates a SpriteObj structure, and fills in any
|||	    parts that are requested by the TagArg list.
|||
|||	    If the TagArg list is empty, or the pointer is null, then an
|||	    "empty" sprite object is created.
|||
|||	  Arguments
|||
|||	    t
|||	        Pointer to TagArg list
|||
|||
|||	  Return Value
|||
|||	    This routine will return a pointer to the SpriteObj structure
|||	    created, or 0 on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    Spr_Delete(), Spr_SetWidth(), Spr_SetHeight(), Spr_SetPosition(),
|||	    Spr_SetCorners(), Spr_SetHSlice(), Spr_SetVSlice(), Spr_SetSkip(),
|||	    Spr_SetGeometryEnable(), Spr_SetColors(), Spr_SetZValues(),
|||	    Spr_SetTextureAttr(), Spr_SetDBlendAttr(), Spr_SetPIP(),
|||	    Spr_SetTextureData(), Spr_RemoveTextureAttr(),
|||	    Spr_RemoveDBlendAttr(), Spr_RemovePIP(), Spr_RemoveTextureData(),
|||	    Spr_GetWidth(), Spr_GetHeight(), Spr_GetPosition(),
|||	    Spr_GetCorners(), Spr_GetHSlice(), Spr_GetVSLice(), Spr_GetSkip(),
|||	    Spr_GetGeometryEnable(), Spr_GetColors(), Spr_GetZValues(),
|||	    Spr_GetTextureAttr(), Spr_GetDBlendAttr(), Spr_GetPIP(),
|||	    Spr_GetTextureData(), Spr_CreateExtended, Spr_CreateShort()
|||
|||
**/
SpriteObj *
Spr_Create (TagArg *t)
{
  SpriteObj *sp;

  /* Allocate memory for sprite object */
  sp = AllocMem(sizeof(SpriteObj), MEMTYPE_NORMAL | MEMTYPE_TRACKSIZE);
  if (!sp) goto error;

  /* Call core setup routine */
  return _spr_create_core(t, sp, SPRITEOBJNODE, sizeof(SpriteObj));

error:
  return 0;
}


/**
|||	AUTODOC -class frame2d -name Spr_CreateExtended
|||	Create a an extended sprite object.
|||
|||	  Synopsis
|||
|||	    SpriteObj *Spr_CreateExtended (TagArg *t);
|||
|||	  Description
|||
|||	    This routine creates an extended SpriteObj structure, and fills
|||	    in any parts that are requested by the TagArg list.
|||
|||	    If the TagArg list is empty, or the pointer is null, then an
|||	    "empty" sprite object is created.
|||
|||	    Even though an extended sprite object is created, the return
|||	    type of the function is a pointer to sprite object.  Extended
|||	    sprite objects and short sprite objects can be referred to as
|||	    sprite objects so that they can be generally passed to the
|||	    same routines that manipulate all sprite objects without having
|||	    to be explicitly cast.
|||
|||	  Arguments
|||
|||	    t
|||	        Pointer to TagArg list
|||
|||
|||	  Return Value
|||
|||	    This routine will return a pointer to the SpriteObj structure
|||	    created, or 0 on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    Spr_Create(), Spr_CreateShort(), Spr_Delete()
|||
|||
**/
SpriteObj *
Spr_CreateExtended (TagArg *t)
{
  ExtSpriteObj *sp;

  /* Allocate memory for sprite object */
  sp = AllocMem(sizeof(ExtSpriteObj), MEMTYPE_NORMAL | MEMTYPE_TRACKSIZE);
  if (!sp) goto error;

  /* Call core setup routine */
  return _spr_create_core (t, (SpriteObj*)sp,
			   EXTSPRITEOBJNODE, sizeof(ExtSpriteObj));

error:
  return 0;
}


/**
|||	AUTODOC -class frame2d -name Spr_CreateShort
|||	Create a short sprite object.
|||
|||	  Synopsis
|||
|||	    SpriteObj *Spr_CreateShort (TagArg *t);
|||
|||	  Description
|||
|||	    This routine creates an short SpriteObj structure, and fills
|||	    in any parts that are requested by the TagArg list.
|||
|||	    If the TagArg list is empty, or the pointer is null, then an
|||	    "empty" sprite object is created.
|||
|||	    Even though an short sprite object is created, the return
|||	    type of the function is a pointer to sprite object.  Extended
|||	    sprite objects and short sprite objects can be referred to as
|||	    sprite objects so that they can be generally passed to the
|||	    same routines that manipulate all sprite objects without having
|||	    to be explicitly cast.
|||
|||	  Arguments
|||
|||	    t
|||	        Pointer to TagArg list
|||
|||
|||	  Return Value
|||
|||	    This routine will return a pointer to the SpriteObj structure
|||	    created, or 0 on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    Spr_Create(), Spr_CreateExtended(), Spr_Delete()
|||
|||
**/
SpriteObj *
Spr_CreateShort (TagArg *t)
{
  ShortSpriteObj *sp;

  /* Allocate memory for sprite object */
  sp = AllocMem(sizeof(ShortSpriteObj), MEMTYPE_NORMAL | MEMTYPE_TRACKSIZE);
  if (!sp) goto error;

  /* Call core setup routine */
  return _spr_create_core (t, (SpriteObj*)sp,
			   SHORTSPRITEOBJNODE, sizeof(ShortSpriteObj));

error:
  return 0;
}


/*
 * Utility cleanup functions for sprite objects
 */
static void
Spr_CltTxData_Delete (SpriteObj *sp)
{
  if (sp->spr_Flags&SPR_SYSTEMTEXTUREDATA) {
    if (sp->spr_TexBlend) {
      if (sp->spr_TexBlend->tb_txData) {
	if (sp->spr_TexBlend->tb_txData->texelData) {
	  FreeMem(sp->spr_TexBlend->tb_txData->texelData->texelData, TRACKED_SIZE);
	}
      }
      sp->spr_Flags &= ~SPR_SYSTEMTEXTUREDATA;
    }
  }
  CltTxData_Delete (sp->spr_TexBlend->tb_txData);
  sp->spr_Flags |= SPR_CALC_TXLOAD;
}

static void
Spr_CltPipControlBlock_Delete (SpriteObj *sp)
{
  if (sp->spr_Flags&SPR_SYSTEMPIPDATA) {
    if (sp->spr_TexBlend) {
      if (sp->spr_TexBlend->tb_PCB) {
	FreeMem(sp->spr_TexBlend->tb_PCB->pipData, TRACKED_SIZE);
      }
    }
    sp->spr_Flags &= ~SPR_SYSTEMPIPDATA;
  }
  CltPipControlBlock_Delete (sp->spr_TexBlend->tb_PCB);
}


/**
|||	AUTODOC -class frame2d -name Spr_Delete
|||	Delete a sprite object.
|||
|||	  Synopsis
|||
|||	    Err Spr_Delete (SpriteObj* sp);
|||
|||	  Description
|||
|||	    Delete a SpriteObj structure, and also free up any elements
|||	    that were created as part of the sprite object.
|||
|||	  Arguments
|||
|||	    sp
|||	        Pointer to sprite object to delete
|||
|||
|||	  Return Value
|||
|||	    This routine will return 0 on success or a negative error code
|||	    on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    Spr_Create()
|||
**/
Err
Spr_Delete (SpriteObj* sp)
{
  if (sp) {
    DBUG (("Free the name node\n"));
    FreeMem(sp->spr_Node.n_Name, TRACKED_SIZE);
    if (sp->spr_TexBlend) {
      if (sp->spr_Flags&SPR_SYSTEMTXDATA) {
	DBUG (("Remove the CltTxData\N"));
	Spr_CltTxData_Delete (sp);
      };
      if (sp->spr_Flags&SPR_SYSTEMPIP) {
	DBUG (("Remove the PipControlBlock\n"));
	Spr_CltPipControlBlock_Delete (sp);
      }
      DBUG (("Remove the TAB @ %08lx\n", sp->spr_TexBlend->tb_TAB));
      /* Tx_FreeAttributeList (sp->spr_TexBlend->tb_TAB); */
      CLT_FreeSnippet (&sp->spr_TexBlend->tb_TAB);
      DBUG (("Remove the DAB @ %08lx\n", sp->spr_TexBlend->tb_DAB));
      /* Dbl_FreeAttributeList (sp->spr_TexBlend->tb_DAB); */
      CLT_FreeSnippet (&sp->spr_TexBlend->tb_DAB);
      DBUG (("Remove the TexBlend\n"));
      FreeMem(sp->spr_TexBlend, TRACKED_SIZE);
    }
    if (sp->spr_Cache) {
        SpriteCache *spc = (SpriteCache *)sp->spr_Cache;
        /* SpriteCache *clone; */
        SpriteCache *next;
	do {
	    if (spc->spc_TxLoad) {
	        FreeMem(spc->spc_TxLoad->tlc_TxLoad, (spc->spc_TxLoad->tlc_TxLoadSize * 4));
		FreeMem(spc->spc_TxLoad, sizeof(TxLoadCache));
	    }
	    next = (SpriteCache *)spc->spc_Node.n_Next;
	    FreeMem(spc->spc_Vertices, spc->spc_ByteCount);
	    FreeMem(spc, sizeof(SpriteCache));
	} while (spc = next);
    }
    DBUG (("Remove the sprite\n"));
    FreeMem(sp, TRACKED_SIZE);
  }
  return 0;
}


/**
|||	AUTODOC -class frame2d -name Spr_SetTextureData
|||	Attach texture data to a sprite object.
|||
|||	  Synopsis
|||
|||	    Err Spr_SetTextureData (SpriteObj *sp, CltTxData *t);
|||
|||	  Description
|||
|||	    Attach texture data described by the CltTxData structure to the
|||	    sprite object.  If the sprite object already has texture data
|||	    that was created with the Spr_CreateTxData() routine, the old
|||	    CltTxData structure will be deleted.
|||
|||	    Note, as of V30, changes that you make directly to the
|||	    CltTxData structure that has been attached to the sprite
|||	    will no longer be picked up by F2_Draw(). You must call
|||	    Spr_SetTextureData() again with the modified CltTxData structure.
|||
|||	  Arguments
|||
|||	    sp
|||	        Pointer to the sprite object to modify
|||
|||	    t
|||	        Pointer to the CltTxData to attach to the sprite object
|||
|||
|||	  Return Value
|||
|||	    This routine will return 0 on success or a negative error code
|||	    on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    Spr_CreateTxData(), Spr_RemoveTextureData(),
|||	    Spr_GetTextureData()
|||
**/
Err
Spr_SetTextureData (SpriteObj *sp, const CltTxData *t)
{
  if (sp->spr_TexBlend) {
    if (sp->spr_Flags&SPR_SYSTEMTXDATA) {
      Spr_CltTxData_Delete (sp);
    }
  }
  sp->spr_Flags &= ~(SPR_SYSTEMTXDATA | SPR_CALC_TXLOAD);
  if (t) {
    if (!sp->spr_TexBlend) {
      if (!(sp->spr_TexBlend = AllocMem(sizeof(TexBlendData), MEMTYPE_NORMAL | MEMTYPE_TRACKSIZE))) goto error;
      bzero (sp->spr_TexBlend, sizeof(TexBlendData));
    }
    sp->spr_TexBlend->tb_txData = (CltTxData*)t;
    sp->spr_Flags |= SPR_CALC_TXLOAD;
  } else {
    if (sp->spr_TexBlend) {
      sp->spr_TexBlend->tb_txData = (CltTxData*)t;
    }
  }
  if (t && (sp->spr_Node.n_Type!=SHORTSPRITEOBJNODE)) {
    sp->spr_Width = t->minX<<(t->maxLOD-1);
    sp->spr_Height = t->minY<<(t->maxLOD-1);
  }

  return 0;

 error: return -1;
}



/**
|||	AUTODOC -class frame2d -name Spr_GetTextureData
|||	Attach texture data to a sprite object.
|||
|||	  Synopsis
|||
|||	    CltTxData *Spr_GetTextureData (SpriteObj *sp);
|||
|||	  Description
|||
|||	    Get a pointer to the texture data description attached to a sprite.
|||
|||	  Arguments
|||
|||	    sp
|||	        Pointer to the sprite object
|||
|||	  Return Value
|||
|||	    This routine will return a pointer to the CltTxData structure
|||	    attached to the sprite object, or 0 if there is none.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    Spr_CreateTxData(), Spr_RemoveTextureData(),
|||	    Spr_SetTextureData()
|||
**/
CltTxData *
Spr_GetTextureData (const SpriteObj *sp)
{
  return sp->spr_TexBlend ? sp->spr_TexBlend->tb_txData : 0;
}


/**
|||	AUTODOC -class frame2d -name Spr_SetWidth
|||	Set the default width of the sprite object.
|||
|||	  Synopsis
|||
|||	    Err Spr_SetWidth (SpriteObj *sp, uint32 width);
|||
|||	  Description
|||
|||	    Set the default width of the sprite object to the specified value.
|||	    The default width of a sprite object is set to the texel width
|||	    of a texture any time a texture is attached to a sprite object.
|||
|||	  Arguments
|||
|||	    sp
|||	        Pointer to the sprite object to modify
|||
|||	    width
|||	        New default width for a sprite object
|||
|||
|||	  Return Value
|||
|||	    This routine will return 0 on success or a negative error code
|||	    on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    Spr_GetWidth()
|||
**/
Err
Spr_SetWidth (SpriteObj *sp, const uint32 w)
{
  if (sp->spr_Node.n_Type==SHORTSPRITEOBJNODE)
    return -1;

  sp->spr_Width = w;

  return 0;
}


/**
|||	AUTODOC -class frame2d -name Spr_GetWidth
|||	Query the default width of the sprite object.
|||
|||	  Synopsis
|||
|||	    uint32 Spr_GetWidth (const SpriteObj *sp);
|||
|||	  Description
|||
|||	    Query the current default width of a sprite object.
|||
|||	  Arguments
|||
|||	    sp
|||	        Pointer to the sprite object to query
|||
|||
|||	  Return Value
|||
|||	    This routine will return the width of the sprite object.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    Spr_SetWidth()
|||
**/
uint32
Spr_GetWidth (const SpriteObj *sp)
{
  if (sp->spr_Node.n_Type==SHORTSPRITEOBJNODE) {
    if (sp->spr_TexBlend) {
      if (sp->spr_TexBlend->tb_txData) {
	return sp->spr_TexBlend->tb_txData->minX;
      }
    }
    return 0;
  }

  return sp->spr_Width;
}


/**
|||	AUTODOC -class frame2d -name Spr_SetHeight
|||	Set the default height of the sprite object.
|||
|||	  Synopsis
|||
|||	    Err Spr_SetHeight (SpriteObj *sp, uint32 height);
|||
|||	  Description
|||
|||	    Set the default height of the sprite object to the specified value.
|||	    The default height of a sprite object is set to the texel height
|||	    of a texture any time a texture is attached to a sprite object.
|||
|||	  Arguments
|||
|||	    sp
|||	        Pointer to the sprite object to modify
|||
|||	    height
|||	        New default height for a sprite object
|||
|||
|||	  Return Value
|||
|||	    This routine will return 0 on success or a negative error code
|||	    on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    Spr_GetHeight()
|||
**/
Err
Spr_SetHeight (SpriteObj *sp, const uint32 h)
{
  if (sp->spr_Node.n_Type==SHORTSPRITEOBJNODE)
    return -1;

  sp->spr_Height = h;

  return 0;
}


/**
|||	AUTODOC -class frame2d -name Spr_GetHeight
|||	Query the default height of the sprite object.
|||
|||	  Synopsis
|||
|||	    uint32 Spr_GetHeight (const SpriteObj *sp);
|||
|||	  Description
|||
|||	    Query the current default height of a sprite object.
|||
|||	  Arguments
|||
|||	    sp
|||	        Pointer to the sprite object to query
|||
|||
|||	  Return Value
|||
|||	    This routine will return the height of the sprite object.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    Spr_SetHeight()
|||
**/
uint32
Spr_GetHeight (const SpriteObj *sp)
{
  if (sp->spr_Node.n_Type==SHORTSPRITEOBJNODE) {
    if (sp->spr_TexBlend) {
      if (sp->spr_TexBlend->tb_txData) {
	return sp->spr_TexBlend->tb_txData->minY;
      }
    }
    return 0;
  }

  return sp->spr_Height;
}



/**
|||	AUTODOC -class frame2d -name Spr_SetSkip
|||	Set the state of the skip flag of the sprite object.
|||
|||	  Synopsis
|||
|||	    Err Spr_SetSkip (SpriteObj *sp, uint32 skip);
|||
|||	  Description
|||
|||	    Set the skip flag of the sprite object to the specified value.
|||	    If the skip flag is set to 1, then the sprite object will not
|||	    be processed by draw commands.  The skip flag defaults to 0.
|||
|||	  Arguments
|||
|||	    sp
|||	        Pointer to the sprite object to modify
|||
|||	    skip
|||	        Value to set the skip flag to
|||
|||
|||	  Return Value
|||
|||	    This routine will return 0 on success or a negative error code
|||	    on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    Spr_GetSkip()
|||
**/
Err
Spr_SetSkip (SpriteObj *sp, const uint32 value)
{
  if (value) {
    sp->spr_Flags |= SPR_SKIP;
  } else {
    sp->spr_Flags &= ~SPR_SKIP;
  }

  return 0;
}


/**
|||	AUTODOC -class frame2d -name Spr_GetSkip
|||	Query the skip flag of the sprite object.
|||
|||	  Synopsis
|||
|||	    uint32 Spr_GetSkip (const SpriteObj *sp);
|||
|||	  Description
|||
|||	    Query the skip flag of a sprite object.
|||
|||	  Arguments
|||
|||	    sp
|||	        Pointer to the sprite object to query
|||
|||
|||	  Return Value
|||
|||	    This routine will return the skip of the sprite object.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    Spr_SetSkip()
|||
**/
uint32
Spr_GetSkip (const SpriteObj *sp)
{
  return (sp->spr_Flags&SPR_SKIP)!=0;
}


/**
|||	AUTODOC -class frame2d -name Spr_SetGeometryEnable
|||	Enable/disable the geometry on a sprite object.
|||
|||	  Synopsis
|||
|||	    Err Spr_SetGeometryEnable (SpriteObj *sp, uint32 value);
|||
|||	  Description
|||
|||	    Enable/disable geometry on a sprite object.  If the geometry
|||	    on a sprite object is disabled, then nothing is drawn when
|||	    the sprite object is passed to the draw functions.  Any
|||	    attribute settings of the sprite object will still be sent
|||	    to the triangle engine.
|||
|||	    Sprite objects default to having geometry enabled.
|||
|||	  Arguments
|||
|||	    sp
|||	        Pointer to the sprite object to modify
|||
|||	    value
|||	        0 to disable geomtry, 1 to enable geomtry
|||
|||
|||	  Return Value
|||
|||	    This routine will return 0 on success or a negative error code
|||	    on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    Spr_GetGeometryEnable()
|||
**/
Err
Spr_SetGeometryEnable (SpriteObj *sp, const uint32 value)
{
  if (sp->spr_Node.n_Type==SHORTSPRITEOBJNODE)
    return -1;

  if (value) {
    sp->spr_Flags |= SPR_GEOMETRYENABLE;
  } else {
    sp->spr_Flags &= ~SPR_GEOMETRYENABLE;
  }

  return 0;
}


/**
|||	AUTODOC -class frame2d -name Spr_GetGeometryEnable
|||	Query the geometry enable state of the sprite object.
|||
|||	  Synopsis
|||
|||	    uint32 Spr_GetGeometryEnable (const SpriteObj *sp);
|||
|||	  Description
|||
|||	    Query the geometry enable flag of a sprite object.
|||
|||	  Arguments
|||
|||	    sp
|||	        Pointer to the sprite object to modify
|||
|||
|||	  Return Value
|||
|||	    This routine will return 0 if geometry is disabled or 1 if geometry
|||	    is enabled in the sprite object.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    Spr_SetGeometryEnable()
|||
**/
uint32
Spr_GetGeometryEnable (const SpriteObj *sp)
{
  return (sp->spr_Flags&SPR_GEOMETRYENABLE)!=0;
}


/**
|||	AUTODOC -class frame2d -name Spr_ResetCorners
|||	Reset the corners of a sprite object
|||
|||	  Synopsis
|||
|||	    Err Spr_ResetCorners (SpriteObj *sp, uint32 where);
|||
|||	  Description
|||
|||	    Reset the corners of a sprite object so that the sprite is
|||	    oriented in an upright position and is scaled 1:1.
|||
|||	    The corners are placed so that the position of the sprite
|||	    object is at one of the four corners, or is at the center
|||	    of the sprite object, specified by the argument where.
|||
|||	  Arguments
|||
|||	    sp
|||	        Pointer to the sprite object to modify
|||
|||	    where
|||	        Specifies which of the corners are to be the anchor
|||	        of the sprite object.  Must be one of:  SPR_TOPLEFT,
|||	        SPR_TOPRIGHT, SPR_BOTTOMLEFT, SPR_BOTTOMRIGHT, SPR_CENTER
|||
|||
|||	  Return Value
|||
|||	    This routine will return 0 on success or a negative error
|||	    code on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    Spr_GetCorners(), Spr_MapCorners(), Spr_SetCorners()
|||
**/
Err
Spr_ResetCorners (SpriteObj *sp, const uint32 where)
{
  if (sp->spr_Node.n_Type==SHORTSPRITEOBJNODE)
    return -1;

  sp->spr_Flags |= SPR_CALC_VERTICES;

  switch (where) {
  case SPR_TOPLEFT:
    sp->spr_Corners[0].x = 0;
    sp->spr_Corners[0].y = 0;
    sp->spr_Corners[1].x = sp->spr_Width;
    sp->spr_Corners[1].y = 0;
    sp->spr_Corners[2].x = sp->spr_Width;
    sp->spr_Corners[2].y = sp->spr_Height;
    sp->spr_Corners[3].x = 0;
    sp->spr_Corners[3].y = sp->spr_Height;
    break;
  case SPR_TOPRIGHT:
    sp->spr_Corners[0].x = -sp->spr_Width;
    sp->spr_Corners[0].y = 0;
    sp->spr_Corners[1].x = 0;
    sp->spr_Corners[1].y = 0;
    sp->spr_Corners[2].x = 0;
    sp->spr_Corners[2].y = sp->spr_Height;
    sp->spr_Corners[3].x = -sp->spr_Width;
    sp->spr_Corners[3].y = sp->spr_Height;
    break;
  case SPR_BOTTOMLEFT:
    sp->spr_Corners[0].x = 0;
    sp->spr_Corners[0].y = -sp->spr_Height;
    sp->spr_Corners[1].x = sp->spr_Width;
    sp->spr_Corners[1].y = -sp->spr_Height;
    sp->spr_Corners[2].x = sp->spr_Width;
    sp->spr_Corners[2].y = 0;
    sp->spr_Corners[3].x = 0;
    sp->spr_Corners[3].y = 0;
    break;
  case SPR_BOTTOMRIGHT:
    sp->spr_Corners[0].x = -sp->spr_Width;
    sp->spr_Corners[0].y = -sp->spr_Height;
    sp->spr_Corners[1].x = 0;
    sp->spr_Corners[1].y = -sp->spr_Height;
    sp->spr_Corners[2].x = 0;
    sp->spr_Corners[2].y = 0;
    sp->spr_Corners[3].x = -sp->spr_Width;
    sp->spr_Corners[3].y = 0;
  break;
  case SPR_CENTER:
    sp->spr_Corners[0].x = (gfloat)(-sp->spr_Width)/2.;
    sp->spr_Corners[0].y = (gfloat)(-sp->spr_Height)/2.;
    sp->spr_Corners[1].x = (gfloat)(sp->spr_Width)/2.;
    sp->spr_Corners[1].y = (gfloat)(-sp->spr_Height)/2.;
    sp->spr_Corners[2].x = (gfloat)(sp->spr_Width)/2.;
    sp->spr_Corners[2].y = (gfloat)(sp->spr_Height)/2.;
    sp->spr_Corners[3].x = (gfloat)(-sp->spr_Width)/2.;
    sp->spr_Corners[3].y = (gfloat)(sp->spr_Height)/2.;
    break;
  default:
    return -1;
  }

  return 0;
}


/**
|||	AUTODOC -class frame2d -name Spr_SetCorners
|||	Set the locations of the corners of a sprite object.
|||
|||	  Synopsis
|||
|||	    Err Spr_SetCorners (const SpriteObj* sp, Point2* c);
|||
|||	  Description
|||
|||	    Set the locations of the 4 corners of a sprite object.
|||	    The corners are relative pixel offsets from the position
|||	    of the sprite object.
|||
|||	  Arguments
|||
|||	    sp
|||	        Pointer to the sprite object to modify
|||
|||	    c
|||	        Pointer to a 4 element array of Point2 structures to
|||	        containing the new corner values
|||
|||
|||	  Return Value
|||
|||	    This routine will return 0 on success or a negative error
|||	    code on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    Spr_GetCorners(), Spr_MapCorners(), Spr_ResetCorners()
|||
**/
Err
Spr_SetCorners (SpriteObj* sp, const Point2* c)
{
  if (sp->spr_Node.n_Type==SHORTSPRITEOBJNODE)
    return -1;

  sp->spr_Flags |= SPR_CALC_VERTICES;

  sp->spr_Corners[0].x = c[0].x;
  sp->spr_Corners[0].y = c[0].y;
  sp->spr_Corners[1].x = c[1].x;
  sp->spr_Corners[1].y = c[1].y;
  sp->spr_Corners[2].x = c[2].x;
  sp->spr_Corners[2].y = c[2].y;
  sp->spr_Corners[3].x = c[3].x;
  sp->spr_Corners[3].y = c[3].y;

  return 0;
}


/**
|||	AUTODOC -class frame2d -name Spr_MapCorners
|||	Set the position and corners of a sprite object
|||
|||	  Synopsis
|||
|||	    Err Spr_MapCorners (SpriteObj *sp, Point2 *c, uint32 where);
|||
|||	  Description
|||
|||	    Map a sprite object so that when drawn on the display it will
|||	    map to the 4 corners passed in.
|||
|||	    The position of the sprite object is updated to be at one of
|||	    corners, or to be at the center of the 4 corners, and the
|||	    placement is specified by the argument where.
|||
|||	  Arguments
|||
|||	    sp
|||	        Pointer to the sprite object to modify
|||
|||	    c
|||	        Pointer to a 4 element array of Point2 structures to
|||	        containing the screen coordinates to map the sprite
|||	        object to.
|||
|||	    where
|||	        Specifies where the position of the sprite object is to
|||	        be set relative to the new corner settings.  Must be one
|||	        of:  SPR_TOPLEFT, SPR_TOPRIGHT, SPR_BOTTOMLEFT,
|||	        SPR_BOTTOMRIGHT, SPR_CENTER
|||
|||
|||	  Return Value
|||
|||	    This routine will return 0 on success or a negative error
|||	    code on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    Spr_GetCorners(), Spr_MapCorners(), Spr_ResetCorners()
|||
**/
Err
Spr_MapCorners (SpriteObj *sp, const Point2 *c, const uint32 where)
{
  if (sp->spr_Node.n_Type==SHORTSPRITEOBJNODE)
    return -1;

  sp->spr_Flags |= SPR_CALC_VERTICES;

  switch (where) {
  case SPR_TOPLEFT:
    Spr_SetPosition (sp, sp->spr_Corners+0);
    break;
  case SPR_TOPRIGHT:
    Spr_SetPosition (sp, sp->spr_Corners+1);
    break;
  case SPR_BOTTOMLEFT:
    Spr_SetPosition (sp, sp->spr_Corners+3);
    break;
  case SPR_BOTTOMRIGHT:
    Spr_SetPosition (sp, sp->spr_Corners+2);
    break;
  case SPR_CENTER:
    sp->spr_Position.x = (c[0].x+c[1].x+c[2].x+c[3].x)/4.;
    sp->spr_Position.y = (c[0].y+c[1].y+c[2].y+c[3].y)/4.;
    break;
  default:
    return -1;
  }

  sp->spr_Corners[0].x = c[0].x-sp->spr_Position.x;
  sp->spr_Corners[0].y = c[0].y-sp->spr_Position.y;
  sp->spr_Corners[1].x = c[1].x-sp->spr_Position.x;
  sp->spr_Corners[1].y = c[1].y-sp->spr_Position.y;
  sp->spr_Corners[2].x = c[2].x-sp->spr_Position.x;
  sp->spr_Corners[2].y = c[2].y-sp->spr_Position.y;
  sp->spr_Corners[3].x = c[3].x-sp->spr_Position.x;
  sp->spr_Corners[3].y = c[3].y-sp->spr_Position.y;

  return 0;
}


/**
|||	AUTODOC -class frame2d -name Spr_GetCorners
|||	Query the locations of the corners of a sprite object.
|||
|||	  Synopsis
|||
|||	    Err Spr_GetCorners (const SpriteObj* sp, Point2* c);
|||
|||	  Description
|||
|||	    Return the locations of the 4 corners of a sprite object.
|||	    The corners are relative pixel offsets from the position
|||	    of the sprite object.
|||
|||	  Arguments
|||
|||	    sp
|||	        Pointer to the sprite object to query
|||
|||	    c
|||	        Pointer to a 4 element array of Point2 structures to
|||	        place the result
|||
|||
|||	  Return Value
|||
|||	    If successful, this routine will return a positive value and
|||	    the corners of the sprite object will be placed in the array
|||	    pointed to by c.  A negative value will be returned on error,
|||	    and c will be unmodified.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    Spr_SetCorners(), Spr_MapCorners(), Spr_ResetCorners()
|||
**/
Err
Spr_GetCorners (const SpriteObj* sp, Point2* c)
{
  if (sp->spr_Node.n_Type==SHORTSPRITEOBJNODE)
    return -1;

  c[0].x = sp->spr_Corners[0].x;
  c[0].y = sp->spr_Corners[0].y;
  c[1].x = sp->spr_Corners[1].x;
  c[1].y = sp->spr_Corners[1].y;
  c[2].x = sp->spr_Corners[2].x;
  c[2].y = sp->spr_Corners[2].y;
  c[3].x = sp->spr_Corners[3].x;
  c[3].y = sp->spr_Corners[3].y;
  return 0;
}



/**
|||	AUTODOC -class frame2d -name Spr_SetPosition
|||	Set the position of the sprite object.
|||
|||	  Synopsis
|||
|||	    Err Spr_SetPostion (const SpriteObj* sp, Point2* p);
|||
|||	  Description
|||
|||	    Set the screen position of the sprite object
|||
|||	  Arguments
|||
|||	    sp
|||	        Pointer to the sprite object to modify
|||
|||	    p
|||	        Pointer to the structure containing the new position
|||
|||
|||	  Return Value
|||
|||	    This routine will return zero on success or a negative
|||	    error code on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    Spr_GetPosition()
|||
**/
Err
Spr_SetPosition (SpriteObj* sp, const Point2* p)
{
  if (sp->spr_Node.n_Type==SHORTSPRITEOBJNODE)
    return -1;

  sp->spr_Flags |= SPR_CALC_VERTICES;

  sp->spr_Position.x = p->x;
  sp->spr_Position.y = p->y;

  return 0;
}



/**
|||	AUTODOC -class frame2d -name Spr_GetPosition
|||	Obtain the position of the sprite object.
|||
|||	  Synopsis
|||
|||	    Err Spr_GetPosition (const SpriteObj* sp, Point2* p);
|||
|||	  Description
|||
|||	    Query the position of the sprite object
|||
|||	  Arguments
|||
|||	    sp
|||	        Pointer to the sprite object to query
|||
|||	    p
|||	        Pointer to the Point2 structure to place the result
|||
|||
|||	  Return Value
|||
|||	    If successful, this routine will return a positive value, and will
|||	    place the position of the sprite object in the structure pointed
|||	    to by p.  On error a negative error code will be returned and p
|||	    will be unmodified.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    Spr_SetPosition()
|||
**/
Err
Spr_GetPosition (const SpriteObj* sp, Point2* p)
{
  if (sp->spr_Node.n_Type==SHORTSPRITEOBJNODE)
    return -1;

  p->x = sp->spr_Position.x;
  p->y = sp->spr_Position.y;
  return 0;
}


/**
|||	AUTODOC -class frame2d -name Spr_SetHSlice
|||	Set the hslice value of the sprite object.
|||
|||	  Synopsis
|||
|||	    Err Spr_SetHSlice (SpriteObj *sp, uint32 hslice);
|||
|||	  Description
|||
|||	    Set the hslice value of the sprite object.  The hslice parameter
|||	    controls how many horizontal strips a sprite object is carved
|||	    into when it is rendered.
|||
|||	  Arguments
|||
|||	    sp
|||	        Pointer to the sprite object to modify
|||
|||	    hslice
|||	        New default hslice for a sprite object
|||
|||
|||	  Return Value
|||
|||	    This routine will return 0 on success or a negative error code
|||	    on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    Spr_GetHSlice()
|||
**/
Err
Spr_SetHSlice (SpriteObj* sp, const uint32 h)
{
  if (sp->spr_Node.n_Type==SHORTSPRITEOBJNODE)
    return -1;

  sp->spr_HSlice = h;

  return 0;
}


/**
|||	AUTODOC -class frame2d -name Spr_GetHSlice
|||	Query the hslice value of the sprite object.
|||
|||	  Synopsis
|||
|||	    uint32 Spr_GetHSlice (const SpriteObj *sp);
|||
|||	  Description
|||
|||	    Query the current hslice value of a sprite object.
|||
|||	  Arguments
|||
|||	    sp
|||	        Pointer to the sprite object to query
|||
|||
|||	  Return Value
|||
|||	    This routine will return the hslice value of the sprite object.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    Spr_SetHSlice()
|||
**/
uint32
Spr_GetHSlice (const SpriteObj* sp)
{
  if (sp->spr_Node.n_Type==SHORTSPRITEOBJNODE)
    return -1;

  return sp->spr_HSlice;
}


/**
|||	AUTODOC -class frame2d -name Spr_SetVSlice
|||	Set the vslice value of the sprite object.
|||
|||	  Synopsis
|||
|||	    Err Spr_SetVSlice (SpriteObj *sp, uint32 vslice);
|||
|||	  Description
|||
|||	    Set the vslice value of the sprite object.  The vslice parameter
|||	    controls how many vertical strips a sprite object is carved
|||	    into when it is rendered.
|||
|||	  Arguments
|||
|||	    sp
|||	        Pointer to the sprite object to modify
|||
|||	    vslice
|||	        New default vslice for a sprite object
|||
|||
|||	  Return Value
|||
|||	    This routine will return 0 on success or a negative error code
|||	    on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    Spr_GetVSlice()
|||
**/
Err
Spr_SetVSlice (SpriteObj* sp, const uint32 v)
{
  if (sp->spr_Node.n_Type==SHORTSPRITEOBJNODE)
    return -1;

  sp->spr_VSlice = v;

  return 0;
}


/**
|||	AUTODOC -class frame2d -name Spr_GetVSlice
|||	Query the vslice value of the sprite object.
|||
|||	  Synopsis
|||
|||	    uint32 Spr_GetVSlice (const SpriteObj *sp);
|||
|||	  Description
|||
|||	    Query the current vslice value of a sprite object.
|||
|||	  Arguments
|||
|||	    sp
|||	        Pointer to the sprite object to query
|||
|||
|||	  Return Value
|||
|||	    This routine will return the vslice value of the sprite object.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    Spr_SetVSlice()
|||
**/
uint32
Spr_GetVSlice (const SpriteObj* sp)
{
  if (sp->spr_Node.n_Type==SHORTSPRITEOBJNODE)
    return -1;

  return sp->spr_VSlice;
}


/**
|||	AUTODOC -class frame2d -name Spr_SetColors
|||	Set the colors at the 4 corners of an extended sprite object.
|||
|||	  Synopsis
|||
|||	    Err Spr_SetColors (SpriteObj *sp, Color4 *c);
|||
|||	  Description
|||
|||	    Set the colors at the 4 corners of the extended sprite
|||	    object.
|||
|||	  Arguments
|||
|||	    sp
|||	        Pointer to the sprite object to modify
|||
|||	    c
|||	        Array of colors to use
|||
|||
|||	  Return Value
|||
|||	    This routine will return 0 on success or a negative error code
|||	    on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    Spr_GetColors()
|||
**/
Err
Spr_SetColors (SpriteObj *sp, const Color4 *c)
{
  if (sp->spr_Node.n_Type!=EXTSPRITEOBJNODE)
    return -1;

  ((ExtSpriteObj*)sp)->spr_Colors[0] = c[0];
  ((ExtSpriteObj*)sp)->spr_Colors[1] = c[1];
  ((ExtSpriteObj*)sp)->spr_Colors[2] = c[2];
  ((ExtSpriteObj*)sp)->spr_Colors[3] = c[3];
  return 0;
}


/**
|||	AUTODOC -class frame2d -name Spr_GetColors
|||	Get the colors at the 4 corners of an extended sprite object.
|||
|||	  Synopsis
|||
|||	    Err Spr_GetColors (SpriteObj *sp, Color4 *c);
|||
|||	  Description
|||
|||	    Get the colors at the 4 corners of the extended sprite
|||	    object.
|||
|||	  Arguments
|||
|||	    sp
|||	        Pointer to the sprite object to modify
|||
|||	    c
|||	        Pointer to array to return the color values
|||
|||
|||	  Return Value
|||
|||	    This routine will return 0 on success or a negative error code
|||	    on failure.  If successful the array c will be filled in with
|||	    the color values from the corners of the extended sprite.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    Spr_SetColors()
|||
**/
Err
Spr_GetColors (const SpriteObj *sp, Color4 *c)
{
  if (sp->spr_Node.n_Type!=EXTSPRITEOBJNODE)
    return -1;

  c[0] = ((ExtSpriteObj*)sp)->spr_Colors[0];
  c[1] = ((ExtSpriteObj*)sp)->spr_Colors[1];
  c[2] = ((ExtSpriteObj*)sp)->spr_Colors[2];
  c[3] = ((ExtSpriteObj*)sp)->spr_Colors[3];

  return 0;
}


/**
|||	AUTODOC -class frame2d -name Spr_SetZValues
|||	Set the Z values at the 4 corners of an extended sprite object.
|||
|||	  Synopsis
|||
|||	    Err Spr_SetZValues (SpriteObj *sp, gfloat *zv);
|||
|||	  Description
|||
|||	    Set the Z values at the 4 corners of the extended sprite
|||	    object.
|||
|||	  Arguments
|||
|||	    sp
|||	        Pointer to the sprite object to modify
|||
|||	    zv
|||	        Array of Z values to use
|||
|||
|||	  Return Value
|||
|||	    This routine will return 0 on success or a negative error code
|||	    on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    Spr_GetZValues()
|||
**/
Err
Spr_SetZValues (SpriteObj *sp, const gfloat *zv)
{
  if (sp->spr_Node.n_Type!=EXTSPRITEOBJNODE)
    return -1;

  ((ExtSpriteObj*)sp)->spr_ZValues[0] = zv[0];
  ((ExtSpriteObj*)sp)->spr_ZValues[1] = zv[1];
  ((ExtSpriteObj*)sp)->spr_ZValues[2] = zv[2];
  ((ExtSpriteObj*)sp)->spr_ZValues[3] = zv[3];

  return 0;
}


/**
|||	AUTODOC -class frame2d -name Spr_GetZValues
|||	Get the Z values at the 4 corners of an extended sprite object.
|||
|||	  Synopsis
|||
|||	    Err Spr_GetZValues (SpriteObj *sp, gfloat *zv);
|||
|||	  Description
|||
|||	    Get the Z values at the 4 corners of the extended sprite
|||	    object.
|||
|||	  Arguments
|||
|||	    sp
|||	        Pointer to the sprite object to modify
|||
|||	    zv
|||	        Pointer to array to return the Z values
|||
|||
|||	  Return Value
|||
|||	    This routine will return 0 on success or a negative error code
|||	    on failure.  If successful the array zv will be filled in with
|||	    the Z values from the corners of the extended sprite.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    Spr_SetZValues()
|||
**/
Err
Spr_GetZValues (const SpriteObj *sp, gfloat *zv)
{
  if (sp->spr_Node.n_Type!=EXTSPRITEOBJNODE)
    return -1;

  zv[0] = ((ExtSpriteObj*)sp)->spr_ZValues[0];
  zv[1] = ((ExtSpriteObj*)sp)->spr_ZValues[1];
  zv[2] = ((ExtSpriteObj*)sp)->spr_ZValues[2];
  zv[3] = ((ExtSpriteObj*)sp)->spr_ZValues[3];

  return 0;
}



/**
|||	AUTODOC -class frame2d -name Spr_SetTextureAttr
|||	Set a texture attribute in the sprite object
|||
|||	  Synopsis
|||
|||	    Err Spr_SetTextureAttr (SpriteObj *sp, CltTxAttribute attr, uint32 value);
|||
|||	  Description
|||
|||	    Set a specified texture attribute for the sprite object.
|||
|||	    Unless overridden by a later object, the texture attribute
|||	    will stay in effect for following rendered objects.
|||
|||	    See the CLT documentation for description of the texture
|||	    attributes.
|||
|||	  Arguments
|||
|||	    sp
|||	        Pointer to the sprite object to modify
|||
|||	    attr
|||	        index of the attribute to set
|||
|||	    value
|||	        value to set the attribute to
|||
|||
|||	  Return Value
|||
|||	    This routine will return 0 on success or a negative error code
|||	    on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    Spr_SetTextureAttr(), Spr_SetDBlendAttr()
|||	    Spr_RemoveTextureAttr(), Spr_RemoveDBlendAttr()
|||	    Spr_GetTextureAttr(), Spr_GetDBlendAttr()
|||
**/
Err
Spr_SetTextureAttr (SpriteObj *sp,
		    const CltTxAttribute attr, const uint32 value)
{
  if (!sp->spr_TexBlend) {
    if (!(sp->spr_TexBlend = AllocMem(sizeof(TexBlendData), MEMTYPE_NORMAL | MEMTYPE_TRACKSIZE | MEMTYPE_FILL))) goto error;
  }

  return CLT_SetTxAttribute (&sp->spr_TexBlend->tb_TAB, attr, value);

 error: return -1;
}


/**
|||	AUTODOC -class frame2d -name Spr_SetDBlendAttr
|||	Set a destination blend attribute in the sprite object
|||
|||	  Synopsis
|||
|||	    Err Spr_SetDBlendAttr (SpriteObj *sp, CltDblAttribute attr, uint32 value);
|||
|||	  Description
|||
|||	    Set a specified destination blend attribute for the sprite object.
|||
|||	    Unless overridden by a later object, the destination blend
|||	    attribute will stay in effect for following rendered objects.
|||
|||	    See the CLT documentation for description of the destination
|||	    blend attributes.
|||
|||	  Arguments
|||
|||	    sp
|||	        Pointer to the sprite object to modify
|||
|||	    attr
|||	        index of the attribute to set
|||
|||	    value
|||	        value to set the attribute to
|||
|||
|||	  Return Value
|||
|||	    This routine will return 0 on success or a negative error code
|||	    on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    Spr_SetTextureAttr(), Spr_SetDBlendAttr()
|||	    Spr_RemoveTextureAttr(), Spr_RemoveDBlendAttr()
|||	    Spr_GetTextureAttr(), Spr_GetDBlendAttr()
|||
**/
Err
Spr_SetDBlendAttr (SpriteObj *sp,
		   const CltDblAttribute attr, const uint32 value)
{
  if (!sp->spr_TexBlend) {
    if (!(sp->spr_TexBlend = AllocMem(sizeof(TexBlendData), MEMTYPE_NORMAL | MEMTYPE_TRACKSIZE | MEMTYPE_FILL))) goto error;
  }

  return CLT_SetDblAttribute (&sp->spr_TexBlend->tb_DAB, attr, value);

 error: return -1;
}



/**
|||	AUTODOC -class frame2d -name Spr_SetPIP
|||	Set the PIP table to be loaded for a sprite object
|||
|||	  Synopsis
|||
|||	    Err Spr_SetPIP (SpriteObj *sp, CltPipControlBlock *p);
|||
|||	  Description
|||
|||	    Set a PIP table to be loaded with the specified sprite object.
|||
|||	    Unless overridden by a later object, the PIP table
|||	    will stay in effect for following rendered objects.
|||
|||	  Arguments
|||
|||	    sp
|||	        Pointer to the sprite object to modify
|||
|||	    p
|||	        Pointer to the PIP table control block to use
|||
|||
|||	  Return Value
|||
|||	    This routine will return 0 on success or a negative error code
|||	    on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    Spr_SetPIP(), Spr_RemovePIP(), Spr_GetPIP()
|||
**/
Err
Spr_SetPIP (SpriteObj *sp, const CltPipControlBlock *p)
{
  if (!sp->spr_TexBlend) {
    if (!(sp->spr_TexBlend = AllocMem(sizeof(TexBlendData), MEMTYPE_NORMAL | MEMTYPE_TRACKSIZE | MEMTYPE_FILL))) goto error;
  }
  if (sp->spr_Flags&SPR_SYSTEMPIP) {
    Spr_CltPipControlBlock_Delete (sp);
  }

  sp->spr_TexBlend->tb_PCB = (CltPipControlBlock*)p;
  sp->spr_Flags &= ~SPR_SYSTEMPIP;

  return 0;

 error: return -1;
}


/**
|||	AUTODOC -class frame2d -name Spr_RemoveTextureAttr
|||	Remove a texture attribute from the sprite object
|||
|||	  Synopsis
|||
|||	    Err Spr_RemoveTextureAttr (SpriteObj *sp, CltTxAttribute attr);
|||
|||	  Description
|||
|||	    Remove the setting of a specified texture attribute for the
|||	    sprite object.
|||
|||	    When this sprite object is rendered, the setting of the texture
|||	    attribute will retain any previous setting.
|||
|||	    See the CLT documentation for description of the texture
|||	    attributes.
|||
|||	  Arguments
|||
|||	    sp
|||	        Pointer to the sprite object to modify
|||
|||	    attr
|||	        index of the attribute to remove
|||
|||
|||	  Return Value
|||
|||	    This routine will return 0 on success or a negative error code
|||	    on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    Spr_SetTextureAttr(), Spr_SetDBlendAttr()
|||	    Spr_RemoveTextureAttr(), Spr_RemoveDBlendAttr()
|||	    Spr_GetTextureAttr(), Spr_GetDBlendAttr()
|||
**/
Err
Spr_RemoveTextureAttr (SpriteObj *sp, const CltTxAttribute attr)
{
  if (!sp->spr_TexBlend) return 0;

  return CLT_IgnoreTxAttribute (&sp->spr_TexBlend->tb_TAB, attr);
}


/**
|||	AUTODOC -class frame2d -name Spr_RemoveDBlendAttr
|||	Remove a destination blend attribute in the sprite object
|||
|||	  Synopsis
|||
|||	    Err Spr_RemoveDBlendAttr (SpriteObj *sp, CltDblAttribute attr);
|||
|||	  Description
|||
|||	    Remove the setting of a specified destination blend attribute
|||	    for the sprite object.
|||
|||	    When this sprite object is rendered, the setting of the destination
|||	    blend attribute will retain any previous setting.
|||
|||	    See the CLT documentation for description of the destination blend
|||	    attributes.
|||
|||	  Arguments
|||
|||	    sp
|||	        Pointer to the sprite object to modify
|||
|||	    attr
|||	        index of the attribute to remove
|||
|||
|||	  Return Value
|||
|||	    This routine will return 0 on success or a negative error code
|||	    on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    Spr_SetTextureAttr(), Spr_SetDBlendAttr()
|||	    Spr_RemoveTextureAttr(), Spr_RemoveDBlendAttr()
|||	    Spr_GetTextureAttr(), Spr_GetDBlendAttr()
|||
**/
Err
Spr_RemoveDBlendAttr (SpriteObj *sp, const CltDblAttribute attr)
{
  if (!sp->spr_TexBlend) return 0;

  return CLT_IgnoreDblAttribute (&sp->spr_TexBlend->tb_DAB, attr);
}


/**
|||	AUTODOC -class frame2d -name Spr_RemovePIP
|||	Remove the PIP from a sprite object
|||
|||	  Synopsis
|||
|||	    Err Spr_RemovePIP (SpriteObj *sp);
|||
|||	  Description
|||
|||	    Remove the PIP table from a sprite object.
|||
|||	    The current sprite object will use the most recently loaded PIP
|||	    table when rendered
|||
|||	  Arguments
|||
|||	    sp
|||	        Pointer to the sprite object to modify
|||
|||
|||	  Return Value
|||
|||	    This routine will return 0 on success or a negative error code
|||	    on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    Spr_SetPIP(), Spr_RemovePIP(), Spr_GetPIP()
|||
**/
Err
Spr_RemovePIP (SpriteObj *sp)
{
  if (!sp->spr_TexBlend) return 0;
  if (sp->spr_Flags&SPR_SYSTEMPIP) {
    Spr_CltPipControlBlock_Delete (sp);
    sp->spr_Flags &= ~SPR_SYSTEMPIP;
  }
  sp->spr_TexBlend->tb_PCB = 0;
  return 0;
}


/**
|||	AUTODOC -class frame2d -name Spr_RemoveTextureData
|||	Remove the texture data from a sprite object.
|||
|||	  Synopsis
|||
|||	    Err Spr_RemoveTextureData (SpriteObj *sp);
|||
|||	  Description
|||
|||	    Remove the texture data from a sprite object.  If the sprite
|||	    object has texture data that was created with the
|||	    Spr_CreateTxData() routine, the CltTxData structure will be
|||	    deleted.
|||
|||	  Arguments
|||
|||	    sp
|||	        Pointer to the sprite object to modify
|||
|||
|||	  Return Value
|||
|||	    This routine will return 0 on success or a negative error code
|||	    on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    Spr_CreateTxData(), Spr_SetTextureData(), Spr_GetTextureData()
|||
**/
Err
Spr_RemoveTextureData (SpriteObj *sp)
{
  if (!sp->spr_TexBlend) return 0;
  if (sp->spr_Flags&SPR_SYSTEMTXDATA) {
    Spr_CltTxData_Delete (sp);
    sp->spr_Flags &= ~SPR_SYSTEMTXDATA;
  }
  sp->spr_TexBlend->tb_txData = 0;
  sp->spr_Flags |= SPR_CALC_TXLOAD;
  return 0;
}


/**
|||	AUTODOC -class frame2d -name Spr_GetTextureAttr
|||	Get the setting of a texture attribute in the sprite object
|||
|||	  Synopsis
|||
|||	    Err Spr_GetTextureAttr (SpriteObj *sp, CltTxAttribute attr, uint32 *value);
|||
|||	  Description
|||
|||	    Get the setting of a specified texture attribute for the sprite
|||	    object.
|||
|||	    See the CLT documentation for description of the texture
|||	    attributes.
|||
|||	  Arguments
|||
|||	    sp
|||	        Pointer to the sprite object to inspect
|||
|||	    attr
|||	        index of the attribute to get
|||
|||	    value
|||	        pointer to location to return the setting
|||
|||
|||	  Return Value
|||
|||	    This routine will return 0 on success or a negative error code
|||	    on failure.  If successful, the attribute setting will be placed
|||	    in the location pointed to by value.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    Spr_SetTextureAttr(), Spr_SetDBlendAttr()
|||	    Spr_RemoveTextureAttr(), Spr_RemoveDBlendAttr()
|||	    Spr_GetTextureAttr(), Spr_GetDBlendAttr()
|||
**/
Err
Spr_GetTextureAttr (const SpriteObj *sp,
		    const CltTxAttribute attr, uint32 *value)
{
  if (!sp->spr_TexBlend) goto error;
  return CLT_GetTxAttribute (&sp->spr_TexBlend->tb_TAB, attr, value);

 error: return -1;
}


/**
|||	AUTODOC -class frame2d -name Spr_GetDBlendAttr
|||	Get the setting of a destination blend attribute in the sprite object
|||
|||	  Synopsis
|||
|||	    Err Spr_GetDBlendAttr (SpriteObj *sp, CltTxAttribute attr, uint32 *value);
|||
|||	  Description
|||
|||	    Get the setting of a specified destination blend attribute for
|||	    the sprite object.
|||
|||	    See the CLT documentation for description of the destination blend
|||	    attributes.
|||
|||	  Arguments
|||
|||	    sp
|||	        Pointer to the sprite object to inspect
|||
|||	    attr
|||	        index of the attribute to get
|||
|||	    value
|||	        pointer to location to return the setting
|||
|||
|||	  Return Value
|||
|||	    This routine will return 0 on success or a negative error code
|||	    on failure.  If successful, the attribute setting will be placed
|||	    in the location pointed to by value.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    Spr_SetTextureAttr(), Spr_SetDBlendAttr()
|||	    Spr_RemoveTextureAttr(), Spr_RemoveDBlendAttr()
|||	    Spr_GetTextureAttr(), Spr_GetDBlendAttr()
|||
**/
Err
Spr_GetDBlendAttr (const SpriteObj *sp,
		   const CltDblAttribute attr, uint32 *value)
{
  if (!sp->spr_TexBlend) goto error;
  return CLT_GetDblAttribute (&sp->spr_TexBlend->tb_DAB, attr, value);

 error: return -1;
}


/**
|||	AUTODOC -class frame2d -name Spr_GetPIP
|||	Get the address of the PIP table associated with a sprite object
|||
|||	  Synopsis
|||
|||	    CltPipControlBlock *Spr_GetPIP (SpriteObj *sp);
|||
|||	  Description
|||
|||	    Return the address of the PIP table control block associated
|||	    with the sprite object.
|||
|||	  Arguments
|||
|||	    sp
|||	        Pointer to the sprite object to inspect
|||
|||
|||	  Return Value
|||
|||	    This routine will return the address of the PIP table control
|||	    block on success or zero on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    Spr_SetPIP(), Spr_RemovePIP(), Spr_GetPIP()
|||
**/
CltPipControlBlock *
Spr_GetPIP (const SpriteObj *sp)
{
  return sp->spr_TexBlend ? sp->spr_TexBlend->tb_PCB : 0;
}




/**
|||	AUTODOC -class frame2d -name Spr_Translate
|||	Change the position of a sprite object by specified deltas
|||
|||	  Synopsis
|||
|||	    Err Spr_Translate (SpriteObj* sp, gfloat x, gfloat y);
|||
|||	  Description
|||
|||	    Move the screen position of a sprite object by the specified
|||	    deltas.
|||
|||	  Arguments
|||
|||	    sp
|||	        Pointer to the sprite object to modify
|||
|||	    x
|||	        Delta to be added to the X position of the sprite
|||
|||	    y
|||	        Delta to be added to the Y position of the sprite
|||
|||
|||	  Return Value
|||
|||	    This routine will return 0 on success or a negative error
|||	    code on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    Spr_SetPosition(), Spr_Scale(), Spr_Rotate()
|||
**/
Err
Spr_Translate (SpriteObj* sp, const gfloat x, const gfloat y)
{
  if (sp->spr_Node.n_Type==SHORTSPRITEOBJNODE)
    return -1;

  sp->spr_Flags |= SPR_CALC_VERTICES;

  sp->spr_Position.x += x;
  sp->spr_Position.y += y;

  return 0;
}


/**
|||	AUTODOC -class frame2d -name Spr_Scale
|||	Change the size of a sprite object by specified ratios
|||
|||	  Synopsis
|||
|||	    Err Spr_Scale (SpriteObj* sp, gfloat x, gfloat y);
|||
|||	  Description
|||
|||	    Change the size of a sprite object by the specified ratios;
|||	    multiplying the current width and the height of the sprite
|||	    by the respective parameters.  This does not affect the default
|||	    width and height of the sprite as set by the Spr_SetWidth()
|||	    and Spr_SetHeight() functions.
|||
|||	  Arguments
|||
|||	    sp
|||	        Pointer to the sprite object to modify
|||
|||	    x
|||	        Ratio to multiply the width of the sprite by
|||
|||	    y
|||	        Ratio to multiply the height of the sprite by
|||
|||
|||	  Return Value
|||
|||	    This routine will return 0 on success or a negative error
|||	    code on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    Spr_SetPosition(), Spr_Translate(), Spr_Rotate(), Spr_SetWidth(),
|||	    Spr_SetHeight()
|||
**/
Err
Spr_Scale (SpriteObj *sp, const gfloat x, const gfloat y)
{
  if (sp->spr_Node.n_Type==SHORTSPRITEOBJNODE)
    return -1;

  sp->spr_Flags |= SPR_CALC_VERTICES;

  sp->spr_Corners[0].x *= x;
  sp->spr_Corners[0].y *= y;
  sp->spr_Corners[1].x *= x;
  sp->spr_Corners[1].y *= y;
  sp->spr_Corners[2].x *= x;
  sp->spr_Corners[2].y *= y;
  sp->spr_Corners[3].x *= x;
  sp->spr_Corners[3].y *= y;

  return 0;
}


/**
|||	AUTODOC -class frame2d -name Spr_Rotate
|||	Rotate the sprite counterclockwise by the specified angle
|||
|||	  Synopsis
|||
|||	    Err Spr_Rotate (SpriteObj *sp, gfloat angle);
|||
|||	  Description
|||
|||	    Rotate a sprite counterclockwise about its origin by the
|||	    specified angle.
|||
|||	  Arguments
|||
|||	    sp
|||	        Pointer to the sprite object to modify
|||
|||	    angle
|||	        Amount to rotate the sprite by (in degrees)
|||
|||
|||	  Return Value
|||
|||	    This routine will return 0 on success or a negative error
|||	    code on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    Spr_SetPosition(), Spr_Translate(), Spr_Scale()
|||
**/
Err
Spr_Rotate (SpriteObj *sp, const gfloat angle)
{
  gfloat s, c, x, y;

  if (sp->spr_Node.n_Type==SHORTSPRITEOBJNODE)
    return -1;

  sp->spr_Flags |= SPR_CALC_VERTICES;

  s = sinf(angle*(PI/180.));
  c = cosf(angle*(PI/180.));

  x = sp->spr_Corners[0].x*c + sp->spr_Corners[0].y*s;
  y = sp->spr_Corners[0].y*c - sp->spr_Corners[0].x*s;
  sp->spr_Corners[0].x = x;
  sp->spr_Corners[0].y = y;
  x = sp->spr_Corners[1].x*c + sp->spr_Corners[1].y*s;
  y = sp->spr_Corners[1].y*c - sp->spr_Corners[1].x*s;
  sp->spr_Corners[1].x = x;
  sp->spr_Corners[1].y = y;
  x = sp->spr_Corners[2].x*c + sp->spr_Corners[2].y*s;
  y = sp->spr_Corners[2].y*c - sp->spr_Corners[2].x*s;
  sp->spr_Corners[2].x = x;
  sp->spr_Corners[2].y = y;
  x = sp->spr_Corners[3].x*c + sp->spr_Corners[3].y*s;
  y = sp->spr_Corners[3].y*c - sp->spr_Corners[3].x*s;
  sp->spr_Corners[3].x = x;
  sp->spr_Corners[3].y = y;

  return 0;
}

Err SetGridClipMode(GridObj *gr, bool enable)
{
  int32 i, j;
  SpriteObj *so;

  for (j=0; j<gr->gro_Height; j++) {
      for (i=0; i<gr->gro_Width; i++) {
          so = gr->gro_SpriteArray[j*gr->gro_Width+i];
          if (enable) {
              so->spr_Flags &= ~SPR_DONT_CLIP;
          } else {
              so->spr_Flags |= SPR_DONT_CLIP;
          }
      }
  }

  return 0;
}

/**
|||	AUTODOC -class frame2d -name Spr_DisableClipping
|||	Disables clipping for a sprite.
|||
|||	  Synopsis
|||
|||	    Err Spr_DisableClipping(void *f2obj);
|||
|||	  Description
|||
|||	    Turns off clipping for the object, which will improve
|||	    performance for F2_Draw() for this object. You should only
|||	    disable clipping when you know that the object will always
|||	    be rendered wholly on-screen (for example, static objects
|||	    such as icons).
|||
|||	    For Grid objects, the clipping is disabled for all the
|||	    sprites in the spritearray. You can, if you wish, enable
|||	    clipping for any individual sprite.
|||
|||	    By default, clipping is enabled for objects.
|||
|||	  Arguments
|||
|||	    f2obj
|||	        Pointer to the object to modify
|||
|||	  Return Value
|||
|||	    This routine will return 0 on success or a negative error
|||	    code on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    Spr_EnableClipping()
|||
**/
Err Spr_DisableClipping(void* f2obj)
{
  switch (((Frame2Obj*)f2obj)->Node.n_Type) {
  case SPRITEOBJNODE:
  case EXTSPRITEOBJNODE:
  case SHORTSPRITEOBJNODE:
      ((SpriteObj *)f2obj)->spr_Flags |= SPR_DONT_CLIP;
      break;
  case GRIDOBJNODE:
      return SetGridClipMode(f2obj, FALSE);
  default:
    return -1;
  }
  return 0;
}

/**
|||	AUTODOC -class frame2d -name Spr_EnableClipping
|||	Enables clipping for a sprite.
|||
|||	  Synopsis
|||
|||	    Err Spr_EnableClipping(void *f2obj);
|||
|||	  Description
|||
|||	    Turns on clipping for the object, which will decrease
|||	    performance for F2_Draw() for this object. You should
|||	    enable clipping when you know that the object will sometimes
|||	    be rendered partially or wholly off-screen.
|||
|||	    For Grid objects, the clipping is enabled for all the
|||	    sprites in the spritearray. You can, if you wish, disable
|||	    clipping for any individual sprite.
|||
|||	    By default, clipping is enabled for objects.
|||
|||	  Arguments
|||
|||	    f2obj
|||	        Pointer to the object to modify
|||
|||	  Return Value
|||
|||	    This routine will return 0 on success or a negative error
|||	    code on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    Spr_DisableClipping()
|||
**/
Err Spr_EnableClipping(void* f2obj)
{
  switch (((Frame2Obj*)f2obj)->Node.n_Type) {
  case SPRITEOBJNODE:
  case EXTSPRITEOBJNODE:
  case SHORTSPRITEOBJNODE:
      ((SpriteObj *)f2obj)->spr_Flags &= ~SPR_DONT_CLIP;
      break;
  case GRIDOBJNODE:
      return SetGridClipMode(f2obj, TRUE);
  default:
    return -1;
  }
  return 0;
}
