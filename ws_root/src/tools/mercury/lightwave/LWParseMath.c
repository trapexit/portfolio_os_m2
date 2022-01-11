typedef float gfloat;

#include "M2TXTypes.h"
#include "LWSURF.h"
#include "vec.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "texmap.h"
#include "qmem.h"
#include "LWMath.h"

extern double   SmoothAngle;
extern UVCoord  *UVs;
extern uint16   UVCount;
extern uint16   UVSize;
extern uint16   UVIncrement;
extern double FilterVerts[MAX_DISC_SIZE+10][2];
extern int    FilterLookup[MAX_DISC_SIZE+10];
extern int    NumFiltered;
extern int CommentLevel;
static void AverageNormals(int32 *ptFacetList, int32 listSize, Point3 *norm,
			   Point3 *surfNormals, gfloat smoothAngle)
{
  uint32 i, index;
  Normal fNormal, tNormal, vNormal;
  double dot, angle, len;

  index = ptFacetList[0];

  fNormal[0] = vNormal[0] = surfNormals[index].x;
  fNormal[1] = vNormal[1] = surfNormals[index].y;
  fNormal[2] = vNormal[2] = surfNormals[index].z;

  for (i=1; i<listSize; i++)
    {
      index = ptFacetList[i];
      tNormal[0] = surfNormals[index].x;
      tNormal[1] = surfNormals[index].y;
      tNormal[2] = surfNormals[index].z;
      dot = DOT3(fNormal, tNormal);
      /* 
	 if ((dot>1.0) || (dot<-1.0))
	 fprintf(stderr,"dot outside of range! dot=%f\n", dot);
	 */
      angle = acos(dot);       
      /* See if the angle between polys is too great */ 
      if (angle <= smoothAngle)
	{
	  VPV3(vNormal, tNormal, vNormal);
	}
    }
  len = NORMSQRD3(vNormal);
  len = sqrt(len);
  if (!FP_EQUAL(len,0.0))
    {	
      vNormal[0] = vNormal[0]/len;
      vNormal[1] = vNormal[1]/len;
      vNormal[2] = vNormal[2]/len;
    }
  norm->x = vNormal[0]; 
  norm->y = vNormal[1];
  norm->z = vNormal[2];
}

/*
**  PlaneEquation--computes the plane equation of an arbitrary
**  3D polygon using Newell's method.
**
*/
void PlaneEquation (uint16 *poly, PointTag *points, Plane plane, 
			   bool UVTranslate, int pInPoly)
{
    int i;
    Point refpt;
    Normal normal;
    Point u, v;
    float len;
    uint16 nVerts, index, index1;


    nVerts = poly[0];

    /* compute the polygon normal and a reference point on
       the plane. Note that the actual reference point is
       refpt / nverts
    */
    ZEROVEC3(normal);
    ZEROVEC3(refpt);
    for(i = 1; i <= nVerts; i++) 
      {
	if (UVTranslate)
	  index = UVs[poly[i]].Geometry;
	else
	  {
	    if (i<pInPoly)      /* Has the point index been changed yet? */
	      index = UVs[poly[i]].Geometry;
	    else
	      index = poly[i];
	  }
        u[0] = points[index].p.x;
	u[1] = points[index].p.y;
	u[2] = points[index].p.z;
/*	fprintf(stderr,"Pt %d:%g %g %g\n",index, u[0], u[1], u[2]); */
	if (i < nVerts)
	  {	
	    if (UVTranslate)
	      index1 = UVs[poly[i+1]].Geometry;
	    else
	      {
		if ((i+1)<pInPoly)      /* Has the point index been changed yet? */
		  index1 = UVs[poly[i+1]].Geometry;
		else
		  index1 = poly[i+1];
	      }
	  }
	else
	  {
	    if (UVTranslate)
	      index1 = UVs[poly[1]].Geometry; 
	    else
	      {
		if (1<pInPoly)      /* Has the point index been changed yet? */
		  index1 = UVs[poly[1]].Geometry; 
		else
		  index1 = poly[1]; 	      
	      }
	  }
        v[0] = points[index1].p.x;
	v[1] = points[index1].p.y;
	v[2] = points[index1].p.z;
        normal[X] += (u[Y] - v[Y]) * (u[Z] + v[Z]);
        normal[Y] += (u[Z] - v[Z]) * (u[X] + v[X]);
        normal[Z] += (u[X] - v[X]) * (u[Y] + v[Y]);
        VPV3(refpt,refpt, u);
    }
    /* normalize the polygon normal to obtain the first
       three coefficients of the plane equation
       */
    len = NORMSQRD3(normal);
    len = sqrt(len);
    if (!FP_EQUAL(len,0.0))
      {
	plane[X] = normal[X] / len;
	plane[Y] = normal[Y] / len;
	plane[Z] = normal[Z] / len;
	/* compute the last coefficient of the plane equation */
	len *= nVerts;
	plane[D] = - (DOT3(refpt, normal)) / len;
      }
    else
      {
	fprintf(stderr,"WARNING:Plane Vector of length 0\n");
	plane[X] = normal[X];
	plane[Y] = normal[Y];
	plane[Z] = normal[Z];
	plane[D] = - (DOT3(refpt, normal));
      }
}

