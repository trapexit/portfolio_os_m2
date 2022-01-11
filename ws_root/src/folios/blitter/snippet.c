/* @(#) snippet.c 96/11/19 1.11
 *
 *  Code to generate and handle snippets
 */

#include <kernel/types.h>
#include <kernel/mem.h>
#include <graphics/blitter.h>
#include <stdio.h>
#include <string.h>
#include "blitter_internal.h"

#define DPRT(x) /* printf x;*/

void RemoveSlices(BlitObject *bo);
Err AssociateBOToVtx(BlitObject *bo, VerticesSnippet *vtx);
void RemoveBOToVtxAssociation(BlitObject *bo, VerticesSnippet *vtx);

const uint32 snippetSizes[] =
{
    sizeof(DBlendSnippet),
    sizeof(TxBlendSnippet),
    sizeof(TxLoadSnippet),
    sizeof(PIPLoadSnippet),
    (sizeof(BltTxData) + sizeof(TextureSlice)),
};

extern const TxBlendSnippet defaultTxBlend;
extern const TxLoadSnippet defaultTxLoad;
extern const PIPLoadSnippet defaultPIPLoad;
extern const DBlendSnippet defaultDBlend;
extern const BltTxData defaultTxData;
const BlitterSnippetHeader * const snippetDefaults[] =
{
    &defaultDBlend.dbl_header,
    &defaultTxBlend.txb_header,
    &defaultTxLoad.txl_header,
    &defaultPIPLoad.ppl_header,
    &defaultTxData.btd_header,
};

BlitterSnippetHeader **GetSnippetHeader(BlitObject *bo, uint32 type)
{
    BlitterSnippetHeader **bsh;

    switch (type)
    {
        case (BLIT_TAG_DBLEND):
        {
            bsh = (BlitterSnippetHeader **)&bo->bo_dbl;
            break;
        }
        case (BLIT_TAG_TBLEND):
        {
            bsh = (BlitterSnippetHeader **)&bo->bo_tbl;
            break;
        }
        case (BLIT_TAG_TXLOAD):
        {
            bsh = (BlitterSnippetHeader **)&bo->bo_txl;
            break;
        }
        case (BLIT_TAG_PIP):
        {
            bsh = (BlitterSnippetHeader **)&bo->bo_pip;
            break;
        }
        case (BLIT_TAG_TXDATA):
        {
            bsh = (BlitterSnippetHeader **)&bo->bo_txdata;
            break;
        }
        case (BLIT_TAG_VERTICES):
        {
            bsh = (BlitterSnippetHeader **)&bo->bo_vertices;
            break;
        }
        default:
        {
            bsh = NULL;
            break;
        }
    }
    return(bsh);
}

void *CreateSnippet(uint32 type, BlitterSnippetHeader *bsh)
{
    BlitterSnippetHeader *snip;
    uint32 size;

    size = snippetSizes[type - BLIT_TAG_DBLEND];
    snip = (BlitterSnippetHeader *)AllocMem(size, MEMTYPE_NORMAL);
    DPRT(("Snippet type %ld at 0x%lx\n", type, snip));
    if (snip)
    {
        memcpy(snip, bsh, size);
        snip->bsh_flags = SYSTEM_SNIPPET;
    }

    return((void *)snip);
}


