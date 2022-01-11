/* @(#) autodocs.c 96/10/18 1.18 */
/**
|||	AUTODOC -class Blitter -group Blit -name Blt_RectangleToBlitObject
|||	Blits a rectangular area of a bitmap into a BlitObject.
|||
|||	  Synopsis
|||
|||	    Err Blt_RectangleToBlitObject(GState *gs, const BlitObject *bo,
|||	                                  Item srcBitmap, const BlitRect *rect);
|||
|||	  Description
|||
|||	    Copies a rectangular area of the srcBitmap into the
|||	    BltTxData of the BlitObject. By default, the blitter folio
|||	    will allocate a buffer large enough for  the
|||	    data. However, if the application allocates the buffer,
|||	    and the buffer is too small, an error will be returned.
|||
|||	  Arguments
|||
|||	    gs
|||	        Pointer to a GState that may be used to copy the data. A
|||	        NULL value forces the blit to be performed with the CPU.
|||
|||	    bo
|||	         Pointer to a BlitObject.
|||
|||	    srcBitmap
|||	         The Item of the source BitMap to copy from.
|||
|||	    rect
|||	         Pointer to a BlitRect defining the area of the srcBitmap
|||	         to copy.
|||
|||	  Return Value
|||
|||	    Negative on error.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Blitter folio V32.
|||
|||	  Caveats
|||
|||	    Depending on the blit, the GState might be used. If it is,
|||	    then the contents of the data buffer will not be filled
|||	    until the Triangle Engine instructions are executed with
|||	    GS_SendList() and GS_WaitIO(). Also in this case, the
|||	    function will call GS_SetDestBuffer(), so you must call
|||	    GS_SetDestBuffer() again to set up your destintaion.
|||
|||	  Associated Files
|||
|||	    <graphics/blitter.h> <graphics/clt/gstate.h>
|||
|||	  See Also
|||
|||	    Blt_CreateBlitObject() Blt_BlitObjectToBitmap()
|||	    Blt_RectangleToBuffer()
|||
**/

/**
|||	AUTODOC -class Blitter -group Blit -name Blt_RectangleToBuffer
|||	Blits a rectangular area of a bitmap into a buffer
|||
|||	  Synopsis
|||
|||	    Err Blt_RectangleToBuffer(GState *gs, Item srcBitmap,
|||	                              const BlitRect *rect, void *buffer);
|||
|||	  Description
|||
|||	    Copies a rectangular area of the srcBitmap into the
|||	    buffer. It is up to the application to ensure that the
|||	    buffer is large enough to store the data.
|||
|||	  Arguments
|||
|||	    gs
|||	         Pointer to a GState that may be used to copy the data. A
|||	         NULL value forces the blit to be performed with the CPU.
|||
|||	    srcBitmap
|||	         The Item of the source BitMap to copy from.
|||
|||	    rect
|||	         Pointer to a BlitRect defining the area of the srcBitmap
|||	         to copy.
|||
|||	    buffer
|||	        Pointer to a buffer to fill with the copied data.
|||
|||	  Return Value
|||
|||	    Negative on error.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Blitter folio V32.
|||
|||	  Associated Files
|||
|||	    <graphics/blitter.h> <graphics/clt/gstate.h>
|||
|||	  See Also
|||
|||	    Blt_RectangleToBlitObject()
|||
**/

/**
|||	AUTODOC -class Blitter -group Blit -name Blt_BlitObjectToBitmap
|||	Writes the TriangleEngine instructions to blit from the
|||	BlitObject to the destination Bitmap.
|||
|||	  Synopsis
|||
|||	    Err Blt_BlitObjectToBitmap(GState *gs, const BlitObject *bo,
|||	                               Item dstBitmap, uint32 flags)
|||
|||	  Description
|||
|||	    Copies into the GState the instructions needed to blit
|||	    from the buffer in the BlitObject into the destination
|||	    BitMap. If the buffer is too large to fit into TRAM, new
|||	    vertices are calculated based on the vertices in the
|||	    bo->bo_vertices list to slice the buffer into strips that
|||	    the TriangeEngine can handle. This slicing will be
|||	    performed the first time this BlitObject is used, and will
|||	    not be recalculated until either a buffer of a different
|||	    dimension is used in the BlitObject or the vertices are
|||	    modified.
|||
|||	    The TriangleEngine instructions are divided into four
|||	    structures, controlling the four logical operations of the
|||	    Triangle Engine.
|||
|||	    The PIPLoadSnippet contains instructions for loading the
|||	    PIP.
|||
|||	    The TxBlendSnippet contains instructions for setting up
|||	    the Texture Blending when the texture is loaded into TRAM.
|||
|||	    The TxLoadSnippet contains instructions for loading the
|||	    texture into TRAM.
|||
|||	    The DBLendSnippet contains instructions for setting up the
|||	    Destination Blender.
|||
|||	    The VerticesSnippet contains the list of vertices that the
|||	    data will be rendered through.
|||
|||	    By default, Blt_BlitObjectToBitmap() will copy all of
|||	    these snippets into the GState list. However, the
|||	    application can override this by specifying snippets to
|||	    not copy. The exceptions to this are:
|||
|||	      Operations that require the source texture to be sliced
|||	      will always use the TxLoadSnippet.
|||
|||	      dbl_instruction1, dbl_userGenCntl and dbl_discardCntl
|||	      are always copied.
|||
|||	  Arguments
|||
|||	    gs
|||	         Pointer to a GState to be filled with TriangleEngine
|||	         instructions.
|||
|||	    bo
|||	         Pointer to the source BlitObject, containing the source
|||	         image and the rendering vertices and parameters.
|||
|||	    dstBitmap
|||	         Item of the bitmap to render into.
|||
|||	    flags
|||	         Bit field specifying which snippets to not copy into the
|||	         GState. See skipSnippet_flags in <graphics/blitter.h>.
|||
|||	  Return Value
|||
|||	    Negative on error.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Blitter folio V32.
|||
|||	  Associated Files
|||
|||	    <graphics/blitter.h> <graphics/clt/gstate.h>
|||
|||	  See Also
|||
|||	    Blt_CreateBlitObject() Blt_RectangleToBlitObject()
|||
**/
/**
|||	AUTODOC -class Blitter -group Blit -name Blt_CreateBlitObject
|||	Creates a BlitObject.
|||
|||	  Synopsis
|||
|||	    Err Blt_CreateBlitObject(BlitObject **bo, const TagArg *tagList);
|||	    Err Blt_CreateBlitObjectVA(BlitObject **bo, uint32 tag, ...);
|||
|||	  Description
|||
|||	    Creates a new BlitObject. By default, it will create all
|||	    the Snippets needed for a simple blit copy. The
|||	    application can optionally create the BlitObject using
|||	    previously-intialised Snippets instead of creating new
|||	    ones. These snippets are reused (see Blt_ReuseSnippet())
|||	    rather than copied (Blt_CopySnippet()).
|||
|||	    You can avoid having a snippet allocated if you pass NULL
|||	    as the ta_Arg value for one of the BLIT_TAG_... arguments.
|||
|||	  Arguments
|||
|||	    bo
|||	         Pointer to a pointer to a BlitObject. This will be
|||	         filled with the pointer to the newly created BlitObject.
|||
|||	    tagList
|||	         Pointer to a list of TagArgs.
|||
|||	  Tag Arguments
|||
|||	    BLIT_TAG_DBLEND, DBlendSnippet *
|||	         Pointer to a DBlendSnippet to use in the new BlitObject
|||
|||	    BLIT_TAG_TBLEND, TxBlendSnippet *
|||	         Pointer to a TxBlendSnippet to use in the new BlitObject
|||
|||	    BLIT_TAG_TXLOAD, TxLoadSnippet *
|||	         Pointer to a TxLoadSnippet to use in the new BlitObject
|||
|||	    BLIT_TAG_PIP, PIPLoadSnippet *
|||	         Pointer to a PIPLoadSnippet to use in the new BlitObject
|||
|||	    BLIT_TAG_TXDATA, BltTxData *
|||	         Pointer to a BltTxData to use in the new BlitObject
|||
|||	    BLIT_TAG_VERTICES, VerticesSnippet *
|||	         Pointer to a VerticesSnippet to use in the new BlitObject
|||
|||	  Return Value
|||
|||	    Negative on error.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Blitter folio V32.
|||
|||	  Caveats
|||
|||	    The vertices are created as
|||	    CLA_TRIANGLE(1, RC_FAN, 1, 1, 0, 4), and the actual
|||	    vertices are undefined and must be set with
|||	    Blt_SetVertices(). You can override this by creating a
|||	    VerticesSnippet of the required format and passing that as
|||	    the BLIT_TAG_VERTICES tag.
|||
|||	  Associated Files
|||
|||	    <graphics/blitter.h> <graphics/clt/gstate.h>
|||
|||	  See Also
|||
|||	    Blt_DeleteBlitObject()
|||
**/