void TransformPt(uint16 *surfPoly, PointTag *points, int pInPoly, 
			uint16 index, float *outU, float *outV, LWTex *lwTex, 
			float *minU, float *maxU, float *minV, float *maxV, 
			int *uCrossing, int *vCrossing, bool uvTranslate)
{
  double p[3];
  double u, v, diff;
  double xAxis[3] = { 1, 0 , 0 };
  double yAxis[3] = { 0, 1 , 0 };
  double zAxis[3] = { 0, 0 , 1 };
  double xCAxis[3] = { 1, 0 , 0 };
  double yCAxis[3] = { 0, 1 , 0 };
  double zCAxis[3] = { 0, 0 , 1 };
  double temp3[3];
  double mapXAxis[3], mapYAxis[3], mapZAxis[3];
  double negXAxis[3], negYAxis[3], negZAxis[3];
  double origin[3];
  Plane plane;
  bool flag;
  double dX, dY, dZ;
  int axisSwitch;
  bool start;

  if (pInPoly == 1)
    start = TRUE;
  else 
    start = FALSE;

  SXV3(xAxis, lwTex->TSIZ.x, xAxis);
  SXV3(yAxis, lwTex->TSIZ.y, yAxis);
  SXV3(zAxis, lwTex->TSIZ.z, zAxis);

  SXV3(negXAxis, -1.0, xAxis);
  SXV3(negYAxis, -1.0, yAxis);
  SXV3(negZAxis, -1.0, zAxis);

  p[0] = points[index].p.x;
  p[1] = points[index].p.y;
  p[2] = points[index].p.z;
  origin[0] = lwTex->TCTR.x;
  origin[1] = lwTex->TCTR.y;
  origin[2] = lwTex->TCTR.z;
  
  if (!strcmp(lwTex->Name, "Planar Image Map"))
    {
      /*      fprintf(stderr, "Planar Image Map\n"); */
      LWTex_GetFXAxis(*lwTex, &flag);
      if (flag)
	{
	  SET3(mapXAxis, zAxis);
	  SET3(mapYAxis, yAxis);
	  SET3(mapZAxis, xAxis);
	}
      else
	{
	  LWTex_GetFYAxis(*lwTex, &flag);
	  if (flag)
	    {
	      SET3(mapXAxis, xAxis);
	      SET3(mapYAxis, zAxis);
	      SET3(mapZAxis, yAxis);
	    }
	  else
	    {
	      LWTex_GetFZAxis(*lwTex, &flag);
	      if (flag)
		{
		  SET3(mapXAxis, xAxis);
		  SET3(mapYAxis, yAxis);
		  SET3(mapZAxis, zAxis);
		}
	    }
	}
      /* This routine wants the origin in the lower left corner */
      SXV3(temp3, 0.5, mapXAxis);
      VMV3(origin, origin, temp3);
      SXV3(temp3, 0.5, mapYAxis);
      VMV3(origin, origin, temp3);
      PlaneMap(p, &u, &v, origin, mapXAxis, mapYAxis, mapZAxis); 
      *outU = u;
      *outV = 1.0 - v;  /* To compensate for the 0,0 being in the Upper left */
    }
  else if (!strcmp(lwTex->Name, "Cubic Image Map"))
    {
      PlaneEquation(surfPoly, points, plane, uvTranslate, pInPoly);
      
      temp3[0] = (double)plane[0];
      temp3[1] = (double)plane[1];
      temp3[2] = (double)plane[2];

      dX = fabs(DOT3(temp3, xCAxis));
      dY = fabs(DOT3(temp3, yCAxis));
      dZ = fabs(DOT3(temp3, zCAxis));
      
      if (dX > dY)
	{
	  if(dX > dZ)
	    axisSwitch = 0;
	  else
	    axisSwitch = 2;
	}
      else
	{
	  if (dY > dZ)
	    axisSwitch = 1;
	  else
	    axisSwitch = 2;
	}

      switch (axisSwitch)
	{
	case 0:
	default:
	  SET3(mapXAxis, zAxis);
	  SET3(mapYAxis, yAxis);
	  SET3(mapZAxis, xAxis);
	  break;
	case 1:
	  SET3(mapXAxis, xAxis);
	  SET3(mapYAxis, zAxis);
	  SET3(mapZAxis, yAxis);
	  break;
	case 2:
	  SET3(mapXAxis, xAxis);
	  SET3(mapYAxis, yAxis);
	  SET3(mapZAxis, zAxis);
	  break;
	}
      /* This routine wants the origin in the lower left corner */
      SXV3(temp3, 0.5, mapXAxis);
      VMV3(origin, origin, temp3);
      SXV3(temp3, 0.5, mapYAxis);
      VMV3(origin, origin, temp3);
      PlaneMap(p, &u, &v, origin, mapXAxis, mapYAxis, mapZAxis); 
      *outU = u;
      *outV = 1.0 - v;  /* To compensate for the 0,0 being in the Upper left */
    }
  else if (!strcmp(lwTex->Name, "Cylindrical Image Map"))
    {
       
      LWTex_GetFXAxis(*lwTex, &flag);
      if (flag)
	{
	  SET3(mapXAxis, yAxis);
	  SET3(mapYAxis, negZAxis);
	  SET3(mapZAxis, xAxis);
	}
      else
	{
	  LWTex_GetFYAxis(*lwTex, &flag);
	  if (flag)
	    {
	      SET3(mapXAxis, zAxis);
	      SET3(mapYAxis, negXAxis);
	      SET3(mapZAxis, yAxis);
	    }
	  else
	    {
	      LWTex_GetFZAxis(*lwTex, &flag);
	      if (flag)
		{
		  SET3(mapXAxis, negYAxis);
		  SET3(mapYAxis, negXAxis);
		  SET3(mapZAxis, zAxis);
		}
	    }
	}
      /* This routine wants it to be on the bottom of the central axis */
      SXV3(temp3, 0.5, mapZAxis);
      VMV3(origin, origin, temp3);
      CylinMap(p, &u, &v, origin, mapXAxis, mapYAxis, mapZAxis);
      v = 1.0 - v;

      if (start)
	*uCrossing = 0;
      else
	{
	  if (*uCrossing == 1)
	    {
	      if (u <0.5)
		u += 1.0;
	    }
	  else if (*uCrossing == -1)
	    {
	      if (u>0.5)
		u -= 1.0;
	    }
	  else
	    {
	      if (*minU > u)
		{
		  diff = *maxU - u;
		  if (diff > 0.5)
		    {
		      u += 1.0;
		      *uCrossing = 1;
		    }
		}
	      else if (*maxU < u)
		{
		  diff = u - *minU;
		  if (diff > 0.5)
		    {
		      u -= 1.0;
		      *uCrossing = -1;
		    }
		}
	    }
	}
      *outU = (float)u;
      *outV = (float)v;  /* To compensate for the 0,0 being in the Upper left */
    }
  else if (!strcmp(lwTex->Name, "Spherical Image Map"))
    {
      /*       fprintf(stderr, "Spherical Image Map\n"); */
      LWTex_GetFXAxis(*lwTex, &flag);
      if (flag)
	{
	  SET3(mapXAxis, yAxis);
	  SET3(mapYAxis, negZAxis);
	  SET3(mapZAxis, xAxis);
	}
      else
	{
	  LWTex_GetFYAxis(*lwTex, &flag);
	  if (flag)
	    {
	      SET3(mapXAxis, zAxis);
	      SET3(mapYAxis, negXAxis);
	      SET3(mapZAxis, yAxis);
	    }
	  else
	    {
	      LWTex_GetFZAxis(*lwTex, &flag);
	      if (flag)
		{
		  SET3(mapXAxis, negYAxis);
		  SET3(mapYAxis, negXAxis);
		  SET3(mapZAxis, zAxis);
		}
	    }
	}
/*
      SXV3(temp3, 0.5, mapZAxis);
      VMV3(origin, origin, temp3);
*/ 
     SphereMap(p, &u, &v, origin, mapXAxis, mapYAxis, mapZAxis); 
      
     v = 1.0 - v;

      if (start)
	{
	  *uCrossing = 0;
	  *vCrossing = 0;
	}
      else
	{
	  if (*uCrossing == 1)
	    {
	      if (u <0.5)
		u += 1.0;
	    }
	  else if (*uCrossing == -1)
	    {
	      if (u>0.5)
		u -= 1.0;
	    }
	  else
	    {
	      if (*minU > u)
		{
		  diff = *maxU - u;
		  if (diff > 0.5)
		    {
		      u += 1.0;
		      *uCrossing = 1;
		    }
		}
	      else if (*maxU < u)
		{
		  diff = u - *minU;
		  if (diff > 0.5)
		    {
		      u -= 1.0;
		      *uCrossing = -1;
		    }
		}
	    }
/*
	  if (*vCrossing == 1)
	    {
	      if (v <0.5)
		v += 1.0;
	    }
	  else if (*vCrossing == -1)
	    {
	      if (v>0.5)
		v -= 1.0;
	    }
	  else
	    {
	      if (*minV > v)
		{
		  diff = *maxV - v;
		  if (diff > 0.5)
		    {
		      v += 1.0;
		      *vCrossing = 1;
		    }
		}
	      else if (*maxV < v)
		{
		  diff = v - *minV;
		  if (diff > 0.5)
		    {
		      v -= 1.0;
		      *vCrossing = -1;
		    }
		}
	    }
	    */
	}
      *outU = (float)u;
      *outV = (float)v;  /* To compensate for the 0,0 being in the Upper left */
    }
  else
    {
      fprintf(stderr,"Using default  should be \"%s\"\n", lwTex->Name);
      PlaneMap(p, &u, &v, origin, xAxis, yAxis, zAxis); 
      *outU = u;
      *outV = 1.0 - v;  /* To compensate for the 0,0 being in the Upper left */
    }

  if (start)
    {
      *minU = *outU;
      *maxU = *outU;
      *minV = *outV;
      *maxV = *outV;
    }
  else
    {
      if (*outU > (*maxU))
	*maxU = *outU;
      if ( *outU < (*minU))
	*minU = *outU;
      if (*outV > (*maxV))
	*maxV = *outV;
      if ( *outV < (*minV))
	*minV = *outV;  
    }
}

