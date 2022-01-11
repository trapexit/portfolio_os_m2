/***************************************************************************
**
** @(#) mask.c 96/06/07 1.2
**
**  Code to enable and disable masks (pixel-discard mode)
**
****************************************************************************/

#include <kernel/types.h>
#include <graphics/clt/cltmacros.h>
#include <graphics/blitter.h>

Err Blt_EnableMask(BlitObject *bo, uint32 discardType)
{
    bool maskSSB;
    bool maskAlpha;
    bool maskRGB;
     
        /* Check we have everything we need to enable the mask */
    if (bo == NULL)
    {
        return(BLITTER_ERR_BADPTR);
    }
    if (bo->bo_dbl == NULL)
    {
        return(BLITTER_ERR_NODBLEND);
    }    
    if (bo->bo_tbl == NULL)
    {
        return(BLITTER_ERR_NOTBLEND);
    }
    if (bo->bo_pip == NULL)
    {
        return(BLITTER_ERR_NOPIP);
    }

    maskSSB = (((discardType & FV_DBDISCARDCONTROL_SSB0_MASK) &&
                (bo->bo_tbl->txb_expType & FV_TXTEXPTYPE_HASSSB_MASK)) ?
               TRUE : FALSE);
    maskAlpha = (((discardType & FV_DBDISCARDCONTROL_ALPHA0_MASK) &&
                  (bo->bo_tbl->txb_expType & FV_TXTEXPTYPE_HASALPHA_MASK)) ?
                 TRUE : FALSE);
    maskRGB = (((discardType & FV_DBDISCARDCONTROL_RGB0_MASK) &&
                (bo->bo_tbl->txb_expType & FV_TXTEXPTYPE_HASCOLOR_MASK)) ?
               TRUE : FALSE);

        /* Enable pixelDiscard */
    bo->bo_dbl->dbl_userGenCntl |=
        (FV_DBUSERCONTROL_BLENDEN_MASK | FV_DBUSERCONTROL_SRCEN_MASK);
    bo->bo_dbl->dbl_discardCntl =
        CLA_DBDISCARDCONTROL(0, maskSSB, maskRGB, maskAlpha);
    bo->bo_dbl->dbl_aluCntl =
        CLA_DBALUCNTL(RC_DBALUCNTL_ALUOPERATION_A, 0);
    /* Set up the PIP to pass the desired bits from the texture */
    if (maskSSB)
    {
        bo->bo_pip->ppl_cntl |=
            (RC_TXTPIPCNTL_PIPSSBSELECT_TEXTURE << FV_TXTPIPCNTL_PIPSSBSELECT_SHIFT);
    }
    if (maskAlpha)
    {
        bo->bo_pip->ppl_cntl |=
            (RC_TXTPIPCNTL_PIPALPHASELECT_TEXTURE << FV_TXTPIPCNTL_PIPALPHASELECT_SHIFT);
    }
    if (maskRGB)
    {
        bo->bo_pip->ppl_cntl |=
            (RC_TXTPIPCNTL_PIPCOLORSELECT_TEXTURE << FV_TXTPIPCNTL_PIPCOLORSELECT_SHIFT);
    }

    return(0);
}    
    
Err Blt_DisableMask(BlitObject *bo)
{
    if (bo == NULL)
    {
        return(BLITTER_ERR_BADPTR);
    }
    if (bo->bo_dbl == NULL)
    {
        return(BLITTER_ERR_NODBLEND);
    }    
        /* Disable pixelDiscard */
    bo->bo_dbl->dbl_discardCntl =
        CLA_DBDISCARDCONTROL(0, 0, 0, 0);

    return(0);
}
