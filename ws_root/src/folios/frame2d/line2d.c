/*
 *	@(#) line2d.c 96/02/16 1.14
 *
 * 2D Framework for M2 graphics
 *
 */

/*
 * 2D Line draw routines
 */

#include "frame2.i"

#define DBUGX(x) /* printf x */


/*
 * line2d.c Stephen H. Landrum, 1995
 *
 * I have taken a different approach than Adam.  I believe that the line drawn
 * from point A to point B should be identical to the line drawn from point
 * B to point A.  I also believe that the endpoints of the specified line should
 * always be drawn.
 *
 * A side effect is that a zero length line will always draw as a point.
 */




/* 
 * qfill draws a quadrilateral output by the create_line routine
 * color c0 will be attached points 1 and 4 of the quad, color c1
 * will be attached to points 2 and 3.  Shading will be enabled.
 */
static void
qfill( GState *gs, int32 x1, int32 y1, int32 x2, int32 y2, 
		int32 x3, int32 y3, int32 x4, int32 y4,
		Color4 *c0, Color4 *c1 )
{
  DBUGX (("%d,%d, %d,%d, %d,%d, %d,%d\n", x1,y1, x2,y2, x3,y3, x4,y4));

  GS_Reserve (gs, 7*4+1);
  CLT_TRIANGLE (GS_Ptr(gs), 1, RC_FAN, 1, 0, 1, 4);
  CLT_VertexRgbaW (GS_Ptr(gs), x1, y1, c0->r, c0->g, c0->b, c0->a, .999998);
  CLT_VertexRgbaW (GS_Ptr(gs), x2, y2, c0->r, c0->g, c0->b, c0->a, .999998);
  CLT_VertexRgbaW (GS_Ptr(gs), x3, y3, c1->r, c1->g, c1->b, c1->a, .999998);
  CLT_VertexRgbaW (GS_Ptr(gs), x4, y4, c1->r, c1->g, c1->b, c1->a, .999998);
}



/* *******************************************************
   The create_line routine is the heart of this program.
   Given an arbitrary set of endpoints, it figures out
   the best two triangles to draw based on the octant that 
   the line falls in, and passes the vertices to the
   qfill routine.

   Again, I've taken a slightly different approach than Adam
   had.  I number the quadrants as follows:

                \ 2 | 1 /
                 \  |  /
                  \ | /
                3  \|/  0
              ------*------
                4  /|\  7
                  / | \      
                 /  |  \
                / 5 | 6 \
   With coordinate (0,0) in the upper left corner, x 
   increasing right, and y increasing down.
   ******************************************************* */

static void
create_line(GState *gs, int32 x1, int32 y1, int32 x3, int32 y3, Color4 *c0, Color4 *c1)
{
  int32 deltax, deltay, quad=0;

  deltax = x3 - x1; deltay = y1 - y3;
  if (x3<x1) {
    quad ^= 3;
    deltax = -deltax;
  }
  if (y3>y1) {
    quad ^= 7;
    deltay = -deltay;
  }
  if (deltax<deltay) quad ^= 1;

  switch (quad) {
  case 0:
    if (y3<1) y3=1;	/* Temporary hack to prevent negative coordinate */
    qfill (gs,  x1, y1,  x1, y1+1,  x3+1, y3,  x3+1, y3-1,  c0, c1);
    break;
  case 1:
    if (x1<1) x1=1;	/* Temporary hack to prevent negative coordinate */
    qfill (gs,  x1-1, y1+1,  x1, y1+1,  x3+1, y3,  x3, y3,  c0, c1);
    break;
  case 2:
    qfill (gs,  x1, y1+1,  x1+1, y1+1,  x3+1, y3,  x3, y3,  c0, c1);
    break;
  case 3:
    if (y3<1) y3=1;	/* Temporary hack to prevent negative coordinate */
    qfill (gs,  x1+1, y1,  x1+1, y1+1,  x3, y3,  x3, y3-1,  c0, c1);
    break;
  case 4:
    if (y1<1) y1=1;	/* Temporary hack to prevent negative coordinate */
    qfill (gs,  x1+1, y1-1,  x1+1, y1,  x3, y3+1,  x3, y3,  c0, c1);
    break;
  case 5:
    if (x3<1) x3=1;	/* Temporary hack to prevent negative coordinate */
    qfill (gs,  x1, y1,  x1+1, y1,  x3, y3+1,  x3-1, y3+1,  c0, c1);
    break;
  case 6:
    qfill (gs,  x1, y1,  x1+1, y1,  x3+1, y3+1,  x3, y3+1,  c0, c1);
    break;
  case 7:
    if (y1<1) y1=1;	/* Temporary hack to prevent negative coordinate */
    qfill (gs,  x1, y1-1,  x1, y1,  x3+1, y3+1,  x3+1, y3,  c0, c1);
    break;
  }
}


