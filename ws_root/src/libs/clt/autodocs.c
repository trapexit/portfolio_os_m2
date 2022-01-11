/* @(#) autodocs.c 96/12/05 1.8 */

/**
|||	AUTODOC -class CLT -name CLT_CreatePipCommandList
|||	Creates a command list snippet to load a PIP.
|||
|||	  Synopsis
|||
|||	    Err CLT_CreatePipCommandList(CltPipControlBlock* txPipCB);
|||
|||	  Description
|||
|||	    Creates a command list snippet to load the specified block of memory
|||	    as a PIP, or Pen Index Palette, for indexed textures to use.  The
|||	    fields of the CltPipControlBlock should be filled in as follows:
|||
|||	    pipData
|||	        Pointer to the PIP data in RAM
|||
|||	    pipIndex
|||	        Starting index of the data to load
|||
|||	    pipNumEntries
|||	        Number of entries in the data to be loaded
|||
|||	  Arguments
|||
|||	    txPipCB
|||	        Pointer to a CltPipControlBlock structure.  On entry, txPipCB
|||	        should be filled in as described above.  On exit, the field
|||	        pipCommandList will be filled in with a command list snippet that
|||	        can be copied into a command list buffer when the PIP load needs
|||	        to occur.  See CLT_CopySnippetData() for more details.
|||
|||	  Return Value
|||
|||	    Returns 0 for success or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Library routine in libclt.a
|||
|||	  See Also
|||
|||	    CLT_CopySnippetData(), CLT_ComputePipLoadCmdListSize()
|||
**/

/**
|||	AUTODOC -class CLT -name CLT_ComputePipLoadCmdListSize
|||	Computes the size of a PIP load command list.
|||
|||	  Synopsis
|||
|||	    int32 CLT_ComputePipLoadCmdListSize(CltPipControlBLock* txPipCB);
|||
|||	  Description
|||
|||	    Computes the size of a PIP load command list snippet, without actually
|||	    creating the snippet.  This routine is most useful when users want to
|||	    have a PIP load command list be created directly into the command list
|||	    buffer, and need to know how much space will be needed ahead of time
|||	    to prevent overflowing the command list buffer.
|||
|||	  Arguments
|||
|||	    txPipCB
|||	        Pointer to a CltPipControlBlock structure.  See
|||	        CLT_CreatePipCommandList() for information on how to correctly
|||	        fill in this structure.
|||
|||	  Return Value
|||
|||	    Returns the necessary size, in words, if successful, or a negative
|||	    error code if the call fails.
|||
|||	  Implementation
|||
|||	    Library call in libclt.a
|||
|||	  See Also
|||
|||	    CLT_CreatePipCommandList(), GS_Reserve()
|||
**/

