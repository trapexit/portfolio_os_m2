/* @(#) slice.c 96/10/18 1.15
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

void RemoveSlices(BlitObject *bo);

#define DPRT(x) /* printf x; */
#define DPRTSV(x) /* printf x; */
#define DPRTSR(x) /* printf x; */
#define DPRTBO(x) /* printf x; */

static gfloat *CopyVertex(gfloat *dst, VerticesSnippet *vtx, gfloat v[], uint32 *count, BoundingBox *bbox)
{
    uint32 size = (vtx->vtx_wordsPerVertex * sizeof(gfloat));
    gfloat *ret;
    
    DPRT(("CopyVertex() (%g, %g) {%g, %g}  to 0x%lx\n", v[0], v[1], v[vtx->vtx_uOffset], v[vtx->vtx_uOffset + 1], dst));
    /* Don`t copy this vertex if it is the same as the last one in the list */
    if ((*count == 0) ||
        ((*(dst - vtx->vtx_wordsPerVertex) != v[0]) || (*(dst - vtx->vtx_wordsPerVertex + 1) != v[1])))
    {
        memcpy(dst, v, size);
        *count += 1;
        ret = (dst + vtx->vtx_wordsPerVertex);

            /* Change the dimensions of the bounding box? */
        if (v[0] < bbox->leftMost)
        {
            bbox->leftMost = v[0];
        }
        if (v[0] > bbox->rightMost)
        {
            bbox->rightMost = v[0];
        }
        if (v[1] < bbox->topMost)
        {
            bbox->topMost = v[1];
        }
        if (v[1] > bbox->bottomMost)
        {
            bbox->bottomMost = v[1];
        }
    }
    else
    {
        ret = dst;
        DPRT(("REJECTED\n"));
    }
    
    return(ret);
}

/*
 * SliceRectangle() special cases the slicling of a rectangle whose edges are
 * parallel to the bitmap edges.
 */
