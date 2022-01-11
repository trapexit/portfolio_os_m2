
#include "ifflib.h"
#include "string.h"
#include <stdio.h>
#include "M2TXTypes.h"
#include "qmem.h"
#include "LWSURF.h"


void LWSURF_Init(LWSURF *surf)
{
  surf->ValSet = 0;
  surf->RIMG = NULL;
  surf->CTEX = NULL;
  surf->DTEX = NULL;
  surf->STEX = NULL;
  surf->RTEX = NULL;
  surf->TTEX = NULL;
  surf->BTEX = NULL;
  surf->TexIndex = -1;
  surf->MatIndex = -1;
  surf->MinU = surf->MinV = 10000;
  surf->MaxU = surf->MaxV = -10000;
  surf->XTile = FALSE;
  surf->YTile = FALSE;
  surf->EnvMap = FALSE;
  surf->TRAN = 0.0;
}

bool LWSURF_GetCOLR(LWSURF surf, uint32 *color)
{
  if (surf.ValSet & LWSURF_COLRMask)
  {
    *color = surf.COLR;
    return(TRUE);
 }
 else
    return(FALSE);
}
 
bool LWSURF_GetRed(LWSURF surf, uint8 *red)
{
  if (surf.ValSet & LWSURF_COLRMask)
    {
      *red = surf.COLR & 0xFF;
      return(TRUE);
    }
  else
    return(FALSE);
}

bool LWSURF_GetGreen(LWSURF surf, uint8 *green)
{
  if (surf.ValSet & LWSURF_COLRMask)
    {
      *green = (surf.COLR & 0xFF00)>>8;
      return(TRUE);
    }
  else
    return(FALSE);
}

bool LWSURF_GetBlue(LWSURF surf, uint8 *blue)
{
  if (surf.ValSet & LWSURF_COLRMask)
    {
      *blue = (surf.COLR & 0xFF0000)>>16;
      return(TRUE);
    }
  else
    return(FALSE);
}

bool LWSURF_GetLUMI(LWSURF surf, uint16 *lumi)
{
  if (surf.ValSet & LWSURF_LUMIMask)
    {
      *lumi = surf.LUMI;
      return(TRUE);
    }
  else
    return(FALSE);
}

bool LWSURF_GetDIFF(LWSURF surf, uint16 *diff)
{
  if (surf.ValSet & LWSURF_DIFFMask)
    {
      *diff = surf.DIFF;
      return(TRUE);
    }
  else
    return(FALSE);
}

bool LWSURF_GetSMAN(LWSURF surf, gfloat *size)
{
  if (surf.ValSet & LWSURF_SMANMask)
    {
      *size = surf.SMAN;
      return(TRUE);
    }
  else
    return(FALSE);
}


bool LWSURF_GetSPEC(LWSURF surf, uint16 *spec)
{
  if (surf.ValSet & LWSURF_SPECMask)
  {
    *spec = surf.SPEC;
    return(TRUE);
  }
  else
    return(FALSE);
}

bool LWSURF_GetREFL(LWSURF surf, uint16 *refl)
{
  if (surf.ValSet & LWSURF_REFLMask)
    {
      *refl = surf.REFL;
      return(TRUE);
    }
  else
    return(FALSE);
}

bool LWSURF_GetGLOS(LWSURF surf, uint16 *glos)
{
  if (surf.ValSet & LWSURF_GLOSMask)
    {
      *glos = surf.GLOS;
      return(TRUE);
    }
  else
    return(FALSE);
}

bool LWSURF_GetTRAN(LWSURF surf, uint16 *tran)
{
  if (surf.ValSet & LWSURF_TRANMask)
    {
      *tran = surf.TRAN;
      return(TRUE);
    }
  else
    return(FALSE);
}

bool LWSURF_GetFLAG(LWSURF surf, uint16 *flag)
{
  if (surf.ValSet & LWSURF_FLAGMask)
    {
      *flag = surf.FLAG;
      return(TRUE);
    }
  else
    return(FALSE);
}

bool LWSURF_GetFLuminous(LWSURF surf, bool *flag)
{
  if (surf.ValSet & LWSURF_FLAGMask)
    {
      *flag = (surf.FLAG & LWSURF_FLuminous) ? TRUE : FALSE;
      return(TRUE);
    }
  else
    return(FALSE);
}

bool LWSURF_GetF2Sided(LWSURF surf, bool *flag)
{
  if (surf.ValSet & LWSURF_FLAGMask)
    {
      *flag = (surf.FLAG & LWSURF_F2Sided) ? TRUE : FALSE;
      return(TRUE);
    }
  else
    return(FALSE);
}

