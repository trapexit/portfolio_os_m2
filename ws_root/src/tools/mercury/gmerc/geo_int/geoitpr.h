/****
 *
 *	@(#) geoitpr.h 6/10/96
 *	Copyright 1996, The 3DO Company
 *
 ****/
#ifndef _GEOITPR_
#define _GEOITPR_

#include "kerneltypes.h"
#include "gp.h"
#include "kf_spline.h"
#include "kf_eng.h"

#define	SDF_CompileSurfaces	0x1		/* compile all primitives */
#define	SDF_SnakePrimitives	0x2		/* snake primitives */

/* Built-in class IDs */
#define Class_Scene			1
#define Class_Character		2
#define Class_Model			3
#define Class_Camera		4
#define Class_Light			5
#define Class_Surface		6
#define Class_Texture		7
#define Class_TexBlend		8
#define Class_Transform		9
#define Class_PipTable		10 
#define Class_Engine		11 
#define Class_Link_Array	12 
#define Class_Anim			19 

/* this is added for reader/parser and geolib interface */

#define Class_Mat_Array		13
#define Class_Tex_Array		14
#define Class_Obj_Array		15
#define Class_Float_Array	16
#define Class_Color_Array	17
#define Class_Point_Array	18

#define Class_Data_Array	20
#define Class_Mata_Array	26

#define Class_Int32 		21
#define Class_Float			22
#define Class_Point3		23
#define Class_Color4		24
#define Class_Box3			25

typedef struct gTexBlend
{
  char filename[32];
  int32 pageindex;
  int32 subindex;
  uint32 xwrap;
  uint32 ywrap;
  uint16 xsize;
  uint16 ysize;
  uint8  lod;
  uint8  used;
} gTexBlend;

typedef struct CharData
{
  /* Everything in GfxObj */
  ObjFuncs*	m_Funcs;		/* -> virtual function table */
  int8		m_Type;			/* type GFX_xxx */
  int8		m_Flags;		/* memory allocation flags */
  int16		m_Use;			/* reference count */
  int32		m_Objid;		/* obj id */
  void		*m_Ref;			/* reference pointer */
  /* CharData specific */
  GfxObj		m_Obj;
  char		        name[64];
  Box3		        m_Bound;
  Transform	        m_Transform;
  struct CharData*      m_Parent;
  struct CharData*      m_First;
  struct CharData*      m_Next;
  void*		        m_UserData;
} CharData;


typedef struct ModelData
{
  /* Everything in GfxObj */
  ObjFuncs*	m_Funcs;		/* -> virtual function table */
  int8		m_Type;			/* type GFX_xxx */
  int8		m_Flags;		/* memory allocation flags */
  int16		m_Use;			/* reference count */
  int32		m_Objid;		/* obj id */
  void		*m_Ref;			/* reference pointer */
  /* ModelData specific */
  CharData	m_Char;                 /* parent class is Character */
  GfxRef	m_Surface;              /* surface geometry */
  Array*	m_Materials;	        /* material array */
  Array*	m_Textures;	        /* texture array */
  uint16	m_NumPod;	        /* number of pod */
  uint16	m_ModelIndex;	        /* model index */
  uint32        m_Visited;
} ModelData;


void InterpretSDFTokens(char *tokens);
void InterpretGroup(CharData *pd);
void InterpretModel(void);
void InterpretSurface(ModelData *tmodel);
void InterpretTexgen(TexGenProp *texgen);
void InterpretTriangles(SurfaceData *tsurface,int32 autoNormal,int32 matindex,int32 texindex,TexGenProp *texgen);
void InterpretTristrip(SurfaceData *tsurface,int32 autoNormal,int32 matindex,int32 texindex,TexGenProp *texgen);
void InterpretTrifan(SurfaceData *tsurface,int32 autoNormal,int32 matindex,int32 texindex,TexGenProp *texgen);
void InterpretQuadmesh(SurfaceData *tsurface,int32 autoNormal,int32 matindex,int32 texindex,TexGenProp *texgen);
void InterpretTrimesh(SurfaceData *tsurface, int32 autoNormal, int32 matindex, int32 texindex,TexGenProp *texgen);
void InterpretVertexlist(TriMesh *tm);
void InterpretEngine(void);
void InterpretMatArray(void);
void InterpretTexArray(void);
void InterpretMaterial(MatProp *mat);
void InterpretTexture(gTexBlend *txb);
void InterpretKFObjArray(void);
void InterpretKFObject(KfEngine *kf);

/* Char method */
CharData *Char_Create(void);
void Char_Delete(CharData *cd);
void Char_Attach(CharData *pd, CharData *cd, int32 *tokens);
void Char_mDelete(CharData *cd);

/* Model method */
ModelData *Model_Create(void);
void Model_Delete(ModelData *md);

/* surface method */
SurfaceData *Surf_Create(void);
void Surf_Delete(SurfaceData *sd);

/* GeoPrim method */
GeoPrim *GeoPrim_Create(int32 type);
void GeoPrim_Delete(GeoPrim *geo);

/* Glib method */
void Glib_Init(void);
void Glib_Delete(void);
int32 Glib_GetRootCharCount(void);
CharData *Glib_GetRootChar(int32 index);
int32 Glib_GetModelCount(void);
ModelData *Glib_GetModel(int32 index);
void Glib_AddRootChar(CharData *am);
void Glib_AddMat(MatProp *mat);
MatProp *Glib_GetMat(int32 i);
int32 Glib_GetMatCount(void);
int32 Glib_GetMatId(void);
void Glib_SetMatId(int32 id);
void Glib_AddTexBlend(gTexBlend *txb);
gTexBlend *Glib_GetTexBlend(int32 i);
int32 Glib_GetTexBlendIndex(void);
void Glib_MarkTexBlendUsed(int32 i);
int32 Glib_GetMarkedTexBlend(void);
void Glib_SetTexArrayName(char *tname);
void Glib_SetMatArrayName(char *mname);
char *Glib_GetTexArrayName(void);
char * Glib_GetMatArrayName(void);
void Glib_AddKfEng(KfEngine *kf);
int32 Glib_GetKfEngCount(void);
KfEngine *Glib_GetKfEng(int32 i);
void Glib_AddModel(ModelData *m);
int32 Glib_GetModelIndex(ModelData *md);

/* Symbol method */
void Symbol_Init(void);
void Symbol_Delete(void);

/* material method */
MatProp *Mat_Create(void);
void Mat_Delete(MatProp *mat);

/* texblend method */
gTexBlend* Txb_Create(void);
void Txb_Delete(gTexBlend *txb);

/* Transform method */
void Trans_Create(Transform *tr);
void Trans_Copy(Transform* dst_obj, Transform* src_obj);
void Trans_Delete(Transform* tr);

/* KFObject method */
KfEngine *KfEng_Create(void);
void KfEng_Delete(KfEngine *kf);

/* Array method */
Array *Array_Create(void);
void Array_Delete(Array *a);

/* ArrayElement method */
void Array_CreateElement(Array *a);
void Array_DeleteElement(Array *a);

#endif /* GEOITPR */
