/* @(#) vertices.c 96/10/15 1.10
 *
 *  Code to generate and handle vertices
 */

#include <kernel/types.h>
#include <kernel/mem.h>
#include <graphics/clt/cltmacros.h>
#include <graphics/blitter.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "blitter_internal.h"

#define DPRT(x) /* printf x;*/
void RemoveSlices(BlitObject *bo);
void RemoveBOToVtxAssociation(BlitObject *bo, VerticesSnippet *vtx);
static void ParseSlicedVertices(BOArray *boa, uint32 count, uint32 wordsPer, void (*callback)(gfloat *, void *), void *params);

Err Blt_CreateVertices(VerticesSnippet **vtx, uint32 triangles)
{
    VerticesSnippet *v;
    uint32 count;
    uint32 wordsPerVtx;
    bool shad;
    bool text;
    bool prsp;

    /* The triangles parameter should be the result of a CLA_TRIANGLE macro */
    count = ((triangles & FV_TRIANGLE_COUNT_MASK) + 1);
    shad = ((triangles & FV_TRIANGLE_SHADING_MASK) ? TRUE : FALSE);
    text = ((triangles & FV_TRIANGLE_TEXTURE_MASK) ? TRUE : FALSE);
    prsp = ((triangles & FV_TRIANGLE_PERSPECTIVE_MASK) ? TRUE : FALSE);
    wordsPerVtx = (2 + /* x, y*/
                   (shad ? 4 : 0) + /* r, g, b, a */
                   (text ? 2 : 0) +  /* u, v */
                   (prsp ? 1 : 0));  /* 1/w */

    v = (VerticesSnippet *)AllocMem((sizeof(VerticesSnippet) + (wordsPerVtx * count * sizeof(gfloat)) - sizeof(gfloat)),
                                 (MEMTYPE_NORMAL | MEMTYPE_FILL | 0));
        /* the -sizeof(gfloat) is for the vtx_vertex[0] which is included in the calculations */
    if (v == NULL)
    {
        return(BLITTER_ERR_NOMEM);
    }

    v->vtx_header.bsh_usageCount = 0;
    v->vtx_header.bsh_flags = SYSTEM_SNIPPET;
    v->vtx_header.bsh_type = BLIT_TAG_VERTICES;
    v->vtx_vertices = count;
    v->vtx_wordsPerVertex = wordsPerVtx;
    v->vtx_uOffset =
        (text ? (2 + /*x, y */
                 (shad ? 4 : 0) + /* r, g, b, a */
                 (prsp ? 1 : 0)) :  /* 1/w */
         0);
    v->vtx_rOffset = (shad ? 2 : 0); /* just x, y */
    v->vtx_wOffset =
        (prsp ? (2 + /*x, y */
                 (shad ? 4 : 0)) : /* r, g, b, a */
         0);
    v->vtx_instruction = triangles;
    /* The vertex list is set to all 0s */

    *vtx = v;
    return(0);
}

Err Blt_SetVertices(VerticesSnippet *vtx, const gfloat *vertices)
{
    BOArray *boa;

    if (vtx == NULL)
    {
        return(BLITTER_ERR_BADPTR);
    }

        /* the vertex parameters (x, y, u, v) etc for each vertex are contiguous. Makes
         * it kinda easy, huh?
         */
    memcpy(&vtx->vtx_vertex[0], vertices, (vtx->vtx_vertices * vtx->vtx_wordsPerVertex * sizeof(gfloat)));

        /* Invalidate all the sliced and clipped vertex information of any BlitObject that 
         * is associated with this VerticesSnippet.
         */
    boa = BOARRAY(vtx);
    while (boa)
    {
        uint32 i;
        BlitObject *bo;

        for (i = 0; i < BO_ARRAY_SIZE; i++)
        {
            if (bo = boa->bo[i])
            {
                RemoveSlices(bo);
                BLITSTUFF(bo)->flags &= ~(INVALIDATE_CLIP);
            }
        }
        boa = boa->next;
    }

    vtx->vtx_header.bsh_flags &= ~PERFECT_RECT;
    
    return(0);
}

static void doMoveVertices(gfloat *v, void *params)
{
    gfloat *gParams = (gfloat *)params;

    *v += gParams[0];           /* dx */
    *(v + 1) += gParams[1];   /* dy */

    return;
}

Err Blt_MoveVertices(VerticesSnippet *vtx, gfloat dx, gfloat dy)
{
    uint32 i;
    uint32 count;
    uint32 wordsPer;
    gfloat *v;
    gfloat params[2];
    BOArray *boa;

    if (vtx == NULL)
    {
        return(BLITTER_ERR_BADPTR);
    }

        /* Walk through the array of vertices, and modify all the (x, y) values */
    count = vtx->vtx_vertices;
    wordsPer = vtx->vtx_wordsPerVertex;
    v = &vtx->vtx_vertex[0];
    for (i = 0; i < count; i++)
    {
        *v += dx;
        *(v + 1) += dy;
        v += wordsPer;
    }

    /* Modify all the vertices of the sliced vertices associated with this VerticesSnippet */
    boa = BOARRAY(vtx);
    params[0] = dx;
    params[1] = dy;
    ParseSlicedVertices(boa, count, wordsPer, doMoveVertices, (void *)params);

    return(0);
}

