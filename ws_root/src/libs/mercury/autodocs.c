/* @(#) autodocs.c 96/10/18 1.20 */

/**
|||	AUTODOC -class Mercury -name M_Init
|||	M_Init initializes the system for rendering. 
|||	It must be called once before any other Mercury functions are called.
|||	It allocates, initializes and returns a CloseData structure that can
|||	subsequently be used for rendering. 
|||
|||	  Synopsis
|||	    CloseData * M_Init (uint32 nverts, uint32 clistsize, GState *gs);
|||
|||	  Description
|||	    M_Init initializes the system for rendering. 
|||	    It must be called once before any other Mercury functions are called.
|||	    It allocates, initializes and returns a CloseData structure that can
|||	    subsequently be used for rendering. 
|||
|||	  Arguments
|||	    nverts
|||	        The maximum number of vertices to be used in a POD.
|||	    clistsize
|||	        The number of words that provide a margin of error for command lists. Normally this
|||	        is set to 0.
|||	    gs
|||	        The GState object used to send command lists to the TE.
|||
|||	  Return Value
|||	    CloseData*
|||	        Pointer to the new CloseData structure.
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <mercury.h>, mercury/lib
|||
|||	  See Also
|||	    M_End(), M_Sort(), M_Draw()
|||	    M_PreLight(), M_BoundsTest()
|||
**/
/**
|||	AUTODOC -class Mercury -name M_End
|||	The M_End function frees all memory previously allocated by the M_Init 
|||	function, including the memory allocated to the CloseData structure
|||	initializes by M_Init.
|||
|||	  Synopsis
|||	    void M_End(CloseData *pclosedata);
|||
|||	  Description
|||	    The M_End function frees all memory previously allocated by the M_Init 
|||	    function, including the memory allocated to the CloseData structure
|||	    initializes by M_Init. If the CloseData is used for MP operations the MP
|||	    data is automatically freed.
|||
|||	  Arguments
|||	    pclosedata
|||	      A value returned by a previous call to M_Init.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <mercury.h>, mercury/lib
|||
|||	  See Also
|||	    M_Init(), M_InitMP(), M_Sort(), M_Draw()
|||	    M_PreLight(), M_BoundsTest()
|||
**/
/**
|||	AUTODOC -class Mercury -name M_EndMP
|||	The M_End function frees all memory associated for MP operations on the CloseData
|||	After calling this function the CloseData can no longer be used for MP rendering.
|||	
|||
|||	  Synopsis
|||	    void M_EndMP(CloseData *pclosedata);
|||
|||	  Description
|||	    The M_End function frees all memory used for MP operations on the Closedata.
|||
|||	  Arguments
|||	    pclosedata
|||	      A value returned by a previous call to M_Init.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <mercury.h>, mercury/lib
|||
|||	  See Also
|||	    M_InitMP()
|||
**/
/**
|||	AUTODOC -class Mercury -name M_Sort
|||	The M_Sort function sorts the linked list of PODs passed in according 
|||	to each field in the POD structure. The linked list of PODs that
|||	exists prior to a call to M_Sort may be arranged in a different
|||	order when M_Sort returns. M_Sort returns a pointer to the first
|||	pod in the sorted list.
|||
|||	  Synopsis
|||	    Pod * M_Sort (uint32 count, Pod *podlist, Pod **plastpod);
|||
|||	  Description
|||	    The M_Sort function sorts the linked list of PODs passed in according 
|||	    to each field in the POD structure. The linked list of PODs that
|||	    exists prior to a call to M_Sort may be arranged in a different
|||	    order when M_Sort returns. M_Sort returns a pointer to the first
|||	    pod in the sorted list.
|||
|||	  Arguments
|||	    count
|||	      The number of PODs passed in.
|||	    podlist
|||	      The first POD in a linked list of PODs.
|||	    plastpod
|||	      A pointer to a pointer to the last POD.
|||
|||	  Return Value
|||	    Pod*
|||	      A pointer to the first POD.
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <mercury.h>, mercury/lib
|||
|||	  See Also
|||	    M_Init(), M_End(), M_Draw()
|||	    M_PreLight(), M_BoundsTest()
|||
**/
/**
|||	AUTODOC -class Mercury -name M_Draw
|||	The M_Draw function takes a linked list of PODs as inputs, and
|||	outputs the corresponding M2 command list for rendering the PODs.
|||	It is the responsibility of your application to send all other M2
|||	command lists for setting up the state of the M2 Triangle Engine
|||	state and for producing any effects. The command list produced by
|||	M_Draw consists of copies of command lists that setup the triangle
|||	engine for a case, load or select textures, supplied in the POD
|||	data structures, and vertex commands for drawing triangles.
|||	M_Draw returns the number of triangles that actually are drawn to
|||	the screen. This number does not include rejected triangles. But
|||	it does include triangles created as a result of clipping.
|||
|||	  Synopsis
|||	    uint32 M_Draw (Pod *firstpod, CloseData *pclose);
|||
|||	  Description
|||	    The M_Draw function takes a linked list of PODs as inputs, and
|||	    outputs the corresponding M2 command list for rendering the PODs.
|||	    It is the responsibility of your application to send all other M2
|||	    command lists for setting up the state of the M2 Triangle Engine
|||	    state and for producing any effects. The command list produced by
|||	    M_Draw consists of copies of command lists that setup the triangle
|||	    engine for a case, load or select textures, supplied in the POD
|||	    data structures, and vertex commands for drawing triangles.
|||	    
|||	    M_Draw returns the number of triangles that actually are drawn to
|||	    the screen. This number does not include rejected triangles. But
|||	    it does include triangles created as a result of clipping.
|||
|||	  Arguments
|||	    firstpod
|||	        The first pod in a linked list of PODs in which all the 
|||	        changed flags are set as per M_Sort.
|||	    pclose
|||	        A CloseData area which has been properly initialized.
|||
|||	  Return Value
|||	    uint32
|||	        A count of displayed triangles in the low 16 bits. A count of the
|||	        clipped triangles in the high 16 bits.
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <mercury.h>, mercury/lib
|||
|||	  See Also
|||	    M_DrawEnd(), M_Init(), M_End(), M_Sort(),
|||	    M_PreLight(), M_BoundsTest()
|||
**/
/**
|||	AUTODOC -class Mercury -name M_DrawPod
|||	The M_Draw function takes a pointer to a single pod as input and 
|||	outputs the corresponding M2 command list for the POD.
|||
|||	  Synopsis
|||	    uint32 M_DrawPod (Pod *ppod, CloseData *pclose);
|||
|||	  Description
|||	    The M_DrawPod function takes a pointer to a single POD as input, and
|||	    outputs the corresponding M2 command list for that POD.
|||	    Before calling this function the values in the passed CloseData structure
|||	    pVIwrite points to the address where the command lists are written and
|||	    pVIwritemax is the maximum address where command lists can be written.
|||
|||	  Arguments
|||	    ppod
|||	        The address of the pod to render.
|||	    pclose
|||	        A CloseData area which has been properly initialized.
|||
|||	  Return Value
|||	    uint32
|||	        A count of displayed triangles in the low 16 bits. A count of the
|||	        clipped triangles in the high 16 bits.
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <mercury.h>, mercury/lib
|||
|||	  See Also
|||	    M_DrawEnd(), M_Init(), M_End()
|||	    M_PreLight(), M_BoundsTest()
|||
**/
/**
|||	AUTODOC -class Mercury -name M_DrawEnd
|||	The M_DrawEnd function is used at the end of each frame. 
|||
|||	  Synopsis
|||	    uint32 M_DrawEnd (CloseData *pclose);
|||
|||	  Description
|||	    M_DrawEnd is only needed when M_InitMP() is used.
|||	    It switches the command list buffer used in the CloseData.
|||	    This is necessary when doing MP because the TE might be rendering 
|||	    to the current buffer.
|||
|||	  Arguments
|||	    pclose
|||	        A CloseData area which has been properly initialized.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <mercury.h>, mercury/lib
|||
|||	  See Also
|||	    M_InitMP(), M_Draw()
|||
**/
/**
|||	AUTODOC -class Mercury -name M_BuildAAEdgeTable
|||	The M_BuildAAEdgeTable function is used to enable a Pod to render with the antialiasing setup functions.
|||
|||	  Synopsis
|||	    void M_BuildAAEdgeTable (Pod *ppod);
|||
|||	  Description
|||	    M_BuildAAEdgeTable is only needed when an antialiasing setup function is used as the pcase for the Pod.
|||	    It allocates, and initializes, the data pointed to by the paaedge field on the Pod's geometry structure 
|||	    and the AAData structure pointed to by the Pod's paadata field.
|||	    All the data allocated by this function is freed by calling M_FreeAAEdgeTable().
|||
|||
|||	  Arguments
|||	    ppod
|||	        The Pod for which the edge information is computed.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.1
|||
|||	  Associated Files
|||	    <mercury.h>, mercury/lib
|||
|||	  See Also
|||	    M_FreeAAEdgeTable()
|||
**/
/**
|||	AUTODOC -class Mercury -name M_FreeAAEdgeTable
|||	The M_FreeAAEdgeTable function frees all data allocated on a Pod by M_BuildAAEdgeTable()
|||
|||	  Synopsis
|||	    void M_FreeAAEdgeTable (Pod *ppod);
|||
|||	  Description
|||	    M_FreeAAEdgeTable frees all data allocated on a Pod to do antialising, namely, 
|||	    the data pointed to by the paaedge field on the Pod's geometry structure and the 
|||	    AAData structure pointed to by the Pod's paadata field.
|||
|||
|||	  Arguments
|||	    ppod
|||	        The Pod for which the edge information is freed.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.1
|||
|||	  Associated Files
|||	    <mercury.h>, mercury/lib
|||
|||	  See Also
|||	    M_BuildAAEdgeTable()
|||
**/
/**
|||	AUTODOC -class Mercury -name M_InitMP
|||	The M_InitMP function is used once to change the CloseData structure into MP mode.
|||
|||	  Synopsis
|||	    Err M_InitMP(CloseData *pclosedata, uint32 maxpods, uint32 clistwords, uint32 numverts)
|||
|||	  Description
|||	    The M_InitMP function allocates necessary buffers and 2 command lists for each CPU
|||	    to allow Mercury to generate command lists on both CPUs simultaneously. 
|||	    The command lists generated are interwoven with each other and to the command list of the GState
|||	    with JUMP commands.
|||	    When using M_InitMP() a small command list can be used on the GState because all the
|||	    Mercury commands are kept on the CloseData. Note, the command lists generated in MP mode
|||	    must be large enough to hold the entire scene. If they are not large enough, Mercury reverts to
|||	    single processor mode.
|||	    All buffer areas allocated with this call are freed when M_EndMP() is called.
|||
|||	  Arguments
|||	    pclose
|||	        A CloseData area which has been properly initialized. This CloseData becomes MP capable and can
|||	        not be reverted back to single processor operation.
|||	    maxpods
|||	        The maximum number of Pods that will be passed to M_Draw().
|||	    clistwords
|||	        The size, in words, of the command lists that is to be allocated.
|||	    numverts
|||	        The number of vertices that each CPU should consider when dequeuing Pods to render. A small value will 
|||	        result in larger overhead while a large value will starve a CPU. Passing a value of 0 will result
|||	        in the default value of 500 to be used.
|||
|||	  Return Value
|||	    0 if successful
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <mercury.h>, mercury/lib
|||
|||	  See Also
|||	    M_DrawEnd(), M_EndMP()
|||
**/
/**
|||	AUTODOC -class Mercury -name M_Prelight
|||	For each POD in a linked list provided to Mercury, 
|||	the M_PreLight function computes the rgb values that
|||	result from the lighting calculation of the affected
|||	POD and places the result in a corresponding POD in your
|||	application’s destination list. The source POD provided
|||	to the M_Prelight function must have normal vectors in its
|||	geometry data fields. When the M_Prelight function returns,
|||	the destination POD has rgb values in its geometry data fields.
|||
|||	  Synopsis
|||	    void M_PreLight (Pod* src, Pod* dst, CloseData* pclose);
|||
|||	  Description
|||	    For each POD in a linked list provided to Mercury, 
|||	    the M_PreLight function computes the rgb values that
|||	    result from the lighting calculation of the affected
|||	    POD and places the result in a corresponding POD in your
|||	    application’s destination list. The source POD provided
|||	    to the M_Prelight function must have normal vectors in its
|||	    geometry data fields. When the M_Prelight function returns,
|||	    the destination POD has rgb values in its geometry data fields.
|||
|||	  Arguments
|||	    src
|||	      The source POD list.
|||	    dst
|||	      The destination POD list.
|||	    pclose
|||	      A CloseData area which has been properly initialized.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <mercury.h>, mercury/lib
|||
|||	  See Also
|||	    M_Init(), M_End(), M_Sort(), M_Draw(),
|||	    M_BoundsTest()
|||
**/
/**
|||	AUTODOC -class Mercury -name M_BoundsTest
|||	The M_BoundsTest function tests each bounding box in a list
|||	of boxes and sets the flags field to indicate whether the box
|||	is visible or not.
|||
|||	  Synopsis
|||	    void M_BoundsTest (BBoxList* bbox, CloseData* pclose);
|||
|||	  Description
|||	    The M_BoundsTest function tests each bounding box in a list
|||	    of boxes and sets the flags field to indicate whether the box
|||	    is visible or not.
|||
|||	  Arguments
|||	    bbox
|||	      A list of bounding boxes to test.
|||	    pclose
|||	      A CloseData area which has been properly initialized.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <mercury.h>, mercury/lib
|||
|||	  See Also
|||	    M_Init(), M_End(), M_Sort(), M_Draw(),
|||	    M_PreLight()
|||
**/
/**
|||	AUTODOC -class Vector -name Vector3D_Construct
|||	Creates an instance of Vector3D.
|||
|||	  Synopsis
|||	    Vector3D* Vector3D_Construct(void);
|||
|||	  Description
|||	    Creates an instance of Vector3D.
|||
|||	  Arguments
|||	    void
|||
|||	  Return Value
|||	    Vector3D*
|||	        Pointer to the new Vector3D structure.
|||
|||	  Implementation
|||	    Verion 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Vector3D_Destruct()
|||
**/
/**
|||	AUTODOC -class Vector -name Vector3D_Destruct
|||	Deletes a Vector3D instance.
|||
|||	  Synopsis
|||	    void Vector3D_Destruct( Vector3D *v );
|||
|||	  Description
|||	    Deletes a Vector3D instance.
|||
|||	  Arguments
|||	    *v
|||	        Pointer to the Vector3D structure to be deleted.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Verion 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Vector3D_Construct()
|||
**/
/**
|||	AUTODOC -class Vector -name Vector3D_Set
|||	Initializes a Vector3D struct.
|||
|||	  Synopsis
|||	    void Vector3D_Set(Vector3D *v, float x, float y, float z);
|||
|||	  Description
|||	    Initializes a Vector3D struct.
|||
|||	  Arguments
|||	    *v
|||	        Pointer to a Vector3D structure
|||	    x
|||	        The new X value for the Vector3D
|||	    y
|||	        The new Y value for the Vector3D
|||	    z
|||	        The new Z value for the Vector3D
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Vector3D_Construct()
|||	    Vector3D_GetX(), Vector3D_GetY(), Vector3D_GetZ()
|||
**/
/**
|||	AUTODOC -class Vector -name Vector3D_GetX
|||	Returns the X component of a Vector3D struct.
|||
|||	  Synopsis
|||	    float Vector3D_GetX(Vector3D *v);
|||
|||	  Description
|||	    Returns the X component of a Vector3D struct.
|||
|||	  Arguments
|||	    *v
|||	        Pointer to a Vector3D structure.
|||
|||	  Return Value
|||	    float
|||	        X value.
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Vector3D_Construct()
|||	    Vector3D_GetY(), Vector3D_GetZ()
|||
**/
/**
|||	AUTODOC -class Vector -name Vector3D_GetY
|||	Returns the Y component of a Vector3D struct.
|||
|||	  Synopsis
|||	    float Vector3D_GetY(Vector3D *v);
|||
|||	  Description
|||	    Returns the Y component of a Vector3D struct.
|||
|||	  Arguments
|||	    *v
|||	        Pointer to a Vector3D structure.
|||
|||	  Return Value
|||	    float
|||	        Y value.
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Vector3D_Construct()
|||	    Vector3D_GetX(), Vector3D_GetZ()
|||
**/
/**
|||	AUTODOC -class Vector -name Vector3D_GetZ
|||	Returns the Z component of a Vector3D struct.
|||
|||	  Synopsis
|||	    float Vector3D_GetZ(Vector3D *v);
|||
|||	  Description
|||	    Returns the Z component of a Vector3D struct.
|||
|||	  Arguments
|||	    *v
|||	        Pointer to a Vector3D structure.
|||
|||	  Return Value
|||	    float
|||	        Z value.
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Vector3D_Construct()
|||	    Vector3D_GetX(), Vector3D_GetZ()
|||
**/
/**
|||	AUTODOC -class Vector -name Vector3D_Average
|||	Calculated the average of two vectors.
|||
|||	  Synopsis
|||	    void Vector3D_Average(Vector3D *v, Vector3D *a, Vector3D *b);
|||
|||	  Description
|||	    Calculated the average of two vectors.
|||
|||	  Arguments
|||	    *v
|||	        Pointer to the destination Vector3D structure
|||	    *a
|||	        Pointer to first source Vector3D structure
|||	    *b
|||	        Pointer to second source Vector3D structure
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Vector3D_Construct()
|||
**/
/**
|||	AUTODOC -class Vector -name Vector3D_Negate
|||	Negates the given vector.
|||
|||	  Synopsis
|||	    void Vector3D_Negate(Vector3D *v);
|||
|||	  Description
|||	    Negates the given vector.
|||
|||	  Arguments
|||	    *v
|||	        Pointer to the destination Vector3D structure
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Vector3D_Construct()
|||
**/
/**
|||	AUTODOC -class Vector -name Vector3D_Dot
|||	Calculated the dot product between two vectors.
|||
|||	  Synopsis
|||	    float Vector3D_Dot(Vector3D *v, Vector3D *a);
|||
|||	  Description
|||	    Calculated the dot product between two vectors.
|||
|||	  Arguments
|||	    *v
|||	        Pointer to the destination Vector3D structure.
|||	    *a
|||	        Pointer to the source Vector3D structure.
|||
|||	  Return Value
|||	    float
|||	        dot product value
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Vector3D_Construct()
|||
**/
/**
|||	AUTODOC -class Vector -name Vector3D_Length
|||	Calculated the length of a vector.
|||
|||	  Synopsis
|||	    float Vector3D_Length(Vector3D *v);
|||
|||	  Description
|||	    Calculated the length of a vector.
|||
|||	  Arguments
|||	    *v
|||	        Pointer to the destination Vector3D structure.
|||
|||	  Return Value
|||	    float
|||	        length value.
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Vector3D_Construct()
|||
**/
/**
|||	AUTODOC -class Vector -name Vector3D_Normalize
|||	Normalize the given vector.
|||
|||	  Synopsis
|||	    float Vector3D_Normalize(Vector3D *v);
|||
|||	  Description
|||	    Normalize the given vector.
|||
|||	  Arguments
|||	    *v
|||	        Pointer to the destination Vector3D structure.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Vector3D_Construct()
|||
**/
/**
|||	AUTODOC -class Vector -name Vector3D_Minimum
|||	Find the minimum vector from two source vectors.
|||
|||	  Synopsis
|||	    void Vector3D_Minimum(Vector3D *v, Vector3D *a, Vector3D *b);
|||
|||	  Description
|||	    Find the minimum vector from two source vectors.
|||
|||	  Arguments
|||	    *v
|||	        Pointer to the destination Vector3D structure.
|||	    *a
|||	        Pointer to the first source Vector3D structure.
|||	    *b
|||	        Pointer to the second source Vector3D structure.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Vector3D_Construct()
|||
**/
/**
|||	AUTODOC -class Vector -name Vector3D_Maximum
|||	Find the maximum vector from two source vectors.
|||
|||	  Synopsis
|||	    void Vector3D_Maximum(Vector3D *v, Vector3D *a, Vector3D *b);
|||
|||	  Description
|||	    Find the minimum vector from two source vectors.
|||
|||	  Arguments
|||	    *v
|||	        Pointer to the destination Vector3D structure.
|||	    *a
|||	        Pointer to the first source Vector3D structure.
|||	    *b
|||	        Pointer to the second source Vector3D structure.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Vector3D_Construct()
|||
**/
/**
|||	AUTODOC -class Vector -name Vector3D_Compare
|||	Compare two source vectors.
|||
|||	  Synopsis
|||	    bool Vector3D_Compare(Vector3D *v, Vector3D *a);
|||
|||	  Description
|||	    Compare two source vectors.
|||
|||	  Arguments
|||	    *v
|||	        Pointer to the first source Vector3D structure.
|||	    *a
|||	        Pointer to the second source Vector3D structure.
|||
|||	  Return Value
|||	    bool
|||	        TRUE Both vectors are exactly equal.
|||	        FALSE Both vectors are not equal.
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Vector3D_Construct()
|||
**/
/**
|||	AUTODOC -class Vector -name Vector3D_CompareFuzzy
|||	Compare two source vectors with fuzzy edges.
|||
|||	  Synopsis
|||	    bool Vector3D_CompareFuzzy(Vector3D *v, Vector3D *a,
|||	                                float fuzzy);
|||
|||	  Description
|||	    Compare two source vectors with fuzzy edges.
|||
|||	  Arguments
|||	    *v
|||	        Pointer to the first source Vector3D structure.
|||	    *a
|||	        Pointer to the second source Vector3D structure.
|||	    fuzzy
|||	        The fuzzy tolerance level.
|||
|||	  Return Value
|||	    bool
|||	        TRUE Both vectors are exactly equal.
|||	        FALSE Both vectors are not equal.
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Vector3D_Construct()
|||
**/
/**
|||	AUTODOC -class Vector -name Vector3D_Copy
|||	Copy a source vector to a destination vector.
|||
|||	  Synopsis
|||	    void Vector3D_Copy(Vector3D *v, Vector3D *a);
|||
|||	  Description
|||	    Copy a source vector to a destination vector.
|||
|||	  Arguments
|||	    *v
|||	        Pointer to the destination Vector3D structure.
|||	    *a
|||	        Pointer to the source Vector3D structure.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Vector3D_Construct()
|||
**/
/**
|||	AUTODOC -class Vector -name Vector3D_Add
|||	Add a source vector to a destination vector.
|||
|||	  Synopsis
|||	    void Vector3D_Add(Vector3D *v, Vector3D *a, Vector3D *b);
|||
|||	  Description
|||	    Add two vectors and store the result in a destination vector.
|||
|||	  Arguments
|||	    *v
|||	        Pointer to the destination Vector3D structure.
|||	    *a
|||	        Pointer to the first Vector3D structure.
|||	    *b
|||	        Pointer to the second Vector3D structure.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Vector3D_Construct()
|||
**/
/**
|||	AUTODOC -class Vector -name Vector3D_Subtract
|||	Subtract two vectors and store the result in a destination vector.
|||
|||	  Synopsis
|||	    void Vector3D_Subtract( Vector3D *v, Vector3D *a, Vector3D *b);
|||
|||	  Description
|||	    Subtract two vectors and store the result in a destination vector.
|||	    [ v = a - b ]
|||
|||	  Arguments
|||	    *v
|||	        Pointer to the destination Vector3D structure.
|||	    *a
|||	        Pointer to the first source Vector3D structure.
|||	    *b
|||	        Pointer to the second source Vector3D structure.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Vector3D_Construct()
|||
**/
/**
|||	AUTODOC -class Vector -name Vector3D_Scale
|||	Scale a vector.
|||
|||	  Synopsis
|||	    void Vector3D_Scale( Vector3D *v, float x, float y, float z );
|||
|||	  Description
|||	    Scale a vector in x,y & z.
|||	    [ v.x *= x; v.y *= y; v.z *= z ]
|||
|||	  Arguments
|||	    *v
|||	        Pointer to the destination Vector3D structure.
|||	    x
|||	        X scale value.
|||	    y
|||	        Y scale value.
|||	    z
|||	        Z scale value.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Vector3D_Construct()
|||
**/
/**
|||	AUTODOC -class Vector -name Vector3D_Multiply
|||	Multiply two source vectors and place result in a destination vector.
|||
|||	  Synopsis
|||	    void Vector3D_Multiply(Vector3D *v, Vector3D *a, Vector3D *b);
|||
|||	  Description
|||	    Multiply two source vectors and place result in a destination vector.
|||	    [ v.x = a.x * b.x ]
|||	    [ v.y = a.y * b.y ]
|||	    [ v.z = a.z * b.z ]
|||
|||	  Arguments
|||	    *v
|||	        Pointer to the destination Vector3D structure.
|||	    *a
|||	        Pointer to the first source Vector3D structure.
|||	    *b
|||	        Pointer to the second source Vector3D structure.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Vector3D_Construct()
|||
**/
/**
|||	AUTODOC -class Vector -name Vector3D_Cross
|||	Calculates the cross product between two vectors.
|||
|||	  Synopsis
|||	    void Vector3D_Multiply(Vector3D *v, Vector3D *a, Vector3D *b);
|||
|||	  Description
|||	    Calculates the cross product between two vectors and places result in
|||	    destination vector.
|||
|||	  Arguments
|||	    *v
|||	        Pointer to the destination Vector3D structure.
|||	    *a
|||	        Pointer to the first source Vector3D structure.
|||	    *b
|||	        Pointer to the second source Vector3D structure.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Vector3D_Construct()
|||
**/
/**
|||	AUTODOC -class Vector -name Vector3D_Zero
|||	Zeros the vector.
|||
|||	  Synopsis
|||	    void Vector3D_Multiply(Vector3D *v, Vector3D *a, Vector3D *b);
|||
|||	  Description
|||	    Zeros the vector.
|||	    [ v.x = 0; v.y = 0; v.z = 0 ]
|||
|||	  Arguments
|||	    *v
|||	        Pointer to the destination Vector3D structure.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Vector3D_Construct()
|||
**/
/**
|||	AUTODOC -class Vector -name Vector3D_Print
|||	Prints the vector.
|||
|||	  Synopsis
|||	    void Vector3D_Print(Vector3D *v );
|||
|||	  Description
|||	    Prints the vector to standard output.
|||
|||	  Arguments
|||	    *v
|||	        Pointer to the destination Vector3D structure.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Vector3D_Construct()
|||
**/
/**
|||	AUTODOC -class Vector -name Vector3D_MultiplyByMatrix
|||	Multiplies the vector by a matrix.
|||
|||	  Synopsis
|||	    void Vector3D_MultiplyByMatrix( Vector3D *v, Matrix *m );
|||
|||	  Description
|||	    Multiplies the vector by a matrix.
|||
|||	  Arguments
|||	    *v
|||	        Pointer to the destination Vector3D structure.
|||	    *m
|||	        Pointer to the Matrix structure.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Vector3D_Construct()
|||
**/
/**
|||	AUTODOC -class Vector -name Vector3D_OrientateByMatrix
|||	Multiplies the vector by the orientation of the matrix.
|||
|||	  Synopsis
|||	    void Vector3D_OrientateByMatrix( Vector3D *v, Matrix *m );
|||
|||	  Description
|||	    Multiplies the vector by the orientation of the matrix.
|||
|||	  Arguments
|||	    *v
|||	        Pointer to the destination Vector3D structure.
|||	    *m
|||	        Pointer to the Matrix structure.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Vector3D_Construct()
|||
**/




