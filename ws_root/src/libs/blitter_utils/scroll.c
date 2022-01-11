/***************************************************************************
**
** @(#) blend.c 96/06/07 1.3
**
**  Code to scroll a region of a bitmap
**
****************************************************************************/

#include <kernel/types.h>
#include <graphics/clt/cltmacros.h>
#include <graphics/clt/clt.h>
#include <graphics/blitter.h>
#include <stdio.h>

Err Blt_Scroll(GState *gs, BlitObject *bo, Item bitmap, BlitScroll *bs)
{
    VerticesSnippet *vtx;
    gfloat height, width;
    gfloat x, y, u, v;
    bool up, left;
    Err err;
    
        /* This code assumes the VerticesSnippet for the scroll
         * has been allocated by the application.
         *
         * All we need for this operation is (x, y, u, v, w),
         * CLA_TRIANGLE(1, 1, 1, 1, 0, 4).
         */
    vtx = bo->bo_vertices;
    if (vtx == NULL)
    {
        return(BLITTER_ERR_NOVTX);
    }
    
        /* Build the vertices.
         * If scrolling up (bsc_dy is -ve), then set the V value of the top vertices
         * to -bsc_dy, which "pulls" the region up. Set the V and Y values of the bottom vertices
         * to (height - -bsc_dy).
         *
         * If scrolling down, then set the V value of the top vertices to 0, set Y to
         * (top + bsc_dy), and set the V value at the bottom to (height - bsc_dy).
         *
         * Perform similar tricks in the X direction.
         */
    width = (bs->bsc_region.max.x - bs->bsc_region.min.x);
    height = (bs->bsc_region.max.y - bs->bsc_region.min.y);
    up = (bs->bsc_dy < 0);
    left = (bs->bsc_dx < 0);
    
        /* First vertex is TopLeft */
    *BLITVERTEX_X(vtx, 0) = (left ? bs->bsc_region.min.x : (bs->bsc_region.min.x + bs->bsc_dx));
    *BLITVERTEX_Y(vtx, 0) = y = (up ? bs->bsc_region.min.y : (bs->bsc_region.min.y + bs->bsc_dy));
    *BLITVERTEX_U(vtx, 0) = (left ? -bs->bsc_dx : 0);
    *BLITVERTEX_V(vtx, 0) = v = (up ? -bs->bsc_dy : 0);
    *BLITVERTEX_W(vtx, 0) = W_2D;
        /* Second vertex is TopRight */
    *BLITVERTEX_X(vtx, 1) = x = (left ? (bs->bsc_region.max.x + bs->bsc_dx) : bs->bsc_region.max.x);
    *BLITVERTEX_Y(vtx, 1) = y;
    *BLITVERTEX_U(vtx, 1) = u = (left ? width : (width - bs->bsc_dx));
    *BLITVERTEX_V(vtx, 1) = v;
    *BLITVERTEX_W(vtx, 1) = W_2D;
        /* Third vertex is BottomRight */
    *BLITVERTEX_X(vtx, 2) = x;
    *BLITVERTEX_Y(vtx, 2) = y = (up ? (bs->bsc_region.max.y + bs->bsc_dy) : bs->bsc_region.max.y);
    *BLITVERTEX_U(vtx, 2) = u;
    *BLITVERTEX_V(vtx, 2) = v = (up ? height : (height - bs->bsc_dy));
    *BLITVERTEX_W(vtx, 2) = W_2D;
        /* Fourth vertex is BottomLeft */
    *BLITVERTEX_X(vtx, 3) = *BLITVERTEX_X(vtx, 0);
    *BLITVERTEX_Y(vtx, 3) = y;
    *BLITVERTEX_U(vtx, 3) = *BLITVERTEX_U(vtx, 0);
    *BLITVERTEX_V(vtx, 3) = v;
    *BLITVERTEX_W(vtx, 3) = W_2D;

        /* Even though we are writing directly into the VertexSnippet's vertex list,
         * we still need to let the Blitter folio know that the vertices have changed.
         *
         * This is so the Blitter folio can recalculate the sliced and/or clipped vertices if
         * necessary.
         */
    err = Blt_SetVertices(vtx, &vtx->vtx_vertex[0]);
    if (err < 0)
    {
        return(err);
    }
    
        /* Perform the actual blit */
    err = Blt_RectangleInBitmap(gs, bo, bitmap, &bs->bsc_region);
    if (err == BLITTER_ERR_TOOBIG)
    {
        BlitRect rect;
            /* No can do. The area will not fit in the TRAM, so we have
             * to blit the rectangle into a buffer and let the blitter folio
             * slice the texture and vertices for us.
             */
        rect = bs->bsc_region;
        if (up)
        {
            rect.min.y -= bs->bsc_dy;
        }
        else
        {
            rect.max.y -= bs->bsc_dy;
        }
        if (left)
        {
            rect.min.x -= bs->bsc_dx;
        }
        else
        {
            rect.max.x -= bs->bsc_dx;
        }
        *BLITVERTEX_U(vtx, 0) = 0;
        *BLITVERTEX_U(vtx, 1) = (rect.max.x - rect.min.x);
        *BLITVERTEX_U(vtx, 2) = (rect.max.x - rect.min.x);
        *BLITVERTEX_U(vtx, 3) = 0;
        *BLITVERTEX_V(vtx, 0) = 0;
        *BLITVERTEX_V(vtx, 1) = 0;
        *BLITVERTEX_V(vtx, 2) = (rect.max.y - rect.min.y);
        *BLITVERTEX_V(vtx, 3) = (rect.max.y - rect.min.y);

        err = Blt_RectangleToBlitObject(gs, bo, bitmap, &rect);
        if (err >= 0)
        {
                /* Blt_RectangleToBlitObject() called GS_SetDestBuffer(),
                 * so we must execute the TE instructions that blit
                 * from the src bitmap into the BlitObject and then
                 * restore the destination buffer.
                 */
            GS_SendList(gs);
            GS_WaitIO(gs);
            GS_SetDestBuffer(gs, bitmap);
            err = Blt_BlitObjectToBitmap(gs, bo, bitmap, 0);
        }
    }

    if ((err >= 0) && (bs->bsc_replaceColor))
    {
            /* Fill in the area revealed with a solid color. */
        gfloat r, g, b, a;
            
        r = ((gfloat)((bs->bsc_replaceColor & 0x00ff0000) >> 16) / 255.0);
        g = ((gfloat)((bs->bsc_replaceColor & 0x0000ff00) >> 8) / 255.0);
        b = ((gfloat)(bs->bsc_replaceColor & 0x000000ff) / 255.0);
        a = ((gfloat)((bs->bsc_replaceColor & 0xff000000) >> 24) / 255.0);

                /* Turn off the textures. */
        GS_Reserve(gs, CltNoTextureSnippet.size);
        CLT_CopySnippetData(GS_Ptr(gs), &CltNoTextureSnippet);
            
                /* We will draw a colored rectangle (two triangles) to replace
                 * the scrolled area, which are triangles of the form (x, y, r, g, b, a).
                 * If we are scrolling in both x and y directions we need two of these
                 * rectangles.
                 */
        GS_Reserve(gs, (((6 * 4) + 1) *
                        (((bs->bsc_dx != 0.0) && (bs->bsc_dy != 0.0)) ? 2 : 1)));
            
        if (bs->bsc_dx != 0.0)
        {
            gfloat xLeft, xRight;
                
            CLT_TRIANGLE(GS_Ptr(gs), 1, RC_FAN, 0, 0, 1, 4);
                /* TopLeft */
            xLeft = (left ? *BLITVERTEX_X(vtx, 1) : bs->bsc_region.min.x);
            CLT_VertexRgba(GS_Ptr(gs),
                           xLeft,
                           bs->bsc_region.min.y,
                           r, g, b, a);
                /* TopRight */
            xRight = (left ? bs->bsc_region.max.x : *BLITVERTEX_X(vtx, 0));
            CLT_VertexRgba(GS_Ptr(gs),
                           xRight,
                           bs->bsc_region.min.y,
                           r, g, b, a);
                /* BottomRight */
            CLT_VertexRgba(GS_Ptr(gs),
                           xRight,
                           bs->bsc_region.max.y,
                           r, g, b, a);
                /* BottomLeft */
            CLT_VertexRgba(GS_Ptr(gs),
                           xLeft, 
                           bs->bsc_region.max.y,
                           r, g, b, a);
        }
        if (bs->bsc_dy != 0.0)
        {
            gfloat yTop, yBottom;
                
            CLT_TRIANGLE(GS_Ptr(gs), 1, RC_FAN, 0, 0, 1, 4);
                /* TopLeft */
            yTop = (up ? *BLITVERTEX_Y(vtx, 3) : bs->bsc_region.min.y);
            CLT_VertexRgba(GS_Ptr(gs),
                           bs->bsc_region.min.x,
                           yTop,
                           r, g, b, a);
                /* TopRight */
            CLT_VertexRgba(GS_Ptr(gs),
                           bs->bsc_region.max.x,
                           yTop,
                           r, g, b, a);
                /* BottomRight */
            yBottom = (up ? bs->bsc_region.max.y : *BLITVERTEX_Y(vtx, 0));
            CLT_VertexRgba(GS_Ptr(gs),
                           bs->bsc_region.max.x,
                           yBottom,
                           r, g, b, a);
                /* BottomLeft */
            CLT_VertexRgba(GS_Ptr(gs),
                           bs->bsc_region.min.x,
                           yBottom,
                           r, g, b, a);
        }
    }
    
    return(err);
}


