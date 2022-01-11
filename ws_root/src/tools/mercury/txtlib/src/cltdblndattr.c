/*
 * File: @(#) cltdblndattr.c 95/08/09 1.14
 * Author: David Somayajulu
 * Copyright: The 3DO Company, June 28, 1995
 */

#include "kerneltypes.h"
#include "clt.h"
#include "clttxdblend.h"
#include <string.h>

/*
 * definitions for indices into the destination blender command list
 */

typedef enum {
	Dblnd_UserGeneralControl = 1,
	Dblnd_DiscardControl,
	Dblnd_XWindowClip = (Dblnd_DiscardControl+2),
	Dblnd_YWindowClip,
	Dblnd_SourceControl = (Dblnd_YWindowClip+2),
	Dblnd_SourceBaseAddress,
	Dblnd_SourceXStride,
	Dblnd_SourceOffset,
	Dblnd_ZControl,
	Dblnd_ZOffset = Dblnd_ZControl+2,
	Dblnd_SSBDSBControl = (Dblnd_ZOffset+2),
	Dblnd_ConstantIn,
	Dblnd_TxtMultControl,
	Dblnd_TxtCoefConstant0,
	Dblnd_TxtCoefConstant1,
	Dblnd_SourceMultControl,
	Dblnd_SourceCoefConstant0,
	Dblnd_SourceCoefConstant1,
	Dblnd_ALUControl,
	Dblnd_SourceAlphaControl,
	Dblnd_DestAlphaControl,
	Dblnd_DestAlphaConstant,
	Dblnd_DitherMatrixA,
	Dblnd_DitherMatrixB,
	Dblnd_LastEntry
}DblndRegIndex;

typedef struct DblndAttr {
	uint32	attrMask;
	uint32	attrShift;
	DblndRegIndex regIndex;
} DblndAttr;


/*
 * Start address of relevant destination blender registers
 */

#define DBLND_CMDLIST_CLEAR0_OFFSET	(Dblnd_UserGeneralControl - 1)
#define DBLND_CMDLIST_CLEAR1_OFFSET	(Dblnd_DiscardControl + 1)
#define DBLND_CMDLIST_CLEAR2_OFFSET	(Dblnd_YWindowClip + 1)
#define DBLND_CMDLIST_CLEAR3_OFFSET	(Dblnd_ZControl + 1)
#define DBLND_CMDLIST_CLEAR4_OFFSET	(Dblnd_ZOffset + 1)

#define DBLND_CMDLIST_SET0_OFFSET	(Dblnd_DitherMatrixB + 1)
#define DBLND_CMDLIST_SET1_OFFSET	(DBLND_CMDLIST_SET0_OFFSET + \
							DBLND_CMDLIST_CLEAR1_OFFSET)
#define DBLND_CMDLIST_SET2_OFFSET	(DBLND_CMDLIST_SET0_OFFSET + \
							DBLND_CMDLIST_CLEAR2_OFFSET)
#define DBLND_CMDLIST_SET3_OFFSET	(DBLND_CMDLIST_SET0_OFFSET + \
							DBLND_CMDLIST_CLEAR3_OFFSET)
#define DBLND_CMDLIST_SET4_OFFSET	(DBLND_CMDLIST_SET0_OFFSET + \
							DBLND_CMDLIST_CLEAR4_OFFSET)

#define DBLND_CMD_CLEARREGISTER0		(\
			     ((Dblnd_DiscardControl-Dblnd_UserGeneralControl)<<16)|\
				       (RA_DBUSERCONTROL|RC_CLEAR_REGISTER))
#define DBLND_CMD_CLEARREGISTER1		(\
			     ((Dblnd_YWindowClip-Dblnd_XWindowClip)<<16)|\
				       (RA_DBXWINCLIP|RC_CLEAR_REGISTER))
#define DBLND_CMD_CLEARREGISTER2		(\
			     ((Dblnd_ZControl-Dblnd_SourceControl)<<16)|\
				       (RA_DBSRCCNTL|RC_CLEAR_REGISTER))
#define DBLND_CMD_CLEARREGISTER3		(\
			     ((Dblnd_ZOffset-Dblnd_ZOffset)<<16)|\
				       (RA_DBZOFFSET|RC_CLEAR_REGISTER))
