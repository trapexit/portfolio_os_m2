#ifndef __GRAPHICS_PIPE_GPDATA_H
#define __GRAPHICS_PIPE_GPDATA_H


/******************************************************************************
**
**  @(#) gpdata.h 96/02/20 1.54
**
**  Graphics Pipeline data structures
**
**  Because the graphics pipeline has a considerable amount of data
**  which must match between the C and C++ bindings, we define the
**  structure format in a separate header file.
**
**  The C implementation uses a dispatch vector to allow rendering
**  functions to be implemented in two ways (using SGI GL or in software).
**  The C++ binding uses virtual functions to accomplish this.
**
******************************************************************************/


    int32               m_RenderMode;
    ProjTrans			m_Projection;
    struct ViewTrans	m_ModelView;
	int32				m_CullFaces;
	int32				m_VertexOrder;
	int32				m_HiddenSurf;
	Color4				m_BackColor;
	gfloat      		m_TexCoord[4];
	Color4				m_Ambient;
	Texture*			m_Texture;
	TexBlend*			m_TexBlend;
	const MatProp*		m_Material;
	uint32				m_Capabilities;
/*
 * Private stuff common to GL and software rendering
 */
	Box2				m_Viewport;
	struct LightInfo*	m_Lights;
/*
 * Private stuff only used by GL rendering
 */
	uint32				m_BgPixel;
/*
 * Private stuff only used by software rendering
 */
	bool		m_TransValid;		/* TRUE if model/view valid */
	Transform	m_TotalMatrix;		/* total transformation matrix */
	bool        m_HasWindow;
	Box3		m_ClipVol;
/*
 * Private stuff used by optimized rendering
 */
    Vector3     m_LocalInfViewer;   /* Eye point transformed into local device coords*/
    gfloat      m_WScale;           /* Scale factor applied to 1/w */
    void        *m_TempBuffer;      /* Temporary storage */
    uint32      m_TempBufferSize;   /* Number of bytes in Temp buffer */
    void        (*m_TempReallocateFunction)(void*, uint32, void**, uint32 *);
                        /* Reallocate function used when more temporary
                           storage is needed */
    uint32      m_NumLightsOn;      /* Number of compiled lights */
	struct LightInfo*	m_CompiledLights; /* List of compiled lights */

/*
 * Command list management is done with the gstate
 */
	GState*		m_GState;

/*
 * Lazy evaluation flags
 *
 */
    uint8		m_TexChanged;	/* = TRUE when texture has changed */
    uint8		m_DblChanged;	/* = TRUE when any blend attribute has changed */

/*
 * Command lists that are used to change modes and to clear the screen
 *
 */
	TexBlend*   m_txbDefault;
	CltSnippet	m_clearSnip;


#endif /* __GRAPHICS_PIPE_GPDATA_H */
