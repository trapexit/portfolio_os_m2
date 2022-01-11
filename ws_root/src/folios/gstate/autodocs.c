/* @(#) autodocs.c 96/11/20 1.36 */

/**
|||	AUTODOC -class GState -name GS_Create
|||	Creates a GState (Graphics State) object.
|||
|||	  Synopsis
|||
|||	    GState *GS_Create(void);
|||
|||	  Description
|||
|||	    Creates a GState, or Graphics State, object.  This object is used to
|||	    encapsulate the information which needs to be maintained for command
|||	    list buffers. The triangle engine operates by executing the series of
|||	    instructions which are placed into these command list buffers.
|||
|||	    The 2D graphics folio, font folio, and 3D graphics library all use
|||	    command list buffers to communicate with the triangle engine. By
|||	    creating a common GState that can be shared by these folios and
|||	    libraries, the user can combine text, 2D graphics, and 3D graphics
|||	    more easily.
|||
|||	    After a GState is created, it still must be intialized by calling
|||	    GS_AllocLists(), to create one or more buffers of the specified
|||	    size.
|||
|||	  Return Value
|||
|||	    Returns a pointer to a GState if successful, or NULL if it fails.
|||
|||	  Implementation
|||
|||	    Folio call implemented in gstate V29
|||
|||	  Associated Files
|||
|||	    <graphics/clt/gstate.h>, System.m2/Modules/gstate
|||
|||	  See Also
|||
|||	    GS_AllocLists(), GS_Reserve(), GS_SendList(), GS_SetList(),
|||	    GS_WaitIO()
|||
**/

/**
|||	AUTODOC -class GState -name GS_Delete
|||	Frees all memory associated with a GState object.
|||
|||	  Synopsis
|||
|||	    Err GS_Delete(GState *g);
|||
|||	  Description
|||
|||	    Frees up all memory used by a GState object. If command list buffers
|||	    have been allocated by calling GS_AllocLists(), GS_Delete() will also
|||	    free the memory used by these buffers.
|||
|||	  Arguments
|||
|||	    g
|||	        Pointer to a GState object.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in gstate V29
|||
|||	  Associated Files
|||
|||	    <graphics/clt/gstate.h>, System.m2/Modules/gstate
|||
|||	  See Also
|||
|||	    GS_Create(), GS_AllocLists(), GS_FreeLists()
|||
**/
/**
|||	AUTODOC -class GState -name GS_Clone
|||	Makes a copy of a gstate that will work in the same context.
|||
|||	  Synopsis
|||
|||	    Gstate *GS_Clone(GState *g);
|||
|||	  Description
|||
|||	    Copies all the attributes of the passed in GState.
|||	    The command lists of the new gstate are independent of the old.
|||	    The copied GState can be deleted and freed as any other GState.
|||
|||	  Arguments
|||
|||	    g
|||	        Pointer to a GState object to be copied.
|||
|||	  Return Value
|||
|||	    Pointer to the newly created GState if successful, or NULL for
|||	    failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in gstate V32.
|||
|||	  Associated Files
|||
|||	    <graphics/clt/gstate.h>, System.m2/Modules/gstate
|||
|||	  See Also
|||
|||	    GS_Create(), GS_Delete(), GS_FreeLists()
|||
**/

/**
|||	AUTODOC -class GState -name GS_SetList
|||	Sets a GState to use a new command list buffer.
|||
|||	  Synopsis
|||
|||	    Err GS_SetList(GState *g, uint32 listNum);
|||
|||	  Description
|||
|||	    Set a GState to use a new command list buffer. These buffers
|||	    should be allocated with GS_AllocLists(). GS_SendList() will
|||	    call this routine automatically to set up the GState to use the
|||	    next available command list buffer, if more than one buffer has
|||	    been allocated.
|||
|||	  Arguments
|||
|||	    g
|||	        Pointer to a GState object.
|||
|||	    listNum
|||	        Index stating which command list buffer should be used.
|||	        Value should be in the range [0..numberCmdLists-1].
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in gstate V29
|||
|||	  Associated Files
|||
|||	    <graphics/clt/gstate.h>, System.m2/Modules/gstate
|||
|||	  See Also
|||
|||	    GS_Create(), GS_AllocLists(), GS_Reserve(), GS_SendList()
|||
**/