#define DBLND_CMD_CLEARREGISTER4		(\
			     ((Dblnd_DitherMatrixB-Dblnd_SSBDSBControl)<<16)|\
				       (RA_DBSSBDSBCNTL|RC_CLEAR_REGISTER))

#define DBLND_CMD_SETREGISTER0		(\
			     ((Dblnd_DiscardControl-Dblnd_UserGeneralControl)<<16)|\
				       (RA_DBUSERCONTROL|RC_SET_REGISTER))
#define DBLND_CMD_SETREGISTER1		(\
			     ((Dblnd_YWindowClip-Dblnd_XWindowClip)<<16)|\
				       (RA_DBXWINCLIP|RC_SET_REGISTER))
#define DBLND_CMD_SETREGISTER2		(\
			     ((Dblnd_ZControl-Dblnd_SourceControl)<<16)|\
				       (RA_DBSRCCNTL|RC_SET_REGISTER))
#define DBLND_CMD_SETREGISTER3		(\
			     ((Dblnd_ZOffset-Dblnd_ZOffset)<<16)|\
				       (RA_DBZOFFSET|RC_SET_REGISTER))
#define DBLND_CMD_SETREGISTER4		(\
			     ((Dblnd_DitherMatrixB-Dblnd_SSBDSBControl)<<16)|\
				       (RA_DBSSBDSBCNTL|RC_SET_REGISTER))

	
#define SIZEOF_DBLNDCMD_LIST		((Dblnd_LastEntry)*2)