/**
|||	AUTODOC -class CLT -name CLT_CreateTxLoadCommandList
|||	Creates a command list snippet to load a texture.
|||
|||	  Synopsis
|||
|||	    Err CLT_CreateTxLoadCommandList(CltTxLoadControlBlock* txLoadCB);
|||
|||	  Description
|||
|||	    Creates a command list snippet to load the specified texture into
|||	    the Triangle Engine's TRAM, or Texture RAM.  For uncompressed textures
|||	    which do not require runtime carving, the data for the one or more
|||	    levels of detail is transferred as one large block of contiguous RAM,
|||	    for optimal performance.  For indexed textures, the PIP must be loaded
|||	    separately, by calling CLT_CreatePipCommandList().  The fields of
|||	    the txLoadCB field should be filled in as follows:
|||
|||	    textureBlock
|||	        Pointer to a CltTxData block.  This block contains data about the
|||	        actual texels, such as num LODs, texture format, etc.
|||
|||	    numLOD
|||	        Number of LODs (Levels of Detail) in texture
|||
|||	    XWrap
|||	        Wrap mode in X-direction (0=Clamp, 1=Tile)
|||
|||	    YWrap
|||	        Wrap mode in Y-direction (0=Clamp, 1=Tile)
|||
|||	    XSize
|||	        Width of area to load.  Normally equal to the minX field of the
|||	        textureBlock structure
|||
|||	    YSize
|||	        Height of area to load.  Normally equal to the minY field of the
|||	        textureBlock structure
|||
|||	    XOffset
|||	        Left edge of sub-texture to load.  For non-carved
|||	        textures, this will normally be 0.
|||
|||	    YOffset
|||	        Top edge of sub-texture to load.  For non-carved textures, this
|||	        will normally be 0.
|||
|||	    tramOffset
|||	        Offset into the 16k TRAM where the texture should be placed.
|||	        For uncompressed, uncarved textures, this value must be
|||	        32-bit aligned.
|||
|||	    tramSize
|||	        Size texture will require in TRAM.
|||
|||	  Arguments
|||
|||	    txLoadCB
|||	        Pointer to a CltTxLoadControlBlock structure.  On entry, txLoadCB
|||	        must be filled in as described above.  On exit, the field
|||	        lcbCommandList will contain a command list snippet to load and
|||	        use the specified texture.  Additionally, the useCommandList can
|||	        be used to just re-use the texture, assuming that it is already
|||	        loaded.  The useCommandList is most useful when multiple textures
|||	        reside in the TRAM at once, and the user data needs to alternate
|||	         between these different textures.
|||
|||	  Return Value
|||
|||	    Returns 0 for success or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Library routine in libclt.a
|||
|||	  See Also
|||
|||	    CLT_CopySnippetData(), CLT_ComputeTxLoadCmdListSize(),
|||	    CLT_CreatePipCommandList()
|||
**/

/**
|||	AUTODOC -class CLT -name CLT_ComputeTxLoadCmdListSize
|||	Computes the size of a texture load command list.
|||
|||	  Synopsis
|||
|||	    int32 CLT_ComputeTxLoadCmdListSize(CltTxLoadControlBlock* txLoadCB);
|||
|||	  Description
|||
|||	    Computes the size of a texture load command list snippet, without
|||	    actually creating the snippet.  This routine is most useful when users
|||	    want to have a texture load command list be created directly into the
|||	    command list buffer, and need to know how much space will be needed
|||	    ahead of time to prevent overflowing the command list buffer.
|||
|||	  Arguments
|||
|||	    txLoadCB
|||	        Pointer to a CltTxLoadControlBlock structure.  See
|||	        CLT_CreateTxLoadCommandList() for information on how to correctly
|||	        fill in this structure.
|||
|||	  Return Value
|||
|||	    Returns the necessary size, in words, if successful, or a negative
|||	    error code if the call fails.
|||
|||	  Implementation
|||
|||	    Library call in libclt.a
|||
|||	  See Also
|||
|||	    CLT_CreateTxLoadCommandList(), GS_Reserve()
**/

/**
|||	AUTODOC -class CLT -name CLT_InitSnippet
|||	Initializes a CltSnippet structure.
|||
|||	  Synopsis
|||
|||	    void CLT_InitSnippet(CltSnippet* s);
|||
|||	  Description
|||
|||	    Initializes a CltSnippet structure.  The structure must be created
|||	    before calling this routine, either by calling AllocMem() or by just
|||	    declaring a variable of type CltSnippet.  Once initialized, a
|||	    CltSnippet can be used in many other CLT calls.
|||
|||	    Note that initializing a CltSnippet does not allocate any memory for
|||	    the snippet's data space.  To allocate that memory, call
|||	    CLT_AllocSnippet() after calling CLT_InitSnippet().
|||
|||	  Arguments
|||
|||	    s
|||	        Pointer to a valid CltSnippet
|||
|||	  Implementation
|||
|||	    Library call in libclt.a
|||
|||	  See Also
|||
|||	    CLT_AllocSnippet(), CLT_CopySnippet(), CLT_CopySnippetData(),
|||	    CLT_FreeSnippet(), AllocMem()
**/

