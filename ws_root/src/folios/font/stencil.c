/* @(#) stencil.c 96/07/09 1.2 */

#include <kernel/types.h>
#include <kernel/mem.h>
#include <graphics/font.h>
#include <graphics/clt/clt.h>
#include <stdio.h>
#include <string.h>
#include "fonttable.h"

#ifdef BUILD_PARANOIA
#define CheckTextState(ts) if (!ts || (ts->ts_Cookie != ts)) return(FONT_ERR_BADTS);
#else
#define CheckTextState(ts)
#endif

#define DPRTCTS(x) /* printf x */
#define DPRTDTS(x) /* printf x */

int32 CharWidth(char theChar, FontDescriptor *fd);

/**
|||	AUTODOC -class Font -name CreateTextStencil
|||	Creates a TextStencil
|||
|||	  Synopsis
|||
|||	    Err CreateTextStencil(TextStencil **ts, const TextStencilInfo *tsi);
|||
|||	  Description
|||
|||	    Creates a TextStencil. A TextStencil is a blank TextState,
|||	    which is created to render a maximum number of characters
|||	    within a contiguous character range. Unlike a TextState,
|||	    where the characters cannot be changed once the TextState
|||	    has been created, a TextStencil allows the application to
|||	    change the characters rendered provided the characters are
|||	    within the range specified when the TextStencil was
|||	    created. This makes it ideal for rendering strings such as
|||	    scores, which are constantly changing, but whose
|||	    characters are always in the range '0' - '9', and is
|||	    faster than using DrawString() which is the alternative
|||	    method.
|||
|||	    A TextStencil contains a TextState, which can be found
|||	    with GetTextStateFromStencil(), so that after DrawTextStencil()
|||	    has been called with the string to render,
|||	    the usual TextState functions can be called, such as
|||	    SetClipBox() or TransformText() before being sent to the
|||	    TriangleEngine.
|||
|||	  Arguments
|||
|||	    ts
|||	        Pointer to a TextStencil pointer which will be
|||	        filled by teh function with the new TextStencil.
|||
|||	    tsi
|||	        Pointer to a TextStencilInfo, containing details of
|||	        the TextStencil to create (see <graphics/font.h>).
|||
|||	  Return Value
|||
|||	    The new TextStencil is stored in *ts, or a negative error
|||	    value is returned.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Font folio V30.
|||
|||	  Caveats
|||
|||	    FONT_ERR_BADSTENCILRANGE can be returned if the range of
|||	    characters set in the TextStencilInfo will not fit in the TRAM.
|||
|||	  Associated Files
|||
|||	    <graphics/font.h>, System.m2/Modules/font
|||
|||	  See Also
|||
|||	    DeleteTextStencil() DrawTextStencil() GetTextStateFromStencil()
|||
**/

Err CreateTextStencil(TextStencil **ts, const TextStencilInfo *tsi)
{
    TextStencil *myts;
    FontDescriptor *fd;
    FontTextArray fta;
    char *string;
    Err err;
    uint32 i;

    fd = CheckItem(tsi->tsi_Font, NST_FONT, FONT_FONT_NODE);
    if (fd == NULL)
    {
        return(FONT_ERR_BADITEM);
    }

    if ((tsi->tsi_MinChar < fd->fd_firstChar) || (tsi->tsi_MaxChar > fd->fd_lastChar) ||
        ((tsi->tsi_MaxChar - tsi->tsi_MinChar + 1) > fd->fd_maxCharsLoad))
    {
        return(FONT_ERR_BADSTENCILRANGE);
    }

    string = (char *)AllocMem((tsi->tsi_NumChars + 1), 0);
    if (string == NULL)
    {
        return(FONT_ERR_NOMEM);
    }

    myts = (TextStencil *)AllocMem(sizeof(TextStencil), 0);
    if (myts)
    {

        myts->info = *tsi;
            /* Create a FontTextArray designed to cause CreateTextState() to generate a TEList
             * that will set up a background rectangle, load the full range of characters into TRAM,
             * and generate  the vertices for the maximum number of characters to be rendered.
             */
        fta.fta_StructSize = sizeof(FontTextArray);
        memset(&fta.fta_Pen, 0, sizeof(FontTextArray));
        fta.fta_Pen.pen_X = 20;
        fta.fta_Pen.pen_Y = 20;
        fta.fta_Pen.pen_FgColor = tsi->tsi_FgColor;
        fta.fta_Pen.pen_BgColor = tsi->tsi_BgColor;
        fta.fta_Pen.pen_XScale = 1.0;
        fta.fta_Pen.pen_YScale = 1.0;
        fta.fta_Clip.min.x = 0.0;
        fta.fta_Clip.min.y = 0.0;
        fta.fta_Clip.max.x = 16384;  /* effectively disable clipping */
        fta.fta_Clip.max.y = 16384;
        fta.fta_String = string;
        fta.fta_NumChars = tsi->tsi_NumChars;

        for (i = 0; i < (tsi->tsi_NumChars - 1); i++)
        {
            string[i] = tsi->tsi_MinChar;
        }
        string[i++] = tsi->tsi_MaxChar;   /* This forces the full range of characters */
        string[i] = '\0';

        err = doCreateTextState(&myts->textState, tsi->tsi_Font, &fta, 1, FALSE);
        DPRTCTS(("TextState created = 0x%lx (TEList = 0x%lx)\n", myts->textState, myts->textState->ts_TEList));
    }
    else
    {
        err = FONT_ERR_NOMEM;
    }

    FreeMem(string, (tsi->tsi_NumChars + 1));

    *ts = myts;
    return(err);
}

