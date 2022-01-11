/* @(#) blitobject.c 96/11/19 1.9
 *
 *  Code to handle BlitObject housekeeping
 */

#include <kernel/types.h>
#include <kernel/mem.h>
#include <graphics/blitter.h>
#include <stdio.h>
#include "blitter_internal.h"

#define DPRT(x) /* printf x;*/
#define CHK_NULL(x) if ((x) == NULL) {err = BLITTER_ERR_NOMEM; goto cleanup;}

static void DecrementSnippetUsage(BlitterSnippetHeader *sh, uint32 size);
void RemoveSlices(BlitObject *bo);
void RemoveClippedVertices(BlitObject *bo);
Err AssociateBOToVtx(BlitObject *bo, VerticesSnippet *vtx);
void RemoveBOToVtxAssociation(BlitObject *bo, VerticesSnippet *vtx);

Err Blt_CreateBlitObject(BlitObject **_bo, const TagArg *tagList)
{
    BlitObject *bo;
    TagArg *next, *tag;
    Err err = 0;
    uint32 apps = 0;

    DPRT(("Blt_CreateBlitObject()\n"));
    if (_bo == NULL)
    {
        return(BLITTER_ERR_BADPTR);
    }

    bo = (BlitObject *)AllocMem((sizeof(BlitObject) + sizeof(BlitStuff)), (MEMTYPE_NORMAL | MEMTYPE_FILL | 0));
    if (bo == NULL)
    {
        return(BLITTER_ERR_NOMEM);
    }

    bo->bo_blitStuff = (void *)((uint32)bo + sizeof(BlitObject));
    DPRT(("BlitObject = 0x%lx. BlitStuff @ 0x%lx\n", bo, bo->bo_blitStuff));

    /* Walk through the taglist for snippets that the app is specifying for this BlitObject. Then,
     * allocate any snippets that were not specified with default snippets.
     */
    next = (TagArg *)tagList;
    while ((err == 0) && ((tag = NextTagArg(&next)) != NULL))
    {
        switch (tag->ta_Tag)
        {
            case (BLIT_TAG_DBLEND):
            {
                DBlendSnippet *dbl;
                if (bo->bo_dbl)
                {
                    err = BLITTER_ERR_DUPLICATETAG;
                }
                else
                {
                    apps |= SKIP_SNIPPET_DBLEND;
                    if (bo->bo_dbl = dbl = (DBlendSnippet *)tag->ta_Arg)
                    {
                        dbl->dbl_header.bsh_usageCount++;
                    }
                }
                break;
            }
            case (BLIT_TAG_TBLEND):
            {
                TxBlendSnippet *tbl;
                if (bo->bo_tbl)
                {
                    err = BLITTER_ERR_DUPLICATETAG;
                }
                else
                {
                    apps |= SKIP_SNIPPET_TBLEND;
                    if (bo->bo_tbl = tbl = (TxBlendSnippet *)tag->ta_Arg)
                    {
                        tbl->txb_header.bsh_usageCount++;
                    }
                }
                break;
            }
            case (BLIT_TAG_TXLOAD):
            {
                TxLoadSnippet *txl;
                if (bo->bo_txl)
                {
                    err = BLITTER_ERR_DUPLICATETAG;
                }
                else
                {
                    apps |= SKIP_SNIPPET_TXLOAD;
                    if (bo->bo_txl = txl = (TxLoadSnippet *)tag->ta_Arg)
                    {
                        txl->txl_header.bsh_usageCount++;
                    }
                }
                break;
            }
            case (BLIT_TAG_PIP):
            {
                PIPLoadSnippet *ppl;
                if (bo->bo_pip)
                {
                    err = BLITTER_ERR_DUPLICATETAG;
                }
                else
                {
                    apps |= SKIP_SNIPPET_PIP;
                    if (bo->bo_pip = ppl = (PIPLoadSnippet *)tag->ta_Arg)
                    {
                        ppl->ppl_header.bsh_usageCount++;
                    }
                }
                break;
            }
            case (BLIT_TAG_TXDATA):
            {
                BltTxData *btd;
                if (bo->bo_txdata)
                {
                    err = BLITTER_ERR_DUPLICATETAG;
                }
                else
                {
                    apps |= SKIP_SNIPPET_TXDATA;
                    if (bo->bo_txdata = btd = (BltTxData *)tag->ta_Arg)
                    {
                        btd->btd_header.bsh_usageCount++;
                    }
                }
                break;
            }
            case (BLIT_TAG_VERTICES):
            {
                VerticesSnippet *vtx;
                if (bo->bo_vertices)
                {
                    err = BLITTER_ERR_DUPLICATETAG;
                }
                else
                {
                    apps |= SKIP_SNIPPET_VERTICES;
                    if (bo->bo_vertices = vtx = (VerticesSnippet *)tag->ta_Arg)
                    {
                        vtx->vtx_header.bsh_usageCount++;
                        err = AssociateBOToVtx(bo, vtx);
                    }
                }
                break;
            }
            default:
            {
                err = BLITTER_ERR_BADTAGARG;
                break;
            }
        }
    }

    if (err == 0)
    {
        if ((bo->bo_pip == NULL) && (!(apps & SKIP_SNIPPET_PIP)))
        {
            bo->bo_pip = (PIPLoadSnippet *)Blt_CreateSnippet(BLIT_TAG_PIP);
            CHK_NULL(bo->bo_pip);
            bo->bo_pip->ppl_header.bsh_usageCount++;
            bo->bo_pip->ppl_header.bsh_flags |= CBO_SNIPPET;
        }
        if ((bo->bo_tbl == NULL) && (!(apps & SKIP_SNIPPET_TBLEND)))
        {
            bo->bo_tbl = (TxBlendSnippet *)Blt_CreateSnippet(BLIT_TAG_TBLEND);
            CHK_NULL(bo->bo_tbl);
            bo->bo_tbl->txb_header.bsh_usageCount++;
            bo->bo_tbl->txb_header.bsh_flags |= CBO_SNIPPET;
        }
        if ((bo->bo_txl == NULL) && (!(apps & SKIP_SNIPPET_TXLOAD)))
        {
            bo->bo_txl = (TxLoadSnippet *)Blt_CreateSnippet(BLIT_TAG_TXLOAD);
            CHK_NULL(bo->bo_txl);
            bo->bo_txl->txl_header.bsh_usageCount++;
            bo->bo_txl->txl_header.bsh_flags |= CBO_SNIPPET;
        }
        if ((bo->bo_dbl == NULL) && (!(apps & SKIP_SNIPPET_DBLEND)))
        {
            bo->bo_dbl = (DBlendSnippet *)Blt_CreateSnippet(BLIT_TAG_DBLEND);
            CHK_NULL(bo->bo_dbl);
            bo->bo_dbl->dbl_header.bsh_usageCount++;
            bo->bo_dbl->dbl_header.bsh_flags |= CBO_SNIPPET;
        }
        if ((bo->bo_txdata == NULL) && (!(apps & SKIP_SNIPPET_TXDATA)))
        {
            bo->bo_txdata = (BltTxData *)Blt_CreateSnippet(BLIT_TAG_TXDATA);
            CHK_NULL(bo->bo_txdata);
            bo->bo_txdata->btd_header.bsh_usageCount++;
            bo->bo_txdata->btd_header.bsh_flags |= CBO_SNIPPET;
        }
        if ((bo->bo_vertices == NULL) && (!(apps & SKIP_SNIPPET_VERTICES)))
        {
            bo->bo_vertices = (VerticesSnippet *)Blt_CreateSnippet(BLIT_TAG_VERTICES);
            CHK_NULL(bo->bo_vertices);
            bo->bo_vertices->vtx_header.bsh_usageCount++;
            bo->bo_vertices->vtx_header.bsh_flags |= CBO_SNIPPET;
            err = AssociateBOToVtx(bo, bo->bo_vertices);
        }

		if (err >= 0)
        {
            BLITSTUFF(bo)->bmI = CreateItemVA(MKNODEID(NST_GRAPHICS, GFX_BITMAP_NODE),
                                              BMTAG_WIDTH, 16,
                                              BMTAG_HEIGHT, 1,
                                              BMTAG_TYPE, BMTYPE_16,
                                              BMTAG_BUFFER, NULL,
                                              BMTAG_RENDERABLE, TRUE,
                                              TAG_END);
            err = (Err)BLITSTUFF(bo)->bmI;
        }
    }

  cleanup:
    if (err < 0)
    {
        /* Bummer. Clean up time */
        Blt_DeleteBlitObject(bo);
    }
    else
    {
        *_bo = bo;
    }

    return(err);
}