void *Blt_CreateSnippet(uint32 type)
{
    void *snip;
    switch (type)
    {
        case (BLIT_TAG_DBLEND):
        case (BLIT_TAG_TBLEND):
        case (BLIT_TAG_TXLOAD):
        case (BLIT_TAG_PIP):
        {
            snip = CreateSnippet(type, snippetDefaults[type - BLIT_TAG_DBLEND]);
            break;
        }
        case (BLIT_TAG_TXDATA):
        {
            snip = CreateSnippet(type, snippetDefaults[type - BLIT_TAG_DBLEND]);
            if (snip)
            {
                BltTxData *btd = (BltTxData *)snip;
                btd->btd_txData.texelData = &btd->btd_txLOD;
                btd->btd_txData.dci = NULL;
                /* Point btd_slice to the end of this BltTxData structure */
                btd->btd_slice = (void *)((uint32)snip + sizeof(BltTxData));
                memset(btd->btd_slice, 0, sizeof(TextureSlice));
            }
            break;
        }
        case (BLIT_TAG_VERTICES):
        {
            VerticesSnippet *vtx;
            Err err;

            /* The vertices snippet is created through Blt_CreateVertices(). */
            snip = NULL;
            err = Blt_CreateVertices(&vtx, CLA_TRIANGLE(1, RC_FAN, 1, 1, 0, 4));
            if (err >= 0)
            {
                snip = (void *)vtx;
                DPRT(("Snippet type %ld at 0x%lx\n", type, snip));
            }
            break;
        }
        default:
        {
            snip = NULL;
            break;
        }
    }

    return(snip);
}

Err Blt_DeleteSnippet(const void *snippet)
{
    BlitterSnippetHeader *snip;
    Err err = 0;

    if (snippet == NULL)
    {
        return(BLITTER_ERR_BADPTR);
    }

    snip = (BlitterSnippetHeader *)snippet;
    /* Can't free any snippets that are in use. */
    if (snip->bsh_usageCount != 0)
    {
        return(BLITTER_ERR_SNIPPETUSED);
    }
    /* The app needs to take care of any snippet it allocates itself. */
    if (!(snip->bsh_flags & SYSTEM_SNIPPET))
    {
        return(BLITTER_ERR_APPSSNIPPET);
    }

    switch (snip->bsh_type)
    {
        case (BLIT_TAG_TXDATA):
        {
            if (snip->bsh_flags & SYSTEM_TEXEL)
            {
                CltTxLOD *data = ((BltTxData *)snip)->btd_txData.texelData;
                    /* Restore the shadow pointer in case data->texelData
                     * was changed by Blt_SetTexture().
                     */
                data->texelData = TEXTURESLICE((BltTxData *)snip)->texelData;
                DPRT(("FreeMem() the texeldata at 0x%lx (size 0x%lx)\n", data->texelData, data->texelDataSize));
                FreeMem(data->texelData, data->texelDataSize);
            }
            /* Fall through */
        }
        case (BLIT_TAG_DBLEND):
        case (BLIT_TAG_TBLEND):
        case (BLIT_TAG_TXLOAD):
        case (BLIT_TAG_PIP):
        {
            FreeMem(snippet, snippetSizes[snip->bsh_type - BLIT_TAG_DBLEND]);
            break;
        }
        case (BLIT_TAG_VERTICES):
        {
            BOArray *boa;
            VerticesSnippet *v = (VerticesSnippet *)snippet;

            boa = BOARRAY(v);
            while (boa)
            {
                BOArray *tmp;
                tmp = boa->next;
                FreeMem(boa, sizeof(BOArray));
                boa = tmp;
            }

            FreeMem(snippet, (sizeof(VerticesSnippet) + (v->vtx_vertices * v->vtx_wordsPerVertex * sizeof(gfloat)) - sizeof(gfloat)));
            break;
        }
        default:
        {
            err = BLITTER_ERR_BADTYPE;
            break;
        }
    }

    return(err);
}

