/*
 *	@(#) geointerface.c 3/21/96
 *	Copyright 1996, The 3DO Company
 */

		
/****
 *
 * This is a interface between the reader/parser module and the geometry library
 *
 * The first step is to saperate the 3do parser (sdf parser with the rest of the
 * gcomp code. As an immediate step, a temporary mechanism will be used to gap
 * the framework parser and geometry processing code. We will take the data out of
 * the framework dictionary and move it into geometry library internal data structure.
 * The next step will be swapping all of the framework reader/parser out of the 
 * project and repalce it with the new ASCII SDF parser.
 *
 ****/
 

/* interface */
#include <stdlib.h>
#include <string.h>
#include "geoitpr.h"


#define MAXARTIMODEL 	200
#define MAXMODEL 	800
#define MAXMATERIAL 	200
#define MAXTEXBLEND 	200
#define MAXKFENGINE 	200

typedef struct GeoLibData
{
  CharData	*gRootChar[MAXARTIMODEL];
  int32 		currRootChar;
  ModelData	*gModel[MAXMODEL];
  int32 		currModel;
  MatProp	*gMaterial[MAXMATERIAL];
  int32 		currMaterial;
  int32		currMaterialId;
  gTexBlend	*gTexBlend[MAXTEXBLEND];
  int32 		currTexture;
  KfEngine	*KfEngine[MAXKFENGINE];
  int32		currKfEngine;
  char            matname[64];
  char            texname[64];
} GeoLibData;

static GeoLibData gLib;


/* char method */
CharData*
Char_Create(void)
{
  CharData *cd;
	
  cd = (CharData *)malloc(sizeof(CharData));
  memset(cd, 0, sizeof(CharData));
  Trans_Create(&cd->m_Transform);
  ((GfxObj *)cd)->m_Type = Class_Character;
  return cd;
}

void
Char_Delete
(
 CharData *cd
 )
{
  if (cd)
    free(cd);
}

void
Char_mDelete
(
 CharData *cd
 )
{
  CharData *child, *sibling;
  if (cd)
    {
      child = (CharData *)cd->m_First;
      Char_mDelete(child);
      sibling = (CharData *)cd->m_Next;
      Char_mDelete(sibling);
      Char_Delete(cd);
    }
}

void
Char_Attach
(
 CharData *pd,
 CharData *cd,
 int32 *tokens
 )
{
  CharData *child;
  if (pd)
    {
      if (pd->m_First)
	{
	  child = pd->m_First;
	  while (child->m_Next)
	    child = child->m_Next;
	  child->m_Next = cd;
	}
      else
	pd->m_First = cd;
      cd->m_Parent = pd;
    }
}

/* model method */
ModelData*
Model_Create(void)
{
  ModelData *md;
	
  md = (ModelData *)malloc(sizeof(ModelData));
  memset(md, 0, sizeof(ModelData));
  Trans_Create(&((CharData *)md)->m_Transform);
  ((GfxObj *)md)->m_Type = Class_Model;
  return md;
}

void
Model_Delete
(
 ModelData *md
 )
{
  if (md)
    {
      Surf_Delete((SurfaceData *)md->m_Surface);
      Array_Delete(md->m_Materials);
      Array_Delete(md->m_Textures);
      free(md);
    }
}

void
Glib_AddModel
(
 ModelData *m
 )
{
  m->m_ModelIndex = gLib.currModel;
  gLib.gModel[gLib.currModel] = m;
  gLib.currModel++;
}

int32
Glib_GetModelIndex
(
 ModelData *md
 )
{
  int32 i;
  for (i = 0; i < gLib.currModel; i++)
    if (md == gLib.gModel[i])
      return i;
  return -1;
}

/* surface method */
SurfaceData*
Surf_Create(void)
{
  SurfaceData *sd;
	
  sd = (SurfaceData *)malloc(sizeof(SurfaceData));
  memset(sd, 0, sizeof(SurfaceData));
  ((GfxObj *)sd)->m_Type = Class_Surface;
  return sd;
}

void
Surf_Delete
(
 SurfaceData *sd
 )
{
  GeoPrim *geo;
  char *ptr;
  if (sd)
    {
      geo = (GeoPrim *)sd->m_FirstPrim;
      while (geo)
	{
	  ptr = (char *)geo;
	  geo = (GeoPrim *)geo->nextPrim;
	  GeoPrim_Delete((GeoPrim *)ptr);
	}
      free(sd);
    }
}

