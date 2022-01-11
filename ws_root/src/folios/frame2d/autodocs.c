/**
|||	AUTODOC -public -class frame2d -name Gro_Create
|||	Create a grid object.
|||
|||	  Synopsis
|||
|||	    GridObj *Gro_Create (TagArg *t);
|||
|||	  Description
|||
|||	    This routine creates a GridObj structure, and fills in any
|||	    parts that are requested by the TagArg list.
|||
|||	    If the TagArg list is empty, or the pointer is null, then an
|||	    "empty" grid object is created.
|||
|||	  Arguments
|||
|||	    t
|||	        Pointer to TagArg list
|||
|||	  TagArg Tags
|||
|||	    GRO_TAG_SPRITEARRAY
|||	        Set the array of pointers to sprites for the GridObj
|||
|||	    GRO_TAG_WIDTH
|||	        Set the width of the sprite array
|||
|||	    GRO_TAG_HEIGHT
|||	        Set the height of the sprite array
|||
|||	    GRO_TAG_XPOS
|||	        Set the X component of the GridObj's screen position
|||
|||	    GRO_TAG_YPOS
|||	        Set the Y component of the GridObj's screen position
|||
|||	    GRO_TAG_HDELTAX
|||	        Set the X component of the HDelta
|||
|||	    GRO_TAG_HDELTAY
|||	        Set the Y component of the HDelta
|||
|||	    GRO_TAG_VDELTAX
|||	        Set the X component of the VDelta
|||
|||	    GRO_TAG_VDELTAY
|||	        Set the Y component of the VDelta
|||
|||	  Return Value
|||
|||	    This routine will return a pointer to the GridObj structure
|||	    created, or 0 on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/gridobj.h>
|||	    <graphics/frame2d/frame2d.h>
|||
|||	  See Also
|||
|||	    Gro_Delete(), Gro_SetSpriteArray(), Gro_SetWidth(), 
|||	    Gro_SetHeight(), Gro_SetPosition(), Gro_SetHDelta(), 
|||	    Gro_SetVDelta(), Gro_GetSpriteArray(), Gro_GetWidth(),
|||	    Gro_GetHeight(), Gro_GetPosition(), Gro_GetHDelta(),
|||	    Gro_GetVDelta()
|||
|||
**/


/**
|||	AUTODOC -public -class frame2d -name Gro_Delete
|||	Delete a grid object.
|||
|||	  Synopsis
|||
|||	    Err Gro_Delete (GridObj* gr);
|||
|||	  Description
|||
|||	    Delete a GridObj structure, and also free up any elements
|||	    that were created as part of the grid object.
|||
|||	  Arguments
|||
|||	    gr
|||	        Pointer to grid object to delete
|||
|||
|||	  Return Value
|||
|||	    This routine will return 0 on success or a negative error code
|||	    on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/gridobj.h>
|||
|||	  See Also
|||
|||	    Gro_Create()
|||
**/

/**
|||	AUTODOC -public -class frame2d -name Gro_SetSpriteArray
|||	Attach a sprite array to a GridObj
|||
|||	  Synopsis
|||
|||	    Err Gro_SetSpriteArray (GridObj *gr, SpriteObj **sp)
|||
|||	  Description
|||
|||	    Attach an array of pointers to sprite objects to a GridObj.
|||	    The entries in the sprite array can also point to extended
|||	    or short sprite objects.  Since the geometry associated with
|||	    the sprite objects is not used when drawn as part of a grid
|||	    object, short sprites can save on memory.
|||
|||	  Arguments
|||
|||	    gr
|||	        Pointer to the grid object to modify
|||
|||	    sp
|||	        Pointer to the array of pointers to sprites
|||
|||
|||	  Return Value
|||
|||	    This routine will return 0 on success or a negative error code
|||	    on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/gridobj.h>
|||
|||	  See Also
|||
|||	    Gro_GetSpriteArray(), Gro_SetWidth(), Gro_SetHeight()
|||
**/