static DblndAttr dblndAttrTable[DBLA_NoMore]= {
	{	/* Dbl_EnableAttrs */
		(CLT_Mask(DBUSERCONTROL,ZBUFFEN) |
		 CLT_Mask(DBUSERCONTROL,ZOUTEN) |
		 CLT_Mask(DBUSERCONTROL,WINCLIPINEN) |
		 CLT_Mask(DBUSERCONTROL,WINCLIPOUTEN) |
		 CLT_Mask(DBUSERCONTROL,BLENDEN) |
		 CLT_Mask(DBUSERCONTROL,SRCEN) |
		 CLT_Mask(DBUSERCONTROL,DITHEREN) |
		 CLT_Mask(DBUSERCONTROL,DESTOUTMASK)),
		CLT_Shift(DBUSERCONTROL,DESTOUTMASK),
		Dblnd_UserGeneralControl
	},
	{	/* Dbl_ZBuffEnable */
		CLT_Mask(DBUSERCONTROL,ZBUFFEN),
		CLT_Shift(DBUSERCONTROL,ZBUFFEN),
		Dblnd_UserGeneralControl
	},
	{	/* Dbl_ZBuffOutEnable */
		 CLT_Mask(DBUSERCONTROL,ZOUTEN),
		CLT_Shift(DBUSERCONTROL,ZOUTEN),
		Dblnd_UserGeneralControl
	},
	{	/* Dbl_WinClipInEbale */
		 CLT_Mask(DBUSERCONTROL,WINCLIPINEN),
		CLT_Shift(DBUSERCONTROL,WINCLIPINEN),
		Dblnd_UserGeneralControl
	},
	{	/* Dbl_WinClipOutEnable */
		CLT_Mask(DBUSERCONTROL,WINCLIPOUTEN),
		CLT_Shift(DBUSERCONTROL,WINCLIPOUTEN),
		Dblnd_UserGeneralControl
	},
	{	/* Dbl_BlendEnable */
		CLT_Mask(DBUSERCONTROL,BLENDEN),
		CLT_Shift(DBUSERCONTROL,BLENDEN),
		Dblnd_UserGeneralControl
	},
	{	/* Dbl_SrcInputEnable */
		CLT_Mask(DBUSERCONTROL,SRCEN),
		CLT_Shift(DBUSERCONTROL,SRCEN),
		Dblnd_UserGeneralControl
	},
	{	/* Dbl_DitherEnable */
		CLT_Mask(DBUSERCONTROL,DITHEREN),
		CLT_Shift(DBUSERCONTROL,DITHEREN),
		Dblnd_UserGeneralControl
	},
	{	/* Dbl_RGBDestOut */
		CLT_Mask(DBUSERCONTROL,DESTOUTMASK),
		CLT_Shift(DBUSERCONTROL,DESTOUTMASK),
		Dblnd_UserGeneralControl
	},
	{
		/* Dbl_Discard */
		(CLT_Mask(DBDISCARDCONTROL,ZCLIPPED) |
		 CLT_Mask(DBDISCARDCONTROL,SSB0) |
		 CLT_Mask(DBDISCARDCONTROL,RGB0) |
		 CLT_Mask(DBDISCARDCONTROL,ALPHA0)),
		CLT_Shift(DBDISCARDCONTROL,ALPHA0),
		Dblnd_DiscardControl
	},
	{
		/* Dbl_DiscardZClipped */
		CLT_Mask(DBDISCARDCONTROL,ZCLIPPED),
		CLT_Shift(DBDISCARDCONTROL,ZCLIPPED),
		Dblnd_DiscardControl
	},
	{
		/* Dbl_DiscardSSB0 */
		 CLT_Mask(DBDISCARDCONTROL,SSB0),
		CLT_Shift(DBDISCARDCONTROL,SSB0),
		Dblnd_DiscardControl
	},
	{
		/* Dbl_Discard */
		 CLT_Mask(DBDISCARDCONTROL,RGB0),
		CLT_Shift(DBDISCARDCONTROL,RGB0),
		Dblnd_DiscardControl
	},
	{
		/* Dbl_DiscardAlpha0 */
		CLT_Mask(DBDISCARDCONTROL,ALPHA0),
		CLT_Shift(DBDISCARDCONTROL,ALPHA0),
		Dblnd_DiscardControl
	},
	{	/* Dbl_XWinClipMin */
		CLT_Mask(DBXWINCLIP,XWINCLIPMIN),
		CLT_Shift(DBXWINCLIP,XWINCLIPMIN),
		Dblnd_XWindowClip
	},
	{	/* Dbl_XWinClipMax */
		CLT_Mask(DBXWINCLIP,XWINCLIPMAX),
		CLT_Shift(DBXWINCLIP,XWINCLIPMAX),
		Dblnd_XWindowClip
	},
	{	/* Dbl_YWinClipMin */
		CLT_Mask(DBYWINCLIP,YWINCLIPMIN),
		CLT_Shift(DBYWINCLIP,YWINCLIPMIN),
		Dblnd_YWindowClip
	},
	{	/* Dbl_YWinClipMax */
		CLT_Mask(DBYWINCLIP,YWINCLIPMAX),
		CLT_Shift(DBYWINCLIP,YWINCLIPMAX),
		Dblnd_YWindowClip
	},
	{	/* Dbl_ZCompareControl */
		(CLT_Mask(DBZCNTL,ZUPDATEONGREATERZ) |
		 CLT_Mask(DBZCNTL,PIXOUTONGREATERZ) |
		 CLT_Mask(DBZCNTL,ZUPDATEONEQUALZ) |
		 CLT_Mask(DBZCNTL,PIXOUTONEQUALZ) |
		 CLT_Mask(DBZCNTL,ZUPDATEONSMALLERZ) |
		 CLT_Mask(DBZCNTL,PIXOUTONSMALLERZ)),
		CLT_Shift(DBZCNTL,PIXOUTONSMALLERZ),
		Dblnd_ZControl
	},
	{	/* Dbl_ZXOffset */
		CLT_Mask(DBZOFFSET,ZXOFFSET),
		CLT_Shift(DBZOFFSET,ZXOFFSET),
		Dblnd_ZOffset
	},
	{	/* Dbl_ZYOffset */
		CLT_Mask(DBZOFFSET,ZYOFFSET),
		CLT_Shift(DBZOFFSET,ZYOFFSET),
		Dblnd_ZOffset
	},
	{	/* Dbl_DSBConst */
		CLT_Mask(DBSSBDSBCNTL,DSBCONST),
		CLT_Shift(DBSSBDSBCNTL,DSBCONST),
		Dblnd_SSBDSBControl
	},
	{	/* Dbl_DSBSelect */
		CLT_Mask(DBSSBDSBCNTL,DSBSELECT),
		CLT_Shift(DBSSBDSBCNTL,DSBSELECT),
		Dblnd_SSBDSBControl
	},
	{	/* Dbl_RGBConstIn */
		(CLT_Mask(DBCONSTIN,RED) |
		 CLT_Mask(DBCONSTIN,GREEN) |
		 CLT_Mask(DBCONSTIN,BLUE)),
		CLT_Shift(DBCONSTIN,BLUE),
		Dblnd_ConstantIn
	},
	{	/* Dbl_AInputSelect */
		CLT_Mask(DBAMULTCNTL,AINPUTSELECT),
		CLT_Shift(DBAMULTCNTL,AINPUTSELECT),
		Dblnd_TxtMultControl
	},
	{	/* Dbl_AMultCoefSelect */
		CLT_Mask(DBAMULTCNTL,AMULTCOEFSELECT),
		CLT_Shift(DBAMULTCNTL,AMULTCOEFSELECT),
		Dblnd_TxtMultControl
	},
	{	/* Dbl_AMultConstSelect */
		CLT_Mask(DBAMULTCNTL,AMULTCONSTCONTROL),
		CLT_Shift(DBAMULTCNTL,AMULTCONSTCONTROL),
		Dblnd_TxtMultControl
	},
	{	/* Dbl_AMultRtJustify */
		CLT_Mask(DBAMULTCNTL,AMULTRJUSTIFY),
		CLT_Shift(DBAMULTCNTL,AMULTRJUSTIFY),
		Dblnd_TxtMultControl
	},
	{	/* Dbl_AMultConstSSB0 */
		(CLT_Mask(DBAMULTCONSTSSB0,RED)|
		 CLT_Mask(DBAMULTCONSTSSB0,GREEN)|
		 CLT_Mask(DBAMULTCONSTSSB0,BLUE)),
		CLT_Shift(DBAMULTCONSTSSB0,BLUE),
		Dblnd_TxtCoefConstant0
	},
	{	/* Dbl_AMultConstSSB1 */
		(CLT_Mask(DBAMULTCONSTSSB1,RED)|
		 CLT_Mask(DBAMULTCONSTSSB1,GREEN)|
		 CLT_Mask(DBAMULTCONSTSSB1,BLUE)),
		CLT_Shift(DBAMULTCONSTSSB1,BLUE),
		Dblnd_TxtCoefConstant1
	},
	{	/* Dbl_BInputSelect */
		CLT_Mask(DBBMULTCNTL,BINPUTSELECT),
		CLT_Shift(DBBMULTCNTL,BINPUTSELECT),
		Dblnd_SourceMultControl
	},
	{	/* Dbl_BMultCoefSelect */
		CLT_Mask(DBBMULTCNTL,BMULTCOEFSELECT),
		CLT_Shift(DBBMULTCNTL,BMULTCOEFSELECT),
		Dblnd_SourceMultControl
	},
	{	/* Dbl_BMultConstSelect */
		CLT_Mask(DBBMULTCNTL,BMULTCONSTCONTROL),
		CLT_Shift(DBBMULTCNTL,BMULTCONSTCONTROL),
		Dblnd_SourceMultControl
	},
	{	/* Dbl_BMultRtJustify */
		CLT_Mask(DBBMULTCNTL,BMULTRJUSTIFY),
		CLT_Shift(DBBMULTCNTL,BMULTRJUSTIFY),
		Dblnd_SourceMultControl
	},
	{	/* Dbl_BMultConstSSB0 */
		(CLT_Mask(DBBMULTCONSTSSB0,RED) |
		 CLT_Mask(DBBMULTCONSTSSB0,GREEN) |
		 CLT_Mask(DBBMULTCONSTSSB0,BLUE)),
		CLT_Shift(DBBMULTCONSTSSB0,BLUE),
		Dblnd_SourceCoefConstant0
	},
	{	/* Dbl_BMultConstSSB1 */
		(CLT_Mask(DBBMULTCONSTSSB1,RED) |
		 CLT_Mask(DBBMULTCONSTSSB1,GREEN) |
		 CLT_Mask(DBBMULTCONSTSSB1,BLUE)),
		CLT_Shift(DBBMULTCONSTSSB1,BLUE),
		Dblnd_SourceCoefConstant1
	},
	{	/* Dbl_ALUOperation */
		CLT_Mask(DBALUCNTL,ALUOPERATION),
		CLT_Shift(DBALUCNTL,ALUOPERATION),
		Dblnd_ALUControl
	},
	{	/* Dbl_FinalDivide */
		CLT_Mask(DBALUCNTL,FINALDIVIDE),
		CLT_Shift(DBALUCNTL,FINALDIVIDE),
		Dblnd_ALUControl
	},
	{	/* Dbl_Alpha0ClampControl */
		CLT_Mask(DBSRCALPHACNTL,ALPHA0),
		CLT_Shift(DBSRCALPHACNTL,ALPHA0),
		Dblnd_SourceAlphaControl
	},
	{	/* Dbl_Alpha1ClampControl */
		CLT_Mask(DBSRCALPHACNTL,ALPHA1),
		CLT_Shift(DBSRCALPHACNTL,ALPHA1),
		Dblnd_SourceAlphaControl
	},
	{	/* Dbl_AlphaFracClampControl */
		CLT_Mask(DBSRCALPHACNTL,ALPHAFRAC),
		CLT_Shift(DBSRCALPHACNTL,ALPHAFRAC),
		Dblnd_SourceAlphaControl
	},
	{	/* Dbl_AlphaClampControl */
		CLT_Mask(DBSRCALPHACNTL,ACLAMP),
		CLT_Shift(DBSRCALPHACNTL,ACLAMP),
		Dblnd_SourceAlphaControl
	},
	{	/* Dbl_DestAlphaSelect */
		CLT_Mask(DBDESTALPHACNTL,DESTCONSTSELECT),
		CLT_Shift(DBDESTALPHACNTL,DESTCONSTSELECT),
		Dblnd_DestAlphaControl
	},
	{	/* Dbl_DestAlphaConstSSB0 */
		CLT_Mask(DBDESTALPHACONST,DESTALPHACONSTSSB0),
		CLT_Shift(DBDESTALPHACONST,DESTALPHACONSTSSB0),
		Dblnd_DestAlphaConstant
	},
	{	/* Dbl_DestAlphaConstSSB1 */
		CLT_Mask(DBDESTALPHACONST,DESTALPHACONSTSSB1),
		CLT_Shift(DBDESTALPHACONST,DESTALPHACONSTSSB1),
		Dblnd_DestAlphaConstant
	},
	{	/* Dbl_DitherMatrixA */
		0xFFFFFFFF,
		0,
		Dblnd_DitherMatrixA
	},
	{	/* Dbl_DitherMatrixB */
		0xFFFFFFFF,
		0,
		Dblnd_DitherMatrixB
	},

/* Items below here are not available in the texture file format */

	{	/* Dbl_SrcPixels32Bit */
		CLT_Mask(DBSRCCNTL,SRC32BPP),
		CLT_Shift(DBSRCCNTL,SRC32BPP),
		Dblnd_SourceControl
	},
	{	/* Dbl_SrcBaseAddr */
		0xFFFFFFFF,
		0,
		Dblnd_SourceBaseAddress
	},
	{	/* Dbl_SrcXStride */
		CLT_Mask(DBSRCXSTRIDE,XSTRIDE),
		CLT_Shift(DBSRCXSTRIDE,XSTRIDE),
		Dblnd_SourceXStride
	},
	{	/* Dbl_SrcXOffset */
		CLT_Mask(DBSRCOFFSET,XOFFSET),
		CLT_Shift(DBSRCOFFSET,XOFFSET),
		Dblnd_SourceOffset
	},
	{	/* Dbl_SrcYOffset */
		CLT_Mask(DBSRCOFFSET,YOFFSET),
		CLT_Shift(DBSRCOFFSET,YOFFSET),
		Dblnd_SourceOffset
	},
		
};