static void FindArbitraryAxis(double xAngle, double yAngle, double zAngle, 
		  double arbAxis[3],double *arbAngle)
{
  double rotXMat[4][4];
  double rotYMat[4][4];
  double rotZMat[4][4];
  double rotXYMat[4][4];
  double rotMat[4][4];
  double *tmp;

  double angle; double c; double s;
  
  tmp = arbAngle;
  IDENTMAT4(rotXMat);
  IDENTMAT4(rotYMat);
  IDENTMAT4(rotZMat);

  s = sin(xAngle*PI/180.0);
  c = cos(xAngle*PI/180.0);
  rotXMat[1][1] = c;
  rotXMat[2][2] = c;
  rotXMat[1][2] = -s;
  rotXMat[2][1] = s;    /* X Rotation */

  s = sin(yAngle*PI/180.0);
  c = cos(yAngle*PI/180.0);
  rotXMat[0][0] = c;
  rotXMat[2][2] = c;
  rotXMat[0][2] = s;
  rotXMat[2][0] = -s;    /* Y Rotation */

  s = sin(zAngle*PI/180.0);
  c = cos(zAngle*PI/180.0);
  rotXMat[0][0] = c;
  rotXMat[1][1] = c;
  rotXMat[0][1] = -s;
  rotXMat[1][0] = s;    /* Z Rotation */

  MXM4(rotXYMat, rotXMat, rotYMat);
  MXM4(rotMat, rotXYMat, rotZMat);

  c = (rotMat[0][0] + rotMat[1][1] + rotMat[2][2] -1)*0.5;
  
  angle = acos(c);
  s = 1.0/2*sin(angle);
  arbAxis[0] = (rotMat[1][2] - rotMat[2][1])*s;
  arbAxis[1] = (rotMat[2][0] - rotMat[0][2])*s;
  arbAxis[2] = (rotMat[0][1] - rotMat[1][0])*s;
  
}