Err Blt_CopySnippet(const BlitObject *src, BlitObject *dest, void **original, uint32 type)
{
    BlitterSnippetHeader **dstH, **srcH;
    void *newSnippet;
    Err err = 0;

    if ((src == NULL) || (dest == NULL))
    {
        return(err);
    }

    dstH = GetSnippetHeader(dest, type);
    srcH = GetSnippetHeader(src, type);
    if ((dstH == NULL) || (srcH == NULL))
    {
        /* unknown type passed in */
        return(err);
    }

    /* Note the snippet we will be replacing. */
    if (original)
    {
        *original = (void *)*dstH;
    }

    /* ...and allocate a new one, copying the src one at the same time */
    newSnippet = CreateSnippet(type, *srcH);
    if (newSnippet == NULL)
    {
        err = BLITTER_ERR_NOMEM;
    }
    else
    {
        BlitterSnippetHeader *snip;

            /* Decrement the usage count of the snippet being replaced */
        if (*dstH)
        {
            (*dstH)->bsh_usageCount--;
        }
        /* and fix up the header */
        snip = (BlitterSnippetHeader *)newSnippet;
        snip->bsh_usageCount = 1;
        snip->bsh_flags &= ~DIMENSIONS_VALID;
		/* If the src snippet was created by Blt_CreateBlitObject(), then so
		 * is this one. This means that the snippet will be deleted by
		 * Blt_DeleteBlitObject(). If the snippet was not created by
		 * BLt_CreateBlitObject(), then it must have been allocated by the
		 * application with Blt_CreateSnippet(), so the app is responsible
		 * for freeing it.
		 */
		snip->bsh_flags |= ((*srcH)->bsh_flags & CBO_SNIPPET);
		/* and finally set it */
        *dstH = snip;

        /* If the dest BlitObject has just got itself a new TxData or vertices,
         * then we should clean up its "sliced" status.
         */
        if ((type == BLIT_TAG_TXDATA) || (type == BLIT_TAG_VERTICES))
        {
            RemoveSlices(dest);
        }
        /* If the dest BlitObject just got itself some new vertices, then
         * associate the BlitObject with the VerticesSnippet.
         */
        if ((type == BLIT_TAG_VERTICES) && (dest->bo_vertices))
        {
            err = AssociateBOToVtx(dest, dest->bo_vertices);
        }
    }

    return(err);
}

void *Blt_ReuseSnippet(const BlitObject *src, BlitObject *dest, uint32 type)
{
    BlitterSnippetHeader **dstH, **srcH;
    void *snip;

    /* We have to reduce the count of the snippet being removed from the
     * dest BlitObject, and increment the count of the snippet being copied,
     * bearing in mind that the snippet we are copying may be NULL.
     */
    if ((src == NULL) || (dest == NULL))
    {
        return(NULL);
    }

    dstH = GetSnippetHeader(dest, type);
    srcH = GetSnippetHeader(src, type);
    if ((dstH == NULL) || (srcH == NULL))
    {
        /* unknown type passed in */
        return(NULL);
    }

    /* Note the snippet we will be replacing. */
    snip = (void *)*dstH;
    /* ...and copy. */
    *dstH = *srcH;
    if (*srcH)
    {
        (*srcH)->bsh_usageCount++;
        (*srcH)->bsh_flags &= ~DIMENSIONS_VALID;
        DPRT(("Usage count of src snippet 0x%lx (type 0x%lx) = %ld\n", *srcH, type, (*srcH)->bsh_usageCount));
    }

    if (snip)
    {
        /* Decrement its usage counter */
        BlitterSnippetHeader *head = (BlitterSnippetHeader *)snip;
        head->bsh_usageCount--;
        head->bsh_flags &= ~DIMENSIONS_VALID;
        DPRT(("Usage count of dest snippet 0x%lx (type 0x%lx) = %ld\n", snip, type, head->bsh_usageCount));
    }

        /* If the dest BlitObject has just got itself a new TxData or vertices,
         * then we should clean up its "sliced" status.
         */
    if ((type == BLIT_TAG_TXDATA) || (type == BLIT_TAG_VERTICES))
    {
        RemoveSlices(dest);
    }
        /* If the dest BlitObject just got itself some new vertices, then
         * associate the BlitObject with the VerticesSnippet.
         */
    if ((type == BLIT_TAG_VERTICES) && (dest->bo_vertices))
    {
        AssociateBOToVtx(dest, dest->bo_vertices);
    }

    return(snip);
}

