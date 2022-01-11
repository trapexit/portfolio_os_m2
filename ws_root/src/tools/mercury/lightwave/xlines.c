#include <math.h>

#define C_EPS 1.0e-7
#define FP_EQUAL(s, t) (fabs(s - t) <= C_EPS)

/* lines_intersect:  AUTHOR: Mukesh Prasad
 *
 *   This function computes whether two line segments,
 *   respectively joining the input points (x1,y1) -- (x2,y2)
 *   and the input points (x3,y3) -- (x4,y4) intersect.
 *   If the lines intersect, the output variables x, y are
 *   set to coordinates of the point of intersection.
 *
 *   All values are in integers.  The returned value is rounded
 *   to the nearest integer point.
 *
 *   If non-integral grid points are relevant, the function
 *   can easily be transformed by substituting floating point
 *   calculations instead of integer calculations.
 *
 *   Entry
 *        x1, y1,  x2, y2   Coordinates of endpoints of one segment.
 *        x3, y3,  x4, y4   Coordinates of endpoints of other segment.
 *
 *   Exit
 *        x, y              Coordinates of intersection point.
 *
 *   The value returned by the function is one of:
 *
 *        DONT_INTERSECT    0
 *        DO_INTERSECT      1
 *        COLLINEAR         2
 *
 * Error conditions:
 *
 *     Depending upon the possible ranges, and particularly on 16-bit
 *     computers, care should be taken to protect from overflow.
 *
 *     In the following code, 'long' values have been used for this
 *     purpose, instead of 'int'.
 *
 */

#define	DONT_INTERSECT    0
#define	DO_INTERSECT      1
#define COLLINEAR         2

#define SAME_SIGNS( a, b )	\
( (a*b)>=0.0 ? 1 : 0 )

int lines_intersect(double x1, double y1, double x2, double y2,
		    double x3, double y3, double x4, double y4,
		    double *x, double *y);

int lines_intersect(double x1, double y1, /* First line segment */
		    double x2, double y2,
		    double x3, double y3,  /* Second line segment */
		    double x4, double y4,
		    double *x, double *y   /* Output value:
					      point of intersection */
		    )
{
    double a1, a2, b1, b2, c1, c2; /* Coefficients of line eqns. */
    double r1, r2, r3, r4;         /* 'Sign' values */
    double denom, offset, num;     /* Intermediate values */

    double minX1, minY1, minX2, minY2;
    double maxX1, maxY1, maxX2, maxY2;


    /* Compute a1, b1, c1, where line joining points 1 and 2
     * is "a1 x  +  b1 y  +  c1  =  0".
     */

    a1 = y2 - y1;
    b1 = x1 - x2;
    c1 = x2 * y1 - x1 * y2;

    /* Compute r3 and r4.
     */


    r3 = a1 * x3 + b1 * y3 + c1;
    r4 = a1 * x4 + b1 * y4 + c1;

    /* Check signs of r3 and r4.  If both point 3 and point 4 lie on
     * same side of line 1, the line segments do not intersect.
     */

    if ( (!(FP_EQUAL(r3,0.0))) &&
         (!(FP_EQUAL(r4,0.0))) &&
         SAME_SIGNS( r3, r4 ))
        return ( DONT_INTERSECT );

    /* Compute a2, b2, c2 */

    a2 = y4 - y3;
    b2 = x3 - x4;
    c2 = x4 * y3 - x3 * y4;

    /* Compute r1 and r2 */

    r1 = a2 * x1 + b2 * y1 + c2;
    r2 = a2 * x2 + b2 * y2 + c2;

    /* Check signs of r1 and r2.  If both point 1 and point 2 lie
     * on same side of second line segment, the line segments do
     * not intersect.
     */

    if ( (!(FP_EQUAL(r1,0.0))) &&
         (!(FP_EQUAL(r2,0.0))) &&
         SAME_SIGNS( r1, r2 ))
        return ( DONT_INTERSECT );

    /* Line segments intersect: compute intersection point. 
     */

    denom = a1 * b2 - a2 * b1;
    if ( denom == 0 )
      {
	if (x1<x2)
	  {
	    minX1 = x1;
	    maxX1 = x2;
	  }
	else
	  {
	    minX1 = x2;
	    maxX1 = x1;
	  }
	if (x3<x4)
	  {
	    minX2 = x3;
	    maxX2 = x4;
	  }
	else
	  {
	    minX2 = x4;
	    maxX2 = x3;
	 }
	if (y1<y2)
	  {
	    minY1 = y1;
	    maxY1 = y2;
	  }
	else
	 {
	   minY1 = y2;
	   maxY1 = y1;
	 }
	if (y3<y4)
	  {
	    minY2 = y3;
	    maxY2 = y4;
	  }
	else
	  {
	    minY2 = y4;
	    maxY2 = y3;
	  }
	
	if (maxY1 < minY2)
	  return(DONT_INTERSECT);
	else if (maxY2 < minY1)
	 return(DONT_INTERSECT);
	else if (maxX1 < minX2)
	  return(DONT_INTERSECT);
	else if (maxX2 < minY1)
	  return(DONT_INTERSECT);
	else
	  return ( COLLINEAR );
      }
    
    
    offset = (denom < 0) ? - denom / 2 : denom / 2;
    
    /* The denom/2 is to get rounding instead of truncating.  It
     * is added or subtracted to the numerator, depending upon the
     * sign of the numerator.
     */

    num = b1 * c2 - b2 * c1;
    *x = ( num < 0 ? num - offset : num + offset ) / denom;

    num = a2 * c1 - a1 * c2;
    *y = ( num < 0 ? num - offset : num + offset ) / denom;

    return ( DO_INTERSECT );
    } /* lines_intersect */

/* A main program to test the function.
 */