/**
|||	AUTODOC -class Blitter -group Blit -name Blt_DeleteBlitObject
|||	Deletes a BlitObject
|||
|||	  Synopsis
|||
|||	    void Blt_DeleteBlitObject(const BlitObject *bo);
|||
|||	  Description
|||
|||	    Deletes a BlitObject, and decrements the usage counters of
|||	    all the BlitObject's snippets. If any usage count reaches
|||	    0, then all the resources associated with the snippet are
|||	    also freed.
|||
|||	  Arguments
|||
|||	    bo
|||	         Pointer to the BlitObject to delete.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Blitter folio V32.
|||
|||	  Associated Files
|||
|||	    <graphics/blitter.h>
|||
|||	  See Also
|||
|||	    Blt_CreateBlitObject()
|||
**/

/**
|||	AUTODOC -class Blitter -group Mask -name Blt_MakeMask
|||	Modifies texture data for blit-thru-mask.
|||
|||	  Synopsis
|||
|||	    Err Blt_MakeMask(const BlitObject *bo, const BlitMask *mask);
|||
|||	  Description
|||
|||	    The texture buffer in the BlitObject is modified according
|||	    to the BlitMask->blm_data bit array, in order to use the
|||	    DestinationBlender's PixelDiscard feature.
|||
|||	    The BlitMask->blm_data is a bit array defining the shape
|||	    of the mask, where a 1 indicates pixels that will be seen,
|||	    and a 0 indicates pixels that will be discarded.
|||
|||	    If the mask is of a different dimension to the texture
|||	    data in the BlitObject, then the BlitMask->blm_flags field
|||	    specifies whether the mask should be centered in the
|||	    texture (FLAG_BLM_CENTER), and whether the mask should be
|||	    repeated throughout the texture in a tiled-fashion
|||	    (FLAG_BLM_REPEAT). The meaning of the bits in the mask can
|||	    be switched with the FLAG_BLM_INVERT flag.
|||
|||	    The application should specify which of the three
|||	    PixelDiscard modes to use in
|||	    BlitMask->blm_discardType. These are
|||	    FV_DBDISCARDCONTROL_SSB0_MASK,
|||	    FV_DBDISCARDCONTROL_RGB0_MASK,
|||	    FV_DBDISCARDCONTROL_ALPHA0_MASK, or any combination.
|||
|||	    For .._SSB0, the SSB of the texture in the BlitObject is
|||	    reset to 0 to discard the texture pixel.
|||
|||	    For .._RGB0, the RGB of the texture in the BlitObject is
|||	    set to 0 to discard the texture pixel.
|||
|||	    For .._ALPHA0, the Alpha value of the texture in the BlitObject is
|||	    set to 0 to discard the texture pixel.
|||
|||	    Pixels other than those set by the mask array in the
|||	    BlitObject's texture data may also have the necessary
|||	    values to be discarded. For example, if the BlitObject has
|||	    a copy of a region of a bitmap that has all the SSB bits
|||	    in all the pixels set to 0, then all the pixels in
|||	    the texture will be discarded. Setting
|||	    FLAG_BLM_FORCE_VISIBLE in the BitMask->blm_flags will
|||	    force the SSB bit to 1 of pixels in the area of the mask
|||	    that are to be visible if _SSB0 discarding is used, or
|||	    will set the RGB to 0x000001 if _RGB0 discarding is used,
|||	    or set the Alpha to 1 if _ALPHA0 is used.
|||
|||	  Arguments
|||
|||	    bo
|||	         Pointer to a BlitOject to mask.
|||
|||	    mask
|||	         Pointer to a BlitMask structure defining the mask and parameters.
|||
|||	  Return Value
|||
|||	    Negative on error.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Blitter folio V32.
|||
|||	  Caveats
|||
|||	    This function will modify the texture data.
|||
|||	  Associated Files
|||
|||	    <graphics/blitter.h>
|||
|||	  See Also
|||
|||	    Blt_EnableMask() Blt_DisableMask()
|||
**/

