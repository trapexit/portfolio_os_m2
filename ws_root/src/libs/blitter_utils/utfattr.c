/***************************************************************************
**
** @(#) utfattr.c 96/09/16 1.1
**
**  Code to convert a texture or dblend attribute in a UTF file into
**  CLT values.
**
****************************************************************************/

#include <kernel/types.h>
#include <graphics/clt/cltmacros.h>
#include <graphics/clt/clttxdblend.h>
#include <graphics/blitter.h>

#define TA_TBL 0
#define TA_PIP 1
#define DB_DBL 0
#define UNSUPPORTED -1


/* The following converts the attribute values in the UTF files into
 * CLT values to set in the BlitObject snippets.
 */

typedef struct AttrTable
{
    int32 snippet;             /* which of the two snippets */
    uint32 offset;              /* offset into that snippet to change */
    uint32 mask;                /* mask these bits */
    uint32 shift;               /* shift value this much */
} AttrTable;

static const AttrTable taTable[] =
{
    {                           /* Tx_MinFilter */
        TA_TBL,
        offsetof(TxBlendSnippet, txb_addrCntl),
        CLT_Mask(TXTADDRCNTL, MINFILTER),
        CLT_Shift(TXTADDRCNTL, MINFILTER),
    },
    {                           /* Tx_MagFilter */
        TA_TBL,
        offsetof(TxBlendSnippet, txb_addrCntl),
        CLT_Mask(TXTADDRCNTL, MAGFILTER),
        CLT_Shift(TXTADDRCNTL, MAGFILTER),
    },
    {                           /* Tx_InterFilter */
        TA_TBL,
        offsetof(TxBlendSnippet, txb_addrCntl),
        CLT_Mask(TXTADDRCNTL, INTERFILTER),
        CLT_Shift(TXTADDRCNTL, INTERFILTER),
    },
    {                           /* Tx_TextureEnableFilter */
        TA_TBL,
        offsetof(TxBlendSnippet, txb_addrCntl),
        CLT_Mask(TXTADDRCNTL, TEXTUREENABLE),
        CLT_Shift(TXTADDRCNTL, TEXTUREENABLE),
    },

    {                           /* Tx_PipIndexOffset */
        TA_PIP,
        offsetof(PIPLoadSnippet, ppl_cntl),
        CLT_Mask(TXTPIPCNTL, PIPINDEXOFFSET),
        CLT_Shift(TXTPIPCNTL, PIPINDEXOFFSET),
    },
    {                           /* Tx_PipColorSel */
        TA_PIP,
        offsetof(PIPLoadSnippet, ppl_cntl),
        CLT_Mask(TXTPIPCNTL, PIPCOLORSELECT),
        CLT_Shift(TXTPIPCNTL, PIPCOLORSELECT),
    },
    {                           /* Tx_PipAlphaSel */
        TA_PIP,
        offsetof(PIPLoadSnippet, ppl_cntl),
        CLT_Mask(TXTPIPCNTL, PIPALPHASELECT),
        CLT_Shift(TXTPIPCNTL, PIPALPHASELECT),
    },
    {                           /* Tx_PipSSBSel */
        TA_PIP,
        offsetof(PIPLoadSnippet, ppl_cntl),
        CLT_Mask(TXTPIPCNTL, PIPSSBSELECT),
        CLT_Shift(TXTPIPCNTL, PIPSSBSELECT),
    },

    {                           /* Tx_PipConstSSB0 */
        TA_PIP,
        offsetof(PIPLoadSnippet, ppl_constant[0]),
        0xffffffff,
        0,
    },
    {                           /* Tx_PipConstSSB1 */
        TA_PIP,
        offsetof(PIPLoadSnippet, ppl_constant[1]),
        0xffffffff,
        0,
    },

    {                           /* Tx_FirstColor */
        TA_TBL,
        offsetof(TxBlendSnippet, txb_tabCntl),
        CLT_Mask(TXTTABCNTL, FIRSTCOLOR),
        CLT_Shift(TXTTABCNTL, FIRSTCOLOR),
    },
    {                           /* Tx_SecondColor */
        TA_TBL,
        offsetof(TxBlendSnippet, txb_tabCntl),
        CLT_Mask(TXTTABCNTL, SECONDCOLOR),
        CLT_Shift(TXTTABCNTL, SECONDCOLOR),
    },
    {                           /* Tx_ThirdColor */
        TA_TBL,
        offsetof(TxBlendSnippet, txb_tabCntl),
        CLT_Mask(TXTTABCNTL, THIRDCOLOR),
        CLT_Shift(TXTTABCNTL, THIRDCOLOR),
    },
    {                           /* Tx_FirstAlpha */
        TA_TBL,
        offsetof(TxBlendSnippet, txb_tabCntl),
        CLT_Mask(TXTTABCNTL, FIRSTALPHA),
        CLT_Shift(TXTTABCNTL, FIRSTALPHA),
    },
    {                           /* Tx_SecondAlpha */
        TA_TBL,
        offsetof(TxBlendSnippet, txb_tabCntl),
        CLT_Mask(TXTTABCNTL, SECONDALPHA),
        CLT_Shift(TXTTABCNTL, SECONDALPHA),
    },
    {                           /* Tx_ColorOut */
        TA_TBL,
        offsetof(TxBlendSnippet, txb_tabCntl),
        CLT_Mask(TXTTABCNTL, COLOROUT),
        CLT_Shift(TXTTABCNTL, COLOROUT),
    },
    {                           /* Tx_AlphaOut */
        TA_TBL,
        offsetof(TxBlendSnippet, txb_tabCntl),
        CLT_Mask(TXTTABCNTL, ALPHAOUT),
        CLT_Shift(TXTTABCNTL, ALPHAOUT),
    },
    {                           /* Tx_BlendOp */
        TA_TBL,
        offsetof(TxBlendSnippet, txb_tabCntl),
        CLT_Mask(TXTTABCNTL, BLENDOP),
        CLT_Shift(TXTTABCNTL, BLENDOP),
    },

    {                           /* Tx_BlendColorSSB0 */
        TA_TBL,
        offsetof(TxBlendSnippet, txb_tabConst[0]),
        0xffffffff,
        0,
    },
    {                           /* Tx_BlendColorSSB1 */
        TA_TBL,
        offsetof(TxBlendSnippet, txb_tabConst[1]),
        0xffffffff,
        0,
    },
};

