/*
	File:		page.c

	Contains:	Handles page manipulation

	Written by:	Todd Allendorf

	Copyright:	© 1996 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):

		<2+>	12/15/95	TMA		Added support for TexBlend parsing.

	To Do:
*/

#include "ifflib.h"
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "M2TXTypes.h"
#include "M2TXattr.h"
#include "M2TXFormat.h"
#include "M2TXHeader.h"
#include "SDFTexBlend.h"
#include "qmem.h"
#include "clt.h"
#include "clttxdblend.h"
#include  "utfpage.h"
#include "page.h"

#include "LWSURF.h"
#include "LWToken.h"
#include <assert.h>
#include "os.h"
#include "lws.i"
#include "lws.h"

extern LWS  *cur_lws;

#ifdef applec
static int strcasecmp(char *s, char *t)
{
  for (; tolower(*s) == tolower(*t); s++, t++)
    if (*s == '\0')
      return 0;
  return tolower(*s)-tolower(*t);
}

#endif

M2Err PageCLT_Extend(CltSnippet *snip, uint32 size)
{
  uint32 *tmp, allocSize;

  if ((snip->size+size) > snip->allocated)
    {
      allocSize = snip->allocated + CLT_BUF_SIZE;
      tmp = (uint32 *)qMemResizePtr(snip->data, allocSize*sizeof(uint32));
      if (tmp == NULL)
	return(M2E_NoMem);
      else
	snip->data = tmp;
      snip->allocated = allocSize;
    }
  return(M2E_NoErr);
}

M2Err PageCLT_Init(CltSnippet *snip)
{
  snip->data = NULL;
  snip->allocated = 0;
  snip->size = 0;
  return(PageCLT_Extend (snip, CLT_BUF_SIZE));
}

M2Err PageNames_Extend(PageNames *pn, uint32 size)
{
  char **tmp;
  uint32 allocSize;

  if ((pn->NumNames+size) > pn->Allocated)
    {
      allocSize = pn->Allocated + PAGE_BUF_SIZE;
      tmp = (char **)qMemResizePtr(pn->SubNames, allocSize*sizeof(char *));
      if (tmp == NULL)
	return(M2E_NoMem);
      else
	pn->SubNames = tmp;
    }
  pn->Allocated = allocSize;
  return(M2E_NoErr);
}

M2Err PageNames_Init(PageNames *pn)
{
  pn->PageName = NULL;
  pn->NumNames = 0;
  pn->Allocated = 0;

  return(PageNames_Extend(pn, PAGE_BUF_SIZE));
}

void TxPage_Init(TxPage *pg)
{
  int i;

  pg->PgHeader.Offset = 0;
  pg->PgHeader.PgFlags = 0;
  pg->PgHeader.MinXSize = 0;
  pg->PgHeader.MinYSize = 0;
  pg->PgHeader.TexFormat = 0;
  pg->PgHeader.NumLOD = 0;
  pg->PgHeader.PIPIndexOffset = 0;
  pg->TABRegSet = 0;
  for (i=0; i<TAB_REG_SIZE; i++)
    pg->TABReg[i] = 0;
  pg->DABRegSet = 0;
  for (i=0; i<DAB_REG_SIZE; i++)
    pg->DABReg[i] = 0;
  PageCLT_Init(&(pg->CLT));
}


static void DAB_UsrCntl(uint32 field, uint32 value, 
			    uint32 *regVal)
{
  uint32 temp;

  switch(field)
    {
    case DBLA_ZBuffEnable:
      temp = (*regVal) & (~CLT_Mask(DBUSERCONTROL, ZBUFFEN));
      temp |= CLT_Bits( DBUSERCONTROL, ZBUFFEN, value);
      break;
    case DBLA_ZBuffOutEnable:
      temp = (*regVal) & (~CLT_Mask(DBUSERCONTROL, ZOUTEN));
      temp |= CLT_Bits( DBUSERCONTROL, ZOUTEN, value);
      break;
    case DBLA_WinClipInEnable:
      temp = (*regVal) & (~CLT_Mask(DBUSERCONTROL, WINCLIPINEN));
      temp |= CLT_Bits( DBUSERCONTROL, WINCLIPINEN, value);
      break;
    case DBLA_WinClipOutEnable:
      temp = (*regVal) & (~CLT_Mask(DBUSERCONTROL, WINCLIPOUTEN));
      temp |= CLT_Bits( DBUSERCONTROL, WINCLIPOUTEN, value);
      break;
    case DBLA_BlendEnable:
      temp = (*regVal) & (~CLT_Mask(DBUSERCONTROL, BLENDEN));
      temp |= CLT_Bits( DBUSERCONTROL, BLENDEN, value);
      break;
    case DBLA_SrcInputEnable:
      temp = (*regVal) & (~CLT_Mask(DBUSERCONTROL, SRCEN));
      temp |= CLT_Bits( DBUSERCONTROL, SRCEN, value);
      break;
    case DBLA_DitherEnable:
      temp = (*regVal) & (~CLT_Mask(DBUSERCONTROL, DITHEREN));
      temp |= CLT_Bits( DBUSERCONTROL, DITHEREN, value);
      break;
    case DBLA_RGBADestOut:
      temp = (*regVal) & (~CLT_Mask(DBUSERCONTROL, DESTOUTMASK));
      temp |= CLT_Bits( DBUSERCONTROL, DESTOUTMASK, value);
      break;
    }
  *regVal = temp;
}

static void DAB_DiscardCntl(uint32 field, uint32 value, 
			    uint32 *regVal)
{
  uint32 temp;

  switch(field)
    {
    case DBLA_DiscardZClipped:
      temp = (*regVal) & (~CLT_Mask(DBDISCARDCONTROL, ZCLIPPED));
      temp |= CLT_Bits( DBDISCARDCONTROL, ZCLIPPED, value);
      break;
    case DBLA_DiscardSSB0:
      temp = (*regVal) & (~CLT_Mask(DBDISCARDCONTROL, SSB0));
      temp |= CLT_Bits( DBDISCARDCONTROL, SSB0, value);
      break;
    case DBLA_DiscardRGB0:
      temp = (*regVal) & (~CLT_Mask(DBDISCARDCONTROL, RGB0));
      temp |= CLT_Bits( DBDISCARDCONTROL, RGB0, value);
      break;
    case DBLA_DiscardAlpha0:
      temp = (*regVal) & (~CLT_Mask(DBDISCARDCONTROL, ALPHA0));
      temp |= CLT_Bits( DBDISCARDCONTROL, ALPHA0, value);
      break;
    }
  *regVal = temp;
}

static void DAB_XWinClip(uint32 field, uint32 value, 
			    uint32 *regVal)
{
  uint32 temp;

  switch(field)
    {
    case DBLA_XWinClipMin:
      temp = (*regVal) & (~CLT_Mask(DBXWINCLIP, XWINCLIPMIN));
      temp |= CLT_Bits( DBXWINCLIP, XWINCLIPMIN, value);
      break;
    case DBLA_XWinClipMax:
      temp = (*regVal) & (~CLT_Mask(DBXWINCLIP, XWINCLIPMAX));
      temp |= CLT_Bits( DBXWINCLIP, XWINCLIPMAX, value);
      break;
    }
  *regVal = temp;
}

