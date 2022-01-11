/*
 *	@(#) copyrect.c 96/02/16 1.8
 *
 * 2D Framework for M2 graphics
 *
 */

/*
 * 2D rectangle copy routines
 */

#include "frame2.i"

#define DBUGX(x) /* printf x */


/*
 * copyrect.c Stephen H. Landrum, 1995
 */


/**
|||	AUTODOC -public -class frame2d -name F2_CopyRect
|||	Copy a rectangle from memory to the display
|||
|||	  Synopsis
|||
|||	    void F2_CopyRect (GState *gs, void *sptr, int32 width, int32 type, Point2 *src, Point2 *dest, Point2 *size)
|||
|||	  Description
|||
|||	    Place commands in the current command list to copy a rectangle
|||	    from memory to the display.  The commands generated will affect
|||	    destination blender settings, and also modify the source
|||	    buffer for blending operations.
|||
|||	  Arguments
|||
|||	    gs
|||	        Pointer to the GState (graphics state) structure
|||
|||	    sptr
|||	        Pointer to memory that contains the rectangle to be
|||	        copied.  
|||
|||	    width
|||	        Width (in pixels) of the region of memory to extract the
|||	        rectangle from (this is not the width of the rectangle
|||	        itself)
|||
|||	    type
|||	        Format of the source memory - must be one of:
|||	            BMTYPE_16 for 16 bit
|||	            BMTYPE_32 for 32 bit
|||
|||	    src
|||	        Upper left corner of the rectangle in the source
|||
|||	    dest
|||	        Location in the destination to place the upper left
|||	        corner of the rectangle
|||
|||	    size
|||	        Width and height of the rectanlge to copy
|||
|||
|||	  Return Value
|||
|||	    This routine will place commands into the current command list
|||	    to draw the rectangle, and has no other status return.
|||
|||	  Associated Files
|||
|||	    <graphics/frame2d/frame2d.h>
|||
|||	  See Also
|||
|||	    F2_FillRect()
|||
**/
void
F2_CopyRect (GState *gs, const void *sptr, const int32 width, const int32 type,
	     const Point2 *src, const Point2 *dest, const Point2 *size)
{
  int32 x, y;

  /* PASS1 silicon workaround: Add 16MB to base addr. */
  _set_srcpassthrough (gs);

  GS_Reserve (gs, 8+(3*4+1));
  CLT_WriteRegister ((*GS_Ptr(gs)), DBSRCBASEADDR, (uint32)sptr+0x01000000);
  CLT_DBSRCXSTRIDE (GS_Ptr(gs), width);
  x = (int32)(src->x-dest->x);
  y = (int32)(src->y-dest->y);
  CLT_DBSRCOFFSET (GS_Ptr(gs), x, y); 
  CLT_DBSRCCNTL(GS_Ptr(gs), 1, (type==BMTYPE_32));

  CLT_TRIANGLE (GS_Ptr(gs), 1, RC_FAN, 1, 0, 0, 4);
  CLT_VertexW (GS_Ptr(gs), dest->x, dest->y, .999998);
  CLT_VertexW (GS_Ptr(gs), dest->x+size->x, dest->y, .999998);
  CLT_VertexW (GS_Ptr(gs), dest->x+size->x, dest->y+size->y, .999998);
  CLT_VertexW (GS_Ptr(gs), dest->x, dest->y+size->y, .999998);
}