/**
|||	AUTODOC -class Blitter -group Snippets -name Blt_CreateSnippet
|||	Creates a BlitObject snippet.
|||
|||	  Synopsis
|||
|||	    void *Blt_CreateSnippet(uint32 type);
|||
|||	  Description
|||
|||	    Creates a BlitObject snippet of the required type, for
|||	    using in BlitObjects. The snippet is initialised with
|||	    default values appropriate for a simple blit.
|||
|||	    The default values for the various snippets are:
|||	  -preformatted
|||	    TxBlendSnippet defaultTxBlend =
|||	    {
|||	         // BlitterSnippetHeader
|||	        CLT_WriteRegistersHeader(TXTADDRCNTL, 1),
|||	        CLA_TXTADDRCNTL(1, 0, 0, 0, 0),
|||	        CLT_WriteRegistersHeader(TXTTABCNTL, 1),
|||	        CLA_TXTTABCNTL(0, 0, 0, 0, 0,
|||	                       RC_TXTTABCNTL_COLOROUT_TEXCOLOR,
|||	                       RC_TXTTABCNTL_ALPHAOUT_TEXALPHA,
|||	                       0),
|||	        CLT_WriteRegistersHeader(TXTSRCTYPE01, 7),
|||	        0, 0,
|||	            // Assume 5bits RGB, 1 bit SSB = 16bit frame buffer
|||	        CLA_TXTEXPTYPE(5, 4, 0, 1, 1, 0, 1),
|||	        0, 0, 0, 0,
|||	    }
|||
|||	    TxLoadSnippet defaultTxLoad =
|||	    {
|||	         // BlitterSnippetHeader
|||	        CLT_WriteRegistersHeader(TXTLDCNTL, 1),
|||	        CLA_TXTLDCNTL(0, RC_TXTLDCNTL_LOADMODE_TEXTURE, 0),
|||	        CLT_WriteRegistersHeader(TXTLDDSTBASE, 1),
|||	        0,
|||	        CLT_WriteRegistersHeader(TXTLDSRCADDR, 3),
|||	        0,
|||	        0,
|||	        0,
|||	        CLT_WriteRegistersHeader(DCNTL, 1),
|||	        CLT_Bits(DCNTL, TLD, 1),
|||	        CLT_WriteRegistersHeader(TXTUVMAX, 2),
|||	        (CLT_Bits(TXTUVMAX, UMAX, 0x3f) |
|||	         CLT_Bits(TXTUVMAX, VMAX, 0x3f)),
|||	        (CLT_Bits(TXTUVMASK, UMASK, 0x3ff) |
|||	         CLT_Bits(TXTUVMASK, VMASK, 0x3ff)),
|||	    }
|||
|||	    PIPLoadSnippet defaultPIPLoad =
|||	    {
|||	         // BlitterSnippetHeader
|||	        CLT_WriteRegistersHeader(TXTPIPCNTL, 1),
|||	        CLA_TXTPIPCNTL(RC_TXTPIPCNTL_PIPSSBSELECT_TEXTURE,
|||	                       RC_TXTPIPCNTL_PIPALPHASELECT_TEXTURE,
|||	                       RC_TXTPIPCNTL_PIPCOLORSELECT_TEXTURE,
|||	                       0),
|||	        CLT_WriteRegistersHeader(TXTCONST0, 2),
|||	        0,
|||	        0,
|||	        CLT_WriteRegistersHeader(TXTLDCNTL, 1),
|||	        CLA_TXTLDCNTL(0, RC_TXTLDCNTL_LOADMODE_PIP, 0),
|||	        CLT_WriteRegistersHeader(TXTLODBASE0, 1),
|||	        TEPIPRAM,
|||	        CLT_WriteRegistersHeader(TXTLDSRCADDR, 2),
|||	        0,
|||	        0,
|||	        CLT_WriteRegistersHeader(DCNTL, 1),
|||	        CLT_Bits(DCNTL, TLD, 1),
|||	    };
|||
|||	    DBlendSnippet defaultDBlend =
|||	    {
|||	         // BlitterSnippetHeader
|||	        CLT_WriteRegistersHeader(DBUSERCONTROL, 2),
|||	        CLA_DBUSERCONTROL (0, 0, 0, 0, 0, 0, 0,
|||	                           (CLT_SetConst(DBUSERCONTROL, DESTOUTMASK, RED) | 
|||	                            CLT_SetConst(DBUSERCONTROL, DESTOUTMASK, GREEN) | 
|||	                            CLT_SetConst(DBUSERCONTROL, DESTOUTMASK, BLUE) | 
|||	                            CLT_SetConst(DBUSERCONTROL, DESTOUTMASK, ALPHA))),
|||	        CLA_DBDISCARDCONTROL(0, 0, 0, 0),
|||	        CLT_WriteRegistersHeader(DBSRCCNTL, 4),
|||	        CLA_DBSRCCNTL(1, 0),
|||	        0,
|||	        320,
|||	        CLA_DBSRCOFFSET(0, 0),
|||	        CLT_WriteRegistersHeader(DBSSBDSBCNTL, 9),
|||	        CLA_DBSSBDSBCNTL(0, RC_DBSSBDSBCNTL_DSBSELECT_OBJSSB),
|||	        0x0fff,
|||	        CLA_DBAMULTCNTL(RC_DBAMULTCNTL_AINPUTSELECT_TEXCOLOR,
|||	                        RC_DBAMULTCNTL_AMULTCOEFSELECT_CONST,
|||	                        RC_DBAMULTCNTL_AMULTCONSTCONTROL_TEXSSB,
|||	                        0),
|||	        0x444444,
|||	        0x444444,
|||	        CLA_DBBMULTCNTL(RC_DBBMULTCNTL_BINPUTSELECT_SRCCOLOR,
|||	                        RC_DBBMULTCNTL_BMULTCOEFSELECT_CONST,
|||	                        RC_DBBMULTCNTL_BMULTCONSTCONTROL_TEXSSB,
|||	                        3),
|||	        0xcccccc,
|||	        0xcccccc,
|||	        CLA_DBALUCNTL(RC_DBALUCNTL_ALUOPERATION_A_PLUS_BCLAMP, 0),
|||	        CLT_WriteRegistersHeader(DBSRCALPHACNTL, 3),
|||	        0,
|||	        0,
|||	        0,
|||	    }
|||
|||	  Arguments
|||
|||	    type
|||	         One of the snippet types (see BlitObject_tags in
|||	         <graphics/blitter.h>).
|||
|||	  Return Value
|||
|||	    A pointer to the created snippet, or NULL if the snippet
|||	    could not be created.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Blitter folio V32.
|||
|||	  Associated Files
|||
|||	    <graphics/blitter.h>
|||
|||	  See Also
|||
|||	    Blt_DeleteSnippet()
|||
**/

/**
|||	AUTODOC -class Blitter -group Snippets -name Blt_DeleteSnippet
|||	Deletes a BlitObject snippet.
|||
|||	  Synopsis
|||
|||	    Err Blt_DeleteSnippet(const void *snippet);
|||
|||	  Description
|||
|||	    Deletes the snippet, and all resources associated with it
|||	    if its usage count reaches 0.
|||
|||	  Arguments
|||
|||	    snippet
|||	        Pointer to the snippet to be deleted.
|||
|||	  Return Value
|||
|||	    Negative on error (such as the snippet still being used).
|||
|||	  Implementation
|||
|||	    Folio call implemented in Blitter folio V32.
|||
|||	  Associated Files
|||
|||	    <graphics/blitter.h>
|||
|||	  See Also
|||
|||	    Blt_CreateSnippet() Blt_ReuseSnippet()
|||
**/

