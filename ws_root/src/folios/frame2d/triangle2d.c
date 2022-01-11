/*
 *	@(#) triangle2d.c 96/03/15 1.16
 *
 * 2D Framework for M2 graphics
 *
 */

/*
 * 2D triangle routines
 */

#include "frame2.i"

#define DBUGX(x) /* printf x */


/*
 * triangle2d.c Stephen H. Landrum, 1995
 */


static Err
_tricore (GState *gs, int32 style, int32 numpoints, 
	  Point2 *p, Color4 *c, TexCoord *tx, int32 fanstrip)
{
  DBUGX (("Tricore - style=%d, npts=%d, fanstrip=%d\n",
	  style, numpoints, fanstrip));
  if (numpoints<=0) return 0;

  {
    int32 i;
    Bitmap* bm = (Bitmap*)LookupItem(GS_GetDestBuffer(gs));
    gfloat w = (gfloat)bm->bm_Width;
    gfloat h = (gfloat)bm->bm_Height;
    for (i=0; i<numpoints; i++) {
      if (p[i].x<0. || p[i].x>w || p[i].y<0. || p[i].y>h) goto clipped;
    }
  }

  DBUGX (("Unclipped triangles\n"));
  switch (style) {
  case 0:
    GS_Reserve (gs, numpoints*3+1);
    CLT_TRIANGLE (GS_Ptr(gs), 1, fanstrip, 1, 0, 0, numpoints);
    while (numpoints--) {
      CLT_VertexW (GS_Ptr(gs), p->x, p->y, .999998);
      p++;
    }
    return 0;
  case GEOF2_COLORS:
    GS_Reserve (gs, numpoints*7+1);
    CLT_TRIANGLE (GS_Ptr(gs), 1, fanstrip, 1, 0, 1, numpoints);
    while (numpoints--) {
      CLT_VertexRgbaW (GS_Ptr(gs), p->x, p->y, c->r, c->g, c->b, c->a, 
		       .999998);
      p++;
      c++;
    }
    return 0;
  case GEOF2_TEXCOORDS:
    GS_Reserve (gs, numpoints*5+1);
    CLT_TRIANGLE (GS_Ptr(gs), 1, fanstrip, 1, 1, 0, numpoints);
    while (numpoints--) {
      CLT_VertexUvW (GS_Ptr(gs), p->x, p->y, tx->u, tx->v, .999998);
      p++;
      tx++;
    }
    return 0;
  case GEOF2_COLORS+GEOF2_TEXCOORDS:
    GS_Reserve (gs, numpoints*9+1);
    CLT_TRIANGLE (GS_Ptr(gs), 1, fanstrip, 1, 1, 1, numpoints);
    while (numpoints--) {
      CLT_VertexRgbaUvW (GS_Ptr(gs), p->x, p->y, c->r, c->g, c->b, c->a, 
			 tx->u, tx->v, .999998);
      p++;
      c++;
      tx++;
    }
    return 0;
  default:
    return -1;
  }


clipped:
  DBUGX (("Clipped\n"));
  {
    Point2 *oldp, *midp, *newp;
    Color4 *oldc=0, *midc=0, *newc=0;
    TexCoord *oldt=0, *midt=0, *newt=0;
    _geouv geo[3];

    memset (geo, 0, sizeof(geo));
    oldp = p++;
    midp = p++;
    if (style&GEOF2_COLORS) {
      oldc = c++;
      midc = c++;
    }
    if (style&GEOF2_TEXCOORDS) {
      oldt = tx++;
      midt = tx++;
    }
    numpoints -= 2;

    geo[0].w = .999998;
    geo[1].w = .999998;
    geo[2].w = .999998;
    while (numpoints--) {
      newp = p++;
      geo[0].x = oldp->x;
      geo[0].y = oldp->y;
      geo[1].x = midp->x;
      geo[1].y = midp->y;
      geo[2].x = newp->x;
      geo[2].y = newp->y;
      if (style&GEOF2_TEXCOORDS) {
	newt = tx++;
	geo[0].u = oldt->u;
	geo[0].v = oldt->v;
	geo[1].u = midt->u;
	geo[1].v = midt->v;
	geo[2].u = newt->u;
	geo[2].v = newt->v;
      }
      if (style&GEOF2_COLORS) {
	newc = c++;
	geo[0].r = oldc->r;
	geo[0].g = oldc->g;
	geo[0].b = oldc->b;
	geo[0].a = oldc->a;
	geo[1].r = midc->r;
	geo[1].g = midc->g;
	geo[1].b = midc->b;
	geo[1].a = midc->a;
	geo[2].r = newc->r;
	geo[2].g = newc->g;
	geo[2].b = newc->b;
	geo[2].a = newc->a;
      }
      
      _clip_fan2d (gs, NULL, style, 3, geo, FALSE);
      if (fanstrip==RC_STRIP) {
	TOUCH(midt);
	oldp = midp;
	oldc = midc;
	oldt = midt;
      }
      midp = newp;
      midc = newc;
      midt = newt;
    }
  }
  return 0;
}


