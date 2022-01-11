/*
 *	@(#) rect2d.c 96/02/16 1.10
 *
 * 2D Framework for M2 graphics
 *
 */

/*
 * 2D rectangle fill routines
 */

#include "frame2.i"

#define DBUGX(x) /* printf x */


/*
 * rect2d.c Stephen H. Landrum, 1995
 */


/**
|||	AUTODOC -public -class frame2d -name F2_FillRect
|||	Draw a filled rectangle on the display
|||
|||	  Synopsis
|||
|||	    void F2_FillRect (GState *gs, gfloat x1, gfloat y1, gfloat x2, gfloat y2, Color4 *c);
|||
|||	  Description
|||
|||	    Place commands in the current command list to draw a rectangle
|||	    filled with the specified color on the display.  The filled 
|||	    rectangle will include the leftmost and topmost lines, but will
|||	    not include the rightmost and bottommost lines.  The commands 
|||	    added to the triangle engine command list alter the state of the 
|||	    texture and destination blend attribute settings.
|||
|||	  Arguments
|||
|||	    gs
|||	        Pointer to the GState (graphics state) structure
|||
|||	    x1, y1, x2, y2
|||	        Coordinates of the corners of the rectangle to fill
|||
|||	    c
|||	        Color used to fill the rectangle
|||
|||
|||	  Return Value
|||
|||	    This routine will place commands into the current command list
|||	    to fill the rectangle, and has no other status return.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/frame2d.h>
|||
|||	  See Also
|||
|||	    F2_DrawLine(), F2_Point()
|||
**/
void
F2_FillRect (GState *gs, gfloat x1, gfloat y1, gfloat x2, gfloat y2, 
	     const Color4 *c)
{
  Bitmap* bm = (Bitmap*)LookupItem(GS_GetDestBuffer(gs));
  gfloat width, height;
  
  width = (gfloat)bm->bm_Width;
  height = (gfloat)bm->bm_Height;

  if (x1<0.) x1 = 0.;
  if (x1>width) x1 = width;
  if (y1<0.) y1 = 0.;
  if (y1>height) y1 = height;
  if (x2<0.) x2 = 0.;
  if (x2>width) x2 = width;
  if (y2<0.) y2 = 0.;
  if (y2>height) y2 = height;

  _set_notexture (gs);
  if (c->a<1.) {
    _enable_alpha (gs);
  } else {
    _disable_alpha (gs);
  }

  GS_Reserve (gs, 7*4+1);
  CLT_TRIANGLE (GS_Ptr(gs), 1, RC_FAN, 1, 0, 1, 4);
  CLT_VertexRgbaW (GS_Ptr(gs), x1, y1, c->r, c->g, c->b, c->a, .999998);
  CLT_VertexRgbaW (GS_Ptr(gs), x1, y2, c->r, c->g, c->b, c->a, .999998);
  CLT_VertexRgbaW (GS_Ptr(gs), x2, y2, c->r, c->g, c->b, c->a, .999998);
  CLT_VertexRgbaW (GS_Ptr(gs), x2, y1, c->r, c->g, c->b, c->a, .999998);
}

