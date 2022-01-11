#ifndef __GRAPHICS_PIPE_SURF_H
#define __GRAPHICS_PIPE_SURF_H


/******************************************************************************
**
**  @(#) surf.h 96/02/20 1.62
**
**  Surfaces: Recording engine for the 3D graphics pipeline
**
******************************************************************************/


/*
 * Material and Texture index options
 */
#define	SURF_None		-1		/* turn material/texture off */
#define	SURF_UseLast	-2		/* use material/texture from last primitve */

/*
 * Forward references
 */
struct MatProp;

#if defined(__cplusplus) && !defined(GFX_C_Bind)
/***
 *
 * C++ API
 *
 ***/

/****
 *
 * Surface is the virtual base class for both compiled surfaces (CSurface)
 * and GP-base surface (GPSurface). It does not contain a data area
 * so that subclasses may define their own.
 *
 ****/
class Surface : public GfxObj
   {
	GFX_CLASS_DECLARE(Surface)
public:
	Surface(int32 = 0);
	virtual	~Surface();

	virtual	Err		Copy(GfxObj*);
	virtual	Err		Display(GP*, TexBlend**, struct MatProp*);
	virtual	Err		CalcBound(Box3*);
	virtual Err 	FindGeometry(int32 = 0, Geometry*);
	virtual Err 	FindGeometry(int32 = 0, Geometry&);
	Err				GetBound(Box3*);
	Err				AddGeometry(Geometry*, int32, int32);
	Err				AddGeometry(Geometry&, int32, int32);
	Err				AddPrim(uint32*);
	Err				MakeColors();
	void			Touch();
	Err				Reset();

Protected:
	virtual char*	Extend(int32);	// grow surface data area

    char*		m_FirstPrim;		// data area for geometry
	char*		m_LastPrim;			// -> last primitive added
	Box3		m_Bound;			// bounding volume

    uint32  **m_CmdList;            /* TE command lists */
    uint32  m_NumCmdLists;          /* how many command lists */
    uint32  m_NumTriangles;         /* Computed for statistical purposes only */
    uint32  m_NumVerts;             /* Computed for statistical purposes only */
  };

GFX_CLASS_REF(SurfRef,Surface)

#else


/****
 *
 * Dispatch vector for C binding of basic Surface
 *
 ****/
typedef struct
   {
	ObjFuncs	base;
	Err			(*Display)(Surface*, GP*, TexBlend**, struct MatProp*);
	Err			(*CalcBound)(Surface*, Box3*);
	Err			(*FindGeometry)(Surface*, int32, Geometry*);
	Err			(*AddGeometry)(Surface*, Geometry*, int32, int32);
	char*		(*Extend)(Surface*, int32);
   } SurfFuncs;

/*
 * SurfaceData is a structure that represents the INTERNAL FORMAT of
 * class Surface in the C binding. DO NOT RELY ON THE FIELDS IN THIS
 * STRUCTURE - THEY ARE SUBJECT TO CHANGE WITHOUT NOTICE! GPSurface
 * attributes should only be accessed by the Surf_XXX functions.
 */
typedef struct SurfaceData
   {
	GfxObj		m_Obj;
	char*		m_FirstPrim;		/* -> first primitive added */
	char*		m_LastPrim;			/* -> last primitive added */
    Box3		m_Bound;    		/* bounding box */

    uint32  m_NumTriangles;         /* Computed for statistical purposes only */
    uint32  m_NumVerts;             /* Computed for statistical purposes only */
   } SurfaceData;

#ifdef __cplusplus
extern "C" {
#endif

/***
 *
 * PUBLIC C API
 *
 ***/
void		Surf_Touch(Surface*);
Err			Surf_FindGeometry(Surface*, int32, Geometry*);
Surface*	Surf_Create(void);
Err			Surf_GetBound(Surface*, Box3*);
Err			Surf_AddGeometry(Surface*, Geometry*, int32, int32);
Err			Surf_MakeColors(Surface*, GP*);
void		Surf_AddPrim(Surface*, uint32*);
#ifdef __cplusplus
}
#endif

#endif

#include <graphics/pipe/surf.inl>


#endif /* __GRAPHICS_PIPE_SURF */
