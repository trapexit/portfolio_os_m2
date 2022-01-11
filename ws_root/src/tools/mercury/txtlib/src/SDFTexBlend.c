/*
	File:		SDFTexBlend.c

	Contains:	Handles TexBlend manipulation

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
#include "M2TXTypes.h"
#include "M2TXattr.h"
#include "SDFTexBlend.h"
#include "qmem.h"
#include "clt.h"
#include "clttxdblend.h"

#include "LWSURF.h"
#include "LWToken.h"
#include <assert.h>
#include "os.h"
#include "lws.i"
#include "lws.h"

extern LWS  *cur_lws;
extern int  tokenType;		/* type of input token */
extern char token[256];			/* input token buffer */

extern int CommentLevel;

#ifdef applec 
 int strcasecmp(char *s, char *t);
#endif

void SDFTex_Init(SDFTex *tb)
{
  int i;

  tb->FileName = NULL;
  tb->OrigName = NULL;
  tb->DABSetHi = (uint32)0;
  tb->DABSetLo = (uint32)0;
  tb->TABSet = (uint32)0;
  for (i=0; i<TAB_SIZE; i++)
    tb->TAB[i] = 0;
  for (i=0; i<DAB_SIZE; i++)
    tb->DAB[i] = 0;
  tb->XClampPos = -1;
  tb->YClampPos = -1;
  tb->XTile = tb->YTile = -1;
  tb->PageIndex[0] = tb->PageIndex[1] = -1;
  tb->EnvMap = FALSE;
}

void SDFTex_Free(SDFTex *tb)
{
  if (tb->FileName != NULL)
    qMemReleasePtr(tb->FileName);
  if (tb->OrigName != NULL)
    qMemReleasePtr(tb->OrigName);
}

#define MkTABMask(command)  (uint32)(1 << command)
#define MkDABMask(command)  (uint32)(1 << (command%32))

void SDFTex_SetTAB(SDFTex *tb, uint32 command, uint32 value)
{
  tb->TABSet |= MkTABMask(command);
  tb->TAB[command] = value;
}

bool SDFTex_GetTAB(SDFTex *tb, uint32 command, uint32 *value)
{
  if ((MkTABMask(command))&(tb->TABSet))
    {
      *value = tb->TAB[command];
      return(TRUE);
    }
  
  *value = 0;
  return(FALSE);
}

void SDFTex_SetDAB(SDFTex *tb, uint32 command, uint32 value)
{
  if (command < 32)
    tb->DABSetLo |= MkDABMask(command);
  else
    tb->DABSetHi |= MkDABMask(command);
  tb->DAB[command] = value;
}

bool SDFTex_GetDAB(SDFTex *tb, uint32 command, uint32 *value)
{
  if (command <32)
    {
      if ((MkDABMask(command))&(tb->DABSetLo))
	{
	  *value = tb->DAB[command];
	  return(TRUE);
	}
    }
  else if ((MkDABMask(command))&(tb->DABSetHi))
    {
      *value = tb->DAB[command];
      return(TRUE);
    }

  *value = 0;
  return(FALSE);
}

bool SDFTex_Compare(SDFTex *tb1, SDFTex *tb2)
{
  int i;
  uint32 val1, val2;
  bool   set1, set2;

  if ((tb1->FileName != NULL) && (tb2->FileName != NULL))
    {
      if (strcasecmp(tb1->FileName,tb2->FileName))
	return(FALSE);
    }
  else
    {
      if (tb1->FileName != tb2->FileName)
	return(FALSE);
    }
  /*
  if ((tb1->OrigName != NULL) && (tb2->OrigName != NULL))
    {
      if (strcasecmp(tb1->OrigName,tb2->OrigName))
	return(FALSE);
    }
  else
    {
      if (tb1->OrigName != tb2->OrigName)
	return(FALSE);
    }
    */
  if (tb1->TABSet != tb2->TABSet)
    return(FALSE);
  
  for(i=0; i<TAB_SIZE; i++)
    {
      set1 = SDFTex_GetTAB(tb1, i, &val1);
      set2 = SDFTex_GetTAB(tb2, i, &val2);

      if (set1 != set2)
	return(FALSE);
      if (set1 && set2)
	{
	  if (val1 != val2)
	    return(FALSE);
	}
    }

  for(i=0; i<DAB_SIZE; i++)
    {
      set1 = SDFTex_GetDAB(tb1, i, &val1);
      set2 = SDFTex_GetDAB(tb2, i, &val2);

      if (set1 != set2)
	return(FALSE);
      if (set1 && set2)
	{
	  if (val1 != val2)
	    return(FALSE);
	}
    }

  if (tb1->PageIndex[0] != tb2->PageIndex[0])
    return(FALSE);

  if (tb1->PageIndex[1] != tb2->PageIndex[1])
    return(FALSE);

  if (tb1->YTile != tb2->YTile)
    {
      if ((tb1->XTile == TRUE) && (tb1->YTile == FALSE) && (tb2->YTile==TRUE))
	return(FALSE);
      if ((tb2->XTile == TRUE) && (tb2->YTile == FALSE) && (tb1->YTile==TRUE))
	return(FALSE);
      if (tb1->EnvMap == TRUE)
	if (!((tb2->YTile == FALSE) && (tb2->XTile == TRUE)))
	  return(FALSE);
      if (tb2->EnvMap == TRUE)
	if (!((tb1->YTile == FALSE) && (tb1->XTile == TRUE)))
	  return(FALSE);
      if ((tb1->YTile==TRUE) || (tb2->YTile==TRUE))
	{
	  tb1->YTile = tb2->YTile = TRUE;
	}
      if (tb1->YTile==-1)
	tb1->YTile = tb2->YTile;
      else if (tb2->YTile==-1)
	tb2->YTile = tb1->YTile;
    }

  if (tb1->XTile != tb2->XTile)
    {
      if ((tb1->XTile==TRUE) || (tb2->XTile==TRUE))
	{
	  tb1->XTile = tb2->XTile = TRUE;
	}
      if (tb1->XTile==-1)
	tb1->XTile = tb2->XTile;
      else if (tb2->XTile==-1)
	tb2->XTile = tb1->XTile;
    }

  return(TRUE); 
}