static void DecrementSnippetUsage(BlitterSnippetHeader *sh, uint32 size)
{
    if (sh)
    {
        sh->bsh_usageCount--;
        if ((sh->bsh_usageCount == 0) && (sh->bsh_flags & CBO_SNIPPET))
        {
            /* Count is down to 0, and we allocated this inside BLt_CreateBlitObject(),
             * so we should delete it now.
             */
            if ((sh->bsh_type == BLIT_TAG_TXDATA) && (sh->bsh_flags & SYSTEM_TEXEL))
            {
                    /* We should free up the texel data too */
                CltTxLOD *data = ((BltTxData *)sh)->btd_txData.texelData;
                    /* Restore the shadow pointer in case data->texelData
                     * was changed by Blt_SetTexture().
                     */
                data->texelData = TEXTURESLICE((BltTxData *)sh)->texelData;
                DPRT(("FreeMem() the texeldata at 0x%lx (size 0x%lx)\n", data->texelData, data->texelDataSize));
                FreeMem(data->texelData, data->texelDataSize);
            }
			if (sh->bsh_type == BLIT_TAG_VERTICES)
			{
				BOArray *next, *boa;
				/* Need to FreeMem() the BOArrays. */
				next = BOARRAY((VerticesSnippet *)sh);
				while (next)
				{
					boa = next;
					next = boa->next;
					FreeMem(boa, sizeof(BOArray));
				}
			}

            DPRT(("FreeMem() on snippet at 0x%lx\n", sh));
            FreeMem(sh, size);
        }
   }
}