void Project (Plane plane, Point point, float *x, float *y)
{
  double rotXMat[4][4];
  double rotYMat[4][4];
  double rotMat[4][4];
  double zProjMat[4][4];
  double planeProjMat[4][4];
  double d;
  double out[3];

  d = sqrt(plane[1]*plane[1]+plane[2]*plane[2]);
  IDENTMAT4(rotXMat);
  IDENTMAT4(rotYMat);
  IDENTMAT4(zProjMat);
  zProjMat[2][2] = 0;

 /* Rotate plane normal to point along point along Z-axis */
  rotXMat[2][2] = rotXMat[1][1] = plane[2]/d; 
  rotXMat[2][1] = rotXMat[1][2] = plane[1]/d;
  rotXMat[2][1] = -(plane[1]/d);
  rotYMat[0][0] = rotYMat[2][2] = d;
  rotYMat[2][0] = plane[0];
  rotYMat[2][0] = -plane[0];

  MXM4(rotMat, rotXMat, rotYMat);
  MXM4(planeProjMat, rotMat, zProjMat);
  V3XM4(out, point, planeProjMat);

  *x = out[0];
  *y = out[1];
}

void PlaneEquation2D (uint16 nVerts, point2 *points, Plane plane)
{
    int i;
    Point refpt;
    Normal normal;
    Point u, v;
    float len;
    uint16 index, index1;


    /* compute the polygon normal and a reference point on
       the plane. Note that the actual reference point is
       refpt / nverts
    */
    ZEROVEC3(normal);
    ZEROVEC3(refpt);
    for(i = 0; i < nVerts; i++) 
      {
	index = i;
        u[0] = points[index][0];
	u[1] = points[index][1];
	u[2] = 0.0;
/*	fprintf(stderr,"Pt %d:%g %g %g\n",index, u[0], u[1], u[2]); */
	if (i < nVerts)
	  {
	    index1 = i+1;
	  }
	else
	  {
	    index1 = 0; 
	  }
        v[0] = points[index1][0];
	v[1] = points[index1][1];
	v[2] = 0.0;
        normal[X] += (u[Y] - v[Y]) * (u[Z] + v[Z]);
        normal[Y] += (u[Z] - v[Z]) * (u[X] + v[X]);
        normal[Z] += (u[X] - v[X]) * (u[Y] + v[Y]);
        VPV3(refpt,refpt, u);
    }
    /* normalize the polygon normal to obtain the first
       three coefficients of the plane equation
       */
    len = NORMSQRD3(normal);
    len = sqrt(len);

    if (!FP_EQUAL(len, 0.0))
      {
	plane[X] = normal[X] / len;
	plane[Y] = normal[Y] / len;
	plane[Z] = normal[Z] / len;
	/* compute the last coefficient of the plane equation */
	len *= nVerts;
	plane[D] = - (DOT3(refpt, normal)) / len;
      }
    else
      {
	fprintf(stderr,"WARNING:Plane Vector of length 0\n");
	plane[X] = normal[X];
	plane[Y] = normal[Y];
	plane[Z] = normal[Z];
	plane[D] = - (DOT3(refpt, normal));
      }
}