/**
|||	AUTODOC -class CLT -name CLT_AllocSnippet
|||	Allocates space for the data for a CltSnippet.
|||
|||	  Synopsis
|||
|||	    Err CLT_AllocSnippet(CltSnippet* s, uint32 nWords);
|||
|||	  Description
|||
|||	    Allocate space for the specified number of words of command list
|||	    data for a given CltSnippet.  The CltSnippet should already have been
|||	    initialized by calling CLT_InitSnippet().
|||
|||	  Arguments
|||
|||	    s
|||	        Pointer to a valid, initialized CltSnippet
|||
|||	    nWords
|||	        Number of words of memory to be allocated for the CltSnippet
|||
|||	  Return Value
|||
|||	    Returns 0 for success or a negative error code for failure.
|||
|||	  Caveats
|||
|||	    If the CltSnippet already has memory allocated for its data, this
|||	    memory will be left dangling in memory.  Use CLT_FreeSnippet() to
|||	    free the memory first.
|||
|||	  Implementation
|||
|||	    Library call in libclt.a
|||
|||	  See Also
|||
|||	    CLT_FreeSnippet(), CLT_InitSnippet()
**/

/**
|||	AUTODOC -class CLT -name CLT_FreeSnippet
|||	Frees memory allocated for a CltSnippet's data.
|||
|||	  Synopsis
|||
|||	    void CLT_FreeSnippet(CltSnippet* s);
|||
|||	  Description
|||
|||	    Frees the memory allocated to a CltSnippet's data area.  The memory
|||	    is only freed if it was allocated for the snippet.  That is, there
|||	    are times when several snippets may all share the same data area, such
|||	    as when one is only supposed to represent a smaller portion of another.
|||	    In that case, calling CLT_FreeSnippet() on the smaller sub-snippet will
|||	    not cause the smaller portion of memory to be freed from the larger
|||	    memory block.
|||
|||	  Arguments
|||
|||	    s
|||	        Pointer to a valid, initialized CltSnippet
|||
|||	  Caveats
|||
|||	    In the case where a snippet is using memory from a larger snippet,
|||	    and CLT_FreeSnippet() is called to free the memory used by the larger
|||	    snippet, there is no way to mark the smaller snippet that is sharing
|||	    the memory as freed.  This could result in a CltSnippet pointing to
|||	    garbage memory.
|||
|||	  Implementation
|||
|||	    Library call implemented in libclt.a
|||
|||	  See Also
|||
|||	    CLT_AllocSnippet()
**/

/**
|||	AUTODOC -class CLT -name CLT_CopySnippetData
|||	Copies the data portion of a command list snippet to a specified
|||	location.
|||
|||	  Synopsis
|||
|||	    void CLT_CopySnippetData(uint32** dest, CltSnippet* src)
|||
|||	  Description
|||
|||	    Copies data from a CltSnippet to another memory location, updating
|||	    the dest ptr as it goes.  Most often, this routine is used to copy
|||	    command list snippets into a command list buffer within the GState.
|||
|||	  Arguments
|||
|||	    dest
|||	        Pointer to a pointer to a buffer of 32-bit quantities.  On
|||	        exit from this routine, *dest will be updated to point to just
|||	        beyond the copied data area, so that a subsequent call to
|||	        CLT_CopySnippetData() can be made without updating the pointer
|||	        manually.
|||
|||	    src
|||	        Pointer to a valid, initialized CltSnippet
|||
|||	  Implementation
|||
|||	    Library call in libclt.a
|||
|||	  See Also
|||
|||	    GS_Reserve(), GS_SendList(), CLT_InitSnippet()
**/
/**
|||	AUTODOC -class CLT -name CLT_CopySnippet
|||	Copies the data area of a command list snippet into the data area
|||	of another snippet.
|||
|||	  Synopsis
|||
|||	    void CLT_CopySnippet(CltSnippet *dest, CltSnippet* src)
|||
|||	  Description
|||
|||	    Copies data from one CltSnippet to another CltSnippet. No checks are made for the
|||	    case where the destination CltSnippet cannot hold the source data.
|||
|||	  Arguments
|||
|||	    dest
|||	        Pointer to a valid, initialized CltSnippet which will hold the destination
|||	        data. 
|||
|||	    src
|||	        Pointer to a valid, initialized CltSnippet holding the source data.
|||
|||	  Implementation
|||
|||	    Library call in libclt.a
|||
|||	  See Also
|||
|||	    CLT_CopySnippetData()
**/