/**
|||	AUTODOC -public -class frame2d -name Gro_SetWidth
|||	Set the width of the sprite array associated with a grid object.
|||
|||	  Synopsis
|||
|||	    Err Gro_SetWidth (GridObj *gr, uint32 w);
|||
|||	  Description
|||
|||	    Set the width of the sprite array associated with the grid
|||	    object.
|||
|||	  Arguments
|||
|||	    gr
|||	        Pointer to the grid object to modify
|||
|||	    w
|||	        New width for the sprite array
|||
|||
|||	  Return Value
|||
|||	    This routine will return 0 on success or a negative error code
|||	    on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/gridobj.h>
|||
|||	  See Also
|||
|||	    Gro_GetWidth(), Gro_SetHeight()
|||
**/

/**
|||	AUTODOC -public -class frame2d -name Gro_SetHeight
|||	Set the height of the sprite array associated with a grid object.
|||
|||	  Synopsis
|||
|||	    Err Gro_SetHeight (GridObj *gr, uint32 h);
|||
|||	  Description
|||
|||	    Set the height of the sprite array associated with the grid
|||	    object.
|||
|||	  Arguments
|||
|||	    gr
|||	        Pointer to the grid object to modify
|||
|||	    h
|||	        New height for the sprite array
|||
|||
|||	  Return Value
|||
|||	    This routine will return 0 on success or a negative error code
|||	    on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/gridobj.h>
|||
|||	  See Also
|||
|||	    Gro_GetHeight(), Gro_SetWidth()
|||
**/

/**
|||	AUTODOC -public -class frame2d -name Gro_SetPosition
|||	Set the screen position of the grid object
|||
|||	  Synopsis
|||
|||	    Err Gro_SetPosition (GridObj* gr, Point2* p)
|||
|||	  Description
|||
|||	    Set the screen position of the grid object.
|||
|||	  Arguments
|||
|||	    gr
|||	        Pointer to the grid object to modify
|||
|||	    p
|||	        New screen position for the grid object
|||
|||
|||	  Return Value
|||
|||	    This routine will return 0 on success or a negative error code
|||	    on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/gridobj.h>
|||
|||	  See Also
|||
|||	    Gro_GetPosition(), Gro_SetHDelta(), Gro_SetVDelta()
|||
**/

/**
|||	AUTODOC -public -class frame2d -name Gro_SetHDelta
|||	Set the hdelta of the grid object
|||
|||	  Synopsis
|||
|||	    Err Gro_SetHDelta (GridObj* gr, Point2* h)
|||
|||	  Description
|||
|||	    Set the hdelta of the grid object.  The hdelta is the change in
|||	    screen position from one sprite to the next in the same row of
|||	    of the sprite array.
|||
|||	  Arguments
|||
|||	    gr
|||	        Pointer to the grid object to modify
|||
|||	    h
|||	        New screen hdelta for the grid object
|||
|||
|||	  Return Value
|||
|||	    This routine will return 0 on success or a negative error code
|||	    on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/gridobj.h>
|||
|||	  See Also
|||
|||	    Gro_GetHDelta(), Gro_SetPosition(), Gro_SetVDelta()
|||
**/

/**
|||	AUTODOC -public -class frame2d -name Gro_SetVDelta
|||	Set the vdelta of the grid object
|||
|||	  Synopsis
|||
|||	    Err Gro_SetVDelta (GridObj* gr, Point2* h)
|||
|||	  Description
|||
|||	    Set the vdelta of the grid object.  The vdelta is the change in
|||	    screen position from a sprite in one row to the sprite in the 
|||	    next row of the sprite array.
|||
|||	  Arguments
|||
|||	    gr
|||	        Pointer to the grid object to modify
|||
|||	    h
|||	        New screen vdelta for the grid object
|||
|||
|||	  Return Value
|||
|||	    This routine will return 0 on success or a negative error code
|||	    on failure.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/gridobj.h>
|||
|||	  See Also
|||
|||	    Gro_GetVDelta(), Gro_SetPosition(), Gro_SetHDelta()
|||
**/

/**
|||	AUTODOC -public -class frame2d -name Gro_GetSpriteArray
|||	Get a pointer to the sprite array associated with a grid object
|||
|||	  Synopsis
|||
|||	    SpriteObj **Gro_GetSpriteArray (GridObj *gr)
|||
|||	  Description
|||
|||	    Return a pointer to the sprite array associated with a GridObj.
|||
|||	  Arguments
|||
|||	    gr
|||	        Pointer to the grid object to modify
|||
|||
|||	  Return Value
|||
|||	    This routine will return a pointer to the sprite array associated
|||	    with a grid object, or 0 if there is none.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/gridobj.h>
|||
|||	  See Also
|||
|||	    Gro_SetSpriteArray(), Gro_GetWidth(), Gro_GetHeight()
|||
**/

