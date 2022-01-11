/* @(#) transformtext.c 96/06/13 1.3 */

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

void doTransformText(gfloat *v, FontMatrix fm)
{
    gfloat x, y;

    x = *v;
    y = *(v + 1);
    
    *v = ((fm[0][0] * x) + (fm[1][0] * y) + fm[2][0]);
    *(v + 1) = ((fm[0][1] * x) + (fm[1][1] * y) + fm[2][1]);

    return;
}

/**
|||	AUTODOC -class Font -name TransformText
|||	Applies a transformation matrix to a TextState
|||
|||	  Synopsis
|||
|||	    Err TransformText(TextState *ts, FontMatrix fm);
|||
|||	  Description
|||
|||	    Applies the transformation matrix fm to the vectors in the
|||	    TextState. This allows for combinations of move, rotate
|||	    and scaling of the text.
|||
|||	  Arguments
|||
|||	    ts
|||	        Pointer to the TextState returned by
|||	        CreateTextState().
|||
|||	    fm
|||	      A FontMatrix. This is a 3x2 matrix (it is assumed that
|||	      the third column is (0, 0, 1) of the transformation.
|||
|||	  Return Value
|||
|||	    Negative if there is an error.
|||
|||	  Implementation
|||
|||	    Folio call implemented in Font folio V32.
|||
|||	  Associated Files
|||
|||	    <graphics/font.h>
|||
|||	  See Also
|||
|||	    CreateTextState() DrawText() GetTextPosition()
|||
**/

Err TransformText(TextState *ts, FontMatrix fm)
{
    Err err;
    gfloat *param;
    
    CheckTextState(ts);
    
    param = &fm[0][0];
    err = ParseVertices(ts, (void (*)(gfloat *, void *))doTransformText, (void *)param);
    
    return(err);
}


