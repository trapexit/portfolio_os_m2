#define EXTERN 
#include "basic.h"
#include "math.h"
#include "string.h"

int monotonate_trapezoids(int n);
int triangulate_monotone_polygons(int nmonpoly, int op[][3]);
int generate_random_ordering(int n);
int triangulate_polygon(int n, double vertices[][2], int triangles[][3]);
int construct_trapezoids(int nseg, segment_t *seg);
int locate_endpoint(point_t *v, point_t *vo, int r);
int _greater_than_equal_to(point_t *v0, point_t *v1);

extern int CommentLevel;
static int initialise(nseg)
     int nseg;
{
  register int i;

  for (i = 1; i <= nseg; i++)
    seg[i].is_inserted = FALSE;
  generate_random_ordering(nseg);
  
  return 0;
}

int lines_intersect(double x1, double y1, double x2, double y2,
		    double x3, double y3, double x4, double y4,
		    double *x, double *y);
			
			
void triangulate_init()
{
	seg = (segment_t *)calloc(SEGSIZE, sizeof(segment_t));
	tr  = (trap_t *)calloc(TRSIZE, sizeof(trap_t));
	qs  = (node_t *)calloc(QSIZE, sizeof(node_t));
	mchain = (monchain_t *)calloc(TRSIZE, sizeof(monchain_t));
	vert =  (vertexchain_t *)calloc(SEGSIZE, sizeof(vertexchain_t));
	mon =  (int *)calloc(SEGSIZE, sizeof(int));
	visited = (int *)calloc(TRSIZE, sizeof(int));
	permute = (int *)calloc(SEGSIZE, sizeof(int));
}


/* The points constituting the polygon are specified in anticlockwise
 * order. If there are n points in the polygon, i/p would be the
 * points p0, p1....p(n) (where p0 and pn are the same point). The
 * output is contained in the array "triangles".
 * Every triangle is output in anticlockwise order and the 3
 * integers are the indices of the points. Thus, the triangle (i, j, k)
 * refers to the triangle formed by the points (pi, pj, pk). Before
 * using this routine, please check-out that you do not conflict with
 * the global variables  defined in basic.h.
 *
 * n:         number of points in polygon (p0 = pn)
 * vertices:  the vertices p0, p1..., p(n) of the polygon
 * triangles: output array containing the triangles 
 */