static void DAB_YWinClip(uint32 field, uint32 value, 
			    uint32 *regVal)
{
  uint32 temp;

  switch(field)
    {
    case DBLA_YWinClipMin:
      temp = (*regVal) & (~CLT_Mask(DBYWINCLIP, YWINCLIPMIN));
      temp |= CLT_Bits( DBYWINCLIP, YWINCLIPMIN, value);
      break;
    case DBLA_YWinClipMax:
      temp = (*regVal) & (~CLT_Mask(DBYWINCLIP, YWINCLIPMAX));
      temp |= CLT_Bits( DBYWINCLIP, YWINCLIPMAX, value);
      break;
    }
  *regVal = temp;
}

static void DAB_ZOffset(uint32 field, uint32 value, 
			    uint32 *regVal)
{
  uint32 temp;

  switch(field)
    {
    case DBLA_ZXOffset:
      temp = (*regVal) & (~CLT_Mask(DBZOFFSET, ZXOFFSET));
      temp |= CLT_Bits( DBZOFFSET, ZXOFFSET, value);
      break;
    case DBLA_ZYOffset:
      temp = (*regVal) & (~CLT_Mask(DBZOFFSET, ZYOFFSET));
      temp |= CLT_Bits( DBZOFFSET, ZYOFFSET, value);
      break;
    }
  *regVal = temp;
}

static void DAB_SSBDSBCntl(uint32 field, uint32 value, 
			    uint32 *regVal)
{
  uint32 temp;

  switch(field)
    {
    case DBLA_DSBConst:
      temp = (*regVal) & (~CLT_Mask(DBSSBDSBCNTL, DSBCONST));
      temp |= CLT_Bits( DBSSBDSBCNTL, DSBCONST, value);
      break;
    case DBLA_DSBSelect:
      temp = (*regVal) & (~CLT_Mask(DBSSBDSBCNTL, DSBSELECT));
      temp |= CLT_Bits( DBSSBDSBCNTL, DSBSELECT, value);
      break;
    }
  *regVal = temp;
}

static void DAB_AMultCntl(uint32 field, uint32 value, 
			    uint32 *regVal)
{
  uint32 temp;

  switch(field)
    {
    case DBLA_AInputSelect:
      temp = (*regVal) & (~CLT_Mask(DBAMULTCNTL, AINPUTSELECT));
      temp |= CLT_Bits( DBAMULTCNTL, AINPUTSELECT, value);
      break;
    case DBLA_AMultCoefSelect:
      temp = (*regVal) & (~CLT_Mask(DBAMULTCNTL, AMULTCOEFSELECT));
      temp |= CLT_Bits( DBAMULTCNTL, AMULTCOEFSELECT, value);
      break;
    case DBLA_AMultConstControl:
      temp = (*regVal) & (~CLT_Mask(DBAMULTCNTL, AMULTCONSTCONTROL));
      temp |= CLT_Bits( DBAMULTCNTL, AMULTCONSTCONTROL, value);
      break;
    case DBLA_AMultRtJustify:
      temp = (*regVal) & (~CLT_Mask(DBAMULTCNTL, AMULTRJUSTIFY));
      temp |= CLT_Bits( DBAMULTCNTL, AMULTRJUSTIFY, value);
      break;
    }
  *regVal = temp;
}

static void DAB_BMultCntl(uint32 field, uint32 value, 
			    uint32 *regVal)
{
  uint32 temp;

  switch(field)
    {
    case DBLA_BInputSelect:
      temp = (*regVal) & (~CLT_Mask(DBBMULTCNTL, BINPUTSELECT));
      temp |= CLT_Bits( DBBMULTCNTL, BINPUTSELECT, value);
      break;
    case DBLA_BMultCoefSelect:
      temp = (*regVal) & (~CLT_Mask(DBBMULTCNTL, BMULTCOEFSELECT));
      temp |= CLT_Bits( DBBMULTCNTL, BMULTCOEFSELECT, value);
      break;
    case DBLA_BMultConstControl:
      temp = (*regVal) & (~CLT_Mask(DBBMULTCNTL, BMULTCONSTCONTROL));
      temp |= CLT_Bits( DBBMULTCNTL, BMULTCONSTCONTROL, value);
      break;
    case DBLA_BMultRtJustify:
      temp = (*regVal) & (~CLT_Mask(DBBMULTCNTL, BMULTRJUSTIFY));
      temp |= CLT_Bits( DBBMULTCNTL, BMULTRJUSTIFY, value);
      break;
    }
  *regVal = temp;
}

static void DAB_ALUCntl(uint32 field, uint32 value, 
			    uint32 *regVal)
{
  uint32 temp;

  switch(field)
    {
    case DBLA_ALUOperation:
      temp = (*regVal) & (~CLT_Mask(DBALUCNTL, ALUOPERATION));
      temp |= CLT_Bits( DBALUCNTL, ALUOPERATION, value);
      break;
    case DBLA_FinalDivide:
      temp = (*regVal) & (~CLT_Mask(DBALUCNTL, FINALDIVIDE));
      temp |= CLT_Bits( DBALUCNTL, FINALDIVIDE, value);
      break;
    }
  *regVal = temp;
}

static void DAB_SrcAlphaCntl(uint32 field, uint32 value, 
			    uint32 *regVal)
{
  uint32 temp;

  switch(field)
    {
    case DBLA_Alpha0ClampControl:
      temp = (*regVal) & (~CLT_Mask(DBSRCALPHACNTL, ALPHA0));
      temp |= CLT_Bits( DBSRCALPHACNTL, ALPHA0, value);
      break;
    case DBLA_Alpha1ClampControl:
      temp = (*regVal) & (~CLT_Mask(DBSRCALPHACNTL, ALPHA1));
      temp |= CLT_Bits( DBSRCALPHACNTL, ALPHA1, value);
      break;
    case DBLA_AlphaFracClampControl:
      temp = (*regVal) & (~CLT_Mask(DBSRCALPHACNTL, ALPHAFRAC));
      temp |= CLT_Bits( DBSRCALPHACNTL, ALPHAFRAC, value);
      break;
    case DBLA_AlphaClampControl:
      temp = (*regVal) & (~CLT_Mask(DBSRCALPHACNTL, ACLAMP));
      temp |= CLT_Bits( DBSRCALPHACNTL, ACLAMP, value);
      break;
    }
  *regVal = temp;
}

