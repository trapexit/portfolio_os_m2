/*
 *	@(#) geoitpr.c 6/10/96
 *	Copyright 1996, The 3DO Company
 */

		
/****
 *
 * Interpret a list of tokens into geometry library memory foramt
 *
 ****/
 

/* interface */
#include <stdlib.h>
#include <string.h>
#ifdef applec
#include "M2TXlib.h"
#include "geoitpr.h"
#else
#include "geoitpr.h"
#include "M2TXlib.h"
#endif
#include "sdftokens.h"
#include "texpage.h"

#define MAXSYM	       1500
#define MAXTEXCOORD    65535

typedef struct Symbol
{
  void	*obj;
  int32	*addr;
} Symbol;

static Symbol *sym = NULL;
int32 gSym = 0;
char symbolname[64];

void
Symbol_Init(void)
{
  sym = (Symbol *)malloc(MAXSYM * sizeof(Symbol));
  memset(sym, 0, MAXSYM * sizeof(Symbol));
  gSym = 0;
}

void
Symbol_Delete(void)
{
  if (sym)
    free(sym);
}

static void
AddSymbol
(
 void	*obj,
 int32	*addr
 )
{
  sym[gSym].obj = obj;
  sym[gSym].addr = addr;
  gSym++;
}

static void *
GetSymbol
(
 int32	*addr
 )
{
  int32 i;
	
  for (i = 0; i < gSym; i++)
    {
      if (sym[i].addr == addr)
	return sym[i].obj;
    }
  return NULL;
}

static int32 *buff = NULL;

void
InterpretSDFTokens
(
 char *tokens
 )
{
  int32 count, offset;

		
  buff = (int32 *)tokens;
  count = *(buff+2);
  buff += 3;
  while (count > 0)
    {
      switch (*buff)
	{
	case SYMBOL_NAME :
	  memcpy(symbolname, (buff+2), *(buff+1));
	  symbolname[*(buff+1)] = 0;
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	case NEXT_BUFFER :
	  buff = (int32 *) *(buff + 1);
	  count -= 8;
	  break;
	case CLASS_PAD :
	case SDF_UNITS :
	case USE_SYMBOL :
	  buff += 2;
	  count -= 8;
	  break;
	case CLASS_OBJECT :
	case ARRAY_SHORTARRAY :
	case ARRAY_FLOATARRAY :
	case ARRAY_COLOR :
	case ARRAY_COLORARRAY :
	case ARRAY_POINT :
	case ARRAY_POINTARRAY :
	case ARRAY_TEXCOORD :
	case ARRAY_BOX :
	case ARRAY_TRANSFORM :
	case ARRAY_VTX :
	case ARRAY_VTX_ARRAY :
	case ARRAY_TEXCOORDARAY :
	case CLASS_MATERIAL :
	case CLASS_SDF_TEXGEN :
	case CLASS_VTX_LIST :
	case CLASS_TRIMESH :
	case CLASS_QUADMESH :
	case CLASS_GEOMETRY :
	case CLASS_CUBE :
	case CLASS_BLOCK :
	case CLASS_SPHERE :
	case CLASS_ELLIPSOID :
	case CLASS_TORUS :
	case CLASS_CYLINDER :
	case CLASS_TRUNCATEDCONE :
	case CLASS_CHARACTER :
	case CLASS_ENGLINK :
	case CLASS_TEXTURE :
	case CLASS_PIPTABLE :
	case CLASS_SURFACE :
	case CLASS_TEXBLEND :
	case ARRAY_OBJARRAY :
	case ARRAY_LINKARRAY :
	case CLASS_LIGHT :
	case CLASS_CAMERA :
	case CLASS_SCENE :
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	case ARRAY_MATARRAY :
	  count = count - *(buff+1) - 8;
	  InterpretMatArray();
	  break;
	case CLASS_ENGINE :
	  count = count - *(buff+1) - 8;
	  InterpretEngine();
	  break;
	case ARRAY_TEXARRAY :
	  count = count - *(buff+1) - 8;
	  InterpretTexArray();
	  break;
	case CLASS_GROUP :
	  count = count - *(buff+1) - 8;
	  InterpretGroup(NULL);
	  break;
	case CLASS_MODEL :
	  count = count - *(buff+1) - 8;
	  InterpretModel();
	  break;
	case ARRAY_KFOBJARRAY :
	  count = count - *(buff+1) - 8;
	  InterpretKFObjArray();
	  break;
	default:
	  printf("error parsing token list\n");
	  printf("case(hex) = %x, case(dec) = %d, count = %d\n", *buff, *buff, count);
	  printf("offset = %d\n", *(buff+1));
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	}	
    }
}

void
InterpretGroup
(
 CharData *pd
 )
{
  int32 count = *(buff+1);
  int32 *tokens = buff;
  int32 offset;
  CharData *cd;
  ModelData *nm;
  void *obj;
  int32 *modelbuff;
	
  cd = Char_Create();
  if (pd == NULL)
    Glib_AddRootChar(cd);
  /* AddSymbol(cd, tokens+1); */
  AddSymbol(cd, tokens);
  Char_Attach(pd, cd, tokens);
  buff = tokens + 2;
  while (count > 0)
    {
      switch (*buff)
	{
	case SYMBOL_NAME :
	  memcpy(symbolname, (buff+2), *(buff+1));
	  symbolname[*(buff+1)] = 0;
	  strcpy(cd->name, symbolname);
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	case NEXT_BUFFER :
	  buff = (int32 *) *(buff + 1);
	  count -= 8;
	  break;
	case SDF_UNITS :
	case CLASS_PAD :
	  buff += 2;
	  count -= 8;
	  break;
	case USE_SYMBOL :
	  obj = GetSymbol((int32 *)*(buff+1));
	  if (obj == NULL)
	    fprintf(stderr,"WARNING:GfxObj for Use not found. Probably unsupported target (e.g. camera or light). Skipping.\n",
		    (char *)(buff+2));
	  else
	    switch (((GfxObj *)obj)->m_Type)
	      {
	      case Class_Character:
		break;
	      case Class_Model:
		nm = Model_Create();
		memcpy(nm, obj, sizeof(ModelData));
		nm->m_ModelIndex = Glib_GetModelIndex((ModelData *)obj);
		Char_Attach(cd, (CharData *)nm, tokens);
		break;
	      case Class_Mat_Array:
		break;
	      case Class_Tex_Array:
		break;
	      default:
		printf("UNKNOWN TYPE\n");
		printf("type = %d\n", ((GfxObj *)obj)->m_Type);
		break;
	      }
				
	  buff += 2;
	  count -= 8;
	  break;
	case CLASS_GROUP :
	  count = count - *(buff+1) - 8;
	  InterpretGroup(cd);
	  break;
	case CLASS_CHARACTER_CULLING :
	case CLASS_CHARACTER_VISIBLE :
	case CLASS_CHARACTER_LOOKAT :
	case CLASS_CHARACTER_MOVE :
	case CLASS_CHARACTER_TRANSLATE :
	case CLASS_CHARACTER_TURN :
	case CLASS_CHARACTER_ROTATE :
	case CLASS_CHARACTER_SCALE :
	case CLASS_CHARACTER_SIZE :
	case CLASS_CHARACTER_ROLL :
	case CLASS_CHARACTER_PITCH :
	case CLASS_CHARACTER_YAW :
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	case CLASS_CHARACTER_TRANSFORM :
	  memcpy(&cd->m_Transform.data, (buff+2), sizeof(MatrixData));
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	case CLASS_MODEL :
	  count = count - *(buff+1) - 8;
	  modelbuff = buff;
	  InterpretModel();
	  obj = GetSymbol(modelbuff);
	  nm = Model_Create();
	  memcpy(nm, obj, sizeof(ModelData));
	  nm->m_ModelIndex = Glib_GetModelIndex((ModelData *)obj);
	  Char_Attach(cd, (CharData *)nm, tokens);
	  break;
	default:
	  printf("1 other types\n");
	  printf("case = %x\n", *buff);
	  printf("size = %d\n", *(buff+1));
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	}
    }
}

