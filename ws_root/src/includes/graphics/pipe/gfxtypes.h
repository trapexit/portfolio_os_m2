#ifndef __GRAPHICS_PIPE_GFXTYPES_H
#define __GRAPHICS_PIPE_GFXTYPES_H


/******************************************************************************
**
**  @(#) gfxtypes.h 96/07/09 1.51
**
**  Basic types and constants for 3D libraries
**
******************************************************************************/


#ifndef PI
#define PI 3.141592653589793
#endif

#if defined(__cplusplus) && !defined(GFX_C_Bind)

/***
 *
 * Classes defined in M2
 *
 ***/

/***
 *
 * C++ Pipeline classes
 *
 ***/
class Vector3;
class Point3;
class Point4;
class Box3;
class Color4;
class Geometry;
	class QuadMesh;
	class TriMesh;
class MatProp;
class GfxObj;
	class GP;
	class PipTable;
	class TexBlend;
	class Texture;
	class Transform;
	class Surface;

/***
 *
 * C++ Framework classes
 *
 ***/
class GfxObj;
	class Fog;
	class Scene;
	class SDF;
	class Engine;
	class Character;
		class Model;
		class Light;
		class Camera;
class CharIter;

#else
/***
 *
 * C Pipeline Objects
 *
 ***/
#define Point3 Vector3

typedef struct Vector3 Vector3;
typedef struct Point4 Point4;
typedef struct Box3 Box3;
typedef struct Color4 Color4;
typedef struct Geometry Geometry;
typedef struct QuadMesh QuadMesh;
typedef struct TriMesh TriMesh;
typedef struct Transform Transform;
typedef struct GfxObj GfxObj;
typedef struct GfxObj GP;
typedef struct GfxObj Surface;

/***
 *
 * C Framework Objects
 *
 ***/
#ifdef _PIP_INTERNAL
typedef struct PipData PipTable;
#else
typedef struct GfxObj PipTable;
#endif

#ifdef _TXB_INTERNAL
typedef struct TxbData TexBlend;
#else
typedef struct GfxObj TexBlend;
#endif

#ifdef _TEX_INTERNAL
typedef struct TexData Texture;
#else
typedef struct GfxObj Texture;
#endif

#ifdef _SCENE_INTERNAL
typedef struct SceneData Scene;
#else
typedef struct GfxObj Scene;
#endif

#ifdef _SDF_INTERNAL
typedef struct SDFData SDF;
#else
typedef struct GfxObj SDF;
#endif

#ifdef _ENG_INTERNAL
typedef struct EngineData Engine;
#else
typedef struct GfxObj Engine;
#endif

#ifdef _ARRAY_INTERNAL
typedef struct ArrayData Array;
#else
typedef struct GfxObj Array;
#endif

#ifdef _CHAR_INTERNAL
typedef struct CharData Character;
#else
typedef struct GfxObj Character;
#endif

#ifdef _CAM_INTERNAL
typedef struct CameraData Camera;
#else
typedef struct GfxObj Camera;
#endif

#ifdef _MOD_INTERNAL
typedef struct ModelData Model;
#else
typedef struct GfxObj Model;
#endif

#ifdef _LIGHT_INTERNAL
typedef struct LightData Light;
#else
typedef struct GfxObj Light;
#endif

typedef struct CharIter CharIter;

#endif

/*
 * Framework object types
 */
#define	GFX_GP				1
#define	GFX_Surface			2
#define	GFX_Texture			3
#define	GFX_Transform		4
#define	GFX_ProjTrans		5
#define GFX_PipTable		6
#define GFX_TexBlend		7

#define	GFX_Scene			8
#define	GFX_Array			9
#define	GFX_Engine			10
#define	GFX_CharTrans		11
#define	GFX_Character		12

#ifndef Protected
#undef Protected
#endif
#define	Protected	protected

#ifdef BUILD_STRINGS
#define	_DEBUG
#endif


#define GfxBitmap Bitmap
typedef struct Bitmap * BitmapPtr;

#endif /* __GRAPHICS_PIPE_GFXTYPES_H */
