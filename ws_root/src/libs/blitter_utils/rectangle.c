/***************************************************************************
**
**  @(#) rectangle.c 96/06/19 1.1
**
**  Code to blit a rectangular area of a bitmap through vertices into
**  the same bitmap.
**
****************************************************************************/

#include <kernel/types.h>
#include <graphics/clt/cltmacros.h>
#include <graphics/blitter.h>

Err Blt_RectangleInBitmap(GState *gs, BlitObject *bo, Item bmI, BlitRect *br)
{
    Bitmap *bm = (Bitmap *)LookupItem(bmI);
    uint32 boxWidth, boxHeight;
    uint32 depth;
    Err err;
    
            /* Check we have everything we need */
    if (bm == NULL)
    {
        return(BLITTER_ERR_BADITEM);
    }
    if (bo == NULL)
    {
        return(BLITTER_ERR_BADPTR);
    }
    if (bo->bo_tbl == NULL)
    {
        return(BLITTER_ERR_NOTBLEND);
    }
    if (bo->bo_txl == NULL)
    {
        return(BLITTER_ERR_NOTXLOAD);
    }

    boxWidth = (uint32)(br->max.x - br->min.x + 1.0);
    boxHeight = (uint32)(br->max.y - br->min.y + 1.0);
    depth = ((bm->bm_Type == BMTYPE_32) ? 32 : 16);
    
    /* We are going to set up the snippets to load from the required area of the
     * bitmap into TRAM, and then render the triangles.
     * First, see if the number of bytes that will be loaded into TRAM is smaller
     * than the TRAM limit. If not, then we cannot do anymore here, and the
     * application will need to use Blt_RectangleToBitmap() to cause the vertices
     * and the texture to be sliced.
     */
    if ((boxWidth * boxHeight * (depth / 8)) > TRAM_SIZE)
    {
        return(BLITTER_ERR_TOOBIG);
    }
    
    /* TxBlend stuff first. Leave the txb_addrCntl and txb_expType fields alone.
     * The default values that were set when the TxBlendSnippet was created are
     * correct for this operation, and if they have been changed then it was under
     * the control of the application.
     */
    bo->bo_tbl->txb_expType = 
        ((bm->bm_Type == BMTYPE_32) ?
         CLA_TXTEXPTYPE(8, 7, 0, 1, 1, 1, 1) :
         CLA_TXTEXPTYPE(5, 0, 0, 1, 1, 0, 1));

    /* TxLoad stuff is next. Again, only modify the settings we really need for
     * this operation.
     */
    bo->bo_txl->txl_base = 0;
    bo->bo_txl->txl_src = (uint32)PixelAddress(bmI, (uint32)br->min.x, (uint32)br->min.y);
    bo->bo_txl->txl_count = boxHeight;
    bo->bo_txl->txl_width = CLA_TXTLDWIDTH((boxWidth * depth),
                                           (bm->bm_Width * depth));
    bo->bo_txl->txl_uvMax = CLA_TXTUVMAX((boxWidth - 1), (boxHeight - 1));

    err = Blt_BlitObjectToBitmap(gs, bo, bmI, 0);
    return(err);
}

