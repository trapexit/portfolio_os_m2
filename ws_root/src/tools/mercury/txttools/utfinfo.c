/*
	File:		utfinfo.c

	Contains:	Prints out header information for a given UTF file

	Written by:	Todd Allendorf 

	Copyright:	© 1996 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <3>	 7/15/95	TMA		No reads in the levels of detail to get an accurate size for
									each level of detail.
		 <2>	 5/16/95	TMA		Autodocs added.

	To Do:
*/

/**
|||	AUTODOC -public -class tools -group m2tx -name utfinfo
|||	Print out information about the UTF texture.
|||	
|||	  Synopsis
|||	
|||	    utfinfo <input file> 
|||	
|||	  Description
|||	
|||	    This tool prints to standard output relevant Header and DCI information  
|||	    in a UTF texture file.
|||	
|||	  Arguments
|||	
|||	    <input file>
|||	        The input UTF texture.
|||	
**/

#include <stdio.h>

#ifdef applec
#include "M2TXlib.h"
#include "ifflib.h"
#else
#include "ifflib.h"
#include "M2TXlib.h"
#endif


#include "M2TXattr.h"
#include "clt.h"
#include "clttxdblend.h"
#include <stdlib.h>
#include <string.h>

Err TXTR_GetNext(IFFParser *iff, M2TX *tex, bool *foundM2);
Err TXTR_SetupFrame(IFFParser *iff, bool inLOD);

M2Err M2TX_OpenFile(char *fileName, IFFParser **iff, bool writeMode, bool isMac, void *spec);

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
  fprintf(fPtr,"%x ", value);
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

static void DAB_ZComparePrint(FILE *fPtr, uint32 value)
{
  bool first = TRUE;

  if (value >= DBL_ZUpdateOnGreaterZ)
    {
      fprintf(fPtr, "DBL_ZUpdateOnGreaterZ | ");
      value -= DBL_ZUpdateOnGreaterZ;
    }
  if (value >= DBL_PixOutOnGreaterZ)
    {
      fprintf(fPtr, "DBL_PixOutOnGreaterZ | ");
      value -= DBL_PixOutOnGreaterZ;
    }
  if (value >= DBL_ZUpdateOnEqualZ)
    {
      fprintf(fPtr, "DBL_ZUpdateOnEqualZ | ");
      value -= DBL_ZUpdateOnEqualZ;
    }
  if (value >= DBL_PixOutOnEqualZ)
    {
      fprintf(fPtr, "DBL_PixOutOnEqualZ | ");
      value -= DBL_PixOutOnEqualZ;
    }
  if (value >= DBL_ZUpdateOnSmallerZ)
    {
      fprintf(fPtr, "DBL_ZUpdateOnSmallerZ | ");
      value -= DBL_ZUpdateOnSmallerZ;
    }
  if (value >= DBL_PixOutOnSmallerZ)
    {
      fprintf(fPtr, "DBL_PixOutOnSmallerZ | ");
      value -= DBL_PixOutOnSmallerZ;
    }
}

static void DAB_MAConstPrint(FILE *fPtr, uint32 value)
{
  
  switch (value)
    {
    case DBL_MAConstByTexSSB:
      fprintf(fPtr, "DBL_MAConstByTexSSB ");
      break;
    case DBL_MAConstBySrcSSB:
      fprintf(fPtr, "DBL_MAConstBySrcSSB ");
      break;
    }
}

static void DAB_MASelectPrint(FILE *fPtr, uint32 value)
{
  
  switch (value)
    {
    case DBL_MASelectTexAlpha:
      fprintf(fPtr, "DBL_MASelectTexAlpha ");
      break;
    case DBL_MASeelctSrcalpha:
      fprintf(fPtr, "DBL_MASelectSrcAlpha ");
      break;
    case DBL_MASelectConst:
      fprintf(fPtr, "DBL_MASelectConst ");
      break;
    case DBL_MASelectSrcColor:
      fprintf(fPtr, "DBL_MASelectSrcColor ");
      break;
    case DBL_MASelectTexAlphaComplement:
      fprintf(fPtr, "DBL_MASelectTexAlphaComplement ");
      break;
    case DBL_MASelectSrcAlphaComplement:
      fprintf(fPtr, "DBL_MASelectSrcAlphaComplement ");
      break;
    case DBL_MASelectConstComplement:
      fprintf(fPtr, "DBL_MASelectConstComplement ");
      break;
    case DBL_MASelectSrcColorComplement:
      fprintf(fPtr, "DBL_MASelectSrcColorComplement ");
      break;
    }
}