/**
|||	AUTODOC -class Blitter -group Snippets -name Blt_CopySnippet
|||	Duplicates a BlitOjbect snippet.
|||
|||	  Synopsis
|||
|||	    Err Blt_CopySnippet(const BlitObject *src, BlitObject *dest,
|||	                        void **original, uint32 type)
|||
|||	  Description
|||
|||	    Duplicates a snippet in the src BlitObject, puts the
|||	    duplicate in the dest BlitObject, and returns the pointer
|||	    to the snippet in the dest BlitObject that is being
|||	    replaced.
|||
|||	    The usage count of the snippet being replaced in the dest
|||	    BlitObject is decremented by 1.
|||
|||	  Arguments
|||
|||	    src
|||	        Pointer to the source BlitObject.
|||
|||	    dest
|||	         Pointer to the destination BlitObject
|||
|||	    original
|||	         Pointer to a pointer that will be set with the pointer
|||	         to the snippet being replaced in the dest BlitObject.
|||	         original can be NULL.
|||
|||	    type
|||	         The snippet type to copy (see BlitObject_tags in
|||	         <graphics/blitter.h>).
|||
|||	  Return Value
|||
|||	    Negative on error.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Blitter folio V32.
|||
|||	  Associated Files
|||
|||	    <graphics/blitter.h>
|||
|||	  See Also
|||
|||	    Blt_ReuseSnippet()
|||
**/

/**
|||	AUTODOC -class Blitter -group Snippets -name Blt_ReuseSnippet
|||	Shares a snippet across BlitObjects.
|||
|||	  Synopsis
|||
|||	    void *Blt_ReuseSnippet(const BlitObject *src, BlitObject *dest, uint32 type);
|||
|||	  Description
|||
|||	    Copies the pointer of the snippet in the src BlitObject to
|||	    the dest BlitObject, and increments the usage count of the
|||	    snippet by 1. The usage count of the snippet being
|||	    replaced in the dest BlitObject is decremented by 1.
|||
|||	  Arguments
|||
|||	    src
|||	         Pointer to the source BiitObject
|||
|||	    dest
|||	         Pointer to the destination BlitObject
|||
|||	    type
|||	         The snippet type to reuse (see BlitObject_tags in
|||	         <graphics/blitter.h>).
|||
|||	  Return Value
|||
|||	    Pointer to the snippet being replaced in the dest BlitObject.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Blitter folio V32.
|||
|||	  Associated Files
|||
|||	    <graphics/blitter.h>
|||
|||	  See Also
|||
|||	    Blt_CopySnippet()
|||
**/
/**
|||	AUTODOC -class Blitter -group Snippets -name Blt_SwapSnippet
|||	Exchanges a snippet between BlitObjects.
|||
|||	  Synopsis
|||
|||	    void Blt_SwapSnippet(BlitObject *src, BlitObject *dest, uint32 type);
|||
|||	  Description
|||
|||	    The snippet "type" in the dest BlitObject is swapped for
|||	    snippet type in the src BlitObject.
|||
|||	  Arguments
|||
|||	    src
|||	         The source BlitObject
|||
|||	    dest
|||	         The destination BlitObject
|||
|||	    type
|||	         The snippet type to swap (see BlitObject_tags in
|||	         <graphics/blitter.h>).
|||
|||	  Implementation
|||
|||	    Folio call implemented in Blitter folio V32.
|||
|||	  Associated Files
|||
|||	    <graphics/blitter.h>
|||
|||	  See Also
|||
|||	    Blt_ReuseSnippet() Blt_CopySnippet()
|||
**/

/**
|||	AUTODOC -class Blitter -group Snippets -name Blt_RemoveSnippet
|||	Removes a snippet from a BlitObject.
|||
|||	  Synopsis
|||
|||	    void *Blt_RemoveSnippet(BlitObject *bo, uint32 type);
|||
|||	  Description
|||
|||	    The snippet "type" is removed from the BlitObject, and its
|||	    usage counter decremented by one. The BlitObject's snippet
|||	    pointer is replaced with NULL.
|||
|||	  Arguments
|||
|||	    bo
|||	         Pointer to the BlitObject.
|||
|||	    type
|||	         The snippet type to remove (see BlitObject_tags in
|||	         <graphics/blitter.h>).
|||
|||	  Return Value
|||
|||	    A pointer to the snippet that was removed.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Blitter folio V32.
|||
|||	  Associated Files
|||
|||	    <graphics/blitter.h>
|||
|||	  See Also
|||
|||	    Blt_ReuseSnippet() Blt_CopySnippet() Blt_CreateSnippet()
|||	    Blt_SetSnippet().
|||
**/

/**
|||	AUTODOC -class Blitter -group Snippets -name Blt_SetSnippet
|||	Assigns a snippet to a BlitObject
|||
|||	  Synopsis
|||
|||	    void *Blt_SetSnippet(BlitObject *bo, void *snippet);
|||
|||	  Description
|||
|||	    Assigns the snippet to the BlitObject, and returns the
|||	    pointer to the snippet being replaced in the BlitObject,
|||	    which may be NULL.
|||
|||	    The usage count of "snippet" is incremented by one, and
|||	    the usage count of the snippet being replaced, if there
|||	    is one, is decremented by one.
|||
|||	  Arguments
|||
|||	    bo
|||	         Pointer to the BlitObject.
|||
|||	    snippet
|||	         Pointer to the snippet.
|||
|||	  Return Value
|||
|||	    Pointer to the snippet being replaced, or NULL if there
|||	    was no snippet of the same type in the BlitObject.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Blitter folio V32.
|||
|||	  Associated Files
|||
|||	    <graphics/blitter.h>
|||
|||	  See Also
|||
|||	    Blt_RemoveSnippet() Blt_CreateSnippet()
|||
**/
/**
|||	AUTODOC -class Blitter -group Vertices -name Blt_CreateVertices
|||	Creates a VerticesSnippet.
|||
|||	  Synopsis
|||
|||	    Err Blt_CreateVertices(VerticesSnippet **vtx, uint32 triangles);
|||
|||	  Description
|||
|||	    Creates a VerticesSnippet, and allocates memory to store
|||	    the specified number of vertices of the specified type.
|||
|||	  Arguments
|||
|||	    vtx
|||	         Pointer to a pointer to a VerticesSnippet, which will be
|||	         set with the pointer to the VerticesSnippet created.
|||
|||	    triangles
|||	         A bit field specifying the number of vertices, the type
|||	         of vertices, and the type of fan. This bitfield is
|||	         created using the CLT macro CLA_TRIANGLE.
|||
|||	  Return Value
|||
|||	    Negative on error.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Blitter folio V32.
|||
|||	  Caveats
|||
|||	    The memory allocatef for the vertices is
|||	    uninitialised. You need to set the actual vertex values
|||	    (x, y, u, v, r, g, b, a, w) using Blt_SetVertices().
|||
|||	  Associated Files
|||
|||	    <graphics/blitter.h> <graphics/clt/cltmacros.h>
|||
|||	  See Also
|||
|||	    Blt_SetVertices() Blt_DeleteVertices()
|||
**/

