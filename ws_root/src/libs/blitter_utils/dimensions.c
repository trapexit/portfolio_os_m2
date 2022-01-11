/***************************************************************************
**
** @(#) dimensions.c 96/10/15 1.2
**
**  Code to initialise all the Snippet values that determine the
**  blit size and type.
**
****************************************************************************/
#include <kernel/types.h>
#include <graphics/blitter.h>
#include <graphics/clt/cltmacros.h>
#include <graphics/clt/clttxdblend.h>

#define EXPANSIONFORMATMASK 0x00001FFF

void Blt_InitDimensions(BlitObject *bo, uint32 width, uint32 height, uint32 bitsPerPixel)
{
    CltTxData *ctd = NULL;
    CltTxLOD *lod = NULL;
    CltTxDCI *dci = NULL;
    uint32 uncompressed = FV_TXTEXPTYPE_ISLITERAL_MASK;
    
    if (bo->bo_txdata)
    {
        ctd = &bo->bo_txdata->btd_txData;
        lod = ctd->texelData;
        dci = ctd->dci;
    }
    
    if (bo->bo_tbl && ctd)
    {
            /* Set the expType based on the expansion format in the CltTxData */
        bo->bo_tbl->txb_expType = ctd->expansionFormat;
            /* We want to know if this texture is compressed or literal when we set the
             * size registers.
             */
        uncompressed = (ctd->expansionFormat & FV_TXTEXPTYPE_ISLITERAL_MASK);
    }

    if (bo->bo_tbl && dci)
    {
            /* Set up the decompression info. Note that the registers use the order
             * 1:0 and 3:2, but the data in the CltTxDCI is ordered 0:1 2:3
             */
        bo->bo_tbl->txb_srcType[0] = ((((uint32)dci->texelFormat[1] & EXPANSIONFORMATMASK) << 16) |
                                      ((uint32)dci->texelFormat[0] & EXPANSIONFORMATMASK));
        bo->bo_tbl->txb_srcType[1] = ((((uint32)dci->texelFormat[3] & EXPANSIONFORMATMASK) << 16) |
                                      ((uint32)dci->texelFormat[2] & EXPANSIONFORMATMASK));
        bo->bo_tbl->txb_constant[0] = dci->expColor[0];
        bo->bo_tbl->txb_constant[1] = dci->expColor[1];
            /* tabConst[0..1] is equivalent to constant[2..3] */
        bo->bo_tbl->txb_tabConst[0] = dci->expColor[2];
        bo->bo_tbl->txb_tabConst[1] = dci->expColor[3];
    }

    if (bo->bo_txl)
    {
        if (lod)
        {
                /* This is the source of the texture data */
            bo->bo_txl->txl_src = (uint32)lod->texelData;
        }
        if (!uncompressed && dci)
        {
                /* For compressed textures, count is the number of  texels */
            bo->bo_txl->txl_count = (width * height * ((bitsPerPixel + 7) / 8));
        }
        else
        {
                /* For uncompressed textures, count is the number of texel rows */
            bo->bo_txl->txl_count = height;
        }
        bo->bo_txl->txl_width = CLA_TXTLDWIDTH((width * bitsPerPixel),
                                               (width * bitsPerPixel));
        bo->bo_txl->txl_uvMax = CLA_TXTUVMAX((width - 1), (height - 1));            
    }

    if (bo->bo_txdata)
    {
        bo->bo_txdata->btd_txData.minX = width;
        bo->bo_txdata->btd_txData.minY = height;
        bo->bo_txdata->btd_txData.bitsPerPixel = bitsPerPixel;
   }
}