void SDFTex_Copy(SDFTex *tb1, SDFTex *tb2)
{
  int i;
  
  tb1->FileName = tb2->FileName;
  tb1->OrigName = tb2->OrigName;
  tb1->TABSet = tb2->TABSet;
  tb1->DABSetHi = tb2->DABSetHi;
  tb1->DABSetLo = tb2->DABSetLo;
  tb1->XTile = tb2->XTile;
  tb1->YTile = tb2->YTile;
  tb1->EnvMap = tb2->EnvMap;

  for(i=0; i<TAB_SIZE; i++)
      tb1->TAB[i] = tb2->TAB[i];
  for(i=0; i<DAB_SIZE; i++)
      tb1->DAB[i] = tb2->DAB[i];

  tb1->PageIndex[0] = tb2->PageIndex[0];
  tb1->PageIndex[1] = tb2->PageIndex[1];
}


static void TAB_FilterPrint(FILE *fPtr, uint32 value)
{
  switch(value)
    {
    case TX_Nearest:
      fprintf(fPtr,"Nearest ");
      break;
    case TX_Linear:
      fprintf(fPtr,"Linear ");
      break;
    case TX_Bilinear:
      fprintf(fPtr,"Bilinear ");
      break;
    case TX_QuasiTrilinear:
      fprintf(fPtr,"QuasiTrilinear ");
      break;
    default:
      fprintf(fPtr,"Nearest ");
      break;
    }
}

static void TAB_BlendOpPrint(FILE *fPtr, uint32 value)
{
  switch(value)
    {
    case TX_BlendOpLerp:
      fprintf(fPtr,"Lerp ");
      break;
    case TX_BlendOpMult:
      fprintf(fPtr,"Mult ");
      break;
    default:
      fprintf(fPtr,"Mult ");
      break;
    }
}

static void TAB_PipSelPrint(FILE *fPtr, uint32 value)
{
  switch(value)
    {
    case TX_PipSelectConst:
      fprintf(fPtr,"Const ");
      break;
    case TX_PipSelectTexture:
      fprintf(fPtr,"Texture ");
      break;
    case TX_PipSelectColorTable:
      fprintf(fPtr,"ColorTable ");
      break;
    default:
      fprintf(fPtr,"Const ");
      break;
    }
}

static void TAB_ConstPrint(FILE *fPtr, uint32 value)
{
  fprintf(fPtr,"0x%x ", value);
}

static void TAB_ValuePrint(FILE *fPtr, uint32 value)
{
  fprintf(fPtr,"%d ", value);
}

static void TAB_ColorSrcPrint(FILE *fPtr, uint32 value)
{
  switch(value)
    {
    case TX_ColorSelectPrimAlpha:
      fprintf(fPtr,"PrimAlpha ");
      break;
    case TX_ColorSelectPrimColor:
      fprintf(fPtr,"PrimColor ");
      break;
    case TX_ColorSelectTexAlpha:
      fprintf(fPtr,"TexAlpha ");
      break;
    case TX_ColorSelectTexColor:
      fprintf(fPtr,"TexColor ");
      break;
    case TX_ColorSelectConstAlpha:
      fprintf(fPtr,"ConstAlpha ");
      break;
    case TX_ColorSelectConstColor:
      fprintf(fPtr,"ConstColor ");
      break;
    default:
      fprintf(fPtr,"PrimColor ");
      break;
    }
}

static void TAB_AlphaSrcPrint(FILE *fPtr, uint32 value)
{
  switch(value)
    {
    case TX_AlphaSelectPrimAlpha:
      fprintf(fPtr,"PrimAlpha ");
      break;
    case TX_AlphaSelectTexAlpha:
      fprintf(fPtr,"TexAlpha ");
      break;
    case TX_AlphaSelectConstAlpha:
      fprintf(fPtr,"ConstAlpha ");
      break;
    default:
      fprintf(fPtr,"PrimAlpha ");
      break;
    }
}

static void TAB_OutPrint(FILE *fPtr, uint32 value)
{
  switch(value)
    {
    case TX_BlendOutSelectPrim:
      fprintf(fPtr,"Prim ");
      break;
    case TX_BlendOutSelectTex:
      fprintf(fPtr,"Texture ");
      break;
    case TX_BlendOutSelectBlend:
      fprintf(fPtr,"Blend ");
      break;
    default:
      fprintf(fPtr,"Prim ");
      break;
    }
}