void
InterpretModel(void)
{
  int32 count = *(buff+1);
  int32 offset;
  ModelData *tmodel;
  void *obj;
		
  tmodel = Model_Create();
  Glib_AddModel(tmodel);
  /* this is a hack before Spense fix the problem */
  /* AddSymbol(tmodel, buff+1); */
  AddSymbol(tmodel, buff);
  buff = buff + 2;
  while (count > 0)
    {
      switch (*buff)
	{
	case SYMBOL_NAME :
	  memcpy(symbolname, (buff+2), *(buff+1));
	  symbolname[*(buff+1)] = 0;
	  strcpy(((CharData *)tmodel)->name, symbolname);
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	case NEXT_BUFFER :
	  buff = (int32 *) *(buff + 1);
	  count -= 8;
	  break;
	case CLASS_PAD :
	case SDF_UNITS :
	  buff += 2;
	  count -= 8;
	  break;
	case USE_SYMBOL :
#if 0
	  obj = GetSymbol((int32 *)*(buff+1));
	  switch (((GfxObj *)obj)->m_Type)
	    {
	    case Class_Character:
	      printf("Class_Character\n");
	      break;
	    case Class_Model:
	      printf("Class_Model\n");
	      break;
	    case Class_Mat_Array:
	      printf("Class_Mat_Array\n");
	      break;
	    case Class_Tex_Array:
	      printf("Class_Tex_Array\n");
	      break;
	    default:
	      printf("UNKNOWN TYPE\n");
	      printf("type = %d\n", ((GfxObj *)obj)->m_Type);
	      break;
	    }
#endif
	  buff += 2;
	  count -= 8;
	  break;
	case CLASS_MODEL_SURFACE :
	  count = count - *(buff+1) - 8;
	  InterpretSurface(tmodel);
	  break;
	case CLASS_MODEL_MATERIALS :
	case CLASS_MODEL_TEXTURES :
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	case CLASS_CHARACTER_TRANSFORM :
	  memcpy(&((CharData *)tmodel)->m_Transform.data, (buff+2), sizeof(MatrixData));
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	default:
	  printf("2 other types\n");
	  printf("case = %x\n", *buff);
	  printf("size = %d\n", *(buff+1));
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	}
    }
}

void
InterpretSurface
(
 ModelData *tmodel
 )
{
  int32 count = *(buff+1);
  int32 offset;
  SurfaceData *tsurface;
  int32 autoNormal = 0;
  int32 matindex = -1, texindex = -1;
  TexGenProp *ctexgen = NULL;
	
  tsurface = Surf_Create();
  /* AddSymbol(tsurface, buff+1); */
  AddSymbol(tsurface, buff);
  tmodel->m_Surface = (GfxRef)tsurface;
  buff = buff + 2;
  while (count > 0)
    {
      switch (*buff)
	{
	case SYMBOL_NAME :
	  memcpy(symbolname, (buff+2), *(buff+1));
	  symbolname[*(buff+1)] = 0;
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	case NEXT_BUFFER :
	  buff = (int32 *) *(buff + 1);
	  count -= 8;
	  break;
	case CLASS_PAD :
	case SDF_UNITS :
	case USE_SYMBOL :
	  buff += 2;
	  count -= 8;
	  break;
	case CLASS_SURFACE_AUTONORMALS :
	  autoNormal = *(buff+1);
	  buff += 2;
	  count -= 8;
	  break;
	case CLASS_SURFACE_AUTOTEXCOORDS :
	case CLASS_SURFACE_NORMALIZEDTEXCOORDS :
	case CLASS_SURFACE_TEXGEN :
	  count = count - *(buff+1) - 8;
	  if (ctexgen)
	    free(ctexgen);
	  ctexgen = malloc(sizeof(TexGenProp));
	  InterpretTexgen(ctexgen);
	  break;
	case CLASS_SURFACE_MATERIAL :
	case CLASS_SURFACE_NORMAL :
	case CLASS_SURFACE_COLOR :
	case CLASS_SURFACE_CUBE :
	case CLASS_SURFACE_BLOCK :
	case CLASS_SURFACE_SPHERE :
	case CLASS_SURFACE_TORUS :
	case CLASS_SURFACE_ELLIPSOID :
	case CLASS_SURFACE_CYLINDER :
	case CLASS_SURFACE_CONE :
	case CLASS_SURFACE_TRUNCATEDCONE :
	case CLASS_SURFACE_POINTS :
	case CLASS_SURFACE_LINES :
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	case CLASS_SURFACE_MATINDEX :
	  matindex = *(buff+1);
	  buff += 2;
	  count -= 8;
	  break;
	case CLASS_SURFACE_TEXINDEX :
	  texindex = *(buff+1);
	  buff += 2;
	  count -= 8;
	  break;
	case CLASS_SURFACE_TRIANGLES :
	  count = count - *(buff+1) - 8;
	  InterpretTriangles(tsurface, autoNormal, matindex, texindex, ctexgen);
	  if (ctexgen)
	    {
	      free(ctexgen);
	      ctexgen = NULL;
	    }
	  break;
	case CLASS_SURFACE_TRISTRIP :
	  count = count - *(buff+1) - 8;
	  InterpretTristrip(tsurface, autoNormal, matindex, texindex, ctexgen);
	  if (ctexgen)
	    {
	      free(ctexgen);
	      ctexgen = NULL;
	    }
	  break;
	case CLASS_SURFACE_TRIFAN :
	  count = count - *(buff+1) - 8;
	  InterpretTrifan(tsurface, autoNormal, matindex, texindex, ctexgen);
	  if (ctexgen)
	    {
	      free(ctexgen);
	      ctexgen = NULL;
	    }
	  break;
	case CLASS_SURFACE_POLYGON :
	  count = count - *(buff+1) - 8;
	  InterpretTrifan(tsurface, autoNormal, matindex, texindex, ctexgen);
	  if (ctexgen)
	    {
	      free(ctexgen);
	      ctexgen = NULL;
	    }
	  break;
	case CLASS_SURFACE_QUADMESH :
	  count = count - *(buff+1) - 8;
	  InterpretQuadmesh(tsurface, autoNormal, matindex, texindex, ctexgen);
	  if (ctexgen)
	    {
	      free(ctexgen);
	      ctexgen = NULL;
	    }
	  break;
	case CLASS_SURFACE_TRIMESH :
	  count = count - *(buff+1) - 8;
	  InterpretTrimesh(tsurface, autoNormal, matindex, texindex, ctexgen);
	  if (ctexgen)
	    {
	      free(ctexgen);
	      ctexgen = NULL;
	    }
	  break;
	default:
	  printf("3 other types\n");
	  printf("case = %x\n", *buff);
	  printf("size = %d\n", *(buff+1));
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	}
    }
  if (ctexgen)
    free(ctexgen);
}

