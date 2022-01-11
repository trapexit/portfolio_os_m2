#ifndef __GRAPHICS_PIPE_MTX_H
#define __GRAPHICS_PIPE_MTX_H


/******************************************************************************
**
**  @(#) mtx.h 96/02/20 1.37
**
**  Transform: 4x4 Matrix transformation class
**
******************************************************************************/


/****
 *
 * Create			Initialize matrix
 * Copy				Copy one matrix into another
 * SetData			Initialize matrix from data array
 * GetData			Load data array from matrix
 * Transpose		Compute the transpose
 * Inverse			Compute the inverse
 * Add, +=			Add matrix to this matrix
 * Subtract, -= 	Subtract matrix from this matrix
 * Mul, *= 			Post concatenate to this matrix
 * PreMul			Pre concatenate to this matrix
 * Translate		Concatenate translation matrix
 * Scale			Concatenate scaling matrix
 * Rotate			Concatenation rotate about coordinate axis
 * LookAt			Concatenate matrix to rotate to a point
 * Polarview		Concatenate matrix to define viewpoint in polar
 * Perspective		Concatenate perspective projection matrix
 * Frustum			Concatenate matrix to define view volume
 * Ortho			Concatenate orthographic projection matrix
 * Ortho2			Concatenate orthographic projection matrix (2D)
 * Print			Print contents of matrix on standard output
 * Factor			Factor scaling, translation and rotation components
 *
 ****/

typedef gfloat MatrixData[4][4];

/*
 * Transform Axis types (Trans_Rotate)
 */
#define	TRANS_XAxis	'x'
#define	TRANS_YAxis	'y'
#define	TRANS_ZAxis	'z'

/*
 * Indicate for Trans_Init not to initialize
 * the matrix data area, just the dispatch tables
 */
#define	TRANS_NoInitData	((const MatrixData*) (-1L))
#define	TRANS_Identity		1
#define	TRANS_DoTouch		2
#define	TRANS_ExternalData	4

#if defined(__cplusplus) && !defined(GFX_C_Bind)
/*
 * C++ Binding: Transform class definition
 */
class Transform : public GfxObj
   {
public:
	GFX_CLASS_DECLARE(Transform)

//	Constructors
	Transform();
	Transform(const MatrixData&);
	Transform(const MatrixData*);
	Transform(Transform&);

//	Operators
	virtual Transform&	operator+=(const Transform&);
	virtual Transform&	operator-=(const Transform&);
	virtual Transform&	operator*=(const Transform&);

//	Accessors
	virtual MatrixData* GetData() const;
	virtual Err	 SetData(const MatrixData&);
	virtual Err	 SetData(const MatrixData*);
			bool IsIdentity() const;

//	Operations
	virtual Err	 Identity();
	virtual Err	 Transpose();
	virtual Err	 Transpose(const Transform&);
	virtual Err	 Transpose(const Transform*);
	virtual Err	 Inverse();
	virtual Err	 Inverse(const Transform&);
	virtual Err	 Inverse(const Transform*);
	virtual Err	 Add(const Transform*);
	virtual Err	 Subtract(const Transform*);
	virtual Err	 Mul(const Transform&, const Transform&);
	virtual Err	 Mul(const Transform*, const Transform*);
	virtual Err	 PreMul(const Transform&);
	virtual Err	 PreMul(const Transform*);
	virtual Err	 Translate(const Point3&);
	virtual Err	 Translate(const Point3*);
	virtual Err	 PostMul(const Transform&);
	virtual Err	 PostMul(const Transform*);
	virtual Err	 Scale(const Point3&);
	virtual Err	 Scale(const Point3*);
	virtual Err	 Rotate(int32 axis, gfloat angle);
	virtual Err	 AxisRotate(const Vector3* axis, gfloat angle);
	virtual Err	 AxisRotate(const Vector3& axis, gfloat angle);
	virtual Err	 LookAt(const Point3& v, gfloat twist);
	virtual Err	 LookAt(const Point3* v, gfloat twist);
	virtual Err	 Polarview(gfloat dist, gfloat azim, gfloat inc, gfloat twist);
	virtual Err  Perspective(gfloat fovy, gfloat aspect, gfloat n, gfloat f);
	virtual Err Frustum(const Box3&);
	virtual Err Frustum(const Box3*);
	virtual Err Ortho(const Box3&);
	virtual Err Ortho(const Box3*);
	virtual Err Ortho2(gfloat left, gfloat right, gfloat bottom, gfloat top);
	virtual Err Ortho(gfloat left, gfloat right, gfloat bottom, gfloat top);
	virtual Err	Factor(Vector3* trans, Vector3* scale = NULL,
					   Vector3* rot = NULL);
	virtual const Transform* GetInverse();
	virtual	Err	Push();
	virtual	Err	Pop();
			void Touch();

//	Overrides
	virtual Err Copy(GfxObj*);
	virtual void PrintInfo() const;
Protected:

//	Data members
	MatrixData	data;
	gfloat		hither;		// ?? remove in 1.1
  };

#else