static void DAB_Print(FILE *fPtr, uint32 command, uint32 value)
{
  switch(command)
    {
    case DBLA_EnableAttrs:
      fprintf(fPtr,"dblEnableAttrs ");
      break;
    case DBLA_ZBuffEnable:
      fprintf(fPtr,"dblZBuffEnable ");
      break;
    case DBLA_ZBuffOutEnable:
      fprintf(fPtr,"dblZBuffOutEnable ");
      break;
    case DBLA_WinClipInEnable:
      fprintf(fPtr,"dblWinClipInEnable ");
      break;
    case DBLA_WinClipOutEnable:
      fprintf(fPtr,"dblWinClipOutEnable ");
      break;
    case DBLA_BlendEnable:
      fprintf(fPtr,"dblBlendEnable ");
      break;
    case DBLA_SrcInputEnable:
      fprintf(fPtr,"dblScrInputEnable ");
      break;
    case DBLA_DitherEnable:
      fprintf(fPtr,"dblDitherEnable ");
      break;
    case DBLA_RGBADestOut:
      fprintf(fPtr,"dblRGBADestOut ");
      break;
    case DBLA_Discard:
      fprintf(fPtr, "dblDiscard ");
      break;
    case DBLA_DiscardZClipped:
      fprintf(fPtr, "dblDiscardZClipped ");
      break;
    case DBLA_DiscardSSB0:
      fprintf(fPtr, "dblDiscardSSB0 ");
      break;
    case DBLA_DiscardRGB0:
      fprintf(fPtr, "dblDiscardRGB0 ");
      break;
    case DBLA_DiscardAlpha0:
      fprintf(fPtr, "dblDiscardAlpha0 ");
      break;
    case  DBLA_RGBConstIn:
      fprintf(fPtr, "dblRGBConstIn ");
      break;
    case  DBLA_XWinClipMin:
      fprintf(fPtr, "dblXWinClipMin ");
      break;
    case  DBLA_XWinClipMax:
      fprintf(fPtr, "dblXWinClipMax ");
      break;
    case  DBLA_YWinClipMin:
      fprintf(fPtr,"dblYWinClipMin ");
      break;
    case  DBLA_YWinClipMax:
      fprintf(fPtr,"dblYWinClipMax ");
      break;
    case  DBLA_ZCompareControl:
      fprintf(fPtr,"dblZCompareControl ");
      break;
    case  DBLA_ZXOffset:
      fprintf(fPtr,"dblZXOffset ");
      break;
    case  DBLA_ZYOffset:
      fprintf(fPtr,"dblZYOffset ");
      break;
    case  DBLA_DSBConst:
      fprintf(fPtr,"dblDsbConst ");
      break;
    case  DBLA_DSBSelect:
      fprintf(fPtr,"dblDsbSelect ");
      break;
    case  DBLA_AInputSelect:
      fprintf(fPtr,"dblAInputSelect ");
      break;
    case  DBLA_AMultCoefSelect:
      fprintf(fPtr,"dblAMultCoefSelect ");
      break;
    case  DBLA_AMultConstControl:
      fprintf(fPtr,"dblAMultConstControl ");
      break;
    case  DBLA_AMultRtJustify:
      fprintf(fPtr,"dblAMultRtJustify ");
      break;
    case  DBLA_AMultConstSSB0:
      fprintf(fPtr,"dblAMultConstSSB0 ");
      break;
    case  DBLA_AMultConstSSB1:
      fprintf(fPtr,"dblAMultConstSSB1 ");
      break;
    case  DBLA_BInputSelect:
      fprintf(fPtr,"dblBInputSelect ");
      break;
    case  DBLA_BMultCoefSelect:
      fprintf(fPtr,"dblBMultCoefSelect ");
      break;
    case  DBLA_BMultConstControl:
      fprintf(fPtr,"dblBMultConstControl ");
      break;
    case  DBLA_BMultRtJustify:
      fprintf(fPtr,"dblBMultRtJustify ");
      break;
    case  DBLA_BMultConstSSB0:
      fprintf(fPtr,"dblBMultConstSSB0 ");
      break;
    case  DBLA_BMultConstSSB1:
      fprintf(fPtr,"dblBMultConstSSB1 ");
      break;
    case  DBLA_ALUOperation:
      fprintf(fPtr,"dblALUOperation ");
      break;
    case  DBLA_FinalDivide:
      fprintf(fPtr,"dblFinalDivide ");
      break;
    case  DBLA_Alpha0ClampControl:
      fprintf(fPtr,"dblAlpha0ClampControl ");
      break;
    case  DBLA_Alpha1ClampControl:
      fprintf(fPtr,"dblAlpha1ClampControl ");
      break;
    case  DBLA_AlphaFracClampControl:
      fprintf(fPtr,"dblAlphaFracClampControl ");
      break;
    case  DBLA_AlphaClampControl:
      fprintf(fPtr,"dblAlphaClampControl ");
      break;
    case  DBLA_DestAlphaSelect:
      fprintf(fPtr,"dblDestAlphaSelect ");
      break;
    case  DBLA_DestAlphaConstSSB0:
      fprintf(fPtr,"dblDestAlphaConstSSB0 ");
      break;
    case  DBLA_DestAlphaConstSSB1:
      fprintf(fPtr,"dblDestAlphaConstSSB1 ");
      break;
    case  DBLA_DitherMatrixA:
      fprintf(fPtr,"dblDitherMatrixA ");
      break;
    case  DBLA_DitherMatrixB:
      fprintf(fPtr,"dblDitherMatrixB ");
      /* Not Available in texture file format */
      break;
    case  DBLA_SrcPixels32Bit:
      fprintf(fPtr,"dblSrcPixels32Bit ");
      break;
    case  DBLA_SrcBaseAddr:
      fprintf(fPtr,"dblSrcBaseAddr ");
      break;
    case  DBLA_SrcXStride:
      fprintf(fPtr,"dblSrcXStride ");
      break;
    case  DBLA_SrcXOffset:
      fprintf(fPtr,"dblSrcXOffset ");
      break;
    case  DBLA_SrcYOffset:
      fprintf(fPtr,"dblSrcYOffset ");
    default:
      break;
    }

  TAB_ValuePrint(fPtr, value);  

}

