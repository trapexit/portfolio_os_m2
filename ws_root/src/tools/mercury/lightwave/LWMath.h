/* definition for the components of vectors and plane equations */
#define X   0
#define Y   1
#define Z   2
#define D   3

/* type definitions for vectors and plane equations */
typedef float Vector[3];
typedef Vector Point;
typedef Vector Normal;
typedef float Plane[4];

#define PI 3.14159265359

typedef void *PntID;

typedef struct 
{
  PntID     pnt;
  Point3 p;
  uint16    UVCount;
  uint16    UVSize;
  uint16    IsNow;
  uint16    UVOnly;
  uint16    *UVList;
  bool   inUse;
} PointTag;

typedef struct 
{
  bool   UVSet;
  float  U;
  float  V;
  uint16 Geometry;
  Point3 N;
} UVCoord;


#define C_EPS 1.0e-15

#define FP_EQUAL(s, t) (fabs(s - t) <= C_EPS)

typedef enum { NotConvex=0, NotConvexDegenerate, NotConvexCW, NotConvexCCW,
	       ConvexDegenerate, ConvexCCW, ConvexCW } PolygonClass;

typedef struct { double x, y; } Point2d;

typedef double point2[2];

typedef int    triIndex[3];

#define MAX_DISC_SIZE 2000