void InterpretTexgen
(
 TexGenProp *texgen
 )
{
  int32 count = *(buff+1);
  int32 offset, i;
	
  buff += 2;
  while (count > 0)
    {
      switch (*buff)
	{
	case SYMBOL_NAME :
	  memcpy(symbolname, (buff+2), *(buff+1));
	  symbolname[*(buff+1)] = 0;
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	case NEXT_BUFFER :
	  buff = (int32 *) *(buff + 1);
	  count -= 8;
	  break;
	case CLASS_PAD :
	case SDF_UNITS :
	case USE_SYMBOL :
	  buff += 2;
	  count -= 8;
	  break;
	case CLASS_SDF_TEXGEN_KIND :
	  texgen->Kind = *(buff+1);
	  buff += 2;
	  count -= 8;
	  break;
	case CLASS_SDF_TEXGEN_UREPEAT :
	case CLASS_SDF_TEXGEN_VREPEAT :
	  buff += 2;
	  count -= 8;
	  break;
	case CLASS_SDF_TEXGEN_TRANSFORM :
	case CLASS_SDF_TEXGEN_TEXSCALE :
	case CLASS_SDF_TEXGEN_TEXOFFSET :
	case CLASS_SDF_TEXGEN_TRANSLATE :
	case CLASS_SDF_TEXGEN_ROTATE :
	case CLASS_SDF_TEXGEN_SCALE :
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	default:
	  printf("4 other types\n");
	  printf("case = %x\n", *buff);
	  printf("size = %d\n", *(buff+1));
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	}
    }
}

static bool IsPower2
(
 int32 size
 )
{
  int32 tmp = 1;
  while (tmp < size)
    tmp = tmp << 1;
  if (tmp == size)
    return TRUE;
  else
    return FALSE;
}

static void ClampUV
(
 TexCoord *tc,
 int32 Size,
 int32 texindex
 )
{
  gTexBlend *txb;
  int32 i;
  float ninf, inc, tmp;

  txb = Glib_GetTexBlend(texindex);
  /* clamp */
  if (txb->xwrap == 0)
    {
      for (i = 0; i < Size; i++)
	if (tc[i].u < 0)
	  tc[i].u = 0;
	else if (tc[i].u > 1)
	  tc[i].u = 1;
    }
  /* tile */
  else
    {
      if ((txb->xsize > 0) && (!IsPower2(txb->xsize)))
	printf("ERROR: The texture xmin value is %d in texblend %d. It should be a power of two.\n", txb->xsize, texindex);
      ninf = MAXTEXCOORD;
      inc = 0.0;
      for (i = 0; i < Size; i++)
	if (tc[i].u < ninf)
	  ninf = tc[i].u;
      if (ninf < 0)
	{
	  while (ninf < 0)
	    {
	      inc += 1.0;
	      ninf += 1.0;
	    }
	  for (i = 0; i < Size; i++)
	    tc[i].u += inc;
	}
    }
  if (txb->ywrap == 0)
    {
      for (i = 0; i < Size; i++)
	if (tc[i].v < 0)
	  tc[i].v = 0;
	else if (tc[i].v > 1)
	  tc[i].v = 1;
    }
  /* tile */
  else
    {
      if ((txb->ysize > 0) && (!IsPower2(txb->ysize)))
	printf("ERROR: The texture ymin value is %d in texblend %d. It should be a power of two.\n", txb->ysize, texindex);
      ninf = MAXTEXCOORD;
      inc = 0.0;
      for (i = 0; i < Size; i++)
	if (tc[i].v < ninf)
	  ninf = tc[i].v;
      if (ninf < 0)
	{
	  while (ninf < 0.0)
	    {
	      inc += 1.0;
	      ninf += 1.0;
	    }
	  for (i = 0; i < Size; i++)
	    tc[i].v += inc;
	}
    }
  if (txb->xsize > 0) {
    /* checking texcoords */
    tmp = txb->xsize << (txb->lod - 1);
    for (i = 0; i < Size; i++)
      if (tc[i].u * tmp >= 1024)
	printf("ERROR: Texcoord x is out of range (1024). The error is near texindex %d and texcoord %d. Please check the texture size or remap the texcoord.\n", texindex, i);
  }
  if (txb->ysize > 0) {
    /* checking texcoords */
    tmp = txb->ysize << (txb->lod - 1);
    for (i = 0; i < Size; i++)
      if (tc[i].v * tmp >= 1024)
	printf("ERROR: Texcoord y is out of range (1024). The error is near texindex %d and texcoord %d. Please check the texture size or remap the texcoord.\n", texindex, i);
  }
}

void
InterpretTriangles
(
 SurfaceData *tsurface,
 int32 autoNormal,
 int32 matindex,
 int32 texindex,
 TexGenProp *texgen
 )
{
  int32 count = *(buff+1);
  int32 offset, i;
  GeoPrim *geo, *fgeo;
  Geometry *tlist;
  gTexBlend *txb;
	
  geo = GeoPrim_Create(GEO_TriList);	
  if (tsurface->m_FirstPrim == NULL)
    tsurface->m_FirstPrim = (char *)geo;
  else
    {
      fgeo = (GeoPrim *)tsurface->m_FirstPrim;
      while (fgeo->nextPrim != NULL)
	fgeo = (GeoPrim *)fgeo->nextPrim;
      fgeo->nextPrim = (PrimHeader *)geo;
    }
  geo->matIndex = matindex;
  geo->texIndex = texindex;
  geo->texgenkind = 0;
  if (texgen)
    geo->texgenkind = texgen->Kind;
  tlist = (Geometry *) &geo->geoData;
  tlist->Type = GEO_TriList;
	
  buff = buff + 2;
  while (count > 0)
    {
      switch (*buff)
	{
	case SYMBOL_NAME :
	  memcpy(symbolname, (buff+2), *(buff+1));
	  symbolname[*(buff+1)] = 0;
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	case NEXT_BUFFER :
	  buff = (int32 *) *(buff + 1);
	  count -= 8;
	  break;
	case CLASS_PAD :
	case SDF_UNITS :
	case USE_SYMBOL :
	  buff += 2;
	  count -= 8;
	  break;
	case CLASS_GEOMETRY_VERTEXLIST :
	  count = count - *(buff+1) - 8;
	  InterpretVertexlist((TriMesh *)tlist);
	  break;
	default:
	  printf("4 other types\n");
	  printf("case = %x\n", *buff);
	  printf("size = %d\n", *(buff+1));
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	}
    }
	
  if (!(tlist->Style & GEO_Normals) && (autoNormal))
    {
      tlist->Style |= GEO_Normals;
      tlist->Normals = malloc(tlist->Size * sizeof(Vector3));
      memset(tlist->Normals, 0, tlist->Size * sizeof(Vector3));
      tlist_generate_normals(tlist);
    }
  if (1)
    {
      /*
	vlist = malloc(tlist->Size * (sizeof(Point3) + sizeof(Vector3)));
	for (i = 0; i < tlist->Size-1; i++)
	{
	memcpy(vlist + 2 * i, tlist->Locations + i, sizeof(Point3));
	memcpy(vlist + 2 * i + 1, tlist->Normals + i, sizeof(Vector3));
	}
	for (i = 0; i < tlist->Size-1; i++)
	{
	for (j = 1; j < tlist->Size; j++)
	{
	}
	}
	*/
    }
  if ((tlist->Style & GEO_TexCoords) && (texindex >= 0))
    ClampUV(tlist->TexCoords, tlist->Size, texindex);
}

