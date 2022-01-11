#ifndef __GRAPHICS_PIPE_OBJ_INL
#define __GRAPHICS_PIPE_OBJ_INL


/******************************************************************************
**
**  @(#) obj.inl 96/02/20 1.23
**
**  Inlines for Base Framework object class
**
******************************************************************************/


#if defined(__cplusplus) && !defined(GFX_C_Bind)

inline int32 GfxObj::GetType() const
	{ return m_Type; }

inline void GfxObj::IncUse()
	{ ++m_Use; }

inline GfxObj& GfxObj::operator=(GfxObj& src)
	{ Copy(&src); return *this; }

inline GfxRef::GfxRef(GFX_DO_NOTHING_TYPE)
	{ }

inline GfxRef::GfxRef()
	{ m_ObjPtr = NULL; }

inline GfxRef::GfxRef(GfxObj* ptr)
	{ if (m_ObjPtr = ptr) m_ObjPtr->IncUse(); }

inline GfxRef::~GfxRef()
	{ Delete(m_ObjPtr); }

inline GfxRef& GfxRef::operator=(const GfxRef& src)
	{ *this = src.m_ObjPtr; return *this; }

inline bool GfxRef::operator==(const GfxObj* p)
	{ return m_ObjPtr == p; }

inline bool GfxRef::operator!=(const GfxObj* p)
	{ return m_ObjPtr != p; }

inline bool GfxRef::IsNull() const
	{ return m_ObjPtr == NULL; }

inline ObjRef::ObjRef(GFX_DO_NOTHING_TYPE) : GfxRef(GFX_DO_NOTHING_VALUE)
	{ }

inline ObjRef::ObjRef() : GfxRef() { }

inline ObjRef::ObjRef(GfxObj* ptr) : GfxRef(ptr) { }

inline ObjRef::operator GfxObj*()
	{ return (GfxObj*) m_ObjPtr; }

inline ObjRef::operator const GfxObj*() const
	{ return (const GfxObj*) m_ObjPtr; }

#define	Obj_GetFlags(c)			((c)->m_Flags)

#else

#define Obj_GetClassName(c)		Classes[(c)->m_Type].m_Name
#define Obj_GetParentClass(c)	Classes[(c)->m_Type].m_Base
#define Obj_PrintInfo(o)		(*((o)->m_Funcs)->PrintInfo)(o)
#define Obj_Copy(d, s)			(*((d)->m_Funcs)->Copy)(d, s)


#define Obj_GetClass(c)			((c)->m_Type)
#define	Obj_GetType(c)			((c)->m_Type)
#define	Obj_GetUse(c)			((c)->m_Use)
#define	Obj_GetFlags(c)			((c)->m_Flags)
#define	Obj_IncUse(c)			(++((c)->m_Use))
#define	Obj_IsNull(c)			((c) == NULL)

/*
 * Internal access to GfxObj flags
 */
#define Obj_OrFlags(o, f)       (((GfxObj*) (o))->m_Flags |= (f))
#define Obj_ClearFlags(o, f)    (((GfxObj*) (o))->m_Flags &= ~(f))
#define Obj_IsSet(o, f)         (((GfxObj*) (o))->m_Flags & (f))

#define Obj_CopyFlags(d, s) \
    (((GfxObj*) (d))->m_Flags = (((GfxObj*) (d))->m_Flags & OBJ_NoFree) | \
                (((GfxObj*) (s))->m_Flags & ~OBJ_NoFree))



#endif


#endif /* __GRAPHICS_PIPE_OBJ_INL */