/**
|||	AUTODOC -class GState -name GS_SendList
|||	Sends a GState's current command list to the triangle engine.
|||
|||	  Synopsis
|||
|||	    Err GS_SendList(GState *g);
|||
|||	  Description
|||
|||	    Send the data in the GState's current command list buffer to the
|||	    triangle engine device driver to be rendered. If more than one
|||	    command list buffer was allocated when GS_AllocLists() was called,
|||	    the GState will use the next available command list buffer when
|||	    this function returns.
|||
|||	    The Font folio, 2D graphics folio, and 3D graphics libraries build
|||	    buffers of triangle engine instructions using CLT, or the Command
|||	    List Toolkit.  Once the buffer fills up, or gets close to becoming
|||	    full, GS_SendList() is called to flush the buffer and let the
|||	    Triangle Engine begin rendering.
|||
|||	    To support tear-free double-buffering of the frame buffers,
|||	    GS_SendList() will wait on a signal from the video hardware, if one
|||	    was associated with a GState by calling GS_SetVidSignal().  This
|||	    wait will only occur when the first command list buffer is to be
|||	    rendered into a frame. To specify that this wait should occur, the
|||	    user should call GS_BeginFrame() whenever a new frame buffer has
|||	    just been displayed.
|||
|||	    Normally, the user application should call g->gs_SendList(g) instead
|||	    of calling this routine directly, so that if it becomes necessary
|||	    to debug the command list output, a user function can easily replace
|||	    GS_SendList(). g->gs_SendList() is initialized to use GS_SendList()
|||	    by default when a GState is created.
|||
|||	    g->gs_SendList() is called automatically by the GState folio whenever
|||	    GS_Reserve() cannot allocate enough space in the current command
|||	    list buffer.
|||
|||	    In low latency mode (after calling GS_LowLatency()) only a portion of
|||	    the list is sent at a time. This portion represents the amount of latency
|||	    between the CPU computing commands and the triangle engine consuming them.
|||
|||	    In low latency mode a portion of the list remains unsent until a call is
|||	    made to GS_SendLastList()
|||
|||
|||	  Arguments
|||
|||	    g
|||	        Pointer to a GState object.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in gstate V29
|||
|||	  Associated Files
|||
|||	    <graphics/clt/gstate.h>, System.m2/Modules/gstate
|||
|||	  See Also
|||
|||	    GS_Create(), GS_AllocLists(), GS_Reserve(), GS_SetList(),
|||	    GS_SetVidSignal(), GS_GetVidSignal(), GS_BeginFrame(), GS_LowLatency(),
|||	    GS_SendLastList()
**/
/**
|||	AUTODOC -class GState -name GS_SendLastList
|||	Used to send the last list when in low latency mode
|||
|||	  Synopsis
|||
|||	    Err GS_SendLastList(GState *g)
|||
|||	  Description
|||
|||	    When using low latency mode and TE is already running, this function
|||	    places a Pause instruction at the end and sends the last part of the buffer to the TE.
|||
|||
|||	  Arguments
|||
|||	    gs
|||	        Pointer to a GState object.
|||
|||	  Return Value
|||
|||	    Negative on error.
|||
|||	  Implementation
|||
|||	    Folio call implemented in gstate folio V32.
|||
|||	  Associated Files
|||
|||	    <graphics/clt/gstate.h>
|||
|||	  See Also
|||
|||	    GS_SendList()
|||
**/

/**
|||	AUTODOC -class GState -name GS_AllocLists
|||	Allocates one or more command list buffers for graphics rendering.
|||
|||	  Synopsis
|||
|||	    Err GS_AllocLists(GState *g, uint32 numLists, uint32 numWords);
|||
|||	  Description
|||
|||	    Allocates one or more command list buffers of the specified size.
|||	    These buffers are used by the Font folio, 2D graphics folio, and
|||	    3D graphics libraries to build command lists for the triangle engine
|||	    to render.
|||
|||	    Each command list buffer must be large enough that the largest
|||	    triangle strip or fan in a program's data can fit within the buffer.
|||	    Triangle strips and fans can sometimes be several hundred vertices
|||	    long, and each vertex can take up to 9 32-bit words (36 bytes) of
|||	    command list buffer space.
|||
|||	    Normally, a minimum of two command list buffers should be allocted,
|||	    so that while the triangle engine is processing one buffer's commands,
|||	    the CPU can begin preparing the next set of rendering commands.
|||	    Larger buffers will often yield better performance than smaller
|||	    buffers, since the CPU spends less time flushing buffers and waiting
|||	    for new ones to be freed up by the Triangle Engine.  The parameters
|||	    to GS_AllocLists() should be adjusted during development of a title,
|||	    to optimally blend between high performance and memory usage.
|||
|||	  Arguments
|||
|||	    g
|||	        Pointer to a GState object.
|||
|||	    numLists
|||	        Number of command lists buffers to allocte.
|||
|||	    numWords
|||	        Size in 32-bit words of each command list buffer.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in gstate V29
|||
|||	  Associated Files
|||
|||	    <graphics/clt/gstate.h>, System.m2/Modules/gstate
|||
|||	  See Also
|||
|||	    GS_Create(), GS_FreeLists(), GS_Reserve(), GS_SendList(), GS_SetList()
**/

/**
|||	AUTODOC -class GState -name GS_FreeLists
|||	Frees the memory used by command list buffers for a GState object.
|||
|||	  Synopsis
|||
|||	    Err GS_FreeLists(GState *g);
|||
|||	  Description
|||
|||	    Free the memory used by the command list buffer(s) for a GState
|||	    object. These buffers are allocated by calling GS_AllocLists().
|||
|||	    Normally, this routine does not need to be called, since it will
|||	    be called by GS_Delete() if the command list buffers are still
|||	    allocated. However, it can be used in special situations, such as
|||	    when, because of memory considerations at run-time, it becomes
|||	    necessary to re-size the command list buffers.
|||
|||	  Arguments
|||
|||	    g
|||	        Pointer to a GState object.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in gstate V29
|||
|||	  Associated Files
|||
|||	    <graphics/clt/gstate.h>, System.m2/Modules/gstate
|||
|||	  See Also
|||
|||	    GS_Create(), GS_AllocLists(), GS_Delete()
|||
**/

/**
|||	AUTODOC -class GState -name GS_SetVidSignal
|||	Attaches a signal to a GState object, for video synchronization.
|||
|||	  Synopsis
|||
|||	    Err GS_SetVidSignal(GState *g, int32 signal);
|||
|||	  Description
|||
|||	    Associate a signal bit with a GState object. This signal will
|||	    allow a GState to stay synchronized with the video in a double-
|||	    buffering scheme.
|||
|||	    To correctly enable this feature of GState, allocate a signal
|||	    by calling AllocSignal() before a View is created.  Then, use
|||	    this allocated signal as the argument for the VIEWTAG_RENDERSIGNAL
|||	    tag when a view is created.  Last, associate this signal with a
|||	    GState by calling GS_SetVidSignal().  Whenever one bitmap has
|||	    been fully rendered and sent to the Graphics folio, the user should
|||	    also call GS_BeginFrame(), to tell a GState that it needs to wait
|||	    for this signal before it sends the next command list.
|||
|||	  Arguments
|||
|||	    g
|||	        Pointer to a GState object.
|||
|||	    signal
|||	        A signal mask as obtained from AllocSignal().
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in gstate V29
|||
|||	  Associated Files
|||
|||	    <graphics/clt/gstate.h>, System.m2/Modules/gstate
|||
|||	  See Also
|||
|||	    GS_Create(), GS_GetVidSignal(), GS_BeginFrame(), GS_SendList(),
|||	    View(@)
|||
**/

