/*
 * @(#) clip.c 96/10/15 1.4
 *
 * Code to clip the vertices.
 */

#include <kernel/types.h>
#include <kernel/mem.h>
#include <graphics/blitter.h>
#include <stdio.h>
#include <string.h>
#include "blitter_internal.h"

#define DPRT(x) /* printf x; */
#define DPRTCT(x) /* printf x; */
#define DPRTDCT(x) /* printf x; */

typedef struct _geouv
{
  gfloat x, y, u, v, w, r, g, b, a;
} _geouv;

uint32 *DoClipTriangle (BlitStuff *bs,  _geouv *geo, uint32 *dstInstruction,
                        uint32 toR, uint32 toW, uint32 toU, uint32 count);
bool PerfectRectangle(VerticesSnippet *vtx, bool replaceMedium);

Err Blt_SetClipBox(BlitObject *bo, bool set, bool clipOut, Point2 *tl, Point2 *br)
{
    BlitStuff *bs;
    Err err = 0;
    
    if (bo == NULL)
    {
        return(BLITTER_ERR_BADPTR);
    }

    bs = BLITSTUFF(bo);
    if (set)
    {
        bs->flags |= CLIP_ENABLED;
        if (clipOut)
        {
            bs->flags |= CLIP_OUT;
        }
        else
        {
            bs->flags &= ~CLIP_OUT;
        }
        bs->tl = *tl;
        bs->br = *br;
    }
    else
    {
        bs->flags &= ~CLIP_ENABLED;
    }

    return(err);
}