/*
 * C Function Dispatch Table for Character Virtual Functions
 * A Transform is also a GfxObj so the GfxObj functions must
 * go at the top of this table. If a function is added to GfxObj,
 * it must also be added here.
 */
typedef struct
   {
    Err     (*Construct)(Transform* dst);
    void    (*DeleteAttrs)(Transform*);
    void    (*PrintInfo)(const Transform*);
    Err	    (*Copy)(Transform*, Transform*);

	MatrixData* (*GetData)(const Transform*);
	Err (*SetData)(Transform*, const MatrixData*);
	Err (*Identity)(Transform*);
	Err (*Transpose)(Transform* dst, const Transform* src);
	Err (*Inverse)(Transform* dst, const Transform* src);
	Err (*Add)(Transform* dst, const Transform* src);
	Err (*Subtract)(Transform* dst, const Transform* src);
	Err (*Mul)(Transform* dst, const Transform* a, const Transform* b);
	Err (*PreMul)(Transform* dst, const Transform* src);
	Err (*PostMul)(Transform* dst, const Transform* src);
	Err (*Translate)(Transform* dst, const Point3* p);
	Err (*Scale)(Transform* dst, const Point3* p);
	Err (*Rotate)(Transform* dst, int32 axis, gfloat angle);
	Err (*AxisRotate)(Transform* dst, const Vector3* axis, gfloat angle);
	Err (*LookAt)(Transform* dst, const Point3* v, gfloat twist);
	Err (*Polarview)(Transform* dst, gfloat dist, gfloat azim,
					  gfloat inc, gfloat twist);
	Err (*Perspective)(Transform* dst, gfloat fovy, gfloat aspect,
						gfloat n, gfloat f);
	Err (*Frustum)(Transform*, const Box3*);
	Err (*Ortho)(Transform*, const Box3*);
	Err (*Ortho2)(Transform* dst, gfloat left, gfloat right,
				   gfloat bottom, gfloat top);
	Err (*Factor)(Transform*, Vector3*, Vector3*, Vector3*);
	void (*Touch)(Transform*);
/*
 * SurfTrans/ViewTrans only
 */
	const Transform* (*GetInverse)(Transform*);
	Err	(*Push)(Transform*);
	Err	(*Pop)(Transform*);
   } TransFuncs;

extern TransFuncs Trans_Func;

/*
 * Transform is a structure that represents the INTERNAL FORMAT of
 * class Transform in the C binding. DO NOT RELY ON THE FIELDS IN THIS
 * STRUCTURE - THEY ARE SUBJECT TO CHANGE WITHOUT NOTICE! Transform
 * attributes should only be accessed by the Trans_XXX functions.
 */
struct Transform
   {
	GfxObj		obj;					/* standard object header */
	MatrixData	data;					/* 4x4 matrix data area */
	gfloat		hither;					/* ?? remove in 1.1 */
   };

#ifdef __cplusplus
extern "C" {
#endif

/****
 *
 * PUBLIC C API
 *
 ****/
Transform*	Trans_Create(const MatrixData*);
Err			Trans_Init(Transform* m, const MatrixData* d);
void		Trans_Print(const Transform*);
MatrixData* Trans_GetData(const Transform* m);
const Transform* Trans_GetInverse(Transform* m);
Err			Trans_SetData(Transform* m, const MatrixData* d);
Err			Trans_Identity(Transform* m);
Err			Trans_Transpose(Transform* dst, const Transform* src);
Err			Trans_Inverse(Transform* dst, const Transform* src);
Err			Trans_Add(Transform* dst, const Transform* src);
Err			Trans_Subtract(Transform* dst, const Transform* src);
Err			Trans_Mul(Transform* dst, const Transform* a, const Transform* b);
Err			Trans_PreMul(Transform* dst, const Transform* src);
Err			Trans_PostMul(Transform* dst, const Transform* src);
Err			Trans_Translate(Transform* m, const Point3* p);
Err			Trans_Scale(Transform* m, const Point3* p);
Err			Trans_Rotate(Transform* m, int32 axis, gfloat angle);
Err			Trans_LookAt(Transform* m, const Point3* v, gfloat twist);
Err			Trans_Polarview(Transform* m, gfloat dist, gfloat azim,
							gfloat inc, gfloat twise);
Err			Trans_AxisRotate(Transform* m, const Vector3* axis, gfloat angle);
Err			Trans_Perspective(Transform* m, gfloat fovy, gfloat aspect,
							  gfloat hither, gfloat yon);
Err			Trans_Frustum(Transform* m, const Box3* b);
Err			Trans_Ortho(Transform* m, const Box3* b);
Err			Trans_Ortho2(Transform* m, gfloat left, gfloat right,
						 gfloat top, gfloat bottom);
Err			Trans_Factor(Transform* m, Vector3* trans,
						Vector3* scale, Vector3* rot);
void		Trans_Touch(Transform* m);
#ifdef __cplusplus
}
#endif

#endif

#include <graphics/pipe/mtx.inl>

#endif /* __GRAPHICS_PIPE_MTX_H */