/**
|||	AUTODOC -class CLT -name CLT_SetSrcToCurrentDest
|||	Sets the destination blender so that source blends will occur with
|||	the current frame buffer.
|||
|||	  Synopsis
|||
|||	    void CLT_SetSrcToCurrentDest(GState* g);
|||
|||	  Description
|||
|||	    This routine adds some commands to the current command list buffer
|||	    of a GState which set the destination blend source buffer equal to
|||	    the current destination frame buffer.  This is usually most useful
|||	    when trying to make objects appear translucent.  To achieve a
|||	    translucent effect, an object gets blended with the current frame
|||	    buffer in the destination blender of the Triangle Engine.
|||
|||	    Note that this routine does not actually enable source blending, nor
|||	    does it set up the blend operation.  It merely sets the following
|||	    CLT attributes: DBSRCBASEADDR, DBSRCXSTRIDE, DBSRCOFFSET, DBSRCCNTL.
|||
|||	  Arguments
|||
|||	    g
|||	        Pointer to a GState object
|||
|||	  Implementation
|||
|||	    Library call in libclt.a
|||
|||	  See Also
|||
|||	    GS_Create(), GS_GetDestBuffer()
**/

/**
|||	AUTODOC -class CLT -name CLT_ClearFrameBuffer
|||	Clears the frame buffer and/or Z-buffer via CLT commands.
|||
|||	  Synopsis
|||
|||	    void CLT_ClearFrameBuffer(GState* gs, float red, float green,
|||	             float blue, float alpha, bool clearScreen, bool clearZ);
|||
|||	  Description
|||
|||	    Clear the frame buffer and/or Z-buffer via the Triangle Engine.
|||	    This is done by drawing two large triangles of the specified
|||	    color.  If clearing of the Z-buffer is desired, 0 is written to the
|||	    whole Z-buffer as well.  Note that because the screen clear is done
|||	    via Triangle Engine commands, the clear doesn't actually occur until
|||	    the command list is sent to the Triangle Engine.
|||
|||
|||	  Arguments
|||
|||	    gs
|||	        Pointer to a GState object
|||
|||	    red, green, blue, alpha
|||	        Floating point numbers between 0.0 and 1.0, inclusive, which
|||	        specify the intensities of the R, G, B, and alpha channels,
|||	        respectively.
|||
|||	    clearScreen
|||	        Boolean specifying whether the screen should be cleared. This
|||	        would be FALSE when the user only wants to clear the Z-buffer.
|||
|||	    clearZ
|||	        Boolean specifying whether the Z-buffer should be cleared.
|||	        Note that this value is ignored when a Z-buffer is not being
|||	        used.  Also, when a Z-buffer is being used, this routine will
|||	        NOT enable Z-buffering if clearZ is set to TRUE.  It is the
|||	        responsibility of the user's code to enable Z-buffering before
|||	        calling this routine.  This is done because normally, the
|||	        Z-buffer would just be enabled once when the application
|||	        first starts up, whereas clearing the Z-buffer would be done
|||	        once per frame.
|||
|||	  Implementation
|||
|||	    Library call in libclt.a
|||
|||	  See Also
|||
|||	    GS_Create(), GS_SendList()
**/