static Err SliceRectangle(BlitObject *bo, VerticesSnippet *vtx, TextureSlice *ts,
                          bool replaceMedium, uint32 wordsPer, gfloat **last)
{
    gfloat *te;
    gfloat range;
    gfloat linesPerSlice;
    gfloat linesLastSlice;
    gfloat lastSliceHeight;
    uint32 linesPerSliceI;
    gfloat height;
    gfloat v;
    Point2 tl, br;
    BoundingBox *bbox;
    gfloat gSlices;
    uint32 slices;
    uint32 toR, toW;
    uint32 i;
    uint32 startingSlice;
    uint32 opposite;
    
    toR = vtx->vtx_rOffset;
    toW = vtx->vtx_wOffset;

    opposite = (replaceMedium ? 2 : 3);
    
    tl.x = *BLITVERTEX_X(vtx, 0);
    tl.y = *BLITVERTEX_Y(vtx, 0);
    br.x = *BLITVERTEX_X(vtx, opposite);
    br.y = *BLITVERTEX_Y(vtx, opposite);

    bbox = ts->bbox;
    
        /* Although the vertices are perfectly rectangular, we did not specify
         * that the texture should cover the entire area. So, just how many
         * lines of texture are needed?
         */
    range = (*BLITVERTEX_V(vtx, 3) - *BLITVERTEX_V(vtx, 0));
        /* How many slices is that? */
    gSlices = (range / ts->ySliceHeight);
    slices = (uint32)gSlices;
        /* How many pixel lines per slice? */
    height = (*BLITVERTEX_Y(vtx, 3) - *BLITVERTEX_Y(vtx, 0));
    linesPerSliceI = (uint32)(height / gSlices);
    linesPerSlice = (gfloat)linesPerSliceI;
        /* How many pixel lines in the last slice? */
    linesLastSlice = (height - (slices * linesPerSlice));
        /* How many texture lines in the last slice?*/
    lastSliceHeight = (range - (slices * ts->ySliceHeight));
    DPRTSR(("Rectangle: %ld whole slices of %ld lines each over %g tlines (%g plines)\n", slices, ts->ySliceHeight, range, height));
    DPRTSR(("%g lines per slice (+ %g lines in the last slice)\n", linesPerSlice, linesLastSlice));
    DPRTSR(("Last slice has %g lines of texture\n", lastSliceHeight));
    DPRTSR(("BBox = (%g, %g) - (%g, %g)\n", tl.x, tl.y, br.x, br.y));
    if (linesLastSlice != 0.0)
    {
        slices++;
    }
    else
    {
        linesLastSlice = linesPerSlice;
        lastSliceHeight = ts->ySliceHeight;
    }
    
        /* Allocate RAM to store the sliced vertices. 4 vertices, SYNC and
         * TxLoad instructions for every slice.
         */
    BLITSTUFF(bo)->slicedVerticesSize =
        (slices *
         ((4 * wordsPer * sizeof(gfloat)) +
          sizeof(uint32) +      /* Vertex instruction */
          (2 * sizeof(uint32)) + /* SYNC */
          (sizeof(TxLoadSnippet) - sizeof(BlitterSnippetHeader))));
    BLITSTUFF(bo)->slicedVertices = (gfloat *)AllocMem(BLITSTUFF(bo)->slicedVerticesSize, MEMTYPE_NORMAL);
    if (BLITSTUFF(bo)->slicedVertices == NULL)
    {
        return(BLITTER_ERR_NOMEM);
    }
    DPRTSR(("Rectangle: Sliced vertices @ 0x%lx\n", BLITSTUFF(bo)->slicedVertices));
    te = BLITSTUFF(bo)->slicedVertices;

        /* We don't necessarily need slice 0. Remember, we don't need V == 0 at the top. */
    startingSlice = (*BLITVERTEX_V(vtx, 0) / ts->ySliceHeight);
    for (i = startingSlice; i < slices; i++)
    {
        TxLoadSnippet *txl;
        CmdListP foo;
        uint32 size = (sizeof(TxLoadSnippet) - sizeof(BlitterSnippetHeader));
        gfloat y;

        bbox->instructions = (uint32 *)te;

        foo = (CmdListP)te;
        CLT_Sync(&foo);
        te = (gfloat *)foo;
        txl = (TxLoadSnippet *)te;
        memcpy(txl, &ts->txlOrig->txl_instruction1, size);
        txl = (TxLoadSnippet *)((uint32)txl - sizeof(BlitterSnippetHeader));
        txl->txl_src = (ts->txlOrig->txl_src + (((i * ts->ySliceHeight) * ts->bitsPerRow) / 8));
        txl->txl_count = ((i < (slices - 1)) ? ts->ySliceHeight : lastSliceHeight);
            /* There may not be an exact number of bytes in a row. The txl_cntl.SrcBitOff field
             * contains the bit offset into the next byte at the start of the slice.
             */
        txl->txl_cntl &= ~FV_TXTLDCNTL_SRCBITOFF_MASK;
        txl->txl_cntl |= ((((i * ts->ySliceHeight) * ts->bitsPerRow) & 7)
                          << FV_TXTLDCNTL_SRCBITOFF_SHIFT);
        txl->txl_uvMax &= FV_TXTUVMAX_UMAX_MASK;
        txl->txl_uvMax |= (txl->txl_count - 1);
        te += (size / sizeof(uint32));
        BLITSTUFF(bo)->ySlicesUsed++;

        foo = (CmdListP)te;
        CLT_TRIANGLE(&foo, 1, 1,
                     ((vtx->vtx_instruction & FV_TRIANGLE_PERSPECTIVE_MASK) >> FV_TRIANGLE_PERSPECTIVE_SHIFT),
                     1,
                     ((vtx->vtx_instruction & FV_TRIANGLE_SHADING_MASK) >> FV_TRIANGLE_SHADING_SHIFT),
                     4);
        te = (gfloat *)foo;
        BLITSTUFF(bo)->totalStrips++;

            /* top left corner of the slice: */
        *te++ = tl.x;
        *te++ = y = (tl.y + (i * linesPerSlice));
        DPRTSR(("%g -> ",y));
        if (toR)
        {
            *te++ = (*BLITVERTEX_R(vtx, 0) + (((*BLITVERTEX_R(vtx, (opposite ^ 1)) - *BLITVERTEX_R(vtx, 0)) * linesPerSlice) / height));
            *te++ = (*BLITVERTEX_G(vtx, 0) + (((*BLITVERTEX_G(vtx, (opposite ^ 1)) - *BLITVERTEX_G(vtx, 0)) * linesPerSlice) / height));
            *te++ = (*BLITVERTEX_B(vtx, 0) + (((*BLITVERTEX_B(vtx, (opposite ^ 1)) - *BLITVERTEX_B(vtx, 0)) * linesPerSlice) / height));
            *te++ = (*BLITVERTEX_A(vtx, 0) + (((*BLITVERTEX_A(vtx, (opposite ^ 1)) - *BLITVERTEX_A(vtx, 0)) * linesPerSlice) / height));
        }
        if (toW)
        {
            *te++ = (*BLITVERTEX_W(vtx, 0) + (((*BLITVERTEX_W(vtx, (opposite ^ 1)) - *BLITVERTEX_W(vtx, 0)) * linesPerSlice) / height));
        }
        *te++ = *BLITVERTEX_U(vtx, 0);
        *te++ = v = ((i == startingSlice) ? (*BLITVERTEX_V(vtx, 0) - (i * linesPerSlice)) : 0);
        bbox->leftMost = bbox->topLeft.x = tl.x;
        bbox->topMost = bbox->topLeft.y = y;
            /* top right corner */
        *te++ = br.x;
        *te++ = y;
        if (toR)
        {
            *te++ = (*BLITVERTEX_R(vtx, 1) + (((*BLITVERTEX_R(vtx, opposite) - *BLITVERTEX_R(vtx, 1)) * linesPerSlice) / height));
            *te++ = (*BLITVERTEX_G(vtx, 1) + (((*BLITVERTEX_G(vtx, opposite) - *BLITVERTEX_G(vtx, 1)) * linesPerSlice) / height));
            *te++ = (*BLITVERTEX_B(vtx, 1) + (((*BLITVERTEX_B(vtx, opposite) - *BLITVERTEX_B(vtx, 1)) * linesPerSlice) / height));
            *te++ = (*BLITVERTEX_A(vtx, 1) + (((*BLITVERTEX_A(vtx, opposite) - *BLITVERTEX_A(vtx, 1)) * linesPerSlice) / height));
        }
        if (toW)
        {
            *te++ = (*BLITVERTEX_W(vtx, 1) + (((*BLITVERTEX_W(vtx, opposite) - *BLITVERTEX_W(vtx, 1)) * linesPerSlice) / height));
        }
        *te++ = *BLITVERTEX_U(vtx, 1);
        *te++ = v;
        bbox->topRight.x = br.x;
        bbox->topRight.y = y;
            /* bottom right corner */
        *te++ = br.x;
        *te++ = y = (y + ((i < (slices - 1)) ? linesPerSlice : linesLastSlice));
        DPRTSR(("%g\n", y));
        if (toR)
        {
            *te++ = (*BLITVERTEX_R(vtx, 1) + (((*BLITVERTEX_R(vtx, opposite) - *BLITVERTEX_R(vtx, 1)) * (linesPerSlice * 2)) / height));
            *te++ = (*BLITVERTEX_G(vtx, 1) + (((*BLITVERTEX_G(vtx, opposite) - *BLITVERTEX_G(vtx, 1)) * (linesPerSlice * 2)) / height));
            *te++ = (*BLITVERTEX_B(vtx, 1) + (((*BLITVERTEX_B(vtx, opposite) - *BLITVERTEX_B(vtx, 1)) * (linesPerSlice * 2)) / height));
            *te++ = (*BLITVERTEX_A(vtx, 1) + (((*BLITVERTEX_A(vtx, opposite) - *BLITVERTEX_A(vtx, 1)) * (linesPerSlice * 2)) / height));
        }
        if (toW)
        {
            *te++ = (*BLITVERTEX_W(vtx, 1) + (((*BLITVERTEX_W(vtx, opposite) - *BLITVERTEX_W(vtx, 1)) * (linesPerSlice * 2)) / height));
        }
        *te++ = *BLITVERTEX_U(vtx, 1);
        *te++ = v = (v + ((i < (slices - 1)) ? ts->ySliceHeight : linesLastSlice));
        bbox->rightMost = bbox->bottomRight.x = br.x;
        bbox->bottomMost = bbox->bottomRight.y = y;
            /* bottom left corner */
        *te++ = tl.x;
        *te++ = y;
        if (toR)
        {
            *te++ = (*BLITVERTEX_R(vtx, 0) + (((*BLITVERTEX_R(vtx, (opposite ^ 1)) - *BLITVERTEX_R(vtx, 0)) * (linesPerSlice * 2)) / height));
            *te++ = (*BLITVERTEX_G(vtx, 0) + (((*BLITVERTEX_G(vtx, (opposite ^ 1)) - *BLITVERTEX_G(vtx, 0)) * (linesPerSlice * 2)) / height));
            *te++ = (*BLITVERTEX_B(vtx, 0) + (((*BLITVERTEX_B(vtx, (opposite ^ 1)) - *BLITVERTEX_B(vtx, 0)) * (linesPerSlice * 2)) / height));
            *te++ = (*BLITVERTEX_A(vtx, 0) + (((*BLITVERTEX_A(vtx, (opposite ^ 1)) - *BLITVERTEX_A(vtx, 0)) * (linesPerSlice * 2)) / height));
        }
        if (toW)
        {
            *te++ = (*BLITVERTEX_W(vtx, 0) + (((*BLITVERTEX_W(vtx, (opposite ^ 1)) - *BLITVERTEX_W(vtx, 0)) * (linesPerSlice * 2)) / height));
        }
        *te++ = *BLITVERTEX_U(vtx, 0);
        *te++ = v;
        bbox->bottomLeft.x = tl.x;
        bbox->bottomLeft.y = y;

        bbox++;
    }
    *last = te;
    
    return(0);
}

