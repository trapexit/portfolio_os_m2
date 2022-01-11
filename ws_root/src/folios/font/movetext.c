/* @(#) movetext.c 96/06/13 1.9 */

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

void doMoveText(gfloat *v, void *params)
{
    gfloat *gParams = (gfloat *)params;
    gfloat dx, dy;

    dx = gParams[0];
    dy = gParams[1];

    *v += dx;
    *(v + 1) += dy;

    return;
}

    
/**
|||	AUTODOC -public -class Font -name MoveText
|||	Changes the position of text in a TextState
|||
|||	  Synopsis
|||
|||	    Err MoveText(TextState *ts, gfloat dx, gfloat dy);
|||
|||	  Description
|||
|||	    Moves the text in the TextState by the delta values dx and
|||	    dy.
|||
|||	    After calling MoveText(), you need to call DrawText() to
|||	    link the new Triangle Engine instructions into the GState,
|||	    followed by GS_SendList() and GS_WaitIO().
|||
|||	  Arguments
|||
|||	    ts
|||	        Pointer to the TextState returned by
|||	        CreateTextState().
|||
|||	    dx
|||	        Delta x value for moving the text.
|||
|||	    dy
|||	        Delta y value for moving the text.
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
|||	    CreateTextState(), DrawText(), GetTextPosition()
|||
**/

Err MoveText(TextState *ts, gfloat dx, gfloat dy)
{
    Err err;
    gfloat params[2];
    
    CheckTextState(ts);
    
    params[0] = dx;
    params[1] = dy;
    err = ParseVertices(ts, doMoveText, (void *)params);
    
    return(err);
}

/**
|||	AUTODOC -public -class Font -name GetTextPosition
|||	Gets the current position of the text in the TextState
|||
|||	  Synopsis
|||
|||	    Err GetTextPosition(const TextState *ts, Point2 *pt);
|||
|||	  Description
|||
|||	    Puts the current (x, y) coordinates of the baseline of the
|||	    first string in the TextState in the pt
|||	    structure. Remember, a TextState can contain more than one
|||	    string that to be rendered, so the (x, y) value refers
|||	    to the first string in the FontTextArray that was passed
|||	    to CreateTextState().
|||
|||	  Arguments
|||
|||	    ts
|||	        Pointer to the TextState returned by
|||	        CreateTextState().
|||
|||	    pt
|||	        Pointer to a Point2 structure that will be filled with
|||	        the Text's current position.
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
|||	    CreateTextState(), MoveText()
|||
**/

Err GetTextPosition(const TextState *ts, Point2 *pt)
{
    CheckTextState(ts);
    *pt = ts->ts_BaselineLeft;
    return(0);
}