/**
|||	AUTODOC -class Blitter -group Vertices -name Blt_DeleteVertices
|||	Deletes the VerticesSnippet, and all associated resources.
|||
|||	  Synopsis
|||
|||	    Err Blt_DeleteVertices(VerticesSnippet *vtx);
|||
|||	  Description
|||
|||	    Is actually a macro to call Blt_DeleteSnippet().
|||
|||	  Arguments
|||
|||	    vtx
|||	         Pointer to the VerticesSnippet to be deleted.
|||
|||	  Return Value
|||
|||	    Negative on error (such as the snippet still being used).
|||
|||	  Implementation
|||
|||	    Folio call implemented in Blitter folio V32.
|||
|||	  Associated Files
|||
|||	    <graphics/blitter.h>
|||
|||	  See Also
|||
|||	    Blt_CreateVertices() Blt_DeleteSnippet()
|||
**/

/**
|||	AUTODOC -class Blitter -group Vertices -name Blt_SetVertices
|||	Sets the vertex values in a VerticesSnippet.
|||
|||	  Synopsis
|||
|||	    Err Blt_SetVertices(VerticesSnippet *vtx, const gfloat *vertices);
|||
|||	  Description
|||
|||	    Copies the array of vertices into the VerticesSnippet.
|||
|||	    This function must be called every time you modify the
|||	    vertices in the VerticesSnippet, even if you use the
|||	    BLITVERTEX_... macros in <graphics/blitter.h> to modify
|||	    the vtx_vertices list in-line. This is to tell the
|||	    Blitter folio that the vertices may need to be reclipped
|||	    or the texture slices recalculated.
|||
|||	  Arguments
|||
|||	    vtx
|||	         Pointer to the VerticesSnippet.
|||
|||	    vertices
|||	         Pointer to an array of vertices. The array must be of
|||	         the same format as specified in the "triangles" argument
|||	         of Blt_CreateVertices(). For example, if the
|||	         VerticesSnippet was created with
|||	         CLA_TRIANGLE(1, 1, 0, 1, 1, 4), then the vertices array
|||	         must have 4 vertices, each of the type (x, y, r, g, b, a, u, v).
|||
|||	  Return Value
|||
|||	    Negative on error.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Blitter folio V32.
|||
|||	  Associated Files
|||
|||	    <graphics/blitter.h>
|||
|||	  See Also
|||
|||	    Blt_CreateVertices()
|||
**/

/**
|||	AUTODOC -class Blitter -group Vertices -name Blt_MoveVertices
|||	Moves all the BlitObjects using the VerticesSnippet.
|||
|||	  Synopsis
|||
|||	    Err Blt_MoveVertices(VerticesSnippet *vtx, gfloat dx, gfloat dy);
|||
|||	  Description
|||
|||	    Modifies all the (x, y) values in the VerticesSnippet by
|||	    (dx, dy). This includes all the vertices created for
|||	    textures that are sliced to fit into TRAM.
|||
|||	  Arguments
|||
|||	    vtx
|||	         Pointer to the VerticesSnippet.
|||
|||	    dx
|||	         Delta by which to move the vertices in the X direction.
|||
|||	    dy
|||	         Delta by which to move the vertices in the Y direction.
|||
|||	  Return Value
|||
|||	    Negative on error.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Blitter folio V32.
|||
|||	  Associated Files
|||
|||	    <graphics/blitter.h>
|||
|||	  See Also
|||
|||	    Blt_RotateVertices() Blt_TransformVertices()
|||
**/

/**
|||	AUTODOC -class Blitter -group Vertices -name Blt_TransformVertices
|||	Applies a transformation matrix to a VerticesSnippet.
|||
|||	  Synopsis
|||
|||	    Err Blt_TransformVertices(VerticesSnippet *vtx, BlitMatrix bm);
|||
|||	  Description
|||
|||	    Applies the 2d transformation matrix bm to all the vertices
|||	    in the VerticesSnippet. This includes all the vertices created for
|||	    textures that are sliced to fit into TRAM.
|||
|||	  Arguments
|||
|||	    vtx
|||	         Pointer to the VerticesSnippet.
|||
|||	    bm
|||	         The 2d transformation matrix. This is a 3x2 matrix (it is assumed that
|||	          the third column is (0, 0, 1) of the transformation).
|||
|||	  Return Value
|||
|||	    Negative on error.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Blitter folio V32.
|||
|||	  Associated Files
|||
|||	    <graphics/blitter.h>
|||
|||	  See Also
|||
|||	    Blt_RotateVertices() Blt_MoveVertices()
|||
**/

/**
|||	AUTODOC -class Blitter -group Vertices -name Blt_RotateVertices
|||	Rotates all the BlitObjects using the VerticesSnippet.
|||
|||	  Synopsis
|||
|||	    Err Blt_RotateVertices(VerticesSnippet *vtx,
|||	                           gfloat angle, gfloat x, gfloat y);
|||
|||	  Description
|||
|||	    Modifies all the (x, y) values in the VerticesSnippet by
|||	    rotating the vertices through "angle" degress, about the
|||	    point (x, y). This includes all the vertices created for
|||	    textures that are sliced to fit into TRAM.
|||
|||	  Arguments
|||
|||	    vtx
|||	         Pointer to the VerticesSnippet.
|||
|||	    angle
|||	         Angle through which to rotate the points (in degrees).
|||
|||	    x, y
|||	         Point about which to rotate the vertices.
|||
|||	  Return Value
|||
|||	    Negative on error.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Blitter folio V32.
|||
|||	  Associated Files
|||
|||	    <graphics/blitter.h>
|||
|||	  See Also
|||
|||	    Blt_MoveVertices() Blt_TransformVertices()
|||
**/

/**
|||	AUTODOC -class Blitter -group Utilities -name Blt_BlendBlitObject
|||	Sets up the BlitObject snippets for Destination Blending.
|||
|||	  Synopsis
|||
|||	    Err Blt_BlendBlitObject(BlitObject *bo,
|||	                            const gfloat src, const gfloat dest);
|||
|||	  Description
|||
|||	    Calculate the Triangle Engine instructions required to
|||	    blend so much of the src BlitObject with so much of the
|||	    destination Bitmap, and sets the BlitObject's snippets as
|||	    required.
|||
|||	  Arguments
|||
|||	    bo
|||	         Pointer to a BlitObject.
|||
|||	    src
|||	         Percentage of the source BlitObject to blend with (in
|||	         the range (0.0 - 1.0)).
|||
|||	    dest
|||	         Percentage of the destination Bitmap to blend with (in
|||	         the range (0.0 - 1.0)).
|||
|||	  Return Value
|||
|||	    Negative on error.
|||
|||	  Implementation
|||
|||	    Library call implemented in libblitter_utils.a V32.
|||
|||	  Associated Files
|||
|||	    <graphics/blitter.h> libblitter_utils.a
|||
**/

