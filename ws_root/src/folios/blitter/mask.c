/* @(#) mask.c 96/08/15 1.3
 *
 *  Code to build  a mask.
 */

#include <kernel/types.h>
#include <kernel/mem.h>
#include <graphics/blitter.h>
#include <graphics/bitmap.h>
#include <stdio.h>
#include "blitter_internal.h"

#define DPRT(x) /*printf x*/

static void MaskSSB(bool maskSSB, bool txDepth32, void **txt, uint32 maskBits, uint32 flags)
{
    if (maskSSB)
    {
        if (txDepth32)
        {
            uint32 *tmp = (uint32 *)*txt;

            if (!(maskBits & 0x80000000))
            {
                    /* Discard this pixel. Set SSB to 0 */
                *tmp &= ~0x80000000;
            }
            else if (flags & FLAG_BLM_FORCE_VISIBLE)
            {
                    /* Must set SSB to 1 to make this pixel visible */
                *tmp |= 0x80000000;
            }                
            tmp++;
            *txt = (void *)tmp;
        }
        else
        {
            uint16 *tmp = (uint16 *)*txt;
                            
            if (!(maskBits & 0x80000000))
            {
                *tmp &= ~0x8000;
            }
            else if (flags & FLAG_BLM_FORCE_VISIBLE)
            {
                    /* Must set SSB to 1 to make this pixel visible */
                *tmp |= 0x8000;
            }            
            tmp++;
            *txt = (void *)tmp;
        }
    }
}
static void MaskAlpha(bool maskAlpha, bool txDepth32, void **txt, uint32 maskBits, uint32 flags)
{
    if (maskAlpha)
    {
        if (txDepth32)
        {
            uint32 *tmp = (uint32 *)*txt;

            if (!(maskBits & 0x80000000))
            {
                    /* Discard this pixel. Set alpha to 0 */
                *tmp &= 0x80ffffff;
            }
            else if (((*tmp & 0x7f000000) == 0) && (flags & FLAG_BLM_FORCE_VISIBLE))
            {
                    /* this pixel has an alpha of 0. Make it non-transparent by giving
                     * it a small alpha value.
                     */
                *tmp |= 0x01000000;
            }
            tmp++;
            *txt = (void *)tmp;
        }
        else
        {
            uint16 *tmp = (uint16 *)*txt;
                            
            printf("How deep is Alpha? -- NYI\n");
            tmp++;
            *txt = (void *)tmp;
        }
    }
}
static void MaskRGB(bool maskRGB, bool txDepth32, void **txt, uint32 maskBits, uint32 flags)
{
    if (maskRGB)
    {
        if (txDepth32)
        {
            uint32 *tmp = (uint32 *)*txt;

            if (!(maskBits & 0x80000000))
            {
                    /* Discard this pixel. Make RGB = 0 */
                *tmp &= 0xff000000;
            }
            else if (((*tmp & 0x00ffffff) == 0) && (flags & FLAG_BLM_FORCE_VISIBLE))
            {
                     /* this pixel has an RGB of 0. Make it non-transparent by giving
                     * it a small RGB value.
                     */
               *tmp |= 1;
            }
            tmp++;
            *txt = (void *)tmp;
        }
        else
        {
            uint16 *tmp = (uint16 *)*txt;
                            
            if (!(maskBits & 0x80000000))
            {
                *tmp &= 0x8000;
            }
            else if (((*tmp & 0x7ff) == 0) && (flags & FLAG_BLM_FORCE_VISIBLE))
            {
                     /* this pixel has an RGB of 0. Make it non-transparent by giving
                     * it a small RGB value.
                     */
               *tmp |= 1;
            }
            tmp++;
            *txt = (void *)tmp;
        }
    }
}

