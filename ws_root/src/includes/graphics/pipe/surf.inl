#ifndef __GRAPHICS_PIPE_SURF_INL
#define __GRAPHICS_PIPE_SURF_INL


/******************************************************************************
**
**  @(#) surf.inl 96/02/20 1.12
**
**  Inlines for Surface class
**
******************************************************************************/


#if !defined(__cplusplus) || defined(GFX_C_Bind)

#define	Surf_Print	Obj_Print
#define	Surf_Delete	Obj_Delete
#define	Surf_Copy	Obj_Copy

#define	SURF_CALL(s, f) (*((SurfFuncs*) ((GfxObj*) (s))->m_Funcs)->f)

#define Surf_Display(s,gp,t,m)		SURF_CALL(s, Display)(s, gp, t, m)
#define Surf_CalcBound(s,b)			SURF_CALL(s, CalcBound)(s, b)
#define Surf_FindGeometry(s,n,g)	SURF_CALL(s, FindGeometry)(s,n,g)
#define Surf_AddGeometry(s,g,t,m)	SURF_CALL(s, AddGeometry)(s,g,t,m)
#define Surf_Extend(s,n)			SURF_CALL(s, Extend)(s,n)

#endif


#endif /* __GRAPHICS_PIPE_SURF_INL */
