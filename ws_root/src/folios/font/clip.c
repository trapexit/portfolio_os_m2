/* @(#) clip.c 96/06/13 1.5 */

#include <kernel/types.h>
#include <kernel/mem.h>
#include <graphics/font.h>
#include "fonttable.h"
#include <stdio.h>

#define DBUGC(x) /*printf x;*/

#ifdef BUILD_PARANOIA
#define CheckTextState(ts) if (!ts || (ts->ts_Cookie != ts)) return(FONT_ERR_BADTS);
#else
#define CheckTextState(ts)
#endif

/**
|||	AUTODOC -public -class Font -name SetClipBox
|||	Sets the clip box for a TextState.
|||
|||	  Synopsis
|||
|||	    Err SetClipBox(TextState *ts, bool set, Point2 *tl, Point2 *br);
|||
|||	  Description
|||
|||	    Enables or disables clipping for the TextState. The
|||	    boundary of the clip region is defined by the points tl
|||	    and br (top left and bottom right); pixels falling outside
|||	    of these points will be clipped.
|||
|||	  Arguments
|||
|||	    ts
|||	        Pointer to the TextState returned by
|||	        CreateTextState().
|||
|||	    set
|||	       Set this TRUE to enable clipping, FALSE to disable
|||	       clipping. Clipping is disabled by default.
|||
|||	    tl
|||	       Point specifying the top and left hand sides of the
|||	       clip region.
|||
|||	    br
|||	       Point specifying the bottom and right hand sides of the
|||	       clip region.
|||
|||	  Return Value
|||
|||	    Negative if there is an error.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Font folio V30.
|||
|||	  Caveat
|||
|||	    Calling SetClipBox() will not cause the text to be clipped
|||	    the next time DrawText() is called, but becomes effective
|||	    with the next call to MoveText(), ScaleText() or
|||	    RotateText(). To workaround this, call SetClipBox()
|||	    followed by MoveText(ts, 0.0, 0.0), DrawText().
|||	    (This workaround is not needed from V32 onwards).
|||
|||	  Associated Files
|||
|||	    <graphics/font.h>, System.m2/Modules/font
|||
|||	  See Also
|||
|||	    CreateTextState(), MoveText(), ScaleText(), RotateText()
|||
**/

Err SetClipBox(TextState *ts, bool set, Point2 *tl, Point2 *br)
{
    vertexInfo *nextVertex;
    uint32 *v;

    CheckTextState(ts);

    ts->ts_Clip = set;
    if (set)
    {
        ts->ts_LeftEdge = tl->x;
        ts->ts_RightEdge = br->x;
        ts->ts_TopEdge = tl->y;
        ts->ts_BottomEdge = br->y;
        
            /* Calculate the clip box by looking at all the vertices */
        ts->ts_Leftmost = ts->ts_Topmost = 100000;
        ts->ts_Rightmost = ts->ts_Bottommost = 0;
        nextVertex = (vertexInfo *)&ts->ts_Vertex;
        while (v = nextVertex->vertex)
        {
            uint32 shad;
            uint32 i;
            Point2 pt;
            
            nextVertex++;
                /* v points to the vertex instruction */
            shad = (*v & FV_TRIANGLE_SHADING_MASK);
            v++;
        
            for (i = 0; i < 4; i++)
            {
                    /* The first 2 words after the instruction are the x,y values */
                pt.x = *(gfloat *)v++;
                pt.y = *(gfloat *)v++;
                v += (shad ? 5 : 3);
                CalcClipBox(ts, &pt);
            }
        }
        DBUGC(("CLIPBOX = %g, %g, %g, %g\n", ts->ts_Leftmost, ts->ts_Topmost, ts->ts_Rightmost, ts->ts_Bottommost));

        /* Now force the vertices to be clipped. */
        MoveText(ts, 0.0, 0.0);
    }
    
    return(0);
}

void CalcClipBox(TextState *ts, Point2 *pt)
{
    if (pt->x < ts->ts_Leftmost)
    {
        ts->ts_Leftmost = pt->x;
    }
    if (pt->x > ts->ts_Rightmost)
    {
        ts->ts_Rightmost = pt->x;
    }
    if (pt->y < ts->ts_Topmost)
    {
        ts->ts_Topmost = pt->y;
    }
    if (pt->y > ts->ts_Bottommost)
    {
        ts->ts_Bottommost = pt->y;
    }
}