Err Blt_MakeMask(const BlitObject *bo, const BlitMask *mask)
{
    CltTxData *ctd;
    uint32 startBitX, startBitY;
    int32 x, y;
    uint32 invert = 0;
    uint32 txWidth, txHeight;
    uint32 maskWidth, maskHeight;
    uint32 rptX, rptY;
    uint32 i, j, ii, jj;
    uint32 xCount, yCount;
    void *txt = NULL;
    uint8 *lastTxt;
    uint32 *lastTxtCol32;
    uint16 *lastTxtCol16;
    uint32 *maskRow;
    uint32 *maskCol;
    uint32 maskBits;
    uint32 bytesPerPixel;
    bool maskSSB;
    bool maskAlpha;
    bool maskRGB;
    bool txDepth32;

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
    
    if ((bo->bo_txdata) &&
        (bo->bo_txdata->btd_txData.texelData))
    {
        txt = bo->bo_txdata->btd_txData.texelData->texelData;
    }
    if (txt == NULL)
    {
        return(BLITTER_ERR_NOTXDATA);
    }
    ctd = &bo->bo_txdata->btd_txData;
    
    txWidth = ctd->minX;
    txHeight = ctd->minY;
    txDepth32 = (((bo->bo_tbl->txb_expType & FV_TXTEXPTYPE_CDEPTH_MASK) >>
                  FV_TXTEXPTYPE_CDEPTH_SHIFT) == 8);
    bytesPerPixel = (ctd->bitsPerPixel / 8);
    maskWidth = mask->blm_width;
    maskHeight = mask->blm_height;
    
    if (mask->blm_flags & FLAG_BLM_INVERT)
    {
        invert = 0xffffffff;
    }

    rptX = rptY = 1;
    if (mask->blm_flags & FLAG_BLM_CENTER)
    {
        x = ((txWidth - maskWidth) / 2);
        y = ((txHeight - maskHeight) / 2);
        startBitX = ((x > 0) ? 0 : -x);
        startBitY = ((y > 0) ? 0 : -y);

        if (mask->blm_flags & FLAG_BLM_REPEAT)
        {
            if (startBitX >= 0)
            {
                startBitX = (maskWidth - (x % maskWidth));
                rptX = (((x + (maskWidth - 1)) / maskWidth)  /* Repetitions to left of center */
                        + 1   /* center */
                        + ((txWidth - (x + maskWidth) + (maskWidth - 1)) / maskWidth)); /* Repetitions to right of center */
                x = 0;
            }
            if (startBitY >= 0)
            {
                startBitY = (maskHeight - (y % maskHeight));
                rptY = (((y + (maskHeight - 1)) / maskHeight)  /* Repetitions above center */
                        + 1   /* center */
                        + ((txHeight - (y + maskHeight) + (maskHeight - 1)) / maskHeight)); /* Repetitions below center */
                y = 0;
            }
        }
    }
    else
    {
        x = 0;
        y = 0;
        startBitX = 0;
        startBitY = 0;
        if (mask->blm_flags & FLAG_BLM_REPEAT)
        {
            rptX = ((txWidth + (maskWidth - 1)) / maskWidth);
            rptY = ((txHeight + (maskHeight - 1)) / maskHeight);
        }
    }

    maskSSB = (((mask->blm_discardType & FV_DBDISCARDCONTROL_SSB0_MASK) &&
                (bo->bo_tbl->txb_expType & FV_TXTEXPTYPE_HASSSB_MASK)) ?
               TRUE : FALSE);
    maskAlpha = (((mask->blm_discardType & FV_DBDISCARDCONTROL_ALPHA0_MASK) &&
                  (bo->bo_tbl->txb_expType & FV_TXTEXPTYPE_HASALPHA_MASK)) ?
                 TRUE : FALSE);
    maskRGB = (((mask->blm_discardType & FV_DBDISCARDCONTROL_RGB0_MASK) &&
                (bo->bo_tbl->txb_expType & FV_TXTEXPTYPE_HASCOLOR_MASK)) ?
               TRUE : FALSE);
    
        /* Loops to modify the texture data as per the mask
         * First, find the starting point in the texture.
         */
    lastTxt = (uint8 *)bo->bo_txdata->btd_txData.texelData->texelData;
    lastTxt += ((y * (txWidth * bytesPerPixel)) +
                (x * bytesPerPixel));
    txt = (void *)lastTxt;
    for (j = 0; j < rptX; j++)
    {
        yCount = 0;
        lastTxtCol32 = (uint32 *)txt;
        lastTxtCol16 = (uint16 *)txt;
        for (i = 0; i < rptY; i++)
        {
            maskRow = mask->blm_data;
            maskRow += ((startBitY * (i == 0 ? 1 : 0)) * ((maskWidth + 31) / 32));
            maskCol = maskRow;
            for (ii = (i == 0 ? startBitY : 0); ((ii < maskHeight) && (yCount < txHeight)); ii++)
            {
                lastTxt = (uint8 *)txt;
                /* Do first column of mask */
                if (j == 0)
                {
                    maskCol += (startBitX / 32);
                }
                maskBits = *maskCol;
                maskBits ^= invert;
                if (j == 0)
                {
                    maskBits <<= (startBitX % 32);
                }
                xCount = (j == 0 ? 0 : ((j * maskWidth) - startBitX));
                for (jj = (j == 0 ? (startBitX % 32) : 0); ((jj < (maskWidth >= 32 ? 32 : maskWidth)) &&  (xCount < txWidth)); jj++)
                {
                    MaskSSB(maskSSB, txDepth32, &txt, maskBits, mask->blm_flags);
                    MaskAlpha(maskAlpha, txDepth32, &txt, maskBits, mask->blm_flags);
                    MaskRGB(maskRGB, txDepth32, &txt, maskBits, mask->blm_flags);
                    maskBits <<= 1;
                    xCount++;
                }
                /* If there is more than one column, keep going */
                for (jj = (j == 0 ? (32 * ((startBitX / 32) + 1)) : 32); ((jj < maskWidth) && (xCount < txWidth)); jj++)
                {
                    if ((jj & 0x1f) == 0)
                    {
                        maskCol++;
                        maskBits = *maskCol;
                        maskBits ^= invert;
                    }
                    MaskSSB(maskSSB, txDepth32, &txt, maskBits, mask->blm_flags);
                    MaskAlpha(maskAlpha, txDepth32, &txt, maskBits, mask->blm_flags);
                    MaskRGB(maskRGB, txDepth32, &txt, maskBits, mask->blm_flags);
                    maskBits <<= 1;
                    xCount++;
                }
                maskCol = (maskRow + ((maskWidth + 31) / 32));  /* Should move down to the next row of mask bits */
                maskRow = maskCol;
                txt = (void *)(lastTxt + (txWidth * bytesPerPixel));
                yCount++;
            }
        }
        if (txDepth32)
        {
            lastTxtCol32 += (j == 0 ? (maskWidth - startBitX) : maskWidth);
            txt = (void *)lastTxtCol32;
        }
        else
        {
            lastTxtCol16 += (j == 0 ? (maskWidth - startBitX) : maskWidth);
            txt = (void *)lastTxtCol16;
        }        
    }
    
    return(0);
}


                    
                            