static void DAB_DestAlphaConst(uint32 field, uint32 value, 
			    uint32 *regVal)
{
  uint32 temp;

  switch(field)
    {
    case DBLA_DestAlphaConstSSB0:
      temp = (*regVal) & (~CLT_Mask(DBDESTALPHACONST, DESTALPHACONSTSSB0));
      temp |= CLT_Bits( DBDESTALPHACONST, DESTALPHACONSTSSB0, value);
      break;
    case DBLA_DestAlphaConstSSB1:
      temp = (*regVal) & (~CLT_Mask(DBDESTALPHACONST, DESTALPHACONSTSSB1));
      temp |= CLT_Bits( DBDESTALPHACONST, DESTALPHACONSTSSB1, value);
      break;
    }
  *regVal = temp;
}

static void TAB_AddrCntl(uint32 field, uint32 value, 
			    uint32 *regVal)
{
  uint32 temp;

  switch(field)
    {
    case TXA_TextureEnable:
      temp = (*regVal) & (~CLT_Mask(TXTADDRCNTL, TEXTUREENABLE));
      temp |= CLT_Bits(TXTADDRCNTL, TEXTUREENABLE, value);
      break;
    case TXA_MinFilter:
      temp = (*regVal) & (~CLT_Mask(TXTADDRCNTL, MINFILTER));
      temp |= CLT_Bits(TXTADDRCNTL, MINFILTER, value);
      break;
    case TXA_InterFilter:
      temp = (*regVal) & (~CLT_Mask(TXTADDRCNTL, INTERFILTER));
      temp |= CLT_Bits(TXTADDRCNTL, INTERFILTER, value);
      break;
    case TXA_MagFilter:
      temp = (*regVal) & (~CLT_Mask(TXTADDRCNTL, MAGFILTER));
      temp |= CLT_Bits(TXTADDRCNTL, MAGFILTER, value);
      break;
    default:
      fprintf(stderr, "Unknown value in AddrCntl=%d\n", field);
      return;
      break;
    }
  *regVal = temp;
}

static void TAB_TABCntl(uint32 field, uint32 value, 
			    uint32 *regVal)
{
  uint32 temp;

  switch(field)
    {
    case TXA_FirstColor:
      temp = (*regVal) & (~CLT_Mask(TXTTABCNTL, FIRSTCOLOR));
      temp |= CLT_Bits(TXTTABCNTL, FIRSTCOLOR, value);
      break;
    case TXA_SecondColor:
      temp = (*regVal) & (~CLT_Mask(TXTTABCNTL, SECONDCOLOR));
      temp |= CLT_Bits(TXTTABCNTL, SECONDCOLOR, value);
      break;
    case TXA_ThirdColor:
      temp = (*regVal) & (~CLT_Mask(TXTTABCNTL, THIRDCOLOR));
      temp |= CLT_Bits(TXTTABCNTL, THIRDCOLOR, value);
      break;
    case TXA_FirstAlpha:
      temp = (*regVal) & (~CLT_Mask(TXTTABCNTL, FIRSTALPHA));
      temp |= CLT_Bits(TXTTABCNTL, FIRSTALPHA, value);
      break;
    case TXA_SecondAlpha:
      temp = (*regVal) & (~CLT_Mask(TXTTABCNTL, SECONDALPHA));
      temp |= CLT_Bits(TXTTABCNTL, SECONDALPHA, value);
      break;
    case TXA_ColorOut:
      temp = (*regVal) & (~CLT_Mask(TXTTABCNTL, COLOROUT));
      temp |= CLT_Bits(TXTTABCNTL, COLOROUT, value);
      break;
    case TXA_AlphaOut:
      temp = (*regVal) & (~CLT_Mask(TXTTABCNTL, ALPHAOUT));
      temp |= CLT_Bits(TXTTABCNTL, ALPHAOUT, value);
      break;
    case TXA_BlendOp:
      temp = (*regVal) & (~CLT_Mask(TXTTABCNTL, BLENDOP));
      temp |= CLT_Bits(TXTTABCNTL, BLENDOP, value);
      break;
    default:
      fprintf(stderr, "Unknown value in TABCntl=%d\n", field); 
      return;
      break;
    }
  *regVal = temp;
}

static void TAB_PIPCntl(uint32 field, uint32 value, 
			    uint32 *regVal)
{
  uint32 temp;

  switch(field)
    {
    case TXA_PipIndexOffset:
      temp = (*regVal) & (~CLT_Mask(TXTPIPCNTL, PIPINDEXOFFSET));
      temp |= CLT_Bits(TXTPIPCNTL, PIPINDEXOFFSET, value);
      break;
    case TXA_PipColorSelect:
      temp = (*regVal) & (~CLT_Mask(TXTPIPCNTL, PIPCOLORSELECT));
      temp |= CLT_Bits(TXTPIPCNTL, PIPCOLORSELECT, value);
      break;
    case TXA_PipAlphaSelect:
      temp = (*regVal) & (~CLT_Mask(TXTPIPCNTL, PIPALPHASELECT));
      temp |= CLT_Bits(TXTPIPCNTL, PIPALPHASELECT, value);
      break;
    case TXA_PipSSBSelect:
      temp = (*regVal) & (~CLT_Mask(TXTPIPCNTL, PIPSSBSELECT));
      temp |= CLT_Bits(TXTPIPCNTL, PIPSSBSELECT, value);
      break;
    default:
      fprintf(stderr, "Unknown value in PIPCntl=%d\n", field); 
      return;
      break;
    }
  *regVal = temp;
}

#define MkTABRegMask(command) (uint32)(1<<command)

static void TxPage_SetTABReg(TxPage *pg, uint32 reg, uint32 value)
{
  pg->TABRegSet |= MkTABRegMask(reg);
  pg->TABReg[reg] = value;
}

static bool TxPage_GetTABReg(TxPage *pg, uint32 reg, uint32 *value)
{
  if ((MkTABRegMask(reg))&(pg->TABRegSet))
    {
      *value = pg->TABReg[reg];
      return(TRUE);
    }
  *value = 0;
  return(FALSE);
}

static void TxPage_SetDABReg(TxPage *pg, uint32 reg, uint32 value)
{
  pg->DABRegSet |= MkTABRegMask(reg);
  pg->DABReg[reg] = value;
}

static bool TxPage_GetDABReg(TxPage *pg, uint32 reg, uint32 *value)
{
  if ((MkTABRegMask(reg))&(pg->DABRegSet))
    {
      *value = pg->DABReg[reg];
      return(TRUE);
    }
  *value = 0;
  return(FALSE);
}

static void TxPage_ConstRegSet(TxPage *pg, uint32 reg, uint32 value)
{

  switch(reg)
    {
    case TXA_PipConstSSB0:
      TxPage_SetTABReg(pg, TXA_PIPConstSSB0, value);
      break;
    case TXA_PipConstSSB1:
      TxPage_SetTABReg(pg, TXA_PIPConstSSB1, value);
      break;
    case TXA_BlendColorSSB0:
      TxPage_SetTABReg(pg, TXA_TABConst0, value);
      break;
    case TXA_BlendColorSSB1:
      TxPage_SetTABReg(pg, TXA_TABConst1, value);
      break;
    default:
      fprintf(stderr, "Unknown value in ConstRegSet=%d\n", reg); 
      return;
      break;
    }
}