int Compare(Point2d p, Point2d q) /* Lexicographic comparison of p and q*/
{
    if (p.x < q.x) return -1;	/* p is less than q.			*/
    if (p.x > q.x) return  1;	/* p is greater than q.			*/
    if (p.y < q.y) return -1;	/* p is less than q.			*/
    if (p.y > q.y) return  1;	/* p is greater than q.			*/
    return 0;			/* p is equal to q.			*/
}

int WhichSide(Point2d p, Point2d q, Point2d r)
	/* Given a directed line pq, determine	*/
	/* whether qr turns CW or CCW.		*/
{
    double result;
    result = (p.x - q.x) * (q.y - r.y) - (p.y - q.y) * (q.x - r.x);
    if (result < 0) return -1;	/* q lies to the left  (qr turns CW).	*/
    if (result > 0) return  1;	/* q lies to the right (qr turns CCW).	*/
    return 0;			/* q lies on the line from p to r.	*/
}

static double WhichSideD(p, q, r)/* Given a directed line pq, determine	*/
     Point2d	      p, q, r;		/* whether qr turns CW or CCW.		*/
{
  double v1[2], v2[2];
  double result, len1, len2;
  double len1Sq, len2Sq;
  v1[0] = p.x - q.x;
  v1[1] = p.y - q.y;
  v2[0] = q.x - r.x;
  v2[1] = q.y - r.y;
  
  len1Sq = NORMSQRD2(v1);
  len2Sq = NORMSQRD2(v2);
  len1 = sqrt(len1Sq);
  len2 = sqrt(len2Sq);
  if (!FP_EQUAL(len1, 0.0))
      {
	len1 = 1.0/len1;
	SXV2(v1, len1, v1);
      }
  else
    {
      fprintf(stderr,"WARNING: len1 = 0.0\n");
    }
  if (!FP_EQUAL(len2, 0.0))
      {
	len2 = 1.0/len2;
	SXV2(v1, len2, v1);
      }
  else
    {
      fprintf(stderr,"WARNING: len2 = 0.0\n");
    }
 
/*
  fprintf(stderr,"v1:x=%g y=%g v2:x=%g y=%g\n", v1[0], v1[1], v2[0], v2[1]);
  fprintf(stderr,"V1 LenSQD=%g, V2 Len=%g, V2 LenSQD=%g, V2 Len=%g\n",
	  len1Sq, len1, len2Sq, len2);
*/
  SXV2(v2, 1.0/len2, v2);

/*
  fprintf(stderr,"v1:x=%g y=%g v2:x=%g y=%g\n", v1[0], v1[1], v2[0], v2[1]); 
*/
/*
  result = v1[0]*v2[1] -v1[1]*v2[0];
*/

   result = (p.x - q.x) * (q.y - r.y) - (p.y - q.y) * (q.x - r.x);

  /* 
     fprintf(stderr,"p:x=%g y=%g q:x=%g y=%g r:x=%g y=%g result=%g\n",
	  p.x, p.y, q.x, q.y, r.x, r.y, result);
	  */
  return result;	
}