static const AttrTable dbTable[] =
{
    {                           /* Dbl_EnableAttrs */
        DB_DBL,
        offsetof(DBlendSnippet, dbl_userGenCntl),
        (CLT_Mask(DBUSERCONTROL,ZBUFFEN) |
         CLT_Mask(DBUSERCONTROL,ZOUTEN) |
         CLT_Mask(DBUSERCONTROL,WINCLIPINEN) |
         CLT_Mask(DBUSERCONTROL,WINCLIPOUTEN) |
         CLT_Mask(DBUSERCONTROL,BLENDEN) |
         CLT_Mask(DBUSERCONTROL,SRCEN) |
         CLT_Mask(DBUSERCONTROL,DITHEREN) |
         CLT_Mask(DBUSERCONTROL,DESTOUTMASK)),
        CLT_Shift(DBUSERCONTROL,DESTOUTMASK),
    },
    {                           /* Dbl_ZBuffEnable */
        DB_DBL,
        offsetof(DBlendSnippet, dbl_userGenCntl),
        CLT_Mask(DBUSERCONTROL, ZBUFFEN),
        CLT_Shift(DBUSERCONTROL, ZBUFFEN),
    },
    {                           /* Dbl_ZBuffOutEnable */
        DB_DBL,
        offsetof(DBlendSnippet, dbl_userGenCntl),
        CLT_Mask(DBUSERCONTROL, ZOUTEN),
        CLT_Shift(DBUSERCONTROL, ZOUTEN),
    },
    {                           /* Dbl_WinClipInEnable */
        DB_DBL,
        offsetof(DBlendSnippet, dbl_userGenCntl),
        CLT_Mask(DBUSERCONTROL, WINCLIPINEN),
        CLT_Shift(DBUSERCONTROL, WINCLIPINEN),
    },
    {                           /* Dbl_WinClipOutEnable */
        DB_DBL,
        offsetof(DBlendSnippet, dbl_userGenCntl),
        CLT_Mask(DBUSERCONTROL, WINCLIPOUTEN),
        CLT_Shift(DBUSERCONTROL, WINCLIPOUTEN),
    },
    {                           /* Dbl_BlendEnable */
        DB_DBL,
        offsetof(DBlendSnippet, dbl_userGenCntl),
        CLT_Mask(DBUSERCONTROL, BLENDEN),
        CLT_Shift(DBUSERCONTROL, BLENDEN),
    },
    {                           /* Dbl_SrcInputEnable */
        DB_DBL,
        offsetof(DBlendSnippet, dbl_userGenCntl),
        CLT_Mask(DBUSERCONTROL, SRCEN),
        CLT_Shift(DBUSERCONTROL, SRCEN),
    },
    {                           /* Dbl_DitherEnable */
        DB_DBL,
        offsetof(DBlendSnippet, dbl_userGenCntl),
        CLT_Mask(DBUSERCONTROL, DITHEREN),
        CLT_Shift(DBUSERCONTROL, DITHEREN),
    },
    {                           /* Dbl_RGBDestOut */
        DB_DBL,
        offsetof(DBlendSnippet, dbl_userGenCntl),
        CLT_Mask(DBUSERCONTROL, DESTOUTMASK),
        CLT_Shift(DBUSERCONTROL, DESTOUTMASK),
    },
    
    {                           /* Dbl_Discard */
        DB_DBL,
        offsetof(DBlendSnippet, dbl_discardCntl),
        (CLT_Mask(DBDISCARDCONTROL,ZCLIPPED) |
         CLT_Mask(DBDISCARDCONTROL,SSB0) |
         CLT_Mask(DBDISCARDCONTROL,RGB0) |
         CLT_Mask(DBDISCARDCONTROL,ALPHA0)),
        CLT_Shift(DBDISCARDCONTROL,ALPHA0),
    },
    {                           /* Dbl_DiscardZClipped */
        DB_DBL,
        offsetof(DBlendSnippet, dbl_discardCntl),
        CLT_Mask(DBDISCARDCONTROL,ZCLIPPED),
        CLT_Shift(DBDISCARDCONTROL,ZCLIPPED),
    },
    {                           /* Dbl_DiscardSSB0 */
        DB_DBL,
        offsetof(DBlendSnippet, dbl_discardCntl),
        CLT_Mask(DBDISCARDCONTROL, SSB0),
        CLT_Shift(DBDISCARDCONTROL, SSB0),
    },
    {                           /* Dbl_Discard */
        DB_DBL,
        offsetof(DBlendSnippet, dbl_discardCntl),
        CLT_Mask(DBDISCARDCONTROL, RGB0),
        CLT_Shift(DBDISCARDCONTROL, RGB0),
    },
    {                           /* Dbl_DiscardAlpha0 */
        DB_DBL,
        offsetof(DBlendSnippet, dbl_discardCntl),
        CLT_Mask(DBDISCARDCONTROL, ALPHA0),
        CLT_Shift(DBDISCARDCONTROL, ALPHA0),
    },

    {                           /* Dbl_XWinClipMin */
        UNSUPPORTED,
        0,
        0,
        0,
    },
    {                           /* Dbl_XWinClipMax */
        UNSUPPORTED,
        0,
        0,
        0,
    },
    {                           /* Dbl_YWinClipMin */
        UNSUPPORTED,
        0,
        0,
        0,
    },
    {                           /* Dbl_YWinClipMax */
        UNSUPPORTED,
        0,
        0,
        0,
    },
    {                           /* Dbl_ZCompareControl */
        UNSUPPORTED,
        0,
        0,
        0,
    },
    {                           /* Dbl_ZXOffset */
        UNSUPPORTED,
        0,
        0,
        0,
    },
    {                           /* Dbl_ZYOffset */
        UNSUPPORTED,
        0,
        0,
        0,
    },

    {                           /* Dbl_DSBConst */
        DB_DBL,
        offsetof(DBlendSnippet, dbl_dsbCntl),
        CLT_Mask(DBSSBDSBCNTL, DSBCONST),
        CLT_Shift(DBSSBDSBCNTL, DSBCONST),
    },
    {                           /* Dbl_DSBSelect */
        DB_DBL,
        offsetof(DBlendSnippet, dbl_dsbCntl),
        CLT_Mask(DBSSBDSBCNTL, DSBSELECT),
        CLT_Shift(DBSSBDSBCNTL, DSBSELECT),
    },
    {                           /* Dbl_RGBConstIn */
        DB_DBL,
        offsetof(DBlendSnippet, dbl_constIn),
        (CLT_Mask(DBCONSTIN,RED) |
         CLT_Mask(DBCONSTIN,GREEN) |
         CLT_Mask(DBCONSTIN,BLUE)),
        CLT_Shift(DBCONSTIN,BLUE),
    },
    {                           /* Dbl_AInputSelect*/
        DB_DBL,
        offsetof(DBlendSnippet, dbl_txtMultCntl),
        CLT_Mask(DBAMULTCNTL, AINPUTSELECT),
        CLT_Shift(DBAMULTCNTL, AINPUTSELECT),
    },
    {                           /* Dbl_AMultCoefSelect*/
        DB_DBL,
        offsetof(DBlendSnippet, dbl_txtMultCntl),
        CLT_Mask(DBAMULTCNTL, AMULTCOEFSELECT),
        CLT_Shift(DBAMULTCNTL, AMULTCOEFSELECT),
    },
    {                           /* Dbl_AMultConstSelect*/
        DB_DBL,
        offsetof(DBlendSnippet, dbl_txtMultCntl),
        CLT_Mask(DBAMULTCNTL, AMULTCONSTCONTROL),
        CLT_Shift(DBAMULTCNTL, AMULTCONSTCONTROL),
    },
    {                           /* Dbl_AMultRTJustifySelect*/
        DB_DBL,
        offsetof(DBlendSnippet, dbl_txtMultCntl),
        CLT_Mask(DBAMULTCNTL, AMULTRJUSTIFY),
        CLT_Shift(DBAMULTCNTL, AMULTRJUSTIFY),
    },
    {                           /* Dbl_AMultConstSSB0 */
        DB_DBL,
        offsetof(DBlendSnippet, dbl_txtCoefConst[0]),
        (CLT_Mask(DBAMULTCONSTSSB0, RED)|
         CLT_Mask(DBAMULTCONSTSSB0, GREEN)|
         CLT_Mask(DBAMULTCONSTSSB0, BLUE)),
        CLT_Shift(DBAMULTCONSTSSB0, BLUE),
    },
    {                           /* Dbl_AMultConstSSB1 */
        DB_DBL,
        offsetof(DBlendSnippet, dbl_txtCoefConst[1]),
        (CLT_Mask(DBAMULTCONSTSSB1, RED)|
         CLT_Mask(DBAMULTCONSTSSB1, GREEN)|
         CLT_Mask(DBAMULTCONSTSSB1, BLUE)),
        CLT_Shift(DBAMULTCONSTSSB1, BLUE),
    },
    {                           /* Dbl_BInputSelect */
        DB_DBL,
        offsetof(DBlendSnippet, dbl_srcMultCntl),
        CLT_Mask(DBBMULTCNTL, BINPUTSELECT),
        CLT_Shift(DBBMULTCNTL, BINPUTSELECT),
    },
    {                           /* Dbl_BMultCoefSelect */
        DB_DBL,
        offsetof(DBlendSnippet, dbl_srcMultCntl),
        CLT_Mask(DBBMULTCNTL, BMULTCOEFSELECT),
        CLT_Shift(DBBMULTCNTL, BMULTCOEFSELECT),
    },
    {                           /* Dbl_BMultConstSelect */
        DB_DBL,
        offsetof(DBlendSnippet, dbl_srcMultCntl),
        CLT_Mask(DBBMULTCNTL, BMULTCONSTCONTROL),
        CLT_Shift(DBBMULTCNTL, BMULTCONSTCONTROL),
    },
    {                           /* Dbl_BMultRtJustify */
        DB_DBL,
        offsetof(DBlendSnippet, dbl_srcMultCntl),
        CLT_Mask(DBBMULTCNTL, BMULTRJUSTIFY),
        CLT_Shift(DBBMULTCNTL, BMULTRJUSTIFY),
    },
    {                           /* Dbl_BMultConstSSB0 */
        DB_DBL,
        offsetof(DBlendSnippet, dbl_multCoefConst[0]),
        (CLT_Mask(DBBMULTCONSTSSB0, RED) |
         CLT_Mask(DBBMULTCONSTSSB0, GREEN) |
         CLT_Mask(DBBMULTCONSTSSB0, BLUE)),
        CLT_Shift(DBBMULTCONSTSSB0, BLUE),
    },
    {                           /* Dbl_BMultConstSSB1 */
        DB_DBL,
        offsetof(DBlendSnippet, dbl_multCoefConst[1]),
        (CLT_Mask(DBBMULTCONSTSSB1, RED) |
         CLT_Mask(DBBMULTCONSTSSB1, GREEN) |
         CLT_Mask(DBBMULTCONSTSSB1, BLUE)),
        CLT_Shift(DBBMULTCONSTSSB1, BLUE),
    },
    {                           /* Dbl_ALUOperation */
        DB_DBL,
        offsetof(DBlendSnippet, dbl_aluCntl),
        CLT_Mask(DBALUCNTL, ALUOPERATION),
        CLT_Shift(DBALUCNTL, ALUOPERATION),
    },
    {                           /* Dbl_FinalDivide */
        DB_DBL,
        offsetof(DBlendSnippet, dbl_aluCntl),
        CLT_Mask(DBALUCNTL, FINALDIVIDE),
        CLT_Shift(DBALUCNTL, FINALDIVIDE),
    },
    {                           /* Dbl_Alpha0ClampControl */
        DB_DBL,
        offsetof(DBlendSnippet, dbl_srcAlphaCntl),
        CLT_Mask(DBSRCALPHACNTL, ALPHA0),
        CLT_Shift(DBSRCALPHACNTL, ALPHA0),
    },
    {                           /* Dbl_Alpha1ClampControl */
        DB_DBL,
        offsetof(DBlendSnippet, dbl_srcAlphaCntl),
        CLT_Mask(DBSRCALPHACNTL, ALPHA1),
        CLT_Shift(DBSRCALPHACNTL, ALPHA1),
    },
    {                           /* Dbl_AlphaFracClampControl */
        DB_DBL,
        offsetof(DBlendSnippet, dbl_srcAlphaCntl),
        CLT_Mask(DBSRCALPHACNTL, ALPHAFRAC),
        CLT_Shift(DBSRCALPHACNTL, ALPHAFRAC),
    },
    {                           /* Dbl_AlphaClampControl */
        DB_DBL,
        offsetof(DBlendSnippet, dbl_srcAlphaCntl),
        CLT_Mask(DBSRCALPHACNTL, ACLAMP),
        CLT_Shift(DBSRCALPHACNTL, ACLAMP),
    },
    {                           /* Dbl_DestAlphaSelect */
        DB_DBL,
        offsetof(DBlendSnippet, dbl_dstAlphaCntl),
        CLT_Mask(DBDESTALPHACNTL, DESTCONSTSELECT),
        CLT_Shift(DBDESTALPHACNTL, DESTCONSTSELECT),
    },
    {                           /* Dbl_DestAlphaConstSSB0 */
        DB_DBL,
        offsetof(DBlendSnippet, dbl_dstAlphaConst),
        CLT_Mask(DBDESTALPHACONST, DESTALPHACONSTSSB0),
        CLT_Shift(DBDESTALPHACONST, DESTALPHACONSTSSB0),
    },
     {                           /* Dbl_DestAlphaConstSSB1 */
        DB_DBL,
        offsetof(DBlendSnippet, dbl_dstAlphaConst),
        CLT_Mask(DBDESTALPHACONST, DESTALPHACONSTSSB1),
        CLT_Shift(DBDESTALPHACONST, DESTALPHACONSTSSB1),
    },
    {                           /* Dbl_DitherMatrixA */
        UNSUPPORTED,
        0,
        0,
        0,
    },
    {                           /* Dbl_DitherMatrixB */
        UNSUPPORTED,
        0,
        0,
        0,
    },
/* Items below here are not available in the texture file format */
    {                           /* Dbl_SrcPixels32 */
        UNSUPPORTED,
        0,
        0,
        0,
    },
    {                           /* Dbl_SrcBaseAddr */
        UNSUPPORTED,
        0,
        0,
        0,
    },
    {                           /* Dbl_SrcXStride */
        UNSUPPORTED,
        0,
        0,
        0,
    },
    {                           /* Dbl_SrcXOffset */
        UNSUPPORTED,
        0,
        0,
        0,
    },
    {                           /* Dbl_SrcYOffset */
        UNSUPPORTED,
        0,
        0,
        0,
    },
};

void SetTextureAttr(BlitterSnippetHeader *snippets[], uint32 type, uint32 value)
{
    uint32 *snip;
    uint32 *data;

    if (taTable[type].snippet != UNSUPPORTED)
    {
        snip = (uint32 *)snippets[taTable[type].snippet];
        if (snip)
        {
            data = (snip + (taTable[type].offset / sizeof(uint32)));
            *data &= ~taTable[type].mask;
            *data |= (value << taTable[type].shift);        
        }
    }
    
    return;
}

void SetDBlendAttr(BlitterSnippetHeader *snippets[], uint32 type, uint32 value)
{
    uint32 *snip;
    uint32 *data;

    if (dbTable[type].snippet != UNSUPPORTED)
    {
        snip = (uint32 *)snippets[dbTable[type].snippet];
        if (snip)
        {
            data = (snip + (dbTable[type].offset / sizeof(uint32)));
            *data &= ~dbTable[type].mask;
            *data |= (value << dbTable[type].shift);        
        }
    }
    
    return;
}