/**
|||	AUTODOC -public -class frame2d -name F2_TriFan
|||	Draw a 2D triangle fan
|||
|||	  Synopsis
|||
|||	    Err F2_TriFan (GState *gs, int32 style, int32 numpoints, Point2 *p, Color4 *c, TexCoord *tx)
|||
|||	  Description
|||
|||	    Place commands in the current command list to draw a 2D
|||	    triangle fan on the display.  The triangle fan can optionally
|||	    include colors and/or texture coordinates.  No modification
|||	    of the texture or destination blend attributes is done, so
|||	    any desired setup must be done previously with a sprite
|||	    object or with the CLT macros.
|||
|||	  Arguments
|||
|||	    gs
|||	        Pointer to the GState (graphics state) structure
|||
|||	    style
|||	        Bits in the style setting select whether the triangle
|||	        includes colors or texture coordinates at the vertices.
|||	        Use GEOF2_COLORS for colors, and GEOF2_TEXCOORDS to
|||	        include texture coordinates.
|||
|||	    numpoints
|||	        Number of vertices (numpoints-2 triangles will be drawn).
|||
|||	    p
|||	        Array of vertices for the corners of the triangles
|||
|||	    c
|||	        Array of colors to use at the corners
|||
|||	    tx
|||	        Array of texture coordinates to use at the corners
|||
|||
|||	  Return Value
|||
|||	    This routine will place commands into the current command list
|||	    to draw the triangles.  A negative error return indicates an
|||	    error in the passed parameters.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/frame2d.h>
|||
|||	  See Also
|||
|||	    F2_TriStrip(), F2_Triangles()
|||
**/
Err
F2_TriFan (GState *gs, const int32 style, int32 numpoints, 
	   const Point2 *p, const Color4 *c, const TexCoord *tx)
{
  return _tricore(gs, style, numpoints, p, c, tx, RC_FAN);
}