static void DAB_ASelectPrint(FILE *fPtr, uint32 value)
{

  switch (value)
    {
    case DBL_ASelectTexColor:
      fprintf(fPtr, "DBL_ASelectTexColor ");
      break;
    case DBL_ASelectConstColor:
      fprintf(fPtr, "DBL_ASelectConstColor ");
      break;
    case DBL_ASelectSrcColorComplement:
      fprintf(fPtr, "DBL_ASelectSrcColorComplement ");
      break;
    case DBL_ASelectTexAlpha:
      fprintf(fPtr, "DBL_ASelectTexAlpha ");
      break;
    }
}

static void DAB_MBConstPrint(FILE *fPtr, uint32 value)
{
  
  switch (value)
    {
    case DBL_MAConstByTexSSB:
      fprintf(fPtr, "DBL_MBConstByTexSSB ");
      break;
    case DBL_MAConstBySrcSSB:
      fprintf(fPtr, "DBL_MBConstBySrcSSB ");
      break;
    }
}

static void DAB_DASelectPrint(FILE *fPtr, uint32 value)
{
  
  switch (value)
    {
    case DBL_DestAlphaSelectTexAlpha:
      fprintf(fPtr, "DBL_DestAlphaSelectTexAlpha ");
      break;
    case DBL_DestAlphaSelectTexSSBConst:
      fprintf(fPtr, "DBL_DestAlphaSelectTexSSBConst ");
      break;
    case DBL_DestAlphaSelectSrcSSBConst:
      fprintf(fPtr, "DBL_DestAlphaSelectSrcSSBConst ");
      break;
    case DBL_DestAlphaSelectSrcAlpha:
      fprintf(fPtr, "DBL_DestAlphaSelectSrcAlpha ");
      break;
    case DBL_DestAlphaSelectRBlend:
      fprintf(fPtr, "DBL_DestAlphaSelectRBlend ");
      break;
    }
}

static void DAB_AlphaClampPrint(FILE *fPtr, uint32 value)
{
  
  switch (value)
    {
    case DBL_AlphaClampLeaveAlone:
      fprintf(fPtr, "DBL_AlphaClampLeaveAlone ");
      break;
    case DBL_AlphaClampForceTo1:
      fprintf(fPtr, "DBL_AlphaClampForceTo1 ");
      break;
    case DBL_AlphaClampForceTo0:
      fprintf(fPtr, "DBL_AlphaClampForceTo0 ");
      break;
    }
}

