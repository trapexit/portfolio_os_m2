#ifndef __GRAPHICS_PIPE_GPCLASS_H
#define __GRAPHICS_PIPE_GPCLASS_H


/******************************************************************************
**
**  @(#) gpclass.h 96/02/20 1.80
**
**  Graphics Pipeline Class
**
******************************************************************************/


#define   GP_MaxLights         7

/*
 * Geometry pipeline capabilities
 */

#define	GP_All					0xFF	/* all capabilities */
#define	GP_Lighting				0x1		/* lighting calculations */
#define	GP_Texturing			0x2		/* texture interpolation */
#define	GP_Dithering			0x4		/* enable dithering */
#define	GP_Clipping				0x8		/* enable clipping */
#define	GP_PerspCorrect			0x10	/* enable persp correct texturing */

/*
 * Rendering environment controls
 */
#define	GP_SoftRender	GFX_GP
#define	GP_GLRender		GFX_GPGL

/*
 * VertexOrder values
 */
#define	GP_Clockwise		0	/* vertices in clockwise order */
#define	GP_CounterClockwise	1	/* vertices in counter-clockwise order */
/*
 * CullFaces values
 */
#define	GP_None		0		/* don't cull any faces */
#define	GP_Front	1		/* cull front faces */
#define	GP_Back		2		/* cull back faces */

/*
 * HiddenSurf values
 */
/* 		GP_None		0	 * don't do any hidden surface removal */
#define	GP_ZBuffer	1	/* Z buffering hidden surface removal */

/*
 * Begin options
 */
#define	GP_ClearHiddenSurf	1	/* clear hidden surface buffer */
#define	GP_ClearScreen		2	/* clear screen to background color */
#define	GP_ClearAll			3	/* clear screen and hidden surface buffer */

/*
 * Shading Enables for materials
 */
#define MAT_Ambient		0x0001   /* ambient lighting computations */
#define MAT_Diffuse		0x0002   /* diffuse lighting computations */
#define MAT_Specular	0x0004   /* specular lighting computations */
#define MAT_Emissive	0x0008   /* emissive lighting computations */
#define MAT_TwoSided	0x0010   /* two-sided lighting computations */

/*
 * Material Properties
 */
typedef struct MatProp
   {
	uint32	ShadeEnable;
    Color4	Diffuse;
    Color4	Specular;
    Color4	Emission;
    Color4	Ambient;
    gfloat  Shine;
   } MatProp;

/*
 * Light types
 */
#define LIGHT_Point         0
#define LIGHT_Directional   1
#define LIGHT_Spot          2
#define LIGHT_SoftSpot      3

/*
 * Light Properties
 */
typedef struct LightProp
   {
    int32       Kind;
	Point3		Position;
	Vector3		Direction;
    Color4		Color;
    gfloat      Angle;
    gfloat      FallOff;
    bool        Enabled;
   } LightProp;

/*
 * Forward references
 */
struct LightInfo;

/*
 * Internal structures needed for GP declaration. These structures
 * are NOT PUBLIC but are used internally by class GP. THEY ARE
 * SUBJECT TO CHANGE WITHOUT NOTICE. Please do not rely on fields
 * in these structures.
 */

#if defined(__cplusplus) && !defined(GFX_C_Bind)
/***
 *
 * C++ API
 *
 ***/
class ProjTrans : public Transform
   {
Protected:
    inline ProjTrans() { };
    void Update();
  };