/**
|||	AUTODOC -class Blitter -group Utilities -name Blt_Scroll
|||	Sets up the BlitObject snippets to scroll a rectangular area.
|||
|||	  Synopsis
|||
|||	    Err Blt_Scroll(GState *gs, BlitObject *bo, Item bitmap,
|||	                  BlitScroll *bs);
|||
|||	  Description
|||
|||	    Calculate the Triangle Engine instructions required to
|||	    scroll a rectangular area of a bitmap. Optionally build
|||	    the instructions to fill the area that is not scrolled
|||	    with a solid color.
|||
|||	  Arguments
|||
|||	    gs
|||	        Pointer to a GState
|||
|||	    bo
|||	         Pointer to a BlitObject.
|||
|||	    bitmap
|||	         Item of the source bitmap.
|||
|||	    bs
|||	         Pointer to an initialised BlitScroll structure,
|||	         containing the region to be scrolled, the direction
|||	         and amount to scroll by, and an optional color to
|||	         replace the unscrolled region with. If bsc_dx is
|||	         negative, the region will scroll to the left. If
|||	         bsc_dy is negative, the region will scroll up.
|||
|||	  Return Value
|||
|||	    Negative on error.
|||
|||	  Implementation
|||
|||	    Library call implemented in libblitter_utils.a V32.
|||
|||	  Associated Files
|||
|||	    <graphics/blitter.h> libblitter_utils.a
|||
**/

/**
|||	AUTODOC -class Blitter -group Mask -name Blt_EnableMask
|||	Enables PixelDiscard mode for blit-thru-mask.
|||
|||	  Synopsis
|||
|||	    Err Blt_EnableMask(BlitObject *bo, uint32 discardType);
|||
|||	  Description
|||
|||	    Calculate the Triangle Engine instructions required to
|||	    discard the specified pixels, and sets the BlitObject's
|||	    snippets as required. The texture data in the
|||	    BlitObject will have been set up with the mask in
|||	    Blt_MakeMask().
|||
|||	  Arguments
|||
|||	    bo
|||	         Pointer to the BlitObject.
|||
|||	    discardType
|||	         Bit field specifying the pixel type or types to
|||	         discard. This will probably be the same value specified
|||	         in the BlitMask.blm_discardType field passed to
|||	         Blt_MakeMask().
|||
|||	  Return Value
|||
|||	    Negative on error.
|||
|||	  Implementation
|||
|||	    Library call implemented in libblitter_utils.a V32.
|||
|||	  Associated Files
|||
|||	    <graphics/blitter.h> libblitter_utils.a
|||
|||	  See Also
|||
|||	    Blt_MakeMask() Blt_DisableMask()
|||
**/

/**
|||	AUTODOC -class Blitter -group Mask -name Blt_DisableMask
|||	Disables blit-thru-mask mode.
|||
|||	  Synopsis
|||
|||	    Err Blt_DisableMask(BlitObject *bo);
|||
|||	  Description
|||
|||	    Modifies the BlitObject's snippets to disable pixel discarding.
|||
|||	  Arguments
|||
|||	    bo
|||	         Pointer to the BlitObject.
|||
|||	  Return Value
|||
|||	    Negative on error.
|||
|||	  Implementation
|||
|||	    Library call implemented in libblitter_utils.a V32.
|||
|||	  Associated Files
|||
|||	    <graphics/blitter.h> libblitter_utils.a
|||
|||	  See Also
|||
|||	    Blt_MakeMask() Blt_EnableMask()
|||
**/

/**
|||	AUTODOC -class Blitter -name OpenBlitterFolio
|||	Opens the Blitter Folio
|||
|||	  Synopsis
|||
|||	    Err OpenBlitterFolio(void);
|||
|||	  Description
|||
|||	    Opens the Blitter folio.
|||
|||	  Return Value
|||
|||	    Negative on error.
|||
|||	  See Also
|||
|||	    CloseBlitterFolio().
|||
**/
/**
|||	AUTODOC -class Blitter -name CloseBlitterFolio
|||	Closes the Blitter Folio
|||
|||	  Synopsis
|||
|||	    void CloseBlitterFolio(void);
|||
|||	  Description
|||
|||	    Closes the Blitter folio.
|||
|||	  See Also
|||
|||	    OpenBlitterFolio().
|||
**/

/**
|||	AUTODOC -class Blitter -group Blit -name Blt_RectangleInBitmap
|||	Blits a rectangular area within the src Bitmap.
|||
|||	  Synopsis
|||
|||	    Err Blt_RectangleInBitmap(GState *gs, BlitObject *bo, Item bmI, BlitRect *br);
|||
|||	  Description
|||
|||	    Blits a rectangular area of a bitmap through the
|||	    BlitObject's vertices back into the same bitmap. This
|||	    removes the need to copy the rectangular area into an
|||	    offscreen buffer with Blt_RectangleToBlitObject(),
|||	    provided that the number of bytes in the source rectangle
|||	    are fewer than the 16k TRAM limit.
|||
|||	  Arguments
|||
|||	    gs
|||	        Pointer to a GState that may be used to copy the data. A
|||	        NULL value forces the blit to be performed with the CPU.
|||
|||	    bo
|||	        Pointer to a BlitOject.
|||
|||	    bmI
|||	        Item of the source Bitmap
|||
|||	    br
|||	        Pointer to a BlitRect defining the source rectangle.
|||
|||	  Return Value
|||
|||	    Negative on error. Especially, BLITTER_ERR_TOOBIG is
|||	    returned if the source BlitRect will not fit into TRAM.
|||
|||	  Implementation
|||
|||	    Library call implemented in libblitter_utils.a V32.
|||
|||	  Associated Files
|||
|||	    <graphics/blitter.h> libblitter_utils.a
|||
|||	  See Also
|||
|||	    Blt_RectangleToBlitObject()
|||
**/

/**
|||	AUTODOC -class Blitter -group Vertices -name Blt_BlitObjectSliced
|||	Checks if the texture in a BlitObject has been sliced for TRAM limitations.
|||
|||	  Synopsis
|||
|||	    bool Blt_BlitObjectSliced(BlitObject *bo);
|||
|||	  Description
|||
|||	    If the texture associated with a BlitObject is greater
|||	    than TRAM_SIZE, then the Blitter folio will slice the
|||	    texture into regions that do fit in TRAM, and calculate
|||	    new vertices to render the shape specified in the
|||	    VerticesSnippet. This function tells you if this
|||	    BlitObject has been sliced.
|||
|||	  Arguments
|||
|||	    bo
|||	        Pointer to a BlitObject
|||
|||	  Return Value
|||
|||	    TRUE if the BlitObject has been sliced.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Blitter folio V32.
|||
|||	  Associated Files
|||
|||	    <graphics/blitter.h>
|||
|||	  See Also
|||
|||	    Blt_SetVertices()
|||
**/

/**
|||	AUTODOC -class Blitter -group Vertices -name Blt_BlitObjectClipped
|||	Checks if the vertices in a BlitObject has been clipped.
|||
|||	  Synopsis
|||
|||	    bool Blt_BlitObjectClipped(BlitObject *bo);
|||
|||	  Description
|||
|||	    If clipping has been enabled for a BlitObject with
|||	    Blt_SetClipBox(), then this function will tell you whether
|||	    the BlitObject has actually been clipped.
|||
|||	    The result is only valid after Blt_BlitObjectToBitmap()
|||	    has been called.
|||
|||	  Arguments
|||
|||	    bo
|||	        Pointer to a BlitObject
|||
|||	  Return Value
|||
|||	    TRUE if the BlitObject has been clipped.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Blitter folio V32.
|||
|||	  Associated Files
|||
|||	    <graphics/blitter.h>
|||
|||	  See Also
|||
|||	    Blt_SetClipBox()
|||
**/