void TxPage_DABInterpret(TxPage *pg, uint32 command, uint32 value)
{
  uint32 regVal;
  switch(command)
    {
    case DBLA_ZBuffEnable:
    case DBLA_ZBuffOutEnable:
    case DBLA_WinClipInEnable:
    case DBLA_WinClipOutEnable:
    case DBLA_BlendEnable:
    case DBLA_SrcInputEnable:
    case DBLA_DitherEnable:
    case DBLA_RGBADestOut:
      TxPage_GetDABReg(pg, DBLA_UsrCntl, &regVal);
      DAB_UsrCntl(command, value, &regVal);
      TxPage_SetDABReg(pg, DBLA_UsrCntl, regVal);
      break;
    case DBLA_DiscardZClipped:
    case DBLA_DiscardSSB0:
    case DBLA_DiscardRGB0:
    case DBLA_DiscardAlpha0:
      TxPage_GetDABReg(pg, DBLA_DiscardCntl, &regVal);
      DAB_DiscardCntl(command, value, &regVal);
      TxPage_SetDABReg(pg, DBLA_DiscardCntl, regVal);
      break;
    case  DBLA_XWinClipMin:
    case  DBLA_XWinClipMax:
      TxPage_GetDABReg(pg, DBLA_XWinClip, &regVal);
      DAB_XWinClip(command, value, &regVal);
      TxPage_SetDABReg(pg, DBLA_XWinClip, regVal);
      break;
    case  DBLA_YWinClipMin:
    case  DBLA_YWinClipMax:
      TxPage_GetDABReg(pg, DBLA_YWinClip, &regVal);
      DAB_YWinClip(command, value, &regVal);
      TxPage_SetDABReg(pg, DBLA_YWinClip, regVal);
      break;
    case  DBLA_ZXOffset:
    case  DBLA_ZYOffset:
      TxPage_GetDABReg(pg, DBLA_ZOffset, &regVal);
      DAB_ZOffset(command, value, &regVal);
      TxPage_SetDABReg(pg, DBLA_ZOffset, regVal);
      break;
    case  DBLA_DSBConst:
    case  DBLA_DSBSelect:
      TxPage_GetDABReg(pg, DBLA_SSBDSBCntl, &regVal);
      DAB_SSBDSBCntl(command, value, &regVal);
      TxPage_SetDABReg(pg, DBLA_SSBDSBCntl, regVal);
      break;
    case  DBLA_AInputSelect:
    case  DBLA_AMultCoefSelect:
    case  DBLA_AMultConstControl:
    case  DBLA_AMultRtJustify:
      TxPage_GetDABReg(pg, DBLA_AMultCntl, &regVal);
      DAB_AMultCntl(command, value, &regVal);
      TxPage_SetDABReg(pg, DBLA_AMultCntl, regVal);
      break;
    case  DBLA_BInputSelect:
    case  DBLA_BMultCoefSelect:
    case  DBLA_BMultConstControl:
    case  DBLA_BMultRtJustify:
      TxPage_GetDABReg(pg, DBLA_BMultCntl, &regVal);
      DAB_BMultCntl(command, value, &regVal);
      TxPage_SetDABReg(pg, DBLA_BMultCntl, regVal);
      break;
    case  DBLA_ALUOperation:
    case  DBLA_FinalDivide:
      TxPage_GetDABReg(pg, DBLA_ALUCntl, &regVal);
      DAB_ALUCntl(command, value, &regVal);
      TxPage_SetDABReg(pg, DBLA_ALUCntl, regVal);
      break;
    case  DBLA_Alpha0ClampControl:
    case  DBLA_Alpha1ClampControl:
    case  DBLA_AlphaFracClampControl:
    case  DBLA_AlphaClampControl:
      TxPage_GetDABReg(pg, DBLA_SrcAlphaCntl, &regVal);
      DAB_SrcAlphaCntl(command, value, &regVal);
      TxPage_SetDABReg(pg, DBLA_SrcAlphaCntl, regVal);
      break;
    case  DBLA_DestAlphaConstSSB0:
    case  DBLA_DestAlphaConstSSB1:
      TxPage_GetDABReg(pg, DBLA_DestAlphaConst, &regVal);
      DAB_DestAlphaConst(command, value, &regVal);
      TxPage_SetDABReg(pg, DBLA_DestAlphaConst, regVal);
      break;
    case DBLA_EnableAttrs:
      TxPage_SetDABReg(pg, DBLA_UsrCntl, value);
      break;
    case DBLA_Discard:
      TxPage_SetDABReg(pg, DBLA_DiscardCntl, value);
      break;
    case  DBLA_RGBConstIn:
      TxPage_SetDABReg(pg, DBLA_ConstIn, value);
      break;
    case  DBLA_ZCompareControl:
      TxPage_SetDABReg(pg, DBLA_ZCntl, value);
      break;
    case  DBLA_AMultConstSSB0:
      TxPage_SetDABReg(pg, DBLA_AMConstSSB0, value);
      break;
    case  DBLA_AMultConstSSB1:
      TxPage_SetDABReg(pg, DBLA_AMConstSSB1, value);
      break;
    case  DBLA_BMultConstSSB0:
      TxPage_SetDABReg(pg, DBLA_BMConstSSB0, value);
      break;
    case  DBLA_BMultConstSSB1:
      TxPage_SetDABReg(pg, DBLA_BMConstSSB1, value);
      break;
    case  DBLA_DestAlphaSelect:
      TxPage_SetDABReg(pg, DBLA_DestAlphaCntl, value);
      break;
    case  DBLA_DitherMatrixA:
      TxPage_SetDABReg(pg, DBLA_DitherMatA, value);
      break;
    case  DBLA_DitherMatrixB:
      TxPage_SetDABReg(pg, DBLA_DitherMatB, value);
      break;
      /* Not Available in texture file format */
    case  DBLA_SrcPixels32Bit:
    case  DBLA_SrcBaseAddr:
    case  DBLA_SrcXStride:
    case  DBLA_SrcXOffset:
    case  DBLA_SrcYOffset:
      fprintf(stderr,"DAB TexBlend token %d(enumeration) Not Settable in SDF\n",command);
      break;
    default:
      fprintf(stderr,"#Unknown Command=%d Value=%d", command, value);
      break;
    }
}