static void TAB_Print(FILE *fPtr, uint32 command, uint32 value)
{
  switch(command)
    {
    case TXA_MinFilter:
      fprintf(fPtr,"txMinFilter ");
      break;
    case TXA_MagFilter:
      fprintf(fPtr,"txMagFilter ");
      break;
    case TXA_InterFilter:
      fprintf(fPtr,"txInterFilter ");
      break;
    case TXA_TextureEnable:
      fprintf(fPtr,"txTextureEnable ");
      break;
    case TXA_PipIndexOffset:
      fprintf(fPtr,"txPipIndexOffset ");
      break;
    case TXA_PipColorSelect:
      fprintf(fPtr,"txPipColorSel ");
      break;
    case TXA_PipAlphaSelect:
      fprintf(fPtr,"txPipAlphaSel ");
      break;
    case TXA_PipSSBSelect:
      fprintf(fPtr,"txPipSsbSel ");
      break;
    case TXA_PipConstSSB0:
      fprintf(fPtr,"txPipConstSsb0 ");
      break;
    case TXA_PipConstSSB1:
      fprintf(fPtr,"txPipConstSsb1 ");
      break;
    case TXA_FirstColor:
      fprintf(fPtr,"txFirstColor ");
      break;
    case TXA_SecondColor:
      fprintf(fPtr,"txSecondColor ");
      break;
    case TXA_ThirdColor:
      fprintf(fPtr,"txThirdColor ");
      break;
    case TXA_FirstAlpha:
      fprintf(fPtr,"txFirstAlpha ");
      break;
    case TXA_SecondAlpha:
      fprintf(fPtr,"txSecondAlpha ");
      break;
    case TXA_ColorOut:
      fprintf(fPtr,"txColorOut ");
      break;
    case TXA_AlphaOut:
      fprintf(fPtr,"txAlphaOut ");
      break;
    case TXA_BlendOp:
      fprintf(fPtr,"txBlendOp ");
      break;
    case TXA_BlendColorSSB0:
      fprintf(fPtr,"txBlendColorSsb0 ");
      break;
    case TXA_BlendColorSSB1:
      fprintf(fPtr,"txBlendColorSsb1 ");
      break;
    default:
      break;
    }
  switch(command)
    {
    case TXA_MinFilter: case TXA_MagFilter: case TXA_InterFilter:
      TAB_FilterPrint(fPtr, value);
      break;
    case TXA_PipColorSelect: case TXA_PipAlphaSelect: 
    case TXA_PipSSBSelect:
      TAB_PipSelPrint(fPtr, value);
      break;
    case TXA_PipConstSSB0: case TXA_PipConstSSB1:
    case TXA_BlendColorSSB0: case TXA_BlendColorSSB1:
      TAB_ConstPrint(fPtr, value);
      break;
    case TXA_FirstColor: case TXA_SecondColor: case TXA_ThirdColor:
      TAB_ColorSrcPrint(fPtr, value);
      break;
    case TXA_FirstAlpha: case TXA_SecondAlpha:
      TAB_AlphaSrcPrint(fPtr, value);
      break;
    case TXA_ColorOut: case TXA_AlphaOut:
      TAB_OutPrint(fPtr, value);
      break;
    case TXA_BlendOp:
      TAB_BlendOpPrint(fPtr, value);
      break;
    case TXA_TextureEnable: case TXA_PipIndexOffset:
      TAB_ValuePrint(fPtr, value);
      break;
    default:
      fprintf(fPtr,"#Unknown Command=%d Value=%d", command, value);
    }
}
static void Indent(FILE *fPtr, int tab)
{
  int i;
  for(i=0; i<tab; i++)
    fprintf(fPtr,"\t");
}

void SDFTex_Print(SDFTex *tex, FILE *fPtr, int tab)
{
  int i;
  bool set;
  uint32 val;
  
  Indent(fPtr, tab);
  fprintf(fPtr,"{\n");
  if (tex->FileName != NULL)
    {
      if (strcasecmp("(none).utf", tex->FileName))
	{
	  Indent(fPtr, tab+1);
	  fprintf(fPtr,"fileName \"%s\"\n",tex->FileName);
	}
    }
  for(i=0; i<TAB_SIZE; i++)
    {
      set = SDFTex_GetTAB(tex, i, &val);
      if (set)
	{
	  Indent(fPtr, tab+1);
	  TAB_Print(fPtr, i, val);
	  fprintf(fPtr,"\n");
	}
    }
  Indent(fPtr, tab+1);
  fprintf(fPtr,"xWrap ");
  tex->XClampPos = ftell(fPtr);
  if ((tex->XTile)==TRUE)
    fprintf(fPtr,"Tile  \n");
  else
    fprintf(fPtr,"Clamp \n");
  Indent(fPtr, tab+1);
  fprintf(fPtr,"yWrap ");
  tex->YClampPos = ftell(fPtr);
  if ((tex->YTile)==TRUE)
    fprintf(fPtr,"Tile  \n");
  else
    fprintf(fPtr,"Clamp \n");

  for(i=0; i<DAB_SIZE; i++)
    {
      set = SDFTex_GetDAB(tex, i, &val);
      if (set)
	{
	  Indent(fPtr, tab+1);
	  DAB_Print(fPtr, i, val);
	  fprintf(fPtr,"\n");
	}
    }

  if (tex->PageIndex[0]>=0)
    {
      Indent(fPtr, tab+1);
      fprintf(fPtr,"pageIndex %d\n",tex->PageIndex[0]);
      if (tex->PageIndex[1]>=0)
	{
	  Indent(fPtr, tab+1);
	  fprintf(fPtr,"subIndex %d\n",tex->PageIndex[1]);
	}	
    }

  Indent(fPtr, tab);
  fprintf(fPtr,"}\n");

}