static void
_linecore (GState *gs, Point2 *v0, Point2 *v1, Color4 *c0, Color4 *c1)
{
  gfloat x0,x1,y0,y1;
  gfloat w,h;
  gfloat ratio;
  Color4 c2, c3;
  Bitmap* bm = (Bitmap*)LookupItem(GS_GetDestBuffer(gs));

  x0 = v0->x;
  y0 = v0->y;
  x1 = v1->x;
  y1 = v1->y;

  w = (gfloat)bm->bm_Width;
  h = (gfloat)bm->bm_Height;
  if (x0<0. || x1<0. || y0<0. || y1<0. || x0>=w || x1>=w || y0>=h || y1>=h) {
    /* Clip the line to screen boundaries */
    if (x0<=0. && x1<=0.) return;
    if (x0>=w && x1>=w) return;
    if (x0<0.) {
      ratio = x1/(x1-x0);
      y0 = y1  -  (ratio * (y1-y0));
      c2.r = c1->r  -  (ratio * (c1->r-c0->r));
      c2.g = c1->g  -  (ratio * (c1->g-c0->g));
      c2.b = c1->b  -  (ratio * (c1->b-c0->b));
      c2.a = c1->a  -  (ratio * (c1->a-c0->a));
      c0 = &c2;
      x0 = 0.;
    }
    if (x1<0.) {
      ratio = x0/(x0-x1);
      y1 = y0  -  (ratio * (y0-y1));
      c3.r = c0->r  -  (ratio * (c0->r-c1->r));
      c3.g = c0->g  -  (ratio * (c0->g-c1->g));
      c3.b = c0->b  -  (ratio * (c0->b-c1->b));
      c3.a = c0->a  -  (ratio * (c0->a-c1->a));
      c1 = &c3;
      x1 = 0.;
    }
    if (x0>w) {
      ratio = (x1-w)/(x1-x0);
      y0 = y1  -  (ratio * (y1-y0));
      c2.r = c1->r  -  (ratio * (c1->r-c0->r));
      c2.g = c1->g  -  (ratio * (c1->g-c0->g));
      c2.b = c1->b  -  (ratio * (c1->b-c0->b));
      c2.a = c1->a  -  (ratio * (c1->a-c0->a));
      c0 = &c2;
      x0 = w;
    }
    if (x1>w) {
      ratio = (x0-w)/(x0-x1);
      y1 = y0  -  (ratio * (y0-y1));
      c3.r = c0->r  -  (ratio * (c0->r-c1->r));
      c3.g = c0->g  -  (ratio * (c0->g-c1->g));
      c3.b = c0->b  -  (ratio * (c0->b-c1->b));
      c3.a = c0->a  -  (ratio * (c0->a-c1->a));
      c1 = &c3;
      x1 = w;
    }

    if (y0<=0 && y1<=0) return;
    if (y0>=h && y1>=h) return;
    if (y0<0.) {      
      ratio = y1/(y1-y0);
      x0 = x1  -  (ratio * (x1-x0));
      c2.r = c1->r  -  (ratio * (c1->r-c0->r));
      c2.g = c1->g  -  (ratio * (c1->g-c0->g));
      c2.b = c1->b  -  (ratio * (c1->b-c0->b));
      c2.a = c1->a  -  (ratio * (c1->a-c0->a));
      c0 = &c2;
      y0 = 0.;
    }
    if (y1<0.) {
      ratio = y0/(y0-y1);
      x1 = x0  -  (ratio * (x0-x1));
      c3.r = c0->r  -  (ratio * (c0->r-c1->r));
      c3.g = c0->g  -  (ratio * (c0->g-c1->g));
      c3.b = c0->b  -  (ratio * (c0->b-c1->b));
      c3.a = c0->a  -  (ratio * (c0->a-c1->a));
      c1 = &c3;
      y1 = 0.;
    }
    if (y0>h) {
      ratio = (y1-h)/(y1-y0);
      x0 = x1  -  (ratio * (x1-x0));
      c2.r = c1->r  -  (ratio * (c1->r-c0->r));
      c2.g = c1->g  -  (ratio * (c1->g-c0->g));
      c2.b = c1->b  -  (ratio * (c1->b-c0->b));
      c2.a = c1->a  -  (ratio * (c1->a-c0->a));
      c0 = &c2;
      y0 = h;
    }
    if (y1>h) {
      ratio = (y0-h)/(y0-y1);
      x1 = x0  -  (ratio * (x0-x1));
      c3.r = c0->r  -  (ratio * (c0->r-c1->r));
      c3.g = c0->g  -  (ratio * (c0->g-c1->g));
      c3.b = c0->b  -  (ratio * (c0->b-c1->b));
      c3.a = c0->a  -  (ratio * (c0->a-c1->a));
      c1 = &c3;
      y1 = h;
    }
  }
  create_line(gs, (int32)x0, (int32)y0, (int32)x1, (int32)y1, c0, c1);
}