/**
|||	AUTODOC -class CLT -group Globals -name CltNoTextureSnippet
|||	Global variable used to disable texturing.
|||
|||	  Description
|||
|||	    The CltSnippet CltNoTextureSnippet(@) can be inserted by the user into
|||	    the current command list buffer when texturing needs to be disabled.
|||	    This CltSnippet contains the following commands:
|||	        1.  Sync the Triangle Engine
|||	        2.  Disable the TEXTUREENABLE bit of the TXTADDRCNTL register
|||	        3.  Set the TXTTABCNTL COLOROUT to PRIMCOLOR, and the ALPHAOUT
|||	            to PRIMALPHA
|||
|||	    To actually use this CltSnippet, call CLT_CopySnippetData().
|||
|||	  Implementation
|||
|||	    Global variable exported in library libclt.a
|||
|||	  See Also
|||
|||	    CltEnableTextureSnippet(@), CLT_CopySnippetData()
**/

/**
|||	AUTODOC -class CLT -group Globals -name CltEnableTextureSnippet
|||	Global variable used to enable texturing.
|||
|||	  Description
|||
|||	    The CltSnippet CltEnableTextureSnippet(@) can be inserted by the user
|||	    into the current command list buffer when texturing needs to be
|||	    enabled.  This CltSnippet contains the following commands:
|||	        1.  Enable the TEXTUREENABLE bit of the TXTADDRCNTL register.
|||
|||	    To actually use this CltSnippet, call CLT_CopySnippetData().
|||
|||	    Note that if the CltSnippet CltNoTextureSnippet(@) is used to disable
|||	    texturing, using CltEnableTextureSnippet(@) will not completely restore
|||	    state.  It will also be necessary for the user's code to set the
|||	    COLOROUT and ALPHAOUT fields of the TXTTABCNTL register, to either
|||	    output the texture's colors, or to a blend operation between the
|||	    primitive's color and the texture's color.
|||
|||	  Implementation
|||
|||	    Global variable exported in library libclt.a
|||
|||	  See Also
|||
|||	    CltNoTextureSnippet(@), CLT_CopySnippetData()
**/

/**
|||	AUTODOC -class CLT -name CLT_GetSize
|||	Returns the size of a CLT Snippet
|||
|||	  Synopsis
|||
|||	    CLT_GetSize(CltSnippet *snip);
|||
|||	  Description
|||
|||	    This macro returns the size, in words, of the command list stored
|||	    in the specified CLT Snippet.
|||
|||	  Return Value
|||
|||	    Returns the size of the snippet in words.
|||
|||	  Implementation
|||
|||	    Macro found in clt.h
|||
**/

/**
|||	AUTODOC -class CLT -name CLT_GetData
|||	Returns a pointer to the command list stored in a CLT Snippet
|||
|||	  Synopsis
|||
|||	    CLT_GetSize(CltSnippet *snip);
|||
|||	  Description
|||
|||	    This macro returns a pointer to the beginning of the command list
|||	    stored in the specified CLT Snippet.
|||
|||	  Return Value
|||
|||	    Returns a pointer to the command list,
|||	    or NULL if there currently is none
|||
|||	  Implementation
|||
|||	    Macro found in clt.h
|||
**/

/**
|||	AUTODOC -class CLT -name CLT_SetTxAttribute
|||	Sets texture-related values in a command list
|||
|||	  Synopsis
|||
|||	    CLT_SetTxAttribute(CltSnippet *textureAttrList,
|||	                       CltTxAttribute attrTag, uint32 attrValue);
|||
|||	  Description
|||
|||	    If "textureAttrList" is NULL, a new command list is allocated
|||	    and the register fields corresponding to attrTag are set to
|||	    attrValue. The start address of the command list is returned
|||	    in "textureAttrList".
|||
|||	    If "textureAttrList" is not NULL, the register fields corresponding
|||	    to attrTag are set to attrValue.
|||
|||	    Valid values for attrTag can be found in clttxdblend.h.
|||
|||	  Return Value
|||
|||	    0 is returned if there are no errors.
|||	    negative number is returned on error.
|||
|||	  Implementation
|||
|||	    Folio call in the CLT
|||
**/