M2Err Texture_Keyword(int32 *tab, int32 *dab)
{

  *tab = -1; *dab = -1;

  if(!strcasecmp(token, "txMinFilter"))
    *tab = TXA_MinFilter;
  else if(!strcasecmp(token, "txMagFilter"))     
    *tab = TXA_MagFilter;
  else if(!strcasecmp(token, "txInterFilter"))     
    *tab = TXA_InterFilter;
  else if(!strcasecmp(token, "txTextureEnable"))     
    *tab = TXA_TextureEnable;
  else if(!strcasecmp(token, "txPipIndexOffset"))     
    *tab = TXA_PipIndexOffset;
  else if(!strcasecmp(token, "txPipColorSel"))
    *tab = TXA_PipColorSelect;
  else if(!strcasecmp(token, "txPipAlphaSel"))
    *tab = TXA_PipAlphaSelect;
  else if(!strcasecmp(token, "txPipSsbSel"))     
    *tab = TXA_PipSSBSelect;
  else if(!strcasecmp(token, "txPipConstSsb0"))
    *tab = TXA_PipConstSSB0;
  else if(!strcasecmp(token, "txPipConstSsb1"))
    *tab = TXA_PipConstSSB1; 
  else if(!strcasecmp(token, "txFirstColor"))
    *tab = TXA_FirstColor;
  else if(!strcasecmp(token, "txSecondColor"))
    *tab = TXA_SecondColor;
  else if(!strcasecmp(token, "txThirdColor"))
    *tab = TXA_ThirdColor;
  else if(!strcasecmp(token, "txFirstAlpha"))
    *tab = TXA_FirstAlpha;
  else if(!strcasecmp(token, "txSecondAlpha"))
    *tab = TXA_SecondAlpha;
  else if(!strcasecmp(token, "txColorOut"))
    *tab = TXA_ColorOut;
  else if(!strcasecmp(token, "txAlphaOut"))
    *tab = TXA_AlphaOut;
  else if(!strcasecmp(token, "txBlendOp"))
    *tab = TXA_BlendOp;
  else if(!strcasecmp(token, "txBlendColorSsb0"))
    *tab = TXA_BlendColorSSB0;
  else if(!strcasecmp(token, "txBlendColorSsb1"))
    *tab = TXA_BlendColorSSB1;
  else if(!strcasecmp(token, "dblEnableAttrs"))
    *dab = DBLA_EnableAttrs;
  else if(!strcasecmp(token, "dblZBuffEnable"))
    *dab = DBLA_ZBuffEnable;
  else if(!strcasecmp(token, "dblZBuffOutEnable"))
    *dab = DBLA_ZBuffOutEnable;
  else if(!strcasecmp(token, "dblWinClipInEnable"))
    *dab = DBLA_WinClipInEnable;
  else if(!strcasecmp(token, "dblWinClipOutEnable"))
    *dab = DBLA_WinClipOutEnable;
  else if(!strcasecmp(token, "dblBlendEnable"))
    *dab = DBLA_BlendEnable;
  else if(!strcasecmp(token, "dblSrcInputEnable"))
    *dab = DBLA_SrcInputEnable;
  else if(!strcasecmp(token, "dblDitherEnable"))
    *dab = DBLA_DitherEnable;
  else if(!strcasecmp(token, "dblRGBADestOut"))
    *dab = DBLA_RGBADestOut;
  else if(!strcasecmp(token, "dblDiscard"))
    *dab = DBLA_Discard;
  else if(!strcasecmp(token, "dblDiscardZClipped"))
    *dab = DBLA_DiscardZClipped;
  else if(!strcasecmp(token, "dblDiscardSSB0"))
    *dab = DBLA_DiscardSSB0;
  else if(!strcasecmp(token, "dblDiscardRGB0"))
    *dab = DBLA_DiscardRGB0;
  else if(!strcasecmp(token, "dblDiscardAlpha0"))
    *dab = DBLA_DiscardAlpha0;
  else if (!strcasecmp(token, "dblRGBConstIn"))
    *dab = DBLA_RGBConstIn;
  else if (!strcasecmp(token, "dblXWinClipMin"))
    *dab = DBLA_XWinClipMin;
  else if (!strcasecmp(token, "dblXWinClipMax"))
    *dab = DBLA_XWinClipMax;
  else if (!strcasecmp(token, "dblYWinClipMin"))
    *dab = DBLA_YWinClipMin;
  else if (!strcasecmp(token, "dblYWinClipMax"))
    *dab = DBLA_YWinClipMax;
  else if (!strcasecmp(token, "dblYWinClipMax"))
    *dab = DBLA_YWinClipMax;
  else if (!strcasecmp(token, "dblZCompareControl"))
    *dab = DBLA_ZCompareControl;
  else if (!strcasecmp(token, "dblZXOffset"))
    *dab = DBLA_ZXOffset;
  else if (!strcasecmp(token, "dblZYOffset"))
    *dab = DBLA_ZYOffset;
  else if (!strcasecmp(token, "dblDsbConst"))
    *dab = DBLA_DSBConst;
  else if (!strcasecmp(token, "dblDsbSelect"))
    *dab = DBLA_DSBSelect;
  else if (!strcasecmp(token, "dblAInputSelect"))
    *dab = DBLA_AInputSelect;
  else if (!strcasecmp(token, "dblAMultCoefSelect"))
    *dab = DBLA_AMultCoefSelect;
  else if (!strcasecmp(token, "dblAMultConstControl"))
    *dab = DBLA_AMultConstControl;
  else if (!strcasecmp(token, "dblAMultRtJustify"))
    *dab = DBLA_AMultRtJustify;
  else if (!strcasecmp(token, "dblAMultConstSSB0"))
    *dab = DBLA_AMultConstSSB0;
  else if (!strcasecmp(token, "dblAMultConstSSB1"))
    *dab = DBLA_AMultConstSSB1;
  else if (!strcasecmp(token, "dblBInputSelect"))
    *dab = DBLA_BInputSelect;
  else if (!strcasecmp(token, "dblBMultCoefSelect"))
    *dab = DBLA_BMultCoefSelect;
  else if (!strcasecmp(token, "dblBMultConstControl"))
    *dab = DBLA_BMultConstControl;
  else if (!strcasecmp(token, "dblBMultRtJustify"))
    *dab = DBLA_BMultRtJustify;
  else if (!strcasecmp(token, "dblBMultConstSSB0"))
    *dab = DBLA_BMultConstSSB0;
  else if (!strcasecmp(token, "dblBMultConstSSB1"))
    *dab = DBLA_BMultConstSSB1;
  else if (!strcasecmp(token, "dblALUOperation"))
    *dab = DBLA_ALUOperation;
  else if (!strcasecmp(token, "dblFinalDivide"))
    *dab = DBLA_FinalDivide;
  else if (!strcasecmp(token, "dblAlpha0ClampControl"))
    *dab = DBLA_Alpha0ClampControl;
  else if (!strcasecmp(token, "dblAlpha1ClampControl"))
    *dab = DBLA_Alpha1ClampControl;
  else if (!strcasecmp(token, "dblAlphaFracClampControl"))
    *dab = DBLA_AlphaFracClampControl;
  else if (!strcasecmp(token, "dblAlphaClampControl"))
    *dab = DBLA_AlphaClampControl;
  else if (!strcasecmp(token, "dblDestAlphaSelect"))
    *dab = DBLA_DestAlphaSelect;
  else if (!strcasecmp(token, "dblDestAlphaConstSSB0"))
    *dab = DBLA_DestAlphaConstSSB0;
  else if (!strcasecmp(token, "dblDestAlphaConstSSB1"))
    *dab = DBLA_DestAlphaConstSSB1;
  else if (!strcasecmp(token, "dblDitherMatrixA"))
    *dab = DBLA_DitherMatrixA;
  else if (!strcasecmp(token, "dblDitherMatrixB"))
    *dab = DBLA_DitherMatrixB;
  /* Not Available in texture file format */
  else if (!strcasecmp(token, "dblSrcPixels32Bit"))
    *dab = DBLA_SrcPixels32Bit;
  else if (!strcasecmp(token, "dblSrcBaseAddr"))
    *dab = DBLA_SrcBaseAddr;
  else if (!strcasecmp(token, "dblSrcXStride"))
    *dab = DBLA_SrcXStride;
  else if (!strcasecmp(token, "dblSrcXOffset"))
    *dab = DBLA_SrcXOffset;
  else if (!strcasecmp(token, "dblSrcYOffset"))
    *dab = DBLA_SrcYOffset;
  else
    {
      /*      fprintf(stderr,"Unknown token \"%s\"\n", token); */
      BadToken("Unrecognized TexBlend Keyword:",token);
      return(M2E_Range);
    }
  return(M2_NoErr);
}

