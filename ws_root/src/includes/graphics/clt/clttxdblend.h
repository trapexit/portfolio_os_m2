#ifndef __GRAPHICS_CLT_CLTTXDBLEND_H
#define __GRAPHICS_CLT_CLTTXDBLEND_H


/******************************************************************************
**
**  @(#) clttxdblend.h 96/02/17 1.27
**
**  WARNING: The attributes and values specified in this file are used by the
**  Tools group in generating UTF texture files. Any changes to this file
**  should be communicated to the tools group and should be approved before
**  being made.
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif

#ifndef __GRAPHICS_CLT_CLT_H
#include <graphics/clt/clt.h>
#endif


/********************************************************************************
 * Texture Mapper Related Definitions						*
 ********************************************************************************/

typedef enum  {
	TXA_MinFilter = 0,
	TXA_MagFilter,
	TXA_InterFilter,
	TXA_TextureEnable,
	TXA_PipIndexOffset,
	TXA_PipColorSelect,
	TXA_PipAlphaSelect,
	TXA_PipSSBSelect,
	TXA_PipConstSSB0,
	TXA_PipConstSSB1,
	TXA_FirstColor,
	TXA_SecondColor,
	TXA_ThirdColor,
	TXA_FirstAlpha,
	TXA_SecondAlpha,
	TXA_ColorOut,
	TXA_AlphaOut,
	TXA_BlendOp,
	TXA_BlendColorSSB0,
	TXA_BlendColorSSB1,

	TXA_NoMore
} CltTxAttribute;

/*
 * Texture blend info constants
 */
#define TX_ColorSelectConstColor    RC_TXTTABCNTL_FIRSTCOLOR_CONSTCOLOR
#define TX_ColorSelectConstAlpha    RC_TXTTABCNTL_FIRSTCOLOR_CONSTALPHA
#define TX_ColorSelectTexColor      RC_TXTTABCNTL_FIRSTCOLOR_TEXCOLOR
#define TX_ColorSelectTexAlpha      RC_TXTTABCNTL_FIRSTCOLOR_TEXALPHA
#define TX_ColorSelectPrimColor     RC_TXTTABCNTL_FIRSTCOLOR_PRIMCOLOR
#define TX_ColorSelectPrimAlpha     RC_TXTTABCNTL_FIRSTCOLOR_PRIMALPHA

#define TX_AlphaSelectPrimAlpha     RC_TXTTABCNTL_FIRSTALPHA_PRIMALPHA
#define TX_AlphaSelectTexAlpha      RC_TXTTABCNTL_FIRSTALPHA_TEXALPHA
#define TX_AlphaSelectConstAlpha    RC_TXTTABCNTL_FIRSTALPHA_CONSTALPHA

#define TX_BlendOutSelectPrim       RC_TXTTABCNTL_COLOROUT_PRIMCOLOR
#define TX_BlendOutSelectTex        RC_TXTTABCNTL_COLOROUT_TEXCOLOR
#define TX_BlendOutSelectBlend      RC_TXTTABCNTL_COLOROUT_BLEND

#define TX_BlendOpLerp              RC_TXTTABCNTL_BLENDOP_LERP
#define TX_BlendOpMult              RC_TXTTABCNTL_BLENDOP_MULT

/*
 * Texture render constants
 */
#define TX_WrapModeClamp            0
#define TX_WrapModeTile             1

#define TX_Nearest                  RC_TXTADDRCNTL_INTERFILTER_POINT
#define TX_Linear                   RC_TXTADDRCNTL_INTERFILTER_LINEAR
#define TX_Bilinear                 RC_TXTADDRCNTL_INTERFILTER_BILINEAR
#define TX_QuasiTrilinear           RC_TXTADDRCNTL_INTERFILTER_TRILINEAR

#define TX_PipSelectConst			RC_TXTPIPCNTL_PIPCOLORSELECT_CONSTANT
#define TX_PipSelectTexture			RC_TXTPIPCNTL_PIPCOLORSELECT_TEXTURE
#define TX_PipSelectColorTable		RC_TXTPIPCNTL_PIPCOLORSELECT_PIP