/**
|||	AUTODOC -class GState -name GS_GetVidSignal
|||	Returns the signal bit which a GState is using for video
|||	synchronization.
|||
|||	  Synopsis
|||
|||	    int32 GS_GetVidSignal(GState *g);
|||
|||	  Description
|||
|||	    Returns a signal bit mask showing which signal, if any, is being
|||	    used by a GState object to keep triangle engine rendering in sync
|||	    with double-buffering of bitmaps by the Graphics folio. This signal
|||	    is associated with a GState by calling GS_SetVidSignal().
|||
|||	    This signal should normally be the same value which was passed
|||	    to the Graphics folio as the arg for the VIEWTAG_RENDERSIGNAL tag,
|||	    when the View was created.
|||
|||	  Arguments
|||
|||	    g
|||	        Pointer to a GState object.
|||
|||	  Return Value
|||
|||	    Returns the signal associated with a GState, or 0 if one was not
|||	    associated with the GState.
|||
|||	  Implementation
|||
|||	    Folio call implemented in gstate V29
|||
|||	  Associated Files
|||
|||	    <graphics/clt/gstate.h>, System.m2/Modules/gstate
|||
|||	  See Also
|||
|||	    GS_Create(), GS_SetVidSignal(), GS_BeginFrame(), GS_SendList(),
|||	    View(@)
|||
**/

/**
|||	AUTODOC -class GState -name GS_BeginFrame
|||	Notifies a GState that the next command list is the first for a frame.
|||
|||	  Synopsis
|||
|||	    Err GS_BeginFrame(GState *g);
|||
|||	  Description
|||
|||	    Notifies a GState that the next command list buffer is the first
|||	    to be rendered to a Bitmap. This routine is used when double-
|||	    buffering is desired.  At the first call to GS_SendList() after
|||	    GS_BeginFrame() is called, the GState will wait on a signal
|||	    from the Graphics folio before sending a command list to the
|||	    triangle engine device driver.
|||
|||	    In order for double-buffering to correctly prevent screen tearing,
|||	    this routine should be called after each call to switch between
|||	    frame buffers (usually with a call to ModifyGraphicsItem()). In
|||	    addition, a signal should be associated with the View and a GState
|||	    when these objects are being created. See GS_SetVidSignal() for
|||	    more information about how to correctly allocate this signal.
|||
|||	  Arguments
|||
|||	    g
|||	        Pointer to a GState object.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in gstate V29
|||
|||	  Associated Files
|||
|||	    <graphics/clt/gstate.h>, System.m2/Modules/gstate
|||
|||	  See Also
|||
|||	    GS_Create(), GS_SetVidSignal(), GS_GetVidSignal(), GS_SendList()
|||
**/

/**
|||	AUTODOC -class GState -name GS_Reserve
|||	Reserves a block of memory in GState's current command list buffer.
|||
|||	  Synopsis
|||
|||	    void GS_Reserve(GState *g, uint32 numWords);
|||
|||	  Description
|||
|||	    Check to ensure that there is enough space in a GState's current
|||	    command list buffer for the requested number of words. If there
|||	    isn't enough space, flush the current command list buffer, and if
|||	    more than one command list buffer was allocated by GS_AllocLists(),
|||	    reserve the requested space in the next available buffer.
|||
|||	    Note that this routine will silently fail if the requested number
|||	    of words is larger than the total size of a command list buffer.
|||
|||	  Arguments
|||
|||	    g
|||	        Pointer to a GState object.
|||
|||	    numWords
|||	        Number of words to reserve.
|||
|||	  Implementation
|||
|||	    Folio call implemented in gstate V29
|||
|||	  Associated Files
|||
|||	    <graphics/clt/gstate.h>, System.m2/Modules/gstate
|||
|||	  See Also
|||
|||	    GS_Create(), GS_AllocLists(), GS_SendList(), GS_SetList()
**/

/**
|||	AUTODOC -class GState -name GS_SetDestBuffer
|||	Sets a bitmap as the output frame buffer for a GState.
|||
|||	  Synopsis
|||
|||	    Err GS_SetDestBuffer(GState *g, Item bitmap);
|||
|||	  Description
|||
|||	    Set a bitmap as the output frame buffer for a GState. When commands
|||	    are sent to the Triangle Engine by this GState object, the output
|||	    from the triangle engine will go into the specified
|||	    bitmap.
|||
|||	    Note that this function will also pass the r,g,b,a,z values
|||	    set in GS_SetClearValues() to the TriangleEngine device
|||	    driver.
|||
|||	  Arguments
|||
|||	    g
|||	        Pointer to a GState object.
|||
|||	    bitmap
|||	        Item representing a Bitmap object.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in gstate V29
|||
|||	  Associated Files
|||
|||	    <graphics/clt/gstate.h>, System.m2/Modules/gstate
|||
|||	  See Also
|||
|||	    GS_Create(), GS_GetDestBuffer(), GS_SetZBuffer(),
|||	    GS_GetZBuffer(), TE_CMD_SETFRAMEBUFFER(@)
|||
**/

