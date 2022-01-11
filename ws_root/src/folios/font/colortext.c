/* @(#)colortext.c96/06/121.2 */

#include <kernel/types.h>
#include <graphics/font.h>
#include <graphics/clt/clt.h>
#include "fontfile.h"
#include "fonttable.h"
#include <stdio.h>

#ifdef BUILD_PARANOIA
#define CheckTextState(ts) if (!ts || (ts->ts_Cookie != ts)) return(FONT_ERR_BADTS);
#else
#define CheckTextState(ts)
#endif

/**
|||	AUTODOC -public -class Font -name SetTextColor
|||	Sets the color of all the text in a TextState
|||
|||	  Synopsis
|||
|||	    Err SetTextColor(TextState *ts, uint32 fg, uint32 bg);
|||
|||	  Description
|||
|||	    Sets the color of all the text in a TextState to the
|||	    foreground and background colors specified. Even if the
|||	    TextState was created with multiple strings of different
|||	    colors, all the strings in the TextState will have the
|||	    same color after calling this function.
|||
|||	    Note that if the TextState was created with a background
|||	    color of 0, then the bg parameter of this function is
|||	    ignored. ie, this function will not render a background
|||	    over text which was not created with a background.
|||
|||	    After calling SetTextColor(), you need to call DrawText() to
|||	    link the new Triangle Engine instructions into the GState,
|||	    followed by GS_SendList() and GS_WaitIO().
|||
|||	  Arguments
|||
|||	    ts
|||	        Pointer to the TextState returned by
|||	        CreateTextState().
|||
|||	    fg
|||	        rgb value for the new foreground color.
|||
|||	    bg
|||	        rgba value for the new background color.
|||
|||	  Return Value
|||
|||	    Negative if there is an error.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Font folio V30.
|||
|||	  Associated Files
|||
|||	    <graphics/font.h>, System.m2/Modules/font
|||
|||	  See Also
|||
|||	    CreateTextState(), DrawText()
|||
**/

Err SetTextColor(TextState *ts, uint32 fg, uint32 bg)
{
    vertexInfo *nextVertex;
    uint32 vCount;
    uint32 *v;
    uint32 i, j;
    
    CheckTextState(ts);
    
    nextVertex = (vertexInfo *)&ts->ts_Vertex;
        /* Modify all the background rectangles */
    for (j = 0; j < ts->ts_bgCount; j++)
    {
        v = nextVertex->vertex;
        nextVertex++;
            /* v points to the vertex instruction */
        vCount = 4;  /* All my triangle strips have 4 vertices */
        v++;
        
            /* Modify the rgba values of the next vCount vertices. */
        for (i = 0; i < vCount; i++)
        {
            gfloat r, g ,b, a;
                /* The first 2 words after the instruction are the x,y values */
            v += 2;
            
            a = (((bg >> 24) & 0xff) / 255.0);
            r = (((bg >> 16) & 0xff) / 255.0);
            g = (((bg >> 8) & 0xff) / 255.0);
            b = ((bg & 0xff) / 255.0);
            
            *(gfloat *)v++ = r;
            *(gfloat *)v++ = g;
            *(gfloat *)v++ = b;
            *(gfloat *)v++ = a;
            v++;  /* Skip over w */
        }
    }

    /* Now do all the foreground colors */
    nextVertex = (vertexInfo *)&ts->ts_Vertex;
    j = 0;
    while ((v = nextVertex->vertex) && (j < ts->ts_colorChangeCnt))
    {
        nextVertex++;

        /* Look backwards from this instruction for the TXTCONST0 instruction. */
        if (*(v - 9) == CLT_WriteRegistersHeader(TXTCONST0, 2))
        {
            *(v - 8) = fg;   /* TXTCONST0 */
            *(v - 7) = fg;   /* TXTCONST1 */
            *(v - 5) = bg;   /* DBCONSTIN */
            j++;
        }
    }
    
    return(0);
}
