/* @(#) scaletext.c 96/07/09 1.8 */

#include <kernel/types.h>
#include <graphics/font.h>
#include <graphics/clt/clt.h>
#include "fontfile.h"
#include "fonttable.h"
#include <stdio.h>
#include <math.h>

#ifdef BUILD_PARANOIA
#define CheckTextState(ts) if (!ts || (ts->ts_Cookie != ts)) return(FONT_ERR_BADTS);
#else
#define CheckTextState(ts)
#endif

#define DPRT(x) /*printf x*/

/**
|||	AUTODOC -public -class Font -name ScaleText
|||	Changes the size of text in a TextState
|||
|||	  Synopsis
|||
|||	    Err ScaleText(TextState *ts, gfloat sx, gfloat sy);
|||
|||	  Description
|||
|||	    Resizes the text in the TextState by the size of sx and
|||	    sy. The text is scaled relative to the coordinate of the
|||	    baseline of the first character of the FontTextArray
|||	    associated with this TextState, and is relative to the
|||	    current size of the text, not the original size.
|||
|||	    After calling ScaleText(), you need to call DrawText() to
|||	    link the new Triangle Engine instructions into the GState,
|||	    followed by GS_SendList() and GS_WaitIO().
|||
|||	  Arguments
|||
|||	    ts
|||	        Pointer to the TextState returned by
|||	        CreateTextState().
|||
|||	    sx
|||	        x value for scaling the text.
|||
|||	    sy
|||	        y value for scaling the text.
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
|||	    CreateTextState(), DrawText(), GetTextScale()
|||
**/

Err ScaleText(TextState *ts, gfloat sx, gfloat sy)
{
    FontMatrix fm;
    Err err;

    CheckTextState(ts);

        /* Scale by calling TransformText().
         *
         * The translation matrix to scale about an arbitrary point is:
         *
         *             scale.x                         0
         *               0                            scale.y
         *  (1 - scale.x) * center.x   (1 - scale.y) * center.y
         */

    fm[0][0] = sx;    fm[0][1] = 0;
    fm[1][0] = 0;     fm[1][1] = sy;
    fm[2][0] = ((1 - sx) * ts->ts_BaselineLeft.x);
    fm[2][1] = ((1 - sy) * ts->ts_BaselineLeft.y);

    err = TransformText(ts, fm);

    return(err);
}


/**
|||	AUTODOC -public -class Font -name GetTextScale
|||	Gets the current scale value of the text in the TextState
|||
|||	  Synopsis
|||
|||	    Err GetTextScale(const TextState *ts, gfloat *sx, gfloat *sy);
|||
|||	  Description
|||
|||	    Puts the current scale factor (relative to the size of the
|||	    original text) in the sx and sy variables.
|||
|||	  Arguments
|||
|||	    ts
|||	        Pointer to the TextState returned by
|||	        CreateTextState().
|||
|||	    sx
|||	        Pointer to a gfloat that will be filled with
|||	        the Text's current x scale.
|||
|||	    sy
|||	        Pointer to a gfloat that will be filled with
|||	        the Text's current y scale.
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
|||	    CreateTextState(), ScaleText()
|||
**/

#define TL 0
#define TR 1
#define BL 2
#define POINTS 3
Err GetTextScale(const TextState *ts, gfloat *sx, gfloat *sy)
{
    gfloat angle;
    gfloat s, c;
    gfloat currentx, currenty;
    gfloat newx, newy;
    Point2 pt[POINTS];
    uint32 i;

    CheckTextState(ts);

        /* Get the current angle, "rotate" the text back to 0 degrees, and calculate
         * the scale factor from there.
         */
    GetTextAngle(ts, &angle);
    pt[TL] = ts->ts_TopLeft;
    pt[TR] = ts->ts_TopRight;
    pt[BL] = ts->ts_BaselineLeft;
    if (angle != 0.0)
    {
        s = sinf(-angle * (PI / 180.0));
        c = cosf(-angle * (PI / 180.0));

        for (i = 0; i < POINTS; i++)
        {
            currentx = pt[i].x;
            currenty = pt[i].y;
            currentx -= ts->ts_BaselineLeft.x;
            currenty -= ts->ts_BaselineLeft.y;
            newx = ((currentx * c) + (currenty * s));
            newy = ((currenty * c) - (currentx * s));
            newx += ts->ts_BaselineLeft.x;
            newy += ts->ts_BaselineLeft.y;
            pt[i].x = newx;
            pt[i].y = newy;
        }
    }

    if (sx)
    {
        *sx = ((pt[TR].x - pt[TL].x) / ts->ts_BaselineOrigLen.x);
    }

    if (sy)
    {
        *sy = ((pt[BL].y - pt[TL].y) / ts->ts_BaselineOrigLen.y);
    }

    return(0);
}