/**
|||	AUTODOC -class GState -name GS_SetZBuffer
|||	Sets a bitmap as the Z-buffer for a GState.
|||
|||	  Synopsis
|||
|||	    Err GS_SetZBuffer(GState *g, Item bitmap);
|||
|||	  Description
|||
|||	    Set a bitmap as the Z-buffer for a GState. If Z-buffering is enabled
|||	    for the triangle engine, this bitmap will be used to hold the depth
|||	    information for triangles as they are rendered.
|||
|||	  Arguments
|||
|||	    g
|||	        Pointer to a GState object.
|||
|||	    bitmap
|||	        Item representing a Bitmap object.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in gstate V29
|||
|||	  Associated Files
|||
|||	    <graphics/clt/gstate.h>, System.m2/Modules/gstate
|||
|||	  See Also
|||
|||	    GS_Create(), GS_GetZBuffer(), GS_SetDestBuffer(),
|||	    GS_GetDestBuffer(), TE_CMD_SETZBUFFER(@)
|||
**/

/**
|||	AUTODOC -class GState -name GS_SetView
|||	Sets a View to which rendered Bitmaps are to be attached.
|||
|||	  Synopsis
|||
|||	    Err GS_SetView (GState *g, Item view);
|||
|||	  Description
|||
|||	    Sets the View to which Bitmaps are to be attached when rendering
|||	    is complete.  This function is designed to work hand-in-hand
|||	    with GS_EndFrame().
|||
|||	  Arguments
|||
|||	    g
|||	        Pointer to a GState object.
|||
|||	    view
|||	        Item number of the View.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in gstate V32
|||
|||	  Associated Files
|||
|||	    <graphics/clt/gstate.h>, System.m2/Modules/gstate
|||
|||	  See Also
|||
|||	    GS_Create(), GS_GetZBuffer(), GS_SetDestBuffer(),
|||	    GS_GetDestBuffer(), GS_GetView(), GS_EndFrame(), View(@),
|||	    TE_CMD_SETVIEW(@)
|||
**/

/**
|||	AUTODOC -class GState -name GS_GetDestBuffer
|||	Gets the Item number of the bitmap being used as an output frame buffer.
|||
|||	  Synopsis
|||
|||	    Item GS_GetDestBuffer(GState *g);
|||
|||	  Description
|||
|||	    Returns the Item number for the Bitmap being used as an output frame
|||	    buffer for a GState object. When triangle engine commands are rendered
|||	    through the specified GState object, the Triangle Engine will render
|||	    into this Bitmap. Z-buffer information is stored in a separate
|||	    bitmap.
|||
|||	  Arguments
|||
|||	    g
|||	        Pointer to a GState object.
|||
|||	  Return Value
|||
|||	    Returns an Item number for the Bitmap, or an error code if a valid
|||	    bitmap has not been set for this GState.
|||
|||	  Implementation
|||
|||	    Folio call implemented in gstate V29
|||
|||	  Associated Files
|||
|||	    <graphics/clt/gstate.h>, System.m2/Modules/gstate
|||
|||	  See Also
|||
|||	    GS_Create(), GS_SetDestBuffer(), GS_SetZBuffer(), GS_GetZBuffer()
|||
**/

/**
|||	AUTODOC -class GState -name GS_GetZBuffer
|||	Gets the Item number of the bitmap being used as a Z-buffer.
|||
|||	  Synopsis
|||
|||	    Item GS_GetZBuffer(GState *g);
|||
|||	  Description
|||
|||	    Returns the Item number for the Bitmap being used as a Z-buffer for a
|||	    GState object. When triangle engine commands are rendered through
|||	    the specified GState object, the triangle engine will use this
|||	    bitmap to hold Z-buffer information.  The actual rendered scene is
|||	    stored in a separate bitmap.
|||
|||	  Arguments
|||
|||	    g
|||	        Pointer to a GState object.
|||
|||	  Return Value
|||
|||	    Returns an Item number for the Bitmap, or an error code if a valid
|||	    bitmap has not been set for this GState.
|||
|||	  Implementation
|||
|||	    Folio call implemented in gstate V29
|||
|||	  Associated Files
|||
|||	    <graphics/clt/gstate.h>, System.m2/Modules/gstate
|||
|||	  See Also
|||
|||	    GS_Create(), GS_SetZBuffer(), GS_SetDestBuffer(), GS_GetDestBuffer()
|||
**/

/**
|||	AUTODOC -class GState -name GS_GetView
|||	Gets the Item number of the View being used to display Bitmaps.
|||
|||	  Synopsis
|||
|||	    Item GS_GetView (GState *g);
|||
|||	  Description
|||
|||	    Returns the Item number of the View currently being used to
|||	    display Bitmaps rendered through the specified GState object.
|||	    This value is set using GS_SetView().
|||
|||	  Arguments
|||
|||	    g
|||	        Pointer to a GState object.
|||
|||	  Return Value
|||
|||	    Returns the Item number of the View, or an error code if a valid
|||	    View has not been set for this GState.
|||
|||	  Implementation
|||
|||	    Folio call implemented in gstate V32
|||
|||	  Associated Files
|||
|||	    <graphics/clt/gstate.h>, System.m2/Modules/gstate
|||
|||	  See Also
|||
|||	    GS_Create(), GS_SetZBuffer(), GS_SetDestBuffer(),
|||	    GS_GetDestBuffer(), GS_SetView(), View(@)
|||
**/

