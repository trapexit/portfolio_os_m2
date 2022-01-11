/*
	File:		M2TXattr.h

	Contains:	M2 Texture Library, texture blend attributes 

	Written by:	Todd Allendorf 

	Copyright:	© 1994 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		<6+>	 9/27/95	TMA		Update Destination Blend attributes
		<5+>	  8/4/95	TMA		Fixed compiler warnings.
		 <5>	  8/4/95	TMA		Add prototypes for MTXTA, M2TXDB, M2TXLR functions.
		<1+>	 7/11/95	TMA		Removed unneeded Destination Blend Attributes.
	To Do:
*/

/*
 * Texture blend info constants
 */

#if 0

#define TX_ColorSelectConstColor        (uint8)5
#define TX_ColorSelectConstAlpha        (uint8)4
#define TX_ColorSelectTexColor          (uint8)3
#define TX_ColorSelectTexAlpha          (uint8)2
#define TX_ColorSelectPrimColor         (uint8)1
#define TX_ColorSelectPrimAlpha         (uint8)0

#define TX_AlphaSelectPrimAlpha         (uint8)0
#define TX_AlphaSelectTexAlpha          (uint8)1
#define TX_AlphaSelectConstAlpha        (uint8)2

#define TX_BlendOutSelectPrim           (uint8)0
#define TX_BlendOutSelectTex            (uint8)1
#define TX_BlendOutSelectBlend          (uint8)2

#define TX_BlendOpLerp                  (uint8)0
#define TX_BlendOpMult                  (uint8)1

/*
 * Texture render info constants
 */

#define TX_WrapModeClamp                (uint8)0
#define TX_WrapModeTile                 (uint8)1

#define TX_Nearest                      (uint8)0
#define TX_Linear                       (uint8)1
#define TX_Bilinear                     (uint8)2
#define TX_QuasiTrilinear               (uint8)3

#define TX_PipSelectConst				(uint8)0
#define TX_PipSelectTexture				(uint8)1
#define TX_PipSelectColorTable			(uint8)2

#define TX_IsLiteral					0x08
#define TX_HasAlpha						0x04
#define TX_HasColor						0x02
#define TX_HasSsb						0x01

typedef enum Tx_Attribute {
	Tx_MinFilter = 0,
	Tx_MagFilter,
	Tx_InterFilter,
	Tx_TextureEnable,
	Tx_PipIndexOffset,
	Tx_PipColorSel,
	Tx_PipAlphaSel,
	Tx_PipSsbSel,
	Tx_PipConstSsb0,
	Tx_PipConstSsb1,
	Tx_FirstColor,
	Tx_SecondColor,
	Tx_ThirdColor,
	Tx_FirstAlpha,
	Tx_SecondAlpha,
	Tx_ColorOut,
	Tx_AlphaOut,
	Tx_BlendOp,
	Tx_BlendColorSsb0,
	Tx_BlendColorSsb1,
	TXA_NoMore
}Tx_Attribute;


typedef enum {
	DBLA_EnableAttrs,
	DBLA_ZBuffEnable,
	DBLA_ZBuffOutEnable,
	DBLA_WinClipInEnable,
	DBLA_WinClipOutEnable,
	DBLA_BlendEnable,
	DBLA_SrcInputEnable,
	DBLA_DitherEnable,
	DBLA_RGBADestOut,
	DBLA_Discard,
	DBLA_DiscardZClipped,
	DBLA_DiscardSSB0,
	DBLA_DiscardRGB0,
	DBLA_DiscardAlpha0,
	DBLA_XWinClipMin,
	DBLA_XWinClipMax,
	DBLA_YWinClipMin,
	DBLA_YWinClipMax,
	DBLA_ZCompareControl,
	DBLA_ZXOffset,
	DBLA_ZYOffset,
	DBLA_DSBConst,
	DBLA_DSBSelect,
	DBLA_RGBConstIn,
	DBLA_AInputSelect,
	DBLA_AMultCoefSelect,
	DBLA_AMultConstControl,
	DBLA_AMultRtJustify,
	DBLA_AMultConstSSB0,
	DBLA_AMultConstSSB1,
	DBLA_BInputSelect,
	DBLA_BMultCoefSelect,
	DBLA_BMultConstControl,
	DBLA_BMultRtJustify,
	DBLA_BMultConstSSB0,
	DBLA_BMultConstSSB1,
	DBLA_ALUOperation,
	DBLA_FinalDivide,
	DBLA_Alpha0ClampControl,
	DBLA_Alpha1ClampControl,
	DBLA_AlphaFracClampControl,
	DBLA_AlphaClampControl,
	DBLA_DestAlphaSelect,
	DBLA_DestAlphaConstSSB0,
	DBLA_DestAlphaConstSSB1,
	DBLA_DitherMatrixA,
	DBLA_DitherMatrixB,

/* Items below here are not available in the texture file format */

	DBLA_SrcPixels32Bit,
	DBLA_SrcBaseAddr,
	DBLA_SrcXStride,
	DBLA_SrcXOffset,
	DBLA_SrcYOffset,
	
	DBLA_NoMore
} CltDblAttribute;

#endif

/* TAB Function calls */

M2Err M2TXTA_RemoveAttr(M2TXTA *tab, uint32 attr);
M2Err M2TXTA_SetAttr(M2TXTA *tab, uint32 attr, uint32 value);
M2Err M2TXTA_GetAttr(M2TXTA *tab, uint32 attr, uint32 *value);
M2Err M2TX_CreateTAB(M2TX *tex);

/* DAB Function calls */

M2Err M2TXDB_RemoveAttr(M2TXDB *tab, uint32 attr);
M2Err M2TXDB_SetAttr(M2TXDB *tab, uint32 attr, uint32 value);
M2Err M2TXDB_GetAttr(M2TXDB *tab, uint32 attr, uint32 *value);

/* Load Rect Function calls */

M2Err M2TXLR_Append(M2TXLR *lR, M2TXRect rect);
M2Err M2TXLR_Remove(M2TXLR *lR, uint32 index);
M2Err M2TXLR_Set(M2TXLR *lr, uint32 index, M2TXRect rect);
M2Err M2TXLR_Get(M2TXLR *lr, uint32 index, M2TXRect *rect);
M2Err   M2TX_SetDefaultLoadRect(M2TX *tex);