extern uint32 snippetSizes[];
void Blt_DeleteBlitObject(const BlitObject *bo)
{
    VerticesSnippet *vtx;

    DPRT(("Blt_DeleteBlitObject(0x%lx)\n", bo));

    if (bo == NULL)
    {
        return;
    }

    DeleteItem(BLITSTUFF(bo)->bmI);
    RemoveSlices(bo);
    RemoveClippedVertices(bo);
    if (bo->bo_pip)
    {
        DecrementSnippetUsage(&bo->bo_pip->ppl_header, snippetSizes[BLIT_TAG_PIP - BLIT_TAG_DBLEND]);
    }
    if (bo->bo_tbl)
    {
        DecrementSnippetUsage(&bo->bo_tbl->txb_header, snippetSizes[BLIT_TAG_TBLEND - BLIT_TAG_DBLEND]);
    }
    if (bo->bo_txl)
    {
        DecrementSnippetUsage(&bo->bo_txl->txl_header, snippetSizes[BLIT_TAG_TXLOAD - BLIT_TAG_DBLEND]);
    }
    if (bo->bo_dbl)
    {
        DecrementSnippetUsage(&bo->bo_dbl->dbl_header, snippetSizes[0]);
    }
    if (bo->bo_txdata)
    {
        DecrementSnippetUsage(&bo->bo_txdata->btd_header, snippetSizes[BLIT_TAG_TXDATA - BLIT_TAG_DBLEND]);
    }
    vtx = bo->bo_vertices;
    if (vtx)
    {
        RemoveBOToVtxAssociation(bo, vtx);
        DecrementSnippetUsage(&vtx->vtx_header,
                              (sizeof(VerticesSnippet) + ((vtx->vtx_vertices * vtx->vtx_wordsPerVertex * sizeof(gfloat)) - sizeof(gfloat))));
            /* 4 bytes per word, -sizeof(gfloat) for the inclusion of vtx_vertex[0]. */
    }

    FreeMem(bo, (sizeof(BlitObject) + sizeof(BlitStuff)));
    return;
}