void TxPage_TABInterpret(TxPage *pg, uint32 command, uint32 value)
{
  uint32 regVal;
  switch(command)
    {
    case TXA_MinFilter: case TXA_MagFilter: case TXA_InterFilter:
    case TXA_TextureEnable:
      TxPage_GetTABReg(pg, TXA_AddrCntl, &regVal);
      TAB_AddrCntl(command, value, &regVal);
      TxPage_SetTABReg(pg, TXA_AddrCntl, regVal);
      break;
    case TXA_FirstColor: case TXA_SecondColor: case TXA_ThirdColor:
    case TXA_FirstAlpha: case TXA_SecondAlpha: 
    case TXA_ColorOut: case TXA_AlphaOut: case TXA_BlendOp:
      TxPage_GetTABReg(pg, TXA_TABCntl, &regVal);
      TAB_TABCntl(command, value, &regVal);
      TxPage_SetTABReg(pg, TXA_TABCntl, regVal);
      break;
    case TXA_PipColorSelect: case TXA_PipAlphaSelect:
    case TXA_PipSSBSelect: case TXA_PipIndexOffset:
      TxPage_GetTABReg(pg, TXA_PIPCntl, &regVal);
      TAB_PIPCntl(command, value, &regVal);
      TxPage_SetTABReg(pg, TXA_PIPCntl, regVal);
      break;
    case TXA_PipConstSSB0: case TXA_PipConstSSB1:
    case TXA_BlendColorSSB0: case TXA_BlendColorSSB1:
      TxPage_ConstRegSet(pg, command, value);
      break;


    default:
      fprintf(stderr,"#Unknown Command=%d Value=%d", command, value);
      break;
    }
}

M2Err PageCLT_SetFormat(CltSnippet *snip, M2TXFormat format)
{
  uint8 cDepth, aDepth;
  bool  hasColor, hasAlpha, hasSSB, isLiteral;
  uint32 value, size;
  M2Err err;

  value = 0;

  isLiteral = M2TXFormat_GetFIsLiteral(format);
  value |= CLT_Bits(TXTEXPTYPE, ISLITERAL, isLiteral);
  hasColor = M2TXFormat_GetFHasColor(format);
  if (hasColor)
    {
      cDepth = M2TXFormat_GetCDepth(format);
      value |= CLT_Bits(TXTEXPTYPE, HASCOLOR, 1) |
	CLT_Bits(TXTEXPTYPE, CDEPTH, cDepth);
    }
  hasAlpha = M2TXFormat_GetFHasAlpha(format);
  if (hasAlpha)
    {
      aDepth = M2TXFormat_GetADepth(format);
      value |= CLT_Bits(TXTEXPTYPE, HASALPHA, 1) |
	CLT_Bits(TXTEXPTYPE, ADEPTH, aDepth);
    }
  hasSSB = M2TXFormat_GetFHasSSB(format);
  if (hasSSB)
    {
      value |= CLT_Bits(TXTEXPTYPE, HASSSB, 1);
    }

  size = snip->size;
  err = PageCLT_Extend(snip, 2);
  if (err != M2E_NoErr)
    return(err);

  snip->data[size] = CLT_WriteRegistersHeader(TXTEXPTYPE, 1);
  size++;
  snip->data[size] = value;
  size++;
  snip->size = size;
}

M2Err PageCLT_SetUV(CltSnippet *snip, M2TXPgHeader *pHeader, TxPage *txPage, int index)
{
  uint32 xSize, ySize, numLOD;
  uint32 max, mask, value, size;
  M2Err err;
  
  xSize = pHeader->MinXSize;
  ySize = pHeader->MinYSize;
  numLOD = pHeader->NumLOD;
  value = 0;
  
  max = mask = 0;
  size = snip->size;
  err = PageCLT_Extend(snip, 3);
  if (err != M2E_NoErr)
    return(err);
  
  snip->data[size] = CLT_WriteRegistersHeader(TXTUVMAX, 2);
  size++;
  
  if ((xSize<1) || (ySize<1) || (numLOD<1))
    {
      txPage->UVScale[2*index] = 0;
      txPage->UVScale[2*index+1] = 0;
      max = CLT_Bits(TXTUVMAX, UMAX,  0);
      mask = CLT_Bits(TXTUVMASK, UMASK,  0x03FF);      
      
      max |= CLT_Bits(TXTUVMAX, VMAX,  0);
      mask |= CLT_Bits(TXTUVMASK, VMASK,  0x03FF);      
    }
  else
    {
      txPage->UVScale[2*index] = xSize<<(numLOD-1);     /* U Scaling */
      txPage->UVScale[2*index+1] = ySize<<(numLOD-1);     /* V Scaling */
      
      if (pHeader->PgFlags & M2PG_FLAGS_XWrapMode)
	{  /* Tile */
	  max = CLT_Bits(TXTUVMAX, UMAX, (0x3FF>>(numLOD-1)));
	  mask = CLT_Bits(TXTUVMASK, UMASK,  ((xSize<<(numLOD-1))-1));
	}
      else
	{ /* Clamp */
	  max = CLT_Bits(TXTUVMAX, UMAX,  xSize-1);
	  mask = CLT_Bits(TXTUVMASK, UMASK,  0x03FF);      
	}
      
      if (pHeader->PgFlags & M2PG_FLAGS_YWrapMode)
	{  /* Tile */
	  max |= CLT_Bits(TXTUVMAX, VMAX, (0x3FF>>(numLOD-1)));
	  mask |= CLT_Bits(TXTUVMASK, VMASK,  ((ySize<<(numLOD-1))-1));
	}
      else
	{ /* Clamp */
	  max |= CLT_Bits(TXTUVMAX, VMAX,  ySize-1);
	  mask |= CLT_Bits(TXTUVMASK, VMASK,  0x03FF);      
	} 
    }

  snip->data[size] = max;
  size++;
  snip->data[size] = mask;
  size++;
  snip->size = size;
  return(M2E_NoErr);
}

M2Err PageNames_Entry(int tokenType, PageNames *pn)
{
  M2Err err;
  int newToken;
  int curName;
  
  newToken = tokenType;
  /*
  while ((tokenType = lws_get_token()) != T_LBRACE)
    ;
    */

  while (strcasecmp(token, "}"))
    {
      if(!strcasecmp(token, "fileName"))
	{
	  lws_read_string();
	  err = PageNames_Extend(pn, 1);
	  if (err != M2E_NoErr)
	    {
	      fprintf(stderr,"ERROR: Errors occured in PageName_Entry\n");
	      return(err);
	    }
	  curName = pn->NumNames;
	  pn->SubNames[curName] = (char *)calloc(strlen(token)+4,1);
	  if (pn->SubNames[curName] == NULL)
	    return(M2E_NoMem);
	  strcpy(pn->SubNames[curName], token);
	  pn->NumNames = curName+1;
	}
      do
	{
	  newToken = lws_get_token();
	  if (newToken == T_RBRACE)
	    return (M2E_NoErr);
	} while (newToken != T_KEYWORD);
    }

  return(M2E_NoErr);
}