bool LWSURF_GetFColHilite(LWSURF surf, bool *flag)
{
  if (surf.ValSet & LWSURF_FLAGMask)
    {
      *flag = (surf.FLAG & LWSURF_FColHilite) ? TRUE : FALSE;
      return(TRUE);
    }
  else
    return(FALSE);
}

bool LWSURF_GetFSmooth(LWSURF surf, bool *flag)
{
  if (surf.ValSet & LWSURF_FLAGMask)
    {
      *flag = (surf.FLAG & LWSURF_FSmooth) ? TRUE : FALSE;
      return(TRUE);
    }
  else
    return(FALSE);
}

void LWSURF_SetCOLR(LWSURF *surf, uint8 color[4])
{
  uint32 temp;
  uint32 red, green, blue, alpha;
  
  surf->ValSet |= LWSURF_COLRMask;
  red = color[0];
  green = color[1];
  blue = color[2];
  alpha = color[3];
  temp = red + (green<<8) + (blue<<16) + (alpha<<24);
  surf->COLR = temp;
}
 
void LWSURF_SetRed(LWSURF *surf, uint8 red)
{
  surf->ValSet |= LWSURF_COLRMask;
  surf->COLR = red | (surf->COLR & 0xFFFFFF00) ;
}

void LWSURF_SetGreen(LWSURF *surf, uint8 green)
{
  surf->ValSet |= LWSURF_COLRMask;
  surf->COLR = (green<<8) | (surf->COLR & 0xFFFF00FF);
}

void LWSURF_SetBlue(LWSURF *surf, uint8 blue)
{
  surf->ValSet |= LWSURF_COLRMask;
  surf->COLR = (blue<<16) | (surf->COLR & 0xFF00FFFF);      
}

void LWSURF_SetLUMI(LWSURF *surf, uint16 lumi)
{
  surf->ValSet |= LWSURF_LUMIMask;
  surf->LUMI = lumi;
}

void LWSURF_SetDIFF(LWSURF *surf, uint16 diff)
{
  surf->ValSet |= LWSURF_DIFFMask;
  surf->DIFF = diff;
}

void LWSURF_SetSPEC(LWSURF *surf, uint16 spec)
{
  surf->ValSet |= LWSURF_SPECMask;
  surf->SPEC =  spec;    
}

void LWSURF_SetREFL(LWSURF *surf, uint16 refl)
{
  surf->ValSet |= LWSURF_REFLMask;
  surf->REFL = refl;      
}

void LWSURF_SetGLOS(LWSURF *surf, uint16 glos)
{
  surf->ValSet |= LWSURF_GLOSMask;
  surf->GLOS = glos;      
}

void LWSURF_SetTRAN(LWSURF *surf, uint16 tran)
{
  surf->ValSet |= LWSURF_TRANMask;
  surf->TRAN = tran;
}

void LWSURF_SetFLAG(LWSURF *surf, uint16 flag)
{
  surf->ValSet |= LWSURF_FLAGMask;
  surf->FLAG = flag;
}

void LWSURF_SetRIMG(LWSURF *surf, int16 nameLen, char *nameBuf)
{
  surf->RIMG = (char *)qMemNewPtr(nameLen);
  strcpy(surf->RIMG,nameBuf);
}

void LWSURF_SetSMAN(LWSURF *surf, gfloat param)
{
  surf->ValSet |= LWSURF_SMANMask;
  surf->SMAN = param;
}


bool LWTex_GetTFLG(LWTex tex, uint16 *data)
{
  if (tex.ValSet & LWTex_TFLGMask)
    {
      *data = tex.TFLG;
      return(TRUE);
    }
  else
    return(FALSE);
}

bool LWTex_GetTSIZ(LWTex tex, Point3 *size)
{
  if (tex.ValSet & LWTex_TSIZMask)
    {
      size->x = tex.TSIZ.x;
      size->y = tex.TSIZ.y;
      size->z = tex.TSIZ.z;
      return(TRUE);
    }
  else
    return(FALSE);
}

bool LWTex_GetTSP0(LWTex tex, gfloat *size)
{
  if (tex.ValSet & LWTex_TSP0Mask)
    {
      *size = tex.TSP0;
      return(TRUE);
    }
  else
    return(FALSE);
}