/*
 * Interface Functions
 */

/*
 * Function:
 * Free up the memory associated with the attribute list
 *
 * Return value:
 *	0 is returned if there are no errors.
 * 	negative number is returned on error.
 */
Err CLT_FreeTxAttributeList (uint32 *textureAttrList);


/*
 * Function:
 * Free up the memory associated with the attribute list
 *
 * Return value:
 *	0 is returned if there are no errors.
 * 	negative number is returned on error.
 */
Err CLT_FreeDblAttributeList (uint32 *dblendAttrList);

/*
 * Function:
 * If "textureAttrList is NULL, a new command list is allocated and the register
 * fields corresponding to attrTag are set to attrValue. The start address of the
 * command list is returned in "textureAttrList".
 *
 * If "textureAttrList is not NULL, the register fields corresponding to attrTag
 * are set to attrValue.
 *
 * Return Value:
 *	0 is returned if there are no errors.
 * 	negative number is returned on error.
 */

Err	CLT_SetTxAttribute(CltSnippet 		*textureAttrList,
					   CltTxAttribute	attrTag,
					   uint32			attrValue);


/*
 * Function:
 * 	If textureAttrList is NULL, a negative valued error code is returned.
 *	Otherwise the value corresponding to attrTag is returned in attrValue.
 *
 * Return Value:
 *	0 is returned if the function completes successfully.
 *	a negative number is returned on error.
 */

Err	CLT_GetTxAttribute(CltSnippet		*textureAttrList,
					   CltTxAttribute	attrTag,
					   uint32          	*attrValue);

/*
 * Function:
 * 	If textureAttrList is NULL, a negative valued error code is returned.
 *	Otherwise the fields in the registers corresponding to attrTag are set
 *	to be left unchanged in the command list.
 *
 * Return Value:
 *	0 is returned if the function completes successfully.
 *	a negative number is returned on error.
 */

Err	CLT_IgnoreTxAttribute(CltSnippet 		*textureAttrList,
						  CltTxAttribute 	attrTag);


/*****************************************************************************
 * Destination Blender Related Definitions					*
 *****************************************************************************/


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

/*
 * Destination blend info constants
 *
 * defines for arithmatic operations.
 */
/* DBLA_EnableAttrs constants */

#define		DBL_ZBuffEnable			CLT_Mask(DBUSERCONTROL, ZBUFFEN)
#define		DBL_ZBuffOutEnable		CLT_Mask(DBUSERCONTROL, ZOUTEN)
#define		DBL_WinClipInEnable		CLT_Mask(DBUSERCONTROL, WINCLIPINEN)
#define		DBL_WinClipOutEnable	CLT_Mask(DBUSERCONTROL, WINCLIPOUTEN)
#define		DBL_BlendEnable			CLT_Mask(DBUSERCONTROL, BLENDEN)
#define		DBL_SrcInputEnable		CLT_Mask(DBUSERCONTROL, SRCEN)
#define		DBL_DitherEnable		CLT_Mask(DBUSERCONTROL, DITHEREN)
#define		DBL_AlphaDestOut		CLT_Const(DBUSERCONTROL, DESTOUTMASK, ALPHA)
#define		DBL_RDestOut			CLT_Const(DBUSERCONTROL, DESTOUTMASK, RED)
#define		DBL_GDestOut			CLT_Const(DBUSERCONTROL, DESTOUTMASK, GREEN)
#define		DBL_BDestOut			CLT_Const(DBUSERCONTROL, DESTOUTMASK, BLUE)
#define		DBL_RGBDestOut			(DBL_RDestOut|DBL_GDestOut|DBL_BDestOut)

/* Dbl_Discard constants */

#define		DBL_DiscardZClipped		CLT_Mask(DBDISCARDCONTROL, ZCLIPPED)
#define		DBL_DiscardSSB0			CLT_Mask(DBDISCARDCONTROL, SSB0)
#define		DBL_DiscardRGB0			CLT_Mask(DBDISCARDCONTROL, RGB0)
#define		DBL_DiscardAlpha0		CLT_Mask(DBDISCARDCONTROL, ALPHA0)