ClippedVertices *GetCV(TextState *ts)
{
    ClippedVertices *cv;

    cv = (ClippedVertices *)ts->ts_NextClip;
    if (!IsNode(&ts->ts_ClipList, (Node *)cv))
    {
        /* This is the last node in the list. We need another one. */
        cv = AllocMem(sizeof(ClippedVertices), MEMTYPE_NORMAL);
        if (cv == NULL)
        {
            return(NULL);
        }
        AddTail(&ts->ts_ClipList, (Node *)cv);
    }
    ts->ts_NextClip = (Node *)NextNode((Node *)cv);
    return(cv);
}

#define VERTICES 4
bool ClipBackground(TextState *ts, vertexInfo *vi, uint32 *v, uint32 *nextv)
{
    uint32 i;
    uint32 *vIn = v;
    ClippedVertices *cv;
    _geouv g[VERTICES];
    bool clipped;

    cv = GetCV(ts);
    if (cv == NULL)
    {
        /* Nowhere to stick the clipped vertices. The best we can
         * do is skip over this background and not render it.
         * Not the best solution, but at least it won't crash.
         */
        vi->saved[0] = vIn[0];
        vi->saved[1] = vIn[1];
        vi->saved[2] = vIn[2];
        CLT_JumpAbsolute(&vIn, nextv);
        vi->flags = SAVED_BGND;
        return(FALSE);
    }
    
    v++;    /* Skip over the instruction */
    for (i = 0; i < VERTICES; i++)
    {
        g[i].x = *(gfloat *)v++;
        g[i].y = *(gfloat *)v++;
        g[i].r = *(gfloat *)v++;
        g[i].g = *(gfloat *)v++;
        g[i].b = *(gfloat *)v++;
        g[i].a = *(gfloat *)v++;
        g[i].w = *(gfloat *)v++;
    }
    clipped = DoClip(ts, g, cv, SAVED_BGND);

    vi->saved[0] = vIn[0];
    vi->saved[1] = vIn[1];
    vi->saved[2] = vIn[2];
    if (clipped)
    {
            /* Jump in to these instructions */
        CLT_JumpAbsolute(&vIn, &cv->instructions[0]);
            /* And jump out of the clip instructions */
        CLT_JumpAbsolute(&cv->jumpBack, nextv);
    }
    else
    {
        /* Outside of the clip region! Jump over these vertices.*/
        CLT_JumpAbsolute(&vIn, nextv);
        /* And reuse this buffer */
        ts->ts_NextClip = (Node *)cv;
    }
    
    vi->flags = SAVED_BGND;
    return(TRUE);
}

/* RestoreCharacter() is also RestoreBackground() */
void RestoreBackground(vertexInfo *vi, uint32 *v)
{
    v[0] = vi->saved[0];
    v[1] = vi->saved[1];
    v[2] = vi->saved[2];
    vi->flags = 0;
}

bool ClipCharacter(TextState *ts, vertexInfo *vi, uint32 *v, uint32 *nextv)
{
    uint32 i;
    uint32 *vIn = v;
    ClippedVertices *cv;
    _geouv g[VERTICES];
    bool clipped;

    cv = GetCV(ts);
    if (cv == NULL)
    {
        /* Nowhere to stick the clipped vertices. The best we can
         * do is skip over this character and not render it.
         * Not the best solution, but at least it won't crash.
         */
        vi->saved[0] = vIn[0];
        vi->saved[1] = vIn[1];
        vi->saved[2] = vIn[2];
        CLT_JumpAbsolute(&vIn, nextv);
        vi->flags = SAVED_CHAR;
        return(FALSE);
    }
    
    v++;    /* Skip over the instruction */
    for (i = 0; i < VERTICES; i++)
    {
        g[i].x = *(gfloat *)v++;
        g[i].y = *(gfloat *)v++;
        g[i].w = *(gfloat *)v++;
        g[i].u = *(gfloat *)v++;
        g[i].v = *(gfloat *)v++;
    }
    clipped = DoClip(ts, g, cv, SAVED_CHAR);
    
    vi->saved[0] = vIn[0];
    vi->saved[1] = vIn[1];
    vi->saved[2] = vIn[2];
    if (clipped)
    {
            /* Jump in to these instructions */
        CLT_JumpAbsolute(&vIn, &cv->instructions[0]);
            /* And jump out of the clip instructions */
        CLT_JumpAbsolute(&cv->jumpBack, nextv);
    }
    else
    {
        /* Outside of the clip region! Jump over these vertices.*/
        CLT_JumpAbsolute(&vIn, nextv);
        /* And reuse this buffer */
        ts->ts_NextClip = (Node *)cv;
    }
    
    vi->flags = SAVED_CHAR;
    return(TRUE);
}