/**
|||	AUTODOC -class Matrix -name Matrix_Construct
|||	Allocates an instance of a matrix and sets it to identity.
|||
|||	  Synopsis
|||	    Matrix* Matrix_Construct( void );
|||
|||	  Description
|||	    Allocates an instance of a matrix and sets it to identity.
|||
|||	  Arguments
|||	    void
|||
|||	  Return Value
|||	    Matrix*
|||	        Pointer to the new Matrix structure.
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_Destruct()
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_Destruct
|||	Frees the givem matrix from memory.
|||
|||	  Synopsis
|||	    void Matrix_Destruct( Matrix *m );
|||
|||	  Description
|||	    Frees the givem matrix from memory.
|||
|||	  Arguments
|||	    m*
|||	        Pointer to the Matrix structure to me deleted.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_Construct()
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_Normalize
|||	Normalize the given matrix.
|||
|||	  Synopsis
|||	    void Matrix_Normalize( Matrix *m );
|||
|||	  Description
|||	    Normalize the given matrix.
|||
|||	  Arguments
|||	    m*
|||	        Pointer to the destination Matrix structure.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_Construct(), Matrix_Zero()
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_Copy
|||	Copyies from a source matrix to a detination matrix.
|||
|||	  Synopsis
|||	    void Matrix_Copy( Matrix *m, Matrix *a );
|||
|||	  Description
|||	    Copyies from a source matrix to a detination matrix.
|||
|||	  Arguments
|||	    m*
|||	        Pointer to the destination Matrix structure.
|||	    a*
|||	        Pointer to the source Matrix structure.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_Construct()
|||	    Matrix_Copy(), Matrix_CopyOrientation(), Matrix_CopyTranslation()
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_Mult
|||	Matrix multiply two source matrices.
|||
|||	  Synopsis
|||	    void Matrix_Mult( Matrix *m, Matrix *a, Matrix *b );
|||
|||	  Description
|||	    Matrix multiply two source matrices.
|||	        [ m = a x b ]
|||
|||	  Arguments
|||	    m*
|||	        Pointer to the destination Matrix structure.
|||	    a*
|||	        Pointer to the first source Matrix structure.
|||	    a*
|||	        Pointer to the second source Matrix structure.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_Construct()
|||	    Matrix_Multiply(), Matrix_MultOrientation(), Matrix_MultiplyOrientation()
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_Multiply
|||	Multiplies a source matrix with the destination matrix.
|||
|||	  Synopsis
|||	    void Matrix_Multiply( Matrix *m, Matrix *a );
|||
|||	  Description
|||	    Multiplies a source matrix with the destination matrix.
|||	        [ m = m x a ]
|||
|||	  Arguments
|||	    m*
|||	        Pointer to the destination Matrix structure.
|||	    a*
|||	        Pointer to the source Matrix structure.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_Construct()
|||	    Matrix_Mult(), Matrix_MultOrientation(), Matrix_MultiplyOrientation()
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_MultOrientation
|||	Matrix multiply the orientation section of two source matrices.
|||
|||	  Synopsis
|||	    void Matrix_MultOrientation( Matrix *m, Matrix *a, Matrix *b );
|||
|||	  Description
|||	    Matrix multiply the orientation section of two source matrices.
|||	    [ m = a x b ]
|||
|||	  Arguments
|||	    m*
|||	        Pointer to the destination Matrix structure.
|||	    a*
|||	        Pointer to the first source Matrix structure.
|||	    b*
|||	        Pointer to the second destination Matrix structure.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_Construct()
|||	    Matrix_Mult(), Matrix_Multiply(), Matrix_MultiplyOrientation()
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_MultiplyOrientation
|||	Multiplies the orientation section of a source matrix with the destination matrix.
|||
|||	  Synopsis
|||	    void Matrix_MultiplyOrientation( Matrix *m, Matrix *a );
|||
|||	  Description
|||	    Multiplies the orientation section of a source matrix with the
|||	    destination matrix.
|||	        [ m = m x a ]
|||
|||	  Arguments
|||	    m*
|||	        Pointer to the destination Matrix structure.
|||	    a*
|||	        Pointer to the source Matrix structure.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_Construct()
|||	    Matrix_Mult(), Matrix_Multiply(), Matrix_MultOrientation()
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_Zero
|||	Zeros the contents of a matrix.
|||
|||	  Synopsis
|||	    void Matrix_Zero( Matrix *m );
|||
|||	  Description
|||	    Zeros the contents of a matrix.
|||
|||	  Arguments
|||	    m*
|||	        Pointer to the destination Matrix structure.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_Construct()
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_Identity
|||	Sets the matrix to identity.
|||
|||	  Synopsis
|||	    Matrix_Identity( Matrix *m );
|||
|||	  Description
|||	    Sets the matrix to identity.
|||
|||	  Arguments
|||	    m*
|||	        Pointer to the destination Matrix structure.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_Construct(), Matrix_Normalize()
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_GetTranslation
|||	Copy the transformation information from the matrix to a vector.
|||
|||	  Synopsis
|||	    void Matrix_GetTranslation( Matrix *m, Vector3D *v );
|||
|||	  Description
|||	    Copy the transformation information from the matrix to a vector.
|||
|||	  Arguments
|||	    m*
|||	        Pointer to the source Matrix structure.
|||	    v*
|||	        Pointer to the destination Vector3D structure.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_Construct(), Matrix_GetpTranslation(),
|||	    Matrix_SetTranslationByVector(), Matrix_SetTranslation(),
|||	    Matrix_TranslateByVector(), Matrix_Translate(),
|||	    Matrix_MoveByVector(), Matrix_Move()
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_SetTranslationByVector
|||	Set a matrix's translation from a vector.
|||
|||	  Synopsis
|||	    void Matrix_SetTranslationByVector( Matrix *m, Vector3D *v );
|||
|||	  Description
|||	    Set a matrix's translation from a vector.
|||
|||	  Arguments
|||	    m*
|||	        Pointer to the destination Matrix structure.
|||	    v*
|||	        Pointer to the source Vector3D structure.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_Construct(), Matrix_GetTranslation(), Matrix_GetpTranslation(),
|||	    Matrix_SetTranslation(),
|||	    Matrix_TranslateByVector(), Matrix_Translate(),
|||	    Matrix_MoveByVector(), Matrix_Move()
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_SetTranslation
|||	Set the matrix's translation.
|||
|||	  Synopsis
|||	    void Matrix_SetTranslation(Matrix *m, float x, float y, float z);
|||
|||	  Description
|||	    Set the matrix's translation.
|||
|||	  Arguments
|||	    m*
|||	        Pointer to the destination Matrix structure.
|||	    x
|||	        new X value.
|||	    y
|||	        new Y value.
|||	    z
|||	        new Z value.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_Construct(), Matrix_GetTranslation(), Matrix_GetpTranslation(),
|||	    Matrix_SetTranslationByVector(),
|||	    Matrix_TranslateByVector(), Matrix_Translate(),
|||	    Matrix_MoveByVector(), Matrix_Move()
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_GetpTranslation
|||	Get a pointer to the translation within a matrix.
|||
|||	  Synopsis
|||	    Vector3D* Matrix_GetpTranslation( Matrix *m );
|||
|||	  Description
|||	    Get a pointer to the translation within a matrix.
|||
|||	  Arguments
|||	    m*
|||	        Pointer to the destination Matrix structure.
|||
|||	  Return Value
|||	    v*
|||	        Pointer to the matrix's translation value.
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_Construct(), Matrix_GetTranslation(),
|||	    Matrix_SetTranslationByVector(), Matrix_SetTranslationByVector(),
|||	    Matrix_Translate(),
|||	    Matrix_MoveByVector(), Matrix_Move()
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_TranslateByVector
|||	Translate the matrix by a vector.
|||
|||	  Synopsis
|||	    void Matrix_TranslateByVector( Matrix *m, Vector3D *v );
|||
|||	  Description
|||	    Translate the matrix by a vector.
|||
|||	  Arguments
|||	    m*
|||	        Pointer to the destination Matrix structure.
|||	    v*
|||	        Pointer to the source Vector3D structure.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_Construct(), Matrix_GetTranslation(),
|||	    Matrix_SetTranslationByVector(), Matrix_SetTranslationByVector(),
|||	    Matrix_Translate(),
|||	    Matrix_MoveByVector(), Matrix_Move()
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_Translate
|||	Translate the matrix by three floats.
|||
|||	  Synopsis
|||	    void Matrix_Translate( Matrix *m, float x, float y, float z );
|||
|||	  Description
|||	    Translate the matrix by three floats.
|||
|||	  Arguments
|||	    m*
|||	        Pointer to the destination Matrix structure.
|||	    x
|||	        Translate X value.
|||	    y
|||	        Translate Y value.
|||	    Z
|||	        Translate Z value.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_Construct(), Matrix_GetTranslation(),
|||	    Matrix_SetTranslationByVector(), Matrix_SetTranslationByVector(),
|||	    Matrix_TranslateByVector(),
|||	    Matrix_MoveByVector(), Matrix_Move()
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_MoveByVector
|||	Translate matrix by vector in local orientation.
|||
|||	  Synopsis
|||	    void Matrix_MoveByVector( Matrix *m, Vector3D *v );
|||
|||	  Description
|||	    Translate matrix by vector in local orientation.
|||
|||	  Arguments
|||	    m*
|||	        Pointer to the destination Matrix structure.
|||	    v*
|||	        Pointer to the source Vector3D structure.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_Construct(), Matrix_GetTranslation(),
|||	    Matrix_SetTranslationByVector(), Matrix_SetTranslationByVector(),
|||	    Matrix_TranslateByVector(), Matrix_Translate(),
|||	    Matrix_Move()
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_Move
|||	Translate matrix by three floats in local orientation.
|||
|||	  Synopsis
|||	    void Matrix_Move( Matrix *m, float x, float y, float z );
|||
|||	  Description
|||	    Translate matrix by three floats in local orientation.
|||
|||	  Arguments
|||	    m*
|||	        Pointer to the destination Matrix structure.
|||	    x
|||	        Move X value.
|||	    y
|||	        Move Y value.
|||	    z
|||	        Move Z value.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_Construct(), Matrix_GetTranslation(),
|||	    Matrix_SetTranslationByVector(), Matrix_SetTranslationByVector(),
|||	    Matrix_TranslateByVector(), Matrix_Translate(),
|||	    Matrix_MoveByVector()
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_ScaleByVector
|||	Scales the matrix by a vector.
|||
|||	  Synopsis
|||	    void Matrix_ScaleByVector( Matrix *m, Vector3D *v );
|||
|||	  Description
|||	    Scales the matrix by a vector.
|||
|||	  Arguments
|||	    m*
|||	        Pointer to the destination Matrix structure.
|||	    v*
|||	        Pointer to the source Vector3D structure.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_Construct()
|||	    Matrix_ScaleByVector(),
|||	    Matrix_ScaleLocalByVector(), Matrix_ScaleLocal()
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_Scale
|||	Scale matrix by three floats.
|||
|||	  Synopsis
|||	    void Matrix_Scale( Matrix *m, float x, float y, float z );
|||
|||	  Description
|||	    Scale matrix by three floats.
|||
|||	  Arguments
|||	    m*
|||	        Pointer to the destination Matrix structure.
|||	    x
|||	        Scale X value.
|||	    y
|||	        Scale y value.
|||	    z
|||	        Scale Z value.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_Construct()
|||	    Matrix_ScaleByVector(),
|||	    Matrix_ScaleLocalByVector(), Matrix_ScaleLocal()
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_ScaleLocal
|||	Scale matrix in local coordinates by three floats.
|||
|||	  Synopsis
|||	    void Matrix_ScaleLocal( Matrix *m, float x, float y, float z );
|||
|||	  Description
|||	    Scale matrix in local coordinates by three floats.
|||
|||	  Arguments
|||	    m*
|||	        Pointer to the destination Matrix structure.
|||	    x
|||	        Scale X value.
|||	    y
|||	        Scale y value.
|||	    z
|||	        Scale Z value.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_Construct()
|||	    Matrix_ScaleByVector(), Matrix_Scale(),
|||	    Matrix_ScaleLocalByVector()
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_ScaleLocalByVector
|||	Scale matrix in local coordinates by a vector.
|||
|||	  Synopsis
|||	    void Matrix_ScaleLocalByVector( Matrix *m, Vector3D *v );
|||
|||	  Description
|||	    Scale matrix in local coordinates by three floats.
|||
|||	  Arguments
|||	    m*
|||	        Pointer to the destination Matrix structure.
|||	    v*
|||	        Pointer to the source Vector3D structure.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_Construct()
|||	    Matrix_ScaleByVector(), Matrix_Scale(),
|||	    Matrix_ScaleLocal()
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_Rotate
|||	Rotate the matrix in world space.
|||
|||	  Synopsis
|||	    void Matrix_Rotate( Matrix *m, char axis, float r );
|||
|||	  Description
|||	    Rotate the matrix in world space.
|||
|||	  Arguments
|||	    m*
|||	        Pointer to the destination Matrix structure.
|||	    axis
|||	        World axis to rotate around [ 'X','Y' or 'Z' ]
|||	    r
|||	        Rotation angle in radians
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_Rotate(),
|||	    Matrix_RotateX(), Matrix_RotateY(), Matrix_RotateZ(),
|||	    Matrix_RotateLocal(),
|||	    Matrix_RotateXLocal(), Matrix_RotateYLocal(), Matrix_RotateZLocal(),
|||	    Matrix_Turn(),
|||	    Matrix_TurnX(), Matrix_TurnY(), Matrix_TurnZ(),
|||	    Matrix_TurnLocal(),
|||	    Matrix_TurnXLocal(), Matrix_TurnYLocal(), Matrix_TurnZLocal()
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_RotateX
|||	Rotate the matrix in world space about the X-axis.
|||
|||	  Synopsis
|||	    void Matrix_RotateX( Matrix *m, float r );
|||
|||	  Description
|||	    Rotate the matrix in world space about the X-axis.
|||
|||	  Arguments
|||	    m*
|||	        Pointer to the destination Matrix structure.
|||	    r
|||	        Rotation angle in radians
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_Rotate(),
|||	    Matrix_RotateX(), Matrix_RotateY(), Matrix_RotateZ(),
|||	    Matrix_RotateLocal(),
|||	    Matrix_RotateXLocal(), Matrix_RotateYLocal(), Matrix_RotateZLocal(),
|||	    Matrix_Turn(),
|||	    Matrix_TurnX(), Matrix_TurnY(), Matrix_TurnZ(),
|||	    Matrix_TurnLocal(),
|||	    Matrix_TurnXLocal(), Matrix_TurnYLocal(), Matrix_TurnZLocal()
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_RotateY
|||	Rotate the matrix in world space about the Y-axis.
|||
|||	  Synopsis
|||	    void Matrix_RotateY( Matrix *m, float r );
|||
|||	  Description
|||	    Rotate the matrix in world space about the Y-axis.
|||
|||	  Arguments
|||	    m*
|||	        Pointer to the destination Matrix structure.
|||	    r
|||	        Rotation angle in radians
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_Rotate(),
|||	    Matrix_RotateX(), Matrix_RotateY(), Matrix_RotateZ(),
|||	    Matrix_RotateLocal(),
|||	    Matrix_RotateXLocal(), Matrix_RotateYLocal(), Matrix_RotateZLocal(),
|||	    Matrix_Turn(),
|||	    Matrix_TurnX(), Matrix_TurnY(), Matrix_TurnZ(),
|||	    Matrix_TurnLocal(),
|||	    Matrix_TurnXLocal(), Matrix_TurnYLocal(), Matrix_TurnZLocal()
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_RotateZ
|||	Rotate the matrix in world space about the Z-axis.
|||
|||	  Synopsis
|||	    void Matrix_RotateZ( Matrix *m, float r );
|||
|||	  Description
|||	    Rotate the matrix in world space about the Z-axis.
|||
|||	  Arguments
|||	    m*
|||	        Pointer to the destination Matrix structure.
|||	    r
|||	        Rotation angle in radians
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_Rotate(),
|||	    Matrix_RotateX(), Matrix_RotateY(), Matrix_RotateZ(),
|||	    Matrix_RotateLocal(),
|||	    Matrix_RotateXLocal(), Matrix_RotateYLocal(), Matrix_RotateZLocal(),
|||	    Matrix_Turn(),
|||	    Matrix_TurnX(), Matrix_TurnY(), Matrix_TurnZ(),
|||	    Matrix_TurnLocal(),
|||	    Matrix_TurnXLocal(), Matrix_TurnYLocal(), Matrix_TurnZLocal()
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_RotateLocal
|||	Rotate the matrix in world orientation and in local space.
|||
|||	  Synopsis
|||	    void Matrix_RotateLocal( Matrix *m, char axis, float r );
|||
|||	  Description
|||	    Rotate the matrix in world orientation and in local space.
|||
|||	  Arguments
|||	    m*
|||	        Pointer to the destination Matrix structure.
|||	    axis
|||	        World axis to rotate around [ 'X','Y' or 'Z' ]
|||	    r
|||	        Rotation angle in radians
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_Rotate(),
|||	    Matrix_RotateX(), Matrix_RotateY(), Matrix_RotateZ(),
|||	    Matrix_RotateLocal(),
|||	    Matrix_RotateXLocal(), Matrix_RotateYLocal(), Matrix_RotateZLocal(),
|||	    Matrix_Turn(),
|||	    Matrix_TurnX(), Matrix_TurnY(), Matrix_TurnZ(),
|||	    Matrix_TurnLocal(),
|||	    Matrix_TurnXLocal(), Matrix_TurnYLocal(), Matrix_TurnZLocal()
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_RotateXLocal
|||	Rotate the matrix in world orientation and in local space
|||	about the X-axis.
|||
|||	  Synopsis
|||	    void Matrix_RotateX( Matrix *m, float r );
|||
|||	  Description
|||	    Rotate the matrix in world orientation and in local space
|||	    about the X-axis.
|||
|||	  Arguments
|||	    m*
|||	        Pointer to the destination Matrix structure.
|||	    r
|||	        Rotation angle in radians
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_Rotate(),
|||	    Matrix_RotateX(), Matrix_RotateY(), Matrix_RotateZ(),
|||	    Matrix_RotateLocal(),
|||	    Matrix_RotateXLocal(), Matrix_RotateYLocal(), Matrix_RotateZLocal(),
|||	    Matrix_Turn(),
|||	    Matrix_TurnX(), Matrix_TurnY(), Matrix_TurnZ(),
|||	    Matrix_TurnLocal(),
|||	    Matrix_TurnXLocal(), Matrix_TurnYLocal(), Matrix_TurnZLocal()
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_RotateYLocal
|||	Rotate the matrix in world orientation and in local space
|||	about the Y-axis.
|||
|||	  Synopsis
|||	    void Matrix_RotateYLocal( Matrix *m, float r );
|||
|||	  Description
|||	    Rotate the matrix in world orientation and in local space
|||	    about the Y-axis.
|||
|||	  Arguments
|||	    m*
|||	        Pointer to the destination Matrix structure.
|||	    r
|||	        Rotation angle in radians
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_Rotate(),
|||	    Matrix_RotateX(), Matrix_RotateY(), Matrix_RotateZ(),
|||	    Matrix_RotateLocal(),
|||	    Matrix_RotateXLocal(), Matrix_RotateYLocal(), Matrix_RotateZLocal(),
|||	    Matrix_Turn(),
|||	    Matrix_TurnX(), Matrix_TurnY(), Matrix_TurnZ(),
|||	    Matrix_TurnLocal(),
|||	    Matrix_TurnXLocal(), Matrix_TurnYLocal(), Matrix_TurnZLocal()
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_RotateZLocal
|||	Rotate the matrix in world orientation and in local space
|||	about the Z-axis.
|||
|||	  Synopsis
|||	    void Matrix_RotateZLocal( Matrix *m, float r );
|||
|||	  Description
|||	    Rotate the matrix in world orientation and in local space
|||	    about the Z-axis.
|||
|||	  Arguments
|||	    m*
|||	        Pointer to the destination Matrix structure.
|||	    r
|||	        Rotation angle in radians
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_Rotate(),
|||	    Matrix_RotateX(), Matrix_RotateY(), Matrix_RotateZ(),
|||	    Matrix_RotateLocal(),
|||	    Matrix_RotateXLocal(), Matrix_RotateYLocal(), Matrix_RotateZLocal(),
|||	    Matrix_Turn(),
|||	    Matrix_TurnX(), Matrix_TurnY(), Matrix_TurnZ(),
|||	    Matrix_TurnLocal(),
|||	    Matrix_TurnXLocal(), Matrix_TurnYLocal(), Matrix_TurnZLocal()
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_Turn
|||	Rotate the matrix in local space and local orientation.
|||
|||	  Synopsis
|||	    Matrix_Turn( Matrix *m, char axis, float r );
|||
|||	  Description
|||	    Rotate the matrix in local space and local orientation.
|||
|||	  Arguments
|||	    m*
|||	        Pointer to the destination Matrix structure.
|||	    axis
|||	        World axis to rotate around [ 'X','Y' or 'Z' ]
|||	    r
|||	        Rotation angle in radians
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_Rotate(),
|||	    Matrix_RotateX(), Matrix_RotateY(), Matrix_RotateZ(),
|||	    Matrix_RotateLocal(),
|||	    Matrix_RotateXLocal(), Matrix_RotateYLocal(), Matrix_RotateZLocal(),
|||	    Matrix_Turn(),
|||	    Matrix_TurnX(), Matrix_TurnY(), Matrix_TurnZ(),
|||	    Matrix_TurnLocal(),
|||	    Matrix_TurnXLocal(), Matrix_TurnYLocal(), Matrix_TurnZLocal()
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_TurnX
|||	Rotate the matrix in local space about the X-axis.
|||
|||	  Synopsis
|||	    void Matrix_TurnX( Matrix *m, float r );
|||
|||	  Description
|||	    Rotate the matrix in local space about the X-axis.
|||
|||	  Arguments
|||	    m*
|||	        Pointer to the destination Matrix structure.
|||	    r
|||	        Rotation angle in radians
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_Rotate(),
|||	    Matrix_RotateX(), Matrix_RotateY(), Matrix_RotateZ(),
|||	    Matrix_RotateLocal(),
|||	    Matrix_RotateXLocal(), Matrix_RotateYLocal(), Matrix_RotateZLocal(),
|||	    Matrix_Turn(),
|||	    Matrix_TurnX(), Matrix_TurnY(), Matrix_TurnZ(),
|||	    Matrix_TurnLocal(),
|||	    Matrix_TurnXLocal(), Matrix_TurnYLocal(), Matrix_TurnZLocal()
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_TurnY
|||	Rotate the matrix in local space about the Y-axis.
|||
|||	  Synopsis
|||	    void Matrix_TurnY( Matrix *m, float r );
|||
|||	  Description
|||	    Rotate the matrix in local space about the Y-axis.
|||
|||	  Arguments
|||	    m*
|||	        Pointer to the destination Matrix structure.
|||	    r
|||	        Rotation angle in radians
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_Rotate(),
|||	    Matrix_RotateX(), Matrix_RotateY(), Matrix_RotateZ(),
|||	    Matrix_RotateLocal(),
|||	    Matrix_RotateXLocal(), Matrix_RotateYLocal(), Matrix_RotateZLocal(),
|||	    Matrix_Turn(),
|||	    Matrix_TurnX(), Matrix_TurnY(), Matrix_TurnZ(),
|||	    Matrix_TurnLocal(),
|||	    Matrix_TurnXLocal(), Matrix_TurnYLocal(), Matrix_TurnZLocal()
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_TurnZ
|||	Rotate the matrix in local space about the Z-axis.
|||
|||	  Synopsis
|||	    void Matrix_TurnZ( Matrix *m, float r );
|||
|||	  Description
|||	    Rotate the matrix in local space about the Z-axis.
|||
|||	  Arguments
|||	    m*
|||	        Pointer to the destination Matrix structure.
|||	    r
|||	        Rotation angle in radians
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_Rotate(),
|||	    Matrix_RotateX(), Matrix_RotateY(), Matrix_RotateZ(),
|||	    Matrix_RotateLocal(),
|||	    Matrix_RotateXLocal(), Matrix_RotateYLocal(), Matrix_RotateZLocal(),
|||	    Matrix_Turn(),
|||	    Matrix_TurnX(), Matrix_TurnY(), Matrix_TurnZ(),
|||	    Matrix_TurnLocal(),
|||	    Matrix_TurnXLocal(), Matrix_TurnYLocal(), Matrix_TurnZLocal()
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_TurnLocal
|||	Rotate the matrix in local space updating the orientation only.
|||
|||	  Synopsis
|||	    void Matrix_TurnLocal( Matrix *m, char axis, float r );
|||
|||	  Description
|||	    Rotate the matrix in local space updating the orientation only.
|||
|||	  Arguments
|||	    m*
|||	        Pointer to the destination Matrix structure.
|||	    axis
|||	        World axis to rotate around [ 'X','Y' or 'Z' ]
|||	    r
|||	        Rotation angle in radians
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_Rotate(),
|||	    Matrix_RotateX(), Matrix_RotateY(), Matrix_RotateZ(),
|||	    Matrix_RotateLocal(),
|||	    Matrix_RotateXLocal(), Matrix_RotateYLocal(), Matrix_RotateZLocal(),
|||	    Matrix_Turn(),
|||	    Matrix_TurnX(), Matrix_TurnY(), Matrix_TurnZ(),
|||	    Matrix_TurnLocal(),
|||	    Matrix_TurnXLocal(), Matrix_TurnYLocal(), Matrix_TurnZLocal()
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_TurnXLocal
|||	Rotate the matrix in local space about the X-axis updating the orientation only.
|||
|||	  Synopsis
|||	    void Matrix_TurnXLocal( Matrix *m, float r );
|||
|||	  Description
|||	    Rotate the matrix in local space about the X-axis updating the
|||	    orientation only.
|||
|||	  Arguments
|||	    m*
|||	        Pointer to the destination Matrix structure.
|||	    r
|||	        Rotation angle in radians
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_Rotate(),
|||	    Matrix_RotateX(), Matrix_RotateY(), Matrix_RotateZ(),
|||	    Matrix_RotateLocal(),
|||	    Matrix_RotateXLocal(), Matrix_RotateYLocal(), Matrix_RotateZLocal(),
|||	    Matrix_Turn(),
|||	    Matrix_TurnX(), Matrix_TurnY(), Matrix_TurnZ(),
|||	    Matrix_TurnLocal(),
|||	    Matrix_TurnXLocal(), Matrix_TurnYLocal(), Matrix_TurnZLocal()
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_TurnYLocal
|||	Rotate the matrix in local space about the Y-axis updating the orientation only.
|||
|||	  Synopsis
|||	    void Matrix_TurnYLocal( Matrix *m, float r );
|||
|||	  Description
|||	    Rotate the matrix in local space about the X-axis updating the
|||	    orientation only.
|||
|||	  Arguments
|||	    m*
|||	        Pointer to the destination Matrix structure.
|||	    r
|||	        Rotation angle in radians
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_Rotate(),
|||	    Matrix_RotateX(), Matrix_RotateY(), Matrix_RotateZ(),
|||	    Matrix_RotateLocal(),
|||	    Matrix_RotateXLocal(), Matrix_RotateYLocal(), Matrix_RotateZLocal(),
|||	    Matrix_Turn(),
|||	    Matrix_TurnX(), Matrix_TurnY(), Matrix_TurnZ(),
|||	    Matrix_TurnLocal(),
|||	    Matrix_TurnXLocal(), Matrix_TurnYLocal(), Matrix_TurnZLocal()
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_TurnZLocal
|||	Rotate the matrix in local space about the Z-axis updating the orientation only.
|||
|||	  Synopsis
|||	    void Matrix_TurnZLocal( Matrix *m, float r );
|||
|||	  Description
|||	    Rotate the matrix in local space about the Z-axis updating the
|||	    orientation only.
|||
|||	  Arguments
|||	    m*
|||	        Pointer to the destination Matrix structure.
|||	    r
|||	        Rotation angle in radians
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_Rotate(),
|||	    Matrix_RotateX(), Matrix_RotateY(), Matrix_RotateZ(),
|||	    Matrix_RotateLocal(),
|||	    Matrix_RotateXLocal(), Matrix_RotateYLocal(), Matrix_RotateZLocal(),
|||	    Matrix_Turn(),
|||	    Matrix_TurnX(), Matrix_TurnY(), Matrix_TurnZ(),
|||	    Matrix_TurnLocal(),
|||	    Matrix_TurnXLocal(), Matrix_TurnYLocal(), Matrix_TurnZLocal()
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_Print
|||	Print the matrix.
|||
|||	  Synopsis
|||	    void Matrix_Print( Matrix *m );
|||
|||	  Description
|||	    Print the matrix.
|||
|||	  Arguments
|||	    m*
|||	        Pointer to the destination Matrix structure.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_FullInvert
|||	Full matrix invert.
|||
|||	  Synopsis
|||	    Err Matrix_FullInvert(pMatrix dst, pMatrix src);
|||
|||	  Description
|||	    The code is based on the code presented in "Graphics Gems"
|||	    (ed. Andrew Glassner), in "Matrix Inversion" (pg. 766).  The code
|||	    has been modified to work with the 3x4 matrices we use.  We don't
|||	    have a fourth column, so it is hardwired as (0, 0, 0, 1).
|||	    Print the matrix.
|||
|||	  Arguments
|||	    dst
|||	        Pointer to the destination Matrix structure.
|||	    src
|||	        Pointer to the source Matrix structure.
|||
|||	  Return Value
|||	    Err
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_FullInvert(), Matrix_Invert()
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_Invert
|||	Matrix invert.
|||
|||	  Synopsis
|||	    void Matrix_Invert( Matrix *dst, Matrix *src );
|||
|||	  Description
|||	    Matrix invert.
|||
|||	  Arguments
|||	    dst
|||	        Pointer to the destination Matrix structure.
|||	    src
|||	        Pointer to the source Matrix structure.
|||
|||	  Return Value
|||	    Err
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_FullInvert(), Matrix_Invert()
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_Perspective
|||	Computes a perspective matrix that is concatenated on to the camera matrix.
|||
|||	  Synopsis
|||	    void Matrix_Perspective(pMatrix matrix, pViewPyramid p,
|||	        float screen_xmin, float screen_xmax,
|||	        float screen_ymin, float screen_ymax, float wscale)
|||
|||	  Description
|||	    Computes a perspective matrix that is concatenated on to the camera matrix.
|||
|||	  Arguments
|||	    matrix
|||	        Pointer to the destination Matrix structure.
|||	    p
|||	        Pointer to the .
|||	    screen_xmin
|||	        value.
|||	    screen_xmax
|||	        value.
|||	    screen_ymin
|||	        value.
|||	    screen_ymax
|||	        value.
|||	    wscale
|||	        value.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
**/