/* Dbl_ZCompareControl constants */

#define		DBL_PixOutOnSmallerZ	CLT_Mask(DBZCNTL, PIXOUTONSMALLERZ)
#define		DBL_ZUpdateOnSmallerZ	CLT_Mask(DBZCNTL, ZUPDATEONSMALLERZ)
#define		DBL_PixOutOnEqualZ		CLT_Mask(DBZCNTL, PIXOUTONEQUALZ)
#define		DBL_ZUpdateOnEqualZ		CLT_Mask(DBZCNTL, ZUPDATEONEQUALZ)
#define		DBL_PixOutOnGreaterZ	CLT_Mask(DBZCNTL, PIXOUTONGREATERZ)
#define		DBL_ZUpdateOnGreaterZ	CLT_Mask(DBZCNTL, ZUPDATEONGREATERZ)

/* Dbl_DSBSelect constants */

#define		DBL_DSBSelectObjSSB		CLT_Const(DBSSBDSBCNTL, DSBSELECT, OBJSSB)
#define		DBL_DSBSelectConst		CLT_Const(DBSSBDSBCNTL, DSBSELECT, CONST)
#define		DBL_DSBSelectSrcSSB		CLT_Const(DBSSBDSBCNTL, DSBSELECT, SRCSSB)

/* Dbl_AInputSelect constants */

#define		DBL_ASelectTexColor				CLT_Const(DBAMULTCNTL, AINPUTSELECT, TEXCOLOR)
#define		DBL_ASelectConstColor			CLT_Const(DBAMULTCNTL, AINPUTSELECT, CONSTCOLOR)
#define		DBL_ASelectSrcColorComplement	CLT_Const(DBAMULTCNTL, AINPUTSELECT, SRCCOLORCOMPLEMENT)
#define		DBL_ASelectTexAlpha				CLT_Const(DBAMULTCNTL, AINPUTSELECT, TEXALPHA)

/* Dbl_AMultCoefSelect constants */

#define		DBL_MASelectTexAlpha			CLT_Const(DBAMULTCNTL, AMULTCOEFSELECT, TEXALPHA)
#define		DBL_MASelectSrcalpha			CLT_Const(DBAMULTCNTL, AMULTCOEFSELECT, SRCALPHA)
#define		DBL_MASelectConst				CLT_Const(DBAMULTCNTL, AMULTCOEFSELECT, CONST)
#define		DBL_MASelectSrcColor			CLT_Const(DBAMULTCNTL, AMULTCOEFSELECT, SRCCOLOR)
#define		DBL_MASelectTexAlphaComplement	CLT_Const(DBAMULTCNTL, AMULTCOEFSELECT, TEXALPHACOMPLEMENT)
#define		DBL_MASelectSrcAlphaComplement	CLT_Const(DBAMULTCNTL, AMULTCOEFSELECT, SRCALPHACOMPLEMENT)
#define		DBL_MASelectConstComplement		CLT_Const(DBAMULTCNTL, AMULTCOEFSELECT, CONSTCOMPLEMENT)
#define		DBL_MASelectSrcColorComplement	CLT_Const(DBAMULTCNTL, AMULTCOEFSELECT, SRCCOLORCOMPLEMENT)

/* Dbl_AMultConstControl constants */

#define		DBL_MAConstByTexSSB				CLT_Const(DBAMULTCNTL,AMULTCONSTCONTROL,TEXSSB)
#define		DBL_MAConstBySrcSSB				CLT_Const(DBAMULTCNTL,AMULTCONSTCONTROL,SRCSSB)

/* Dbl_BInputSelect constants */

#define		DBL_BSelectSrcColor				CLT_Const(DBBMULTCNTL, BINPUTSELECT, SRCCOLOR)
#define		DBL_BSelectConstColor			CLT_Const(DBBMULTCNTL, BINPUTSELECT, CONSTCOLOR)
#define		DBL_BSelectTexColorComplement	CLT_Const(DBBMULTCNTL, BINPUTSELECT, TEXCOLORCOMPLEMENT)
#define		DBL_BSelectSrcAlpha				CLT_Const(DBBMULTCNTL, BINPUTSELECT, SRCALPHA)