/**
|||	AUTODOC -class CLT -name CLT_GetTxAttribute
|||	Gets a texture-related value from a texture-attribute-related
|||	command list
|||
|||	  Synopsis
|||
|||	    CLT_GetTxAttribute(CltSnippet *textureAttrList,
|||	                       CltTxAttribute attrTag, uint32 *attrValue);
|||
|||	  Description
|||
|||	    Finds the value in textureAttrList corresponding to the register
|||	    field specified in attrTag, and returns that value in attrValue.
|||
|||	    Valid values for attrTag can be found in clttxdblend.h.
|||
|||	  Return Value
|||
|||	    0 is returned if there are no errors.
|||	    negative number is returned on error.
|||
|||	  Implementation
|||
|||	    Folio call in the CLT
|||
**/

/**
|||	AUTODOC -class CLT -name CLT_IgnoreTxAttribute
|||	Sets a command list not to affect a certain texture-related register
|||
|||	  Synopsis
|||
|||	    Err CLT_IgnoreTxAttribute(CltSnippet *textureAttrList,
|||	                              CltTxAttribute attrTag);
|||
|||	  Description
|||
|||	    Sets textureAttrList so that it does not modify the register
|||	    field specified in attrTag.
|||
|||	    Valid values for attrTag can be found in clttxdblend.h.
|||
|||	  Return Value
|||
|||	    0 is returned if there are no errors.
|||	    negative number is returned on error.
|||
|||	  Implementation
|||
|||	    Folio call in the CLT
|||
**/

/**
|||	AUTODOC -class CLT -name CLT_SetDblAttribute
|||	Sets destination-blending-related values in a command list
|||
|||	  Synopsis
|||
|||	    CLT_SetDblAttribute(CltSnippet *dblendAttrList,
|||	                        CltDblAttribute attrTag, uint32 attrValue);
|||
|||	  Description
|||
|||	    If dblendAttrList is NULL, a new command list is allocated and
|||	    the register fields corresponding to attrTag are set to attrValue.
|||	    The start address of the command list is returned in dblendAttrList.
|||
|||	    If dblendAttrList is not NULL, the register fields corresponding
|||	    to attrTag are set to attrValue.
|||
|||	    Valid values for attrTag can be found in clttxdblend.h.
|||
|||	  Return Value
|||
|||	    0 is returned if there are no errors.
|||	    negative number is returned on error.
|||
|||	  Implementation
|||
|||	    Folio call in the CLT
|||
**/

/**
|||	AUTODOC -class CLT -name CLT_GetDblAttribute
|||	Gets a specific register value from a destination blending command list
|||
|||	  Synopsis
|||
|||	    CLT_GetDblAttribute(CltSnippet *dblendAttrList,
|||	                        CltDblAttribute attrTag, uint32 *attrValue);
|||
|||	  Description
|||
|||	    Finds the value in dblendAttrList corresponding to the register
|||	    field specified in attrTag, and returns that value in attrValue.
|||
|||	    Valid values for attrTag can be found in clttxdblend.h.
|||
|||	  Return Value
|||
|||	    0 is returned if there are no errors.
|||	    negative number is returned on error.
|||
|||	  Implementation
|||
|||	    Folio call in the CLT
|||
**/

/**
|||	AUTODOC -class CLT -name CLT_IgnoreDblAttribute
|||	Sets a command list not to affect a certain destination blending register
|||
|||	  Synopsis
|||
|||	    Err CLT_IgnoreDblAttribute(CltSnippet *dblendAttrList,
|||	                               CltDblAttribute attrTag);
|||
|||	  Description
|||
|||	    Sets dblendAttrList so that it does not modify the register
|||	    field specified in attrTag.
|||
|||	    Valid values for attrTag can be found in clttxdblend.h.
|||
|||	  Return Value
|||
|||	    0 is returned if there are no errors.
|||	    negative number is returned on error.
|||
|||	  Implementation
|||
|||	    Folio call in the CLT
|||
**/