/**
|||	AUTODOC -class Matrix -name Matrix_LookAt
|||	Calculates a matrix that looks at a specificed point.
|||
|||	  Synopsis
|||	    void Matrix_LookAt(Matrix *m, Vector3D *pWhere, Vector3D *pMe, float twist);
|||
|||	  Description
|||	    Calculates a matrix that looks from pMe to pWhere. The up vector is assumed
|||	    to be the world y-axis. A twist (bank) can be applied to the resultant 
|||	    matrix (a rotation around the z axis).  Note - there is no unique
|||	    resolution when looking at a point that is directly above or below the
|||	    point that you are looking from.
|||
|||	  Arguments
|||	    m
|||	        Pointer to the destination Matrix structure.
|||	    pWhere
|||	        Pointer to a vector to 'lookat'.
|||	    pMe
|||	        Pointer to a vector to 'lookfrom'.
|||	    twist
|||	        A rotation in radians away from the up vector.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
**/

/**
|||	AUTODOC -class Matrix -name Matrix_BillboardX
|||	Calculates a matrix that lays perpendicular to a vector about it's X-axis
|||
|||	  Synopsis
|||	    void Matrix_BillboardX( Matrix *m, Vector3D *v );
|||
|||	  Description
|||	    This function is used to rotate a billboard object around its X-axis so that it
|||	    always faces the camera. Care must be taken when constructing the billboard
|||	    object. If the billboard had sides of length 1 with the X-axis dividing one
|||	    of the sides the 4 corners should have the following coordinates:
|||	        1.0, 0.0, -0.5
|||	        1.0, 0.0,  0.5
|||	        0.0, 0.0, -0.5
|||	        0.0, 0.0,  0.5
|||
|||	  Arguments
|||	    m
|||	        Pointer to the destination Matrix structure.
|||	    v
|||	        Pointer to a vector to 'lookat'.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_BillboardY(), Matrix_BillboardZ()
|||
**/