/* Dbl_BMultCoefSelect constants */

#define		DBL_MBSelectTexAlpha			CLT_Const(DBBMULTCNTL, BMULTCOEFSELECT, TEXALPHA)
#define		DBL_MBSelectSrcAlpha			CLT_Const(DBBMULTCNTL, BMULTCOEFSELECT, SRCALPHA)
#define		DBL_MBSelectConst				CLT_Const(DBBMULTCNTL, BMULTCOEFSELECT, CONST)
#define		DBL_MBSelectTexColor			CLT_Const(DBBMULTCNTL, BMULTCOEFSELECT, TEXCOLOR)
#define		DBL_MBSelectTexAlphaComplement	CLT_Const(DBBMULTCNTL, BMULTCOEFSELECT, TEXALPHACOMPLEMENT)
#define		DBL_MBSelectSrcAlphaComplement	CLT_Const(DBBMULTCNTL, BMULTCOEFSELECT, SRCALPHACOMPLEMENT)
#define		DBL_MBSelectConstComplement		CLT_Const(DBBMULTCNTL, BMULTCOEFSELECT, CONSTCOMPLEMENT)
#define		DBL_MBSelectTexColorComplement	CLT_Const(DBBMULTCNTL, BMULTCOEFSELECT, TEXCOLORCOMPLEMENT)

/* Dbl_BMultConstControl constants */

#define		DBL_MBConstByTexSSB				CLT_Const(DBBMULTCNTL,BMULTCONSTCONTROL,TEXSSB)
#define		DBL_MBConstBySrcSSB				CLT_Const(DBBMULTCNTL,BMULTCONSTCONTROL,SRCSSB)

/* Dbl_ALUOperation constants */

#define		DBL_Add							CLT_Const(DBALUCNTL,ALUOPERATION,A_PLUS_B)
#define		DBL_AddClamp					CLT_Const(DBALUCNTL,ALUOPERATION,A_PLUS_BCLAMP)
#define		DBL_Subtract					CLT_Const(DBALUCNTL,ALUOPERATION,A_MINUS_B)
#define		DBL_SubtractClamp				CLT_Const(DBALUCNTL,ALUOPERATION,A_MINUS_BCLAMP)
#define		DBL_SubtractFromB				CLT_Const(DBALUCNTL,ALUOPERATION,B_MINUS_A)
#define		DBL_SubtractFromBClamp			CLT_Const(DBALUCNTL,ALUOPERATION,B_MINUS_ACLAMP)
#define		DBL_OutputZero					CLT_Const(DBALUCNTL,ALUOPERATION,OUTPUTZERO)
#define		DBL_Neither						CLT_Const(DBALUCNTL,ALUOPERATION,NEITHER)
#define		DBL_NotA_AND_B					CLT_Const(DBALUCNTL,ALUOPERATION,NOTA_AND_B)
#define		DBL_NotA						CLT_Const(DBALUCNTL,ALUOPERATION,NOTA)
#define		DBL_NotB_AND_A					CLT_Const(DBALUCNTL,ALUOPERATION,NOTB_AND_A)
#define		DBL_NotB						CLT_Const(DBALUCNTL,ALUOPERATION,NOTB)
#define		DBL_XOR							CLT_Const(DBALUCNTL,ALUOPERATION,XOR)
#define		DBL_Not_A_AND_B					CLT_Const(DBALUCNTL,ALUOPERATION,NOT_A_AND_B)
#define		DBL_A_AND_B						CLT_Const(DBALUCNTL,ALUOPERATION,A_AND_B)
#define		DBL_OneOnEqual					CLT_Const(DBALUCNTL,ALUOPERATION,ONEONEQUAL)
#define		DBL_B							CLT_Const(DBALUCNTL,ALUOPERATION,B)
#define		DBL_NotA_OR_B					CLT_Const(DBALUCNTL,ALUOPERATION,NOTA_OR_B)
#define		DBL_A							CLT_Const(DBALUCNTL,ALUOPERATION,A)
#define		DBL_NotB_OR_A					CLT_Const(DBALUCNTL,ALUOPERATION,NOTB_OR_A)
#define		DBL_A_OR_B						CLT_Const(DBALUCNTL,ALUOPERATION,A_OR_B)
#define		DBL_OutputOne					CLT_Const(DBALUCNTL,ALUOPERATION,OUTPUTONE)