M2Err PageCLT_CreatePIPLoad(CltSnippet *snip, TxPage *txPage, bool hasPIP)
{
  uint32     size, regVal;
  bool       truncate = FALSE;
  M2TXPIP *pip;

  pip = &(txPage->Tex->PIP);
  PageCLT_Init(snip);
	    
  PageCLT_Extend(snip, 2);
  snip->data[0] = CLT_WriteRegistersHeader(TXTLDSRCADDR,1);
  snip->data[1] = 0;
  snip->size = 2;

  size = snip->size;
  PageCLT_Extend(snip, 2);
  snip->data[size] = CLT_WriteRegistersHeader(TXTCOUNT,1); size++;
  if (hasPIP && (pip->NumColors>0))
    {
      snip->data[size] = pip->NumColors*4; 
    }
  else
    {
      snip->data[size] = 0; 
      truncate = TRUE;
    }
  size++;
  snip->size = size;

  if (!truncate)
    {
      PageCLT_Extend(snip, 2);
      snip->data[size] = CLT_WriteRegistersHeader(TXTLDDSTBASE,1); size++;
      snip->data[size] = 0x46000; size++;
      snip->size = size;
      
      PageCLT_Extend(snip, 2);
      snip->data[size] = CLT_WriteRegistersHeader(TXTLDCNTL,1); size++;
      regVal = CLT_SetConst(TXTLDCNTL, LOADMODE, PIP);
      snip->data[size] = regVal; size++;
      snip->size = size;
      
      PageCLT_Extend(snip, 2);
      snip->data[size] = CLT_WriteRegistersHeader(DCNTL, 1); size++;
      snip->data[size] = CLT_Bits(DCNTL, TLD, 1); size++;
      snip->size = size;
    }

  return(M2E_NoErr);
}

/* This is for uncompressed textures */
M2Err PageCLT_CreatePageLoad(TxPage *txPage, uint32 totalBytes)
{
  uint32     size, regVal;
  M2TXHeader *header;
  CltSnippet *snip;
  
  header = &(txPage->Tex->Header);

  /* Create Page Load CLT */
  PageCLT_Init(&(txPage->CLT));
  snip = &(txPage->CLT);
	    
  /* Header that does the SYNC */
  snip->data[0] = CLT_WriteRegistersHeader(DCNTL, 1);
  snip->data[1] = CLT_Bits(DCNTL, SYNC, 1);
  txPage->CLT.size = 2;
  
  size = snip->size;
  PageCLT_Extend(&(txPage->CLT), 2);
  snip->data[size] = CLT_WriteRegistersHeader(TXTLDSRCADDR,1); size++;
  snip->data[size] = 0; size++;
  snip->size = size;

  PageCLT_Extend(&(txPage->CLT), 2);
  snip->data[size] = CLT_WriteRegistersHeader(TXTCOUNT,1); size++;    /* Bytes to load */
  snip->data[size] = totalBytes; size++;
  snip->size = size;

  if (totalBytes > 0)
  {
    PageCLT_Extend(&(txPage->CLT), 2);
    snip->data[size] = CLT_WriteRegistersHeader(TXTLODBASE0,1); size++;
    snip->data[size] = 0; size++;
    snip->size = size;
    
    PageCLT_Extend(&(txPage->CLT), 4);
    snip->data[size] = CLT_WriteRegistersHeader(TXTLDCNTL,1); size++;
    regVal = CLT_SetConst(TXTLDCNTL, LOADMODE, MMDMA);
    snip->data[size] = regVal; size++;
    snip->data[size] = CLT_WriteRegistersHeader(DCNTL, 1); size++;
    snip->data[size] = CLT_Bits(DCNTL, TLD, 1); size++;
    snip->size = size;
  }
    
  return (M2E_NoErr);
}


long M2TXHeader_ComputeLODSize(M2TXHeader *header, uint16 lod)
{
  uint8 cDepth, aDepth, ssbDepth, depth, numLOD;
  bool isLiteral, hasColor, hasAlpha, hasSSB;
  long size;
  uint16 xSize, ySize;
  long curXSize, curYSize;

  M2TXHeader_GetFIsLiteral(header, &isLiteral);
  M2TXHeader_GetFHasColor(header, &hasColor);
  if (hasColor)
    M2TXHeader_GetCDepth(header, &cDepth);
  else
    cDepth = 0;
  M2TXHeader_GetFHasAlpha(header, &hasAlpha);
  if (hasAlpha)
    M2TXHeader_GetADepth(header, &aDepth);
  else
    aDepth = 0;
  M2TXHeader_GetFHasSSB(header, &hasSSB);
  if (hasSSB)
    ssbDepth = 1;
  else
    ssbDepth = 0;

  M2TXHeader_GetNumLOD(header, &numLOD);
  M2TXHeader_GetMinXSize(header, &xSize);
  M2TXHeader_GetMinYSize(header, &ySize);

  curXSize = xSize * (1<<(numLOD-lod-1));
  curYSize = ySize * (1<<(numLOD-lod-1));

  if (isLiteral)
    depth = aDepth + 3*cDepth + ssbDepth;
  else
    depth = aDepth + cDepth + ssbDepth;

  size = (long)ceil(curXSize*curYSize*depth);
  if (size%32)
    size = (size/32)*32+32;
  /* Compute Bytes */

  size = size/8;
  return(size);
}

