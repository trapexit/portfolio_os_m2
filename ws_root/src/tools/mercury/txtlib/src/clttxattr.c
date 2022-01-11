/*
 * File: @(#) clttxattr.c 95/08/24 1.14
 * Author: David Somayajulu
 * Copyright: The 3DO Company, June 28, 1995
 */

#include "kerneltypes.h"
#include "clt.h"
#include "clttxdblend.h"
#include "string.h"

/*
 * definitions for indices into the texture command list
 */

typedef enum {
	Tx_AddrControl=1,
	Tx_PipControl,
	Tx_ApplnControl,
	Tx_SrcExpConstant0=5,
	Tx_SrcExpConstant1,
	Tx_SrcExpConstant2,
	Tx_SrcExpConstant3,
	Tx_LastEntry
}TxRegIndex;

typedef struct TxAttr {
	uint32	attrMask;
	uint32	attrShift;
	TxRegIndex regIndex;
} TxAttr;


/*
 * Start address of relevant texture mapper registers
 */


#define TX_CMDLIST_CLEAR0_OFFSET	(Tx_AddrControl - 1)
#define TX_CMDLIST_CLEAR1_OFFSET	(Tx_ApplnControl + 1)
#define TX_CMDLIST_SET0_OFFSET		(Tx_SrcExpConstant3 + 1)
#define TX_CMDLIST_SET1_OFFSET		(TX_CMDLIST_SET0_OFFSET + \
							TX_CMDLIST_CLEAR1_OFFSET)

#define TX_CMD_CLEARREGISTER0		(\
					       ((Tx_ApplnControl-Tx_AddrControl)<<16)|\
					       (RA_TXTADDRCNTL|RC_CLEAR_REGISTER))
#define TX_CMD_CLEARREGISTER1		(\
						((Tx_SrcExpConstant3-Tx_SrcExpConstant0)<<16)|\
						(RA_TXTCONST0|RC_CLEAR_REGISTER))
#define TX_CMD_SETREGISTER0		(\
					((Tx_ApplnControl-Tx_AddrControl)<<16)|\
						(RA_TXTADDRCNTL|RC_SET_REGISTER))
#define TX_CMD_SETREGISTER1		(\
					((Tx_SrcExpConstant3-Tx_SrcExpConstant0)<<16)|\
						(RA_TXTCONST0|RC_SET_REGISTER))
	
#define SIZEOF_TXCMD_LIST		((Tx_LastEntry)*2)

static TxAttr texAttrTable[TXA_NoMore]= {
	{	/* Tx_MinFilter */
		CLT_Mask(TXTADDRCNTL,MINFILTER),
		CLT_Shift(TXTADDRCNTL,MINFILTER),
		Tx_AddrControl
	},
	{	/* Tx_MagFilter */
		CLT_Mask(TXTADDRCNTL,MAGFILTER),
		CLT_Shift(TXTADDRCNTL,MAGFILTER),
		Tx_AddrControl
	},
	{	/* Tx_InterFilter */
		CLT_Mask(TXTADDRCNTL,INTERFILTER),
		CLT_Shift(TXTADDRCNTL,INTERFILTER),
		Tx_AddrControl
	},
	{	/* Tx_TextureEnable */
		CLT_Mask(TXTADDRCNTL,TEXTUREENABLE),
		CLT_Shift(TXTADDRCNTL,TEXTUREENABLE),
		Tx_AddrControl
	},

	{	/* Tx_PipIndexOffset */
		CLT_Mask(TXTPIPCNTL,PIPINDEXOFFSET),
		CLT_Shift(TXTPIPCNTL,PIPINDEXOFFSET),
		Tx_PipControl
	},
	{	/* Tx_PipColorSel */
		CLT_Mask(TXTPIPCNTL,PIPCOLORSELECT),
		CLT_Shift(TXTPIPCNTL,PIPCOLORSELECT),
		Tx_PipControl
	},
	{	/* Tx_PipAlphaSel */
		CLT_Mask(TXTPIPCNTL,PIPALPHASELECT),
		CLT_Shift(TXTPIPCNTL,PIPALPHASELECT),
		Tx_PipControl
	},
	{	/* Tx_PipSSBSel */
		CLT_Mask(TXTPIPCNTL,PIPSSBSELECT),
		CLT_Shift(TXTPIPCNTL,PIPSSBSELECT),
		Tx_PipControl
	},

	{	/* Tx_PipConstSSB0 */
		0xFFFFFFFF,
		0,
		Tx_SrcExpConstant0
	},
	{	/* Tx_PipConstSSB1 */
		0xFFFFFFFF,
		0,
		Tx_SrcExpConstant1
	},

	{	/* Tx_FirstColor */
		CLT_Mask(TXTTABCNTL,FIRSTCOLOR),
		CLT_Shift(TXTTABCNTL,FIRSTCOLOR),
		Tx_ApplnControl
	},
	{	/* Tx_SecondColor */
		CLT_Mask(TXTTABCNTL,SECONDCOLOR),
		CLT_Shift(TXTTABCNTL,SECONDCOLOR),
		Tx_ApplnControl
	},
	{	/* Tx_ThirdColor */
		CLT_Mask(TXTTABCNTL,THIRDCOLOR),
		CLT_Shift(TXTTABCNTL,THIRDCOLOR),
		Tx_ApplnControl
	},
	{	/* Tx_FirstAlpha */
		CLT_Mask(TXTTABCNTL,FIRSTALPHA),
		CLT_Shift(TXTTABCNTL,FIRSTALPHA),
		Tx_ApplnControl
	},
	{	/* Tx_SecondAlpha */
		CLT_Mask(TXTTABCNTL,SECONDALPHA),
		CLT_Shift(TXTTABCNTL,SECONDALPHA),
		Tx_ApplnControl
	},
	{	/* Tx_ColorOut */
		CLT_Mask(TXTTABCNTL,COLOROUT),
		CLT_Shift(TXTTABCNTL,COLOROUT),
		Tx_ApplnControl
	},
	{	/* Tx_AlphaOut */
		CLT_Mask(TXTTABCNTL,ALPHAOUT),
		CLT_Shift(TXTTABCNTL,ALPHAOUT),
		Tx_ApplnControl
	},
	{	/* Tx_BlendOp */
		CLT_Mask(TXTTABCNTL,BLENDOP),
		CLT_Shift(TXTTABCNTL,BLENDOP),
		Tx_ApplnControl
	},

	{	/* Tx_BlendColorSSB0 */
		0xFFFFFFFF,
		0,
		Tx_SrcExpConstant2
	},
	{	/* Tx_BlendColorSSB1 */
		0xFFFFFFFF,
		0,
		Tx_SrcExpConstant3
	},
		
};