/* Dbl_Alpha[0|1|Frac]ClampControl 0 */

#define		DBL_AlphaClampLeaveAlone		CLT_Const(DBSRCALPHACNTL,ACLAMP,LEAVEALONE)
#define		DBL_AlphaClampForceTo1			CLT_Const(DBSRCALPHACNTL,ACLAMP,FORCE1)
#define		DBL_AlphaClampForceTo0			CLT_Const(DBSRCALPHACNTL,ACLAMP,FORCE0)

/* Dbl_DestAlphaSelect constants */

#define		DBL_DestAlphaSelectTexAlpha		CLT_Const(DBDESTALPHACNTL,DESTCONSTSELECT,TEXALPHA)
#define		DBL_DestAlphaSelectTexSSBConst	CLT_Const(DBDESTALPHACNTL,DESTCONSTSELECT,TEXSSBCONST)
#define		DBL_DestAlphaSelectSrcSSBConst	CLT_Const(DBDESTALPHACNTL,DESTCONSTSELECT,SRCSSBCONST)
#define		DBL_DestAlphaSelectSrcAlpha		CLT_Const(DBDESTALPHACNTL,DESTCONSTSELECT,SRCALPHA)
#define		DBL_DestAlphaSelectRBlend		CLT_Const(DBDESTALPHACNTL,DESTCONSTSELECT,RBLEND)

/*
 * Interface Functions
 */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Function:
 * If "*dblendAttrList is NULL, a new command list is allocated and the register
 * fields corresponding to attrTag are set to attrValue. The start address of the
 * command list is returned in "dblendAttrList".
 *
 * If "*dblendAttrList is not NULL, the register fields corresponding to attrTag
 * are set to attrValue.
 *
 * Return Value:
 *	0 is returned if there are no errors.
 * 	negative number is returned on error.
 */

Err	CLT_SetDblAttribute(CltSnippet 		*dblendAttrList,
						CltDblAttribute	attrTag,
						uint32			attrValue);


/*
 * Function:
 * 	If dblendAttrList is NULL, a negative valued error code is returned.
 *	Otherwise the value corresponding to attrTag is returned in attrValue.
 *
 * Return Value:
 *	0 is returned if the function completes successfully.
 *	a negative number is returned on error.
 */

Err	CLT_GetDblAttribute(CltSnippet 		*dblendAttrList,
						CltDblAttribute	attrTag,
						uint32         	*attrValue);

/*
 * Function:
 * 	If dblendAttrList is NULL, a negative valued error code is returned.
 *	Otherwise the fields in the registers corresponding to attrTag are set
 *	to be left unchanged in the command list.
 *
 * Return Value:
 *	0 is returned if the function completes successfully.
 *	a negative number is returned on error.
 */

Err	CLT_IgnoreDblAttribute(CltSnippet 		*dblendAttrList,
						   CltDblAttribute 	attrTag);


/********************************************************************************
 * Pip Loading Command List Related Definitions					*
 ********************************************************************************/

typedef struct CltPipControlBlock {
	void		*pipData;		/* Ptr to PIP data in memory */
	uint16		pipIndex;		/* index into the PIP (in number of entries)
								 * from where the PIP data is to be loaded */
	uint16		pipNumEntries;	/* Number of entries in the PIP table */
	CltSnippet	pipCommandList;	/* Command list snippet for loading the PIP*/
} CltPipControlBlock;


/*
 * Function:
 *	Computes the number of bytes to load the given PIP
 * Return Value:
 *	size of the command list to load the PIP in number of bytes
 *	a negative number on error
 */