bool polygon_filter(int nVerts, double vertices[][2])
{
  int top, next, last;
  int filtered, skip;
  int i;

  NumFiltered = filtered = 0;
  FilterVerts[filtered][0] = vertices[0][0];
  FilterVerts[filtered][1] = vertices[0][1];
  FilterLookup[filtered]= 0;
  filtered++;
  for (last=0; last < (nVerts-1); last+=skip)
    {
      next = last+1;
      top = next+1;
      skip = 1;
      if ((FP_EQUAL(vertices[last][0], vertices[next][0])) &&
	  (FP_EQUAL(vertices[last][1], vertices[next][1])))
	{
	  skip = 1;
	}
      else if ((FP_EQUAL(vertices[last][0], vertices[top][0])) &&
	  (FP_EQUAL(vertices[last][1], vertices[top][1])))
	{
	  skip = 2;
	}
      else
	{
	  FilterVerts[filtered][0] = vertices[next][0];
	  FilterVerts[filtered][1] = vertices[next][1];
	  FilterLookup[filtered]=next;
	  filtered++;
	}
    }
  /* vertex 0 = vertex n */
  if ((FP_EQUAL(vertices[nVerts][0], vertices[nVerts-1][0])) &&
       (FP_EQUAL(vertices[nVerts][1], vertices[nVerts-1][1])))
    {
      filtered--;
    } 
  else
    {
      FilterVerts[filtered][0] = vertices[0][0];
      FilterVerts[filtered][1] = vertices[0][1];
      FilterLookup[filtered]=next;
    }

  if (filtered == nVerts)
    return(FALSE);
  else
    {
      NumFiltered = filtered;
      if (CommentLevel > 8)
	{
	  fprintf(stderr,"WARNING:There were redundant edges and vertices from %d to %d\n",
		  nVerts, NumFiltered);
	  for (i=0; i<= NumFiltered; i++)
	    {
	      fprintf(stderr,"%d:x=%f y=%f\n", i, FilterVerts[i][0], 
		      FilterVerts[i][1]);
	    }
	}
      return(TRUE);
    }
}

