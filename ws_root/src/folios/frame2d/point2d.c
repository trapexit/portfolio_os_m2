/*
 *	@(#) point2d.c 96/02/16 1.9
 *
 * 2D Framework for M2 graphics
 *
 */

/*
 * 2D point routines
 */

#include "frame2.i"

#define DBUGX(x) /* printf x */


/*
 * point2d.c Stephen H. Landrum, 1995
 */


/**
|||	AUTODOC -public -class frame2d -name F2_Point
|||	Draw a single pixel
|||
|||	  Synopsis
|||
|||	    void F2_Point (GState *gs, gfloat x, gfloat y, Color4 *c)
|||
|||	  Description
|||
|||	    Place commands in the current command list to draw a point
|||	    on the display in the specified color.  The commands added to
|||	    the triangle engine command list alter the state of the texture 
|||	    and destination blend attribute settings.
|||
|||	  Arguments
|||
|||	    gs
|||	        Pointer to the GState (graphics state) structure
|||
|||	    x, y
|||	        Location on the display of the point to be drawn
|||
|||	    c
|||	        Color to draw the point
|||
|||
|||	  Return Value
|||
|||	    This routine will place commands into the current command list
|||	    to draw the point, and has no other status return.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/frame2d.h>
|||
|||	  See Also
|||
|||	    F2_FillRect(), F2_Points(), F2_DrawLine()
|||
**/
void
F2_Point (GState *gs, const gfloat x, const gfloat y, const Color4 *c)
{
  Bitmap* bm = (Bitmap*)LookupItem(GS_GetDestBuffer(gs));
  gfloat width, height;
  
  width = (gfloat)bm->bm_Width;
  height = (gfloat)bm->bm_Height;

  if (x<0. || y<0. || x>width || y>height) return;

  _set_notexture (gs);

  GS_Reserve (gs, 3*7+1);
  CLT_TRIANGLE (GS_Ptr(gs), 1, RC_FAN, 1, 0, 1, 3);
  CLT_VertexRgbaW (GS_Ptr(gs), x, y, c->r, c->g, c->b, c->a, .999998);
  CLT_VertexRgbaW (GS_Ptr(gs), x+1, y, c->r, c->g, c->b, c->a, .999998);
  CLT_VertexRgbaW (GS_Ptr(gs), x, y+1, c->r, c->g, c->b, c->a, .999998);
}


/**
|||	AUTODOC -public -class frame2d -name F2_Points
|||	Draw colored pixels
|||
|||	  Synopsis
|||
|||	    void F2_Points (GState *gs, int32 numpoints, Point2 *p, Color4 *c)
|||
|||	  Description
|||
|||	    Place commands in the current command list to draw points
|||	    on the display in the specified colors.  The commands added to
|||	    the triangle engine command list alter the state of the texture 
|||	    and destination blend attribute settings.
|||
|||	  Arguments
|||
|||	    gs
|||	        Pointer to the GState (graphics state) structure
|||
|||	    numpoints
|||	        Number of points to draw
|||
|||	    p
|||	        Array of locations on the display of the points to be drawn
|||
|||	    c
|||	        Array of colors to draw the points
|||
|||
|||	  Return Value
|||
|||	    This routine will place commands into the current command list
|||	    to draw the points, and has no other status return.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/frame2d.h>
|||
|||	  See Also
|||
|||	    F2_FillRect(), F2_Point(), F2_DrawLine()
|||
**/
void
F2_Points (GState *gs, int32 numpoints, const Point2 *p, const Color4 *c)
{
  Bitmap* bm = LookupItem(GS_GetDestBuffer(gs));
  gfloat width, height, x, y;

  width = (gfloat)bm->bm_Width;
  height = (gfloat)bm->bm_Height;

  _set_notexture (gs);

  while (numpoints-- >= 0) {
    x = p->x;
    y = p->y;
    if (x<0. || y<0. || x>width || y>height) goto skip;
    GS_Reserve (gs, 3*7+1);
    CLT_TRIANGLE (GS_Ptr(gs), 1, RC_FAN, 1, 0, 1, 3);
    CLT_VertexRgbaW (GS_Ptr(gs), x, y, c->r, c->g, c->b, c->a, .999998);
    CLT_VertexRgbaW (GS_Ptr(gs), x+1, y, c->r, c->g, c->b, c->a, .999998);
    CLT_VertexRgbaW (GS_Ptr(gs), x, y+1, c->r, c->g, c->b, c->a, .999998);
  skip:
    p++;
    c++;
  }
}