#define	CLT_GetPipLoadCmdListSize(pcb) (CLT_GetSize((pcb).pipCommandList))

/*
 * Function:
 *	If tx1PipCB->pipCommandList->data is Null, a new command list buffer is allocated
 *	for loading the PIP. Otherwise, it is assumed the buffer to fill in the
 *	command list provided.
 *	The command list is then filled in with the supplied data
 * Return Value:
 * 	0 if there are no errors
 *	a negative number on error
 */
Err CLT_CreatePipCommandList(CltPipControlBlock* txPipCB);

/*
 * Function: CLT_ComputePipLoadCmdListSize
 *  Returns the size that will be needed to do a PIP load for a given
 *  PipControlBlock.  Returns a negative number if an error occurred.
 */
int32 CLT_ComputePipLoadCmdListSize(CltPipControlBlock* txPipCB);

/********************************************************************************
 * Texture Loading Command List Related Definitions				*
 ********************************************************************************/

typedef struct CltTxLoadControlBlock {
	void 		*textureBlock;
	uint16		firstLOD;
	uint16		numLOD;
	uint16		XWrap;
	uint16		YWrap;
	uint16		XSize;			/* For carving. Normally = to txtrBlock-> */
	uint16		YSize;			/*   minx and miny, can be made smaller */
	uint16		XOffset;		/* For carving. 0=whole txtr, else these are */
	uint16		YOffset;		/*   the top/left of the sub-texture to load */
	uint32		tramOffset;		/* Where in the TRAM to place texture */
	uint32		tramSize;		/* Size texture needs in TRAM */
	CltSnippet	lcbCommandList; /* Command list to load and use the texture */
	CltSnippet	useCommandList;	/* Commandlist just to use the texture */
} CltTxLoadControlBlock;

/*
 *	Returns the number of bytes to load the given texture
 * Return Value:
 *	size of the command list to load the texture in number of bytes
 *	a negative number on error
 */
#define	CLT_GetTxLoadCmdListSize(txLCB) (CltGetSize((txLCB)->lcbCommandList))

/*
 * Function: CLT_ComputeTxLoadCmdListSize
 *  Returns the size that will be needed to do a texture load for a given
 *  texture block.  Returns a negative number if an error occurred.  This
 *  routine can be used instead of the macro above when the size is needed,
 *  but the command list hasn't been computed yet.
 */
int32 CLT_ComputeTxLoadCmdListSize(CltTxLoadControlBlock* txLoadCB);


/*
 * Function: CLT_CreateTxLoadCommandList
 *	If txLoadCB->lcbCommandList->data is Null, a new command list buffer is allocated
 *	for loading the Texture. Otherwise, it is assumed the buffer to fill in the
 *	command list is provided.  If the buffer is not large enough, it will be
 *  resized.
 *	The command list is then filled in with the supplied data
 * Return Value:
 *	0 if there are no errors
 *	a negative number on error
 */
Err CLT_CreateTxLoadCommandList(CltTxLoadControlBlock *txLoadCB);

/********************************************************************************
 * Other Definitions								*
 ********************************************************************************/

typedef struct CltTxLOD {
	uint32 	 texelDataSize;
	void    *texelData;
} CltTxLOD;

typedef struct CltTxDCI {
	uint16  texelFormat[4];
    uint32  expColor[4];
} CltTxDCI;

typedef struct CltTxData {
	uint32		flags;		/* compressed or not compressed */
	uint32		minX;		/* width in pixels of the smallest LOD */
	uint32		minY;		/* height in pixels of the smallest LOD */
	uint32		maxLOD;		/* maximum number of LODs */
	uint32		bitsPerPixel;	/* number of bits per pixel */
	uint32		expansionFormat; /* format of texels in TRAM */
	CltTxLOD	*texelData; /* pointer to each level of detail */
	CltTxDCI	*dci; /* pointer to data compression information */
} CltTxData;

#ifdef __cplusplus
}
#endif


#endif /* __GRAPHICS_CLT_CLTTXDBLEND_H */
