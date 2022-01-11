#ifndef __GRAPHICS_PIPE_GP_INL
#define __GRAPHICS_PIPE_GP_INL


/******************************************************************************
**
**  @(#) gp.inl 96/02/20 1.41
**
**  Inlines for Geometry Pipeline class
**
******************************************************************************/


#if defined(__cplusplus) && !defined(GFX_C_Bind)

inline Transform* GP::GetProjection()
	{ return (Transform*) &m_Projection; }

inline uint32 GP::GetCapabilities()
	{ return m_Capabilities; }

inline int32 GP::GetCullFaces()
	{ return m_CullFaces; }

inline int32 GP::GetHiddenSurf()
	{ return m_HiddenSurf; }

inline const Color4& GP::GetBackColor()
	{ return m_BackColor; }

inline const Color4& GP::GetAmbient()
	{ return m_Ambient; }

inline MatProp*	GP::GetMaterial()
	{ return (MatProp*) m_Material; }

inline TexBlend*	GP::GetTexBlend()
	{ return m_TexBlend; }

inline void	GP::GetViewport(Box2* b)
	{ *b = m_Viewport; }

inline TexBlend* GP::GetTxbDefault()
	{ return m_txbDefault; }

inline GState* GP::GetGState()
	{ return m_gstate; }

inline void	GP::GetBackColor(Color4* c)
	{ *c = m_BackColor; }

inline Err GP::DrawLines(int32 n, Point3* l)
{
	Lines lines(n, l);
	return Draw(lines);
}

inline Err GP::DrawPoints(int32 n, Point3* l)
{
	Points points(n, l);
	return Draw(points);
}

inline Err GP::DrawTriList(int16 style, int32 nv,
                        Point3* l, Vector3* n, Color4* c, TexCoord* t)
{
	TriList tl(style, nv, l, n, c, t);
	return Draw(tl);
}

inline Err GP::DrawTriFan(int16 style, int32 nv,
                        Point3* l, Vector3* n, Color4* c, TexCoord* t)
{
	TriFan tl(style, nv, l, n, c, t);
	return Draw(tl);
}

inline Err GP::DrawTriStrip(int16 style, int32 nv,
                        Point3* l, Vector3* n, Color4* c, TexCoord* t)
{
	TriStrip tl(style, nv, l, n, c, t);
	return Draw(tl);
}

inline Err GP::DrawQuadMesh(int16 style, int32 x, int32 y,
                        Point3* l, Vector3* n, Color4* c, TexCoord* t)
{
	QuadMesh m(style, x, y, l, n, c, t);
	return Draw(m);
}

#else

#define	GP_Print	Obj_Print
#define	GP_Delete	Obj_Delete

/*
 * Attribute Update Functions
 */
#define	GP_CALL(gp, f) (*((GPFuncs*) ((GfxObj*) (gp))->m_Funcs)->f)

#define GP_SetCullFaces(gp, v)	GP_CALL(gp, SetCullFaces)(gp, v)
#define GP_SetHiddenSurf(gp, v)	GP_CALL(gp, SetHiddenSurf)(gp, v)
#define GP_SetAmbient(gp, c)	GP_CALL(gp, SetAmbient)(gp, c)
#define GP_SetBackColor(gp, c)	GP_CALL(gp, SetBackColor)(gp, c)
#define GP_SetViewport(gp, b)	GP_CALL(gp, SetViewport)(gp, b)
#define GP_SetMaterial(gp, p)	GP_CALL(gp, SetMaterial)(gp, p)
#define GP_SetTexBlend(gp, p)	GP_CALL(gp, SetTexBlend)(gp, p)
#define GP_SetLight(gp, i, l)	GP_CALL(gp, SetLight)(gp, i, l)
#define GP_ClearLights(gp)		GP_CALL(gp, ClearLights)(gp)
#define GP_Enable(gp, c)		GP_CALL(gp, Enable)(gp, c)
#define GP_Disable(gp, c)		GP_CALL(gp, Disable)(gp, c)
#define GP_Generate(gp, c)		GP_CALL(gp, Generate)(gp, c)
#define GP_Push(gp)				GP_CALL(gp, Push)((GP*) (gp))
#define GP_Pop(gp)				GP_CALL(gp, Pop)((GP*) (gp))
#define GP_Clear(gp,o)			GP_CALL(gp, Clear)(gp, o)
#define GP_Flush(gp)			GP_CALL(gp, Flush)(gp)
#define GP_GetLight(gp, i, l)	GP_CALL(gp, GetLight)(gp, i, l)
#define GP_IsInView(gp, b)		GP_CALL(gp, IsInView)(gp, b)
#define GP_SetDestBuffer(gp, b)	GP_CALL(gp, SetDestBuffer)(gp, b)
#define GP_SetZBuffer(gp, b)	GP_CALL(gp, SetZBuffer)(gp, b)
#define GP_GetDestBuffer(gp)	GP_CALL(gp, GetDestBuffer)(gp)
#define GP_GetZBuffer(gp)		GP_CALL(gp, GetZBuffer)(gp)
#define	GP_Draw(gp, geo)		GP_CALL(gp, Draw)(gp, geo)

#endif


#endif /* __GRAPHICS_PIPE_GP_INL */