typedef struct {
  double x, y;
} point_t, vector_t;

double LENGTH (point_t v0)
{
  return(sqrt((v0).x * (v0).x + (v0).y * (v0).y));
}

bool MakeNormals(bool computeNormals, int32 *pointFacetAlloc, int32 *pointFacetOffset,
		 int32 *pointFacets, int16 nPolys, uint16 j, uint16 k,
		 uint16 **surfPolys, PointTag *points,
		 Point3 *surfNormals, gfloat smoothAngle)
{
  int nPt, nCount, gNCount;
  uint16 nIndex, temp16;
  Point3 norm;
  uint16 index;
  double x,y,z;
  uint16 *surfPoly;
  
  bool skipPt;

  nIndex = nPolys;  /* No reason */
  x = y = z = 0.0;
  index = surfPolys[j][k];
  if (computeNormals)
    {
      AverageNormals(pointFacets+pointFacetOffset[index], pointFacetAlloc[index],
		     &norm, surfNormals, smoothAngle);
      x = norm.x; y = norm.y; z = norm.z;
    }
  skipPt = FALSE;
  surfPoly = surfPolys[j];
  if (points[index].inUse)
    {
      if (computeNormals)    /* If the point is in Use, it might have the "wrong" normal */
	for (nPt=0; nPt < points[index].UVCount; nPt++)
	  {
	    if (points[index].UVSize==1)   /* Special Case */
	      nIndex = (uint16) points[index].UVOnly;
	    else
	      nIndex = points[index].UVList[nPt];
	    if ((x==UVs[nIndex].N.x) && (y==UVs[nIndex].N.y) &&
		(z==UVs[nIndex].N.z))
	      {
		surfPoly[k] = nIndex;
		skipPt = TRUE;    /* If it's here already, skip it */
		break;
	      }
	  }
      else
	{
	  surfPoly[k] = points[index].IsNow;		  
	  skipPt = TRUE;  /* In use + no normals = automatic skip it */
	}
    }
  if (!skipPt)
    {
      /* Add the point to the list of UV coords if it has any */
      nCount = points[index].UVCount;
      points[index].UVCount++;
      if (nCount >= points[index].UVSize)
	{
	  if (points[index].UVSize == 1)  /* Special Case */
	    {
	      temp16 = (uint16)points[index].UVOnly;
	      points[index].UVList = (uint16 *)qMemClearPtr(10,sizeof(uint16));
	      points[index].UVList[0] = temp16;
	      points[index].UVSize = 10;
	    }
	  else
	    {
	      points[index].UVSize += 10;
	      points[index].UVList = (uint16 *)qMemResizePtr(points[index].UVList,
						       sizeof(uint16)*points[index].UVSize);
	    }
	}
      gNCount = UVCount;
      UVCount++;
      if (UVCount >= UVSize)
	{
	  UVSize += UVIncrement;
	  UVs = (UVCoord *)qMemResizePtr(UVs, UVSize*sizeof(UVCoord));
	}
      if (points[index].UVSize == 1)  /* Special Case */
	points[index].UVOnly = gNCount;
      else
	points[index].UVList[nCount] = gNCount;
      UVs[gNCount].N.x = x;
      UVs[gNCount].N.y = y;
      UVs[gNCount].N.z = z;
      UVs[gNCount].UVSet = FALSE;
      UVs[gNCount].Geometry = index;   /* Like UV's  */
      surfPoly[k] = gNCount;
      points[index].inUse = TRUE;
    }

  return (skipPt);
}