static void DAB_ALUOpPrint(FILE *fPtr, uint32 value)
{
  
  switch (value)
    {
    case DBL_Add:
      fprintf(fPtr, "DBL_Add ");
      break;
    case DBL_AddClamp:
      fprintf(fPtr, "DBL_AddClamp ");
      break;
    case DBL_Subtract:
      fprintf(fPtr, "DBL_Subtract ");
      break;
    case DBL_SubtractClamp:
      fprintf(fPtr, "DBL_SubtractClamp ");
      break;
    case DBL_SubtractFromB:
      fprintf(fPtr, "DBL_SubtractFromB ");
      break;
    case DBL_SubtractFromBClamp:
      fprintf(fPtr, "DBL_SubtractFromBClamp ");
      break;
    case DBL_OutputZero:
      fprintf(fPtr, "DBL_OutputZero ");
      break;
    case DBL_Neither:
      fprintf(fPtr, "DBL_Neither ");
      break;
    case DBL_NotA_AND_B:
      fprintf(fPtr, "DBL_NotA_AND_B ");
      break;
    case DBL_NotA:
      fprintf(fPtr, "DBL_NotA ");
      break;
    case DBL_NotB_AND_A:
      fprintf(fPtr, "DBL_NotB_AND_A ");
      break;
    case DBL_NotB:
      fprintf(fPtr, "DBL_NotB ");
      break;
    case DBL_XOR:
      fprintf(fPtr, "DBL_XOR ");
      break;
    case DBL_Not_A_AND_B:
      fprintf(fPtr, "DBL_Not_A_AND_B ");
      break;
    case DBL_A_AND_B:
      fprintf(fPtr, "DBL_A_AND_B ");
      break;
    case DBL_OneOnEqual:
      fprintf(fPtr, "DBL_OneOnEqual ");
      break;
    case DBL_B:
      fprintf(fPtr, "DBL_B ");
      break;
    case DBL_NotA_OR_B:
      fprintf(fPtr, "DBL_NotA_OR_B ");
      break;
    case DBL_A:
      fprintf(fPtr, "DBL_A ");
      break;
    case DBL_NotB_OR_A:
      fprintf(fPtr, "DBL_NotB_OR_A ");
      break;
    case DBL_A_OR_B:
      fprintf(fPtr, "DBL_A_OR_B ");
      break;
    case DBL_OutputOne:
      fprintf(fPtr, "DBL_OutputOne ");
      break;
    }
}


static void DAB_MBSelectPrint(FILE *fPtr, uint32 value)
{
  
  switch (value)
    {
    case DBL_MBSelectTexAlpha:
      fprintf(fPtr, "DBL_MBSelectTexAlpha ");
      break;
    case DBL_MBSelectSrcAlpha:
      fprintf(fPtr, "DBL_MBSelectSrcAlpha ");
      break;
    case DBL_MBSelectConst:
      fprintf(fPtr, "DBL_MBSelectConst ");
      break;
    case DBL_MBSelectTexColor:
      fprintf(fPtr, "DBL_MBSelectTexColor ");
      break;
    case DBL_MBSelectTexAlphaComplement:
      fprintf(fPtr, "DBL_MBSelectTexAlphaComplement ");
      break;
    case DBL_MBSelectSrcAlphaComplement:
      fprintf(fPtr, "DBL_MBSelectSrcAlphaComplement ");
      break;
    case DBL_MBSelectConstComplement:
      fprintf(fPtr, "DBL_MBSelectConstComplement ");
      break;
    case DBL_MBSelectTexColorComplement:
      fprintf(fPtr, "DBL_MBSelectTexColorComplement ");
      break;
    }
}


static void DAB_BSelectPrint(FILE *fPtr, uint32 value)
{

  switch (value)
    {
    case DBL_BSelectSrcColor:
      fprintf(fPtr, "DBL_BSelectSrcColor ");
      break;
    case DBL_BSelectConstColor:
      fprintf(fPtr, "DBL_BSelectConstColor ");
      break;
    case DBL_BSelectTexColorComplement:
      fprintf(fPtr, "DBL_BSelectTexColorComplement ");
      break;
    case DBL_BSelectSrcAlpha:
      fprintf(fPtr, "DBL_BSelectSrcAlpha ");
      break;
    }
}

static void DAB_DSBSelectPrint(FILE *fPtr, uint32 value)
{

  switch (value)
    {
    case DBL_DSBSelectObjSSB:
      fprintf(fPtr, "DBL_DSBSelectObjSSB ");
      break;
    case DBL_DSBSelectConst:
      fprintf(fPtr, "DBL_DSBSelectConst ");
      break;
    case DBL_DSBSelectSrcSSB:
      fprintf(fPtr, "DBL_DSBSelectSrcSSB ");
      break;
    }
}

static void DAB_DiscardPrint(FILE *fPtr, uint32 value)
{
  bool first = TRUE;

  if (value >= DBL_DiscardZClipped)
    {
      fprintf(fPtr, "DBL_DiscardZClipped | ");
      value -= DBL_DiscardZClipped;
    }
  if (value >= DBL_DiscardSSB0)
    {
      fprintf(fPtr, "DBL_DiscardSSB0 | ");
      value -= DBL_DiscardSSB0;
    }
  if (value >= DBL_DiscardRGB0)
    {
      fprintf(fPtr, "DBL_DiscardRGB0 | ");
      value -= DBL_DiscardRGB0;
    }
  if (value >= DBL_DiscardAlpha0)
    {
      fprintf(fPtr, "DBL_DiscardAlpha0 | ");
      value -= DBL_DiscardAlpha0;
    }
}