static gfloat *SliceTriangle(gfloat *dst, BoundingBox *bbox, VerticesSnippet *vtx,
                      gfloat v0[], gfloat v1[], gfloat v2[], gfloat top, gfloat bottom, uint32 *count)
{
    gfloat *corners[4];
    gfloat *dstOrig = dst;
    gfloat ratio;
    gfloat newCorner[4][VTX_FLOATS];
    gfloat spare[2][VTX_FLOATS];
    uint32 size = (vtx->vtx_wordsPerVertex * sizeof(gfloat));
    uint32 toU, toV, toR, toW;
    uint32 i;
    uint32 high, low;
    bool upsideDown;

    toU = vtx->vtx_uOffset;
    toV = (toU + 1);
    toR = vtx->vtx_rOffset;
    toW = vtx->vtx_wOffset;
    
    DPRT(("\tSlice vertices (%g, %g) - (%g, %g) - (%g, %g) [%g : %g]\n",
          v0[0], v0[1], v1[0], v1[1], v2[0], v2[1], top, bottom));
    DPRT(("\t               {%g, %g} - {%g, %g} - {%g, %g}\n",
          v0[toU], v0[toV], v1[toU], v1[toV], v2[toU], v2[toV]));

    memcpy(newCorner[0], v0, size);
    memcpy(newCorner[3], v0, size);
    memcpy(newCorner[1], v1, size);
    memcpy(newCorner[2], v2, size);
    corners[0] = newCorner[0];
    corners[1] = newCorner[1];
    corners[2] = newCorner[2];
    corners[3] = newCorner[3];

    /* Calculate the endpoints of each of the lines in the triangle. */
    for (i = 0; i < 3; i++)
    {
        low = i;
        high = (i + 1);
        upsideDown = FALSE;
        if (corners[i + 1][toV] > corners[i][toV])
        {
            /* upside down */
            low = (i + 1);
            high = i;
            upsideDown = TRUE;
            DPRT((" >>> Upside Down <<<\n"));
        }

        if (((corners[high][toV] >= top) && (corners[high][toV] <= bottom)) &&
            ((corners[low][toV] >= top) && (corners[low][toV] <= bottom)))
        {
            /* Completely inside the region */
            corners[i][toV] -= top;
            dst = CopyVertex(dst, vtx, corners[i], count, bbox);
            corners[i][toV] += top;
            corners[i + 1][toV] -= top;
            dst = CopyVertex(dst, vtx, corners[i + 1], count, bbox);
            corners[i + 1][toV] += top;
        }
        else if ((corners[low][toV] < top) || (corners[high][toV] > bottom))
        {
            DPRT(("%d: Outside region\n", i));
        }
        else
        {
            if ((corners[low][toV] == corners[high][toV]) &&
                ((corners[low][toV] == top) || (corners[low][toV] == bottom)))
            {
                /* Vertices lie across the top or bottom, so we can copy these
                 * vertices, but maintain the same order.
                 */
                DPRT(("%d: Lies along the %g boundary\n", i, corners[low][toV]));
                corners[i][toV] -= top;
                corners[i + 1][toV] -= top;
                dst = CopyVertex(dst, vtx, corners[i], count, bbox);
                dst = CopyVertex(dst, vtx, corners[i + 1], count, bbox);
                corners[i][toV] += top;
                corners[i + 1][toV] += top;
            }
            else
            {
                /* Must cross one or both of the boundaries, so slice. The boundary that
                 * is crossed determines the ratio.
                 */
                if (corners[high][toV] < top)
                {
                    ratio = ((top - corners[high][toV]) / (corners[low][toV] - corners[high][toV]));
                    DPRT(("%d: Cross the top. ratio = %g\n", i, ratio));

                    spare[0][toV] = 0;
                    spare[0][0] = (corners[high][0] + ((corners[low][0] - corners[high][0]) * ratio));
                    spare[0][1] = (corners[high][1] + ((corners[low][1] - corners[high][1]) * ratio));
                    spare[0][toU] = (corners[high][toU] + ((corners[low][toU] - corners[high][toU]) * ratio));
                    if (toW)
                    {
                        spare[0][toW] = (corners[high][toW] + ((corners[low][toW] - corners[high][toW]) * ratio));
                    }
                    if (toR)
                    {
                        spare[0][toR] = (corners[high][toR] + ((corners[low][toR] - corners[high][toR]) * ratio));
                        spare[0][toR + 1] = (corners[high][toR + 1] + ((corners[low][toR + 1] - corners[high][toR + 1]) * ratio));
                        spare[0][toR + 2] = (corners[high][toR + 2] + ((corners[low][toR + 2] - corners[high][toR + 2]) * ratio));
                        spare[0][toR + 3] = (corners[high][toR + 3] + ((corners[low][toR + 3] - corners[high][toR + 3]) * ratio));
                    }                    
                }
                else
                {
                    memcpy(spare[0], corners[high], size);
                    spare[0][toV] -= top;
                }

                if (corners[low][toV] > bottom)
                {
                    ratio = ((bottom - corners[high][toV] + 1) / (corners[low][toV] - corners[high][toV]));
                    DPRT(("%d: Cross the bottom. ratio = %g\n", i, ratio));
                    /* Fudge factor: The +1 in the ratio calculation and the +1 for the V value
                     * is used to join he slices together without gaps between them.
                     */
                    spare[1][toV] = (bottom - top + 1);
                    spare[1][0] = (corners[high][0] + ((corners[low][0] - corners[high][0]) * ratio));
                    spare[1][1] = (corners[high][1] + ((corners[low][1] - corners[high][1]) * ratio));
                    spare[1][toU] = (corners[high][toU] + ((corners[low][toU] - corners[high][toU]) * ratio));
                    if (toW)
                    {
                        spare[1][toW] = (corners[high][toW] + ((corners[low][toW] - corners[high][toW]) * ratio));
                    }
                    if (toR)
                    {
                        spare[1][toR] = (corners[high][toR] + ((corners[low][toR] - corners[high][toR]) * ratio));
                        spare[1][toR + 1] = (corners[high][toR + 1] + ((corners[low][toR + 1] - corners[high][toR + 1]) * ratio));
                        spare[1][toR + 2] = (corners[high][toR + 2] + ((corners[low][toR + 2] - corners[high][toR + 2]) * ratio));
                        spare[1][toR + 3] = (corners[high][toR + 3] + ((corners[low][toR + 3] - corners[high][toR + 3]) * ratio));
                    }                    
                }
                else
                {
                    memcpy(spare[1], corners[low], size);
                    spare[1][toV] -= top;
                }
                    /* Must get the order right! */
                if (upsideDown)
                {
                    dst = CopyVertex(dst, vtx, spare[0], count, bbox);
                    dst = CopyVertex(dst, vtx, spare[1], count, bbox);
                }
                else
                {
                    dst = CopyVertex(dst, vtx, spare[1], count, bbox);
                    dst = CopyVertex(dst, vtx, spare[0], count, bbox);
                }
            }
        }
    }
    
    if ((dstOrig[0] == (dst - vtx->vtx_wordsPerVertex)[0]) &&
        (dstOrig[1] == (dst - vtx->vtx_wordsPerVertex)[1]))
    {
        DPRT(("Moved back\n"));
        dst -= vtx->vtx_wordsPerVertex;
        *count -= 1;
    }
    
    return(dst);
}