/* GeoPrim method */
GeoPrim*
GeoPrim_Create
(
 int32 type
 )
{
  GeoPrim *geo;
  int32 size;
	
  if (type == GEO_GLTriMesh)
    size = sizeof(GeoPrim) - sizeof(Geometry) + sizeof(TriMesh);
  else if (type == GEO_QuadMesh)
    size = sizeof(GeoPrim) - sizeof(Geometry) + sizeof(QuadMesh);
  else
    size = sizeof(GeoPrim);
  geo = (GeoPrim *)malloc(size);
  memset(geo, 0, size);
  return geo;
}

void
GeoPrim_Delete
(
 GeoPrim *geo
 )
{
  if (geo)
    free(geo);
}

/* GeoLib method */

void
Glib_Init(void)
{
  memset(&gLib, 0, sizeof(GeoLibData));
}

void
Glib_Delete(void)
{
  CharData *cd;
  ModelData *md;
  MatProp *mat;
  gTexBlend *txb;
  KfEngine *kf;
  int32 i;

  for (i = 0; i < Glib_GetRootCharCount(); i++)
    {
      cd = Glib_GetRootChar(i);
      if (((GfxObj *)cd)->m_Type == Class_Model)
	Model_Delete((ModelData *)cd);
      Char_mDelete(cd);
    }
  if (Glib_GetRootCharCount() <= 0)
    {
      for (i = 0; i < Glib_GetModelCount(); i++)
	{
	  md = Glib_GetModel(i);
	  Model_Delete(md);
	}
    }
  for (i = 0; i < Glib_GetMatCount(); i++)
    {
      mat = Glib_GetMat(i);
      Mat_Delete(mat);
    }
  for (i = 0; i < Glib_GetTexBlendIndex(); i++)
    {
      txb = Glib_GetTexBlend(i);
      Txb_Delete(txb);
    }
  for (i = 0; i < Glib_GetKfEngCount(); i++)
    {
      kf = Glib_GetKfEng(i);
      KfEng_Delete(kf);
    }
}

int32
Glib_GetRootCharCount(void)
{
  return gLib.currRootChar;
}

CharData *
Glib_GetRootChar
(
 int32 index
 )
{
  return gLib.gRootChar[index];
}

int32
Glib_GetModelCount(void)
{
  return gLib.currModel;
}

ModelData *
Glib_GetModel
(
 int32 index
 )
{
  return gLib.gModel[index];
}

/* Articulated model method */

void
Glib_AddRootChar
(
 CharData *am
 )
{
  gLib.gRootChar[gLib.currRootChar] = am;
  gLib.currRootChar++;
}

/* material method */

MatProp*
Mat_Create(void)
{
  MatProp *mat;
	
  mat = (MatProp *)malloc(sizeof(MatProp));
  memset(mat, 0, sizeof(MatProp));
  /*
    Col_SetRGB(&mat->Diffuse, 0.7, 0.7, 0.7);
    Col_SetRGB(&mat->Specular, 1, 1, 1);
    Col_SetRGB(&mat->Emission, 0, 0, 0);
    Col_SetRGB(&mat->Ambient, 0.3, 0.3, 0.3);
    mat->Shine = 0.05;
    */
  return mat;
}

void
Mat_Delete
(
 MatProp *mat
 )
{
  if (mat)
    free(mat);
}

void
Glib_AddMat
(
 MatProp *mat
 )
{
  gLib.gMaterial[gLib.currMaterial] = mat;
  gLib.currMaterial++;
}

MatProp *
Glib_GetMat
(
 int32 i
 )
{
  return gLib.gMaterial[i];
}

int32
Glib_GetMatCount(void)
{
  return gLib.currMaterial;
}

int32
Glib_GetMatId(void)
{
  return gLib.currMaterialId;
}

void
Glib_SetMatId
(
 int32 id
 )
{
  gLib.currMaterialId = id;
}

/* texture blend method */

gTexBlend*
Txb_Create(void)
{
  gTexBlend *txb;
	
  txb = (gTexBlend *)malloc(sizeof(gTexBlend));
  memset(txb, 0, sizeof(gTexBlend));
  txb->pageindex = -1;
  txb->subindex = -1;
  return txb;
}

