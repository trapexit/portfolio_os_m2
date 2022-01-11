/* @(#) blit.c 96/10/15 1.14
 *
 *  Code to blit the data
 */

#include <kernel/types.h>
#include <kernel/mem.h>
#include <graphics/blitter.h>
#include <graphics/bitmap.h>
#include <stdio.h>
#include <string.h>
#include "blitter_internal.h"

#define DPRT(x) /* printf x */
Err SliceVertices(BlitObject *bo);
void RemoveSlices(BlitObject *bo);
void RemoveBOToVtxAssociation(BlitObject *bo, VerticesSnippet *vtx);
Err ClipVertexList(BlitObject *bo);

#define BUMP_GSPTR(g, s) {uint32 tmp = (uint32)*GS_Ptr(g); tmp += (s); *(GS_Ptr(g)) = (CmdListP)tmp;}

Err doBlit(GState *gs, ubyte *src, ubyte *dst, uint32 srcBytesPerRow, uint32 dstBytesPerRow,
            uint32 rows, uint32 width, uint32 type, BlitStuff *bs)
{
    uint32 i;
    Err err;
    uint32 doBlitSnippet[] =
    {
        CLT_WriteRegistersHeader(DBUSERCONTROL, 2),
        CLA_DBUSERCONTROL(0, 0, 0, 0, 1, 1, 0,   /* enable dblend in here */
                          (CLT_SetConst(DBUSERCONTROL, DESTOUTMASK, RED) | 
                           CLT_SetConst(DBUSERCONTROL, DESTOUTMASK, GREEN) | 
                           CLT_SetConst(DBUSERCONTROL, DESTOUTMASK, BLUE) | 
                           CLT_SetConst(DBUSERCONTROL, DESTOUTMASK, ALPHA))),
        CLA_DBDISCARDCONTROL(0, 0, 0, 0),
            /* default dblend is for mixinf foreground and background, but is
             * disabled in DBUSERCONTROL.
             */
        CLT_WriteRegistersHeader(DBSRCCNTL, 4),
        CLA_DBSRCCNTL(1, 0), /* Put 16/32bpp info here */
        0,                                 /* DBSRCBASEADDR Put bitmap address here */
        0,                             /* DBSRCXSTRIDE Put bitmap width here (in pixels) */
        CLA_DBSRCOFFSET(0, 0), /* DBSRCOFFSET Put offset of blend rect here */
        CLT_WriteRegistersHeader(DBBMULTCNTL, 4),
        CLA_DBBMULTCNTL(RC_DBBMULTCNTL_BINPUTSELECT_SRCCOLOR,
                        RC_DBBMULTCNTL_BMULTCOEFSELECT_CONST,
                        RC_DBBMULTCNTL_BMULTCONSTCONTROL_SRCSSB,
                        0),
        0xffffffff,    /* RA_DBBMULTCONSTSSB0 */
        0xffffffff,    /* RA_DBBMULTCONSTSSB1 */
        CLA_DBALUCNTL(RC_DBALUCNTL_ALUOPERATION_B, 0),
    };

    if (gs == NULL)
    {
            /* Make this a simple CPU copy for now */
        for (i = 0; i < rows; i++)
        {
            memcpy(dst, (void *)src, dstBytesPerRow);
            dst += dstBytesPerRow;
            src += srcBytesPerRow;
        }
    }
    else
    {
            /* Use a Bitmap Item to set the destination buffer as being
             * the buffer "dst".
             */
        Item bmI;
        uint32 size;
        
        if (bs)
        {
                /*  We already have a Bitmap Item to use! */
            bs->bmI = ModifyGraphicsItemVA(bs->bmI,
                                           BMTAG_WIDTH, width,
                                           BMTAG_HEIGHT, rows,
                                           BMTAG_TYPE, type,
                                           BMTAG_BUFFER, dst,
                                           TAG_END);
            bmI = bs->bmI;
        }
        else
        {
                /* Need to create a temporary Bitmap Item */
            bmI = CreateItemVA(MKNODEID(NST_GRAPHICS, GFX_BITMAP_NODE),
                               BMTAG_WIDTH, width,
                               BMTAG_HEIGHT, rows,
                               BMTAG_TYPE, type,
                               BMTAG_BUFFER, dst,
                               BMTAG_RENDERABLE, TRUE,
                               TAG_END);
        }
        if (bmI <= 0)
        {
            return(BLITTER_ERR_BADITEM);
        }

        err = GS_SetDestBuffer(gs, bmI);
        if (err < 0)
        {
            return(err);
        }

            /* Enable dblending. We will use 100% of the source, and 0% of the
             * destination to copy.
             */
        doBlitSnippet[4] = ((type == BMTYPE_32) ?
                            CLA_DBSRCCNTL(0, 1) : CLA_DBSRCCNTL(1, 0));
        doBlitSnippet[5] = (uint32)src;
        doBlitSnippet[6] = (srcBytesPerRow /
                            ((type == BMTYPE_32) ? 4 : 2));
        size = sizeof(doBlitSnippet);
        GS_Reserve(gs, ((size / sizeof(uint32)) + 11)); /* 2 Sync words, 9 vertex words */
        CLT_Sync(GS_Ptr(gs));
        memcpy(*GS_Ptr(gs), doBlitSnippet, size);
        BUMP_GSPTR(gs, size);

            /* Vertex instructions to "render" the rectangular area. */
        CLT_TRIANGLE(GS_Ptr(gs), 1, RC_FAN, 0, 0, 0, 4);
        CLT_Vertex(GS_Ptr(gs), 0.0, 0.0);
        CLT_Vertex(GS_Ptr(gs), 0.0, (gfloat)rows);
        CLT_Vertex(GS_Ptr(gs), (gfloat)width, (gfloat)rows);
        CLT_Vertex(GS_Ptr(gs), (gfloat)width, 0.0);
        
        if (bs == NULL)
        {
            DeleteItem(bmI);
        }
    }
    return(0);
}