void Blt_SwapSnippet(BlitObject *src, BlitObject *dest, uint32 type)
{
    BlitterSnippetHeader **dstH, **srcH;
    BlitterSnippetHeader *tmp;

    /* Just exchange the two snippets over. No need to modify usage counts. */
    if ((src == NULL) || (dest == NULL))
    {
        return;
    }

    dstH = GetSnippetHeader(dest, type);
    srcH = GetSnippetHeader(src, type);
    if ((dstH == NULL) || (srcH == NULL))
    {
        /* unknown type passed in */
        return;
    }

    tmp = *dstH;
    *dstH = *srcH;
    *srcH = tmp;

        /* If the dest or src BlitObject has just got itself a new TxData or vertices,
         * then we should clean up its "sliced" status.
         */
    if ((type == BLIT_TAG_TXDATA) || (type == BLIT_TAG_VERTICES))
    {
        RemoveSlices(dest);
        RemoveSlices(src);
        if (dest->bo_vertices)
        {
            RemoveBOToVtxAssociation(src, dest->bo_vertices);
            AssociateBOToVtx(dest, dest->bo_vertices);
        }
        if (src->bo_vertices)
        {
            RemoveBOToVtxAssociation(dest, src->bo_vertices);
            AssociateBOToVtx(src, src->bo_vertices);
        }
        (*srcH)->bsh_flags &= ~DIMENSIONS_VALID;
        (*dstH)->bsh_flags &= ~DIMENSIONS_VALID;
    }

    return;
}

void *Blt_RemoveSnippet(BlitObject *bo, uint32 type)
{
    BlitterSnippetHeader **srcH;
    void *snip;

    /* We have to reduce the count of the snippet being removed from the
     * BlitObject, and replace it with NULL
     * bearing in mind that the snippet we are removing may be NULL.
     */
    if (bo == NULL)
    {
        return(NULL);
    }

    srcH = GetSnippetHeader(bo, type);
    if (srcH == NULL)
    {
        /* unknown type passed in */
        return(NULL);
    }

    /* Note the snippet we will be replacing. */
    snip = (void *)*srcH;

    if (*srcH)
    {
        /* Decrement the counter of this snippet */
        (*srcH)->bsh_usageCount--;
        DPRT(("Usage count of snippet 0x%lx (type 0x%lx) = %ld\n", *srcH, type, (*srcH)->bsh_usageCount));
    }
    *srcH = NULL;

        /* If the dest BlitObject has just got itself a new TxData or vertices,
         * then we should clean up its "sliced" status.
         */
    if ((type == BLIT_TAG_TXDATA) || (type == BLIT_TAG_VERTICES))
    {
        RemoveSlices(bo);
        if (bo->bo_vertices)
        {
            RemoveBOToVtxAssociation(bo, bo->bo_vertices);
        }
    }

    return(snip);
}

void *Blt_SetSnippet(BlitObject *bo, void *snippet)
{
    BlitterSnippetHeader **srcH;
    void *snip;
    uint32 type;

        /* Add this snippet to the BlitObject and increment its usage
         * count. Decrement the usage count of the snippet that is being replaced,
         * and return that snippet.
         */
    type = ((BlitterSnippetHeader *)snippet)->bsh_type;
    snip = Blt_RemoveSnippet(bo, type);
    srcH = GetSnippetHeader(bo, type);
    *srcH = (BlitterSnippetHeader *)snippet;
    ((BlitterSnippetHeader *)snippet)->bsh_usageCount++;
    ((BlitterSnippetHeader *)snippet)->bsh_flags &= ~DIMENSIONS_VALID;

        /* If the dest BlitObject has just got itself a new TxData or vertices,
         * then we should clean up its "sliced" status.
         */
    if ((type == BLIT_TAG_TXDATA) || (type == BLIT_TAG_VERTICES))
    {
        RemoveSlices(bo);
    }
        /* If the dest BlitObject just got itself some new vertices, then
         * associate the BlitObject with the VerticesSnippet.
         */
    if ((type == BLIT_TAG_VERTICES) && (bo->bo_vertices))
    {
        AssociateBOToVtx(bo, bo->bo_vertices);
    }

    return(snip);
}