static void DAB_EnablePrint(FILE *fPtr, uint32 value)
{
  bool first = TRUE;

  if (value >= DBL_ZBuffEnable)
    {
      fprintf(fPtr, "DBL_ZBuffEnable | ");
      value -= DBL_ZBuffEnable;
    }
  if (value >= DBL_ZBuffOutEnable)
    {
      fprintf(fPtr, "DBL_ZBuffOutEnable | ");
      value -= DBL_ZBuffOutEnable;
    }
  if (value >= DBL_WinClipInEnable)
    {
      fprintf(fPtr, "DBL_ZWinClipInEnable | ");
      value -= DBL_WinClipInEnable;
    }
  if (value >= DBL_WinClipOutEnable)
    {
      fprintf(fPtr, "DBL_ZWinClipOutEnable | ");
      value -= DBL_WinClipOutEnable;
    }
  if (value >= DBL_BlendEnable)
    {
      fprintf(fPtr, "DBL_BlendEnable | ");
      value -= DBL_BlendEnable;
    }
  if (value >= DBL_DitherEnable)
    {
      fprintf(fPtr, "DBL_DitherEnable | ");
      value -= DBL_DitherEnable;
    }
  if (value >= DBL_SrcInputEnable)
    {
      fprintf(fPtr, "DBL_SrcInputEnable | ");
      value -= DBL_SrcInputEnable;
    }
  if (value >= DBL_AlphaDestOut)
    {
      fprintf(fPtr, "DBL_AlphaDestOut | ");
      value -= DBL_AlphaDestOut;
    }
  if (value >= DBL_RGBDestOut)
    {
      fprintf(fPtr, "DBL_RGBDestOut | ");
      value -= DBL_RGBDestOut;
    }
  if (value >= DBL_RDestOut)
    {
      fprintf(fPtr, "DBL_RDestOut | ");
      value -= DBL_RDestOut;
    }
  if (value >= DBL_GDestOut)
    {
      fprintf(fPtr, "DBL_GDestOut | ");
      value -= DBL_GDestOut;
    }
  if (value >= DBL_BDestOut)
    {
      fprintf(fPtr, "DBL_BDestOut | ");
      value -= DBL_BDestOut;
    }
}

