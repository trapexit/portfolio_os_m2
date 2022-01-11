#ifndef __GRAPHICS_PIPE_MTX_INL
#define __GRAPHICS_PIPE_MTX_INL


/******************************************************************************
**
**  @(#) mtx.inl 96/02/20 1.26
**
**  Inlines for Transform class
**
******************************************************************************/


/*
 * Internal Transform flags
 */
#define	TRANS_IsIdentity	1

#if !defined(__cplusplus) || defined(GFX_C_Bind)

#define MTX_CALL(t, f)		(*((TransFuncs*) ((GfxObj*) (t))->m_Funcs)->f)
#define	Trans_Delete(t)		Obj_Delete((GfxObj*) (t))
#define	Trans_IsIdentity(t)	(((GfxObj*) (t))->m_Flags & TRANS_Identity != 0)

#define Trans_Copy(d, s)		MTX_CALL(d, Copy)(d, s)
#define	Trans_Print(t) 			MTX_CALL(t, PrintInfo)(t)
#define	Trans_SetData(t, data)	MTX_CALL(t, SetData)(t, data)
#define	Trans_Identity(t)		MTX_CALL(t, Identity)(t)
#define	Trans_Transpose(d, s)	MTX_CALL(d, Transpose)(d, s)
#define	Trans_Inverse(d, s)		MTX_CALL(d, Inverse)(d, s)
#define	Trans_GetInverse(t)		MTX_CALL(t, GetInverse)(t)
#define	Trans_Add(d, s)			MTX_CALL(d, Add)(d, s)
#define	Trans_Subtract(d, s)	MTX_CALL(d, Subtract)(d, s)
#define	Trans_Mul(d, a, b)		MTX_CALL(d, Mul)(d, a, b)
#define	Trans_PreMul(d, s)		MTX_CALL(d, PreMul)(d, s)
#define	Trans_PostMul(d, s)		MTX_CALL(d, PostMul)(d, s)
#define	Trans_Scale(t, p)		MTX_CALL(t, Scale)(t, p)
#define	Trans_Frustum(t, b)		MTX_CALL(t, Frustum)(t, b)
#define	Trans_Ortho(t, b)		MTX_CALL(t, Ortho)(t, b)
#define	Trans_Push(t)			MTX_CALL(t, Push)((Transform*) t)
#define	Trans_Pop(t)			MTX_CALL(t, Pop)((Transform*) t)

#define	Trans_Translate(t, p)	MTX_CALL(t, Translate)(t, p)
#define	Trans_LookAt(t, v, tw)	MTX_CALL(t, LookAt)(t, v, tw)
#define	Trans_Rotate(t, ax, ang)	MTX_CALL(t, Rotate)(t, ax, (gfloat) ang)

#define Trans_Touch(t) (Obj_IsSet(t, TRANS_DoTouch) ? \
		MTX_CALL(t, Touch)(t) : Obj_OrFlags(t, TRANS_Identity))

#define	Trans_GetData(t) \
	(Obj_IsSet(t, TRANS_ExternalData) ? MTX_CALL(t, GetData)(t) : \
		((MatrixData*) &(t->data)))

#define	Trans_Perspective(t, fovy, aspect, hither, yon) \
		MTX_CALL(t, Perspective)(t, fovy, aspect, hither, yon)

#define	Trans_Polarview(t, dist, azim, inc, twist) \
		MTX_CALL(t, Polarview)(t, dist, azim, inc, twist)

#define	Trans_Ortho2(t, left, right, bottom, top) \
		MTX_CALL(t, Ortho2)(t, left, right, bottom, top)

#define Trans_AxisRotate(t, axis, angle) \
        MTX_CALL(t, AxisRotate)(t, axis, angle)

#define Trans_Factor(t, trans, scale, rot) \
        MTX_CALL(t, Factor)(t, trans, scale, rot)

#endif


#endif /* __GRAPHICS_PIPE_MTX_INL */