void
InterpretTristrip
(
 SurfaceData *tsurface,
 int32 autoNormal,
 int32 matindex,
 int32 texindex,
 TexGenProp *texgen
 )
{
  int32 count = *(buff+1);
  int32 offset, i;
  GeoPrim *geo, *fgeo;
  Geometry *tstrip;
  gTexBlend *txb;
	
  geo = GeoPrim_Create(GEO_TriStrip);	
  if (tsurface->m_FirstPrim == NULL)
    tsurface->m_FirstPrim = (char *)geo;
  else
    {
      fgeo = (GeoPrim *)tsurface->m_FirstPrim;
      while (fgeo->nextPrim != NULL)
	fgeo = (GeoPrim *)fgeo->nextPrim;
      fgeo->nextPrim = (PrimHeader *)geo;
    }
  geo->matIndex = matindex;
  geo->texIndex = texindex;
  geo->texgenkind = 0;
  if (texgen)
    geo->texgenkind = texgen->Kind;
  tstrip = (Geometry *) &geo->geoData;
  tstrip->Type = GEO_TriStrip;
	
  buff = buff + 2;
  while (count > 0)
    {
      switch (*buff)
	{
	case SYMBOL_NAME :
	  memcpy(symbolname, (buff+2), *(buff+1));
	  symbolname[*(buff+1)] = 0;
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	case NEXT_BUFFER :
	  buff = (int32 *) *(buff + 1);
	  count -= 8;
	  break;
	case CLASS_PAD :
	case SDF_UNITS :
	case USE_SYMBOL :
	  buff += 2;
	  count -= 8;
	  break;
	case CLASS_GEOMETRY_VERTEXLIST :
	  count = count - *(buff+1) - 8;
	  InterpretVertexlist((TriMesh *)tstrip);
	  break;
	default:
	  printf("5 other types\n");
	  printf("case = %x\n", *buff);
	  printf("size = %d\n", *(buff+1));
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	}
    }
	
  if (!(tstrip->Style & GEO_Normals) && (autoNormal))
    {
      tstrip->Style |= GEO_Normals;
      tstrip->Normals = malloc(tstrip->Size * sizeof(Vector3));
      memset(tstrip->Normals, 0, tstrip->Size * sizeof(Vector3));
      tstrip_generate_normals(tstrip);
    }
  if ((tstrip->Style & GEO_TexCoords) && (texindex >= 0))
    ClampUV(tstrip->TexCoords, tstrip->Size, texindex);
}

void
InterpretTrifan
(
 SurfaceData *tsurface,
 int32 autoNormal,
 int32 matindex,
 int32 texindex,
 TexGenProp *texgen
 )
{
  int32 count = *(buff+1);
  int32 offset, i;
  GeoPrim *geo, *fgeo;
  Geometry *tfan;
  gTexBlend *txb;
	
  geo = GeoPrim_Create(GEO_TriFan);	
  if (tsurface->m_FirstPrim == NULL)
    tsurface->m_FirstPrim = (char *)geo;
  else
    {
      fgeo = (GeoPrim *)tsurface->m_FirstPrim;
      while (fgeo->nextPrim != NULL)
	fgeo = (GeoPrim *)fgeo->nextPrim;
      fgeo->nextPrim = (PrimHeader *)geo;
    }
  geo->matIndex = matindex;
  geo->texIndex = texindex;
  geo->texgenkind = 0;
  if (texgen)
    geo->texgenkind = texgen->Kind;
  tfan = (Geometry *) &geo->geoData;
  tfan->Type = GEO_TriFan;
	
  buff = buff + 2;
  while (count > 0)
    {
      switch (*buff)
	{
	case SYMBOL_NAME :
	  memcpy(symbolname, (buff+2), *(buff+1));
	  symbolname[*(buff+1)] = 0;
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	case NEXT_BUFFER :
	  buff = (int32 *) *(buff + 1);
	  count -= 8;
	  break;
	case CLASS_PAD :
	case SDF_UNITS :
	case USE_SYMBOL :
	  buff += 2;
	  count -= 8;
	  break;
	case CLASS_GEOMETRY_VERTEXLIST :
	  count = count - *(buff+1) - 8;
	  InterpretVertexlist((TriMesh *)tfan);
	  break;
	default:
	  printf("6 other types\n");
	  printf("case = %x\n", *buff);
	  printf("size = %d\n", *(buff+1));
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	}
    }
	
  if (!(tfan->Style & GEO_Normals) && (autoNormal))
    {
      tfan->Style |= GEO_Normals;
      tfan->Normals = malloc(tfan->Size * sizeof(Vector3));
      memset(tfan->Normals, 0, tfan->Size * sizeof(Vector3));
      tfan_generate_normals(tfan);
    }
  if ((tfan->Style & GEO_TexCoords) && (texindex >= 0))
    ClampUV(tfan->TexCoords, tfan->Size, texindex);
}

void
InterpretQuadmesh
(
 SurfaceData *tsurface,
 int32 autoNormal,
 int32 matindex,
 int32 texindex,
 TexGenProp *texgen
 )
{
  int32 count = *(buff+1);
  int32 offset, i;
  GeoPrim *geo, *fgeo;
  QuadMesh *qmesh;
  gTexBlend *txb;
	
  geo = GeoPrim_Create(GEO_QuadMesh);	
  if (tsurface->m_FirstPrim == NULL)
    tsurface->m_FirstPrim = (char *)geo;
  else
    {
      fgeo = (GeoPrim *)tsurface->m_FirstPrim;
      while (fgeo->nextPrim != NULL)
	fgeo = (GeoPrim *)fgeo->nextPrim;
      fgeo->nextPrim = (PrimHeader *)geo;
    }
  geo->matIndex = matindex;
  geo->texIndex = texindex;
  geo->texgenkind = 0;
  if (texgen)
    geo->texgenkind = texgen->Kind;
  qmesh = (QuadMesh *) &geo->geoData;
  qmesh->Type = GEO_QuadMesh;
	
  buff = buff + 2;
  while (count > 0)
    {
      switch (*buff)
	{
	case SYMBOL_NAME :
	  memcpy(symbolname, (buff+2), *(buff+1));
	  symbolname[*(buff+1)] = 0;
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	case NEXT_BUFFER :
	  buff = (int32 *) *(buff + 1);
	  count -= 8;
	  break;
	case CLASS_PAD :
	case SDF_UNITS :
	case USE_SYMBOL :
	  buff += 2;
	  count -= 8;
	  break;
	case CLASS_QUADMESH_VERTEXLIST :
	  count = count - *(buff+1) - 8;
	  InterpretVertexlist((TriMesh *)qmesh);
	  break;
	case CLASS_QUADMESH_VERTICESPERROW :
	  qmesh->XSize = *(buff+1);
	  buff += 2;
	  count -= 8;
	  break;
	case CLASS_QUADMESH_VERTICESPERCOLUMN :
	  qmesh->YSize = *(buff+1);
	  buff += 2;
	  count -= 8;
	  break;
	default:
	  printf("7 other types\n");
	  printf("case = %x\n", *buff);
	  printf("size = %d\n", *(buff+1));
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	}
    }
	
  if (!(qmesh->Style & GEO_Normals) && (autoNormal))
    {
      qmesh->Style |= GEO_Normals;
      qmesh->Normals = malloc(qmesh->Size * sizeof(Vector3));
      memset(qmesh->Normals, 0, qmesh->Size * sizeof(Vector3));
      qmesh_generate_normals(qmesh);
    }
  if ((qmesh->Style & GEO_TexCoords) && (texindex >= 0))
    ClampUV(qmesh->TexCoords, qmesh->Size, texindex);
}