static void DAB_Print(FILE *fPtr, uint32 command, uint32 value)
{
  switch(command)
    {
    case DBLA_EnableAttrs:
      fprintf(fPtr,"dblEnableAttrs ");
      break;
    case DBLA_Discard:
      fprintf(fPtr, "dblDiscard ");
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
      fprintf(fPtr,"Unknown DAB command %d\n", command);
      break;
    }

  TAB_ValuePrint(fPtr, value);  

  fprintf(fPtr, "\t # ");
  switch (command)
    {
    case DBLA_EnableAttrs:
      DAB_EnablePrint(fPtr, value);
      break;
    case DBLA_Discard:
      DAB_DiscardPrint(fPtr, value);
      break;
    case  DBLA_ZCompareControl:
      DAB_ZComparePrint(fPtr, value);
      break;
    case  DBLA_DSBSelect:
      DAB_DSBSelectPrint(fPtr, value);
      break;
    case  DBLA_AInputSelect:
      DAB_ASelectPrint(fPtr, value);
      break;
    case  DBLA_AMultCoefSelect:
      DAB_MASelectPrint(fPtr, value);
      break;
    case  DBLA_AMultConstControl:
      DAB_MAConstPrint(fPtr, value);
      break;
    case  DBLA_BInputSelect:
      DAB_BSelectPrint(fPtr, value);
      break;
    case  DBLA_BMultCoefSelect:
      DAB_MBSelectPrint(fPtr, value);
      break;
    case  DBLA_BMultConstControl:
      DAB_MBConstPrint(fPtr, value);
      break;
    case  DBLA_ALUOperation:
      DAB_ALUOpPrint(fPtr, value);
      break;
    case  DBLA_AlphaClampControl:
      DAB_AlphaClampPrint(fPtr, value);
      break;
    case  DBLA_DestAlphaSelect:
      DAB_DASelectPrint(fPtr, value);
      break;
    default:
      break;
    }

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

void print_description()
{
  printf("Description:\n");
  printf("   Version %s\n",TEXTOOLS_VERISION);
  printf("   UTF Information\n");
}

long padPixels(int Pixels, int depth)
{
  long bits, remainder;

  bits = Pixels * depth;
  remainder = bits % 32;
  if (remainder)
    bits += (32-remainder);
  return(bits>>3);
}


int main( int argc, char *argv[] )
{
  M2TX tex;
  char fileIn[256];
  FILE *fPtr;
  M2Err err;
  M2TXPgHeader  *pgArray;
  M2TXFormat texFormat;
  IFFParser     *iff;
  bool   foundM2, hasPIP, flag, isCompressed;
  uint32 i, j, val, offset, totalBytes, lodPixels, lodLength, pageBytes;
  uint16 xSize, ySize;
  int    texIndex, nTextures;
  uint8  cDepth, aDepth, ssbDepth, numLOD;
  Err           result;

#ifdef M2STANDALONE
		
	printf("Enter: <FileIn>\n");
	printf("Example: dumb.utf\n");
	fscanf(stdin,"%s ",fileIn);
#else
	if (argc != 2)
	{
		fprintf(stderr,"Usage: %s <Input File>\n",argv[0]);
      print_description();
		return(-1);	
	}	
	else
        strcpy(fileIn, argv[1]);
#endif

	fPtr = fopen(fileIn, "r");
	if (fPtr == NULL)
	{
	  fprintf(stderr,"ERROR:Can't open file \"%s\" \n",fileIn);
		return(-1);
	}
	else 
		fclose(fPtr);		



  err = M2TX_OpenFile(fileIn, &iff, FALSE, FALSE, NULL); 
  if (err != M2E_NoErr)
    {
      fprintf(stderr,"ERROR:Can't open file \"%s\"\n",fileIn);
      return(-1);
    }

  result = TXTR_SetupFrame(iff, TRUE);
  if (result<0)
    {
      fprintf(stderr,"ERROR:Can't setup the frame.\n",fileIn);
      return(-1);
    }

  result = 0;
  texIndex = 0;
  while(result >= 0)
    {

      M2TX_Init(&tex);						/* Initialize a texture */
      
      result = TXTR_GetNext(iff, &tex, &foundM2);
      if (foundM2 && (result>=0))
	{

	  if (err != M2E_NoErr)
	    {
	      fprintf(stderr,"ERROR:Error during read.  Aborting\n");
	      return(-1);
	    }
	  
	  fPtr = stdout;
	  nTextures = tex.Page.NumTex;
	  fprintf(fPtr, "=============\nTexture %d\n", texIndex);

	  if (nTextures < 1)
	    {
	      M2TX_Print(&tex);
	      for(i=0; i<19; i++)
		{
		  err = M2TXTA_GetAttr(&(tex.TexAttr), i, &val);
		  if (err == M2E_NoErr)
		    {
		      TAB_Print(fPtr, i, val);
		      fprintf(fPtr,"\n");
		    }
		}
	      
	      for(i=0; i<46; i++)
		{
		  err = M2TXDB_GetAttr(&(tex.DBl), i, &val);
		  if (err == M2E_NoErr)
		    {
		      DAB_Print(fPtr, i, val);
		      fprintf(fPtr,"\n");
		    }
		}
	    }
	  else
	    {
	      pageBytes=0;
	      M2TXHeader_GetFHasPIP(&(tex.Header), &hasPIP);
	      if (hasPIP)
		{
		  fprintf(fPtr,"Has PIP\n");
		  fprintf(fPtr,"PIP Colors = %d\n", tex.PIP.NumColors);
		}
	      pgArray = tex.Page.PgData;
	      fprintf(fPtr,"Sub-textures = %d\n", nTextures);
	      for (i=0; i<nTextures; i++)
		{
		  fprintf(fPtr,"-----------------\nSub-texture %d\n", i);
		  /* Take care of the Header */
		  fprintf(fPtr,"NumLOD =%d\n", pgArray[i].NumLOD);
		  fprintf(fPtr,"MinXSize=%d \t MinYSize %d\n", 
		  pgArray[i].MinXSize, pgArray[i].MinYSize);
		  fprintf(fPtr,"MaxXSize=%d \t MaxYSize %d\n", 
			  pgArray[i].MinXSize<<(pgArray[i].NumLOD-1), 
			  pgArray[i].MinYSize<<(pgArray[i].NumLOD-1));
		  if (pgArray[i].PgFlags & M2PG_FLAGS_HasTexFormat)
		    {
		      texFormat = pgArray[i].TexFormat;
		    }
		  else
		    {
		      texFormat = tex.Header.TexFormat;
		    }
		  if (pgArray[i].PgFlags & M2PG_FLAGS_IsCompressed)
		    {
		      fprintf(fPtr, "Is Compressed\n");
		      isCompressed = TRUE;
		    }
		  else
		    isCompressed = FALSE;

		  flag = M2TXFormat_GetFIsLiteral(texFormat);
		  if (flag)
		    fprintf(fPtr,"Is Literal\n");
		  flag = M2TXFormat_GetFHasColor(texFormat);
		  if (flag) 
		    {	
		      fprintf(fPtr,"Has Color ");
		      cDepth = M2TXFormat_GetCDepth(texFormat);
		      fprintf(fPtr,"CDepth=%d \n",cDepth);
		    }
		  else 
		    cDepth = 0;
		  flag = M2TXFormat_GetFHasAlpha(texFormat);
		  if (flag)
		    {
		      fprintf(fPtr,"Has Alpha ");
		      aDepth = M2TXFormat_GetADepth(texFormat);
		      fprintf(fPtr,"ADepth=%d \n",aDepth);
		    }
		  else 
		    aDepth = 0;
		  flag = M2TXFormat_GetFHasSSB(texFormat);
		  if (flag)
		    {
		      fprintf(fPtr,"Has SSB ");
		      ssbDepth=1;
		    }
		  else
		    ssbDepth=0;
		  /* The Load Rects */
		  if (pgArray[i].PgFlags & M2PG_FLAGS_XWrapMode)
		    fprintf(fPtr,"XWrapMode Tile\n");
		  else
		    fprintf(fPtr,"XWrapMode Clamp\n");
		  if (pgArray[i].PgFlags & M2PG_FLAGS_YWrapMode)
		    fprintf(fPtr,"YWrapMode Tile\n");
		  else
		    fprintf(fPtr,"YWrapMode Clamp\n");
		  
		  /* The PIP */
		  if (hasPIP)
		    {
		      offset = pgArray[i].PIPIndexOffset;
		      fprintf(fPtr,"PipIndexOffset=%d\n",offset);
		    }
		  
		  totalBytes = 0;
		  numLOD = pgArray[i].NumLOD;
		  xSize = (pgArray[i].MinXSize<<(numLOD-1));
		  ySize = (pgArray[i].MinYSize<<(numLOD-1));
		  
		  for (j=0; j<numLOD; j++)
		    {
		      lodPixels = xSize*ySize;
		      lodLength = padPixels(lodPixels, aDepth + cDepth + ssbDepth);
		      totalBytes += lodLength;
		      xSize = xSize>>1;
		      ySize = ySize>>1;
		    }
		  if (isCompressed)
		    fprintf(fPtr,"Uncompressed ");
		  fprintf(fPtr,"Texel Data Size=%d (%6.3gK)\n", totalBytes,
			  (float)totalBytes/1024.0);
		  pageBytes += totalBytes;
		}
	      fprintf(fPtr,"-----------------\nPage Texel Data Size=%d (%6.3gK)\n",
		      pageBytes, (float)pageBytes/1024.0);

	    }
	  M2TXHeader_FreeLODPtrs(&(tex.Header));
	  texIndex++;
	}
    }
  return(0);
}