Err     CLT_SetDblAttribute(CltSnippet		*dblendAttrList,
							CltDblAttribute attrTag,
							uint32        	attrValue)
{
	uint32 *dblndCmdList;
	uint32 *tmp;

	/* PASS 1 workaround... Remove for PASS 2.  -NBT */
	if (attrTag == DBLA_SrcBaseAddr)
		attrValue += 0x01000000;

	if (dblendAttrList == NULL)
		return (-1);
	if ( (dblndCmdList = dblendAttrList->data) == NULL){
		if (CLT_AllocSnippet(dblendAttrList, SIZEOF_DBLNDCMD_LIST) != 0) {
			return (-1);
		} else {
			dblndCmdList = dblendAttrList->data;
			dblendAttrList->size = SIZEOF_DBLNDCMD_LIST;
			memset (dblndCmdList, 0, SIZEOF_DBLNDCMD_LIST*4);
			*dblndCmdList = DBLND_CMD_CLEARREGISTER0;
			*(dblndCmdList + DBLND_CMDLIST_CLEAR1_OFFSET) = 
							DBLND_CMD_CLEARREGISTER1;
			*(dblndCmdList + DBLND_CMDLIST_CLEAR2_OFFSET) = 
							DBLND_CMD_CLEARREGISTER2;
			*(dblndCmdList + DBLND_CMDLIST_CLEAR3_OFFSET) = 
							DBLND_CMD_CLEARREGISTER3;
			*(dblndCmdList + DBLND_CMDLIST_CLEAR4_OFFSET) = 
							DBLND_CMD_CLEARREGISTER4;

			*(dblndCmdList + DBLND_CMDLIST_SET0_OFFSET) = 
							DBLND_CMD_SETREGISTER0;
			*(dblndCmdList + DBLND_CMDLIST_SET1_OFFSET) = 
							DBLND_CMD_SETREGISTER1;
			*(dblndCmdList + DBLND_CMDLIST_SET2_OFFSET) = 
							DBLND_CMD_SETREGISTER2;
			*(dblndCmdList + DBLND_CMDLIST_SET3_OFFSET) = 
							DBLND_CMD_SETREGISTER3;
			*(dblndCmdList + DBLND_CMDLIST_SET4_OFFSET) = 
							DBLND_CMD_SETREGISTER4;
		}
	}


	*(dblndCmdList + dblndAttrTable[attrTag].regIndex) |= 
						dblndAttrTable[attrTag].attrMask;
	tmp = dblndCmdList + dblndAttrTable[attrTag].regIndex + DBLND_CMDLIST_SET0_OFFSET; 
	*tmp = (*tmp&~dblndAttrTable[attrTag].attrMask) |
			(dblndAttrTable[attrTag].attrMask & 
				(attrValue << dblndAttrTable[attrTag].attrShift));
	return(0);
	
}