/**
|||	AUTODOC -public -class frame2d -name Gro_GetWidth
|||	Query the width of the sprite array in a grid object.
|||
|||	  Synopsis
|||
|||	    uint32 Gro_GetWidth (const GridObj *gr);
|||
|||	  Description
|||
|||	    Query the current width of a sprite array in a grid object.
|||
|||	  Arguments
|||
|||	    gr
|||	        Pointer to the grid object to query
|||
|||
|||	  Return Value
|||
|||	    This routine will return the width of the sprite array
|||	    associated with the grid object.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/gridobj.h>
|||
|||	  See Also
|||
|||	    Gro_SetWidth(), Gro_GetSpriteArray(), Gro_GetHeight()
|||
**/

/**
|||	AUTODOC -public -class frame2d -name Gro_GetHeight
|||	Query the height of the sprite array in a grid object.
|||
|||	  Synopsis
|||
|||	    uint32 Gro_GetHeight (const GridObj *gr);
|||
|||	  Description
|||
|||	    Query the current height of a sprite array in a grid object.
|||
|||	  Arguments
|||
|||	    gr
|||	        Pointer to the grid object to query
|||
|||
|||	  Return Value
|||
|||	    This routine will return the height of the sprite array
|||	    associated with the grid object.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/gridobj.h>
|||
|||	  See Also
|||
|||	    Gro_SetHeight(), Gro_GetSpriteArray(), Gro_GetWidth()
|||
**/

/**
|||	AUTODOC -public -class frame2d -name Gro_GetPosition
|||	Obtain the screen position of the grid object.
|||
|||	  Synopsis
|||
|||	    void Gro_GetPosition (const GridObj* gr, Point2* p);
|||
|||	  Description
|||
|||	    Query the screen position of the grid object
|||
|||	  Arguments
|||
|||	    gr
|||	        Pointer to the grid object to query
|||
|||	    p
|||	        Pointer to the Point2 structure to place the result
|||
|||
|||	  Return Value
|||
|||	    This routine will return the position of the grid object
|||	    placing the result in the structure pointed to by p.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/gridobj.h>
|||
|||	  See Also
|||
|||	    Gro_SetPosition()
|||
**/

/**
|||	AUTODOC -public -class frame2d -name Gro_GetHDelta
|||	Obtain the hdelta of the grid object.
|||
|||	  Synopsis
|||
|||	    void Gro_GetHDelta (const GridObj* gr, Point2* p);
|||
|||	  Description
|||
|||	    Query the hdelta of the grid object.  The hdelta is the change in
|||	    screen position from one sprite to the next in the same row of
|||	    of the sprite array.
|||
|||	  Arguments
|||
|||	    gr
|||	        Pointer to the grid object to query
|||
|||	    p
|||	        Pointer to the Point2 structure to place the result
|||
|||
|||	  Return Value
|||
|||	    This routine will return the HDelta of the grid object
|||	    placing the result in the structure pointed to by p.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/gridobj.h>
|||
|||	  See Also
|||
|||	    Gro_SetHDelta()
|||
**/

/**
|||	AUTODOC -public -class frame2d -name Gro_GetVDelta
|||	Obtain the vdelta of the grid object.
|||
|||	  Synopsis
|||
|||	    void Gro_GetVDelta (const GridObj* gr, Point2* p);
|||
|||	  Description
|||
|||	    Query the vdelta of the grid object.  The vdelta is the change in
|||	    screen position from one sprite to the next in the same row of
|||	    of the sprite array.
|||
|||	  Arguments
|||
|||	    gr
|||	        Pointer to the grid object to query
|||
|||	    p
|||	        Pointer to the Point2 structure to place the result
|||
|||
|||	  Return Value
|||
|||	    This routine will return the VDelta of the grid object
|||	    placing the result in the structure pointed to by p.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/gridobj.h>
|||
|||	  See Also
|||
|||	    Gro_SetVDelta()
|||
**/


extern int foo; /* just to make the compiler happy */

