/* @(#) parsevertices.c 96/06/17 1.2 */

#include <kernel/types.h>
#include <graphics/font.h>
#include "fonttable.h"
#include <stdio.h>

Err ParseVertices(TextState *ts, void (*callback)(gfloat *, void *), void *params)
{
    vertexInfo *nextVertex;
    Point2 *pt;
    uint32 vCount;
    uint32 *v;
    uint32 i;
    bool clip = ts->ts_Clip;
    
    /* Move the TextState's bounding rectangle */
    if (ts->ts_Track || clip)
    {
        pt = &ts->ts_TopLeft;
        ts->ts_Leftmost = ts->ts_Topmost = 100000;
        ts->ts_Rightmost = ts->ts_Bottommost = 0;
        for (i = 0; i < TS_POINTS; i++)
        {
            callback(&pt->x, params);
            if (clip)
            {
                CalcClipBox(ts, pt);
            }
            pt++;
        }

        if (clip &&
            ((ts->ts_Leftmost >= ts->ts_LeftEdge) &&
             (ts->ts_Rightmost <= ts->ts_RightEdge) &&
             (ts->ts_Topmost >= ts->ts_TopEdge) &&
             (ts->ts_Bottommost <= ts->ts_BottomEdge)))
        {
            clip = FALSE;
        }
    }
    else
    {
        /* We still need to track at least the Baseline, the TopLeft and
         * the TopRight, as these are used to calculate the Scale factor.
         * in GetTextScale() and angle of rotation in GetTextAngle().
         */
        callback(&ts->ts_TopLeft.x, params);
        callback(&ts->ts_TopRight.x, params);
        callback(&ts->ts_BaselineLeft.x, params);
        callback(&ts->ts_BaselineRight.x, params);
    }
    
    nextVertex = (vertexInfo *)&ts->ts_Vertex;
    if (!clip)
    {
        while (v = nextVertex->vertex)
        {
            uint32 shad;
        
            /* Restore clipped character or background? */
            if (nextVertex->flags)
            {
                RestoreCharacter(nextVertex, v); /* This is the same as RestoreBackground() */
            }
            
            nextVertex++;
                /* v points to the vertex instruction */
            vCount = 4;  /* All my triangle strips have 4 vertices */
            shad = (*v & FV_TRIANGLE_SHADING_MASK);
            v++;
        
                /* Modify the x and y values of the next vCount vertices. */
            for (i = 0; i < vCount; i++)
            {
                    /* The first 2 words after the instruction are the x,y values */
                callback((gfloat *)v, params);
                if (shad)
                {
                    v += 7;  /* Skip over x, y, r, g, b, a, w */
                }
                else
                {
                    v += 5;  /* Skip over x, y, u, v, w */
                }
            }
        }
    }
    else
    {
        while (v = nextVertex->vertex)
        {
            uint32 shad;
            uint32 *vOrig = v;
            bool clipThis = FALSE;
            
                /* v points to the vertex instruction */
            if (nextVertex->flags)
            {
                RestoreCharacter(nextVertex, v); /* This is the same as RestoreBackground() */
            }
            
            vCount = 4;  /* All my triangle strips have 4 vertices */
            shad = (*v & FV_TRIANGLE_SHADING_MASK);
            v++;
        
                /* Modify the x and y values of the next vCount vertices. */
            for (i = 0; i < vCount; i++)
            {
                float x, y;
                
                    /* The first 2 words after the instruction are the x,y values */
                callback((gfloat *)v, params);
                x = (*(float *)v);
                y = (*(float *)(v + 1));
                if (shad)
                {
                    v += 7;  /* Skip over x, y, r, g, b, a, w */
                }
                else
                {
                    v += 5;  /* Skip over x, y, u, v, w */
                }
                /* Is this vertex outside of our clip area? */
                if ((x < (float)ts->ts_LeftEdge) ||
                    (x > (float)ts->ts_RightEdge) ||
                    (y < (float)ts->ts_TopEdge) ||
                    (y > (float)ts->ts_BottomEdge))
                {
                    clipThis = TRUE;
                }
            }
            if (clipThis)
            {
                if (shad)
                {
                    /* Replace the vertices of the background with vertices that reach
                     * the edge of the clip window.
                     */
                    ClipBackground(ts, nextVertex, vOrig, v);
                }
                else
                {
                    ClipCharacter(ts, nextVertex, vOrig, v);
                }
            }
            nextVertex++;
        }
    }
    
    return(0);
}