static void doRotateVertices(gfloat *v, void *params)
{
    gfloat *gParams = (gfloat *)params;
    gfloat s,c;    /* sine and cosine */
    gfloat x, y;
    gfloat currentx, currenty;
    gfloat newx, newy;

    s = gParams[0];
    c= gParams[1];
    x = gParams[2];
    y = gParams[3];

    currentx = *v;
    currenty = *(v + 1);
    currentx -= x;
    currenty -= y;
    newx = ((currentx * c) + (currenty * s));
    newy = ((currenty * c) - (currentx * s));
    newx += x;
    newy += y;
    *v = newx;
    *(v + 1) = newy;

    return;
}


Err Blt_RotateVertices(VerticesSnippet *vtx, gfloat angle, gfloat x, gfloat y)
{
    uint32 count;
    uint32 wordsPer;
    BOArray *boa;
    gfloat *v;
    gfloat s, c;    /* sine and cosine */
    gfloat currentx, currenty;
    gfloat newx, newy;
    gfloat params[4];
    uint32 i;

    if (vtx == NULL)
    {
        return(BLITTER_ERR_BADPTR);
    }

    s = sinf(angle * (PI / 180.0));
    c = cosf(angle * (PI / 180.0));

        /* Walk through the array of vertices, and modify all the (x, y) values */
    count = vtx->vtx_vertices;
    wordsPer = vtx->vtx_wordsPerVertex;
    v = &vtx->vtx_vertex[0];
    for (i = 0; i < count; i++)
    {
        currentx = *v;
        currenty = *(v + 1);
        currentx -= x;
        currenty -= y;
        newx = ((currentx * c) + (currenty * s));
        newy = ((currenty * c) - (currentx * s));
        newx += x;
        newy += y;
        *v = newx;
        *(v + 1) = newy;
        v += wordsPer;
    }

        /* Modify all the vertices of the sliced vertices associated with this VerticesSnippet */
    boa = BOARRAY(vtx);
    params[0] = s;
    params[1] = c;
    params[2] = x;
    params[3] = y;
    ParseSlicedVertices(boa, count, wordsPer, doRotateVertices, (void *)params);

    return(0);
}

static void doTransformVertices(gfloat *v, BlitMatrix bm)
{
    gfloat x, y;

    x = *v;
    y = *(v + 1);

    *v = ((bm[0][0] * x) + (bm[1][0] * y) + bm[2][0]);
    *(v + 1) = ((bm[0][1] * x) + (bm[1][1] * y) + bm[2][1]);

    return;
}

Err Blt_TransformVertices(VerticesSnippet *vtx, BlitMatrix bm)
{
    uint32 count;
    uint32 wordsPer;
    BOArray *boa;
    gfloat *v;
    gfloat *param;
    uint32 i;

    if (vtx == NULL)
    {
        return(BLITTER_ERR_BADPTR);
    }
        /* Walk through the array of vertices, and modify all the (x, y) values */
    count = vtx->vtx_vertices;
    wordsPer = vtx->vtx_wordsPerVertex;
    v = &vtx->vtx_vertex[0];
    for (i = 0; i < count; i++)
    {
        doTransformVertices(v, bm);
        v += wordsPer;
    }

    /* Modify all the vertices of the sliced vertices associated with this VerticesSnippet */
    boa = BOARRAY(vtx);
    param = &bm[0][0];
    ParseSlicedVertices(boa, count, wordsPer,  (void (*)(gfloat *, void *))doTransformVertices, (void *)param);

    return(0);
}


#define NEXT_BBOX(b) {uint32 foo; foo = (uint32)(b); foo += sizeof(BoundingBox); (b) = (BoundingBox *)foo;}
static void CalcClipRegion(BoundingBox *bbox, Point2 *pt);