/*
 * I am defining the following rules for a perfect rectangle:
 * 1) Top left corner is vertex 0
 * 2) Top right corner is vertex 1
 * ( bottom right corner depends on replaceMedium).
 * 3) (x, y) values form a rectangle.
 * 4) (u. v) values form a rectangle, and are defined in the same order
 * as the (x, y) values.
 * 5) The difference in the y values of the rectangle are the same as the
 * difference in the v values.
 */
bool PerfectRectangle(VerticesSnippet *vtx, bool replaceMedium)
{
    uint32 i, j, k;
    Point2 points[4];
    Point2 textures[4];
    bool rect = FALSE;
    
    if (vtx->vtx_header.bsh_flags & PERFECT_RECT)
    {
        return(TRUE);
    }
    
    if ((vtx->vtx_instruction & FV_TRIANGLE_COUNT_MASK) == (4 - 1)) /* 4 vertices */
    {
        if (replaceMedium)
        {
            i = 2;                  /* opposite 0 */
            j = 3;                  /* opposite 1 */
        }
        else
        {
            i = 3;                  /* opposite 0 */
            j = 2;                  /* opposite 1 */
        }

        for (k = 0; k < 4; k++)
        {
            points[k].x = *BLITVERTEX_X(vtx, k);
            points[k].y = *BLITVERTEX_Y(vtx, k);
            textures[k].x = *BLITVERTEX_U(vtx, k);
            textures[k].y = *BLITVERTEX_V(vtx, k);
        }
            /* Do the X values line up? */
        if ((points[0].x == points[j].x) && (points[1].x == points[i].x) && /* they line up */
            (points[0].x < points[1].x)) /* and they are the right order */
        {
            /* Do the U values line up? */
            if ((textures[0].x == textures[j].x) && (textures[1].x == textures[i].x) && /* they line up */
                (textures[0].x < textures[1].x)) /* and they are the right order */
            {
                /* Do the Y values line up? */
                 if ((points[0].y == points[1].y) && (points[2].y == points[3].y) &&
                     (points[0].y < points[3].y))
                 {
                     /* Do the V values line up? */
                     if ((textures[0].y == textures[1].y) && (textures[2].y == textures[3].y) &&
                         (textures[0].y < textures[3].y))
                     {
                         /* Is the height of the rectangle the same as the height
                          * of the texture?
                          */
                         if ((textures[2].y - textures[0].y) == (points[2].y - points[0].y))
                         {
                                 /* We have ourselves a winner! */
                             rect = TRUE;
                             vtx->vtx_header.bsh_flags |= PERFECT_RECT;
                         }
                     }
                 }
            }
        }
    }
    DPRTSR(("This is %sa perfect rectangle\n", rect ? "" : "not "));
    return(rect);
}