M2Err Texture_Filter(uint32 *value)
{
  lws_get_token();
 
  if(!strcasecmp(token, "Nearest"))     
    *value = TX_Nearest;
  else if(!strcasecmp(token, "Linear"))     
    *value = TX_Linear;
  else if(!strcasecmp(token, "Bilinear"))     
    *value = TX_Bilinear;
  else if(!strcasecmp(token, "QuasiTrilinear"))     
    *value = TX_QuasiTrilinear;
  else
    {
      BadToken("Unrecognized Texture Filter:",token);
      return(M2E_Range);
    }
  return(M2E_NoErr);
}

M2Err Texture_PipSel(uint32 *value)
{
  lws_get_token();
	
  if(!strcasecmp(token, "Const"))     
    *value = TX_PipSelectConst;
  else if(!strcasecmp(token, "Texture"))     
    *value = TX_PipSelectTexture;
  else if(!strcasecmp(token, "ColorTable"))     
    *value = TX_PipSelectColorTable;
  else
    {
      BadToken("Unrecognized Pip Select:",token);
      return(M2E_Range);
    }
  return(M2E_NoErr);
}


M2Err Texture_ColorSrc(uint32 *value)
{
  lws_get_token();
	
  if(!strcasecmp(token, "PrimAlpha"))     
    *value = TX_ColorSelectPrimAlpha;
  else if(!strcasecmp(token, "PrimColor"))
   *value = TX_ColorSelectPrimColor;
  else if(!strcasecmp(token, "TexAlpha"))
   *value = TX_ColorSelectTexAlpha;
  else if(!strcasecmp(token, "TexColor"))
    *value = TX_ColorSelectTexColor;
  else if(!strcasecmp(token, "ConstAlpha"))
    *value = TX_ColorSelectConstAlpha;
  else if(!strcasecmp(token, "ConstColor"))
    *value = TX_ColorSelectConstColor;
  else 
    {
      BadToken("Unrecognized Color Source:",token);
      return(M2E_Range);
    }
  return(M2E_NoErr);
}