void
InterpretTrimesh
(
 SurfaceData *tsurface,
 int32 autoNormal,
 int32 matindex,
 int32 texindex,
 TexGenProp *texgen
 )
{
  int32 count = *(buff+1);
  int32 offset, i;
  GeoPrim *geo, *fgeo;
  TriMesh *tm;
  gTexBlend *txb;
	
  geo = GeoPrim_Create(GEO_GLTriMesh);	
  if (tsurface->m_FirstPrim == NULL)
    tsurface->m_FirstPrim = (char *)geo;
  else
    {
      fgeo = (GeoPrim *)tsurface->m_FirstPrim;
      while (fgeo->nextPrim != NULL)
	fgeo = (GeoPrim *)fgeo->nextPrim;
      fgeo->nextPrim = (PrimHeader *)geo;
    }
  geo->matIndex = matindex;
  geo->texIndex = texindex;
  geo->texgenkind = 0;
  if (texgen)
    geo->texgenkind = texgen->Kind;
  tm = (TriMesh *) &geo->geoData;
  tm->Type = GEO_GLTriMesh;
	
  buff = buff + 2;
  while (count > 0)
    {
      switch (*buff)
	{
	case SYMBOL_NAME :
	  memcpy(symbolname, (buff+2), *(buff+1));
	  symbolname[*(buff+1)] = 0;
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	case NEXT_BUFFER :
	  buff = (int32 *) *(buff + 1);
	  count -= 8;
	  break;
	case CLASS_PAD :
	case SDF_UNITS :
	case USE_SYMBOL :
	  buff += 2;
	  count -= 8;
	  break;
	case CLASS_TRIMESH_VERTEXLIST :
	  count = count - *(buff+1) - 8;
	  InterpretVertexlist(tm);
	  break;
	case CLASS_TRIMESH_VERTEXCOUNT :
	  tm->LengthSize = *(buff+1) / 2;
	  tm->LengthData = malloc(tm->LengthSize * 2);
	  memcpy(tm->LengthData, (buff+2), tm->LengthSize * 2);
				
	  count = count - ((*(buff+1) + 3) & 0xfffc) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	case CLASS_TRIMESH_VERTEXINDICES :
	  tm->IndexSize = *(buff+1) / 2;
	  tm->IndexData = malloc(tm->IndexSize * 2);
	  memcpy(tm->IndexData, (buff+2), tm->IndexSize * 2);
				
	  count = count - ((*(buff+1) + 3) & 0xfffc) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	default:
	  printf("8 other types\n");
	  printf("case = %x\n", *buff);
	  printf("size = %d\n", *(buff+1));
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	}
    }
  if (!(tm->Style & GEO_Normals) && (autoNormal))
    {
      tm->Style |= GEO_Normals;
      tm->Normals = malloc(tm->Size * sizeof(Vector3));
      memset(tm->Normals, 0, tm->Size * sizeof(Vector3));
      tmesh_generate_normals(tm);
    }
  if ((tm->Style & GEO_TexCoords) && (texindex >= 0))
    ClampUV(tm->TexCoords, tm->Size, texindex);
}

void
InterpretVertexlist
(
 TriMesh *tm
 )
{
  int32 count = *(buff+1);
  int32 offset;
  int32 vsize, i;
  char *ptr;
  uint32 style;
  bool nbflag = FALSE;
	
  buff = buff + 2;
  while (count > 0)
    {
      switch (*buff)
	{
	case SYMBOL_NAME :
	  memcpy(symbolname, (buff+2), *(buff+1));
	  symbolname[*(buff+1)] = 0;
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	case NEXT_BUFFER :
	  buff = (int32 *) *(buff + 1);
	  count -= 8;
	  break;
	case CLASS_PAD :
	case SDF_UNITS :
	case USE_SYMBOL :
	  buff += 2;
	  count -= 8;
	  break;
	case CLASS_VTX_LIST_FORMAT :
	  style = *(buff+1);
	  tm->Style = GEO_Locations;
	  if (style & ENUM_VTX_FORMAT_NORMALS)
	    tm->Style |= GEO_Normals;
	  if (style & ENUM_VTX_FORMAT_COLORS)
	    tm->Style |= GEO_Colors;
	  if (style & ENUM_VTX_FORMAT_TEXCOORDS)
	    tm->Style |= GEO_TexCoords;
	  buff += 2;
	  count -= 8;
	  break;
	case CLASS_VTX_LIST_VERTICES :
	  count = count - *(buff+1) - 8;
	  vsize = 4;	/* include the first 4 bytes, sizeof the array */
	  if (tm->Style & GEO_Normals)
	    vsize += 3;
	  if (tm->Style & GEO_Colors)
	    vsize += 4;
	  if (tm->Style & GEO_TexCoords)
	    vsize += 2;
	  tm->Size = *(buff+1) / (4 * vsize);
	  tm->Locations = malloc(tm->Size * sizeof(Point3));
	  if (tm->Style & GEO_Normals)
	    tm->Normals = malloc(tm->Size * sizeof(Vector3));
	  if (tm->Style & GEO_Colors)
	    tm->Colors = malloc(tm->Size * sizeof(Color4));
	  if (tm->Style & GEO_TexCoords)
	    tm->TexCoords = malloc(tm->Size * sizeof(Color4));
	  buff += 2;
	  for (i = 0; i < tm->Size; i++)
	    {
	      if (*buff == NEXT_BUFFER)
		buff = (int32 *) *(buff + 1);
	      buff++;
	      memcpy(tm->Locations + i, buff, sizeof(Point3));
	      buff += 3;
	      if (tm->Style & GEO_Normals)
		{
		  memcpy(tm->Normals + i, buff, sizeof(Vector3));
		  buff += 3;
		}
	      if (tm->Style & GEO_Colors)
		{
		  memcpy(tm->Colors + i, buff, sizeof(Color4));
		  buff += 4;
		}
	      if (tm->Style & GEO_TexCoords)
		{
		  memcpy(tm->TexCoords + i, buff, sizeof(TexCoord));
		  buff += 2;
		}
	    }
				
	  break;
	case CLASS_VTX_LIST_LOCATIONS :
	  count = count - *(buff+1) - 8;
	  tm->Size = *(buff+1) / (4 * 4);	/* vertex size comes from location  */
	  tm->Locations = malloc(tm->Size * sizeof(Point3));
	  buff += 2;
	  for (i = 0; i < tm->Size; i++)
	    {
	      if (*buff == NEXT_BUFFER)
		buff = (int32 *) *(buff + 1);
	      buff++;
	      memcpy(tm->Locations + i, buff, sizeof(Point3));
	      buff += 3;
	    }

	  break;
	case CLASS_VTX_LIST_NORMALS :
	  count = count - *(buff+1) - 8;
	  tm->Normals = malloc(tm->Size * sizeof(Vector3));
	  buff += 2;
	  for (i = 0; i < tm->Size; i++)
	    {
	      if (*buff == NEXT_BUFFER)
		buff = (int32 *) *(buff + 1);
	      buff++;
	      memcpy(tm->Normals + i, buff, sizeof(Vector3));
	      buff += 3;
	    }

	  break;
	case CLASS_VTX_LIST_COLORS :
	  count = count - *(buff+1) - 8;
	  tm->Colors = malloc(tm->Size * sizeof(Color4));
	  buff += 2;
	  for (i = 0; i < tm->Size; i++)
	    {
	      if (*buff == NEXT_BUFFER)
		buff = (int32 *) *(buff + 1);
	      buff++;
	      memcpy(tm->Colors + i, buff, sizeof(Color4));
	      buff += 4;
	    }

	  break;
	case CLASS_VTX_LIST_TEXCOORDS :
	  count = count - *(buff+1) - 8;
	  tm->TexCoords = malloc(tm->Size * sizeof(TexCoord));
	  buff += 2;
	  for (i = 0; i < tm->Size; i++)
	    {
	      if (*buff == NEXT_BUFFER)
		buff = (int32 *) *(buff + 1);
	      buff++;
	      memcpy(tm->TexCoords + i, buff, sizeof(TexCoord));
	      buff += 2;
	    }

	  break;
	default:
	  printf("9 other types\n");
	  printf("case = %x\n", *buff);
	  printf("size = %d\n", *(buff+1));
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	}
    }
}

