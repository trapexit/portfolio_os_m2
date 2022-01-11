/* @(#) texturesrc.c 96/08/15 1.1
*
* Code to change the source texture of a BlitObject.
*/

#include <kernel/types.h>
#include <kernel/mem.h>
#include <graphics/blitter.h>
#include <stdio.h>
#include <string.h>
#include "blitter_internal.h"

#define DPRT(x) /* printf x */

Err Blt_SetTexture(BlitObject *bo, void *texelData)
{
    TxLoadSnippet *txl;
    BltTxData *btd;
    CltTxLOD *data;
    uint32 diff;
    
    txl = bo->bo_txl;
    if (txl == NULL)
    {
        return(BLITTER_ERR_NOTXLOAD);
    }

    btd = bo->bo_txdata;
    if (btd == NULL)
    {
        return(BLITTER_ERR_NOTXDATA);
    }

    data = btd->btd_txData.texelData;
    if (data == NULL)
    {
        return(BLITTER_ERR_NOTXDATA);
    }
  
    data->texelData = texelData;
    diff  = ((uint32)texelData - txl->txl_src);
    txl->txl_src = (uint32)texelData;

        /* Now that the housekeeping is taken care of,
         * see if there are any more txl_src fields to set
         * in the sliced vertices.
         */
    if (BLITSTUFF(bo)->flags & VERTICES_SLICED)
    {
        BlitStuff *bs = BLITSTUFF(bo);
        TextureSlice *ts = TEXTURESLICE(btd);
        gfloat *v = bs->slicedVertices;
        uint32 i;
        uint32 totalStrips = 0;
        uint32 instruction;
        uint32 wordsPer;
        uint32 count;

        if (bo->bo_vertices == NULL)
        {
            return(BLITTER_ERR_NOVTX);
        }
        wordsPer = bo->bo_vertices->vtx_wordsPerVertex;
        
        for (i = 0; v && i < bs->ySlicesUsed; i++)
        {
            if (ts->txlOrig)
            {
                char **txtsrc;
                
                /* This should be the TxLoad stuff */
                v += 2;         /* Sync instruction */
                txtsrc = (char **)&v[5];  /* this is txl_src */
                *txtsrc += diff;
                v += ((sizeof(TxLoadSnippet) - sizeof(BlitterSnippetHeader)) / sizeof(gfloat *));
            }
            instruction = *((uint32 *)v);
            while (((instruction & 0xf0000000) == RC_WRITE_VERTEX) &&
                   (totalStrips < bs->totalStrips))
            {
                v++;
                totalStrips++;
                count = ((instruction & FV_TRIANGLE_COUNT_MASK) + 1);
                v += (wordsPer * count);
                instruction = *((uint32 *)v);
            }
        }
    }

    return(0);
}

Err Blt_GetTexture(BlitObject *bo, void **texelData)
{
    if (bo->bo_txdata)
    {
        if (bo->bo_txdata->btd_txData.texelData)
        {
            *texelData = bo->bo_txdata->btd_txData.texelData->texelData;
            return(0);
        }
    }
    return(BLITTER_ERR_NOTXDATA);
}