M2Err Texture_AlphaSrc(uint32 *value)
{
  lws_get_token();
	
  if(!strcasecmp(token, "PrimAlpha"))     
    *value = TX_AlphaSelectPrimAlpha;
  else if(!strcasecmp(token, "TexAlpha"))
    *value = TX_AlphaSelectTexAlpha;
  else if(!strcasecmp(token, "ConstAlpha"))
    *value = TX_AlphaSelectConstAlpha;
  else 
    {
      BadToken("Unrecognized Alpha Source:",token);
      return(M2E_Range);
    }
  return(M2E_NoErr);
}

M2Err Texture_Out(uint32 *value)
{
  lws_get_token();
	
  if(!strcasecmp(token, "Prim"))     
    *value = TX_BlendOutSelectPrim;
  else if(!strcasecmp(token, "Texture"))
    *value = TX_BlendOutSelectTex;
  else if(!strcasecmp(token, "Blend"))
    *value = TX_BlendOutSelectBlend;
  else 
    {
      BadToken("Unrecognized Out Select:",token);
      return(M2E_Range);
    }
  return(M2E_NoErr);
}

M2Err Texture_BlendOp(uint32 *value)
{
  lws_get_token();
	
  if(!strcasecmp(token, "Lerp"))     
    *value = TX_BlendOpLerp;
  else if(!strcasecmp(token, "Mult"))
    *value = TX_BlendOpMult;
  else 
    {
      BadToken("Unrecognized Blend Op:",token);
      return(M2E_Range);
    }
  return(M2E_NoErr);
}

M2Err Texture_Const(uint32 *value)
{
  if(lws_read_uint(value))	
    return(M2E_NoErr);
  else
    return(M2E_Range);
}

M2Err Texture_Value(uint32 *value)
{
  if(lws_read_uint(value))	
    return(M2E_NoErr);
  else
    return(M2E_Range);
}

M2Err Texture_TAB(SDFTex *tb, int32 command)
{
  uint32 value;
  M2Err err;

  err = M2E_NoErr;
  switch(command)
    {
    case TXA_MinFilter: case TXA_MagFilter: case TXA_InterFilter:
      err = Texture_Filter(&value);
      break;
    case TXA_PipColorSelect: case TXA_PipAlphaSelect: 
    case TXA_PipSSBSelect:
      err= Texture_PipSel(&value);
      break;
    case TXA_PipConstSSB0: case TXA_PipConstSSB1:
    case TXA_BlendColorSSB0: case TXA_BlendColorSSB1:
      err = Texture_Const(&value);
      break;
    case TXA_FirstColor: case TXA_SecondColor: case TXA_ThirdColor:
      err = Texture_ColorSrc(&value);
      break;
    case TXA_FirstAlpha: case TXA_SecondAlpha:
      err = Texture_AlphaSrc(&value);
      break;
    case TXA_ColorOut: case TXA_AlphaOut:
      err = Texture_Out(&value);
      break;
    case TXA_BlendOp:
      err = Texture_BlendOp(&value);
      break;
    case TXA_TextureEnable: case TXA_PipIndexOffset:
      Texture_Value(&value);
      break;
    default:
      BadToken("Unrecognized TexBlend:",token);
      err = M2E_Range;
      break;
    }
  
  if (err == M2E_NoErr)
    SDFTex_SetTAB(tb, (uint32)command, (uint32)value);

  return(err);
}