/**
|||	AUTODOC -class Font -name DrawTextStencil
|||	Sets up the TextStencil to render a string
|||
|||	  Synopsis
|||
|||	    Err DrawTextStencil(GState *gs, TextStencil *ts,
|||	                        PenInfo *pen, char *string, uint32 numChars);
|||
|||	  Description
|||
|||	    Modifies the pre-computed Triangle Engine list created by
|||	    CreateTextStencil() to display the string, and links the
|||	    list into the GState, for rendering with GS_SendList() and
|||	    GS_WaitIO().
|||
|||	  Arguments
|||
|||	    gs
|||	        Pointer to a GState.
|||
|||	    ts
|||	        Pointer to a TextStencil.
|||
|||	    pen
|||	        Pointer to a PenInfo, for defining the position and
|||	        scale of the string.
|||
|||	    string
|||	        Pointer to the string to render
|||
|||	    numChars
|||	        Number of characters in the string.
|||
|||	  Return Value
|||
|||	    Negative on error.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Font folio V32.
|||
|||	  Caveats
|||
|||	    The pen_FgColor and pen_bgColor are ignored by this
|||	    call. The foreground and background colors are specified
|||	    in the TextStencilInfo structure passed to
|||	    CreateTextStencil().
|||
|||	  Associated Files
|||
|||	    <graphics/font.h>, System.m2/Modules/font
|||
|||	  See Also
|||
|||	    CreateTextStencil()
|||
**/