#define EXPANSIONFORMATMASK 0x00001FFF
void InitBlitDimensions(BlitObject *bo, CltTxData *ctd, uint32 width, uint32 height, uint32 bytesPerPixel)
{
    CltTxLOD *lod = ctd->texelData;
    CltTxDCI *dci = ctd->dci;
    
    DPRT(("BlitDims = %ld x %ld x %ld\n", width, height, bytesPerPixel));
    
    if (bo->bo_tbl)
    {
        bo->bo_tbl->txb_expType = ctd->expansionFormat;
        if (dci)
        {
            bo->bo_tbl->txb_srcType[0] = ((((uint32)dci->texelFormat[1] & EXPANSIONFORMATMASK) << 16) |
                                          ((uint32)dci->texelFormat[0] & EXPANSIONFORMATMASK));
            bo->bo_tbl->txb_srcType[1] = ((((uint32)dci->texelFormat[3] & EXPANSIONFORMATMASK) << 16) |
                                          ((uint32)dci->texelFormat[2] & EXPANSIONFORMATMASK));
            bo->bo_tbl->txb_constant[0] = dci->expColor[0];
            bo->bo_tbl->txb_constant[1] = dci->expColor[1];
            bo->bo_tbl->txb_tabConst[0] = dci->expColor[2];
            bo->bo_tbl->txb_tabConst[1] = dci->expColor[3];
    }            
    }
    if (bo->bo_txl)
    {
        bo->bo_txl->txl_src = (uint32)lod->texelData;
        bo->bo_txl->txl_count = height;
        bo->bo_txl->txl_width = CLA_TXTLDWIDTH((width * (bytesPerPixel * 8)),
                                               (width * (bytesPerPixel * 8)));
        bo->bo_txl->txl_uvMax = CLA_TXTUVMAX((width - 1), (height - 1));
    }
    if (bo->bo_txdata)
    {
        bo->bo_txdata->btd_txData.minX = width;
        bo->bo_txdata->btd_txData.minY = height;
        bo->bo_txdata->btd_txData.bitsPerPixel = (bytesPerPixel * 8);
   }
}