/**
|||	AUTODOC -class GState -name GS_WaitIO
|||	Waits for all pending IO to complete for a GState.
|||
|||	  Synopsis
|||
|||	    Err GS_WaitIO(GState *g);
|||
|||	  Description
|||
|||	    For a given GState, wait for any active IORequests to complete.
|||	    This routine is usually used to ensure that the triangle engine is
|||	    not still rendering into a bitmap before that bitmap is displayed.
|||
|||	  Arguments
|||
|||	    g
|||	        Pointer to a GState object.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in gstate V29
|||
|||	  Associated Files
|||
|||	    <graphics/clt/gstate.h>, System.m2/Modules/gstate
|||
|||	  See Also
|||
|||	    GS_Create(), GS_AllocLists(), GS_SendList()
**/

/**
|||	AUTODOC -class GState -name GS_AllocBitmaps
|||	Allocates frame buffers and Z-buffer.
|||
|||	  Synopsis
|||
|||	    Err GS_AllocBitmaps(Item bitmaps[], uint32 xres, uint32 yres,
|||	                        uint32 bmType, uint32 numFrameBufs, bool useZb);
|||
|||	  Description
|||
|||	    Utility routine to allocate frame buffers, and optionally a
|||	    Z-buffer. Since the triangle engine is able to perform better when
|||	    the Z-buffer is aligned to an odd page offset from the frame buffer(s),
|||	    GS_AllocBitmaps() will handle aligning the buffers automatically.
|||
|||	  Arguments
|||
|||	    bitmaps
|||	        Array of Items which will become the bitmaps. This array should
|||	        be large enough to contain one item per frame buffer, plus one
|||	        more if Z-buffering is going to be used. Upon successful return,
|||	        bitmaps will contain the specified number of frame buffers in
|||	        bitmaps[0] ... bitmaps[numFrameBufs-1].  If a Z-buffer is needed,
|||	        it will be allocated in bitmaps[numFrameBufs].
|||
|||	    xres
|||	        The width in pixels of each bitmap.
|||
|||	    yres
|||	        The height in pixels of each bitmap.
|||
|||	    bmType
|||	        Specifies the type of bitmaps to be created.  This value should
|||	        be one of the constants defined in the <graphics/bitmap.h> header
|||	        file, such as BMTYPE_32 or BMTYPE_16.
|||
|||	    numFrameBufs
|||	        The number of frame buffers to be allocated. This number should
|||	        NOT include the Z-buffer.
|||
|||	    useZb
|||	        Specifies whether a Z-buffer should be allocated.  If TRUE, then
|||	        the size of the bitmaps[] array should be numFrameBufs+1.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success or a negative error code for failure
|||
|||	  Implementation
|||
|||	    Folio call implemented in gstate V29
|||
|||	  Associated Files
|||
|||	    <graphics/clt/gstate.h>, System.m2/Modules/gstate
|||
|||	  See Also
|||
|||	    GS_Create(), GS_FreeBitmaps(), GS_SetDestBuffer(), GS_SetZBuffer()
|||
**/

/**
|||	AUTODOC -class GState -name GS_FreeBitmaps
|||	Frees bitmaps and Z-buffer.
|||
|||	  Synopsis
|||
|||	    Err GS_FreeBitmaps(Item bitmaps[], uint32 numBitmaps);
|||
|||	  Description
|||
|||	    Free all bitmaps in the bitmaps[] array.  Usually, these will be
|||	    bitmaps that were created by calling GS_AllocBitmaps().
|||
|||	    Note that if any of these bitmaps were in use by a GState object,
|||	    an error will be returned the next time that GState needs to use
|||	    one of the bitmaps.  Therefore, it is a good idea to always either
|||	    assign new bitmaps to a GState before freeing its bitmaps, or to
|||	    free a GState, by calling GS_Delete(), before freeing the array of
|||	    bitmaps.
|||
|||	  Arguments
|||
|||	    bitmaps
|||	        Array of items representing the bitmaps for the frame buffer(s),
|||	        and optionally, a Z-buffer.
|||
|||	    numBitmaps
|||	        The total number of frame buffers in the bitmaps[] array.  This
|||	        number should equal the number of frame buffers allocated, plus
|||	        one if a Z-buffer was used.
|||
|||	  Return Value
|||
|||	    Returns >= for success or a negative error code for failure
|||
|||	  Implementation
|||
|||	    Folio call implemented in gstate V29
|||
|||	  Associated Files
|||
|||	    <graphics/clt/gstate.h>, System.m2/Modules/gstate
|||
|||	  See Also
|||
|||	    GS_Create(), GS_Delete(), GS_AllocBitmaps()
|||
**/

/**
|||	AUTODOC -class GState -name GS_GetCmdListIndex
|||	Returns the index of which command list buffer is in use.
|||
|||	  Synopsis
|||
|||	    uint32 GS_GetCmdListIndex(GState *g);
|||
|||	  Description
|||
|||	    This routine returns the index into the command list buffer array of
|||	    the command list buffer which is currently in use. In most cases,
|||	    this routine will only be useful to people writing their own routine
|||	    to send a command list to the triangle engine.
|||
|||	    In the case where more than one command list buffer is allocated when
|||	    GS_AllocLists() is called, each call to GS_SendList() will increment
|||	    this index so that the CPU can begin filling the next command list
|||	    buffer.
|||
|||	  Arguments
|||
|||	    g
|||	        Pointer to a GState object.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in gstate V29
|||
|||	  Associated Files
|||
|||	    <graphics/clt/gstate.h>, System.m2/Modules/gstate
|||
|||	  See Also
|||
|||	    GS_Create(), GS_AllocLists(), GS_SendList()
**/

