/* @(#) rotatetext.c 96/07/09 1.10 */

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

/**
|||	AUTODOC -public -class Font -name RotateText
|||	Rotates the text in a TextState
|||
|||	  Synopsis
|||
|||	    Err RotateText(TextState *ts, const gfloat angle,
|||	                   const gfloat x, const gfloat y);
|||
|||	  Description
|||
|||	    Rotates all the text in the TextState by the specified
|||	    angle, with the center of rotation at the specified point.
|||
|||	    After calling RotateText(), you need to call DrawText() to
|||	    link the new Triangle Engine instructions into the GState,
|||	    followed by GS_SendList() and GS_WaitIO().
|||
|||	  Arguments
|||
|||	    ts
|||	        Pointer to the TextState returned by
|||	        CreateTextState().
|||
|||	    angle
|||	        Amount to rotate the text by (in degrees)
|||
|||	    x, y
|||	        Point to rotate the text about.
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
|||	    CreateTextState(), DrawText(), GetTextAngle()
|||
**/

Err RotateText(TextState *ts, const gfloat angle, const gfloat x, const gfloat y)
{
    FontMatrix fm;
    gfloat s, c;    /* sine and cosine */
    Err err;

    CheckTextState(ts);

        /* Rotate by calling TransformText().
         *
         * The translation matrix to rotate about an arbitrary point is:
         *
         *                  cos                                           sin
         *                  - sin                                         cos
         * (1 - cos)*center.x + sin * center.y    (1 - cos)*center.y - sin * center.x
         */

    s = sinf(angle * (PI / 180.0));
    c = cosf(angle * (PI / 180.0));

    fm[0][0] = c;    fm[0][1] = s;
    fm[1][0] = -s;  fm[1][1] = c;
    fm[2][0] = (((1 - c) * x) + (s * y));
    fm[2][1] = (((1 - c) * y) - (s * x));

    err = TransformText(ts, fm);

    return(err);
}




/**
|||	AUTODOC -public -class Font -name GetTextAngle
|||	Gets the current angle of the text in the TextState
|||
|||	  Synopsis
|||
|||	    Err GetTextAngle(const TextState *ts, gfloat *angle);
|||
|||	  Description
|||
|||	    Puts the current angle of rotation in angle.
|||
|||	  Arguments
|||
|||	    ts
|||	        Pointer to the TextState returned by
|||	        CreateTextState().
|||
|||	    angle
|||	        Pointer to a gfloat to be filled with the current
|||	        angle of rotation.
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
|||	    CreateTextState(), RotateText()
|||
**/

Err GetTextAngle(const TextState *ts, gfloat *angle)
{
    gfloat ang;

    CheckTextState(ts);

    /* Quick shortcut here for 0 degrees */
    if (ts->ts_BaselineRight.y == ts->ts_BaselineLeft.y)
    {
        *angle = 0.0;
        return(0);
    }

    ang = (atanf((ts->ts_BaselineRight.y - ts->ts_BaselineLeft.y) /
                 (ts->ts_BaselineRight.x - ts->ts_BaselineLeft.x)) / (PI / 180.0));

        /* Angle should go from 0 at 3 O'Clock, anti-clockwise. */
    if (ts->ts_BaselineRight.x >= ts->ts_BaselineLeft.x)
    {
        if (ts->ts_BaselineRight.y > ts->ts_BaselineLeft.y)
        {
            /* 270 - 360 degrees at 6 O'Clock - 3 O'Clock */
            ang = 360.0 - ang;
        }
        else
        {
            /* 0 - 90 degrees at 3 O'Clock - 12 O'Clock */
            ang = -ang;
        }
    }
    else
    {
        if (ts->ts_BaselineRight.y >= ts->ts_BaselineLeft.y)
        {
            /* 180 - 270 degrees at 9 O'Clock - 6 O'Clock */
            ang = 180.0 - ang;
        }
        else
        {
            /* 90 - 180 degrees at 12 O'Clock - 9 O'Clock */
            ang = 180 - ang;
        }
    }

    *angle = ang;
    return(0);
}