bool ClipTriangles(BlitObject *bo, uint32 *srcInstruction, uint32 **_dstInstruction, bool perfectRectangle)
{
    BlitStuff *bs;
    uint32 *dstInstruction = *_dstInstruction;
    uint32 vCount;
    uint32 i, j;
    uint32 first;
    uint32 wordsPer;
    uint32 toU, toW, toR;
    uint32 instructionType = (*srcInstruction & ~ FV_TRIANGLE_COUNT_MASK);
    gfloat *dstV;
    gfloat *srcV;
    gfloat *tmpV;
    gfloat l, r, t, b;
    _geouv v[4];
    bool replaceMedium;
    bool clipped;
    
    bs = BLITSTUFF(bo);
    l = bs->tl.x;
    t = bs->tl.y;
    r = bs->br.x;
    b = bs->br.y;
    vCount = ((*srcInstruction & FV_TRIANGLE_COUNT_MASK) + 1);
    dstV = (gfloat *)(dstInstruction + 1);
    replaceMedium = ((instructionType & FV_TRIANGLE_STRIPFAN_MASK) ? TRUE : FALSE);
    srcV = (gfloat *)(srcInstruction + 1);
    wordsPer = bo->bo_vertices->vtx_wordsPerVertex;
    toU = bo->bo_vertices->vtx_uOffset;
    toR = bo->bo_vertices->vtx_rOffset;
    toW = bo->bo_vertices->vtx_wOffset;
    clipped = FALSE;
    DPRTCT(("ClipTriangles(). %ld vertices, src = 0x%lx, dst = 0x%lx\n", vCount, srcInstruction, dstInstruction));
    DPRTCT(("rm = %s, %s %s %s\n", (replaceMedium ? "TRUE" : "FALSE"),
            (toU ? "U" : " "), (toR ? "R" : " "), (toW ? "W" : " ")));

        /* Optimisation: if there are 4 or less vertices in the BlitObject, then just run
         * through DoClipTriangle() once.
         */
    first = (vCount <= 4 ? vCount : 3);
        /* Get the first set of vertices */
    for (i = 0; i < first; i++)
    {
        memset(&v[i], 0, sizeof(_geouv));
        tmpV = srcV;
        v[i].x = *tmpV++;
        v[i].y = *tmpV;
        if (toR)
        {
            tmpV = (srcV + toR);
            v[i].r = *tmpV++;
            v[i].g = *tmpV++;
            v[i].b = *tmpV++;
            v[i].a = *tmpV;
        }
        if (toW)
        {
            v[i].w = *(srcV + toW);
        }
        if (toU)
        {
            tmpV = (srcV + toU);
            v[i].u = *tmpV++;
            v[i].v = *tmpV;
        }
        DPRTCT(("Vertex (%g %g) (%g %g) %g (%g %g %g %g)\n",
                v[i].x, v[i].y, v[i].u, v[i].v, v[i].w, v[i].r, v[i].g, v[i].b, v[i].a));
        srcV += wordsPer;
    }

    /* If this is a 4-vertex strip, then we need to swap over vertices 3 and 4. This is
     * because DoClipTriangle() doesn't know about vertex ordering, and so without
     * this switch the resultant vertex list would have vertices 3 and 4 swapped.
     */
    if (!replaceMedium && (vCount == 4))
    {
        _geouv tmp;
        tmp = v[2];
        v[2] = v[3];
        v[3] = tmp;
    }
    
    for (i = 0; i < (vCount - (first - 1)); i++)
    {
            /* Is this triangle inside the clip region? If it's a perfectRectangle,
             * then we know we have to clip, because the check was made
             * in ClipVertexList().
             */
        if (!perfectRectangle &&
            ((((v[0].x >= l) && (v[0].x <= r) && (v[0].y >= t) && (v[0].y <= b)) &&
              ((v[1].x >= l) && (v[1].x <= r) && (v[1].y >= t) && (v[1].y <= b)) &&
              ((v[2].x >= l) && (v[2].x <= r) && (v[2].y >= t) && (v[2].y <= b)) &&
              ((first > 3) && (v[3].x >= l) && (v[3].x <= r) && (v[3].y >= t) && (v[3].y <= b))) ||
                 /* If we are clipping internal pixels, only use the CPU to clip if
                  * any of the coordinates are negative.
                  */
             ((!(bs->flags & CLIP_OUT)) &&
              ((v[0].x >= 0.0) && (v[0].y >= 0.0) &&
               (v[1].x >= 0.0) && (v[1].y >= 0.0) &&
               (v[2].x >= 0.0) && (v[2].y >= 0.0) &&
               ((first > 3) && (v[3].x >= 0.0) && (v[3].y >= 0.0))))))
        {
            DPRTCT(("Vertex not clipped. Triangle @ 0x%lx\n", dstInstruction));
                /* Yep. Copy it into the temporary buffer */
            for (j = 0; j < first; j++)
            {
                *dstV++ = v[j].x;
                *dstV++ = v[j].y;
                if (toR)
                {
                    *dstV++ = v[j].r;
                    *dstV++ = v[j].g;
                    *dstV++ = v[j].b;
                    *dstV++ = v[j].a;
                }
                if (toW)
                {
                    *dstV++ = v[j].w;
                }
                if (toU)
                {
                    *dstV++ = v[j].u;
                    *dstV++ = v[j].v;
                }
            }
                /* This strip only has "first" vertices */
            *dstInstruction = (instructionType | (first - 1));
            dstInstruction = (uint32 *)dstV;
            dstV++;
        }
        else
        {
            DPRTCT(("Vertex clipped. Triangle @ 0x%lx\n", dstInstruction));
            clipped = TRUE;
            dstInstruction = DoClipTriangle(bs, v, dstInstruction, toR, toW, toU, first);
            dstV = ((gfloat *)dstInstruction + 1);
        }
        /* Look at the next triangle */
        if (replaceMedium)
        {
            memcpy(&v[1], &v[2], sizeof(_geouv));
        }
        else
        {
            memcpy(&v[0], &v[1], sizeof(_geouv));
            memcpy(&v[1], &v[2], sizeof(_geouv));
        }
        tmpV = srcV;
        v[2].x = *tmpV++;
        v[2].y = *tmpV;
        if (toR)
        {
            tmpV = (srcV + toR);
            v[2].r = *tmpV++;
            v[2].g = *tmpV++;
            v[2].b = *tmpV++;
            v[2].a = *tmpV;
        }
        if (toW)
        {
            v[2].w = *(srcV + toW);
        }
        if (toU)
        {
            tmpV = (srcV + toU);
            v[2].u = *tmpV++;
            v[2].v = *tmpV;
        }
        DPRTCT(("Vertex (%g %g) (%g %g) %g (%g %g %g %g)\n",
                v[2].x, v[2].y, v[2].u, v[2].v, v[2].w, v[2].r, v[2].g, v[2].b, v[2].a));
        srcV += wordsPer;
    }

        /* How many bytes were actually used? */
    DPRTCT(("Used 0x%lx - 0x%lx bytes\n", dstInstruction, BLITSTUFF(bo)->clippedVertices));
    BLITSTUFF(bo)->clippedBytesUsed = ((uint32)dstInstruction - (uint32)BLITSTUFF(bo)->clippedVertices);

#ifdef BUILD_PARANOIA
    if (BLITSTUFF(bo)->clippedBytesUsed > BLITSTUFF(bo)->clippedVerticesSize)
    {
        printf("*** WARNING: ClippedVertices overran its buffer!! (used 0x%lx of 0x%lx)****\n",
               BLITSTUFF(bo)->clippedBytesUsed, BLITSTUFF(bo)->clippedVerticesSize);
    }
#endif
    
    *_dstInstruction = dstInstruction;
    return(clipped);
}