/**
|||	AUTODOC -public -class frame2d -name F2_DrawLine
|||	Draw a shaded line on the display
|||
|||	  Synopsis
|||
|||	    void F2_DrawLine(GState *gs, Point2 *v0, Point2 *v1, Color4 *c0, Color4 *c1)
|||
|||	  Description
|||
|||	    Place commands in the current command list to draw a shaded
|||	    line on the display.  The line drawn will include both of the
|||	    specified endpoints, and will be shaded from one endpoint to
|||	    the other using the colors specified.  The commands added to
|||	    the triangle engine command list alter the state of the texture 
|||	    and destination blend attribute settings.
|||
|||	  Arguments
|||
|||	    gs
|||	        Pointer to the GState (graphics state) structure
|||
|||	    v0, v1
|||	        Coordinates of the endpoints of the line to draw.
|||
|||	    c0, c1
|||	        Colors used to shade the line.  Color c0 is used at endpoint
|||	        v0, color c1 is used at endpoint v1.
|||
|||
|||	  Return Value
|||
|||	    This routine will place commands into the current command list
|||	    to draw the line, and has no other status return.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/frame2d.h>
|||
|||	  See Also
|||
|||	    F2_FillRect(), F2_Point(), F2_ColoredLines(), F2_ShadedLines()
|||
**/
void
F2_DrawLine (GState *gs, const Point2 *v0, const Point2 *v1, const Color4 *c0,
	     const Color4 *c1)
{
  _set_notexture (gs);
  _enable_alpha (gs);
  _linecore (gs, v0, v1, c0, c1);
}


/**
|||	AUTODOC -public -class frame2d -name F2_ColoredLines
|||	Draw colored lines on the display
|||
|||	  Synopsis
|||
|||	    void F2_ColoredLines (GState *gs, int32 numpoints, Point2 *points, Color4 *colors)
|||
|||	  Description
|||
|||	    Place commands in the current command list to draw connected 
|||	    colored lines on the display.  As line n is drawn, n ranging
|||	    from 0 to numpoints-2, the line will use points[n] and points[n+1]
|||	    and will use colors[n].  The commands added to the triangle engine
|||	    command list alter the state of the texture and destination blend 
|||	    attribute settings.
|||
|||	  Arguments
|||
|||	    gs
|||	        Pointer to the GState (graphics state) structure
|||
|||	    numpoints
|||	        Number of points to use.  numpoints-1 lines will be drawn.
|||
|||	    points
|||	        Array of coordinates of the endpoints of the line to draw.
|||
|||	    colors
|||	        Array of colors used for the lines
|||
|||
|||	  Return Value
|||
|||	    This routine will place commands into the current command list
|||	    to draw the lines, and has no other status return.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/frame2d.h>
|||
|||	  See Also
|||
|||	    F2_FillRect(), F2_Point(), F2_DrawLine(), F2_ShadedLines()
|||
**/
void 
F2_ColoredLines (GState *gs, int32 numpoints, const Point2 *points, 
		 const Color4 *colors)
{
  _set_notexture (gs);
  _enable_alpha (gs);
  while (--numpoints>0) {
    _linecore (gs, points, points+1, colors, colors);
    points++;
    colors++;
  }
}


/**
|||	AUTODOC -public -class frame2d -name F2_ShadedLines
|||	Draw shaded lines on the display
|||
|||	  Synopsis
|||
|||	    void F2_ShadedLines (GState *gs, int32 numpoints, Point2 *points, Color4 *colors)
|||
|||	  Description
|||
|||	    Place commands in the current command list to draw connected 
|||	    shaded lines on the display.  As line n is drawn, n ranging
|||	    from 0 to numpoints-2, the line will use points[n] and points[n+1]
|||	    and will shade from colors[n] to colors[n+1].  The commands added
|||	    to the triangle engine command list alter the state of the texture 
|||	    and destination blend attribute settings.
|||
|||	  Arguments
|||
|||	    gs
|||	        Pointer to the GState (graphics state) structure
|||
|||	    numpoints
|||	        Number of points to use.  numpoints-1 lines will be drawn.
|||
|||	    points
|||	        Array of coordinates of the endpoints of the line to draw.
|||
|||	    colors
|||	        Array of colors used for the lines
|||
|||
|||	  Return Value
|||
|||	    This routine will place commands into the current command list
|||	    to draw the lines, and has no other status return.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/frame2d.h>
|||
|||	  See Also
|||
|||	    F2_FillRect(), F2_Point(), F2_DrawLine(), F2_ColoredLines()
|||
**/
void 
F2_ShadedLines (GState *gs, int32 numpoints, const Point2 *points, 
		const Color4 *colors)
{
  _set_notexture (gs);
  _enable_alpha (gs);
  while (--numpoints>0) {
    _linecore (gs, points, points+1, colors, colors+1);
    points++;
    colors++;
  }
}