bool LWTex_GetTSP1(LWTex tex, gfloat *size)
{
  if (tex.ValSet & LWTex_TSP1Mask)
    {
      *size = tex.TSP1;
      return(TRUE);
    }
  else
    return(FALSE);
}
bool LWTex_GetTFP0(LWTex tex, gfloat *size)
{
  if (tex.ValSet & LWTex_TFP0Mask)
    {
      *size = tex.TFP0;
      return(TRUE);
    }
  else
    return(FALSE);
}

bool LWTex_GetTFP1(LWTex tex, gfloat *size)
{
  if (tex.ValSet & LWTex_TFP1Mask)
    {
      *size = tex.TFP1;
      return(TRUE);
    }
  else
    return(FALSE);
}

bool LWTex_GetTCTR(LWTex tex, Point3 *center)
{
  if (tex.ValSet & LWTex_TCTRMask)
    {
      center->x = tex.TCTR.x;
      center->y = tex.TCTR.y;
      center->z = tex.TCTR.z;
      return(TRUE);
    }
  else
    return(FALSE);
}

bool LWTex_GetTIMG(LWTex tex, char **timg)
{
  if (tex.ValSet & LWTex_TFLGMask)
    {
      *timg = tex.TIMG;
      return(TRUE);
    }
  else
    return(FALSE);
}

bool LWTex_GetFXAxis(LWTex tex, bool *flag)
{
  if (tex.ValSet & LWTex_TFLGMask)
    {
      *flag = (tex.TFLG & LWTex_FXAxis) ? TRUE : FALSE;
      return(TRUE);
    }
  else
    return(FALSE);
}

bool LWTex_GetFYAxis(LWTex tex, bool *flag)
{
  if (tex.ValSet & LWTex_TFLGMask)
    {
      *flag = (tex.TFLG & LWTex_FYAxis) ? TRUE : FALSE;
      return(TRUE);
    }
  else
    return(FALSE);
}

bool LWTex_GetFZAxis(LWTex tex, bool *flag)
{
  if (tex.ValSet & LWTex_TFLGMask)
    {
      *flag = (tex.TFLG & LWTex_FZAxis) ? TRUE : FALSE;
      return(TRUE);
    }
  else
    return(FALSE);
}

bool LWTex_GetFWorld(LWTex tex, bool *flag)
{
  if (tex.ValSet & LWTex_TFLGMask)
    {
      *flag = (tex.TFLG & LWTex_FWorld) ? TRUE : FALSE;
      return(TRUE);
    }
  else
    return(FALSE);
}

bool LWTex_GetFNegative(LWTex tex, bool *flag)
{
  if (tex.ValSet & LWTex_TFLGMask)
    {
      *flag = (tex.TFLG & LWTex_FNegative) ? TRUE : FALSE;
      return(TRUE);
    }
  else
    return(FALSE);
}

bool LWTex_GetFBlend(LWTex tex, bool *flag)
{
  if (tex.ValSet & LWTex_TFLGMask)
    {
      *flag = (tex.TFLG & LWTex_FBlend) ? TRUE : FALSE;
      return(TRUE);
    }
  else
    return(FALSE);
}

bool LWTex_GetFAnti(LWTex tex, bool *flag)
{
  if (tex.ValSet & LWTex_TFLGMask)
    {
      *flag = (tex.TFLG & LWTex_FAnti) ? TRUE : FALSE;
      return(TRUE);
    }
  else
    return(FALSE);
}

void LWTex_Init(LWTex *tex)
{
  tex->TIMG = NULL;
  tex->ValSet = 0;
  tex->TFP1 = 1.0;
  tex->TFP0 = 1.0;
}

void LWTex_SetTIMG(LWTex *tex, int16 nameLen, char *nameBuf)
{
  tex->TIMG = (char *)qMemNewPtr(nameLen+2);
  strcpy(tex->TIMG,nameBuf);
}

void LWTex_SetTSIZ(LWTex *tex, Point3 size)
{
  tex->TSIZ.x = size.x;
  tex->TSIZ.y = size.y;
  tex->TSIZ.z = size.z;
  tex->ValSet |= LWTex_TSIZMask;
}

void LWTex_SetTCTR(LWTex *tex, Point3 center)
{
  tex->TCTR.x = center.x;
  tex->TCTR.y = center.y;
  tex->TCTR.z = center.z;
  tex->ValSet |= LWTex_TCTRMask;
}

void LWTex_SetTFAL(LWTex *tex, Point3 falloff)
{
  tex->TFAL.x = falloff.x;
  tex->TFAL.y = falloff.y;
  tex->TFAL.z = falloff.z;
  tex->ValSet |= LWTex_TFALMask;
}