void
Txb_Delete
(
 gTexBlend *txb
 )
{
  if (txb)
    free(txb);
}

void
Glib_AddTexBlend
(
 gTexBlend *txb
 )
{
  gLib.gTexBlend[gLib.currTexture] = txb;
  gLib.currTexture++;
}

gTexBlend *
Glib_GetTexBlend
(
 int32 i
 )
{
  return gLib.gTexBlend[i];
}

int32
Glib_GetTexBlendIndex(void)
{
  return gLib.currTexture;
}

void
Glib_MarkTexBlendUsed
(
 int32 i
 )
{
  gLib.gTexBlend[i]->used = 1;
}

int32
Glib_GetMarkedTexBlend(void)
{
  int32 i, count = 0;
  for (i = 0; i < gLib.currTexture; i++)
    if (gLib.gTexBlend[i]->used)
      count++;
  return count;
}

void
Glib_SetTexArrayName
(
 char *tname
 )
{
  strcpy(gLib.texname, tname);
}

void
Glib_SetMatArrayName
(
 char *mname
 )
{
  strcpy(gLib.matname, mname);
}

char *
Glib_GetTexArrayName(void)
{
  return gLib.texname;
}

char *
Glib_GetMatArrayName(void)
{
  return gLib.matname;
}

/* Transform method */

void
Trans_Create
(
 Transform *tr
 )
{
  int		i;
  float*	p = &tr->data[0][0];

  for (i = 0; i < 16; ++i)
    *p++ = 0.0;
  for (i = 0; i < 4; ++i)
    tr->data[i][i] = 1.0;
}

void 
Trans_Copy
(
 Transform* dst_obj,
 Transform* src_obj
 )
{
  memcpy(dst_obj, src_obj, sizeof(Transform));
}

void 
Trans_Delete
(
 Transform* tr
 )
{
  if (tr) free(tr);
}

/* KfEngine method */

KfEngine*
KfEng_Create(void)
{
  KfEngine *kf;
	
  kf = (KfEngine *)malloc(sizeof(KfEngine));
  memset(kf, 0, sizeof(KfEngine));
  return kf;
}

void
KfEng_Delete
(
 KfEngine *kf
 )
{
  if (kf)
    {
      if (kf->mObjPos.mFrames)
	Array_Delete(kf->mObjPos.mFrames);
      if (kf->mObjPos.mPnts)
	Array_Delete(kf->mObjPos.mPnts);
      if (kf->mObjPos.mSplData)
	Array_Delete(kf->mObjPos.mSplData);
      if (kf->mObjRot.mFrames)
	Array_Delete(kf->mObjRot.mFrames);
      if (kf->mObjRot.mRots)
	Array_Delete(kf->mObjRot.mRots);
      if (kf->mObjRot.mSplData)
	Array_Delete(kf->mObjRot.mSplData);
      if (kf->mObjScl.mFrames)
	Array_Delete(kf->mObjScl.mFrames);
      if (kf->mObjScl.mScls)
	Array_Delete(kf->mObjScl.mScls);
      if (kf->mObjScl.mSplData)
	Array_Delete(kf->mObjScl.mSplData);
      /* spline data were deleted in ANIM_WriteChunk	*/
      free(kf);
    }
}

void
Glib_AddKfEng
(
 KfEngine *kf
 )
{
  gLib.KfEngine[gLib.currKfEngine] = kf;
  gLib.currKfEngine++;
}

int32
Glib_GetKfEngCount(void)
{
  return gLib.currKfEngine;
}

KfEngine *
Glib_GetKfEng
(
 int32 i
 )
{
  return gLib.KfEngine[i];
}

/* Array method */

Array*
Array_Create(void)
{
  Array *a;
	
  a = (Array *)malloc(sizeof(Array));
  memset(a, 0, sizeof(Array));
  return a;
}

void
Array_Delete
(
 Array *a
 )
{
  if (a)
    {
      Array_DeleteElement(a);
      free(a);
    }
}

/* ArrayElement method */

void
Array_CreateElement
(
 Array *a
 )
{
  a->m_Data = malloc(a->m_Size * a->m_ElemSize);
}

void Array_DeleteElement
(
 Array *a
 )
{
  if (a->m_Data)
    free(a->m_Data);
}
