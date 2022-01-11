/*
 *	@(#) texture2d.c 96/08/08 1.14
 *
 * 2D Framework for M2 graphics
 *
 */

/*
 * 2D Rendering routines
 */

#include "frame2.i"
/*#define DBUG(x) printf x */
#define DBUG(x)


/**
|||	AUTODOC -class frame2d -name CltTxData_Create
|||	Create a CltTxData structure that describes texel data in memory
|||
|||	  Synopsis
|||
|||	    CltTxData *CltTxData_Create (void *texeldata, uint32 width,
|||	                             uint32 height, uint32 bpp,
|||	                             uint32 expformat);
|||
|||	  Description
|||
|||	    This routine creates a CltTxData structure to point to texture
|||	    data that is already in memory, or that is to be created by the
|||	    application code.
|||
|||	    The CltTxData structure can be linked to one or more Sprite
|||	    objects.
|||
|||	  Arguments
|||
|||	    texeldata
|||	        Pointer to the actual texel data in memory
|||
|||	    width
|||	        Width of the texel data in pixels
|||
|||	    height
|||	        Height of the texel data in pixels
|||
|||	    bpp
|||	        Number of bits per pixel of the texel data
|||
|||	    expformat
|||	        Expansion format constant.  See the MAKEEXPFORMAT macro
|||
|||
|||	  Return Value
|||
|||	    This routine will return a pointer to a CltTxData structure, or
|||	    zero on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    CltTxData_Delete(), Spr_CreateTxData(), Spr_SetTextureData()
|||
**/
CltTxData *
CltTxData_Create (const void *texeldata,
		  const uint32 width, const uint32 height,
		  const uint32 bpp, const uint32 expformat)
{
  CltTxData *td;
  CltTxLOD *p=0;

  DBUG (("CltTxData_Create \n\ttexeldata %lx, width %d, height %d, bpp %d, expformat %lx\n", texeldata, width, height, bpp, expformat));

  td = (CltTxData*) AllocMem(sizeof(CltTxData), MEMTYPE_NORMAL | MEMTYPE_TRACKSIZE | MEMTYPE_FILL);
  if (!td) goto error;

  p = (CltTxLOD*) AllocMem(sizeof(CltTxLOD), MEMTYPE_NORMAL | MEMTYPE_TRACKSIZE);
  if (!p) goto error;

  p->texelData = (void*)texeldata;
  p->texelDataSize = (width*height*bpp+7)>>3;
  td->texelData = p;
  td->minX = width;
  td->minY = height;
  td->maxLOD = 1;
  td->bitsPerPixel = bpp;
  td->expansionFormat = expformat;
  td->dci = NULL;

  return td;

 error:
  FreeMem(td, TRACKED_SIZE);
  FreeMem(p, TRACKED_SIZE);
  return 0;
}


/**
|||	AUTODOC -class frame2d -name CltTxData_Delete
|||	Delete a CltTxData structure
|||
|||	  Synopsis
|||
|||	    Err CltTxData_Delete (CltTxData* t);
|||
|||	  Description
|||
|||	    This routine deletes a CltTxData structure.
|||
|||	  Arguments
|||
|||	    t
|||	        Pointer to the CltTxData structure
|||
|||	  Return Value
|||
|||	    This routine will return 0 on success, or a negative error
|||	    code on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    CltTxData_Create(), Spr_CreateTxData()
|||
**/
Err
CltTxData_Delete (CltTxData* t)
{
  if (t) {
    DBUG (("Remove the texelData array\n"));
    FreeMem(t->texelData, TRACKED_SIZE);
    DBUG (("Remove the dci\n"));
    FreeMem(t->dci, TRACKED_SIZE);
    DBUG (("Remove the CltTxData\n"));
    FreeMem(t, TRACKED_SIZE);
  }
  return 0;
}


/**
|||	AUTODOC -class frame2d -name CltPipControlBlock_Create
|||	Create a PIP controlblock.
|||
|||	  Synopsis
|||
|||	    CltPipControlBlock *
|||	    CltPipControlBlock_Create (void *pipdata, uint32 pipindex,
|||	                               uint32 numentries)
|||
|||	  Description
|||
|||	    This routine creates a CltPipControlBlock structure to point
|||	    to PIP data that is already in memory, or that is to be
|||	    created by the application code.
|||
|||	  Arguments
|||
|||	    pipdata
|||	        Pointer to the actual PIP data in memory
|||
|||	    pipindex
|||	        Index of the first PIP color in the table
|||
|||	    numentries
|||	        Number of entries in the PIP table
|||
|||
|||	  Return Value
|||
|||	    This routine will return a pointer to a CltPipControlBlock
|||	    structure, or zero on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    CltPipControlBlock_Delete(), Spr_CreatePipControlBlock()
|||
**/
CltPipControlBlock *
CltPipControlBlock_Create (const void *pipdata,
			   const uint32 pipindex, const uint32 numentries)
{
  CltPipControlBlock *pcb;

  pcb = (CltPipControlBlock*) AllocMem(sizeof(CltPipControlBlock), MEMTYPE_NORMAL | MEMTYPE_TRACKSIZE);
  if (!pcb) goto error;

  pcb->pipData = (void*)pipdata;
  pcb->pipIndex = pipindex;
  pcb->pipNumEntries = numentries;
  CLT_InitSnippet (&pcb->pipCommandList);

  if (CLT_CreatePipCommandList(pcb) >= 0) return pcb;
  DBUG (("Error creating PIP command list\n"));

 error:
  if (pcb) {
    CltPipControlBlock_Delete (pcb);
  }
  return 0;
}