static void ParseSlicedVertices(BOArray *boa, uint32 count, uint32 wordsPer,
                                void (*callback)(gfloat *, void *), void *params)
{
    uint32 i, j, k;
    uint32 instruction;
    BlitObject *bo;
    BlitStuff *bs;
    TextureSlice *ts;
    BoundingBox *bbox;
    uint32 totalStrips;

    gfloat *v;

    while (boa)
    {
        for (j = 0; j < BO_ARRAY_SIZE; j++)
        {
            if (bo = boa->bo[j])
            {
                bs = BLITSTUFF(bo);
                ts = TEXTURESLICE(bo->bo_txdata);
                v = bs->slicedVertices;
                totalStrips = 0;
                /* (Take this opportunity to invalidate the clipped vertices.
                 * All the Vertex-calculation code eventually comes through here.)
                 */
                bs->flags &= ~(INVALIDATE_CLIP);

                bbox = ts->bbox;
                for (k = 0; v && k < bs->ySlicesUsed; k++)
                {
                    if (ts->txlOrig)
                    {
                        /* Jump over the TxLoad stuff */
                        v += 2; /* Sync instrution*/
                        v += ((sizeof(TxLoadSnippet) - sizeof(BlitterSnippetHeader)) / sizeof(gfloat *));
                    }
                    instruction = *((uint32 *)v);
                    while (((instruction & 0xf0000000) == RC_WRITE_VERTEX) &&
                           (totalStrips < bs->totalStrips))
                    {
                        v++;
                        totalStrips++;
                        count = ((instruction & FV_TRIANGLE_COUNT_MASK) + 1);
                        for (i = 0; i < count; i++)
                        {
                            callback(v, params);  /* Perform the operation */
                            v += wordsPer;
                        }
                        instruction = *((uint32 *)v);
                    }

                        /* Perform the operation on the bounding box. */
                    bbox->leftMost = bbox->topMost = 100000;
                    bbox->rightMost = bbox->bottomMost = 0;
                    callback(&bbox->topLeft.x, params);
                    CalcClipRegion(bbox, &bbox->topLeft);
                    callback(&bbox->topRight.x, params);
                    CalcClipRegion(bbox, &bbox->topRight);
                    callback(&bbox->bottomLeft.x, params);
                    CalcClipRegion(bbox, &bbox->bottomLeft);
                    callback(&bbox->bottomRight.x, params);
                    CalcClipRegion(bbox, &bbox->bottomRight);
                    NEXT_BBOX(bbox);
                }
            }
        }
        boa = boa->next;
    }
}

static void CalcClipRegion(BoundingBox *bbox, Point2 *pt)
{
    if (pt->x < bbox->leftMost)
    {
        bbox->leftMost = pt->x;
    }
    if (pt->x > bbox->rightMost)
    {
        bbox->rightMost = pt->x;
    }
    if (pt->y < bbox->topMost)
    {
        bbox->topMost = pt->y;
    }
    if (pt->y > bbox->bottomMost)
    {
        bbox->bottomMost = pt->y;
    }
}

Err Blt_MoveUV(VerticesSnippet *vtx, gfloat du, gfloat dv)
{
    uint32 i, j, k;
    uint32 count;
    uint32 wordsPer;
    uint32 toU;
    gfloat *v;
    
    BOArray *boa;
    uint32 instruction;
    BlitObject *bo;
    BlitStuff *bs;
    TextureSlice *ts;
    uint32 totalStrips;
    
    if (vtx == NULL)
    {
        return(BLITTER_ERR_BADPTR);
    }
    toU = vtx->vtx_uOffset;
    if (toU == 0)
    {
        return(BLITTER_ERR_BADPTR);
    }
    
        /* Walk through the array of vertices, and modify all the (x, y) values */
    count = vtx->vtx_vertices;
    wordsPer = vtx->vtx_wordsPerVertex;
    v = &vtx->vtx_vertex[0];
    for (i = 0; i < count; i++)
    {
        *(v + toU) += du;
        *(v + toU + 1) += dv;
        v += wordsPer;
    }

    /* Modify all the vertices of the sliced vertices associated with this VerticesSnippet.
     * If dv is not 0, then the txl_src of each slice should be modified, not the v values.
     *
     * Don't use ParseSlicedVertices() because this doesn't change txl_src, nor do we
     * want to change the ClipBox of each slice or invalidate the clipped vertices.
     */
    
    boa = BOARRAY(vtx);
    while (boa)
    {
        for (j = 0; j < BO_ARRAY_SIZE; j++)
        {
            if (bo = boa->bo[j])
            {
                uint32 diff = 0;
                
                bs = BLITSTUFF(bo);
                ts = TEXTURESLICE(bo->bo_txdata);
                v = bs->slicedVertices;
                totalStrips = 0;

                if (((uint32)dv != 0) && (ts->txlOrig != NULL))
                {
                    /* Calculate the diff to add to the txl_src of each slice */
                    if (bo->bo_txdata)
                    {
                        diff = ((uint32)dv *
                                (bo->bo_txdata->btd_txData.minX * (bo->bo_txdata->btd_txData.bitsPerPixel / 8)));
                    }
                }
                
                for (k = 0; v && k < bs->ySlicesUsed; k++)
                {
                    if (ts->txlOrig)
                    {
                        char **txtsrc;
                        
                            /* This should be the TxLoad stuff */
                        v += 2; /* Sync instrution*/
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
                        for (i = 0; i < count; i++)
                        {
                            *(v + toU) += du;
                            v += wordsPer;
                        }
                        instruction = *((uint32 *)v);
                    }
                }
            }
        }
        boa = boa->next;
    }
    
    return(0);
}