class GP : public GfxObj
   {
public:
	GFX_CLASS_DECLARE(GP)
/*
 * Initialization and Conversion
 */
	GP(uint32 = GFX_GP);
	~GP();
/*
 * Attribute Update Overrides
 */
	Err		SetCullFaces(int32);
	Err		SetHiddenSurf(int32);
	Err		SetBackColor(const Color4&);
	Err		SetBackColor(const Color4*);
	Err		SetAmbient(const Color4&);
	Err		SetAmbient(const Color4*);
	Err		SetMaterial(const MatProp*);
	Err		SetTexBlend(TexBlend*);
	Err		SetViewport(const Box2*);
	Err		SetViewport(const Box2&);
	Err		SetLight(int32 index, const LightProp*);
	Err		SetGState(GState* gs);
/*
 * Rendering Overrides
 */
	Err		Draw(Geometry&);
	Err		Draw(Geometry*);
	Err		DrawLines(int32 n, Point3* l);
	Err		DrawPoints(int32 n, Point3* l);
	Err		DrawTriList(int16 style, int32 n, Point3* l,
					Vector3* v = NULL, Color4* c = NULL, TexCoord* t = NULL);
	Err		DrawTriFan(int16 style, int32 n, Point3* l,
					Vector3* v = NULL, Color4* c = NULL, TexCoord* t = NULL);
	Err		DrawTriStrip(int16 style, int32 n, Point3* l,
					Vector3* v = NULL, Color4* c = NULL, TexCoord* t = NULL);
	Err		DrawQuadMesh(int16 style, int32 x, int32 y, Point3* l,
					Vector3* v = NULL, Color4* c = NULL, TexCoord* t = NULL);
/*
 * Other Overrides
 */
	Err		Clear(int32);
	Err		Flush();
	Err		Push();
	Err		Pop();
	Err		Enable(uint32 capability);
	Err		Disable(uint32 capability);
	Err		ClearLights();
/*
 * New Attribute Inquiry
 */
	uint32			GetCapabilities();
	int32			GetCullFaces();
	int32			GetHiddenSurf();
	const Color4&	GetBackColor();
	void			GetBackColor(Color4*);
	const Color4&	GetAmbient();
	void			GetAmbient(Color4*);
	MatProp*		GetMaterial();
	Transform*		GetProjection();
	TexBlend*		GetTexBlend();
	Err				GetDisplayLimits(uint32* w, uint32* h, uint32* d);
	void			GetViewport(Box2*);
	const Box2&		GetViewport();
	TexBlend*		GetTxbDefault();
	GState*			GetGState();

	virtual	Err		GetLight(int32, LightProp*);
	virtual Err     SetDestBuffer(BitmapPtr);
	virtual Err     SetZBuffer(BitmapPtr);
	virtual BitmapPtr GetDestBuffer();
	virtual BitmapPtr GetZBuffer();

	virtual int32	IsInView(Box3 *b);

Protected:
/*
 * Internal functions
 */
	Err				Copy(GfxObj*)	{ return GFX_ErrorNotImplemented; }
    virtual	Err	    display_surf(Surface* s, TexBlend** t, MatProp* m);
/*
 * Data members
 */
	Stack	m_AttrStk;

#include <graphics/pipe/gpdata.h>
  };

typedef GP GPData;

#else