/* Most bytes we could use to store all the clipped vertices:
 * 7 vertices per triangle, up to 9 words per vertex, 4 bytes per word.
 * Another uint32 is needed for the vertex instruction.
 */
#define MOST_CLIPPED_BYTES(x) ((x) * ((7 * 9 * sizeof(gfloat)) + sizeof(uint32)))
Err ClipVertexList(BlitObject *bo)
{
    BlitStuff *bs;
    uint32 flags;
    uint32 vCount;
    uint32 size;
    TextureSlice *ts = NULL;
    VerticesSnippet *vtx;
    BoundingBox *bbox;
    uint32 i;
    bool clipped;
    bool replaceMedium;
    bool perfectRectangle;

    bs = BLITSTUFF(bo);

        /* If this is a perfect rectangle, then we can do some quick checks here
         * to see if the vertices don't need to be clipped.
         */
    vtx = bo->bo_vertices;
    replaceMedium = ((vtx->vtx_instruction & FV_TRIANGLE_STRIPFAN_MASK) ? TRUE : FALSE);
    if (perfectRectangle = PerfectRectangle(vtx, replaceMedium))
    {
        Point2 tl, br;
        tl.x = *BLITVERTEX_X(vtx, 0);
        tl.y = *BLITVERTEX_Y(vtx, 0);
        br.x = *BLITVERTEX_X(vtx, (replaceMedium ? 2 : 3));
        br.y = *BLITVERTEX_Y(vtx, (replaceMedium ? 2 : 3));
            /* Is the rectangle inside the clip region? */
        if (((tl.x >= bs->tl.x) && (tl.x <= bs->br.x) && (tl.y >= bs->tl.y) && (tl.y <= bs->br.y)) &&
            ((br.x >= bs->tl.x) && (br.x <= bs->br.x) && (br.y >= bs->tl.y) && (br.y <= bs->br.y)))
        {
            bs->flags &= ~VERTICES_CLIPPED;
            bs->flags |= CLIP_CALCULATED;
            return(0);
        }
    }
    
    flags = bs->flags;

    vCount = ((bo->bo_vertices->vtx_instruction & FV_TRIANGLE_COUNT_MASK) + 1);
    if (!(flags & VERTICES_SLICED))
    {
        size = MOST_CLIPPED_BYTES(vCount - 2);
    }
    else
    {
        ts = TEXTURESLICE(bo->bo_txdata);
            /* For this buffer, we don't know how many triangles are in each slice, but
             * we know how many triangles were in the original list. So, take the number of
             * triangles, *3 worst case per slice, * # of slices.
             */
        size = MOST_CLIPPED_BYTES(((vCount - 2) * 3) * ts->ySlice);
    }
    
    bs->clippedBytesUsed = 0;
        /* Do we have enough RAM already allocated for the clipped vertices,
         * or do we need to allocate a larger block?
         */
    if (bs->clippedVertices && (bs->clippedVerticesSize < size))
    {
        FreeMem(bs->clippedVertices, bs->clippedVerticesSize);
        bs->clippedVertices = NULL;
    }
    if (bs->clippedVertices == NULL)
    {
            /* We need a buffer to put the clipped vertices in. */
        bs->clippedVertices = (uint32 *)AllocMem(size, MEMTYPE_NORMAL);
        DPRT(("clippedVertices 0x%lx 0x%lx\n", bs->clippedVertices, size));
        if (bs->clippedVertices == NULL)
        {
            return(BLITTER_ERR_NOMEM);
        }
        bs->clippedVerticesSize = size;
    }

    if (!(flags & VERTICES_SLICED))
    {
        uint32 *dst = bs->clippedVertices;
        clipped = ClipTriangles(bo, &bo->bo_vertices->vtx_instruction, &dst, perfectRectangle);
    }
    else
    {
            /* For sliced vertices, we are only going to clip slices that cross the clip box.
             * We are going to ignore slices that are totally outside of the clip box.
             */
        uint32 *dstInstruction = bs->clippedVertices;
        uint32 *srcInstruction;
        uint32 totalStrips;
        uint32 instruction;
        
        bbox = ts->bbox;
        clipped = FALSE;
        totalStrips = 0;
        DPRT(("Clip to H(%g %g) V(%g %g)\n", bs->tl.x, bs->br.x, bs->tl.y, bs->br.y));
        for (i = 0; i < bs->ySlicesUsed; i++)
        {
            if ((bbox->bottomMost <= 0.0) || (bbox->rightMost <= 0.0))
            {
                    /* This slice may be off the top or left, but set clipped TRUE so
                     * that if the whole BlitObject is off the top or left then
                     * Blt_BlitObjectToRectangle() will use 0 bytes of Vertex data.
                     */
                clipped = TRUE;
            }
            else if ((!(bs->flags & CLIP_OUT)) ||
                     (((bbox->bottomMost > bs->tl.y) && (bbox->topMost  < bs->br.y)) &&
                      ((bbox->rightMost > bs->tl.x) && (bbox->leftMost < bs->br.x))))
            {
                DPRT(("clip slice %ld: H(%g %g) V(%g, %g)\n", i, bbox->leftMost, bbox->rightMost, bbox->topMost, bbox->bottomMost));
                    /* Copy the TxLoad instructions. Don't forget the Sync instruction! */
                srcInstruction = bbox->instructions;
                size =  ((sizeof(TxLoadSnippet) - sizeof(BlitterSnippetHeader)) + (sizeof(uint32) * 2));
                memcpy(dstInstruction, srcInstruction, size);
                dstInstruction += (size / sizeof(uint32 *));
                srcInstruction += (size / sizeof(uint32 *));

                instruction = *srcInstruction;
                while (((instruction & 0xf0000000) == RC_WRITE_VERTEX) &&
                       (totalStrips < bs->totalStrips))
                {
                    totalStrips++;
                    clipped |= ClipTriangles(bo, srcInstruction, &dstInstruction, perfectRectangle);
                    srcInstruction += ((((instruction & FV_TRIANGLE_COUNT_MASK) + 1) *
                                        bo->bo_vertices->vtx_wordsPerVertex) + 1);
                    instruction = *srcInstruction;
                }
            }
            else
            {
                /* Totall outside the clip region */
                clipped = TRUE;
            }
            bbox++;
        }
    }
        
        /* If this is not a perfect rectangle, then realloc the overflow to regain a
         * potentially large chunk of RAM. If this is a perfect rectangle, then we leave the
         * allocated RAM alone. We may still lose a small number of bytes, but we won't have
         * to call AllocMem() the next time the BlitObject is clipped because we will have
         * enough RAM allocated for the worst case. Yes, this is an optimisation for rectangular
         * sprites and BlitObjects, which should be the vast majority of cases.
         */
    if ((bs->clippedBytesUsed == 0)  || !clipped)
    {
        DPRT(("Free the clippedVertices\n"));
        FreeMem(bs->clippedVertices, bs->clippedVerticesSize);
        bs->clippedVertices = NULL;
    }
    else if (!perfectRectangle && (bs->clippedBytesUsed < bs->clippedVerticesSize))
    {
        void *newBuff;
        newBuff = ReallocMem((void *)bs->clippedVertices, 
                            bs->clippedVerticesSize, bs->clippedBytesUsed, MEMTYPE_NORMAL);
        DPRT(("realloced from 0x%lx (%ld) to 0x%lx (%ld)\n", bs->clippedVertices,
                bs->clippedVerticesSize, newBuff, bs->clippedBytesUsed));
        if (newBuff)
        {
            bs->clippedVertices = (uint32 *)newBuff;
        }
        bs->clippedVerticesSize = bs->clippedBytesUsed;
    }
    if (clipped)
    {
        DPRT(("vertices were clipped\n"));
        bs->flags |= VERTICES_CLIPPED;
    }
    else
    {
        DPRT(("vertices not clipped\n"));      
        bs->flags &= ~VERTICES_CLIPPED;
    }
    
    bs->flags |= CLIP_CALCULATED;
    
    return(0);
}