M2Err PageCLT_CreateRef(TxPage *subTxPage, TxPage *txPage, int index)
{
  uint32     size, regVal, offset, temp, lod;
  bool       result;
  M2TXHeader *subHeader;
  CltSnippet *snip;

  subHeader = &(subTxPage->Tex->Header);

  /* Create Tex Ref CLT */
  PageCLT_Init(&(subTxPage->CLT));
  snip = &(subTxPage->CLT);
  
  /* Header that does the SYNC */
  snip->data[0] = CLT_WriteRegistersHeader(DCNTL, 1);
  snip->data[1] = CLT_Bits(DCNTL, SYNC, 1);
  snip->size = 2;
      
  size = snip->size;
  
  /* UVMask and UVMax */
  PageCLT_SetUV(snip, &(subTxPage->PgHeader), txPage, index);
  size = snip->size;
  
  PageCLT_Extend(snip, 7);
  /* AddrCNTL- LODMAX, FILTER MODES, Texture Enable at offset 6 */
  TxPage_GetTABReg(subTxPage, TXA_AddrCntl, &regVal);
  temp = regVal & (~CLT_Mask(TXTADDRCNTL, LODMAX));
  if(subTxPage->PgHeader.NumLOD<1)
    temp |= CLT_Bits(TXTADDRCNTL, LODMAX, 1);
  else
    temp |= CLT_Bits(TXTADDRCNTL, LODMAX, (subTxPage->PgHeader.NumLOD-1));

  regVal = temp;
  TAB_AddrCntl(TXA_TextureEnable, 1, &regVal);
  TxPage_SetTABReg(subTxPage, TXA_AddrCntl, regVal);
  snip->data[size] = CLT_WriteRegistersHeader(TXTADDRCNTL,1); size++;
  snip->data[size] = regVal; size++;
  
  /* 4 LOD offsets into TRAM starting at CLT offset 8 */
  snip->data[size] = CLT_WriteRegistersHeader(TXTLODBASE0,4); size++;
  offset = subTxPage->PgHeader.Offset; 
  snip->data[size] = offset; size++;
  for (lod=1; (lod<4)&&(lod<subHeader->NumLOD); lod++)
    {
      offset += M2TXHeader_ComputeLODSize(subHeader, lod-1);
      /*
	offset += subHeader->LODDataLength[lod-1];
      */
      snip->data[size] = offset; size++;
    }
  for (; lod<4; lod++)   /* Just in case NumLOD<4 */
    size++;
  
  snip->size = size;
  
  PageCLT_SetFormat(snip, subHeader->TexFormat);
  size = snip->size;
  
  result = TxPage_GetTABReg(subTxPage, TXA_PIPCntl, &regVal);
  if (result)
    {
      PageCLT_Extend(snip, 2);
      snip->data[size] = CLT_WriteRegistersHeader(TXTPIPCNTL,1); size++;
      snip->data[size] = regVal; size++;
      snip->size = size;
    }
  
  result = TxPage_GetTABReg(subTxPage, TXA_TABCntl, &regVal);
  if (result)
    {
      PageCLT_Extend(snip, 2);
      snip->data[size] = CLT_WriteRegistersHeader(TXTTABCNTL,1); size++;
      snip->data[size] = regVal; size++;
      snip->size = size;
    }
  
  result = TxPage_GetTABReg(subTxPage, TXA_PIPConstSSB0, &regVal);
  if (result)
    {
      PageCLT_Extend(snip, 2);
      snip->data[size] = CLT_WriteRegistersHeader(TXTCONST0,1); size++;
      snip->data[size] = regVal; size++;
      snip->size = size;
    }
  
  result = TxPage_GetTABReg(subTxPage, TXA_PIPConstSSB1, &regVal);
  if (result)
    {
      PageCLT_Extend(snip, 2);
      snip->data[size] = CLT_WriteRegistersHeader(TXTCONST1,1); size++;
      snip->data[size] = regVal; size++;
      snip->size = size;
    }
  
  result = TxPage_GetTABReg(subTxPage, TXA_TABConst0, &regVal);
  if (result)
    {
      PageCLT_Extend(snip, 2);
      snip->data[size] = CLT_WriteRegistersHeader(TXTCONST2,1); size++;
      snip->data[size] = regVal; size++;
      snip->size = size;
    }
  
  result = TxPage_GetTABReg(subTxPage, TXA_TABConst1, &regVal);
  if (result)
    {
      PageCLT_Extend(snip, 2);
      snip->data[size] = CLT_WriteRegistersHeader(TXTCONST3,1); size++;
        snip->data[size] = regVal; size++;
	snip->size = size;
    }

  result = TxPage_GetDABReg(subTxPage, DBLA_UsrCntl, &regVal);
  if (result)
    {
      PageCLT_Extend(snip, 2);
      snip->data[size] = CLT_WriteRegistersHeader(DBUSERCONTROL,1); size++;
      snip->data[size] = regVal; size++;
      snip->size = size;
    }

  result = TxPage_GetDABReg(subTxPage, DBLA_DiscardCntl, &regVal);
  if (result)
    {
      PageCLT_Extend(snip, 2);
      snip->data[size] = CLT_WriteRegistersHeader(DBDISCARDCONTROL,1); size++;
      snip->data[size] = regVal; size++;
      snip->size = size;
    }

  result = TxPage_GetDABReg(subTxPage, DBLA_XWinClip, &regVal);
  if (result)
    {
      PageCLT_Extend(snip, 2);
      snip->data[size] = CLT_WriteRegistersHeader(DBXWINCLIP,1); size++;
      snip->data[size] = regVal; size++;
      snip->size = size;
    }

  result = TxPage_GetDABReg(subTxPage, DBLA_YWinClip, &regVal);
  if (result)
    {
      PageCLT_Extend(snip, 2);
      snip->data[size] = CLT_WriteRegistersHeader(DBYWINCLIP,1); size++;
      snip->data[size] = regVal; size++;
      snip->size = size;
    }

  result = TxPage_GetDABReg(subTxPage, DBLA_ZCntl, &regVal);
  if (result)
    {
      PageCLT_Extend(snip, 2);
      snip->data[size] = CLT_WriteRegistersHeader(DBZCNTL,1); size++;
      snip->data[size] = regVal; size++;
      snip->size = size;
    }

  result = TxPage_GetDABReg(subTxPage, DBLA_ZOffset, &regVal);
  if (result)
    {
      PageCLT_Extend(snip, 2);
      snip->data[size] = CLT_WriteRegistersHeader(DBZOFFSET,1); size++;
      snip->data[size] = regVal; size++;
      snip->size = size;
    }

  result = TxPage_GetDABReg(subTxPage, DBLA_ZOffset, &regVal);
  if (result)
    {
      PageCLT_Extend(snip, 2);
      snip->data[size] = CLT_WriteRegistersHeader(DBZOFFSET,1); size++;
      snip->data[size] = regVal; size++;
      snip->size = size;
    }

  result = TxPage_GetDABReg(subTxPage, DBLA_SSBDSBCntl, &regVal);
  if (result)
    {
      PageCLT_Extend(snip, 2);
      snip->data[size] = CLT_WriteRegistersHeader(DBSSBDSBCNTL,1); size++;
      snip->data[size] = regVal; size++;
      snip->size = size;
    }

  result = TxPage_GetDABReg(subTxPage, DBLA_ConstIn, &regVal);
  if (result)
    {
      PageCLT_Extend(snip, 2);
      snip->data[size] = CLT_WriteRegistersHeader(DBCONSTIN,1); size++;
      snip->data[size] = regVal; size++;
      snip->size = size;
    }

  result = TxPage_GetDABReg(subTxPage, DBLA_AMultCntl, &regVal);
  if (result)
    {
      PageCLT_Extend(snip, 2);
      snip->data[size] = CLT_WriteRegistersHeader(DBAMULTCNTL,1); size++;
      snip->data[size] = regVal; size++;
      snip->size = size;
    }

  result = TxPage_GetDABReg(subTxPage, DBLA_AMConstSSB0, &regVal);
  if (result)
    {
      PageCLT_Extend(snip, 2);
      snip->data[size] = CLT_WriteRegistersHeader(DBAMULTCONSTSSB0,1); size++;
      snip->data[size] = regVal; size++;
      snip->size = size;
    }

  result = TxPage_GetDABReg(subTxPage, DBLA_AMConstSSB1, &regVal);
  if (result)
    {
      PageCLT_Extend(snip, 2);
      snip->data[size] = CLT_WriteRegistersHeader(DBAMULTCONSTSSB1,1); size++;
      snip->data[size] = regVal; size++;
      snip->size = size;
    }

  result = TxPage_GetDABReg(subTxPage, DBLA_BMultCntl, &regVal);
  if (result)
    {
      PageCLT_Extend(snip, 2);
      snip->data[size] = CLT_WriteRegistersHeader(DBBMULTCNTL,1); size++;
      snip->data[size] = regVal; size++;
      snip->size = size;
    }

  result = TxPage_GetDABReg(subTxPage, DBLA_BMConstSSB0, &regVal);
  if (result)
    {
      PageCLT_Extend(snip, 2);
      snip->data[size] = CLT_WriteRegistersHeader(DBBMULTCONSTSSB0,1); size++;
      snip->data[size] = regVal; size++;
      snip->size = size;
    }

  result = TxPage_GetDABReg(subTxPage, DBLA_BMConstSSB1, &regVal);
  if (result)
    {
      PageCLT_Extend(snip, 2);
      snip->data[size] = CLT_WriteRegistersHeader(DBBMULTCONSTSSB1,1); size++;
      snip->data[size] = regVal; size++;
      snip->size = size;
    }

  result = TxPage_GetDABReg(subTxPage, DBLA_ALUCntl, &regVal);
  if (result)
    {
      PageCLT_Extend(snip, 2);
      snip->data[size] = CLT_WriteRegistersHeader(DBALUCNTL,1); size++;
      snip->data[size] = regVal; size++;
      snip->size = size;
    }

  result = TxPage_GetDABReg(subTxPage, DBLA_SrcAlphaCntl, &regVal);
  if (result)
    {
      PageCLT_Extend(snip, 2);
      snip->data[size] = CLT_WriteRegistersHeader(DBSRCALPHACNTL,1); size++;
      snip->data[size] = regVal; size++;
      snip->size = size;
    }

  result = TxPage_GetDABReg(subTxPage, DBLA_DestAlphaCntl, &regVal);
  if (result)
    {
      PageCLT_Extend(snip, 2);
      snip->data[size] = CLT_WriteRegistersHeader(DBDESTALPHACNTL,1); size++;
      snip->data[size] = regVal; size++;
      snip->size = size;
    }

  result = TxPage_GetDABReg(subTxPage, DBLA_DestAlphaConst, &regVal);
  if (result)
    {
      PageCLT_Extend(snip, 2);
      snip->data[size] = CLT_WriteRegistersHeader(DBDESTALPHACONST,1); size++;
      snip->data[size] = regVal; size++;
      snip->size = size;
    }

  result = TxPage_GetDABReg(subTxPage, DBLA_DitherMatA, &regVal);
  if (result)
    {
      PageCLT_Extend(snip, 2);
      snip->data[size] = CLT_WriteRegistersHeader(DBDITHERMATRIXA,1); size++;
      snip->data[size] = regVal; size++;
      snip->size = size;
    }

  result = TxPage_GetDABReg(subTxPage, DBLA_DitherMatB, &regVal);
  if (result)
    {
      PageCLT_Extend(snip, 2);
      snip->data[size] = CLT_WriteRegistersHeader(DBDITHERMATRIXB,1); size++;
      snip->data[size] = regVal; size++;
      snip->size = size;
    }
  return (M2E_NoErr);
}