/**
|||	AUTODOC -class Blitter -group Vertices -name Blt_SetClipBox
|||	Sets the clipping region of a BlitObject.
|||
|||	  Synopsis
|||
|||	    Err Blt_SetClipBox(BlitObject *bo, bool set, bool clipOut, Point2 *tl, Point2 *br);
|||
|||	  Description
|||
|||	    Enables or disables clipping for a BlitObject. If enabled,
|||	    then every time the vertices are modified (either by the
|||	    application with Blt_SetVertices() or through one of the
|||	    vertex transformation functions), the vertices will be
|||	    clipped to the boundaries specified.
|||
|||	    Unclipped vertices will still be rendered correctly
|||	    (though maybe less efficiently) if they fall off the right
|||	    hand side or bottom of the destination bitmap. However,
|||	    vertices with negative values will cause incorrect
|||	    triangles to be rendered unless clipped.
|||
|||	    Vertices can be clipped away either inside the clipping
|||	    region, or outside the clipping region. When clipping
|||	    outside is enabled, negative vertices will still be
|||	    clipped to 0.
|||
|||	    Note that clipping the vertices is relatively CPU
|||	    intensive. For better performance, clipping should not be
|||	    enabled (which is the default action) if you know that the
|||	    BlitObject will never be rendered outside of the bounds of
|||	    the destination bitmap.
|||
|||	  Arguments
|||
|||	    bo
|||	        Pointer to the BlitObject.
|||
|||	    set
|||	        TRUE to enable clipping for this BlitObject, FALSE to
|||	        disable clipping.
|||
|||	    clipOut
|||	        TRUE to clip pixels outside of the clipping region,
|||	        FALSE to clip pixels inside the clipping region.
|||
|||	    tl
|||	        Pointer to the (x, y) coordinate of the TopLeft
|||	        corner of the clipping region.
|||
|||	    br
|||	        Pointer to the (x, y) coordinate of the BottomRight
|||	        corner of the clipping region.
|||
|||	  Return Value
|||
|||	    Negative on error.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Blitter folio V32.
|||
|||	  Associated Files
|||
|||	    <graphics/blitter.h>
|||
|||	  See Also
|||
|||	    Blt_BlitObjectClipped()
|||
**/

/**
|||	AUTODOC -class Blitter -group Blit -name Blt_SetTexture
|||	Changes the source texture of a BlitObject.
|||
|||	  Synopsis
|||
|||	    Err Blt_SetTexture(BlitObject *bo, void *texelData);
|||
|||	  Description
|||
|||	    Changes the texelData of the
|||	    BltTxData.btd_txData.texelData to point to the new texture
|||	    source. This new source must be of the same dimension and
|||	    texel type as the original.
|||
|||	    This function also modifes the TxLoadSnippet.txl_src value
|||	    of the BlitObject, and all the TXTLDSRCADDR instructions
|||	    in the list of sliced vertices should the original source
|||	    texture have been larger than TRAM_SIZE.
|||
|||	  Arguments
|||
|||	    bo
|||	        Pointer to a BlitObject.
|||
|||	    texelData
|||	        Pointer to the new texelData.
|||
|||	  Return Value
|||
|||	    Negative on error.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Blitter folio V32.
|||
|||	  Associated Files
|||
|||	    <graphics/blitter.h>
|||
|||	  See Also
|||
|||	    Blt_GetTexture()
|||
**/

/**
|||	AUTODOC -class Blitter -group Blit -name Blt_GetTexture
|||	Returns a pointer to the BlitObject's current texture.
|||
|||	  Synopsis
|||
|||	    Err Blt_GetTexture(BlitObject *bo, void **texelData);
|||
|||	  Description
|||
|||	    Returns a pointer to the BlitObject's current texture.
|||
|||	  Arguments
|||
|||	    bo
|||	        Pointer to a BlitObject
|||
|||	    texelData
|||	        Pointer to a pointer which will be filled with the
|||	        address of the current texelData.
|||
|||	  Return Value
|||
|||	    Negative on error. *texelData will point to the current texeldata.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Blitter folio V32.
|||
|||	  Associated Files
|||
|||	    <graphics/blitter.h>
|||
|||	  See Also
|||
|||	    Blt_SetTexture()
|||
**/

/**
|||	AUTODOC -class Blitter -group Utilities -name Blt_LoadUTF
|||	Creates a BlitObject from a UTF file.
|||
|||	  Synopsis
|||
|||	    Err Blt_LoadUTF(BlitObject **bo, const char *fname);
|||
|||	  Description
|||
|||	    Loads and parses a UTF file, and creates a BlitObject to
|||	    render the image loaded.
|||
|||	  Arguments
|||
|||	    bo
|||	         Pointer to a pointer to a BlitObject. This will be
|||	         filled with the pointer to the newly created BlitObject.
|||
|||	    fname
|||	         Pointer to the filename of the UTF file.
|||	        
|||	  Return Value
|||
|||	    Negative on error. bo will point to the new BlitObject.
|||
|||	  Implementation
|||
|||	    Library call implemented in libblitter_utils.a V32.
|||
|||	  Associated Files
|||
|||	    <graphics/blitter.h>
|||
|||	  See Also
|||
|||	    Blt_FreeUTF()
|||
**/

/**
|||	AUTODOC -class Blitter -group Utilities -name Blt_FreeUTF
|||	Frees the resources allocated with Blt_LoadUTF().
|||
|||	  Synopsis
|||
|||	    Err Blt_FreeUTF(BlitObject *bo);
|||
|||	  Description
|||
|||	    Frees all the resources allocated by Blt_LoadUTF(). You
|||	    must call this if using Blt_LoadUTF().
|||
|||	  Arguments
|||
|||	    bo
|||	        Pointer to the BlitObject to free.
|||
|||	  Return Value
|||
|||	    Negative on error.
|||
|||	  Implementation
|||
|||	    Library call implemented in libblitter_utils.a V32.
|||
|||	  Associated Files
|||
|||	    <graphics/blitter.h>
|||
|||	  See Also
|||
|||	    Blt_LoadUTF()
|||
**/