/* This stuff is lifted and modified from /frame2d/clipdraw.c */
uint32 *DoClipTriangle (BlitStuff *bs,  _geouv *geo, uint32 *dstInstruction,
                     uint32 toR, uint32 toW, uint32 toU, uint32 count)
{
    gfloat w, h, t, l, ratio;
    _geouv geo2[8], geo3[8], *g1, *g2;
    int32 c1, c2, i, iold;

    if (bs->flags & CLIP_OUT)
    {
            /* clip away pixels outside of the clip box. In this case, clip pixels with
             * the CPU.
             */
        l = bs->tl.x;
        t = bs->tl.y;
        w = bs->br.x;
        h = bs->br.y;
    }
    else
    {
            /* clip away pixels inside the clip box. In this case, let the hardware
             * do the clipping (especially as a triangle may start outside the clip box,
             * go through it, and come back outside again).
             *
             * We must clip to the top and left edges at least with the CPU.
             */
        l = 0.0;
        t = 0.0;
        w = 100000.0;
        h = 100000.0;
    }
    
    DPRTDCT(("DoClipTriangle(). Clipping to %g %g %g %g\n", l, t, w, h));
    
    c1 = count;
    g1 = geo;
    g2 = geo2;
  
        /* Clip against left edge of display */
    for (i = 0; i < c1; i++)
    {
        if (g1[i].x < l)
        {
            break;
        }
    }
    if (i < c1)
    {
        c2 = 0;
        iold = (c1 - 1);
        for (i = 0; i < c1; i++)
        {
            if (((g1[iold].x < l) && (g1[i].x > l)) || ((g1[iold].x > l) && (g1[i].x < l)))
            {
                ratio = ((l - g1[iold].x) / (g1[i].x - g1[iold].x));
                DPRTDCT(("Left %ld: ratio = %g\n", i, ratio));
                g2[c2].x = l;
                g2[c2].y = (g1[iold].y + (ratio * (g1[i].y - g1[iold].y)));
                g2[c2].u = (g1[iold].u + (ratio * (g1[i].u - g1[iold].u)));
                g2[c2].v = (g1[iold].v + (ratio * (g1[i].v - g1[iold].v)));
                g2[c2].w = (g1[iold].w + (ratio * (g1[i].w - g1[iold].w)));
                g2[c2].r = (g1[iold].r + (ratio * (g1[i].r - g1[iold].r)));
                g2[c2].g = (g1[iold].g + (ratio * (g1[i].g - g1[iold].g)));
                g2[c2].b = (g1[iold].b + (ratio * (g1[i].b - g1[iold].b)));
                g2[c2].a = (g1[iold].a + (ratio * (g1[i].a - g1[iold].a)));
                c2++;
            }
            if (g1[i].x >= l)
            {
                g2[c2].x = g1[i].x;
                g2[c2].y = g1[i].y;
                g2[c2].u = g1[i].u;
                g2[c2].v = g1[i].v;
                g2[c2].w = g1[i].w;
                g2[c2].r = g1[i].r;
                g2[c2].g = g1[i].g;
                g2[c2].b = g1[i].b;
                g2[c2].a = g1[i].a;
                c2++;
            }
            iold = i;
        }
        g1 = g2;
        c1 = c2;
        g2 = (_geouv*)((int32)g1 ^ (int32)geo2 ^ (int32)geo3);
    }

        /* Clip against top edge of display */
    for (i = 0; i < c1; i++)
    {
        if (g1[i].y < t)
        {
            break;
        }
    }
    if (i < c1)
    {
        c2 = 0;
        iold = c1-1;
        for (i = 0; i < c1; i++)
        {
            if (((g1[iold].y < t) && (g1[i].y > t)) || ((g1[iold].y > t) && (g1[i].y < t)))
            {
                ratio = ((t - g1[iold].y) / (g1[i].y - g1[iold].y));
                DPRTDCT(("Top %ld: ratio = %g\n", i, ratio));
                g2[c2].x = (g1[iold].x + (ratio * (g1[i].x- g1[iold].x)));
                g2[c2].y = t;
                g2[c2].u = (g1[iold].u + (ratio * (g1[i].u - g1[iold].u)));
                g2[c2].v = (g1[iold].v + (ratio * (g1[i].v - g1[iold].v)));
                g2[c2].w = (g1[iold].w + (ratio * (g1[i].w - g1[iold].w)));
                g2[c2].r = (g1[iold].r + (ratio * (g1[i].r - g1[iold].r)));
                g2[c2].g = (g1[iold].g + (ratio * (g1[i].g - g1[iold].g)));
                g2[c2].b = (g1[iold].b + (ratio * (g1[i].b - g1[iold].b)));
                g2[c2].a = (g1[iold].a + (ratio * (g1[i].a - g1[iold].a)));
                c2++;
            }
            if (g1[i].y >= t)
            {
                g2[c2].x = g1[i].x;
                g2[c2].y = g1[i].y;
                g2[c2].u = g1[i].u;
                g2[c2].v = g1[i].v;
                g2[c2].w = g1[i].w;
                g2[c2].r = g1[i].r;
                g2[c2].g = g1[i].g;
                g2[c2].b = g1[i].b;
                g2[c2].a = g1[i].a;
                c2++;
            }
            iold = i;
        }
        g1 = g2;
        c1 = c2;
        g2 = (_geouv*)((int32)g1 ^ (int32)geo2 ^ (int32)geo3);
    }

        /* clip against right edge of display */
    for (i = 0; i < c1; i++)
    {
        if (g1[i].x > w)
        {
            break;
        }
    }
    if (i < c1)
    {
        c2 = 0;
        iold = (c1 - 1);
        for (i = 0; i < c1; i++)
        {
            if (((g1[iold].x < w) && (g1[i].x > w)) || ((g1[iold].x > w) && (g1[i].x < w)))
            {
                ratio = ((w - g1[iold].x) / (g1[i].x - g1[iold].x));
                DPRTDCT(("Right %ld: ratio = %g\n", i, ratio));
                g2[c2].x = w;
                g2[c2].y = (g1[iold].y + (ratio * (g1[i].y - g1[iold].y)));
                g2[c2].u = (g1[iold].u + (ratio * (g1[i].u - g1[iold].u)));
                g2[c2].v = (g1[iold].v + (ratio * (g1[i].v - g1[iold].v)));
                g2[c2].w = (g1[iold].w + (ratio * (g1[i].w - g1[iold].w)));
                g2[c2].r = (g1[iold].r + (ratio * (g1[i].r - g1[iold].r)));
                g2[c2].g = (g1[iold].g + (ratio * (g1[i].g - g1[iold].g)));
                g2[c2].b = (g1[iold].b + (ratio * (g1[i].b - g1[iold].b)));
                g2[c2].a = (g1[iold].a + (ratio * (g1[i].a - g1[iold].a)));
                c2++;
            }
            if (g1[i].x <= w)
            {
                g2[c2].x = g1[i].x;
                g2[c2].y = g1[i].y;
                g2[c2].u = g1[i].u;
                g2[c2].v = g1[i].v;
                g2[c2].w = g1[i].w;
                g2[c2].r = g1[i].r;
                g2[c2].g = g1[i].g;
                g2[c2].b = g1[i].b;
                g2[c2].a = g1[i].a;
                c2++;
            }
            iold = i;
        }
        g1 = g2;
        c1 = c2;
        g2 = (_geouv*)((int32)g1 ^ (int32)geo2 ^ (int32)geo3);
    }

        /* clip against bottom edge of display */
    for (i = 0; i < c1; i++)
    {
        if (g1[i].y > h)
        {
            break;
        }
    }
    if (i < c1)
    {
        c2 = 0;
        iold = (c1-1);
        for (i = 0; i < c1; i++)
        {
            if (((g1[iold].y < h) && (g1[i].y > h)) || ((g1[iold].y > h) && (g1[i].y < h)))
            {
                ratio = ((h - g1[iold].y) / (g1[i].y - g1[iold].y));
                DPRTDCT(("Bottom %ld: ratio = %g\n", i, ratio));
                g2[c2].x = (g1[iold].x + (ratio * (g1[i].x - g1[iold].x)));
                g2[c2].y = h;
                g2[c2].u = (g1[iold].u + (ratio * (g1[i].u - g1[iold].u)));
                g2[c2].v = (g1[iold].v + (ratio * (g1[i].v - g1[iold].v)));
                g2[c2].w = (g1[iold].w + (ratio * (g1[i].w - g1[iold].w)));
                g2[c2].r = (g1[iold].r + (ratio * (g1[i].r - g1[iold].r)));
                g2[c2].g = (g1[iold].g + (ratio * (g1[i].g - g1[iold].g)));
                g2[c2].b = (g1[iold].b + (ratio * (g1[i].b - g1[iold].b)));
                g2[c2].a = (g1[iold].a + (ratio * (g1[i].a - g1[iold].a)));
                c2++;
            }
            if (g1[i].y <= h)
            {
                g2[c2].x = g1[i].x;
                g2[c2].y = g1[i].y;
                g2[c2].u = g1[i].u;
                g2[c2].v = g1[i].v;
                g2[c2].w = g1[i].w;
                g2[c2].r = g1[i].r;
                g2[c2].g = g1[i].g;
                g2[c2].b = g1[i].b;
                g2[c2].a = g1[i].a;
                c2++;
            }
            iold = i;
        }
        g1 = g2;
        c1 = c2;
    }

    if (c1 > 2)
    {
        gfloat *dstV;

        DPRTDCT(("Clipped to %ld vertices at 0x%lx\n", c1, dstInstruction));
        
        CLT_TRIANGLE(&dstInstruction, 1, RC_FAN,
                     (toW ? 1 : 0),
                     (toU ? 1 : 0),
                     (toR ? 1 : 0),
                     c1);
        dstV = (gfloat *)dstInstruction;
        for (i = 0; i < c1; i++)
        {
            *dstV++ = g1[i].x;
            *dstV++ = g1[i].y;
            if (toR)
            {
                *dstV++ = g1[i].r;
                *dstV++ = g1[i].g;
                *dstV++ = g1[i].b;
                *dstV++ = g1[i].a;
            }
            if (toW)
            {
                *dstV++ = g1[i].w;
            }
            if (toU)
            {
                *dstV++ = g1[i].u;
                *dstV++ = g1[i].v;
            }
        }
        return((uint32 *)dstV);
    }
    return(dstInstruction);
}

bool Blt_BlitObjectClipped(BlitObject *bo)
{
    bool clipped;

    clipped = (BLITSTUFF(bo)->flags & VERTICES_CLIPPED);
    return(clipped);
}

void RemoveClippedVertices(BlitObject *bo)
{
    BlitStuff *bs = BLITSTUFF(bo);
    
    if (bs->clippedVertices)
    {
        FreeMem(bs->clippedVertices, bs->clippedVerticesSize);
    }
}