/**
|||	AUTODOC -public -class frame2d -name F2_TriStrip
|||	Draw a 2D triangle strip
|||
|||	  Synopsis
|||
|||	    Err F2_TriStrip (GState *gs, int32 style, int32 numpoints, Point2 *p, Color4 *c, TexCoord *tx)
|||
|||	  Description
|||
|||	    Place commands in the current command list to draw a 2D
|||	    triangle strip on the display.  The triangles can optionally
|||	    include colors and/or texture coordinates.  No modification
|||	    of the texture or destination blend attributes is done, so
|||	    any desired setup must be done previously with a sprite
|||	    object or with the CLT macros.
|||
|||	  Arguments
|||
|||	    gs
|||	        Pointer to the GState (graphics state) structure
|||
|||	    style
|||	        Bits in the style setting select whether the triangle
|||	        includes colors or texture coordinates at the vertices.
|||	        Use GEOF2_COLORS for colors, and GEOF2_TEXCOORDS to
|||	        include texture coordinates.
|||
|||	    numpoints
|||	        Number of vertices (numpoints-2 triangles will be drawn).
|||
|||	    p
|||	        Array of vertices for the corners of the triangles
|||
|||	    c
|||	        Array of colors to use at the corners
|||
|||	    tx
|||	        Array of texture coordinates to use at the corners
|||
|||
|||	  Return Value
|||
|||	    This routine will place commands into the current command list
|||	    to draw the triangles.  A negative error return indicates an
|||	    error in the passed parameters.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/frame2d.h>
|||
|||	  See Also
|||
|||	    F2_TriFan(), F2_Triangles()
|||
**/
Err
F2_TriStrip (GState *gs, const int32 style, int32 numpoints, 
	     const Point2 *p, const Color4 *c, const TexCoord *tx)
{
  return _tricore(gs, style, numpoints, p, c, tx, RC_STRIP);
}