/**
|||	AUTODOC -class GState -name GS_GetCurListStart
|||	Gets a pointer to the start of the current command list buffer.
|||
|||	  Synopsis
|||
|||	    CmdListP GS_GetCurListStart(GState *g);
|||
|||	  Description
|||
|||	    Returns a pointer to the start of the current command list buffer.
|||	    In most cases, this routine will only be useful to people writing
|||	    their own routine to send a command list to the triangle engine.
|||
|||	  Arguments
|||
|||	    g
|||	        Pointer to a GState object.
|||
|||	  Return Value
|||
|||	    Returns >= 0 for success or a negative error code for failure.
|||
|||	  Implementation
|||
|||	    Folio call implemented in gstate V29
|||
|||	  Associated Files
|||
|||	    <graphics/clt/gstate.h>, System.m2/Modules/gstate
|||
|||	  See Also
|||
|||	    GS_Create(), GS_SendList()
**/

/**
|||	AUTODOC -class GState -name GS_GetCount
|||	Gets the current GState counter.
|||
|||	  Synopsis
|||
|||	    uint32 GS_GetCount(GState *g);
|||
|||	  Description
|||
|||	    Returns the number of times that GS_WaitIO() has completed.
|||
|||	  Arguments
|||
|||	    g
|||	        Pointer to a GState object.
|||
|||	  Return Value
|||
|||	    Returns the count value. If the gs parameter is invalid,
|||	    then the return value will be garbage.
|||
|||	  Implementation
|||
|||	    Folio call implemented in gstate V30
|||
|||	  Associated Files
|||
|||	    <graphics/clt/gstate.h>, System.m2/Modules/gstate
|||
**/

/**
|||	AUTODOC -class GState  -name GS_SetTESpeed
|||	Controls the execution speed of the Triangle Engine
|||
|||	  Synopsis
|||
|||	    Err GS_SetTESpeed(GState *gs, uint32 speed);
|||
|||	  Description
|||
|||	    Uses the Triange Engine Device TE_CMD_SPEEDCONTROL to set the
|||	    execution speed of the Triangle Engine.
|||
|||	  Arguments
|||
|||	    gs
|||	        Pointer to a GState object.
|||
|||	    speed
|||	        The number of microseconds of delay to insert between
|||	        each instruction executed by the triangle engine. This
|||	        is in the range TE_FULLSPEED to TE_STOPPED defined in
|||	        <device/te.h>.
|||
|||	  Return Value
|||
|||	    Negative on error.
|||
|||	  Implementation
|||
|||	    Folio call implemented in gstate folio V32.
|||
|||	  Associated Files
|||
|||	    <device/te.h> <graphics/clt/gstate.h>
|||
|||	  See Also
|||
|||	    GS_SingleStep()
|||
**/