Err Blt_RectangleToBlitObject(GState *gs, const BlitObject *bo, Item srcBitmap, const BlitRect *rect)
{
    Bitmap *bm;
    BlitStuff *bs;
    uint32 bytesPerPixel;
    uint32 bytesPerRow, bmBytesPerRow;
    uint32 width, height, size;
    CltTxLOD *lod;
    CltTxData *ctd;
    ubyte *dst, *src;
    Err err;
    
    if (bo == NULL)
    {
        return(BLITTER_ERR_BADPTR);
    }
    if (bo->bo_txdata == NULL)
    {
        return(BLITTER_ERR_NOTXDATA);
    }

    bs = BLITSTUFF(bo);

    /* Calculate how much RAM we need for the texture. */
    bm = (Bitmap *)LookupItem(srcBitmap);
    if (bm == NULL)
    {
        return(BLITTER_ERR_BADITEM);
    }
    bytesPerPixel = ((bm->bm_Type == BMTYPE_32) ? 4 : 2);
    width = (uint32)(rect->max.x - rect->min.x + 1);
    height = (uint32)(rect->max.y - rect->min.y + 1);
    size = (width * height * bytesPerPixel);
    DPRT(("Need (%ld * %ld * %ld) = %ld bytes\n", width, height, bytesPerPixel, size));

    ctd = &bo->bo_txdata->btd_txData;
    lod = ctd->texelData;
        /* Restore the lod->texelData as this may have been changed
         * by Blt_SetTexture().
         */
    lod->texelData = TEXTURESLICE(bo->bo_txdata)->texelData;
        /* If there is a texelbuffer already, but it is too small and owned by the app,
         * then there is nothing more we can do here.
         */
    if (lod->texelData && (lod->texelDataSize < size) &&
        (!(bo->bo_txdata->btd_header.bsh_flags & SYSTEM_TEXEL)))
    {
        return(BLITTER_ERR_SMALLTEXEL);
    }
    /* If the buffer is too small, then allocate another one that is larger */
    if ((lod->texelData == NULL) || (lod->texelDataSize < size))
    {
        if (lod->texelData)
        {
            DPRT(("Freeing old texel @ 0x%lx size %ld\n", lod->texelData, lod->texelDataSize));
            FreeMem(lod->texelData, lod->texelDataSize);
            bo->bo_txdata->btd_header.bsh_flags &= ~SYSTEM_TEXEL;
        }
        lod->texelData = AllocMem(size, bm->bm_BufMemType);
        TEXTURESLICE(bo->bo_txdata)->texelData = lod->texelData;
        if (lod->texelData == NULL)
        {
            return(BLITTER_ERR_NOMEM);
        }
        bo->bo_txdata->btd_header.bsh_flags |= SYSTEM_TEXEL;
        lod->texelDataSize = size;
        DPRT(("Allocated new texel @ 0x%lx size 0x%lx\n", lod->texelData, lod->texelDataSize));
    }
    if ((ctd->minX != width) || (ctd->minY != height) || (ctd->bitsPerPixel != (bytesPerPixel * 8)))
    {
        /* If the texture was sliced, then the sliced vertices can no longer be
         * valid.
         */
        RemoveSlices(bo);
    }

    DPRT(("Blit from bitmap address 0x%lx to texel @ 0x%lx\n",
          PixelAddress(srcBitmap, (uint32)rect->min.x, (uint32)rect->min.y),
          lod->texelData));

    bytesPerRow = (width * bytesPerPixel);   /* bytes to copy */
    dst = (ubyte *)lod->texelData;
    src = (ubyte *)PixelAddress(srcBitmap, (uint32)rect->min.x, (uint32)rect->min.y);
    if (src == NULL)
    {
            /* rectangle is probably out of the bounds of the src bitmap. */
        return(BLITTER_ERR_BADSRCBOUNDS);
    }
    bmBytesPerRow = ((uint32)PixelAddress(srcBitmap, 0, 1) - (uint32)PixelAddress(srcBitmap, 0, 0));
    DPRT(("memcpy(). dst = 0x%lx, src = 0x%lx, bytesPerRow = %ld, bmBytesPerRow = %ld\n",
          dst, src, bytesPerRow, bmBytesPerRow));
    err = doBlit(gs, src, dst, bmBytesPerRow, bytesPerRow, height, width, bm->bm_Type, bs);
    if (err < 0)
    {
        return(err);
    }
    
        /* Now that was succesful, we should set up some of the parameters of the
         * destination-blit TEinstructions.
         */
    ctd->expansionFormat = ((bm->bm_Type == BMTYPE_32) ?
                            CLA_TXTEXPTYPE(8, 7, 0, 1, 1, 1, 1) :
                            CLA_TXTEXPTYPE(5, 0, 0, 1, 1, 0, 1));
    InitBlitDimensions(bo, ctd, width, height, bytesPerPixel);
        /* If the txdata is not shared across other BlitterObjects,
         * then the BlitStuff's width, height and bpp values are safe
         * to cache.
         */
    if ((bo->bo_txdata) && (bo->bo_txdata->btd_header.bsh_usageCount == 1))
    {
        bo->bo_txdata->btd_header.bsh_flags |= DIMENSIONS_VALID;
    }
    
    return(0);
}