/**
|||	AUTODOC -public -class frame2d -name F2_Triangles
|||	Draw 2D triangles 
|||
|||	  Synopsis
|||
|||	    Err F2_Triangles (GState *gs, int32 style, int32 numpoints, Point2 *p, Color4 *c, TexCoord *tx)
|||
|||	  Description
|||
|||	    Place commands in the current command list to draw disjoint 2D
|||	    triangles on the display.  The triangles can optionally
|||	    include colors and/or texture coordinates.  No modification
|||	    of the texture or destination blend attributes is done, so
|||	    any desired setup must be done previously with a sprite
|||	    object or with the CLT macros.
|||
|||	  Arguments
|||
|||	    gs
|||	        Pointer to the GState (graphics state) structure
|||
|||	    style
|||	        Bits in the style setting select whether the triangle
|||	        includes colors or texture coordinates at the vertices.
|||	        Use GEOF2_COLORS for colors, and GEOF2_TEXCOORDS to
|||	        include texture coordinates.
|||
|||	    numpoints
|||	        Number of vertices (numpoints/3 triangles will be drawn).
|||
|||	    p
|||	        Array of vertices for the corners of the triangles
|||
|||	    c
|||	        Array of colors to use at the corners
|||
|||	    tx
|||	        Array of texture coordinates to use at the corners
|||
|||
|||	  Return Value
|||
|||	    This routine will place commands into the current command list
|||	    to draw the triangles.  A negative error return indicates an
|||	    error in the passed parameters.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/frame2d.h>
|||
|||	  See Also
|||
|||	    F2_TriFan(), F2_TriStrip()
|||
**/
Err
F2_Triangles (GState *gs, const int32 style, int32 numpoints, 
	      const Point2 *p, const Color4 *c, const TexCoord *tx)
{
  if (numpoints<=0) return 0;

  {
    int32 i;
    Bitmap* bm = (Bitmap*)LookupItem(GS_GetDestBuffer(gs));
    gfloat w = (gfloat)bm->bm_Width;
    gfloat h = (gfloat)bm->bm_Height;
    for (i=0; i<numpoints; i++) {
      if (p[i].x<0. || p[i].x>w || p[i].y<0. || p[i].y>h) goto clipped;
    }
  }

  switch (style) {
  case 0:
    GS_Reserve (gs, numpoints/3*10);
    while (numpoints>2) {
      CLT_TRIANGLE (GS_Ptr(gs), 1, RC_FAN, 1, 0, 0, 3);
      CLT_VertexW (GS_Ptr(gs), p->x, p->y, .999998);
      p++;
      CLT_VertexW (GS_Ptr(gs), p->x, p->y, .999998);
      p++;
      CLT_VertexW (GS_Ptr(gs), p->x, p->y, .999998);
      p++;
      numpoints -= 3;
    }
    return 0;
  case GEOF2_COLORS:
    GS_Reserve (gs, numpoints/3*22);
    while (numpoints>2) {
      CLT_TRIANGLE (GS_Ptr(gs), 1, RC_FAN, 1, 0, 1, 3);
      CLT_VertexRgbaW (GS_Ptr(gs), p->x, p->y, c->r, c->g, c->b, c->a, 
		       .999998);
      p++;
      c++;
      CLT_VertexRgbaW (GS_Ptr(gs), p->x, p->y, c->r, c->g, c->b, c->a, 
		       .999998);
      p++;
      c++;
      CLT_VertexRgbaW (GS_Ptr(gs), p->x, p->y, c->r, c->g, c->b, c->a, 
		       .999998);
      p++;
      c++;
      numpoints -=3;
    }
    return 0;
  case GEOF2_TEXCOORDS:
    GS_Reserve (gs, numpoints/3*16);
    while (numpoints>2) {
      CLT_TRIANGLE (GS_Ptr(gs), 1, RC_FAN, 1, 1, 0, 3);
      CLT_VertexUvW (GS_Ptr(gs), p->x, p->y, tx->u, tx->v, .999998);
      p++;
      c++;
      CLT_VertexUvW (GS_Ptr(gs), p->x, p->y, tx->u, tx->v, .999998);
      p++;
      c++;
      CLT_VertexUvW (GS_Ptr(gs), p->x, p->y, tx->u, tx->v, .999998);
      p++;
      c++;
      numpoints -=3;
    }
    return 0;
  case GEOF2_COLORS+GEOF2_TEXCOORDS:
    GS_Reserve (gs, numpoints/3*28);
    while (numpoints>2) {
      CLT_TRIANGLE (GS_Ptr(gs), 1, RC_FAN, 1, 1, 1, 3);
      CLT_VertexRgbaUvW (GS_Ptr(gs), p->x, p->y, c->r, c->g, c->b, c->a, 
		       tx->u, tx->v, .999998);
      p++;
      c++;
      CLT_VertexRgbaUvW (GS_Ptr(gs), p->x, p->y, c->r, c->g, c->b, c->a, 
		       tx->u, tx->v, .999998);
      p++;
      c++;
      CLT_VertexRgbaUvW (GS_Ptr(gs), p->x, p->y, c->r, c->g, c->b, c->a, 
		       tx->u, tx->v, .999998);
      p++;
      c++;
      numpoints -= 3;
    }
    return 0;
  default:
    return -1;
  }

clipped:
  {
    _geouv geo[3];
    memset (geo, 0, sizeof(geo));
    geo[0].w = .999998;
    geo[1].w = .999998;
    geo[2].w = .999998;

    while (numpoints>2) {
      geo[0].x = p->x;
      geo[0].y = p->y;
      p++;
      geo[1].x = p->x;
      geo[1].y = p->y;
      p++;
      geo[2].x = p->x;
      geo[2].y = p->y;
      p++;
      if (style&GEOF2_TEXCOORDS) {
	geo[0].u = tx->u;
	geo[0].v = tx->v;
	tx++;
	geo[1].u = tx->u;
	geo[1].v = tx->v;
	tx++;
	geo[2].u = tx->u;
	geo[2].v = tx->v;
	tx++;
      }
      if (style&GEOF2_COLORS) {
	geo[0].r = c->r;
	geo[0].g = c->g;
	geo[0].b = c->b;
	geo[0].a = c->a;
	c++;
	geo[1].r = c->r;
	geo[1].g = c->g;
	geo[1].b = c->b;
	geo[1].a = c->a;
	c++;
	geo[2].r = c->r;
	geo[2].g = c->g;
	geo[2].b = c->b;
	geo[2].a = c->a;
	c++;
      }
      _clip_fan2d (gs, NULL, style, 3, geo, FALSE);
      numpoints -= 3;
    }
  }
  return 0;
}