/**
|||	AUTODOC -class Blitter -group Utilities -name Blt_LoadTexture
|||	Loads one or more textures from a UTF file
|||
|||	  Synopsis
|||
|||	    Err Blt_LoadTexture(List *blitObjectList, const TagArg *tags);
|||	    Err Blt_LoadTextureVA(List *blitObjectList, uint32 tag, ... );
|||
|||	  Description
|||
|||	    This function parses and attempts to load all of the textures
|||	    contained in the given UTF file.  As each texture is encountered,
|||	    a BlitObjectNode containing a BlitObject is allocated, associated with  
|||	    the texture, and added to the list blitObjectList.
|||
|||	  Arguments
|||
|||	    blitObjectList
|||	        A pointer to a List structure, to be initialized and used by
|||	        Blt_LoadTexture() to return all of the BlitObjectNodes loaded.
|||
|||	    tags
|||	        A pointer to an array of tag arguments containing extra data
|||	        for this function.  See below for a description of the tags
|||	        supported.
|||
|||
|||	  Tag Arguments
|||
|||	    The following tag arguments may be supplied in array form to this
|||	    function.  The array must be terminated with TAG_END.
|||
|||	    LOADTEXTURE_TAG_FILENAME (const char *)
|||	        This tag defines the name of the UTF file to load textures
|||	        from.  This tag is mutually exclusive with
|||	        LOADTEXTURE_TAG_IFFPARSER.
|||
|||	    LOADTEXTURE_TAG_IFFPARSER (IFFParser *)
|||	        This tag permits the caller to utilize an already existing
|||	        IFFParser structure from the IFF folio.  This is useful for
|||	        embedding texture data in other IFF files.  This tag is
|||	        mutually exclusive with LOADTEXTURE_TAG_FILENAME, and requires
|||	        the the presence of LOADTEXTURE_TAG_IFFPARSETYPE to be functional.
|||
|||	    LOADTEXTURE_TAG_IFFPARSETYPE (uint32)
|||	        This tag defines options for IFF parsing, and is only valid
|||	        when present with the LOADTEXTURE_TAG_IFFPARSER tag.  The
|||	        following options are available, and can be OR'd together:
|||
|||	        LOADTEXTURE_TYPE_AUTOPARSE
|||	        Requests that BLt_LoadTexture() parse for either a
|||	        FORM TXTR or LIST TXTR header.
|||
|||	        LOADTEXTURE_TYPE_SINGLE
|||	        Indicates that the caller has already parsed up to a
|||	        FORM TXTR, denoting this stream as a single-texture UTF file.
|||
|||	        LOADTEXTURE_TYPE_MULTIPLE
|||	        Indicates that the caller has already parsed up to a
|||	        LIST TXTR, denoting this stream as a multiple-texture UTF file.
|||
|||	    LOADTEXTURE_TAG_CALLBACK (LTCallBack)
|||	        This tag provides the caller with the ability to selectively
|||	        accept or deny loading of textures based on their order in
|||	        the source data file.  Providing the address of the callback
|||	        routine in ta_Arg, and also providing the tag
|||	        LOADTEXTURE_TAG_CALLBACKDATA, whenever a new texture is
|||	        encountered, a callback will be made with the number of the
|||	        encountered texture (0 ... n) and the user data provided by
|||	        LOADTEXTURE_TAG_CALLBACKDATA.  The callback routine itself
|||	        must return LOADTEXTURE_OK if it wishes that texture to be
|||	        loaded, LOADTEXTURE_SKIP if it would like to skip loading
|||	        the current texture, or LOADTEXTURE_STOP if it would like to
|||	        cease parsing the IFF file at the current point.
|||
|||	    LOADTEXTURE_TAG_CALLBACKDATA
|||	        This tag, valid only when presented with the tag
|||	        LOADTEXTURE_TAG_CALLBACK, defines user private callback data
|||	        to be provided to a callback routine when new textures are
|||	        about to be loaded.
|||
|||	  Return Value
|||
|||	    >= 0 for success, or a negative number for an error code.
|||
|||	  Implementation
|||
|||	    Library call implemented in libblitter_utils.a V33.
|||
|||	  Associated Files
|||
|||	    <graphics/blitter.h> <graphics/frame2d/loadtxtr.h>
|||
|||	  See Also
|||
|||	    Blt_FreeTexture()
|||
**/

/**
|||	AUTODOC -class Blitter -group Utilities -name Blt_FreeTexture
|||	Frees the resources allocated with Blt_LoadTexture().
|||
|||	  Synopsis
|||
|||	    Err Blt_FreeTexture(List *blitObjectList);
|||
|||	  Description
|||
|||	    Frees all the resources allocated by Blt_LoadTexture(). You
|||	    must call this if using Blt_LoadTexture().
|||
|||	  Arguments
|||
|||	    blitObjectList
|||	        Pointer to the list of BlitObjectNodes to free.
|||
|||	  Return Value
|||
|||	    Negative on error.
|||
|||	  Implementation
|||
|||	    Library call implemented in libblitter_utils.a V33.
|||
|||	  Associated Files
|||
|||	    <graphics/blitter.h>
|||
|||	  See Also
|||
|||	    Blt_LoadTexture()
|||
**/

/**
|||	AUTODOC -class Blitter -group Utilities -name Blt_InitDimensions
|||	Initialises the dimensions of a blit operation
|||
|||	  Synopsis
|||
|||	    void Blt_InitDimensions(BlitObject *bo, uint32 width, uint32 height,
|||	                                     uint32 bitsPerPixel);
|||
|||	  Description
|||
|||	    Takes the texel data defined in the CltTxData of a
|||	    BlitObject, and sets all the elements of the BlitObject's
|||	    snippets to perform a blit operation with the texture.
|||
|||	    This function will use the CltTxData->expansionFormat
|||	    field, and the contents of CltTxData->dci if non-NULL.
|||
|||	  Arguments
|||
|||	    bo
|||	        Pointer to the BlitObject
|||
|||	    width
|||	        Width, in pixels, of the texture.
|||
|||	    height
|||	        Height, in pixels, of the texture.
|||
|||	    bitsPerPixel
|||	        Number of bits to define each pixel of the texture.
|||
|||	  Implementation
|||
|||	    Library call implemented in libblitter_utils.a V32.
|||
|||	  Associated Files
|||
|||	    <graphics/blitter.h>
|||
**/
/**
|||	AUTODOC -class Blitter -group Vertices -name Blt_MoveUV
|||	Changes the (u, v) coordinates in a list of vertices.
|||
|||	  Synopsis
|||
|||	    Err Blt_MoveUV(VerticesSnippet *vtx, gfloat du, gfloat dv);
|||
|||	  Description
|||
|||	    Modifies all the (u, v) values in the VerticesSnippet by
|||	    (du, dv). This includes all the vertices created for
|||	    textures that are sliced to fit into TRAM.
|||
|||	    Functionally, this call is equivalent to modifying the
|||	    vertex list and calling Blt_SetVertices(), except that
|||	    Blt_SetVertices() will force the texture to be
|||	    resliced.
|||
|||	  Arguments
|||
|||	    vtx
|||	         Pointer to the VerticesSnippet.
|||
|||	    du
|||	         Delta by which to move the texture vertices in the U direction.
|||
|||	    dy
|||	         Delta by which to move the texture vertices in the V direction.
|||
|||	  Return Value
|||
|||	    Negative on error.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Blitter folio V33.
|||
|||	  Associated Files
|||
|||	    <graphics/blitter.h>
|||
|||	  See Also
|||
|||	    Blt_SetVertices()
|||
**/

extern int foo; /* just to make the compiler happy */