Err     CLT_GetDblAttribute(CltSnippet 		*dblendAttrList,
							CltDblAttribute attrTag,
							uint32          *attrValue)
{
	if ((dblendAttrList == NULL) || (dblendAttrList->data == NULL))
		return (-1);

	if (!(*(dblendAttrList->data + dblndAttrTable[attrTag].regIndex) & 
	      dblndAttrTable[attrTag].attrMask))
	  return(-1);	  

	*attrValue = (*(dblendAttrList->data + dblndAttrTable[attrTag].regIndex + 
			DBLND_CMDLIST_SET0_OFFSET) & dblndAttrTable[attrTag].attrMask) >> 
				dblndAttrTable[attrTag].attrShift;

	/* PASS 1 workaround.  Remove for PASS 2 */
	if (attrTag == DBLA_SrcBaseAddr)
		*attrValue -= 0x01000000;

	return (0);
}

Err     CLT_IgnoreDblAttribute(CltSnippet 		*dblendAttrList,
							   CltDblAttribute 	attrTag)
{

	if ((dblendAttrList == NULL) || (dblendAttrList->data == NULL))
		return (-1);

	*(dblendAttrList->data + dblndAttrTable[attrTag].regIndex) = 0x0;
	*(dblendAttrList->data + dblndAttrTable[attrTag].regIndex + 
					DBLND_CMDLIST_SET0_OFFSET) = 0x0;
	
	return (0);
}