/* This stuff is lifted and modified from /frame2d/clipdraw.c */
bool DoClip(TextState *ts, _geouv *geo, ClippedVertices *cv, uint32 type)
{
    gfloat w, h, t, l, ratio;
    _geouv geo2[8], geo3[8], *g1, *g2;
    int32 c1, c2, i, iold;
    
    l = ts->ts_LeftEdge;
    w = ts->ts_RightEdge;
    t = ts->ts_TopEdge;
    h = ts->ts_BottomEdge;

    c1 = 4; g1 = geo;
    g2 = geo2;

    DBUGC(("Clipping to %g, %g, %g, %g\n", t, w, h, l));
    if (type == SAVED_BGND)
    {
        DBUGC(("0: (%g, %g)\n", geo[0].x, geo[0].y));
        DBUGC(("1: (%g, %g)\n", geo[1].x, geo[1].y));
        DBUGC(("2: (%g, %g)\n", geo[2].x, geo[2].y));
        DBUGC(("3: (%g, %g)\n", geo[3].x, geo[3].y));
    }
    else
    {
        DBUGC(("0: (%g, %g) (%g, %g)\n", geo[0].x, geo[0].y, geo[0].u, geo[0].v));
        DBUGC(("1: (%g, %g) (%g, %g)\n", geo[1].x, geo[1].y, geo[1].u, geo[1].v));
        DBUGC(("2: (%g, %g) (%g, %g)\n", geo[2].x, geo[2].y, geo[2].u, geo[2].v));
        DBUGC(("3: (%g, %g) (%g, %g)\n", geo[3].x, geo[3].y, geo[3].u, geo[3].v));
    }

      /* Clip against left edge of display */
    for (i=0; i < c1; i++)
    {
        if (g1[i].x < l) break;
    }
    if (i != c1)
    {
        DBUGC(("clip left\n"));
        c2 = 0;
        iold = c1-1;
        for (i=0; i < c1; i++)
        {
            if ((g1[iold].x < l && g1[i].x > l) ||
                (g1[iold].x > l && g1[i].x < l))
            {
                ratio = (l-g1[iold].x) / (g1[i].x - g1[iold].x);
                g2[c2].x = l;
                g2[c2].y = g1[iold].y+ratio*(g1[i].y-g1[iold].y);
                g2[c2].u = g1[iold].u+ratio*(g1[i].u-g1[iold].u);
                g2[c2].v = g1[iold].v+ratio*(g1[i].v-g1[iold].v);
                g2[c2].w = g1[iold].w+ratio*(g1[i].w-g1[iold].w);
                g2[c2].r = g1[iold].r+ratio*(g1[i].r-g1[iold].r);
                g2[c2].g = g1[iold].g+ratio*(g1[i].g-g1[iold].g);
                g2[c2].b = g1[iold].b+ratio*(g1[i].b-g1[iold].b);
                g2[c2].a = g1[iold].a+ratio*(g1[i].a-g1[iold].a);
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
    for (i=0; i < c1; i++)
    {
        if (g1[i].y < t) break;
    }
    if (i != c1)
    {
        DBUGC(("clip top\n"));
        c2 = 0;
        iold = c1-1;
        for (i=0; i < c1; i++)
        {
            if ((g1[iold].y < t && g1[i].y > t) ||
                (g1[iold].y > t && g1[i].y < t))
            {
                ratio = (t-g1[iold].y)/(g1[i].y-g1[iold].y);
                g2[c2].x = g1[iold].x+ratio*(g1[i].x-g1[iold].x);
                g2[c2].y = t;
                g2[c2].u = g1[iold].u+ratio*(g1[i].u-g1[iold].u);
                g2[c2].v = g1[iold].v+ratio*(g1[i].v-g1[iold].v);
                g2[c2].w = g1[iold].w+ratio*(g1[i].w-g1[iold].w);
                g2[c2].r = g1[iold].r+ratio*(g1[i].r-g1[iold].r);
                g2[c2].g = g1[iold].g+ratio*(g1[i].g-g1[iold].g);
                g2[c2].b = g1[iold].b+ratio*(g1[i].b-g1[iold].b);
                g2[c2].a = g1[iold].a+ratio*(g1[i].a-g1[iold].a);
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
    for (i=0; i < c1; i++)
    {
        if (g1[i].x > w) break;
    }
    if (i != c1)
    {
        DBUGC(("clip right\n"));
        c2 = 0;
        iold = c1-1;
        for (i=0; i < c1; i++)
        {
            if ((g1[iold].x < w && g1[i].x > w) ||
                (g1[iold].x > w && g1[i].x < w))
            {
                ratio = (w-g1[iold].x)/(g1[i].x-g1[iold].x);
                g2[c2].x = w;
                g2[c2].y = g1[iold].y+ratio*(g1[i].y-g1[iold].y);
                g2[c2].u = g1[iold].u+ratio*(g1[i].u-g1[iold].u);
                g2[c2].v = g1[iold].v+ratio*(g1[i].v-g1[iold].v);
                g2[c2].w = g1[iold].w+ratio*(g1[i].w-g1[iold].w);
                g2[c2].r = g1[iold].r+ratio*(g1[i].r-g1[iold].r);
                g2[c2].g = g1[iold].g+ratio*(g1[i].g-g1[iold].g);
                g2[c2].b = g1[iold].b+ratio*(g1[i].b-g1[iold].b);
                g2[c2].a = g1[iold].a+ratio*(g1[i].a-g1[iold].a);
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
    for (i=0; i<c1; i++)
    {
        if (g1[i].y > h) break;
    }
    if (i !=c1)
    {
        DBUGC(("clip bottom\n"));
        c2 = 0;
        iold = c1-1;
        for (i=0; i < c1; i++)
        {
            if ((g1[iold].y < h && g1[i].y > h) ||
                (g1[iold].y > h && g1[i].y < h))
            {
                ratio = (h-g1[iold].y)/(g1[i].y-g1[iold].y);
                g2[c2].x = g1[iold].x+ratio*(g1[i].x-g1[iold].x);
                g2[c2].y = h;
                g2[c2].u = g1[iold].u+ratio*(g1[i].u-g1[iold].u);
                g2[c2].v = g1[iold].v+ratio*(g1[i].v-g1[iold].v);
                g2[c2].w = g1[iold].w+ratio*(g1[i].w-g1[iold].w);
                g2[c2].r = g1[iold].r+ratio*(g1[i].r-g1[iold].r);
                g2[c2].g = g1[iold].g+ratio*(g1[i].g-g1[iold].g);
                g2[c2].b = g1[iold].b+ratio*(g1[i].b-g1[iold].b);
                g2[c2].a = g1[iold].a+ratio*(g1[i].a-g1[iold].a);
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
        CmdListP cl = (CmdListP)&cv->instructions[0];
        
        switch (type)
        {
            case (SAVED_BGND):
            {
                CLT_TRIANGLE (&cl, 1, RC_FAN, 1, 0, 1, c1);
                for (i = 0; i < c1; i++)
                {
                    DBUGC(("%ld --> %g, %g\n", i, g1[i].x, g1[i].y));
                    CLT_VertexRgbaW (&cl, g1[i].x, g1[i].y,
                                     g1[i].r, g1[i].g, g1[i].b, g1[i].a, .999998);
                }
                cv->jumpBack = cl;
                break;
            }
            case (SAVED_CHAR):
            {
                CLT_TRIANGLE (&cl, 1, RC_FAN, 1, 1, 0, c1);
                for (i = 0; i < c1; i++)
                {
                    DBUGC(("%ld --> %g, %g (%g, %g)\n", i, g1[i].x, g1[i].y, g1[i].u, g1[i].v));
                    CLT_VertexUvW (&cl, g1[i].x, g1[i].y,
                                   g1[i].u, g1[i].v, .999998);
                }
                cv->jumpBack = cl;
                break;
            }
            default:
                break;
        }
    }
    else
    {
        return(FALSE);
    }
    
    return(TRUE);
}