M2Err Texture_DAB(SDFTex *tb, int32 command)
{
  uint32 value;
  M2Err err;

  err = M2E_NoErr;
  switch(command)
    { 
    case DBLA_EnableAttrs:
    case DBLA_ZBuffEnable:
    case DBLA_ZBuffOutEnable:
    case DBLA_WinClipInEnable:
    case DBLA_WinClipOutEnable:
    case DBLA_BlendEnable:
    case DBLA_SrcInputEnable:
    case DBLA_DitherEnable:
    case DBLA_RGBADestOut:
    case DBLA_Discard:
    case DBLA_DiscardZClipped:
    case DBLA_DiscardSSB0:
    case DBLA_DiscardRGB0:
    case DBLA_DiscardAlpha0:
    case  DBLA_RGBConstIn:
    case  DBLA_XWinClipMin:
    case  DBLA_XWinClipMax:
    case  DBLA_YWinClipMin:
    case  DBLA_YWinClipMax:
    case  DBLA_ZCompareControl:
    case  DBLA_ZXOffset:
    case  DBLA_ZYOffset:
    case  DBLA_DSBConst:
    case  DBLA_DSBSelect:
    case  DBLA_AInputSelect:
    case  DBLA_AMultCoefSelect:
    case  DBLA_AMultConstControl:
    case  DBLA_AMultRtJustify:
    case  DBLA_AMultConstSSB0:
    case  DBLA_AMultConstSSB1:
    case  DBLA_BInputSelect:
    case  DBLA_BMultCoefSelect:
    case  DBLA_BMultConstControl:
    case  DBLA_BMultRtJustify:
    case  DBLA_BMultConstSSB0:
    case  DBLA_BMultConstSSB1:
    case  DBLA_ALUOperation:
    case  DBLA_FinalDivide:
    case  DBLA_Alpha0ClampControl:
    case  DBLA_Alpha1ClampControl:
    case  DBLA_AlphaFracClampControl:
    case  DBLA_AlphaClampControl:
    case  DBLA_DestAlphaSelect:
    case  DBLA_DestAlphaConstSSB0:
    case  DBLA_DestAlphaConstSSB1:
    case  DBLA_DitherMatrixA:
    case  DBLA_DitherMatrixB:
     Texture_Value(&value);
      break;
      /* Not Available in texture file format */
    case  DBLA_SrcPixels32Bit:
    case  DBLA_SrcBaseAddr:
    case  DBLA_SrcXStride:
    case  DBLA_SrcXOffset:
    case  DBLA_SrcYOffset:
      BadToken("DAB TexBlend token \"%s\" Not Allowed in SDF:",token);
      err = M2E_Range;
      break;
    default:
      BadToken("Unrecognized DAB TexBlend:",token);
      err = M2E_Range;
      break;
    }
  
  if (err == M2E_NoErr)
    SDFTex_SetDAB(tb, (uint32)command, (uint32)value);

  return(err);
}

M2Err Texture_Entry(int tokenType, SDFTex *tb)
{
  M2Err err;
  int32 tab;
  int32 dab;
  int32 value;
  int curTex = 0;
  int newToken;
  
  newToken = tokenType;
  /*
  while ((tokenType = lws_get_token()) != T_LBRACE)
    ;
    */

  while (strcasecmp(token, "}") && (tokenType != T_END))
    {
      do
	{
	  newToken = lws_get_token();
	  if (newToken == T_RBRACE)
	    return (M2E_NoErr);
	  if (newToken == T_END)
	    return (M2E_Range);
	} while (newToken != T_KEYWORD);

      if(!strcasecmp(token, "fileName"))
	{
	  lws_read_string();
	  tb->FileName = (char *)calloc(strlen(token)+4,1);
	  if (tb->FileName == NULL)
	    return(M2E_NoMem);
	  strcpy(tb->FileName, token);
	}
      else if (!strcasecmp(token, "pageIndex"))
	{
	  if(lws_read_int(&value))	
	    tb->PageIndex[0] = value;
	  else
	    fprintf(stderr,"ERROR:pageIndex needs an integer, not \"%s\"\n", token);
   	}
      else if (!strcasecmp(token, "subIndex"))
	{
	  if(lws_read_int(&value))	
	    tb->PageIndex[1] = value;
	  else
	    fprintf(stderr,"ERROR:subIndex needs an integer, not \"%s\"\n", token);
   	}
      else if (!strcasecmp(token, "xWrap"))
	{
	  do
	    {
	      newToken = lws_get_token();
	      if (newToken == T_RBRACE)
		return (M2E_NoErr);
	    } while (newToken != T_KEYWORD);
	  if (!strcasecmp(token, "Tile"))
	    tb->XTile = TRUE;
	  else
	    tb->XTile = FALSE;
   	}
      else if (!strcasecmp(token, "yWrap"))
	{
	  do
	    {
	      newToken = lws_get_token();
	      if (newToken == T_RBRACE)
		return (M2E_NoErr);
	    } while (newToken != T_KEYWORD);
	  if (!strcasecmp(token, "Tile"))
	    tb->YTile = TRUE;
	  else
	    tb->YTile = FALSE;
   	}
      else
	{
	  err = Texture_Keyword(&tab, &dab);
	  if (tab != -1)
	    Texture_TAB(tb, tab);
	  else if (dab != -1)
	    Texture_DAB(tb, dab);
	}
    }

  return(M2E_NoErr);
}


#define TEX_BUF_SIZE 20

M2Err Texture_ReadFile(char *fileIn, SDFTex **myTex, int *numTex, int *cTex)
{
  LWS lws;
  ByteStream*		stream = NULL;
  int  tokenType;
  int nTextures = TEX_BUF_SIZE;
  SDFTex *textures, *temp;
  int curTex = 0;
  
  
  *myTex = NULL;
  textures = (SDFTex *)qMemClearPtr(nTextures, sizeof(SDFTex));
  if (textures == NULL)
    {
      fprintf(stderr,"ERROR:Out of memory in Texture_ReadFile!\n");
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
	  if (!strcasecmp(token, "TexArray"))     
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
			  fprintf(stderr,"ERROR:Out of memory in Texture_ReadFile!\n");
			  return(M2E_NoMem);
			}
		      textures = temp; 
		    }
		}
	    }
	  break;
	default:
	  break;
	}
    }
  *myTex = textures;
  *numTex = nTextures;
  *cTex = curTex;
  return(M2E_NoErr);
}