int triangulate_polygon(n, vertices, triangles)
     int n;
     double vertices[][2];
     int triangles[][3];
{
  register int i;
  int j, back, forward;
  int nmonpoly;
  int intersection;
  double x, y;

  double v0[2], v1[2];

  if (n > (SEGSIZE-100))
    {
      fprintf(stderr,"WARNING:Poly too damn big!!\n");
      return(FALSE);
    }
  memset((void *)seg, 0, SEGSIZE*sizeof(segment_t));
  memset((void *)tr, 0, TRSIZE*sizeof(trap_t));
  memset((void *)qs, 0, QSIZE*sizeof(node_t));
  for (i = 1; i <= n; i++)
    {
      seg[i].is_inserted = FALSE;

      seg[i].v0.x = vertices[i][0]; /* x-cood */
      seg[i].v0.y = vertices[i][1]; /* y-cood */
      if (i == 1)
	{
	  seg[n].v1.x = seg[i].v0.x;
	  seg[n].v1.y = seg[i].v0.y;
	}
      else
	{
	  seg[i - 1].v1.x = seg[i].v0.x;
	  seg[i - 1].v1.y = seg[i].v0.y;
	}	
    }

  /* Todd's modification */
  /* Move apart coincident edges */

  for (i = 1; i <= n; i++)
    {

      v0[0] = seg[i].v0.x;
      v0[1] = seg[i].v0.y;
      v1[0] = seg[i].v1.x;
      v1[1] = seg[i].v1.y;

      for (j= 1; j < i; j++)
	{
	  /* do we have a coincident edge? */
	  back = i-1;
	  forward = i+1;
	  if (i==1)
	    back = n;
	  if (i == n)
	    forward = 1;
	  
	  if (FP_EQUAL(v0[0], seg[j].v1.x) && FP_EQUAL(v0[1], seg[j].v1.y))
	    {
	      if(FP_EQUAL(v1[0], seg[j].v0.x) && FP_EQUAL(v1[1], seg[j].v0.y))
		{
		  if (CommentLevel > 6)
		    {
		      fprintf(stderr,"Modification i=%d j=%d\n",i, j);
		      fprintf(stderr,"v0[0]=%f v0[1]=%f v1[0]=%f v1[1]=%f\n",
			      v0[0], v0[1], v1[0], v1[1]);
		      fprintf(stderr,"bv0[0]=%f bv0[1]=%f fv1[0]=%f fv1[1]=%f\n",
			      seg[back].v0.x, seg[back].v0.y, 
			      seg[forward].v1.x, seg[forward].v1.y);
		    }
		  v0[0] = seg[back].v1.x = seg[i].v0.x = seg[i].v0.x + (seg[back].v0.x - seg[i].v0.x)/2.0;
		  v0[1] = seg[back].v1.y = seg[i].v0.y = seg[i].v0.y + (seg[back].v0.y - seg[i].v0.y)/2.0;
		  v1[0] = seg[forward].v0.x = seg[i].v1.x = seg[i].v1.x + (seg[forward].v1.x - seg[i].v1.x)/2.0;
		  v1[1] = seg[forward].v0.y = seg[i].v1.y = seg[i].v1.y + (seg[forward].v1.y - seg[i].v1.y)/2.0;
		  if (CommentLevel > 6)
		    fprintf(stderr,"v0[0]=%f v0[1]=%f v1[0]=%f v1[1]=%f\n",
			    v0[0], v0[1], v1[0], v1[1]);
		  j=1;
		}
	      else if (j<(i-1))
		{
		  intersection = lines_intersect(v0[0], v0[1], v1[0], v1[1],
						 seg[j].v0.x, seg[j].v0.y,
						 seg[j].v1.x, seg[j].v1.y,
						 &x, &y);
		  if (intersection != 0)
		    {
		      if ((intersection == 1) &&
			  (!(FP_EQUAL(v0[0], seg[j].v1.x) 
			     && FP_EQUAL(v0[1], seg[j].v1.y)))
			  )
			{
			  if (CommentLevel > 6)
			    {
			  fprintf(stderr,"i=%d j=%dPolygon invalid, self-intersecting code %d \n",i,j,intersection);
			  fprintf(stderr,"Segment v0 to v1: x=%f y=%f to x=%f y=%f\nIntersects %d:x=%f y=%f to x=%f y=%f\n",v0[0], v0[1], v1[0], v1[1], j,
				  
				  seg[j].v0.x, seg[j].v0.y, seg[j].v1.x, seg[j].v1.y);
			    }
			  return(FALSE);
			}
		      else if (intersection == 2)
			{
			  if (CommentLevel > 6)
			    {
			      fprintf(stderr,"Polygon invalid, Seg i=%d j=%d colinear\n",
				      i, j);
			      fprintf(stderr,"Segment v0 to v1: x=%f y=%f to x=%f y=%f\nIntersects %d:x=%f y=%f to x=%f y=%f\n",v0[0], v0[1], v1[0], v1[1], j,
				      seg[j].v0.x, seg[j].v0.y, seg[j].v1.x, seg[j].v1.y);
			    }
			  return(FALSE);
			}
		    }
		}
	    }
	  else if (j<(i-1))
	    {
	      intersection = lines_intersect(v0[0], v0[1], v1[0], v1[1],
					     seg[j].v0.x, seg[j].v0.y,
					     seg[j].v1.x, seg[j].v1.y,
					     &x, &y);
	      if (intersection != 0)
		{
		  if ((intersection == 1) &&
		      (!(FP_EQUAL(v1[0], seg[j].v1.x) 
			 && FP_EQUAL(v1[1], seg[j].v1.y))) &&
		      (!(FP_EQUAL(v1[0], seg[j].v0.x) 
			 && FP_EQUAL(v1[1], seg[j].v0.y)))
		      )
		    {
		      if (CommentLevel > 6)
			{
		      fprintf(stderr,"Polygon invalid, Seg i=%d j=%dself-intersecting code %d at x=%f y=%f\n",
			      i, j, intersection, x, y);
		      fprintf(stderr,"Segment v0 to v1: x=%f y=%f to x=%f y=%f\nIntersects %d:x=%f y=%f to x=%f y=%f\n",v0[0], v0[1], v1[0], v1[1], j,
			      seg[j].v0.x, seg[j].v0.y, seg[j].v1.x, seg[j].v1.y);
			}
		      return(FALSE);
		    }
		  else if (intersection == 2)
		    {
		      if (CommentLevel > 6)
			{
			  fprintf(stderr,"Polygon invalid, Seg i=%d j=%d colinear\n",
				  i, j);
			  fprintf(stderr,"Segment v0 to v1: x=%f y=%f to x=%f y=%f\nIntersects %d:x=%f y=%f to x=%f y=%f\n",v0[0], v0[1], v1[0], v1[1], j,
				  seg[j].v0.x, seg[j].v0.y, seg[j].v1.x, seg[j].v1.y);
			}
		      return(FALSE);
		    }
		}
	    }
	}
    }

  global.nseg = n;
  initialise(n);
  construct_trapezoids(n, seg);
  nmonpoly = monotonate_trapezoids(n);
  triangulate_monotone_polygons(nmonpoly, triangles);
  
  return TRUE;
}


/* This function returns TRUE or FALSE depending upon whether the 
 * vertex is inside the polygon or not. The polygon must already have
 * been triangulated before this routine is called.
 * This routine will always detect all the points belonging to the 
 * set (polygon-area - polygon-boundary). The return value for points 
 * on the boundary is not consistent!!!
 */

static int is_point_inside_polygon(vertex)
     double vertex[2];
{
  point_t v;
  int trnum, rseg;
  trap_t *t;

  v.x = vertex[0];
  v.y = vertex[1];
  
  trnum = locate_endpoint(&v, &v, 1);
  t = &tr[trnum];
  
  if (t->state == ST_INVALID)
    return FALSE;
  
  if ((t->lseg <= 0) || (t->rseg <= 0))
    return FALSE;
  rseg = t->rseg;
  return _greater_than_equal_to(&seg[rseg].v1, &seg[rseg].v0);
}