void
InterpretEngine(void)
{
  int32 count = *(buff+1);
  int32 offset;
	
  while (count > 0)
    {
      switch (*buff)
	{
	case SYMBOL_NAME :
	  memcpy(symbolname, (buff+2), *(buff+1));
	  symbolname[*(buff+1)] = 0;
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	case NEXT_BUFFER :
	  buff = (int32 *) *(buff + 1);
	  count -= 8;
	  break;
	case CLASS_PAD :
	case SDF_UNITS :
	case USE_SYMBOL :
	  buff += 2;
	  count -= 8;
	  break;
	default:
	  printf("10 other types\n");
	  printf("case = %x\n", *buff);
	  printf("size = %d\n", *(buff+1));
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	}
    }
}

void
InterpretMatArray(void)
{
  int32 count = *(buff+1);
  int32 offset, next_count;
  MatProp *mat;
	
  buff = buff + 2;

  /* symbol name */
  memcpy(symbolname, (buff+2), *(buff+1));
  symbolname[*(buff+1)] = 0;
  count = count - *(buff+1) - 8;
  offset = (*(buff+1) + 3) >> 2;
  buff = buff + offset + 2;
  if (*Glib_GetMatArrayName() == 0)
    Glib_SetMatArrayName(symbolname);
  else {
    buff += count / 4;
    return;
  }

  while (count > 0)
    {
      next_count = *buff;
      count = count - next_count - 4;
      mat = Mat_Create();
      InterpretMaterial(mat);
      Glib_AddMat(mat);
    }
}

void
InterpretTexArray(void)
{
  int32 count = *(buff+1);
  int32 offset, next_count;
  gTexBlend *txb;
	
  buff = buff + 2;

  /* symbol name */
  memcpy(symbolname, (buff+2), *(buff+1));
  symbolname[*(buff+1)] = 0;
  Glib_SetTexArrayName(symbolname);
  count = count - *(buff+1) - 8;
  offset = (*(buff+1) + 3) >> 2;
  buff = buff + offset + 2;

  while (count > 0)
    {
      next_count = *buff;
      count = count - next_count - 4;
      if (!Glib_GetTexBlendIndex())
	NewTexTable();
      txb = Txb_Create();
      InterpretTexture(txb);
      Glib_AddTexBlend(txb);
    }
}

void
InterpretMaterial
(
 MatProp *mat
 )
{
  int32 count = *buff;
  int32 offset;
  uint32 enable;
	
  buff = buff + 1;
  while (count > 0)
    {
      switch (*buff)
	{
	case SYMBOL_NAME :
	  memcpy(symbolname, (buff+2), *(buff+1));
	  symbolname[*(buff+1)] = 0;
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	case NEXT_BUFFER :
	  buff = (int32 *) *(buff + 1);
	  count -= 8;
	  break;
	case CLASS_PAD :
	case SDF_UNITS :
	case USE_SYMBOL :
	  buff += 2;
	  count -= 8;
	  break;
	case CLASS_MATERIAL_AMBIENT :
	  memcpy(&mat->Ambient, (buff+2), sizeof(Color4));
	  mat->ShadeEnable = mat->ShadeEnable | MAT_Ambient;
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	case CLASS_MATERIAL_DIFFUSE :
	  memcpy(&mat->Diffuse, (buff+2), sizeof(Color4));
	  mat->ShadeEnable = mat->ShadeEnable | MAT_Diffuse;
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	case CLASS_MATERIAL_EMISSION :
	  memcpy(&mat->Emission, (buff+2), sizeof(Color4));
	  mat->ShadeEnable = mat->ShadeEnable | MAT_Emissive;
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	case CLASS_MATERIAL_SPECULAR :
	  memcpy(&mat->Specular, (buff+2), sizeof(Color4));
	  mat->ShadeEnable = mat->ShadeEnable | MAT_Specular;
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	case CLASS_MATERIAL_SHADEENABLE :
	  enable = *(buff+1);
	  /*
	    Col_SetRGB(&mat->Diffuse, 0.7, 0.7, 0.7);
	    Col_SetRGB(&mat->Specular, 1, 1, 1);
	    Col_SetRGB(&mat->Emission, 0, 0, 0);
	    Col_SetRGB(&mat->Ambient, 0.3, 0.3, 0.3);
	    mat->Shine = 0.05;
	    */
	  if (enable & ENUM_SDF_SHADE_AMBIENT)
	    {
	      mat->ShadeEnable = mat->ShadeEnable | MAT_Ambient;
	    }
	  if (enable & ENUM_SDF_SHADE_DIFFUSE)
	    {
	      mat->ShadeEnable = mat->ShadeEnable | MAT_Diffuse;
	    }
	  if (enable & ENUM_SDF_SHADE_SPECULAR)
	    {
	      mat->ShadeEnable = mat->ShadeEnable | MAT_Specular;
	    }
	  if (enable & ENUM_SDF_SHADE_EMISSIVE)
	    {
	      mat->ShadeEnable = mat->ShadeEnable | MAT_Emissive;
	    }
	  if (enable & ENUM_SDF_SHADE_TWOSIDED)
	    {
	      mat->ShadeEnable = mat->ShadeEnable | MAT_TwoSided;
	    }
	  buff += 2;
	  count -= 8;
	  break;
	case CLASS_MATERIAL_SHINE :
	  mat->Shine = *((float *)buff+1);
	  buff += 2;
	  count -= 8;
	  break;
	default:
	  printf("11 other types\n");
	  printf("case = %x\n", *buff);
	  printf("size = %d\n", *(buff+1));
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	}
    }
}