#ifdef __cplusplus
extern "C" {
#endif

/***
 *
 * PUBLIC C API
 *
 ***/

/*
 * API for GP
 */
GP*				GP_Create(uint32 handle);
Err	 			GP_Snapshot(GP*, char*);
Err 			GP_GetDisplayLimits(GP* gp, uint32* w, uint32* h, uint32* d);
int32			GP_GetCullFaces(GP*);
int32			GP_GetHiddenSurf(GP*);
void			GP_GetBackColor(GP*, Color4*);
void			GP_GetAmbient(GP*, Color4*);
MatProp*		GP_GetMaterial(GP*);
TexBlend*		GP_GetTexBlend(GP*);
Transform*		GP_GetProjection(GP*);
void			GP_GetViewport(GP*, Box2*);
TexBlend*		GP_GetTexBlend(GP*);
Err				GP_GetLight(GP*, int32, LightProp*);
Err     		GP_SetDestBuffer(GP*, BitmapPtr);
Err	     		GP_SetZBuffer(GP*, BitmapPtr);
BitmapPtr	 	GP_GetDestBuffer(GP*);
BitmapPtr	 	GP_GetZBuffer(GP*);
Err				GP_SetViewport(GP*, const Box2*);
TexBlend*		GP_GetTxbDefault(GP*);
int32			GP_IsInView(GP*, Box3*);
uint32			GP_GetCapabilities(GP*);
Err				GP_SetGState(GP* gp, GState* gs);
GState*			GP_GetGState(GP* gp);

/*
 * API for GP
 */
Transform*		GP_GetModelView(GP*);
Err				GP_Clear(GP*, int32);
Err				GP_Flush(GP*);
Err				GP_Push(GP*);
Err				GP_Pop(GP*);
Err				GP_Enable(GP*, uint32);
Err				GP_Disable(GP*, uint32);
Err				GP_ClearLights(GP*);
Err				GP_SetCullFaces(GP*, int32);
Err				GP_SetHiddenSurf(GP*, int32);
Err				GP_SetBackColor(GP*, const Color4*);
Err				GP_SetAmbient(GP*, const Color4*);
Err				GP_SetMaterial(GP*, const MatProp*);
Err				GP_SetTexBlend(GP*, TexBlend*);
Err				GP_SetLight(GP*, int index, const LightProp*);

Err				GP_Draw(GP*, Geometry*);
Err				GP_DrawPoints(GP*, int32 nv, Point3* l);
Err				GP_DrawLines(GP*, int32 nv, Point3* l);
Err				GP_DrawTriList(GP*, int16 style, int32 nv,
						Point3* l, Vector3* n, Color4* c, TexCoord* t);
Err				GP_DrawTriFan(GP*, int16 style, int32 nv,
						Point3* l, Vector3* n, Color4* c, TexCoord* t);
Err				GP_DrawTriStrip(GP*, int16 style, int32 nv,
						Point3* l, Vector3* n, Color4* c, TexCoord* t);
Err				GP_DrawQuadMesh(GP*, int16 style, int32 x, int32 y,
						Point3* l, Vector3* n, Color4* c, TexCoord* t);

#ifdef __cplusplus
}
#endif

/****
 *
 * Dispatch vector for C binding GP public and private functions.
 * In the C binding, this dispatch vector is used for polymorphism
 * between different types of graphics pipelines (GL-based, software based
 * or display script).
 *
 ****/
typedef struct
   {
    ObjFuncs    base;
	Err		(*SetCullFaces)(GP*, int32);
	Err		(*SetHiddenSurf)(GP*, int32);
	Err		(*SetBackColor)(GP*, const Color4*);
	Err		(*SetAmbient)(GP*, const Color4*);
	Err		(*SetMaterial)(GP*, const MatProp*);
	Err		(*SetTexBlend)(GP*, TexBlend*);
	Err		(*SetViewport)(GP*, const Box2*);
	Err		(*Enable)(GP*, uint32);
	Err		(*Disable)(GP*, uint32);
	Err		(*Push)(GP*);
	Err		(*Pop)(GP*);
	Err		(*Clear)(GP*, int32);
	Err		(*Flush)(GP*);
	Err		(*SetLight)(GP*, int32, const LightProp*);
	Err		(*ClearLights)(GP*);
	Err		(*Draw)(GP*, Geometry*);
	Err		(*GetLight)(GP*, int32 index, LightProp*);
	int32	(*IsInView)(GP*, Box3*);
	Err     (*SetDestBuffer)(GP*, BitmapPtr);
    Err     (*SetZBuffer)(GP*, BitmapPtr);
	BitmapPtr (*GetDestBuffer)(GP*);
	BitmapPtr (*GetZBuffer)(GP*);
   } GPFuncs;

typedef Transform ProjTrans;

/*
 * GPData is a structure that represents the INTERNAL FORMAT of
 * class GP in the C binding. DO NOT RELY ON THE FIELDS IN THIS
 * STRUCTURE - THEY ARE SUBJECT TO CHANGE WITHOUT NOTICE! GP
 * attributes should only be accessed by the GP_XXX functions.
 */
typedef struct GPData
   {
	GfxObj	m_Obj;		/* base object (Surface) */
	Stack	m_AttrStk;

#include <graphics/pipe/gpdata.h>
   } GPData;

#endif

#include <graphics/pipe/gp.inl>


#endif /* __GRAPHICS_PIPE_GPCLASS_H */