Err DrawTextStencil(GState *gs, TextStencil *ts, PenInfo *pen, char *string, uint32 numChars)
{
    Err err;
    uint32 i;
    gfloat x, y, u, v;
    gfloat fgW, bgW;
    vertexInfo *nextVertex;
    char letter;
    uint32 shad;
    gfloat *vtx;
    gfloat *background = NULL;
    uint32 charHeight, charWidth;
    uint32 descent, ascent;
    gfloat xPos, yPos;
    gfloat maxXPos = 0.0;

    CheckTextState(ts->textState);

    if (numChars > ts->info.tsi_NumChars)
    {
        return(FONT_ERR_BADSTENCILRANGE);
    }

    DPRTDTS(("list @ 0x%lx\n", ts->textState->ts_TEList));

        /* Walk through the string. For each character, set the u,v,x,y parameters
         * of the vertices in the TE list.
         */
    nextVertex = (vertexInfo *)&ts->textState->ts_Vertex;
    xPos = pen->pen_X;
    yPos = pen->pen_Y;
    charHeight = ts->textState->ts_Font->fd_charHeight;
    ascent = ts->textState->ts_Font->fd_ascent;
    descent = ts->textState->ts_Font->fd_descent;
    if (pen->pen_Flags & FLAG_PI_W_VALID)
    {
        fgW = pen->pen_FgW;
        bgW = pen->pen_BgW;
    }
    else
    {
        fgW = DEFAULT_FGW;
        bgW = DEFAULT_BGW;
    }

    for (i = 0; i < ts->info.tsi_NumChars; i++)
    {
        if (i < numChars)
        {
            letter = *string++;
            if ((letter < ts->info.tsi_MinChar) || (letter > ts->info.tsi_MaxChar))
            {
                return(FONT_ERR_BADSTENCILRANGE);
            }
            charWidth = CharWidth(letter, ts->textState->ts_Font);
            DPRTDTS(("charWidth = 0x%lx. xPos = %g, yPos = %g\n", charWidth, xPos, yPos));
        }
        else
        {
                /* There's no quick way to skip over the vertices that are not being used,
                 * so set them to render a pixel at (0, 0), which will be off the top of most
                 * monitors anyway.
                 */
            if (xPos > maxXPos)
            {
                maxXPos = xPos;
            }
            xPos = yPos = 0.0;
            charWidth = charHeight = 0;
            ascent = descent = 0;
            letter = 0;
        }

        vtx = (gfloat *)nextVertex->vertex;
        DPRTDTS(("vtx = 0x%lx\n", vtx));

            /* Restore clipped character or background? */
        if (nextVertex->flags)
        {
            DPRTDTS(("Restore clipped vertices from vertexInfo 0x%lx\n", nextVertex));
            RestoreCharacter(nextVertex, (uint32 *)vtx); /* This is the same as RestoreBackground() */
        }

        shad = (*(uint32 *)vtx & FV_TRIANGLE_SHADING_MASK);
        if (shad)
        {
            /* This is a background rectangle. Save this pointer
             * and we'll fill in the details later.
             */
            background = ++vtx;
            DPRTDTS(("background @ 0x%lx\n", background));
            nextVertex++;
            vtx = (gfloat *)nextVertex->vertex;
        }

        vtx++;                  /* Skip over the vertex instruction  */
            /* v now points to the x value of the first vertex of the character. I know
             * that there are 4 vertices, rendered in the order
             * topleft, topright, bottomright, bottomleft.
             */
        *vtx++ = xPos;
        *vtx++ = y = (yPos - (ascent * pen->pen_YScale));
        *vtx++ = fgW;
        *vtx++ = 0;           /* u */
        *vtx++ = v = (gfloat)(charHeight * (letter - ts->info.tsi_MinChar)); /* v */
              /* Second vertex  - top Right */
        *vtx++ = x = (xPos + (charWidth * pen->pen_XScale));
        *vtx++ = y;
        *vtx++ = fgW;
        *vtx++ = u = (gfloat)charWidth;       /* u */
        *vtx++ = v;
              /* Third vertex  - bottom Right */
        *vtx++ = x;
        *vtx++ = y = (yPos + (descent * pen->pen_YScale));
        *vtx++ = fgW;
        *vtx++ = u;
        *vtx++ = v = (v + charHeight);
            /* Fourth vertex - bottom left */
        *vtx++ = xPos;
        *vtx++ = y;
        *vtx++ = fgW;
        *vtx++ = 0;
        *vtx = v;

        nextVertex++;
        if (i < numChars)
        {
            xPos += ((charWidth + ts->textState->ts_Font->fd_charExtra) * pen->pen_XScale);
        }
    }

    if (background)
    {
        /* The vertices of the background rectangle are rendered
         * in the order TopLeft, BottomLeft, BottomRight, TopRight.
         */
        if (xPos > maxXPos)
        {
            maxXPos = xPos;
        }
        ascent = ts->textState->ts_Font->fd_ascent;
        descent = ts->textState->ts_Font->fd_descent;

        *background = x = pen->pen_X;
        *(background + 1) = (pen->pen_Y - (ascent * pen->pen_YScale));
        *(background + 6) = bgW;
        background += 7;        /* Skip x, y, r, g, b, a, w */
            /* BottomLeft */
        *background = x;
        *(background + 1) = y = (pen->pen_Y + (descent * pen->pen_YScale));
        *(background + 6) = bgW;
        background += 7;        /* Skip x, y, r, g, b, a, w */
            /* BottomRight */
        *background = x = (maxXPos - (ts->textState->ts_Font->fd_charExtra * pen->pen_XScale));
        *(background + 1) = y;
        *(background + 6) = bgW;
        background += 7;        /* Skip x, y, r, g, b, a, w */
            /* TopRight */
        *background = x;
        *(background + 1) = (pen->pen_Y - (ascent * pen->pen_YScale));
        *(background + 6) = bgW;
    }

    err = DrawText(gs, ts->textState);

    return(err);
}

/**
|||	AUTODOC -class Font -name GetTextStateFromStencil
|||	Returns the TextState associated with a TextStencil
|||
|||	  Synopsis
|||
|||	    TextState *GetTextStateFromStencil(TextStencil *ts);
|||
|||	  Description
|||
|||	    Returns the TextState associated with a TextStencil.
|||
|||	  Arguments
|||
|||	    ts
|||	        Pointer to a TextStencil.
|||
|||	  Return Value
|||
|||	    Pointer to the TextState associated with the TextStencil.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Font folio V32.
|||
|||	  Associated Files
|||
|||	    <graphics/font.h>, System.m2/Modules/font
|||
|||	  See Also
|||
|||	    DrawTextStencil()
|||
**/
TextState *GetTextStateFromStencil(TextStencil *ts)
{
    return(ts->textState);
}

/**
|||	AUTODOC -class Font -name DeleteTextStencil
|||	Deletes a TextStencil
|||
|||	  Synopsis
|||
|||	    Err DeleteTextStencil(TextStencil *ts);
|||
|||	  Description
|||
|||	    Deletes the resources associated with a TextStencil. You
|||	    should not call DeleteTextStencil() until the last use of
|||	    the TextStencil has been completed (when GS_WaitIO()
|||	    returns).
|||
|||	  Arguments
|||
|||	    ts
|||	        Pointer to the TextStencil to delete.
|||
|||	  Return Value
|||
|||	    Negative on error.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Font folio V32.
|||
|||	  Associated Files
|||
|||	    <graphics/font.h>, System.m2/Modules/font
|||
|||	  See Also
|||
|||	    CreateTextStencil()
|||
**/

Err DeleteTextStencil(TextStencil *ts)
{
    Err err;

    if (ts == NULL)
    {
        return(FONT_ERR_BADTS);
    }

    err = DeleteTextState(ts->textState);
    if (err < 0)
    {
        return(err);
    }

    FreeMem(ts, sizeof(TextStencil));
    return(0);
}