/**
|||	AUTODOC -class frame2d -name CltPipControlBlock_Delete
|||	Delete a PIP control block.
|||
|||	  Synopsis
|||
|||	    Err CltPipControlBlock_Delete (CltPipControlBlock* pcb);
|||
|||	  Description
|||
|||	    This routine deletes a CltPipControlBlock structure.
|||
|||	  Arguments
|||
|||	    pcb
|||	        Pointer to the CltPipControlBlock structure
|||
|||	  Return Value
|||
|||	    This routine will return 0 on success, or a negative error
|||	    code on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    CltPipControlBlock_Create(), Spr_CreatePipControlBlock()
|||
**/
Err
CltPipControlBlock_Delete (CltPipControlBlock *pcb)
{
  if (pcb) {
    DBUG (("Remove the PIP command list\n"));
    CLT_FreeSnippet (&pcb->pipCommandList);
    DBUG (("Remove the pcb\n"));
    FreeMem(pcb, TRACKED_SIZE);
  }
  return 0;
}


/**
|||	AUTODOC -class frame2d -name Spr_CreateTxData
|||	Create a CltTxData block and attach it to a sprite.
|||
|||	  Synopsis
|||
|||	    Err Spr_CreateTxData (SpriteObj *sp, void *texeldata,
|||	                          uint32 width, uint32 height, uint32 bpp,
|||	                          uint32 expformat)
|||
|||	  Description
|||
|||	    This routine creates a CltTxData structure and attaches it to
|||	    a sprite object.
|||
|||	    If a CltTxData structure that had been created through a
|||	    call to Spr_CreateTxData() is already attached to the sprite
|||	    object, then it will be deleted when the new one is created.
|||
|||	    A CltTxData structure that was created with Spr_CreateTxData()
|||	    will also be deleted with the sprite when Spr_Delete() is called,
|||	    or when Spr_SetTextureData() is called.
|||
|||	  Arguments
|||
|||	    sp
|||	        Pointer to the sprite object to modify
|||
|||	    texeldata
|||	        Pointer to the actual texel data in memory
|||
|||	    width
|||	        Width of the texel data in pixels
|||
|||	    height
|||	        Height of the texel data in pixels
|||
|||	    bpp
|||	        Number of bits per pixel of the texel data
|||
|||	    expformat
|||	        Expansion format constant.  See the MAKEEXPFORMAT macro
|||
|||
|||	  Return Value
|||
|||	    This routine will return 0 on success, or a negative error
|||	    code on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/spriteobj.h>
|||
|||	  See Also
|||
|||	    CltTxData_Create(), CltTxData_Delete(), Spr_SetTextureData()
|||
**/
Err
Spr_CreateTxData (SpriteObj *sp, const void *texeldata, const uint32 width,
		  const uint32 height, const uint32 bpp,
		  const uint32 expformat)
{
  CltTxData *t;
  Err err;

  t = CltTxData_Create (texeldata, width, height, bpp, expformat);
  if (!t) goto error;

  err = Spr_SetTextureData (sp, t);
  if (err<0) goto error;
  sp->spr_Flags |= SPR_SYSTEMTXDATA;
  return err;

 error:
  CltTxData_Delete (t);
  return -1;
}


/**
|||	AUTODOC -class frame2d -name Spr_CreatePipControlBlock
|||	Create a PIP control block and attach it to a sprite.
|||
|||	  Synopsis
|||
|||	    Err Spr_CreatePipControlBlock (SpriteObj *sp, void *pipdata,
|||	                                   uint32 pipindex, uint32 numentries);
|||
|||	  Description
|||
|||	    This routine creates a CltPipControlBlock structure and attaches
|||	    it to a sprite object.
|||
|||	    If a CltPipControlBlock structure that had been created through a
|||	    call to Spr_CreatePipControlBlock() is already attached to the
|||	    sprite object, then it will be deleted when the new one is created.
|||
|||	    A CltPipControlBlock structure that was created with
|||	    Spr_CreatePipControlBlock() will also be deleted with the sprite
|||	    when Spr_Delete() is called, or when Spr_SetPIP is called.
|||
|||	  Arguments
|||
|||	    sp
|||	        Pointer to the sprite object to modify
|||
|||	    pipdata
|||	        Pointer to the actual PIP data in memory
|||
|||	    pipindex
|||	        Index of the first PIP color in the table
|||
|||	    numentries
|||	        Number of entries in the PIP table
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
|||	    CltPipControlBlock_Create()
|||
**/
Err
Spr_CreatePipControlBlock (SpriteObj *sp, const void *pipdata,
			   const uint32 pipindex, const uint32 numentries)
{
  CltPipControlBlock *pcb;
  Err err=-1;

  pcb = CltPipControlBlock_Create (pipdata, pipindex, numentries);
  if (!pcb) goto error;

  err = Spr_SetPIP (sp, pcb);
  if (err<0) goto error;
  sp->spr_Flags |= SPR_SYSTEMPIP;
  return err;

 error:
  CltPipControlBlock_Delete (pcb);
  return err;
}