Err Blt_BlitObjectToBitmap(GState *gs, const BlitObject *bo, Item dstBitmap, uint32 flags)
{
    Bitmap *bm;
    Err err;
    uint32 bsFlags;
    bool sliced = FALSE;
    bool resetUGC = FALSE;
    
    if ((bo == NULL) || (gs == NULL))
    {
        return(BLITTER_ERR_BADPTR);
    }

    bm = (Bitmap *)LookupItem(dstBitmap);
    if (bm == NULL)
    {
        return(BLITTER_ERR_BADITEM);
    }

    /* Do we need to calculate the sliced vertices? */
    if (bo->bo_txdata && (bo->bo_txdata->btd_txData.texelData->texelDataSize > TRAM_SIZE) &&
        (!(BLITSTUFF(bo)->flags & VERTICES_SLICED)) && bo->bo_vertices)
    {
        DPRT(("SliceVertices\n"));
        err = SliceVertices(bo);
        if (err < 0)
        {
            return(err);
        }
    }
    bsFlags = BLITSTUFF(bo)->flags;
    if (bsFlags & VERTICES_SLICED)
    {
        sliced = TRUE;
    }

    /* Do we need to clip the vertices?
     * Clip if clipping is enabled but we have not yet been through the clipping code.
     */
    if (bo->bo_vertices &&
        ((bsFlags & (CLIP_ENABLED | CLIP_CALCULATED)) == CLIP_ENABLED))
    {
        DPRT(("ClipVertices\n"));
        err = ClipVertexList(bo);
        if (err < 0)
        {
            return(err);
        }
        bsFlags = BLITSTUFF(bo)->flags;
    }
    

    /* For speed, call GS_Reserve() with the worst-case size.
     * If this texture is sliced, then we need to reserve enough instructions for all
     * the vertices in the sliced vertex list, including one sync instruction per slice.
     */
    GS_Reserve(gs,
               ((sizeof(TxBlendSnippet) +
                 sizeof(TxLoadSnippet) +
                 sizeof(PIPLoadSnippet) +
                 sizeof(DBlendSnippet) -
                 (sizeof(BlitterSnippetHeader) * 4)  +
                 (sliced ? (BLITSTUFF(bo)->slicedVerticesSize) :
                  (bo->bo_vertices ? ((bo->bo_vertices->vtx_vertices * bo->bo_vertices->vtx_wordsPerVertex) + 1) : 0)))
                 / sizeof(uint32)) + 1 +
               ((bsFlags & CLIP_ENABLED) ? 6 : 0) + 
               (sliced ? TEXTURESLICE(bo->bo_txdata)->ySlice : 0) +
               2 /* Perspective on/off */ );

    /* Start with a SYNC */
    CLT_Sync(GS_Ptr(gs));

    if ((!(flags & SKIP_SNIPPET_PIP)) && bo->bo_pip)
    {
        uint32 size = (sizeof(PIPLoadSnippet) - sizeof(BlitterSnippetHeader));

        memcpy(*GS_Ptr(gs), &bo->bo_pip->ppl_instruction1, size);
        BUMP_GSPTR(gs, size);
    }
    if ((!(flags & SKIP_SNIPPET_TBLEND)) && bo->bo_tbl)
    {
        uint32 size = (sizeof(TxBlendSnippet) - sizeof(BlitterSnippetHeader));

        memcpy(*GS_Ptr(gs), &bo->bo_tbl->txb_instruction1, size);
        BUMP_GSPTR(gs, size);
    }
    if ((!(flags & SKIP_SNIPPET_TXLOAD)) && bo->bo_txl)
    {
        uint32 size = (sizeof(TxLoadSnippet) - sizeof(BlitterSnippetHeader));

        if (!sliced)
        {
            /* TxLoad data for sliced textures is in the slicedVertices buffer */
            memcpy(*GS_Ptr(gs), &bo->bo_txl->txl_instruction1, size);
            BUMP_GSPTR(gs, size);
        }
    }
    if ((!(flags & SKIP_SNIPPET_DBLEND)) && bo->bo_dbl)
    {
        uint32 size;
        uint32 ugc = bo->bo_dbl->dbl_userGenCntl;
        
        if (bsFlags & CLIP_ENABLED)
        {
            bo->bo_dbl->dbl_userGenCntl |= ((bsFlags & CLIP_OUT) ? 
                                            FV_DBUSERCONTROL_WINCLIPOUTEN_MASK :
                                            FV_DBUSERCONTROL_WINCLIPINEN_MASK);
            CLT_DBXWINCLIP(GS_Ptr(gs), (uint32)BLITSTUFF(bo)->tl.x, (uint32)BLITSTUFF(bo)->br.x);
            CLT_DBYWINCLIP(GS_Ptr(gs), (uint32)BLITSTUFF(bo)->tl.y, (uint32)BLITSTUFF(bo)->br.y);
            resetUGC = TRUE;
        }
        
            /* If the DBlendEn bit is not set, then we don't need the whole DBlend snippet */
        if (bo->bo_dbl->dbl_userGenCntl & FV_DBUSERCONTROL_BLENDEN_MASK)
        {
            DPRT(("Enabled DBlend\n"));
            bo->bo_dbl->dbl_srcCntl = ((bm->bm_Type == BMTYPE_32) ?
                                       CLA_DBSRCCNTL(0, 1) : CLA_DBSRCCNTL(1, 0));
            bo->bo_dbl->dbl_srcBaseAddr = (uint32)PixelAddress(dstBitmap, 0, 0);
            bo->bo_dbl->dbl_srcXStride = bm->bm_Width;
            bo->bo_dbl->dbl_srcOffset = CLA_DBSRCOFFSET(0, 0);
            size = (sizeof(DBlendSnippet) - sizeof(BlitterSnippetHeader));
        }
        else
        {
            DPRT(("DBlend disabled\n"));
            size = 12;  /* Just dbl_instruction1 -> dbl_instruction2 */
        }
        memcpy(*GS_Ptr(gs), &bo->bo_dbl->dbl_instruction1, size);
        BUMP_GSPTR(gs, size);
        bo->bo_dbl->dbl_userGenCntl = ugc;
    }
    if ((!(flags & SKIP_SNIPPET_VERTICES)) && bo->bo_vertices)
    {
        uint32 size;
        uint32 perspective;

        /* If the vertices list does not include 1/w values, then disiable
         * perspective correction.
         */
        perspective = (bo->bo_vertices->vtx_instruction & FV_TRIANGLE_PERSPECTIVE_MASK);
        if (perspective)
        {
            CLT_ClearRegister(gs->gs_ListPtr, ESCNTL, FV_ESCNTL_PERSPECTIVEOFF_MASK);
        }
        else
        {
            CLT_SetRegister(gs->gs_ListPtr, ESCNTL, FV_ESCNTL_PERSPECTIVEOFF_MASK);
        }
        
        if (bsFlags & VERTICES_CLIPPED)
        {
            size = BLITSTUFF(bo)->clippedBytesUsed;
            DPRT(("using clipped vertices range 0x%lx 0x%lx (0x%lx 0x%lx)\n",
                  BLITSTUFF(bo)->clippedVertices, ((uint32)BLITSTUFF(bo)->clippedVertices + size),
                  *GS_Ptr(gs), ((uint32)*GS_Ptr(gs) + size)));
            memcpy(*GS_Ptr(gs), BLITSTUFF(bo)->clippedVertices, size);
            BUMP_GSPTR(gs, size);
        }
        else if (sliced)
        {
            size = BLITSTUFF(bo)->slicedBytesUsed;
            DPRT(("using sliced vertices range 0x%lx 0x%lx (0x%lx 0x%lx)\n",
                  BLITSTUFF(bo)->slicedVertices, ((uint32)BLITSTUFF(bo)->slicedVertices + size),
                  *GS_Ptr(gs), ((uint32)*GS_Ptr(gs) + size)));
            memcpy(*GS_Ptr(gs), BLITSTUFF(bo)->slicedVertices, size);
            BUMP_GSPTR(gs, size);
        }
        else
        {
            size = (sizeof(uint32) + /* vtx_instruction */
                    (bo->bo_vertices->vtx_vertices * bo->bo_vertices->vtx_wordsPerVertex * sizeof(gfloat)));
            memcpy(*GS_Ptr(gs), &bo->bo_vertices->vtx_instruction, size);
            BUMP_GSPTR(gs, size);
        }
    }

    if (resetUGC)
    {
        CLT_Sync(GS_Ptr(gs));
        CLT_ClearRegister(gs->gs_ListPtr, DBUSERCONTROL,
                          (FV_DBUSERCONTROL_WINCLIPINEN_MASK | FV_DBUSERCONTROL_WINCLIPOUTEN_MASK));
    }
    
    return(0);
}