/*
 * SliceVertices() takes the list of vertices set up by the application and
 * slices them into horizontal strip. Each strip fits inside the 16K TRAM
 * buffer.
 *
 * For each strip, walk through the list of vertices and determine if any
 * triangles cross the strip. If so, then they need to be sliced and added
 * to the new list of vertices.
 *
 * To prevent gaps between slices, the bottom line of the strip overlaps
 * the top line of the next strip.
 */
#define MOST_VERTICES(v) ((((v) - 2) * 3) + 2)

Err SliceVertices(BlitObject *bo)
{
    TextureSlice *ts;
    VerticesSnippet *vtx;
    uint32 height, width;
    uint32 i, j;
    uint32 wordsPer;
    uint32 toU, toV;
    uint32 vertices;
    gfloat v0[VTX_FLOATS];
    gfloat v1[VTX_FLOATS];
    gfloat v2[VTX_FLOATS];
    gfloat *tri;
    gfloat *new;
    gfloat high, low;
    uint32 newCount;
    uint32 *newInstruction;
    BoundingBox *bbox;
    Err err = 0;
    bool replaceMedium;
    
    vtx = bo->bo_vertices;
    if (!(vtx->vtx_instruction & FV_TRIANGLE_TEXTURE_MASK))
    {
        return(0);
    }

    height = bo->bo_txdata->btd_txData.minY;
    width = bo->bo_txdata->btd_txData.minX;
    DPRTSV(("Slicing vertices for BlitObject 0x%lx. height = %ld, width = %ld\n", bo, height, width));

        /* Calculate the number of slices needed */
    ts = TEXTURESLICE(bo->bo_txdata);
    ts->bitsPerRow = (width * bo->bo_txdata->btd_txData.bitsPerPixel);
    ts->ySliceHeight = ((TRAM_SIZE * 8) / ts->bitsPerRow);
    ts->ySlice = ((height + (ts->ySliceHeight - 2)) / (ts->ySliceHeight - 1));
    ts->ySliceLastHeight = (height - ((ts->ySlice - 1) * (ts->ySliceHeight - 1)));
    ts->txlOrig = bo->bo_txl;
    DPRTSV(("slices = %ld, sliceHeight = %ld, lastSliceHeight = %ld, bitsPerRow = %ld\n", ts->ySlice, ts->ySliceHeight, ts->ySliceLastHeight, ts->bitsPerRow));
        /* Allocate RAM to track the bounds of each slice. */
    ts->bbox = (BoundingBox *)AllocMem((ts->ySlice * sizeof(BoundingBox)), MEMTYPE_NORMAL);
    if (ts->bbox == NULL)
    {
        return(BLITTER_ERR_NOMEM);
    }
    BLITSTUFF(bo)->ySlicesUsed = 0;
    BLITSTUFF(bo)->totalStrips = 0;

    replaceMedium = ((vtx->vtx_instruction & FV_TRIANGLE_STRIPFAN_MASK) ? TRUE : FALSE);
    wordsPer = vtx->vtx_wordsPerVertex;

        /* If these vertices form a perfect rectangle, then special case it. We want to do this for a
         * few reasons:
         *
         * 1) Optimised common case.
         * 2) Going through the regular slicer will create some intermediary vertices which
         * can causing floating point errors in the edge walker thru lack of fp precision.
         * The special case should greatly reduce the chances of this happening.
         */
    if (PerfectRectangle(vtx, replaceMedium))
    {
        err = SliceRectangle(bo, vtx, ts, replaceMedium, wordsPer, &new);
    }
    else
    {
            /* Allocate RAM for to store the maximum number of vertices we need to slice the
             * texture into, which in the worst case is two triangles (4 vertices) per triangle
             * per slice.
             *
             * We also store a copy of the TxLoad instructions for every slice, with a
             * Sync per txLoad.
             */
        vtx = bo->bo_vertices;
        vertices = ((vtx->vtx_instruction & FV_TRIANGLE_COUNT_MASK) + 1);
        BLITSTUFF(bo)->slicedVerticesSize =
            ((MOST_VERTICES(vertices) *  /* vertices-2 = # of triangles, *3 worst case per slice */
              (vtx->vtx_wordsPerVertex * sizeof(gfloat) * ts->ySlice)) +
             ((((sizeof(uint32) * 2) + sizeof(TxLoadSnippet)) + /* SYNC + TxLoad instructions */
               (MOST_VERTICES(vertices) * (sizeof(uint32)))) *     /* Triangle Instructions */
              ts->ySlice));
        BLITSTUFF(bo)->slicedVertices = (gfloat *)AllocMem(BLITSTUFF(bo)->slicedVerticesSize, MEMTYPE_NORMAL);
        if (BLITSTUFF(bo)->slicedVertices == NULL)
        {
            return(BLITTER_ERR_NOMEM);
        }
        DPRTSV(("Sliced vertices @ 0x%lx\n", BLITSTUFF(bo)->slicedVertices));
        
            /* Because the blitter folio does not actually create the vertices itself, but lets the
             * app do that, we cannot make assumptions about the vertices to ease the
             * slicing process.
             *
             * So, for each slice, walk through all the triangles. Where any would cause a triangle
             * to be drawn that uses this slice, calculate the new vertices and add them to the
             * new vertex list.
             */
        new = (gfloat *)BLITSTUFF(bo)->slicedVertices;
        toU = vtx->vtx_uOffset;
        toV = (toU + 1);
        bbox = ts->bbox;
    
        for (i = 0; i < ts->ySlice; i++)
        {
            gfloat top;
            gfloat bottom;
            uint32 used;
            
            used = 0;
            top = (i * (ts->ySliceHeight - 1));  /* top line of this slice */
            bottom = (top + ((i < (ts->ySlice - 1)) ? (ts->ySliceHeight - 2) : (ts->ySliceLastHeight - 1)));
            DPRTSV(("Slice %ld from %g - %g\n", i, top, bottom));
            DPRTSV(("Sliced vertices start at 0x%lx\n", new));
            
                /* Prime the BoundingBox */
            bbox->leftMost = bbox->topMost = 100000;
            bbox->rightMost = bbox->bottomMost = 0;
            bbox->instructions = (uint32 *)new;
            
                /* Set up the TxLoad instructions */
            if (ts->txlOrig)
            {
                TxLoadSnippet *txl;
                CmdListP foo;
                uint32 size = (sizeof(TxLoadSnippet) - sizeof(BlitterSnippetHeader));
                
                foo = (CmdListP)new;
                CLT_Sync(&foo);
                new = (gfloat *)foo;
                txl = (TxLoadSnippet *)new;
                memcpy(txl, &ts->txlOrig->txl_instruction1, size);
                txl = (TxLoadSnippet *)((uint32)txl - sizeof(BlitterSnippetHeader));                
                txl->txl_src = (ts->txlOrig->txl_src + (((uint32)top * ts->bitsPerRow) / 8));
                txl->txl_count = ((i < (ts->ySlice - 1)) ? ts->ySliceHeight : ts->ySliceLastHeight);
                txl->txl_cntl &= ~FV_TXTLDCNTL_SRCBITOFF_MASK;
                txl->txl_cntl |= (((((uint32)top) * ts->bitsPerRow) & 7)
                                  << FV_TXTLDCNTL_SRCBITOFF_SHIFT);
                txl->txl_uvMax &= FV_TXTUVMAX_UMAX_MASK;
                txl->txl_uvMax |= (txl->txl_count - 1);
                new += (size / sizeof(uint32));
            }
            
            tri = &vtx->vtx_vertex[0];
                /* Get the first triangle */
            memcpy(v0, tri, (wordsPer * sizeof(gfloat)));
            tri += wordsPer;
            memcpy(v1, tri, (wordsPer * sizeof(gfloat)));
            tri += wordsPer;
            memcpy(v2, tri, (wordsPer * sizeof(gfloat)));
            tri += wordsPer;
                
            for (j = 0; j < ((vtx->vtx_instruction & FV_TRIANGLE_COUNT_MASK) - 1); j++)
            {
                high = low = v0[toV];
                high = ((high < v1[toV]) ? high : v1[toV]);
                low = ((low > v1[toV]) ? low : v1[toV]);
                high = ((high < v2[toV]) ? high : v2[toV]);
                low = ((low > v2[toV]) ? low : v2[toV]);
                DPRTSV(("\nCheck vertices (%g, %g) - (%g, %g) - (%g, %g) [%g : %g]\n",
                        v0[0], v0[1], v1[0], v1[1], v2[0], v2[1], high, low));
                
                    /* Is this triangle wholly inside the slice? */
                newCount = 0;
                if ((high >= top) && (low <= bottom))
                {
                    v0[toV] -= top;
                    v1[toV] -= top;
                    v1[toV] -= top;
                    newInstruction = (uint32 *)new; /* First 'gfloat' will be the  triangle instruction */
                    new++;
                    new = CopyVertex(new, vtx, v0, &newCount, bbox);
                    new = CopyVertex(new, vtx, v1, &newCount, bbox);
                    new = CopyVertex(new, vtx, v2, &newCount, bbox);
                    if (newCount)
                    {
                        *newInstruction = CLA_TRIANGLE(1, 1,
                                                       ((vtx->vtx_instruction & FV_TRIANGLE_PERSPECTIVE_MASK) >> FV_TRIANGLE_PERSPECTIVE_SHIFT),
                                                       1,
                                                       ((vtx->vtx_instruction & FV_TRIANGLE_SHADING_MASK) >> FV_TRIANGLE_SHADING_SHIFT),
                                                       newCount);
                        used = 1;
                        BLITSTUFF(bo)->totalStrips++;
                    }
                    else
                    {
                        new = (gfloat *)newInstruction;
                    }
                }
                    /* Does this triangle cross the slice boundary? */
                else if (((high <= top) && (low > top)) ||
                         ((high < bottom) && (low >= bottom)))
                {
                    newInstruction = (uint32 *)new; /* First 'gfloat' will be the  triangle instruction */
                    new++;
                    new = SliceTriangle(new, bbox, vtx, v0, v1, v2, top, bottom, &newCount);
                    if (newCount)
                    {
                        *newInstruction = CLA_TRIANGLE(1, 1,
                                                       ((vtx->vtx_instruction & FV_TRIANGLE_PERSPECTIVE_MASK) >> FV_TRIANGLE_PERSPECTIVE_SHIFT),
                                                       1,
                                                       ((vtx->vtx_instruction & FV_TRIANGLE_SHADING_MASK) >> FV_TRIANGLE_SHADING_SHIFT),
                                                       newCount);
                        used = 1;
                        BLITSTUFF(bo)->totalStrips++;
                    }
                    else
                    {
                        new = (gfloat *)newInstruction;
                    }
                }                    
                    /* else, this line is outside of this slice */

                if (replaceMedium)
                {
                        /* Do the ReplaceMedium stuff for the next triangle */
                    memcpy(v1, v2, (wordsPer * sizeof(gfloat)));
                    memcpy(v2, tri, (wordsPer * sizeof(gfloat)));
                }
                else
                {
                        /* ReplaceOldest */
                    memcpy(v0, v1, (wordsPer * sizeof(gfloat)));
                    memcpy(v1, v2, (wordsPer * sizeof(gfloat)));
                    memcpy(v2, tri, (wordsPer * sizeof(gfloat)));
                }
                tri += wordsPer;
            }
            if (used == 0)
            {
                    /* Move the pointer back, to remove the last set of TxLoad instructions */
                new -= (((sizeof(TxLoadSnippet) - sizeof(BlitterSnippetHeader)) / sizeof(uint32))
                        + 2);  /* +2 for the extra instruction reserved for the Sync instruction */
                    
            }
            BLITSTUFF(bo)->ySlicesUsed += used;

            DPRTSV(("BBox = %g, %g, %g, %g\n", bbox->leftMost, bbox->rightMost, bbox->topMost, bbox->bottomMost));
            bbox->topLeft.x = bbox->leftMost;
            bbox->topLeft.y = bbox->topMost;
            bbox->topRight.x = bbox->rightMost;
            bbox->topRight.y = bbox->topMost;
            bbox->bottomLeft.x = bbox->leftMost;
            bbox->bottomLeft.y = bbox->bottomMost;
            bbox->bottomRight.x = bbox->rightMost;
            bbox->bottomRight.y = bbox->bottomMost;
            bbox++;
        }
    }
    
    BLITSTUFF(bo)->slicedBytesUsed = ((uint32)new - (uint32)BLITSTUFF(bo)->slicedVertices);
    DPRTSV(("Used %ld slices, 0x%lx bytes\n", BLITSTUFF(bo)->ySlicesUsed, BLITSTUFF(bo)->slicedBytesUsed));
    
#ifdef BUILD_PARANOIA
    if (BLITSTUFF(bo)->slicedBytesUsed > BLITSTUFF(bo)->slicedVerticesSize)
    {
        printf("*** WARNING: SlicedVertices overran its buffer!! (used 0x%lx of 0x%lx)****\n",
               BLITSTUFF(bo)->slicedBytesUsed, BLITSTUFF(bo)->slicedVerticesSize);
    }
#endif
        /* Realloc the overflow */
    if (BLITSTUFF(bo)->slicedBytesUsed < BLITSTUFF(bo)->slicedVerticesSize)
    {
        void *newBuff;
        newBuff = ReallocMem((void *)BLITSTUFF(bo)->slicedVertices, 
                             BLITSTUFF(bo)->slicedVerticesSize, BLITSTUFF(bo)->slicedBytesUsed, MEMTYPE_NORMAL);
        DPRTSV(("realloced from 0x%lx (%ld) to 0x%lx (%ld)\n", BLITSTUFF(bo)->slicedVertices,
                BLITSTUFF(bo)->slicedVerticesSize, newBuff, BLITSTUFF(bo)->slicedBytesUsed));
        if (newBuff)
        {
            BLITSTUFF(bo)->slicedVertices = (gfloat *)newBuff;
        }
        BLITSTUFF(bo)->slicedVerticesSize = BLITSTUFF(bo)->slicedBytesUsed;
    }
        
    BLITSTUFF(bo)->flags |= VERTICES_SLICED;
    return(err);
}