Err     CLT_SetTxAttribute(CltSnippet 		*textureAttrList,
						   CltTxAttribute   attrTag,
						   uint32          	attrValue)
{
	uint32 *txCmdList;
	uint32 *tmp;

	if (textureAttrList == NULL)
		return (-1);

	if ( (txCmdList = textureAttrList->data) == NULL){
		if (CLT_AllocSnippet(textureAttrList, SIZEOF_TXCMD_LIST) != 0) {
			return (-1);
		} else {
			txCmdList = textureAttrList->data;
			textureAttrList->size = SIZEOF_TXCMD_LIST;
			memset (txCmdList, 0, SIZEOF_TXCMD_LIST*4);
			*txCmdList = TX_CMD_CLEARREGISTER0;
			*(txCmdList + TX_CMDLIST_CLEAR1_OFFSET) = TX_CMD_CLEARREGISTER1;
			*(txCmdList + TX_CMDLIST_SET0_OFFSET) = TX_CMD_SETREGISTER0;
			*(txCmdList + TX_CMDLIST_SET1_OFFSET) = TX_CMD_SETREGISTER1;
			/* Hack in the LookupEnable bit of the Tx_AddrControl reg. */
#if 0
			*(txCmdList + TX_CMDLIST_CLEAR0_OFFSET + Tx_AddrControl) |=
			  CLT_Bits(TXTADDRCNTL,TEXTUREENABLE,1);
#endif
			*(txCmdList + TX_CMDLIST_SET0_OFFSET + Tx_AddrControl) |=
			  CLT_Bits(TXTADDRCNTL,TEXTUREENABLE,1);
;
		}
	}


	*(txCmdList + texAttrTable[attrTag].regIndex) |= texAttrTable[attrTag].attrMask;
	tmp = txCmdList + texAttrTable[attrTag].regIndex + TX_CMDLIST_SET0_OFFSET;
	*tmp = (*tmp&~texAttrTable[attrTag].attrMask) |
			(texAttrTable[attrTag].attrMask & 
				(attrValue << texAttrTable[attrTag].attrShift));
	return(0);
	
}

Err     CLT_GetTxAttribute(CltSnippet		*textureAttrList,
						   CltTxAttribute   attrTag,
						   uint32          	*attrValue)
{

	if ((textureAttrList == NULL) || (textureAttrList->data == NULL))
		return (-1);

	if (!(*(textureAttrList->data + texAttrTable[attrTag].regIndex) & 
	      texAttrTable[attrTag].attrMask))
	  return(-1);	  

	*attrValue = (*(textureAttrList->data + texAttrTable[attrTag].regIndex + 
			TX_CMDLIST_SET0_OFFSET) & texAttrTable[attrTag].attrMask ) >> texAttrTable[attrTag].attrShift;
	return (0);
}

Err     CLT_IgnoreTxAttribute(CltSnippet 		*textureAttrList,
							  CltTxAttribute	attrTag)
{

	if ((textureAttrList == NULL) || (textureAttrList->data == NULL))
		return (-1);

	*(textureAttrList->data + texAttrTable[attrTag].regIndex) = 0x0;
	*(textureAttrList->data + texAttrTable[attrTag].regIndex + 
					TX_CMDLIST_SET0_OFFSET) = 0x0;
	
	return (0);
}