void
InterpretTexture
(
 gTexBlend *txb
 )
{
  int32 count = *buff;
  int32 offset;
  char  name[64];
  M2TX tex;
  M2TXHeader *header;
	
  buff = buff + 1;
  while (count > 0)
    {
      switch (*buff)
	{
	case SYMBOL_NAME :
	  memcpy(symbolname, (buff+2), *(buff+1));
	  symbolname[*(buff+1)] = 0;
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	case NEXT_BUFFER :
	  buff = (int32 *) *(buff + 1);
	  count -= 8;
	  break;
	case CLASS_PAD :
	case SDF_UNITS :
	case USE_SYMBOL :
	  buff += 2;
	  count -= 8;
	  break;
	case CLASS_TEXBLEND_FILENAME :
	  memset(name, 0, 64);
	  memcpy(name, (buff+2), *(buff+1));
	  SetTexName(Glib_GetTexBlendIndex(), name);
	  if (M2TX_ReadFile(name, &tex) != M2E_NoErr)
	    printf("WARNING: Can not open %s. Turn off texcoords checking.\n", name);
	  else {
	    M2TX_GetHeader(&tex, &header);
	    M2TXHeader_GetMinXSize(header, &txb->xsize);
	    M2TXHeader_GetMinYSize(header, &txb->ysize);
	    M2TXHeader_GetNumLOD(header, &txb->lod);
	  }

	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	case CLASS_TEXBLEND_TEXTURE :
	case CLASS_TEXBLEND_PIP :
	case CLASS_TEXBLEND_FIRSTLOD :
	case CLASS_TEXBLEND_NUMLOD :
	case CLASS_TEXBLEND_TXMINFILTER :
	case CLASS_TEXBLEND_TXMAGFILTER :
	case CLASS_TEXBLEND_TXINTERFILTER :
	case CLASS_TEXBLEND_TXPIPINDEXOFFSET :
	case CLASS_TEXBLEND_TXPIPCOLORSEL :
	case CLASS_TEXBLEND_TXPIPALPHASEL :
	case CLASS_TEXBLEND_TXPIPSSBSEL :
	case CLASS_TEXBLEND_TXPIPCONSTSSB0 :
	case CLASS_TEXBLEND_TXPIPCONSTSSB1 :
	case CLASS_TEXBLEND_TXFIRSTCOLOR :
	case CLASS_TEXBLEND_TXSECONDCOLOR :
	case CLASS_TEXBLEND_TXTHIRDCOLOR :
	case CLASS_TEXBLEND_TXFIRSTALPHA :
	case CLASS_TEXBLEND_TXSECONDALPHA :
	case CLASS_TEXBLEND_TXCOLOROUT :
	case CLASS_TEXBLEND_TXALPHAOUT :
	case CLASS_TEXBLEND_TXBLENDOP :
	case CLASS_TEXBLEND_TXBLENDCOLORSSB0 :
	case CLASS_TEXBLEND_TXBLENDCOLORSSB1 :
	case CLASS_TEXBLEND_DBLENABLEATTRS :
	case CLASS_TEXBLEND_DBLDISCARD :
	case CLASS_TEXBLEND_DBLXWINCLIPMIN :
	case CLASS_TEXBLEND_DBLXWINCLIPMAX :
	case CLASS_TEXBLEND_DBLYWINCLIPMIN :
	case CLASS_TEXBLEND_DBLYWINCLIPMAX :
	case CLASS_TEXBLEND_DBLZCOMPARECONTROL :
	case CLASS_TEXBLEND_DBLZXOFFSET :
	case CLASS_TEXBLEND_DBLZYOFFSET :
	case CLASS_TEXBLEND_DBLDSBSELECT :
	case CLASS_TEXBLEND_DBLDSBCONST :
	case CLASS_TEXBLEND_DBLAINPUTSELECT :
	case CLASS_TEXBLEND_DBLAMULTCOEFSELECT :
	case CLASS_TEXBLEND_DBLAMULTCONSTCONTROL :
	case CLASS_TEXBLEND_DBLAMULTRTJUSTIFY :
	case CLASS_TEXBLEND_DBLBINPUTSELECT :
	case CLASS_TEXBLEND_DBLBMULTCOEFSELECT :
	case CLASS_TEXBLEND_DBLBMULTCONSTCONTROL :
	case CLASS_TEXBLEND_DBLBMULTRTJUSTIFY :
	case CLASS_TEXBLEND_DBLALUOPERATION :
	case CLASS_TEXBLEND_DBLFINALDIVIDE :
	case CLASS_TEXBLEND_DBLDITHERMATRIXA :
	case CLASS_TEXBLEND_DBLDITHERMATRIXB :
	case CLASS_TEXBLEND_DBLSRCPIXELS32BIT :
	case CLASS_TEXBLEND_DBLSRCBASEADDR :
	case CLASS_TEXBLEND_DBLSRCXSTRIDE :
	case CLASS_TEXBLEND_DBLSRCXOFFSET :
	case CLASS_TEXBLEND_DBLSRCYOFFSET :
	case CLASS_TEXBLEND_DBLRGBCONSTIN :
	case CLASS_TEXBLEND_DBLBMULTCONSTSSB0 :
	case CLASS_TEXBLEND_DBLBMULTCONSTSSB1 :
	case CLASS_TEXBLEND_DBLAMULTCONSTSSB0 :
	case CLASS_TEXBLEND_DBLAMULTCONSTSSB1 :
	case CLASS_TEXBLEND_DBLALPHA0CLAMPCONTROL :
	case CLASS_TEXBLEND_DBLALPHA1CLAMPCONTROL :
	case CLASS_TEXBLEND_DBLALPHAFRACCLAMPCONTROL :
	case CLASS_TEXBLEND_DBLDESTALPHASELECT :
	case CLASS_TEXBLEND_DBLDESTALPHACONSTSSB0 :
	case CLASS_TEXBLEND_DBLDESTALPHACONSTSSB1 :
	  buff += 2;
	  count -= 8;
	  break;
	case CLASS_TEXBLEND_XWRAP :
	  txb->xwrap = *(buff+1);
	  buff += 2;
	  count -= 8;
	  break;
	case CLASS_TEXBLEND_YWRAP :
	  txb->ywrap = *(buff+1);
	  buff += 2;
	  count -= 8;
	  break;
	case CLASS_TEXBLEND_PAGEINDEX :
	  txb->pageindex = *(buff+1);
	  buff += 2;
	  count -= 8;
	  break;
	case CLASS_TEXBLEND_SUBINDEX :
	  txb->subindex = *(buff+1);
	  buff += 2;
	  count -= 8;
	  break;
	default:
	  printf("12 other types\n");
	  printf("count = %d\n", count);
	  printf("case = %x\n", *buff);
	  printf("size = %d\n", *(buff+1));
	  if (*buff == 0)
	    exit(0);
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	}
    }
}

void
InterpretKFObjArray(void)
{
  int32 count = *(buff+1);
  int32 offset, next_count;
  KfEngine *kfobj;
	
  buff = buff + 2;
  /* symbol name */
	
  memcpy(symbolname, (buff+2), *(buff+1));
  symbolname[*(buff+1)] = 0;
  count = count - *(buff+1) - 8;
  offset = (*(buff+1) + 3) >> 2;
  buff = buff + offset + 2;

  while (count > 0)
    {
      next_count = *buff;
      count = count - next_count - 4;
      kfobj = KfEng_Create();
      InterpretKFObject(kfobj);
      Glib_AddKfEng(kfobj);
    }
}