/**
|||	AUTODOC -class GState -name GS_SingleStep
|||	Singles steps through the Triangle Engine instructions.
|||
|||	  Synopsis
|||
|||	    Err GS_SingleStep(GState *gs);
|||
|||	  Description
|||
|||	    Uses the Triangle Engine device TE_CMD_STEP to advance the
|||	    triangle engine by a single triangle engine command within
|||	    the current command list.
|||
|||	    The Triangle Engine should be stopped first with
|||	    GS_SetTESpeed(). The usual order of execution is
|||	    GS_SetTESpeed(gs, TE_STOPPED);
|||	    GS_SendList(gs);
|||	    GS_SingleStep(gs);
|||	    ....
|||	    GS_SetTESpeed(gs, TE_FULLSPEED);
|||	    GS_WaitIO(gs);
|||
|||	  Arguments
|||
|||	    gs
|||	        Pointer to a GState object.
|||
|||	  Return Value
|||
|||	    Negative on error.
|||
|||	  Implementation
|||
|||	    Folio call implemented in gstate folio V32.
|||
|||	  Caveats
|||
|||	    This function cannot be used if GS_ClearFBZ() is
|||	    called with the clearz parameter set TRUE.
|||
|||	  Associated Files
|||
|||	    <device/te.h> <graphics/clt/gstate.h>
|||
|||	  See Also
|||
|||	    GS_SetTESpeed()
|||
**/
/**
|||	AUTODOC -class GState -name GS_ClearFBZ
|||	Clears the framebuffer and/or the Z buffer on the next new frame.
|||
|||	  Synopsis
|||
|||	    Err GS_ClearFBZ(GState *gs, bool clearfb, bool clearz);
|||
|||	  Description
|||
|||	    Uses the Triangle Engine device TE_CLEAR_FRAME_BUFFER to clear the
|||	    frame buffer and/or the command TE_CLEAR_Z_BUFFER to clear the
|||	    z buffer on the next new frame.
|||
|||	    The function GS_SetClearValues() is used to specify the color that is used to clear
|||	    the frame buffer and the z value that the z buffer is set
|||	    to.
|||
|||	    This function will change some of the register values in
|||	    the Triangle Engine. You should not assume the value of
|||	    any register after calling GS_ClearFBZ().
|||
|||	  Arguments
|||
|||	    gs
|||	        Pointer to a GState object.
|||
|||	    clearfb
|||	      == TRUE if the frame buffer is to be cleared
|||
|||	    clearz
|||	      == TRUE if the z buffer is to be cleared
|||
|||	  Return Value
|||
|||	    Negative on error.
|||
|||	  Implementation
|||
|||	    Folio call implemented in gstate folio V32.
|||
|||	  Caveats
|||
|||	    GS_ClearFBZ() cannot be used in LowLatency mode.
|||
|||	  Associated Files
|||
|||	    <graphics/clt/gstate.h>
|||
|||	  See Also
|||
|||	    GS_SetClearValues()
|||
**/
/**
|||	AUTODOC -class GState -name GS_SetClearValues
|||	Passes the values that are used in GS_ClearFBZ()
|||
|||	  Synopsis
|||
|||	    Err GS_SetClearValues(GState *gs, float R, float G, float B, float A, float Z)
|||
|||	  Description
|||
|||	    The r,g,b,a,z values are registered for the GState for
|||	    subsequent calls to GS_ClearFBZ(). These values are
|||	    actually read by the TriangleEngine device driver when
|||	    GS_SetDestBuffer() is called, so you should call
|||	    GS_SetClearValues() before GS_SetDestBuffer().
|||
|||	  Arguments
|||
|||	    gs
|||	        Pointer to a GState object.
|||
|||	    R,G,B,
|||	       Color to clear the frame buffer to
|||
|||	    A
|||	       Alpha value used for 32 bit frame buffers
|||
|||	    Z
|||	       Value to set the z buffer to
|||
|||	  Return Value
|||
|||	    Negative on error.
|||
|||	  Implementation
|||
|||	    Folio call implemented in gstate folio V32.
|||
|||	  Associated Files
|||
|||	    <graphics/clt/gstate.h>
|||
|||	  See Also
|||
|||	    GS_ClearFBZ()
|||
**/
/**
|||	AUTODOC -class GState -name GS_SetAbortVblank
|||	Specifies whether the TE_ABORT_AT_VBLANK feature is used by the triangle engine device driver.
|||
|||	  Synopsis
|||
|||	    Err GS_SetAbortVblank(GState* gs, bool abort)
|||
|||	  Description
|||
|||	    Either enable or disable the TE_ABORT_AT_VBLANK feature in the hardware which forces the
|||	    triangle engine to abort rendering when a vertical blank interrupt occurs. This means that
|||	    some of the commands in the current command may not be rendered.
|||
|||	  Arguments
|||
|||	    gs
|||	        Pointer to a GState object.
|||
|||	    abort
|||	        Set to TRUE to enable aborting, FALSE to disable.
|||
|||
|||	  Return Value
|||
|||	    Negative on error.
|||
|||	  Implementation
|||
|||	    Folio call implemented in gstate folio V32.
|||
|||	  Caveats
|||
|||	    If the triangle engine list is aborted, then the triangle
|||	    engine is reset. You should not assume the state of any
|||	    triangle engine registers at the start of the next frame.
|||
|||	  Associated Files
|||
|||	    <graphics/clt/gstate.h>
|||
|||	  See Also
|||
|||	    TE_CMD_EXECUTELIST(@)
|||
**/
/**
|||	AUTODOC -class GState -name GS_SetAbortVblankCount
|||	Specifies the quantity of vertical blanks to ignore before aborting
|||	a command list
|||
|||	  Synopsis
|||
|||	    Err GS_SetAbortVblankCount (GState* gs, int32 count)
|||
|||	  Description
|||
|||	    This function works in conjunction with GS_SetAbortVblank().  It
|||	    specifies the quantity of vertical blanking intervals to ignore
|||	    before aborting a commands list using the TE_ABORT_AT_VBLANK
|||	    feature.
|||
|||	    The default value is zero, i.e. no vertical blanks will be
|||	    ignored, and the command list will be aborted at the next
|||	    occurring one.
|||
|||	  Arguments
|||
|||	    gs
|||	        Pointer to a GState object.
|||
|||	    count
|||	        Set to the quantity of vertical blanks to ignore before
|||	        aborting the command list.  Negative values generate an
|||	        error.
|||
|||	  Return Value
|||
|||	    Negative on error.
|||
|||	  Implementation
|||
|||	    Folio call implemented in gstate folio V33.
|||
|||	  Associated Files
|||
|||	    <graphics/clt/gstate.h>
|||
|||	  See Also
|||
|||	    TE_CMD_SETVBLABORTCOUNT(@)
|||
**/
/**
|||	AUTODOC -class GState -name GS_SendIO
|||	Used to in low latency mode to start sending a list to the triangle engine
|||
|||	  Synopsis
|||
|||	    Err GS_SendIO(GState* g, bool wait, uint32 lenbytes)
|||
|||	  Description
|||
|||	    In low latency mode a command list may be prepared by the slave CPU. However
|||	    since the slave CPU can't issue the system call SendIO() it is up to the master
|||	    CPU to do the SendIO() and initiate execution of the list by the triangle engine.
|||
|||	    GS_SendIO() can be issued by the master CPU on behalf of the slave cPU after all
|||	    lists computed by the master CPU have been sent to the Triangle Engine. In the mean
|||	    time the slave may have begun computing this list.
|||
|||
|||	  Arguments
|||
|||	    gs
|||	        Pointer to a GState object.
|||	    wait
|||	        ==TRUE to spin the CPU while waiting for the TE to start. This is necessary
|||	        for apps that want to add things to the list immediately. If the app. has other
|||	        things to do then set this argument to FALSE and spin only before adding to the list.
|||	    lenbytes
|||	        The number of bytes in the command list that the TE should start executing.
|||
|||	  Return Value
|||
|||	    Negative on error.
|||
|||	  Implementation
|||
|||	    Folio call implemented in gstate folio V32.
|||
|||	  Associated Files
|||
|||	    <graphics/clt/gstate.h>
|||
|||	  See Also
|||
|||	     GS_LowLatency()
**/
/**
|||	AUTODOC -class GState -name GS_EndFrame
|||	Causes the Vid unit to start display the current bitmap after all rendering by TE has finished
|||
|||	  Synopsis
|||
|||	    Err GS_EndFrame(GState* g)
|||
|||	  Description
|||
|||	    GS_EndFrame() requests that the TE driver ask the display manager to display the
|||	    current bitmap when the TE has completed rendering.  This allows a program to
|||	    asynchronously request the buffer switch without having to wait for the TE to
|||	    complete the last buffer issued with GS_SendList().
|||
|||	    The View to which the current Bitmap should be posted is
|||	    specified using GS_SetView().  If no View has been specified,
|||	    this call is effectively a no-op.
|||
|||	  Arguments
|||
|||	    gs
|||	        Pointer to a GState object.
|||
|||	  Return Value
|||
|||	    Negative on error.
|||
|||	  Implementation
|||
|||	    Folio call implemented in gstate folio V32.
|||
|||	  Associated Files
|||
|||	    <graphics/clt/gstate.h>
|||
|||	  See Also
|||
|||	     GS_SendList(), GS_SetView(), TE_CMD_DISPLAYFRAMEBUFFER(@)
**/
/**
|||	AUTODOC -class GState -name GS_IsLowLatency
|||	Returns TRUE if is the GState is currently set in Low Latency mode.
|||
|||	  Synopsis
|||
|||	    bool GS_IsLowLatency(GState* g)
|||
|||	  Description
|||
|||	    Returns the value TRUE if is the GState is currently set in Low Latency mode.
|||
|||	  Arguments
|||
|||	    gs
|||	        Pointer to a GState object.
|||
|||	  Return Value
|||
|||	    TRUE or FALSE
|||
|||	  Implementation
|||
|||	    Folio call implemented in gstate folio V32.
|||
|||	  Associated Files
|||
|||	    <graphics/clt/gstate.h>
|||
|||	  See Also
|||
|||	     GS_LowLatency()
**/
/**
|||	AUTODOC -class GState -name GS_LowLatency
|||	Places a GState in low latency mode
|||
|||	  Synopsis
|||
|||	    Err GS_LowLatency(GState* g, bool send, uint32 latency)
|||
|||	  Description
|||
|||	    In low latency mode the CPU computes commands just ahead of the triangle engine's consumption
|||	    of the commands. After the CPU has computed a number of words, called the "latency", of commands
|||	    it sends them to the triangle engine. When the end of the buffer is reached a jump instruction is
|||	    placed in the list and the CPU and triangle engine proceed to write and read commands respectively
|||	    at the top of the buffer.
|||
|||	    The "send" option enables the CPU to automatically initiate the SendIO() operation the first time
|||	    data is to be sent to the TE. After the TE is running and following the CPU the SendIO() operation
|||	    isn't used to advance and synchronize the TE. Instead the light weight sytem calls
|||	    SetTEWritePointer() and GetTEReadPointer() are used.
|||
|||	    The GState remains in low latency until a call is made to GS_SendLastList(). This means that this
|||	    function must be called once per frame.
|||
|||	  Arguments
|||
|||	    gs
|||	        Pointer to a GState object.
|||
|||	    send
|||	       == TRUE if GS_SendIO() is to be called automatically by GS_SendList().
|||	       If this argument is FALSE then it is up to a task on the master CPU to issue the
|||	       call to GS_SendIO()
|||
|||	    latency
|||	       The number of words that the CPU is to stay ahead of the triangle engine. A smaller
|||	       value for this parameter implies that GS_SendList() will be called more frequently
|||	       and hence there will be more overhead and hence a degradation of performance.
|||
|||
|||	  Return Value
|||
|||	    Negative on error.
|||
|||	  Implementation
|||
|||	    Folio call implemented in gstate folio V32.
|||
|||	  Associated Files
|||
|||	    <graphics/clt/gstate.h>
|||
|||	  See Also
|||
|||	    GS_SendIO(), GS_SendLastList(), GS_SendList()
**/
/**
|||	AUTODOC -class GState -name GS_EnableProfiling
|||	Turns on GState profiling which allows a user function to be called when certain events occur
|||
|||	  Synopsis
|||
|||	    Err GS_EnableProfiling(GState* g, GSProfileFunc profile_function)
|||
|||	  Description
|||
|||	    When GState profiling is enabled, the function profile_function is called with the following arguments,
|||	    the GState in use, and an integer. When a SendIO is initiated for the BDA the integer is 1, when the IORequest
|||	    completes, the integer is -1. When the GState is waiting for the VDL to switch the integer is 2, when the
|||	    wait is complete the integer is -2.
|||
|||
|||	  Arguments
|||
|||	    gs
|||	        Pointer to a GState object.
|||
|||	    profile_function
|||	       A function declared as for example, void ProfileFunc(GState *g, int32 arg).
|||	       This function is called when profiling events occur.
|||
|||
|||	  Return Value
|||
|||	    == 0 on success
|||
|||	  Implementation
|||
|||	    Folio call implemented in gstate folio V32.
|||
|||	  Associated Files
|||
|||	    <graphics/clt/gstate.h>
|||
|||	  See Also
|||
|||	    GS_DisableProfiling()
**/
/**
|||	AUTODOC -class GState -name GS_DisableProfiling
|||	Turns off profiling for a GState
|||
|||	  Synopsis
|||
|||	    Err GS_DisableProfiling(GState* g)
|||
|||	  Description
|||
|||	    This function turns off profiling mode for the GState and returns it to a mode where
|||	    the profiling function is not invoked when events occur.
|||
|||	  Arguments
|||
|||	    gs
|||	        Pointer to a GState object.
|||
|||
|||	  Return Value
|||
|||	    0 on success
|||
|||	  Implementation
|||
|||	    Folio call implemented in gstate folio V32.
|||
|||	  Associated Files
|||
|||	    <graphics/clt/gstate.h>
|||
|||	  See Also
|||
|||	    GS_EnableProfiling()
**//* Keep the compiler happy */
extern int foo;