/**
|||	AUTODOC -class Matrix -name Matrix_BillboardY
|||	Calculates a matrix that lays perpendicular to a vector about it's Y-axis
|||
|||	  Synopsis
|||	    void Matrix_BillboardY( Matrix *m, Vector3D *v );
|||
|||	  Description
|||	    This function is used to rotate a billboard object around its Y-axis so that it
|||	    always faces the camera. Care must be taken when constructing the billboard
|||	    object. If the billboard had sides of length 1 with the Y-axis dividing one
|||	    of the sides the 4 corners should have the following coordinates:
|||	        -0.5, 0.0, 1.0
|||	         0.5, 0.0, 1.0
|||	        -0.5, 0.0, 0.0
|||	         0.5, 0.0, 0.0
|||
|||	  Arguments
|||	    m
|||	        Pointer to the destination Matrix structure.
|||	    v
|||	        Pointer to a vector to 'lookat'.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_BillboardX(), Matrix_BillboardZ()
|||
**/

/**
|||	AUTODOC -class Matrix -name Matrix_BillboardZ
|||	Calculates a matrix that lays perpendicular to a vector about it's Z-axis
|||
|||	  Synopsis
|||	    void Matrix_BillboardZ( Matrix *m, Vector3D *v );
|||
|||	  Description
|||	    This function is used to rotate a billboard object around its Z-axis so that it
|||	    always faces the camera. Care must be taken when constructing the billboard
|||	    object. If the billboard had sides of length 1 with the Z-axis dividing one
|||	    of the sides the 4 corners should have the following coordinates:
|||	        0.0, -0.5, 1.0
|||	        0.0,  0.5, 1.0
|||	        0.0, -0.5, 0.0
|||	        0.0,  0.5, 0.0
|||
|||	  Arguments
|||	    m
|||	        Pointer to the destination Matrix structure.
|||	    v
|||	        Pointer to a vector to 'lookat'.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_BillboardX(), Matrix_BillboardY()
|||
**/