M2Err Texture_Entry(int tokenType, SDFTex *tb);

M2Err TexPageArray_ReadFile(char *fileIn, SDFTex **myTex, int *numTex, int *cTex,
			    PageNames **myPages, int *numPages, int *cPage, bool *gotTPA)
{
  LWS lws;
  ByteStream*		stream = NULL;
  int  tokenType;
  int nTextures = TEX_BUF_SIZE;
  int nPages    = PAGE_BUF_SIZE;
  SDFTex *textures, *temp;
  int curTex = 0;
  int curPage = 0;
  bool gotTexArray, gotTexPageArray;
  PageNames *pageNames, *pgTmp;
  
  *myTex = NULL;
  *myPages = NULL;
  gotTexArray = gotTexPageArray = FALSE;
  textures = (SDFTex *)qMemClearPtr(nTextures, sizeof(SDFTex));
  if (textures == NULL)
    {
      fprintf(stderr,"ERROR:Out of memory in TexArray_ReadFile!\n");
      return(M2E_NoMem);
    }

  pageNames = (PageNames *)qMemClearPtr(nPages, sizeof(PageNames));
  if (pageNames == NULL)
    {
      fprintf(stderr,"ERROR:Out of memory in TexPageArray_ReadFile!\n");
      return(M2E_NoMem);
    }

  strcpy(lws.FileName, fileIn);
  stream = K9_OpenByteStream(lws.FileName, Stream_Read, 0);
  if (stream == NULL)
    {
      return (M2E_BadFile);
    }
  lws.Stream = stream;
  lws.Parent = cur_lws;
  cur_lws = &lws;

  while ((tokenType = lws_get_token()) != T_END)
    {
      switch (tokenType)
	{
	case T_KEYWORD:
	  if ((!strcasecmp(token, "TexArray")) && (!gotTexArray))     
	    {
	      if (lws_get_token() != T_KEYWORD)
		{
		  fprintf(stderr,"ERROR:Name expected. Got \"%s\" at line %d\n", token,
			  cur_lws->Lines);
		  return(M2E_BadFile);
		}
	      
	      if (!lws_check_token(T_LBRACE))
		return(M2E_BadFile);
	      while ((tokenType = lws_get_token()) != T_RBRACE)
		{
		  SDFTex_Init(&(textures[curTex]));
		  Texture_Entry(tokenType, &(textures[curTex]));

		  curTex++;
		  if (curTex >= (nTextures-1))
		    {
		      nTextures += TEX_BUF_SIZE;
		      temp = qMemResizePtr(textures, nTextures*sizeof(SDFTex));
		      if (temp == NULL)
			{
			  fprintf(stderr,"ERROR:Out of memory in TexArray_ReadFile!\n");
			  return(M2E_NoMem);
			}
		      textures = temp;
		    }
		}
	      gotTexArray = TRUE;
	    }
	  else if ((!strcasecmp(token, "TexPageArray")) && (!gotTexPageArray))
	    {	      
	      if (!lws_check_token(T_LBRACE))
		{
		  fprintf(stderr,"ERROR: '{' expected. Got \"%s\" at line %d\n",
			  token, cur_lws->Lines);
		  return(M2E_BadFile);
		}
	      while ((tokenType = lws_get_token()) != T_RBRACE)
		{
		  PageNames_Init(&(pageNames[curPage]));
		  PageNames_Entry(tokenType, &(pageNames[curPage]));
		  curPage++;
		  if (curPage >= (nPages-1))
		    {
		      nPages += PAGE_BUF_SIZE;
		      pgTmp = qMemResizePtr(pageNames, nPages*sizeof(PageNames));
		      if (temp == NULL)
			{
			  fprintf(stderr,"ERROR:Out of memory in TexPageArray_ReadFile!\n");
			  return(M2E_NoMem);
			}
		      pageNames = pgTmp;
		    }
		}
	      gotTexPageArray = TRUE;
	    }
	  break;
	default:
	  break;
	}
    }

  *myTex    = textures;
  *myPages =  pageNames;
  *numTex   = nTextures;
  *numPages = nPages;
  *cPage    = curPage;
  *cTex     = curTex;
  *gotTPA   = gotTexPageArray;
  return(M2E_NoErr);
}










