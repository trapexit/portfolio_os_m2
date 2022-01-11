/* @(#) extent.c 96/06/13 1.12 */

#include <kernel/types.h>
#include <graphics/font.h>
#include <stdio.h>
#include "fonttable.h"

#define DPRT(x) /*printf x*/
#ifdef BUILD_PARANOIA
#define CheckTextState(ts) if (!ts || (ts->ts_Cookie != ts)) return(FONT_ERR_BADTS);
#else
#define CheckTextState(ts)
#endif

int32 CharWidth(char theChar, FontDescriptor *fd);
#define LINE_HEIGHT(fd) (fd->fd_charHeight + fd->fd_leading)

/**
|||	AUTODOC -public -class Font -name GetStringExtent
|||	Gets the StringExtent of a string.
|||
|||	  Synopsis
|||
|||	    Err GetStringExtent(StringExtent *ts, Item font, 
|||	                        const PenInfo *pen, const char *string,
|||	                        uint32 numChars);
|||
|||	  Description
|||
|||	    Fills out a StringExtent structure based on the string, the
|||	    font, and the string's position. The StringExtent defines
|||	    the bounding box of the rendered string, accounting for
|||	    the spacing between characters, and the baseline and
|||	    descent defined in the font. Currently, the angle is 0,
|||	    but at somepoint in the future this may change to allow
|||	    for rotating strings, so all four corners of the box, and
|||	    the endpoints of a line defining the baseline are
|||	    specified.
|||
|||	  Arguments
|||
|||	    ts
|||	        Pointer to a StringExtent to fill.
|||
|||	    font
|||	        Font Item for the text.
|||
|||	    pen
|||	        Pointer to a PenInfo defining the coordinates of the
|||	        string.
|||
|||	    string
|||	        String to render.
|||
|||	    numChars
|||	        Number of characters in the string to render.
|||
|||	  Return Value
|||
|||	    Negative value returned if there is an error.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Font folio V30.
|||
|||	  Associated Files
|||
|||	    <graphics/font.h>, System.m2/Modules/font
|||
**/

Err GetStringExtent(StringExtent *se, Item fontI, const PenInfo *pen, const char *string, uint32 numChars)
{
    FontDescriptor *fd;
    uint32 i;
    uint32 charWidth;
    gfloat xPos, yPos;
    gfloat xMost;
    char letter;

        /* Do we have a font item? */
    fd = CheckItem(fontI, NST_FONT, FONT_FONT_NODE);
    if (fd == NULL)
    {
        return(FONT_ERR_BADITEM);
    }

    xPos = xMost = pen->pen_X;
    yPos = pen->pen_Y;
    for (i = 0; i < numChars; i++)
    {
        letter = *string++;
        if (letter == '\n')
        {
            yPos += (LINE_HEIGHT(fd) * pen->pen_YScale);
            xMost = ((xPos > xMost) ? xPos : xMost);
            xPos = pen->pen_X;
        }
        else
        {
            charWidth = CharWidth(letter, fd);
            xPos += ((charWidth + fd->fd_charExtra) * pen->pen_XScale);
        }
    }
    xMost = ((xPos > xMost) ? xPos : xMost);

    /* Currently, we are not dealing with rotated fonts, so the angle is always 0 */
    se->se_TopLeft.x = pen->pen_X;
    se->se_TopLeft.y = (pen->pen_Y - (fd->fd_ascent * pen->pen_YScale));
    se->se_TopRight.x = xMost;
    se->se_TopRight.y = se->se_TopLeft.y;
    se->se_BottomLeft.x = se->se_TopLeft.x;
    se->se_BottomLeft.y = (yPos + (fd->fd_descent * pen->pen_YScale));
    se->se_BottomRight.x = se->se_TopRight.x;
    se->se_BottomRight.y = se->se_BottomLeft.y;

    se->se_BaselineLeft.x = se->se_TopLeft.x;
    se->se_BaselineLeft.y = pen->pen_Y;
    se->se_BaselineRight.x = se->se_TopRight.x;
    se->se_BaselineRight.y = se->se_BaselineLeft.y;

    se->se_Angle = 0;
    se->se_Leading = (fd->fd_leading * pen->pen_YScale);

    return(0);
}

/**
|||	AUTODOC -public -class Font -name GetTextExtent
|||	Gets the TextExtent of a string.
|||
|||	  Synopsis
|||
|||	    Err GetTextExtent(const TextState *ts, TextExtent *te);
|||
|||	  Description
|||
|||	    Fills out a TextExtent structure (which happens to be a
|||	    StringExtent structure) based on the position, size, and
|||	    angle of the text in a TextState. The data returned is
|||	    only valid if bounds-tracking is enabled with
|||	    TrackTextBounds(), which is disabled by default.
|||
|||	  Arguments
|||
|||	    ts
|||	        Pointer to the TextState
|||
|||	    te
|||	        Pointer to a TextExtent to be filled.
|||
|||	  Return Value
|||
|||	    Negative value returned if there is an error.
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
|||	    TrackTextBounds()
|||
**/

Err GetTextExtent(const TextState *ts, TextExtent *te)
{
    gfloat sy;
    
    CheckTextState(ts);
    
        /* Copy the data from the TextState to the TextExtent */
    te->se_TopLeft = ts->ts_TopLeft;
    te->se_TopRight = ts->ts_TopRight;
    te->se_BottomLeft = ts->ts_BottomLeft;
    te->se_BottomRight = ts->ts_BottomRight;

    te->se_BaselineLeft = ts->ts_BaselineLeft;
    te->se_BaselineRight = ts->ts_BaselineRight;
    
    GetTextAngle(ts, &te->se_Angle);
    GetTextScale(ts, NULL, &sy);
    te->se_Leading = (ts->ts_Font->fd_leading * sy);

    return(0);
}
