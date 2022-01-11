#ifndef __GRAPHICS_PIPE_VMTX_H
#define __GRAPHICS_PIPE_VMTX_H


/******************************************************************************
**
**  @(#) vmtx.h 96/02/20 1.33
**
**  ViewTrans: Geometry pipeline Model/View transform
**  The ViewTrans object always references the model/view
**  matrix on the top of the stack.
**
******************************************************************************/


#if defined(__cplusplus) && !defined(GFX_C_Bind)

class ViewTrans : public Transform
   {
	friend class GP;
	friend class GPGL;

public:
	Err	Update();
	const Transform* GetInverse();
    MatrixData* GetData() const;
    Err SetData(const MatrixData&);
    Err SetData(const MatrixData*);
    Err Identity();
    Err Transpose();
    Err Transpose(const Transform& t);
    Err Transpose(const Transform* t);
    Err Inverse();
    Err Inverse(const Transform& t);
    Err Inverse(const Transform* t);
    Err Add(const Transform* t);
    Err Subtract(const Transform* t);
    Err Mul(const Transform& a, const Transform& b);
    Err Mul(const Transform* a, const Transform* b);
    Err PreMul(const Transform& a);
    Err PreMul(const Transform* a);
    Err PostMul(const Transform& a);
    Err PostMul(const Transform* a);
    Err Translate(const Point3&);
    Err Translate(const Point3*);
    Err Scale(const Point3&);
    Err Scale(const Point3*);
    Err Rotate(int32 axis, gfloat angle);
    Err AxisRotate(const Vector3* axis, gfloat angle);
    Err AxisRotate(const Vector3& axis, gfloat angle);
    Err LookAt(const Point3& v, gfloat twist);
    Err LookAt(const Point3* v, gfloat twist);
    Err Polarview(gfloat dist, gfloat azim, gfloat inc, gfloat twist);
    Err Perspective(gfloat fovy, gfloat aspect, gfloat n, gfloat f);
    Err Frustum(const Box3&);
    Err Frustum(const Box3*);
    Err Ortho(const Box3&);
    Err Ortho(const Box3*);
    Err Ortho2(gfloat left, gfloat right, gfloat bottom, gfloat top);
    Err Ortho(gfloat left, gfloat right, gfloat bottom, gfloat top);
    Err Factor(Vector3*, Vector3*, Vector3*);
    Err Copy(GfxObj*);

    virtual Err Push();
    virtual Err Pop();
    virtual Err Update();
    void PrintInfo() const;

Protected:
	ViewTrans(GP*);
	ViewTrans(GFX_DO_NOTHING_TYPE);
    Transform&  Top() const;

    GP*		    m_GP;
    bool        m_Changed;          /* TRUE if M/V changed */
    Stack       m_Stack;
	Transform	m_Inverse;			/* inverse of current M/V matrix */
  };

#else

/*
 * ViewTrans is a structure that represents the INTERNAL FORMAT of
 * class ViewTrans in the C binding. DO NOT RELY ON THE FIELDS IN THIS
 * STRUCTURE - THEY ARE SUBJECT TO CHANGE WITHOUT NOTICE! ViewTrans
 * attributes should only be accessed by the Trans_XXX functions.
 */
typedef struct ViewTrans
   {
	GfxObj		m_Obj;				/* standard object header */
	GP*			m_GP;				/* from SurfTrans */
	bool		m_Changed;			/* TRUE if M/V changed */
	Stack		m_Stack;
	Transform	m_Inverse;			/* inverse of current M/V matrix */
  } ViewTrans;

#endif


#endif /* __GRAPHICS_PIPE_VMTX_H */