void
InterpretKFObject
(
 KfEngine *kf
 )
{
  int32 count = *buff;
  int32 offset;
  uint32 enable;
  void *obj;
	
  buff = buff + 1;	

  while (count > 0)
    {
      switch (*buff)
	{
	case SYMBOL_NAME :
	  memcpy(symbolname, (buff+2), *(buff+1));
	  symbolname[*(buff+1)] = 0;
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	case NEXT_BUFFER :
	  buff = (int32 *) *(buff + 1);
	  count -= 8;
	  break;
	case CLASS_PAD :
	case SDF_UNITS :
	case USE_SYMBOL :
	  buff += 2;
	  count -= 8;
	  break;
	case CLASS_KF_OBJECT_TARGET :
	  obj = GetSymbol((int32 *)*(buff+2));
	  switch (((GfxObj *)obj)->m_Type)
	    {
	    case Class_Character:
	      kf->mObj = (CharData *)obj;
	      break;
	    case Class_Model:
	      kf->mObj = (CharData *)obj;
	      break;
	    default:
	      printf("UNKNOWN TYPE\n");
	      printf("type = %d\n", ((GfxObj *)obj)->m_Type);
	      break;
	    }
	  buff += 3;
	  count -= 12;
	  break;
	case CLASS_KF_OBJECT_OBJPIVOT :
	  memcpy(&kf->mObjPivot, (buff+2), sizeof(Point3));
				
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	case CLASS_KF_OBJECT_PRNTPIVOT :
	  memcpy(&kf->mPobjPivot, (buff+2), sizeof(Point3));

	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	case CLASS_KF_OBJECT_POSFRAMES :
	  kf->mObjPos.mFrames = Array_Create();
	  kf->mObjPos.mFrames->m_ElemSize = sizeof(float);
	  kf->mObjPos.mFrames->m_Size = *(buff+1) / sizeof(float);
	  kf->mObjPos.mFrames->m_MaxSize = *(buff+1) / sizeof(float);
	  Array_CreateElement(kf->mObjPos.mFrames);
	  memcpy(kf->mObjPos.mFrames->m_Data, (buff+2), *(buff+1));
				
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	case CLASS_KF_OBJECT_POSDATA :
	  kf->mObjPos.mPnts = Array_Create();
	  kf->mObjPos.mPnts->m_ElemSize = sizeof(float);
	  kf->mObjPos.mPnts->m_Size = *(buff+1) / sizeof(float);
	  kf->mObjPos.mPnts->m_MaxSize = *(buff+1) / sizeof(float);
	  Array_CreateElement(kf->mObjPos.mPnts);
	  memcpy(kf->mObjPos.mPnts->m_Data, (buff+2), *(buff+1));
				
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	case CLASS_KF_OBJECT_POSSPLDATA :
	  kf->mObjPos.mSplData = Array_Create();
	  kf->mObjPos.mSplData->m_ElemSize = sizeof(float);
	  kf->mObjPos.mSplData->m_Size = *(buff+1) / sizeof(float);
	  kf->mObjPos.mSplData->m_MaxSize = *(buff+1) / sizeof(float);
	  Array_CreateElement(kf->mObjPos.mSplData);
	  memcpy(kf->mObjPos.mSplData->m_Data, (buff+2), *(buff+1));
				
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	case CLASS_KF_OBJECT_ROTFRAMES :
	  kf->mObjRot.mFrames = Array_Create();
	  kf->mObjRot.mFrames->m_ElemSize = sizeof(float);
	  kf->mObjRot.mFrames->m_Size = *(buff+1) / sizeof(float);
	  kf->mObjRot.mFrames->m_MaxSize = *(buff+1) / sizeof(float);
	  Array_CreateElement(kf->mObjRot.mFrames);
	  memcpy(kf->mObjRot.mFrames->m_Data, (buff+2), *(buff+1));
				
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	case CLASS_KF_OBJECT_ROTDATA :
	  kf->mObjRot.mRots = Array_Create();
	  kf->mObjRot.mRots->m_ElemSize = sizeof(float);
	  kf->mObjRot.mRots->m_Size = *(buff+1) / sizeof(float);
	  kf->mObjRot.mRots->m_MaxSize = *(buff+1) / sizeof(float);
	  Array_CreateElement(kf->mObjRot.mRots);
	  memcpy(kf->mObjRot.mRots->m_Data, (buff+2), *(buff+1));
				
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	case CLASS_KF_OBJECT_ROTSPLDATA :
	  kf->mObjRot.mSplData = Array_Create();
	  kf->mObjRot.mSplData->m_ElemSize = sizeof(float);
	  kf->mObjRot.mSplData->m_Size = *(buff+1) / sizeof(float);
	  kf->mObjRot.mSplData->m_MaxSize = *(buff+1) / sizeof(float);
	  Array_CreateElement(kf->mObjRot.mSplData);
	  memcpy(kf->mObjRot.mSplData->m_Data, (buff+2), *(buff+1));
				
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	case CLASS_KF_OBJECT_SCLFRAMES :
	  kf->mObjScl.mFrames = Array_Create();
	  kf->mObjScl.mFrames->m_ElemSize = sizeof(float);
	  kf->mObjScl.mFrames->m_Size = *(buff+1) / sizeof(float);
	  kf->mObjScl.mFrames->m_MaxSize = *(buff+1) / sizeof(float);
	  Array_CreateElement(kf->mObjScl.mFrames);
	  memcpy(kf->mObjScl.mFrames->m_Data, (buff+2), *(buff+1));
				
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	case CLASS_KF_OBJECT_SCLDATA :
	  kf->mObjScl.mScls = Array_Create();
	  kf->mObjScl.mScls->m_ElemSize = sizeof(float);
	  kf->mObjScl.mScls->m_Size = *(buff+1) / sizeof(float);
	  kf->mObjScl.mScls->m_MaxSize = *(buff+1) / sizeof(float);
	  Array_CreateElement(kf->mObjScl.mScls);
	  memcpy(kf->mObjScl.mScls->m_Data, (buff+2), *(buff+1));
				
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	case CLASS_KF_OBJECT_SCLSPLDATA :
	  kf->mObjScl.mSplData = Array_Create();
	  kf->mObjScl.mSplData->m_ElemSize = sizeof(float);
	  kf->mObjScl.mSplData->m_Size = *(buff+1) / sizeof(float);
	  kf->mObjScl.mSplData->m_MaxSize = *(buff+1) / sizeof(float);
	  Array_CreateElement(kf->mObjScl.mSplData);
	  memcpy(kf->mObjScl.mSplData->m_Data, (buff+2), *(buff+1));
				
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	case CLASS_ENGINE_SPEED :
	  ((EngineData *)kf)->m_Speed = *((float *)buff+1);

	  buff += 2;
	  count -= 8;
	  break;
	case CLASS_ENGINE_STARTTIME :
	  ((EngineData *)kf)->m_StartTime = *((float *)buff+1);

	  buff += 2;
	  count -= 8;
	  break;
	case CLASS_ENGINE_DURATION :
	  ((EngineData *)kf)->m_Duration = *((float *)buff+1);

	  buff += 2;
	  count -= 8;
	  break;
	case CLASS_ENGINE_STATUS :
	  ((EngineData *)kf)->m_Status = *(buff+1);

	  buff += 2;
	  count -= 8;
	  break;
	case CLASS_ENGINE_CONTROL :
	  ((EngineData *)kf)->m_Status = *(buff+1);

	  buff += 2;
	  count -= 8;
	  break;
	default:
	  printf("13 other types\n");
	  printf("case = %x\n", *buff);
	  printf("size = %d\n", *(buff+1));
	  count = count - *(buff+1) - 8;
	  offset = (*(buff+1) + 3) >> 2;
	  buff = buff + offset + 2;
	  break;
	}
    }
}