/**
|||	AUTODOC -class CLT -name CLT_GetPipLoadCmdListSize
|||	Returns the size of the CLT Snippet in a CLTPipControlBlock
|||
|||	  Synopsis
|||
|||	    CLT_GetPipLoadCmdListSize(CltPipControlBlock pcb);
|||
|||	  Description
|||
|||	    This macro returns the size, in words, of the command list stored
|||	    in the snippet associated with the given CltPipControlBlock.
|||
|||	  Return Value
|||
|||	    Returns the size of the snippet in words.
|||
|||	  Implementation
|||
|||	    Macro found in clttxdblend.h
|||
**/

/**
|||	AUTODOC -class CLT -name CLT_Bits
|||	Returns bit values for a specified CLT register, field and value
|||
|||	  Synopsis
|||
|||	    CLT_Bits(uint32 r, uint32 f, uint32 d);
|||
|||	  Description
|||
|||	    This macro returns the the bits that would be set in register r if
|||	    field f were set to value d. The macros used to define registers
|||	    and fields can be found in cltmacros.h. If the values being
|||	    specified in d have symbolic meaning, then it is preferable to
|||	    use macros to specify d, in which case CLT_SetConst should be
|||	    used instead.
|||
|||	  Return Value
|||
|||	    Returns a word with the specified bits set
|||
|||	  Implementation
|||
|||	    Macro found in cltmacros.h
|||
**/

/**
|||	AUTODOC -class CLT -name CLT_SetConst
|||	Returns bit values that can be used to set CLT register values
|||
|||	  Synopsis
|||
|||	    CLT_SetConst(uint32 r, uint32 f, uint32 d);
|||
|||	  Description
|||
|||	    This macro returns the the bits that would be set in register r if
|||	    field f were set to value d, where d is a macro representing some 
|||	    possible value of field f. If d is simply a numeric value,
|||	    CLT_Bits should be used instead. The macros used to define
|||	    registers and fields can be found in cltmacros.h.
|||
|||	  Return Value
|||
|||	    Returns a word with the specified bits set
|||
|||	  Implementation
|||
|||	    Macro found in cltmacros.h
|||
**/

/**
|||	AUTODOC -class CLT -name CLT_ESCNTL
|||	Adds commands to a command list to set the ESCNTL register
|||
|||	  Synopsis
|||
|||	    CLT_ESCNTL(uint32 **pp, int perspective, int duscan, int dspoff);
|||
|||	  Description
|||
|||	    This macro modifies the command list pointed to by *pp by adding
|||	    commands to it to set the ESCNTL register. The ESCNTL register
|||	    has three bits. The value in perspective will set the bit which
|||	    turns perspective correction off.  The value in duscan sets the
|||	    bit which sets whether the TE scans up or down.  The value in
|||	    dspoff sets the bit which disables double scan prevention (which 
|||	    is what keeps a triangle drawn from (0,0) to (100,100) from
|||	    actually writing into screen row or column 100).
|||
|||	  Return Value
|||
|||	    none
|||
|||	  Implementation
|||
|||	    Macro found in cltmacros.h
|||
**/

/**
|||	AUTODOC -class CLT -name CLT_Write2Registers
|||	Adds commands to a command list to set 2 consecutive registers
|||
|||	  Synopsis
|||
|||	    CLT_Write2Registers(uint32 **ptr, uint32 reg, uint32 val1, uint32 val2);
|||
|||	  Description
|||
|||	    This macro modifies the command list pointed to by *ptr by adding
|||	    commands to it to set CLT register reg to value val1 and CLT
|||	    register reg+4 (the next register) to value val2.
|||
|||	  Return Value
|||
|||	    none
|||
|||	  Implementation
|||
|||	    Macro found in cltmacros.h
|||
**/