/**
|||	AUTODOC -class Matrix -name Matrix_CopyOrientation
|||	Copy the orientation of a source matrix to a destination matrix.
|||
|||	  Synopsis
|||	    void Matrix_CopyOrientation( Matrix *m, Matrix *b );
|||
|||	  Description
|||	    Copy the orientation of a source matrix to a destination matrix.
|||
|||	  Arguments
|||	    m
|||	        Pointer to the destination Matrix structure.
|||	    b
|||	        Pointer to a source matrix.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_Copy(), Matrix_CopyOrientation(), Matrix_CopyTranslation()
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_CopyTranslation
|||	Copy the translation of a source matrix to a destination matrix.
|||
|||	  Synopsis
|||	    void Matrix_CopyTranslation( Matrix *m, Matrix *b );
|||
|||	  Description
|||	    Copy the translation of a source matrix to a destination matrix.
|||
|||	  Arguments
|||	    m
|||	        Pointer to the destination Matrix structure.
|||	    b
|||	        Pointer to a source matrix.
|||
|||	  Return Value
|||	    void
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_Copy(), Matrix_CopyOrientation(), Matrix_CopyTranslation()
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_GetHeading
|||	Calculates the heading component of a matrix.
|||
|||	  Synopsis
|||	    float Matrix_GetHeading( Matrix *m );
|||
|||	  Description
|||	    Calculates the heading component of a matrix.
|||
|||	  Arguments
|||	    m
|||	        Pointer to the destination Matrix structure.
|||
|||	  Return Value
|||	    float
|||	        angle in radians. 0 is aligned along the -z axis.
|||	            Positive angles are to the righthand side.
|||	            Negative angles are to the lefthand side.
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_GetHeading(), Matrix_GetBank(), Matrix_GetElevation(),
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_GetBank
|||	Calculates the heading component of a matrix.
|||
|||	  Synopsis
|||	    float Matrix_GetBank( Matrix *m );
|||
|||	  Description
|||	    Calculates the banking component of a matrix.
|||
|||	  Arguments
|||	    m
|||	        Pointer to the destination Matrix structure.
|||
|||	  Return Value
|||	    float
|||	        angle in radians. 0 is aligned along the +x axis.
|||	            Positive angles are to the righthand side up.
|||	            Negative angles are to the righthand side down.
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_GetHeading(), Matrix_GetBank(), Matrix_GetElevation(),
|||
**/
/**
|||	AUTODOC -class Matrix -name Matrix_GetElevation
|||	Calculates the elevation component of a matrix.
|||
|||	  Synopsis
|||	    float Matrix_GetElevation( Matrix *m );
|||
|||	  Description
|||	    Calculates the elevation component of a matrix.
|||
|||	  Arguments
|||	    m
|||	        Pointer to the destination Matrix structure.
|||
|||	  Return Value
|||	    float
|||	        angle in radians. 0 is aligned along the -z axis.
|||	            Positive angles are to the up from the z axis.
|||	            Negative angles are to the down from the z axis.
|||
|||	  Implementation
|||	    Version 3.0
|||
|||	  Associated Files
|||	    <matrix.h>, mercury/lib
|||
|||	  See Also
|||	    Matrix_GetHeading(), Matrix_GetBank(), Matrix_GetElevation(),
|||
**/

/* keep the compiler happy... */
extern int foo;