Err Blt_RectangleToBuffer(GState *gs, Item srcBitmap, const BlitRect *rect, void *buffer)
{
    Bitmap *bm;
    uint32 bytesPerPixel;
    uint32 bytesPerRow, bmBytesPerRow;
    uint32 width, height;
    ubyte *src;
    Err err;
    
    if ((rect == NULL) || (buffer == NULL))
    {
        return(BLITTER_ERR_BADPTR);
    }

    /* Calculate how much RAM we need for the texture. */
    bm = (Bitmap *)LookupItem(srcBitmap);
    if (bm == NULL)
    {
        return(BLITTER_ERR_BADITEM);
    }
    bytesPerPixel = ((bm->bm_Type == BMTYPE_32) ? 4 : 2);
    width = (uint32)(rect->max.x - rect->min.x + 1);
    height = (uint32)(rect->max.y - rect->min.y + 1);
    src = (ubyte *)PixelAddress(srcBitmap, (uint32)rect->min.x, (uint32)rect->min.y);
    bytesPerRow = (width * bytesPerPixel);   /* bytes to copy */
    bmBytesPerRow = ((uint32)PixelAddress(srcBitmap, 0, 1) - (uint32)PixelAddress(srcBitmap, 0, 0));
    DPRT(("memcpy(). buffer = 0x%lx, src = 0x%lx, bytesPerRow = %ld, bmBytesPerRow = %ld\n",
          buffer, src, bytesPerRow, bmBytesPerRow));
    err = doBlit(gs, src, buffer, bmBytesPerRow, bytesPerRow, height, width, bm->bm_Type, NULL);
    
    return(err);
}