void RemoveSlices(BlitObject *bo)
{
    TextureSlice *ts = TEXTURESLICE(bo->bo_txdata);
    
    if (ts && ts->bbox)
    {
        FreeMem(ts->bbox, (ts->ySlice * sizeof(BoundingBox)));
        ts->bbox = NULL;
    }
    
    if (BLITSTUFF(bo)->slicedVertices)
    {
        FreeMem(BLITSTUFF(bo)->slicedVertices, BLITSTUFF(bo)->slicedVerticesSize);
        BLITSTUFF(bo)->slicedVertices = NULL;
    }
    BLITSTUFF(bo)->flags &= ~VERTICES_SLICED;
}

Err AssociateBOToVtx(BlitObject *bo, VerticesSnippet *vtx)
{
    uint32 i, j;
    BOArray *boa;
    
    if (BOARRAY(vtx) == NULL)
    {
        vtx->vtx_private = AllocMem(sizeof(BOArray), (MEMTYPE_NORMAL | MEMTYPE_FILL | 0));
        if (BOARRAY(vtx) == NULL)
        {
            return(BLITTER_ERR_NOMEM);
        }
    }
    boa = BOARRAY(vtx);
    j = 0;
    do
    {
        BOArray *tmp;
        
        for (i = 0; i < BO_ARRAY_SIZE; i++)
        {
            j++;
            if (boa->bo[i] == NULL)
            {
                boa->bo[i] = bo;
                boa = NULL;
                DPRTBO(("BlitObject 0x%lx associated with VerticesSnippet 0x%lx\n", bo, vtx));
                break;
            }
        }
        if (boa)
        {
            tmp = boa->next;
            if (tmp == NULL)
            {
                tmp = (BOArray *)AllocMem(sizeof(BOArray), (MEMTYPE_NORMAL | MEMTYPE_FILL | 0));
                if (tmp == NULL)
                {
                    return(BLITTER_ERR_NOMEM);
                }
                boa->next = tmp;
            }
            boa = tmp;
        }
    }
    while (boa);
#ifdef BUILD_PARANOIA
    if (j != vtx->vtx_header.bsh_usageCount)
    {
        printf("**** BOArray mismatch. Should never see this message.\n");
    }
#endif
    return(0);
}

void RemoveBOToVtxAssociation(BlitObject *bo, VerticesSnippet *vtx)
{
    uint32 i;
    BOArray *boa;
    
    boa = BOARRAY(vtx);
    while (boa)
    {        
        for (i = 0; i < BO_ARRAY_SIZE; i++)
        {
            if ((uint32)boa->bo[i] == (uint32)bo)
            {
                boa->bo[i] = NULL;
                boa = NULL;
                DPRTBO(("BlitObject 0x%lx disassociated with VerticesSnippet 0x%lx\n", bo, vtx));
                break;
            }
        }
        if (boa)
        {
            boa = boa->next;
            if (boa == NULL)
            {
                DPRTBO(("BlitObject 0x%lx not found with VerticesSnippet 0x%lx\n", bo, vtx));
            }
        }
    }
   
    return;
}

bool Blt_BlitObjectSliced(BlitObject *bo)
{
    bool sliced;

    sliced = (BLITSTUFF(bo)->flags & VERTICES_SLICED);
    return(sliced);
}