void LWTex_SetTVEL(LWTex *tex, Point3 velocity)
{
  tex->TVEL.x = velocity.x;
  tex->TVEL.y = velocity.y;
  tex->TVEL.z = velocity.z;
  tex->ValSet |= LWTex_TVELMask;
}

void LWTex_SetTAMP(LWTex *tex, gfloat amplitude)
{
  tex->TAMP = amplitude;
  tex->ValSet |= LWTex_TAMPMask;
}

void LWTex_SetTSP0(LWTex *tex, gfloat param)
{
  tex->TSP0 = param;
  tex->ValSet |= LWTex_TSP0Mask;
}

void LWTex_SetTSP1(LWTex *tex, gfloat param)
{
  tex->TSP1 = param;
  tex->ValSet |= LWTex_TSP1Mask;
}

void LWTex_SetTFP0(LWTex *tex, gfloat param)
{
  tex->TFP0 = param;
  tex->ValSet |= LWTex_TFP0Mask;
}

void LWTex_SetTFP1(LWTex *tex, gfloat param)
{
  tex->TFP1 = param;
  tex->ValSet |= LWTex_TFP1Mask;
}


void LWTex_SetTSP2(LWTex *tex, gfloat param)
{
  tex->TSP2 = param;
  tex->ValSet |= LWTex_TSP2Mask;
}

void LWTex_SetTVAL(LWTex *tex, uint16 value)
{
  tex->TVAL = value;
  tex->ValSet |= LWTex_TVALMask;
}

void LWTex_SetTFLG(LWTex *tex, uint16 flags)
{
  tex->TFLG = flags;
  tex->ValSet |= LWTex_TFLGMask;
}

void LWTex_SetTFRQ(LWTex *tex, uint16 frequency)
{
  tex->TFRQ = frequency;
  tex->ValSet |= LWTex_TFRQMask;
}

void LWSURF_SetCTEX(LWSURF *surf, int16 nameLen, char *nameBuf, LWTex **curTex)
{
  LWTex *tex;

  tex = surf->CTEX = (LWTex *)qMemClearPtr(1,sizeof(LWTex));
  tex->Name = (char *)qMemNewPtr(nameLen);
  strcpy(tex->Name,nameBuf);
  LWTex_Init(tex);
  *curTex = surf->CTEX;
}

void LWSURF_SetDTEX(LWSURF *surf, int16 nameLen, char *nameBuf, LWTex **curTex)
{
  surf->DTEX = (LWTex *)qMemClearPtr(1,sizeof(LWTex));
  surf->DTEX->Name = (char *)qMemNewPtr(nameLen);
  strcpy(surf->DTEX->Name,nameBuf);
  LWTex_Init(surf->DTEX);
  *curTex = surf->DTEX;
}

void LWSURF_SetSTEX(LWSURF *surf, int16 nameLen, char *nameBuf, LWTex **curTex)
{
  surf->STEX = (LWTex *)qMemClearPtr(1,sizeof(LWTex));
  surf->STEX->Name = (char *)qMemNewPtr(nameLen);
  strcpy(surf->STEX->Name,nameBuf);
  LWTex_Init(surf->STEX);
  *curTex = surf->STEX;
}

void LWSURF_SetRTEX(LWSURF *surf, int16 nameLen, char *nameBuf, LWTex **curTex)
{
  surf->RTEX = (LWTex *)qMemClearPtr(1,sizeof(LWTex));
  surf->RTEX->Name = (char *)qMemNewPtr(nameLen);
  strcpy(surf->RTEX->Name,nameBuf);
  LWTex_Init(surf->RTEX);
  *curTex = surf->RTEX;
}

void LWSURF_SetTTEX(LWSURF *surf, int16 nameLen, char *nameBuf, LWTex **curTex)
{
  surf->TTEX = (LWTex *)qMemClearPtr(1,sizeof(LWTex));
  surf->TTEX->Name = (char *)qMemNewPtr(nameLen);
  strcpy(surf->TTEX->Name,nameBuf);
  LWTex_Init(surf->TTEX);
  *curTex = surf->TTEX;
}

void LWSURF_SetBTEX(LWSURF *surf, int16 nameLen, char *nameBuf, LWTex **curTex)
{
  surf->BTEX = (LWTex *)qMemClearPtr(1,sizeof(LWTex));
  surf->BTEX->Name = (char *)qMemNewPtr(nameLen);
  strcpy(surf->BTEX->Name,nameBuf);
  LWTex_Init(surf->BTEX);
  *curTex = surf->BTEX;
}