/**
|||	AUTODOC -class CLT -name CLT_TxtControl
|||	Adds commands to a command list to set 3 texture registers
|||
|||	  Synopsis
|||
|||	    CLT_TxtControl(uint32 **ptr, uint32 a, uint32 p, uint32 b);
|||
|||	  Description
|||
|||	    This macro modifies the command list pointed to by *ptr by adding
|||	    commands to it to set the TXTADDRCNTL register to value a, the
|||	    TXTPIPCNTL register to value p and the TXTTABCNTL register to value b.
|||
|||	  Return Value
|||
|||	    none
|||
|||	  Implementation
|||
|||	    Macro found in cltmacros.h
|||
**/

/**
|||	AUTODOC -class CLT -name CLT_TxtLoad4LOD
|||	Adds commands to a command list to set 4 LOD address registers
|||
|||	  Synopsis
|||
|||	    CLT_TxtLoad4LOD(uint32 **ptr, uint32 b0, uint32 b1, uint32 b2, uint32 b3);
|||
|||	  Description
|||
|||	    This macro modifies the command list pointed to by *ptr by adding commands
|||	    to it to set the TXTLODBASE0 register to value b0, the TXTLODBASE1 register
|||	    to value b1, the TXTLODBASE2 register to value b2 and the TXTLODBASE3
|||	    register to value b3.
|||
|||	  Return Value
|||
|||	    none
|||
|||	  Implementation
|||
|||	    Macro found in cltmacros.h
|||
**/

/**
|||	AUTODOC -class CLT -name CLT_DBXYWINCLIP
|||	Adds commands to a command list to set the x and y limits of the clip window
|||
|||	  Synopsis
|||
|||	    CLT_DBXYWINCLIP(uint32 **pp, int xwinclipmin, int xwinclipmax,
|||	                    int ywinclipmin, int ywinclipmax);
|||
|||	  Description
|||
|||	    This macro modifies the command list pointed to by *ptr by adding commands
|||	    to it to set the DBXWINCLIP register to contain values xwinclipmin and
|||	    xwinclipmax, and to set the DBYWINCLIP register to contain values ywinclipmin
|||	    and ywinclipmax.
|||
|||	  Return Value
|||
|||	    none
|||
|||	  Implementation
|||
|||	    Macro found in cltmacros.h
|||
**/

/**
|||	AUTODOC -class CLT -name CLT_Vertex
|||	CLT_VertexW,
|||	CLT_VertexRgba,
|||	CLT_VertexRgbaW,
|||	CLT_VertexUv,
|||	CLT_VertexUvW,
|||	CLT_VertexRgbaUv,
|||	CLT_VertexRgbaUvW,
|||	Adds a triangle vertex to a command list
|||
|||	  Synopsis
|||
|||	    CLT_Vertex(uint32 **ptr, gfloat x,gfloat y,
|||	               {gfloat r,gfloat g,gfloat b,gfloat a,} 
|||	               gfloat {u,v}, gfloat {w});
|||
|||	  Description
|||
|||	    These macros modify the command list pointed to by *ptr by adding
|||	    a triangle vertex.  This vertex has x,y coordinates, and some
|||	    subset of colors (Rgba), texture coordinates (Uv), and a
|||	    perspective value (W). These commands must follow a CLT_TRIANGLE
|||	    command which specifies which of perspective, texture and color
|||	    will be used.
|||
|||	  Return Value
|||
|||	    none
|||
|||	  Implementation
|||
|||	    Macros found in cltmacros.h
|||
**/

/**
|||	AUTODOC -class CLT -name CLT_Nop
|||	Adds commands to a command list which does nothing
|||
|||	  Synopsis
|||
|||	    CLT_Nop(uint32 **pp)
|||
|||	  Description
|||
|||	    This macro modifies the command list pointed to by *ptr by adding a
|||	    command to it which does nothing
|||
|||	  Return Value
|||
|||	    none
|||
|||	  Implementation
|||
|||	    Macro found in cltmacros.h
|||
**/

/* Keep the compiler happy */
extern int foo;
