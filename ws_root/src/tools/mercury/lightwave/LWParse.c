/*
  File:		LWParse.c

  Contains:	Reading of a Lightwave IFF object file 
  
  Written by:	Todd Allendorf 
  
  Copyright:	© 1996 by The 3DO Company. All rights reserved.
  This material constitutes confidential and proprietary
  information of the 3DO Company and shall not be used by
  any Person or for any purpose except as expressly
  authorized in writing by the 3DO Company.
  
  Change History (most recent first):
  
  <4>	12/15/95	PUT YOUR INITIALS HERE		Added better error checking during output and the -t (no
  textures) option.
  
  To Do:
  */


#include "ifflib.h"
#include <stdlib.h>
#include <stdio.h>
#include "M2TXTypes.h"
#include "M2Err.h"
#include "LWSURF.h"
#include "M2TXattr.h"
#include "SDFTexBlend.h"
#include "SDFMat.h"
#include <string.h>
#include <ctype.h>
#include "qstream.h"
#include "qmem.h"
#include <math.h>
#include "clt.h"
#include "clttxdblend.h"
#include "LWMath.h"

#ifndef SEEK_SET
#define SEEK_SET 0
#endif
/* Globals- ICK! */

typedef void *MeshEditOp;
typedef void *Monitor;

typedef struct st_MyPointData
{
  MeshEditOp  *op;
  PointTag    *points;
  Monitor     *mon;
  uint32      nPoints;
  uint32      allocPoints;
  int32       *pointFacetAlloc;
  int32       *pointFacetOffset;
} MyPointData;

typedef struct st_MyPolyData
{
  MyPointData *pData;
  Monitor     *mon;
  bool        detail;
  uint32      surfIndex;
  char        **namePtrs;
  LWSURF      *surfs;
  int16       nSurfs;
  uint16      *nSurfPolys;
  uint16      *nSurfDPolys;
  uint16      **surfPolys;   /* We only need a buffer for 1 Surface At a Time, regular or detail */
  uint16      *polyBuffer;
  uint32      *polyBufSize[2];
  uint32      curBufSize;
  uint32      curPoly;
  uint32      used;
} MyPolyData;


FILE *OutScript = NULL;
FILE *TexScript = NULL;
FILE *MatScript = NULL;
FILE *TexOutFile = NULL;
FILE *MatOutFile = NULL;
char          *BaseName;
int vertexOrder = 0;
int setScale = 1;
double AmbientScale = 0.2;
double SmoothAngle = 89.5 * PI/180.0;
double WhiteIntensity = 0.8;
bool testConvex = TRUE;
bool NoFlat = FALSE;
bool NoAmbient = FALSE;
bool TexModulate = TRUE;
bool EnvMult = FALSE;
bool DoEnvironmentMap = TRUE;
bool DiffForColor = TRUE;
bool ToLower = TRUE;
bool SepMatTex = FALSE;
bool SepTexScript = FALSE;
bool SepMatScript = FALSE;
bool UseDummyTex  = FALSE;
int CommentLevel = 4;
bool RemoveExtensions = TRUE;
bool GlobalTextures = FALSE;
bool GlobalMaterials = FALSE;
bool NoTextures = FALSE;
bool ComputeNormals = TRUE;
bool texAppend = FALSE;
bool matAppend = FALSE;
char TexFileName[256];
char MatFileName[256];

SDFTex *GTextures;
SDFMat *GMaterials;

#define LW_IOBUF_SIZE  65536
#define TEX_BUF_SIZE 20

int TotalPolys = 0;
int TotalFPolys = 0;
int TotalSPolys = 0;
int TotalDPolys = 0;
long TotalVertices = 0;
int NTextures;
int CurTexture;
int NMaterials;
int CurMaterial;
uint32 NPoints;

UVCoord  *UVs;

uint16  UVCount;
uint16  UVSize;
uint16  UVIncrement=900;

static void ID_Decode(uint32 id, char text[5])
{
  text[0] = (uint8)(id>>24);
  text[1] = (uint8)((id>>16)&(0x00FF));
  text[2] = (uint8)((id & 0x0000FFFF)>>8);
  text[3] = (uint8)(id & 0x000000FF);
  text[4] = '\0';
}

bool MakeNormals(bool computeNormals, int32 *pointFacetAlloc, int32 *pointFacetOffset,
		 int32 *pointFacets, int16 nPolys, uint16 j, uint16 k,
		 uint16 **surfPolys, PointTag *points,
		 Point3 *surfNormals, gfloat smoothAngle);

void PlaneEquation (uint16 *poly, PointTag *points, Plane plane, 
			   bool UVTranslate, int pInPoly);

void TransformPt(uint16 *surfPoly, PointTag *points, int pInPoly, 
			uint16 index, float *outU, float *outV, LWTex *lwTex, 
			float *minU, float *maxU, float *minV, float *maxV, 
			int *uCrossing, int *vCrossing, bool uvTranslate);

void Project (Plane plane, Point point, float *x, float *y);


/*
**  PlaneEquation2D--computes the plane equation of an arbitrary
**  2D polygon using Newell's method.
**
*/
void PlaneEquation2D (uint16 nVerts, point2 *points, Plane plane);
int triangulate_polygon(int n, double vertices[][2], int triangles[][3]);
int Compare(Point2d p, Point2d q); /* Lexicographic comparison of p and q*/


static bool GetDifferentPoint(Point2d *poly, uint16 *index, uint16 size, Point2d previous, 
		      Point2d *next)
{		/* return true iff successful.		*/
  
  *next = poly[*index];
  *index = (*index)+1;
  while((*index <= size) && (Compare(previous, *next) == 0))
    {
      *next = poly[*index];
      (*index)++;
    }

  if ((*index) <= size)
    return (TRUE);
  else
    return (FALSE);
}

static uint16 Disc_Table[MAX_DISC_SIZE+10];
static uint16 Disc_List[MAX_DISC_SIZE+10];


static bool DiscDetect(uint16 *poly)
{
  int nVerts, i, tableIndex, discIndex;
  int discInc;
  uint16 top;
  bool turn1;

  turn1 = FALSE;
  nVerts = poly[0];

  if (nVerts > MAX_DISC_SIZE)
    return(FALSE);
  if (nVerts < 6)
    return(FALSE);
  if (nVerts%2)
    return(FALSE);
  else
    {

      discIndex = 0;
      Disc_List[discIndex] = Disc_Table[0] = poly[1];
      discIndex++;
      discInc = 1;
      tableIndex = 0;
      Disc_List[discIndex] = top = poly[2];
      for (i=3; i<=nVerts; i++)
	{
	  if (poly[i] == Disc_Table[tableIndex])  /* doubling back on the disc */
	    {
	      if (turn1 == FALSE)
		{
		  turn1 = TRUE;
		  discIndex = (nVerts/2)-1;
		  discInc = -1;
		  tableIndex = 0;
		  Disc_Table[0] = top;
		  Disc_List[discIndex] = top = poly[i];
		  for (; tableIndex>=0; tableIndex--, i++)
		    {
		      if (poly[i] != Disc_Table[tableIndex])
			{
/*			  fprintf(stderr,"Not a disc\n");	*/

			  return(FALSE);
			}
		    }
		}
	      else
		{
		  for (; tableIndex>0; tableIndex--, i++)
		   if (poly[i] != Disc_Table[tableIndex])
		     {
		       return(FALSE);
		     }
		     if (top == Disc_Table[0])
		    {
		      return(TRUE);
		    }
		}
	    }
	  else
	    {
	      tableIndex++;
	      discIndex += discInc;
	      Disc_Table[tableIndex] = top;
	      Disc_List[discIndex] = top = poly[i];
	    }  
	}

      
      /*     fprintf(stderr,"Not a disc\n"); */
      
      return(FALSE);
    }
}


double FilterVerts[MAX_DISC_SIZE+10][2];
int    FilterLookup[MAX_DISC_SIZE+10];
int    NumFiltered = 0;

bool polygon_filter(int nVerts, double vertices[][2]);

static double StaticVerts[MAX_DISC_SIZE+10][2];

static bool triangulate(uint16 *poly, PointTag *points, triIndex **triangles)
{
  point2 vertTemp;
  int elemSize, allocVerts;
  triIndex *triPtr;
  int n, i, bottom, top ;
  uint16 nVerts, realVerts, index;
  Plane plane;
  Point point;
  float x, y;
  int result;
  bool filtered;

  n = poly[0];

  if (n>MAX_DISC_SIZE)
    {
      return(FALSE);
    }

  realVerts = nVerts = poly[0];
  
  nVerts = poly[0];

  PlaneEquation(poly, points, plane, TRUE, 0);
  elemSize = sizeof(point2);
  allocVerts = nVerts+2;

/*
  fprintf(stderr,"Size of point2=%d; AllocVerts=%d\n",elemSize, allocVerts);
*/ 
  
  for (i=1; i<=(nVerts+1); i++)
    {
      if (i<=nVerts)
	{
	  index = UVs[poly[i]].Geometry;
	}
      else
	index = UVs[poly[1]].Geometry;
      
      point[0] = points[index].p.x;
      point[1] = points[index].p.y;
      point[2] = points[index].p.z;
      Project(plane, point, &x, &y);

      StaticVerts[i-1][0] = x;
      StaticVerts[i-1][1] = y;
/*
      fprintf(stderr,"Vert %d: x=%g y=%g z=%g to x=%g y=%g\n", i-1,
	point[0], point[1], point[2], x, y); 
*/
    }
  
  PlaneEquation2D(nVerts, StaticVerts, plane);
  /*
     fprintf(stderr,"Plane=VX=%g VY=%g VZ=%g VW=%g\n",plane[0], plane[1],
  plane[2], plane[3]);
  */
  
  if (plane[2]<0.0)
    {
      /*
	 fprintf(stderr,"Flipping\n");
	 */
      top = nVerts;
      bottom = 0;
      while (bottom<top)
	{
	  vertTemp[0] = StaticVerts[bottom][0];  vertTemp[1] = StaticVerts[bottom][1]; 
	  StaticVerts[bottom][0] = StaticVerts[top][0]; StaticVerts[bottom][1] = StaticVerts[top][1];
	  StaticVerts[top][0] = vertTemp[0];  StaticVerts[top][1] = vertTemp[1];
	  bottom++;
	  top--;
	}
    } 
  
  triPtr = *triangles = (triIndex *)qMemClearPtr(3*(nVerts+5),sizeof(int));

  filtered = polygon_filter(nVerts, StaticVerts);
  if (filtered)
    {
      if (CommentLevel > 7)
	fprintf(stderr,"NumFiltered =%d\n", NumFiltered);
      if ((NumFiltered - nVerts)<2)
	fprintf(stderr,"WARNING:One of your polys has a duplicate point. Mercury may barf!\n");
      else
	fprintf(stderr,"WARNING:One of your polys has %d duplicate points. Mercury may barf!\n",
		nVerts-NumFiltered);

      result = triangulate_polygon(NumFiltered, FilterVerts, triPtr);
      if (plane[2]<0.0)
	{
	  for (i=0; i<NumFiltered-2; i++)
	    {
	      triPtr[i][0] = FilterLookup[triPtr[i][0]];
	      triPtr[i][0] = nVerts - triPtr[i][0];
	      if (triPtr[i][0] == 0)
		triPtr[i][0] = nVerts;
	      triPtr[i][1] = FilterLookup[triPtr[i][1]];
	      triPtr[i][1] = nVerts - triPtr[i][1];
	      if (triPtr[i][1] == 0)
		triPtr[i][1] = nVerts;
	      triPtr[i][2] = FilterLookup[triPtr[i][2]];
	      triPtr[i][2] = nVerts - triPtr[i][2];
	      if (triPtr[i][2] == 0)
		triPtr[i][2] = nVerts;
	    }
	}
    }
  else
    {
      result = triangulate_polygon(realVerts, StaticVerts, triPtr);
      if (plane[2]<0.0)
	{
	  for (i=0; i<nVerts-2; i++)
	    {
	      triPtr[i][0] = nVerts - triPtr[i][0];
	      if (triPtr[i][0] == 0)
		triPtr[i][0] = nVerts;
	      triPtr[i][1] = nVerts - triPtr[i][1];
	      if (triPtr[i][1] == 0)
		triPtr[i][1] = nVerts;
	      triPtr[i][2] = nVerts - triPtr[i][2];
	      if (triPtr[i][2] == 0)
		triPtr[i][2] = nVerts;
	    }
	}
    }
  
  return(result);
}

int WhichSide(Point2d p, Point2d q, Point2d r);

/* CheckTriple tests three consecutive points for change of direction
 * and for orientation.
 */
#define CheckTriple							\
  thisDir =  Compare(second, third);                            \
  if ( thisDir == (-curDir) )	                            	\
    ++dirChanges;						\
  curDir = thisDir;                                             \
  thisSign = WhichSide(first, second, third);			\
	if ( thisSign ) {		\
	    if ( angleSign == -thisSign )				\
		{                                               \
		   return(NotConvex);					\
		 }                                             \
	    angleSign = thisSign;					\
	}								\
	first = second; second = third;

static Point2d CheckPoly[MAX_DISC_SIZE+10];

static PolygonClass ConvexTest(uint16 *poly, PointTag *points)
{
  uint16 nVerts, index, i;
  PolygonClass pClass;
  int		 curDir, thisDir, thisSign, angleSign = 0, dirChanges = 0;
  Point2d	 first, second, third, saveFirst, saveSecond;
  Plane plane;
  Point point;
  float x, y;

  nVerts = poly[0];

  PlaneEquation(poly, points, plane, TRUE, 0);

  if(nVerts > MAX_DISC_SIZE)
  	return(NotConvex);
  
  for (i=1; i<=nVerts; i++)
    {
      index = UVs[poly[i]].Geometry;
      point[0] = points[index].p.x;
      point[1] = points[index].p.y;
      point[2] = points[index].p.z;
      Project(plane, point, &x, &y);
      CheckPoly[i-1].x = x;
      CheckPoly[i-1].y = y;
    }
  
  first = CheckPoly[0];
  index = 1;
  if (!GetDifferentPoint(CheckPoly, &index, nVerts, first, &second))
    pClass = ConvexDegenerate;
  else
    {
      saveFirst = first;
      saveSecond = second;
      curDir = Compare(first, second);
      while( GetDifferentPoint(CheckPoly, &index, nVerts, second, &third))
	{
	  CheckTriple;
	}
      
      /* Must check that end of list continues back to start properly */
      if ( Compare(second, saveFirst) ) {
	third = saveFirst; 
	CheckTriple;
      }
      third = saveSecond;
      CheckTriple;
      
      if ( dirChanges > 2 ) 
	pClass =  angleSign ? NotConvex : NotConvexDegenerate;
      else if ( angleSign  > 0 ) 
	pClass = ConvexCCW;
      else if ( angleSign  < 0 ) 
	pClass =  ConvexCW;
      else pClass = ConvexDegenerate;
    }


  switch ( pClass )
    {
    case NotConvex:	
    /*  fprintf(stderr,"Not Convex\n"); */
      return (NotConvex);
      break;
    case NotConvexDegenerate:
    /*  fprintf( stderr,"Not Convex Degenerate\n"); */
      return(NotConvexDegenerate);
      break;
    case ConvexDegenerate:
      return(ConvexDegenerate); 
      break;
    case ConvexCCW:
      return(ConvexCCW);
      break;
    case ConvexCW:
      return(ConvexCW); 
      break;
	default:
	  return (NotConvex);
	  break;
    }
}

static void IFF_SkipChunk(FILE *filePtr, uint32 nBytes)
{
  char buf[2];
  uint32 i;
  
  if (nBytes %2)  /* IFF requires even byte alignment */
    nBytes++;
  for (i=0; i<nBytes; i+=2)
    qReadByteStream(filePtr,(BYTE *)buf,2);
}

static M2Err IFF_ReadULong(FILE *filePtr, uint32 *size)
{
  uint8  buf[4];
  
  qReadByteStream(filePtr,buf,4);
  *size = (((uint32)buf[0])<<24) + (((uint32)buf[1])<<16) + 
	  (((uint32)buf[2])<<8) + buf[3];
  return(M2E_NoErr);
}

static M2Err IFF_ReadUShort(FILE *filePtr, uint16 *size)
{
  uint8  buf[2];

  qReadByteStream(filePtr,buf,2);
  *size = (((uint16)buf[0])<<8) + buf[1];
  return(M2E_NoErr);
}

static M2Err IFF_ReadShort(FILE *filePtr, int16 *size)
{
  int8  buf[2];
  
  qReadByteStream(filePtr,(BYTE *)buf,2);
  *size = 0 | buf[0];
  *size = (*size)<<8;
  *size = (*size) | buf[1];
  return(M2E_NoErr);
}

static void memSkipChunk(uint32 nBytes)
{
  char buf[2];
  uint32 i;
  
  if (nBytes %2)  /* IFF requires even byte alignment */
    nBytes++;
  for (i=0; i<nBytes; i+=2)
    qMemGetBytes((BYTE *)buf,2);
}

static int16 getMemShort()
{
  uint8 buf[2];
  int16 size=0;

  qMemGetBytes((BYTE *)buf,2);
  size = (buf[0]<<8) | buf[1];
  return(size);
}


static uint16 getMemUShort()
{
  uint8 buf[2];
  
  qMemGetBytes((BYTE *)buf,  2);
  return( (buf[0] << 8) + (buf[1] <<0)); 
}


static uint32 getMemLong()
{
  uint8 buf[4];
  int32 size;

  qMemGetBytes((BYTE *)buf,4);
  size = buf[0];
  size = (size)<<8;
  size = (size) | buf[1];
  size = (size)<<8;
  size = (size) | buf[2];
  size = (size)<<8;
  size = (size) | buf[3];
  return(size);
}

static uint32 getMemULong()
{
  uint8 buf[4];

	qMemGetBytes((BYTE *)buf,  4);
	return (((uint32) buf[0]) << 24) + (((uint32) buf[1]) << 16)
    + (((uint32) buf[2]) << 8) + buf[3];
}

static M2Err getMemFloat(gfloat *size)
{
  uint8  buf[4];
#ifdef INTEL
  uint8 temp;
#endif

  qMemGetBytes((BYTE *)buf,4);
#ifdef INTEL
  temp = buf[0];  buf[0] = buf[3];  buf[3] = temp;
  temp = buf[1];  buf[1] = buf[2];  buf[2] = temp;
#endif
  memcpy(size, buf,4);
  return(M2E_NoErr);
}

static M2Err getMemString( uint16 *nameLen, char *nameBuf)
{
  *nameLen=0;
  do {
    qMemGetBytes((BYTE *)&(nameBuf[*nameLen]),2);
    *nameLen +=2;
  } while ((nameBuf[*nameLen-1]) != '\0');
  return(M2E_NoErr);
}

static M2Err IFF_ReadString(FILE *filePtr,
		     int16 *nameLen,
		     char *nameBuf)
{
  *nameLen=0;
  do {
    qReadByteStream(filePtr, (BYTE *)&(nameBuf[*nameLen]),2);
    *nameLen +=2;
  } while ((nameBuf[*nameLen-1]) != '\0');
  return(M2E_NoErr);
}

static M2Err IFF_ReadFloat(FILE *filePtr,
		    gfloat *size)
{
  uint8  buf[4];

  qReadByteStream(filePtr,buf,4);
  /* Fix this for Intel later */
  memcpy(size, buf,4);
  return(M2E_NoErr);
}

static M2Err LWOB_Open(char *fileName, 
		FILE **filePtr, 
		uint32 *size)
{
  uint32 WrapperChunkType = 'FORM';
  uint32 FormType = 'LWOB';
  uint32 wrapper;
  uint32 form;

  *filePtr = qOpenReadFile(fileName);
  if ((*filePtr) !=NULL)
    {
      qReadByteStream(*filePtr, (BYTE *)&wrapper,4);
      IFF_ReadULong(*filePtr,size);
      qReadByteStream(*filePtr, (BYTE *)&form,4);
      if (wrapper == WrapperChunkType)
	if (form == FormType)
	  return(M2E_NoErr);
      return(M2E_BadFile);
    }
  else
    return(M2E_NoFile);
}

static M2Err LWOB_ReadPOLS(PropChunk *pc, 
		    int16 nSurfs, 
		    int16 **polsChunk, 
		    uint32 *polsChunkSize,
		    uint16 **nSurfPolys, 
		    uint16 **nSurfDPolys)
{
 uint32 chunkSize;
 void *dataPtr;
 uint32 nShorts, i;
 int16 *chunkPtr;
 uint16 *nSP, *nSDP;
 int16 nVerts, nDetails, tempShort; 
 bool inPoly,inDetail;
 
 chunkSize = pc->pc_DataSize;
 dataPtr = pc->pc_Data;
 BeginMemGetBytes((BYTE *)dataPtr);

 *polsChunkSize = chunkSize;

 /*
  * We have to read in the chunk as bytes because 
  */
 *polsChunk = (int16 *)qMemNewPtr(chunkSize);
 nSP = *nSurfPolys = (uint16 *)qMemNewPtr(nSurfs*sizeof(uint16));
 nSDP = *nSurfDPolys = (uint16 *)qMemNewPtr(nSurfs*sizeof(uint16));
 for (i=0; i<nSurfs; i++)
   {
     nSP[i]=0;
     nSDP[i]=0;
   }
 if ((*polsChunk) != NULL)
   {
     inPoly = inDetail = FALSE;
     nDetails = 0;
     nShorts = chunkSize/2;
     chunkPtr = *polsChunk;
     for (i=0; i<nShorts; i++)
       {
	 tempShort = getMemShort();
	 *chunkPtr = tempShort;
	 chunkPtr++;
	 if (!inPoly)
	   {
	     /* Either starting a new Poly or the Detail Polygons */
	     if ((inDetail)&&(nDetails==0))
	       nDetails = tempShort;
	     else
	       {
		 nVerts = tempShort;
		 inPoly = TRUE;
	       }
	   }
	 else
	   {
	     if (nVerts <= 0)  /* The next short is a surface */
	       {
		 inPoly = FALSE;
		 if (tempShort<0)
		   {
/*		     fprintf(stderr,"Warning! We have detail polygons in Surface %d\n", -tempShort);
*/
		     inDetail = TRUE;
		     nSP[(-tempShort)-1] = nSP[(-tempShort)-1]+1;
		   }
		 else
		   {
		     if (inDetail)
		       {
			 nSDP[tempShort-1] = nSDP[tempShort-1]+1;
			 nDetails--;
			 if (nDetails <=0)
			   inDetail = FALSE;
		       }
		     else
		       nSP[tempShort-1] = nSP[tempShort-1]+1;
		   }
	       }
	     else
	       nVerts--;
	   }
       }
   }
 else
   return (M2E_NoMem);

 return (M2E_NoErr);
}

static M2Err LWOB_ReadSRFS(PropChunk *pc,
		    char **srfsChunk,
		    char ***namePtrs, 
		    int16 *nSurfs)
{
  uint32 i;
  uint32 chunkSize;
  bool inString;
  char *namePtr;
  char **tmpPtr;
  
  chunkSize = pc->pc_DataSize;
  *srfsChunk = pc->pc_Data;
  *nSurfs = 0;
  
  inString = FALSE;
  for (i=0, namePtr = *srfsChunk; i<chunkSize; i++, namePtr++)
    if (*namePtr != '\0')
      inString = TRUE;
    else if (inString)
      {
	inString = FALSE;
	(*nSurfs)++;
      }
  /*
   * Get the name Ptrs
   */
  inString = FALSE;
  *namePtrs = (char **)qMemNewPtr((*nSurfs)*sizeof(char *));
  if ((*namePtrs) != NULL)
    {
      tmpPtr = *namePtrs;
      for (i=0, namePtr = *srfsChunk; i<chunkSize; i++, namePtr++)
	if (*namePtr != '\0')
	  {
	    if (!inString)
	      {
		*tmpPtr = namePtr;
		tmpPtr++;
		inString = TRUE;
	      }
	  }
	else if (inString)
	  inString = FALSE;
      return (M2E_NoErr);
    }
  else
    return (M2E_NoMem);
}

static int32 LWOB_FindNamePtr(char *name, char **namePtrs, uint32 nSurfs)
{
  uint32 i;

  for (i=0; i<nSurfs; i++)
    {
      if (!(strcmp(name, namePtrs[i])))
	return(i); 
    }
  return(-1);

}

static M2Err LW_ParseSURF(void *dataPtr, LWSURF *surfs, int32 surfOff, 
			  uint32 count, uint32 chunkSize)
{
  LWTex tempTex;
  LWTex *curTex;
  uint16 data, flags, diff, lumi, glos, spec, tran, refl;
  gfloat fdata;
  uint32 subChunk;
  uint16 subChunkSize, nameLen;
  uint8 colorBuf[4], nameBuf[512];
  Point3 pdata;
  uint32 byteCount=count;
  char chunkID[5];

  curTex = &tempTex;
  
  BeginMemGetBytes((BYTE *)dataPtr);

  while (byteCount<chunkSize)
    {
      /*       qMemGetBytes((BYTE *)&subChunk,4); */
      subChunk = getMemULong();
      subChunkSize = getMemUShort();
      byteCount += 6 + subChunkSize;
      if (surfOff < 0)
	{
      /*
	  fprintf(stderr,"WARNING:Doing a memSkipChunk because surfOff=%d\n",surfOff);
	  */
	  if (subChunkSize >0)
	    memSkipChunk(subChunkSize);
	}
      else
	{
	  switch (subChunk)
	    {
	    case 'COLR':
	      qMemGetBytes((BYTE *)colorBuf,4);
	      LWSURF_SetCOLR(&(surfs[surfOff]), colorBuf);
	      break;
	    case 'FLAG':
	      /* qMemGetBytes((BYTE *)&flags,2);*/
	      flags = getMemUShort();
	      LWSURF_SetFLAG(&(surfs[surfOff]), flags);
	      break;
	    case 'LUMI':
	      /* qMemGetBytes((BYTE *)&lumi,2); */
	      lumi = getMemUShort();
	      LWSURF_SetLUMI(&(surfs[surfOff]), lumi);
	      break;
	    case 'DIFF':
	      /* qMemGetBytes((BYTE *)&diff,2); */
	      diff = getMemUShort();
	      LWSURF_SetDIFF(&(surfs[surfOff]), diff);
	      break;
	    case 'SMAN':
	      getMemFloat(&fdata);
	      LWSURF_SetSMAN(&(surfs[surfOff]), fdata);
	      break;
	    case 'GLOS':
	      /* qMemGetBytes((BYTE *)&glos,2); */
	      glos = getMemUShort();
	      LWSURF_SetGLOS(&(surfs[surfOff]), glos);
	      break;
	    case 'SPEC':
	      /* qMemGetBytes((BYTE *)&spec,2); */
	      spec = getMemUShort();
	      LWSURF_SetSPEC(&(surfs[surfOff]), spec);
	      break;
	    case 'REFL':
	      /* qMemGetBytes((BYTE *)&spec,2); */
	      refl = getMemUShort();
	      LWSURF_SetREFL(&(surfs[surfOff]), refl);
	      break;
	    case 'TRAN':
	      /* qMemGetBytes((BYTE *)&tran,2); */
	      tran = getMemUShort();
	      LWSURF_SetTRAN(&(surfs[surfOff]), tran);
	      break;
	    case 'TFLG':
	      /* qMemGetBytes((BYTE *)&data,2); */
	      data = getMemUShort();
	      LWTex_SetTFLG(curTex, data);
	      break;
	    case 'TVAL':
	      /* qMemGetBytes((BYTE *)&data,2); */
	      data = getMemUShort();
	      LWTex_SetTVAL(curTex, data);
	      break;
	    case 'TFRQ':
	      data = getMemUShort();
	      /* qMemGetBytes((BYTE *)&data,2); */
	      LWTex_SetTFRQ(curTex, data);
	      break;
	    case 'TSIZ':
	      getMemFloat(&pdata.x);
	      getMemFloat(&pdata.y);
	      getMemFloat(&pdata.z);
	      LWTex_SetTSIZ(curTex, pdata);
	      break;
	    case 'TCTR':
	      getMemFloat(&pdata.x);
	      getMemFloat(&pdata.y);
	      getMemFloat(&pdata.z);
	      LWTex_SetTCTR(curTex, pdata);
	      break;
	    case 'TFAL':	
	      getMemFloat(&pdata.x);
	      getMemFloat(&pdata.y);
	      getMemFloat(&pdata.z);
	      LWTex_SetTFAL(curTex, pdata);
	      break;
	    case 'TVEL':
	      getMemFloat(&pdata.x);
	      getMemFloat(&pdata.y);
	      getMemFloat(&pdata.z);
	      break;
	    case 'TAMP':
	      getMemFloat(&fdata);
	      LWTex_SetTAMP(curTex, fdata);
	      break;
	    case 'TFP0':
	      getMemFloat(&fdata);
	      LWTex_SetTFP0(curTex, fdata);
	      break;
	    case 'TFP1':
	      getMemFloat(&fdata);
	      LWTex_SetTFP1(curTex, fdata);
	      break;
	    case 'TSP0':
	      getMemFloat(&fdata);
	      LWTex_SetTSP0(curTex, fdata);
	      break;
	    case 'TSP1':
	      getMemFloat(&fdata);
	      LWTex_SetTSP1(curTex, fdata);
	      break;
	    case 'TSP2':
	      getMemFloat(&fdata);
	      LWTex_SetTSP2(curTex, fdata);
	      break;
	    case 'TIMG':
	      getMemString(&nameLen, (char *)nameBuf);
	      LWTex_SetTIMG(curTex, nameLen+2, (char *)nameBuf);
	      break;
	    case 'RIMG':
	      getMemString(&nameLen, (char *)nameBuf);
	      LWSURF_SetRIMG(&(surfs[surfOff]), nameLen+2, (char *)nameBuf);
	      break;
	    case 'CTEX':
	      getMemString(&nameLen, (char *)nameBuf);
	      LWSURF_SetCTEX(&(surfs[surfOff]), nameLen+2, (char *)nameBuf, &curTex);
	      break;
	    case 'DTEX':
	      getMemString(&nameLen, (char *)nameBuf);
	      LWSURF_SetDTEX(&(surfs[surfOff]), nameLen+2, (char *)nameBuf, &curTex);
	      break;
	    case 'STEX':
	      getMemString(&nameLen, (char *)nameBuf);
	      LWSURF_SetSTEX(&(surfs[surfOff]), nameLen+2, (char *)nameBuf, &curTex);
	      break;
	    case 'RTEX':
	      getMemString(&nameLen, (char *)nameBuf);
	      LWSURF_SetRTEX(&(surfs[surfOff]), nameLen+2, (char *)nameBuf, &curTex);
	      break;
	    case 'TTEX':
	      getMemString(&nameLen, (char *)nameBuf);
	      LWSURF_SetTTEX(&(surfs[surfOff]), nameLen+2, (char *)nameBuf, &curTex);
	      break;
	    case 'BTEX':
	      getMemString(&nameLen, (char *)nameBuf);
	      LWSURF_SetBTEX(&(surfs[surfOff]), nameLen+2, (char *)nameBuf, &curTex);
	      break;
	    default:
	      ID_Decode(subChunk, chunkID);
	      /*
		if (CommentLevel > 4)
		fprintf(stderr,"WARNING:Unknown SubChunk \"%s\" =%x of size %d\n", 
		chunkID, subChunk, subChunkSize);
		*/
	      if (subChunkSize >0)
		memSkipChunk(subChunkSize);
	      break;
	    }
	}
    }
  EndMemGetBytes();
  return(M2E_NoErr);
}

static M2Err LWOB_ReadSURF(CollectionChunk *cc, 
		    char **namePtrs,
		    LWSURF *surfs,
		    uint32 nSurfs)
{
  uint32 chunkSize, byteCount;
  int32  surfOff;
  uint16 nameLen;
  uint8  nameBuf[512];
  void *dataPtr;

  chunkSize = cc->cc_DataSize;
  dataPtr = cc->cc_Data;
  BeginMemGetBytes((BYTE *)dataPtr);
  
  getMemString(&nameLen, (char *)nameBuf);

  surfOff = LWOB_FindNamePtr((char *)nameBuf, namePtrs, nSurfs);
  
  byteCount = nameLen;
  EndMemGetBytes();

  dataPtr = (void *)((BYTE *)dataPtr + nameLen);
  return(LW_ParseSURF(dataPtr, surfs, surfOff, byteCount, chunkSize));
}


static M2Err Materials_Process(LWSURF *surfs,
			       int16 nSurfs,
			       char **namePtrs)
			       
{
  int32 surfOff;
  bool validMaterial;
  SDFMat tempMat;
  int matIndex;
  M2Err err;

  
  for (surfOff=0; surfOff<nSurfs; surfOff++)
    {
      validMaterial = FALSE;
      matIndex = -1;
      SDFMat_Init(&tempMat);
      err = LWSURF_ToSDFMat(surfs[surfOff], &tempMat, namePtrs[surfOff]);
      if (err != M2E_NoErr)
	{
	  fprintf(stderr,"WARNING:Invalid Materials \"%s\"\n", namePtrs[surfOff]);
	  validMaterial = FALSE;
	}
      else 
	validMaterial = TRUE;

      if (validMaterial)
	{
	  err = SDFMat_Add(&tempMat, &GMaterials, &NMaterials, &CurMaterial, TRUE,
			   &matIndex);
	  if (err!=M2E_NoErr)
	    return(err);
	}	  
      surfs[surfOff].MatIndex = matIndex;
    }
  
  return(M2E_NoErr);
}


static M2Err Materials_Print(FILE *fPtr,
			       char *fileIn
			       )
			       
{
  int i;

  if (CurMaterial > 0)
    {
      if (matAppend)
	{
	  fprintf(fPtr,"SDFVersion 1.0\n\n");
	  fprintf(fPtr,"Define MatArray \"%s\" {\n",fileIn);
	}
      else
	{
	  if (SepMatTex)
	    fprintf(fPtr,"SDFVersion 1.0\n\n");
	  fprintf(fPtr,"Define MatArray \"%s_materials\" {\n",fileIn);
	}

      for (i=0; i<CurMaterial; i++)
	{
	  SDFMat_Print(&(GMaterials[i]), fPtr, 1);
	}
      fprintf(fPtr,"}\n");
    }
  
  if (ferror(fPtr))
    {
      fprintf(stderr,"WARNING:Error detected on SDF MatArray output.  Disk may be full.\n");
    }
  
  return(M2E_NoErr);
}

static M2Err LWOB_ReadPNTS(PropChunk *pc,  
		    PointTag **points,
		    uint32 *nPoints)
{
  uint32 i;
  uint32 chunkSize;
  void *dataPtr;
  PointTag *pntPtr; 
  
  chunkSize = pc->pc_DataSize;
  *nPoints = chunkSize / 12;
  dataPtr = pc->pc_Data;
  BeginMemGetBytes((BYTE *)dataPtr);
  *points = (PointTag *)qMemClearPtr((*nPoints),sizeof(PointTag));
  if ((*points) != NULL)
    {
      pntPtr = *points;
      for (i=0; i<(*nPoints); i++)
	{
	  getMemFloat(&(pntPtr->p.x));
	  getMemFloat(&(pntPtr->p.y));
	  getMemFloat(&(pntPtr->p.z));
	  pntPtr->inUse = FALSE;
	  pntPtr->UVCount = 0;
	  pntPtr->UVSize = 1;
	  pntPtr->IsNow = i;
	  pntPtr++;
	}
      return (M2E_NoErr);
    }
  else
    return (M2E_NoMem);
}

float polyMinU, polyMaxU, polyMinV, polyMaxV;

static uint16 RegisterPoint(bool isTextured, int pInPoly, uint16 index, 
			    uint16 *surfPoly, PointTag *points, LWSURF *surf)
{
  float u, v, tmpFloat;
   int uCrossing, vCrossing, uvCount, gUVCount;
  uint16 temp16;

  u = v = 0.0;
  if (isTextured)
    {
      TransformPt(surfPoly, points, pInPoly, index, &u, &v, 
		  surf->UsedTEX, &polyMinU, &polyMaxU, &polyMinV, &polyMaxV, 
		  &uCrossing, &vCrossing, FALSE);
    }

  /* Add the point to the list of UV coords if it has any */
  uvCount = points[index].UVCount;
  points[index].UVCount++;
  if (uvCount >= points[index].UVSize)
    {
      if (points[index].UVSize == 1)  /* Special Case */
	{
	  temp16 = points[index].UVOnly;
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

  gUVCount = UVCount;
  UVCount++;
  if (UVCount >= UVSize)
    {
      UVSize += UVIncrement;
      UVs = (UVCoord *)qMemResizePtr(UVs, UVSize*sizeof(UVCoord));
    }
  if (points[index].UVSize == 1)  /* Special Case */
    {
      points[index].UVOnly = gUVCount;
    }
  else
    {
      points[index].UVList[uvCount] = gUVCount;
    }
  
  if (isTextured)
    {
      if (LWTex_GetTFP0(*(surf->UsedTEX), &tmpFloat))
	{
	  u = u*tmpFloat;
	}
      if (LWTex_GetTFP1(*(surf->UsedTEX), &tmpFloat))
	{
	  v = v*tmpFloat;
	}
      if (u < surf->MinU)
	surf->MinU = u;
      if (v < surf->MinV)
	surf->MinV = v;
      if (u > surf->MaxU)
	surf->MaxU = u;
      if (v > surf->MaxV)
	surf->MaxV = v;
      UVs[gUVCount].U = u;
      UVs[gUVCount].V = v;
    }

  UVs[gUVCount].Geometry = index;
  return(gUVCount);
}

static void PrintUV(FILE *fPtr, int tabLevel, uint16 index)
{
  int i;

  for(i=0; i<tabLevel; i++)
    fprintf(fPtr,"\t");
  fprintf(fPtr,"{ %f %g }\n", UVs[index].U, UVs[index].V);
}
/*
static void PrintPoint(FILE *fPtr, int tabLevel, PointTag point)
{
  int i;

  for(i=0; i<tabLevel; i++)
    fprintf(fPtr,"\t");
  if (setScale == 0)
    {
      fprintf(fPtr,"{ %g %g %g }\n", point.p.x, point.p.y, point.p.z);
    }
  else
    {
      fprintf(fPtr,"{ %g %g %g }\n", point.p.x, point.p.y, -point.p.z);
    }
}
*/

#define PrintPoint(fPtr, tabLevel, point) \
 for(printLoop=0; printLoop<(tabLevel); printLoop++) \
   fprintf(fPtr,"\t"); \
 if (setScale == 0) \
   fprintf(fPtr,"{ %g %g %g }\n", (point).p.x, (point).p.y, (point).p.z); \
 else \
   fprintf(fPtr,"{ %g %g %g }\n", (point).p.x, (point).p.y, -(point).p.z);

static M2Err LWOB_WriteSurfaceFlat(FILE *fPtr,
			    int16 nPolys,
			    uint16 **surfPolys,
			    PointTag *points,
			    LWSURF  *surface)
{
  bool inTriangles, inPoly, noPoly, skipPoly;
  bool inConvex, isTextured, regular, disc;
  uint32 j, index;
  int l, k, printLoop;
  PolygonClass test;
  triIndex *triPoly;

  inTriangles = FALSE;
  inPoly = FALSE;
  noPoly = TRUE;
  inConvex = FALSE;
  
  UVCount = 0;
  
  if (UseDummyTex)
    {
      if (surface->TexIndex > 0)
	{
	  if (surface->EnvMap == TRUE)
	    isTextured = FALSE;
	  else
	    isTextured = TRUE;
	}
      else
	isTextured = FALSE;
    }
  else
    {
      if (surface->TexIndex >= 0)
	{
	  if (surface->EnvMap == TRUE)
	    isTextured = FALSE;
	  else
	    isTextured = TRUE;
	}
      else
	isTextured = FALSE;
    }
  
  for (j=0; j<nPolys; j++)
    { 
      polyMinU = polyMinV = 10000;
      polyMaxU = polyMaxV = -10000;
      for (k=1; k<=surfPolys[j][0]; k++)
	{
	  index = surfPolys[j][k];
	  surfPolys[j][k] = RegisterPoint(isTextured, k, index, surfPolys[j], points, surface);
	}
    }
  for (j=0; j<nPolys; j++)
    {
      skipPoly = FALSE;
      if (surfPolys[j][0] > 2)
	{
	  noPoly = FALSE;
	  if (!inTriangles)
	    {
	      /* Close up the current structure */
	      if (inPoly || inTriangles)
		fprintf(fPtr,"\t\t\t\t}\n\t\t\t}\n\t\t}\n");
	      fprintf(fPtr,"\t\tTriangles {\n");
	      fprintf(fPtr,"\t\t\tvertexList {\n");
	      fprintf(fPtr,"\t\t\t\tformat( Locations ");
	      if (isTextured)
		fprintf(fPtr,"| TexCoords ");
	      fprintf(fPtr,")\n");
	      fprintf(fPtr,"\t\t\t\tlocations {\n");
	      inPoly = FALSE;
	      inTriangles = TRUE;
	    } 
	  
	  regular = TRUE;
	  if (surfPolys[j][0] >= 3)
	    {
	      if (surfPolys[j][0] > 3)
		{
		  if (testConvex)
		    test = ConvexTest(surfPolys[j], points);
		  else 
		    test = ConvexCW;
		}
	      else
		test = ConvexCW;
	      
	      if ((test != ConvexDegenerate) &&
		  (test != ConvexCCW) && (test != ConvexCW))
		{
		  if (test == NotConvex)
		    {
		      disc = FALSE;
		      disc = DiscDetect(surfPolys[j]);
		      
		      if (!disc)
			regular = TRUE;
		      else
			{
			  regular = FALSE;
			  fprintf(fPtr,"\n# Double disc \n\t\t\t\t",
				  surfPolys[j][0]);
			  
			  /* Each disc is decomposed into two discs */
			  for (k=1; k<(surfPolys[j][0]/2)-1; k++)
			    {
			      index = Disc_List[0];
			      index = UVs[index].Geometry;
			      PrintPoint(fPtr, 5, points[index]);
			      index = Disc_List[k];
			      index = UVs[index].Geometry;
			      PrintPoint(fPtr, 5, points[index]);
			      index = Disc_List[k+1];
			      index = UVs[index].Geometry;
			      PrintPoint(fPtr, 5, points[index]);
			    }
			  
			  for (k=(surfPolys[j][0]/2)-2; k>=1; k--)
			    {
			      index = Disc_List[(surfPolys[j][0]/2)-1];
			      index = UVs[index].Geometry;
			      PrintPoint(fPtr, 5, points[index]);
			      index = Disc_List[k];
			      index = UVs[index].Geometry;
			      PrintPoint(fPtr, 5, points[index]);
			      index = Disc_List[k-1];
			      index = UVs[index].Geometry;
			      PrintPoint(fPtr, 5, points[index]);
			      fprintf(fPtr, "%d ", Disc_List[k]);
			    }
			}
		      if (!disc)
			{
			  fprintf(fPtr,"\n#Non-convex of sides %d\n",surfPolys[j][0]);
			  regular = FALSE;
			  
/*
			  fprintf(stderr,"Surfs:");
			  for (k=1; k<=surfPolys[j][0]; k++)
			    fprintf(stderr," %d",surfPolys[j][k]);
			  fprintf(stderr,"\n");
*/			  
			  if (triangulate(surfPolys[j], points, &triPoly))
			    {
			      regular = FALSE;
			      fprintf(fPtr,"#Decomposed into %d Triangles\n",
				      surfPolys[j][0]-2);
			      for (k=0; k<surfPolys[j][0]-2; k++)
				{
				  if (vertexOrder == 1)
				    {   
				      /* From Left to Right Coordinates */
				      for(l=0; l<3; l++)
					{
					  index = triPoly[k][l]+1;
					  if (index > surfPolys[j][0])
					    index = 1;
					  index = surfPolys[j][index];
					  index = UVs[index].Geometry;
					  PrintPoint(fPtr, 5, points[index]);
					}
				    }
				  else
				    {
				      for(l=2; l>=0; l--)
					{		
					  index = triPoly[k][l] + 1;
					  if (index > surfPolys[j][0])
					    index = 1;
					  index = surfPolys[j][index];
					  index = UVs[index].Geometry;
					  PrintPoint(fPtr, 5, points[index]);
					}
				    }
/*				  fprintf(stderr,"\n"); */
				}
			    }
			  else
			    {
			      fprintf(fPtr,"\n# Self-intersecting polygon that can't be triangulated, Poly %d \n\t\t\t\t",j);
			      regular = TRUE;
			    }
			}
		    }
		  if (regular) /* Business as usual */
		    {
		      if (surfPolys[j][0] > 3)
			fprintf(fPtr,"\n\t\t\t\t#%d Sided Polygon\n", 
				surfPolys[j][0]);
		      if (vertexOrder == 1)
			{   /* From Left to Right Coordinates */
			  for (k=2 ; k <= (surfPolys[j][0]-1); k++) 
			    {
			      index = surfPolys[j][1];
			      index = UVs[index].Geometry;
			      PrintPoint(fPtr, 5, points[index]);
			      index = surfPolys[j][k];
			      index = UVs[index].Geometry;
			      PrintPoint(fPtr, 5, points[index]);
			      index = surfPolys[j][k+1];
			      index = UVs[index].Geometry;
			      PrintPoint(fPtr, 5, points[index]);
			    }			      
			}
		      else
			{  /* From Left to Right Coordinates */
			  for (k=surfPolys[j][0]-1; k>1; k--)
			    {
			      index = surfPolys[j][surfPolys[j][0]];
			      index = UVs[index].Geometry;
			      PrintPoint(fPtr, 5, points[index]);
			      index = surfPolys[j][k];
			      index = UVs[index].Geometry;
			      PrintPoint(fPtr, 5, points[index]);
			      index = surfPolys[j][k-1];
			      index = UVs[index].Geometry;
			      PrintPoint(fPtr, 5, points[index]);
			    }
			}
		    }
		}
	      else
		{
		  if (vertexOrder == 1)
		    {   /* From Left to Right Coordinates */
		      for (k=2 ; k <= (surfPolys[j][0]-1); k++) 
			{
			  index = surfPolys[j][1];
			  index = UVs[index].Geometry;
			  PrintPoint(fPtr, 5, points[index]);
			  index = surfPolys[j][k];
			  index = UVs[index].Geometry;
			  PrintPoint(fPtr, 5, points[index]);
			  index = surfPolys[j][k+1];
			  index = UVs[index].Geometry;
			  PrintPoint(fPtr, 5, points[index]);
			}
		    }
		  else
		    {  /* From Left to Right Coordinates */
		      for (k=surfPolys[j][0]-1; k>1; k--)
			{
			  index = surfPolys[j][surfPolys[j][0]];
			  index = UVs[index].Geometry;
			  PrintPoint(fPtr, 5, points[index]);
			  index = surfPolys[j][k];
			  index = UVs[index].Geometry;
			  PrintPoint(fPtr, 5, points[index]);
			  index = surfPolys[j][k-1];
			  index = UVs[index].Geometry;
			  PrintPoint(fPtr, 5, points[index]);
			}
		    }
		}
	    }
	}
    }
  
   inTriangles = FALSE;
   inConvex = FALSE;

   if ((isTextured) && (UVCount > 0))
     {
       for (j=0; j<nPolys; j++)
	 {
	   skipPoly = FALSE;
	   if (surfPolys[j][0] > 2)
	     {
	       noPoly = FALSE;
	       /* Close up the current structure */
	       regular = TRUE;
	       if (surfPolys[j][0] >= 3)
		 {
		   if (!inTriangles)
		     {
		       fprintf(fPtr,"\t\t\t\t}\n");
		       fprintf(fPtr,"\t\t\t\ttexcoords {\n\t\t\t\t");
		       inTriangles = TRUE;
		     }
		   if (surfPolys[j][0] > 3)
		     {
		       if (testConvex)
			 test = ConvexTest(surfPolys[j], points);
		       else 
			 test = ConvexCW;
		     }
		   else
		     test = ConvexCW;

		   if ((test != ConvexDegenerate) &&
		       (test != ConvexCCW) && (test != ConvexCW))
		     {
		       if (test == NotConvex)
			 {
			   disc = FALSE;
			   disc = DiscDetect(surfPolys[j]);

			   if (!disc)
			     regular = TRUE;
			   else
			     {
			       regular = FALSE;
			       fprintf(fPtr,"\n# Double disc \n\t\t\t\t",
				       surfPolys[j][0]);

			       /* Each disc is decomposed into two discs */
			       for (k=1; k<(surfPolys[j][0]/2)-1; k++)
				 {
				   index = Disc_List[0];
				   PrintUV(fPtr, 5, index);
				   index = Disc_List[k];
				   PrintUV(fPtr, 5, index);
				   index = Disc_List[k+1];
				   PrintUV(fPtr, 5, index);
				 }

			       for (k=(surfPolys[j][0]/2)-2; k>=1; k--)
				 {
				   index = Disc_List[(surfPolys[j][0]/2)-1];
				   PrintUV(fPtr, 5, index);
				   index = Disc_List[k];
				   PrintUV(fPtr, 5, index);
				   index = Disc_List[k-1];
				   PrintUV(fPtr, 5, index);
				   fprintf(fPtr, "%d ", Disc_List[k]);
				 }
			     }
			   if (!disc)
			     {
			       fprintf(fPtr,"\n#Non-convex of sides %d\n",surfPolys[j][0]);
			       regular = FALSE;

			       /*
				  fprintf(stderr,"Surfs:");
				  for (k=1; k<=surfPolys[j][0]; k++)
				  fprintf(stderr," %d",surfPolys[j][k]);
				  fprintf(stderr,"\n");
				  */			  
			       if (triangulate(surfPolys[j], points, &triPoly))
				 {
				   regular = FALSE;
				   fprintf(fPtr,"#Decomposed into %d Triangles\n",
					   surfPolys[j][0]-2);
				   for (k=0; k<surfPolys[j][0]-2; k++)
				     {
				       if (vertexOrder == 1)
					 {   
					   /* From Left to Right Coordinates */
					   for(l=0; l<3; l++)
					     {
					       index = triPoly[k][l]+1;
					       if (index > surfPolys[j][0])
						 index = 1;
					       index = surfPolys[j][index];
					       PrintUV(fPtr, 5, index);
					     }
					 }
				       else
					 {
					   for(l=2; l>=0; l--)
					     {		
					       index = triPoly[k][l] + 1;
					       if (index > surfPolys[j][0])
						 index = 1;
					       index = surfPolys[j][index];
					       PrintUV(fPtr, 5, index);
					     }
					 }
				       /*				  fprintf(stderr,"\n"); */
				     }
				 }
			       else
				 {
				   fprintf(fPtr,"\n# Self-intersecting polygon that can't be triangulated, Poly %d \n\t\t\t\t",j);
				   regular = TRUE;
				 }
			     }
			 }
		       if (regular) /* Business as usual */
			 {
			   if (surfPolys[j][0] > 3)
			     fprintf(fPtr,"\n\t\t\t\t#%d Sided Polygon\n", 
				     surfPolys[j][0]);
			   if (vertexOrder == 1)
			     {   /* From Left to Right Coordinates */
			       for (k=2 ; k <= (surfPolys[j][0]-1); k++) 
				 {
				   index = surfPolys[j][1];
				   PrintUV(fPtr, 5, index);
				   index = surfPolys[j][k];
				   PrintUV(fPtr, 5, index);
				   index = surfPolys[j][k+1];
				   PrintUV(fPtr, 5, index);
				 }			      
			     }
			   else
			     {  /* From Left to Right Coordinates */
			       for (k=surfPolys[j][0]-1; k>1; k--)
				 {
				   index = surfPolys[j][surfPolys[j][0]];
				   PrintUV(fPtr, 5, index);
				   index = surfPolys[j][k];
				   PrintUV(fPtr, 5, index);
				   index = surfPolys[j][k-1];
				   PrintUV(fPtr, 5, index);
				 }
			     }
			 }
		     }
		   else
		     {
		       if (vertexOrder == 1)
			 {   /* From Left to Right Coordinates */
			   for (k=2 ; k <= (surfPolys[j][0]-1); k++) 
			     {
			       index = surfPolys[j][1];
			       PrintUV(fPtr, 5, index);
			       index = surfPolys[j][k];
			       PrintUV(fPtr, 5, index);
			       index = surfPolys[j][k+1];
			       PrintUV(fPtr, 5, index);
			     }
			 }
		       else
			 {  /* From Left to Right Coordinates */
			   for (k=surfPolys[j][0]-1; k>1; k--)
			     {
			       index = surfPolys[j][surfPolys[j][0]];
			       PrintUV(fPtr, 5, index);
			       index = surfPolys[j][k];
			       PrintUV(fPtr, 5, index);
			       index = surfPolys[j][k-1];
			       PrintUV(fPtr, 5, index);
			     }
			 }
		     }
		 }
	     }
	 }
     }
  
  if (!noPoly)
    fprintf(fPtr,"\t\t\t\t}\n\t\t\t}\n\t\t}\n");

   for (j=0; j<UVCount; j++)
     {
       index = UVs[j].Geometry;
       points[index].inUse = FALSE;
       points[index].UVCount = 0;
       points[index].IsNow = index;		
     }

	return(M2E_NoErr);
}


static bool MakeUVs(bool isTextured, uint16 index, uint16 pt, uint16 *surfPoly,
		    PointTag *points, LWSURF *surf, bool uvTranslate)
{
  float u, v, tmpFloat;
  int uCrossing, vCrossing, uvPt, uvCount, gUVCount;
  uint16 uvIndex, temp16;
  bool skipPt;
  bool copyNorm = TRUE;

  u = v = 0.0;
  if (isTextured)
    {
      TransformPt(surfPoly, points, pt, index, &u, &v, surf->UsedTEX,
		  &polyMinU, &polyMaxU, &polyMinV, &polyMaxV, 
		  &uCrossing, &vCrossing, uvTranslate);
      if (LWTex_GetTFP0(*(surf->UsedTEX), &tmpFloat))
	{
	  u = u*tmpFloat;
	}
      if (LWTex_GetTFP1(*(surf->UsedTEX), &tmpFloat))
	{
	  v = v*tmpFloat;
	}
      if (u < surf->MinU)
	surf->MinU = u;
      if (v < surf->MinV)
	surf->MinV = v;
      if (u > surf->MaxU)
	surf->MaxU = u;
      if (v > surf->MaxV)
	surf->MaxV = v;
    }
  skipPt = FALSE;
  
  if ((!isTextured) && (uvTranslate))
    {
      if (points[index].inUse)
	return(skipPt=TRUE);
      else 
	return(skipPt=FALSE);
    }
  
  if (points[index].inUse)
    {
      if (isTextured)    /* If the point is in Use, it might have the wrong tex coords */
	{
	  if (uvTranslate)
	    {
	      temp16= surfPoly[pt];
	      if (!UVs[temp16].UVSet)
		{
		  UVs[temp16].U = u;
		  UVs[temp16].V = v;
		  UVs[temp16].UVSet = TRUE;	
		  skipPt = TRUE; 
		  /* It now is completed, don't skip, write it out */
		  return(skipPt);
		}
	      else
		copyNorm = TRUE;
	    }
	  for (uvPt=0; uvPt < points[index].UVCount; uvPt++)
	    {
	      if (points[index].UVSize==1)   /* Special Case */
		uvIndex = (uint16) points[index].UVOnly;
	      else
		uvIndex = points[index].UVList[uvPt];
	      if ((u==UVs[uvIndex].U) && (v==UVs[uvIndex].V))
		{
		  if (uvTranslate)
		    {
		      temp16 = surfPoly[pt];
		      if ((UVs[uvIndex].N.x == UVs[temp16].N.x)&&
			  (UVs[uvIndex].N.y == UVs[temp16].N.y)&&
			  (UVs[uvIndex].N.z == UVs[temp16].N.z))
			{
			  surfPoly[pt] = uvIndex;
		  skipPt = TRUE;    /* If it's here already, skip it */
		  break;
		}
	    }
		  else
		    {
		      surfPoly[pt] = uvIndex;
		      skipPt = TRUE;    /* If it's here already, skip it */
		      break;
		    }
		}
	    }
	}
      else
	{
	  surfPoly[pt] = points[index].IsNow;		  
	  skipPt = TRUE;  /* In use + no texturing = automatic skip it */
	}
    }
  if (!skipPt)
    {
      /* Add the point to the list of UV coords if it has any */
      uvCount = points[index].UVCount;
      points[index].UVCount++;
      if (uvCount >= points[index].UVSize)
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
      gUVCount = UVCount;
      UVCount++;
      if (UVCount >= UVSize)
	{
	  UVSize += UVIncrement;
	  UVs = (UVCoord *)qMemResizePtr(UVs, UVSize*sizeof(UVCoord));
	}
      if (points[index].UVSize == 1)  /* Special Case */
	{
	  points[index].UVOnly = gUVCount;
	}
      else
	{
	  points[index].UVList[uvCount] = gUVCount;
	}
      UVs[gUVCount].U = u;
      UVs[gUVCount].V = v;
      UVs[gUVCount].Geometry = index;
      UVs[gUVCount].UVSet = TRUE;
      if (copyNorm)
	{
	  uvIndex = surfPoly[pt];
	  UVs[gUVCount].N.x = UVs[uvIndex].N.x;
	  UVs[gUVCount].N.y = UVs[uvIndex].N.y;
	  UVs[gUVCount].N.z = UVs[uvIndex].N.z;
	}
    }

  return (skipPt);
}

#ifdef applec
void IFF_SpinCursor(void);
#endif
static M2Err LWOB_WriteSurfaceSmooth(FILE *fPtr, int16 nPolys,
				     uint16 **surfPolys, MyPointData *ptData,
			      LWSURF  *surface)

{
  register uint32 index, size, offset;
  bool inTriangles, inPoly, noPoly, disc, inConvex;
  bool regular,  isTextured, skipPt;
  uint32 j, uniquePoints, oldPolys;
  int k, l;
  PolygonClass test;
  triIndex *triPoly;
  float  polyMinU, polyMaxU, polyMinV, polyMaxV;
  gfloat smoothAngle;
  Plane plane;
  Point3 *surfNormals;
  PointTag *points = ptData->points;
  uint32   nPoints = ptData->nPoints;

  int32   *pointFacetOffset = ptData->pointFacetOffset;
  int32   *pointFacetAlloc = ptData->pointFacetAlloc;
  int32   *pointFacets;

  inTriangles = FALSE;
  inPoly = FALSE;
  noPoly = TRUE;
  inConvex = FALSE;
 
  UVCount = 0;
  uniquePoints=0;
  
  if (UseDummyTex)
    {
      if (surface->TexIndex > 0)
	{
	  if (surface->EnvMap == TRUE)
	    isTextured = FALSE;
	  else
	    isTextured = TRUE;
	}
      else
	isTextured = FALSE;
    }
  else
    {
      if (surface->TexIndex >= 0)
	{
	  if (surface->EnvMap == TRUE)
	    isTextured = FALSE;
	  else
	    isTextured = TRUE;
	}
      else
	isTextured = FALSE;
    }
  if (ComputeNormals)
    {
      memset((void *)pointFacetAlloc, 0, nPoints*sizeof(int32));
      memset((void *)pointFacetOffset, 0, nPoints*sizeof(int32));
      size=0;
      for (j=0; j<nPolys; j++)
	for (k=1; k<=surfPolys[j][0]; k++)
	  {
	    index = surfPolys[j][k];
	    /* if (nPoints<=index) fprintf(stderr,"Whoops!\n") */
	    pointFacetAlloc[index]++;
	    size++;
	  }
      pointFacets = (int32 *)qMemClearPtr(size,sizeof(int32));
      if (pointFacets==NULL)
	return(M2E_NoMem);
      offset=0;
      for (j=0; j<nPoints; j++)
	{
	  pointFacetOffset[j]=offset;
	  offset += pointFacetAlloc[j];
	  pointFacetAlloc[j]=0;
	}

      for (j=0; j<nPolys; j++)
	for (k=1; k<=surfPolys[j][0]; k++)
	  {
	    index = surfPolys[j][k];
	    /* if (nPoints<=index) fprintf(stderr,"Whoops!\n") */
	    offset = pointFacetOffset[index];
	    offset += pointFacetAlloc[index];
	    pointFacets[offset]= j;
	    pointFacetAlloc[index]++;
	  }
      
      if (!LWSURF_GetSMAN(*surface, &smoothAngle))
	smoothAngle = (gfloat)SmoothAngle;
      surfNormals = (Point3 *)qMemClearPtr(3*(nPolys+5), sizeof(Point3));
      if (surfNormals==NULL)
	return(M2E_NoMem);
      for (j=0; j<nPolys; j++)
	{
	  PlaneEquation(surfPolys[j], points, plane, FALSE, 0);
	  surfNormals[j].x = plane[0];
	  surfNormals[j].y = plane[1];
	  surfNormals[j].z = plane[2];
	}
      for (j=0; j<nPolys; j++)
	for (k=1; k<=surfPolys[j][0]; k++)
	  {
#ifdef applec
	    if (!(j%50))
	      IFF_SpinCursor();
#endif
	    index = surfPolys[j][k];
	    skipPt = MakeNormals(ComputeNormals, pointFacetAlloc, 
				 pointFacetOffset, pointFacets, nPolys, j, k,
				 surfPolys, points, surfNormals, smoothAngle);
	  }
      uniquePoints = UVCount;
      free(pointFacets);
    }
  

  for (j=0; j<nPolys; j++)
    {
      polyMinU = polyMinV = 10000;
      polyMaxU = polyMaxV = -10000;
      if (surfPolys[j][0] >= 3)
	for (k=1; k<=surfPolys[j][0]; k++)
	  {
	    if (ComputeNormals)	      
	      index = UVs[surfPolys[j][k]].Geometry;
	    else
	      index = surfPolys[j][k];
	    skipPt = MakeUVs(isTextured, index, k, surfPolys[j], points, 
			     surface, ComputeNormals);
	    if (!skipPt)
	      {
		points[index].IsNow = uniquePoints;  /* IsNow points to the LATEST point using that geometry */
		points[index].inUse = TRUE;  
		surfPolys[j][k] = uniquePoints;
		uniquePoints++;
	      }
	  }
    }
  if (UVCount>0)
    {
      fprintf(fPtr,"\t\tTriMesh {\n");
      fprintf(fPtr,"\t\t\tvertexList {\n");
      fprintf(fPtr,"\t\t\t\tFormat( Locations ");
      if (ComputeNormals)
	fprintf(fPtr,"| Normals ");
      if (isTextured)
	fprintf(fPtr,"| TexCoords ");
      fprintf(fPtr,")\n");
      fprintf(fPtr,"\t\t\t\tlocations {\n");
      
      for (j=0; j<UVCount; j++)
	{
	  index = UVs[j].Geometry;
		if (setScale == 0)
		  {
		    fprintf(fPtr,"\t\t\t\t\t{ %g %g %g }\n",
			    points[index].p.x, points[index].p.y,
			    points[index].p.z);   /* From Left to Right Coordinates */
		  }
		else
		  {
		    fprintf(fPtr,"\t\t\t\t\t{ %g %g %g }\n",
			    points[index].p.x, points[index].p.y,
			    -points[index].p.z);   /* From Left to Right Coordinates */
		  }
	}

      if (ComputeNormals)
	{
	  fprintf(fPtr,"\t\t\t\t}\n");
	  fprintf(fPtr,"\t\t\t\tNormals {\n");
	  
	  for (j=0; j<UVCount; j++)
	    {
	      if (setScale == 0)
		{
		  fprintf(fPtr,"\t\t\t\t\t{ %g %g %g }\n", UVs[j].N.x, UVs[j].N.y,
			  UVs[j].N.z);
		}
	      else
		{
		  fprintf(fPtr,"\t\t\t\t\t{ %g %g %g }\n", UVs[j].N.x, UVs[j].N.y,
			  -UVs[j].N.z);
		}
	    }
	}
      
      if (isTextured)
	{
	  fprintf(fPtr,"\t\t\t\t}\n");
	  fprintf(fPtr,"\t\t\t\ttexcoords {\n");
	  
	  for (j=0; j<UVCount; j++)
	    {
	      fprintf(fPtr,"\t\t\t\t\t{ %g %g }\n", UVs[j].U, UVs[j].V);
	    }
	}
      
      fprintf(fPtr,"\t\t\t\t}\n\t\t\t}\n");
      fprintf(fPtr,"\t\t\tvertexCount {\n\t\t\t\t");
      for (j=0; j<nPolys; j++)
	{
	  if (surfPolys[j][0] >= 3)
	    {
	      if (surfPolys[j][0] == 3)
		{
		  if (inConvex)
		    {
		      inConvex = FALSE;
		      fprintf(fPtr,"\n#Non-convex ending\n\t\t\t\t");
		    }	
		  fprintf(fPtr,"3 ",surfPolys[j][0]);
		}	  
	      else if (surfPolys[j][0] == 4)  /* Check for convex test */
		{
		  if (inConvex)
		    {
		      inConvex = FALSE;
		      fprintf(fPtr,"\n#Non-convex ending\n\t\t\t\t");
		    }	
		  fprintf(fPtr,"-%d ",surfPolys[j][0]);
		}	      
	      else
		{
		  if (testConvex)
		    test = ConvexTest(surfPolys[j], points);
		  else
		    test = ConvexCW;
		  if ((test != ConvexDegenerate) &&
		      (test != ConvexCCW) && (test != ConvexCW))
		    {
		      if (test ==  NotConvex)
			disc = DiscDetect(surfPolys[j]);
		      else
			{
			  disc = FALSE;
			  /*		      if (!disc)
					      fprintf(stderr,"Warning Non-Convex Polygon Class %d %d of sides %d\n",
					      test, j, surfPolys[j][0]);
					      else
					      fprintf(stderr,"Double sided disc Polygon %d of sides %d\n",
					      j, surfPolys[j][0]/2);
					      */
			}
		      if (!inConvex)
			{
			  inConvex = TRUE;
			  fprintf(fPtr,"\n#Non-convex starting\n");
			}
		      if (disc)
			{
			  fprintf(fPtr,"# Originally -%d \n\t\t\t\t",surfPolys[j][0]);
			  /* Each disc is decomposed into two discs */
			    fprintf(fPtr,"-%d -%d", surfPolys[j][0]/2,
				    surfPolys[j][0]/2);
			}
		      else if (inConvex)
			{
			  if (triangulate(surfPolys[j], points, &triPoly))
			  {
			    fprintf(fPtr,"# Originally -%d \n\t\t\t\t",surfPolys[j][0]);
			    /* Each poly is decomposed into n-2 triangles */
			    for(oldPolys = 2; oldPolys < surfPolys[j][0]; oldPolys++)
			      fprintf(fPtr,"3 ");
			  }
			  else
			    {
			      fprintf(fPtr,"\n# Self-intersecting polygon that can't be triangulated, Poly %d \n\t\t\t\t",j);
			      fprintf(fPtr,"-%d\n\t\t\t\t",surfPolys[j][0]);
			    }
			}
		    }
		  else
		    {
		      fprintf(fPtr,"-%d ",surfPolys[j][0]);
		      if (inConvex)
			{
			  inConvex = FALSE;
			  fprintf(fPtr,"\n#Non-convex ending\n\t\t\t\t");
			}
		    }
		}
	      if (!(j%10))
		fprintf(fPtr,"\n\t\t\t\t");
	    }
	  else
	    {
	      /*	      fprintf(stderr,"Poly %d with %d vertices\n",
			      j, surfPolys[j][0]);
			      */	    }
	}
      fprintf(fPtr,"\n\t\t\t}\n");

      fprintf(fPtr,"\t\t\tvertexIndices {");
      for (j=0; j<nPolys; j++)
	{
	  regular = TRUE;
	  if ((surfPolys[j][0]) < 3)
	    {
	      fprintf(stderr,"Polygon %d has only %d sides\n",
		      j, surfPolys[j][0]);
	    }
	  else
	    {
	      if (surfPolys[j][0] > 4)
		{
		  if (testConvex)
		    test = ConvexTest(surfPolys[j], points);
		  else 
		    test = ConvexCW;
		  if ((test != ConvexDegenerate) &&
		      (test != ConvexCCW) && (test != ConvexCW))
		    {
		      if (test == NotConvex)
			{
			  disc = DiscDetect(surfPolys[j]);
			  if (!disc)
			    {
			      regular = TRUE;
			    }
			  else
			    {
			      regular = FALSE;
			      fprintf(fPtr,"\n# Double disc \n\t\t\t\t",surfPolys[j][0]);
			      /* Each disc is decomposed into two discs */
			      for (k=0; k<surfPolys[j][0]/2; k++)
				{
				  fprintf(fPtr, "%d ", Disc_List[k]);
				}
			      fprintf(fPtr,"\n\t\t\t\t");
			      for (k=(surfPolys[j][0]/2)-1; k>=0; k--)
				{
				  fprintf(fPtr, "%d ", Disc_List[k]);
				}
			    }
			}
		      if (!disc)
			{
			  fprintf(fPtr,"\n#Non-convex of sides %d\n",surfPolys[j][0]);
			  regular = FALSE;
			  if (triangulate(surfPolys[j], points, &triPoly))
			    {
			      regular = FALSE;
			      fprintf(fPtr,"\n\t\t\t\t");
			      for (k=0; k<surfPolys[j][0]-2; k++)
				{
				  /*  fprintf(stderr,"Tri %d:",k); */
				  if (vertexOrder == 0)
				    {   /* From Left to Right Coordinates */
				      for(l=2; l>=0; l--)
					{		
					  index = triPoly[k][l] + 1;
					  if (index > surfPolys[j][0])
					    index = 1;
					  index = surfPolys[j][index];
					  fprintf(fPtr,"%d ", index);
					}
				    }
				  else
				    {
				      for(l=0; l<3; l++)
					{
					  index = triPoly[k][l]+1;
			/*		  fprintf(stderr,"%d ", triPoly[k][l]); */
					  if (index > surfPolys[j][0])
					    index = 1;
					  index = surfPolys[j][index];
					  fprintf(fPtr,"%d ", index);
					}
				    }
				  /* fprintf(stderr,"\n"); */
				}
			    }
			  else
			    {
			      fprintf(fPtr,"\n# Self-intersecting polygon that can't be triangulated, Poly %d \n\t\t\t\t",j);
			      regular = TRUE;
			    }
			}
		    }
		  if (regular) /* Business as usual */
		    {
		      fprintf(fPtr,"\n\t\t\t\t");
		      if (vertexOrder == 0)
			{   /* From Left to Right Coordinates */
			  for (k=surfPolys[j][0]; k>0; k--)
			    {
			      index = surfPolys[j][k];
			      fprintf(fPtr,"%d ",index);
			    }
			}
		      else
			{  /* From Left to Right Coordinates */
			  for (k=1 ; k <= surfPolys[j][0]; k++) 
			    {
			      index = surfPolys[j][k];
			      fprintf(fPtr,"%d ",index);
			    }
			}
		    }
		}
	      else
		{
		  fprintf(fPtr,"\n\t\t\t\t");
		  if (vertexOrder == 0)
		    {   /* From Left to Right Coordinates */
		      for (k=surfPolys[j][0]; k>0; k--)
			{
			  index = surfPolys[j][k];
			  fprintf(fPtr,"%d ",index);
			}
		    }
		  else
		    {  /* From Left to Right Coordinates */
		      for (k=1 ; k <= surfPolys[j][0]; k++) 
			{
			  index = surfPolys[j][k];
			  fprintf(fPtr,"%d ",index);
			}
		    }
		}
	    }
	}
      fprintf(fPtr,"\n\t\t\t}\n\t\t}\n");
    }

  for (j=0; j<UVCount; j++)
    {
      index = UVs[j].Geometry;
	  points[index].inUse = FALSE;
	  points[index].UVCount = 0;
	  points[index].IsNow = index;		
	}

  if (ComputeNormals)
    {
      qMemReleasePtr(surfNormals);
    }

  UVCount = 0;
  return(M2E_NoErr);
}

static M2Err LWOB_WriteModelGroup(FILE *fPtr, MyPolyData *pData)
{
  PointTag *points;
  uint16 *nSP;
  LWSURF *surfs;
  char **namePtrs;
  uint32 nSurfs, surf;
  uint16 **surfPolys, *nSurfPolys;
  int16  nPolys; 
  bool smoothing;
  
  surf  = pData->surfIndex;
  surfs = pData->surfs;
  points = pData->pData->points;
  nSurfs = pData->nSurfs;
  namePtrs = pData->namePtrs;
  if (pData->detail)
    nSurfPolys = pData->nSurfDPolys;
  else
    nSurfPolys = pData->nSurfPolys;
  nSP = nSurfPolys;

  surfPolys = pData->surfPolys;


/* Allocate an intial UV buffer */

  UVCount = 0;
  UVSize = UVIncrement;
  UVs = (UVCoord *)qMemClearPtr(UVSize,sizeof(UVCoord));

  if ((nSurfPolys[surf]>0))
    {
      fprintf(fPtr,"\t\tMatIndex %d\n",surfs[surf].MatIndex);
      fprintf(fPtr,"\t\tTexIndex %d\n",surfs[surf].TexIndex);
      if ((surfs[surf].TexIndex >= 0) && (surfs[surf].EnvMap==TRUE))
	{
	  fprintf(fPtr, "\t\ttexgen {\n\t\t\tkind environment\n\t\t}\n");
      	}

      if (pData->detail)
	{
	  TotalPolys += TotalDPolys += nPolys = nSurfPolys[surf];
	}
      else
	{
	  TotalPolys += nPolys = nSurfPolys[surf];
	}
      LWSURF_GetFSmooth(surfs[surf], &smoothing);
      if ((smoothing) || (NoFlat))
	{
	  TotalSPolys += nPolys;
	  if (nPolys > 0) /* Regular Polygons */
	    {
	      if (smoothing)
		{
		  if (pData->detail)
		    fprintf(stderr," Detail  ");
		  fprintf(stderr,"Surf %d \"%s\": \t\t%d Smooth Polygons\n",
			  surf, namePtrs[surf], nSP[surf]);
		}
	      else
		{
		  if (pData->detail)
		    fprintf(stderr," Detail  ");
		  fprintf(stderr,"Surf %d \"%s\": \t\t%d Flat (Forced Smooth) Polygons\n",
			  surf, namePtrs[surf], nSP[surf]);
		}
	      if (!ComputeNormals)
		fprintf(fPtr,"\t\tautonormals on\n");
	      else
		fprintf(fPtr,"\t\tautonormals off\n");
	      LWOB_WriteSurfaceSmooth(fPtr,nPolys,surfPolys, pData->pData, &(surfs[surf]));
	    }
	}
      else
	{
	  TotalFPolys += nPolys;
	  if (nPolys > 0) /* Regular Polygons */
	    {
	      if (pData->detail)
		fprintf(stderr," Detail  ");
	      fprintf(stderr,"Surf %d \"%s\": \t\t%d Flat Polygons\n",
		      surf, namePtrs[surf], nSP[surf]);
	      fprintf(fPtr,"\t\tautonormals on\n");
	      LWOB_WriteSurfaceFlat(fPtr,nPolys,surfPolys,points, &(surfs[surf]));
	    }
	}
    }
  return (M2E_NoErr);
}

static M2Err LWTex_Print(FILE *fPtr, LWTex *tex)
{
  uint16 data;
  bool flag;
  Point3 point;
  gfloat  tmpFloat;

  if (tex->Name != NULL)
    fprintf(fPtr,"\t\t# Name\"%s\"\n",tex->Name);
  if (tex->TIMG != NULL)
    {
      fprintf(fPtr,"\t\t#FileName\"%s\"\n",tex->TIMG);
    }

  if (LWTex_GetTFLG(*tex, &data))
    {
      fprintf(fPtr,"\t\t# Flags:");
      LWTex_GetFXAxis(*tex, &flag);
      if (flag)
	fprintf(fPtr," XAxis");
      LWTex_GetFYAxis(*tex, &flag);
      if (flag)
	fprintf(fPtr," YAxis");
      LWTex_GetFZAxis(*tex, &flag);
      if (flag)
	fprintf(fPtr," ZAxis");
      LWTex_GetFWorld(*tex, &flag);
      if (flag)
	fprintf(fPtr," WorldCoords");
      fprintf(fPtr,"\n");
    }

  if (LWTex_GetTSIZ(*tex, &point))
    {
      fprintf(fPtr,"\t\t#Size {%.5f %.5f %.5f}\n",point.x, point.y, point.z);
    }
  if (LWTex_GetTCTR(*tex, &point))
    {
      fprintf(fPtr,"\t\t#Center {%.5f %.5f %.5f}\n",point.x, point.y, point.z);
    }
  if (LWTex_GetTFP0(*tex, &tmpFloat))
   {
     fprintf(fPtr,"\t\t#TFP0 %.5g\n",tmpFloat);
   }
  if (LWTex_GetTFP1(*tex, &tmpFloat))
   {
     fprintf(fPtr,"\t\t#TFP1 %.5g\n",tmpFloat);
   }
   return(M2E_NoErr);
}

static Err stopOnExit(IFFParser *iff, void *userData)
{
	void *a;
	
	a = userData;
	a = (void *)iff;
  return (IFF_PARSE_EOC);
}

#define	ID_LWOB	MAKE_ID('L','W','O','B')
#define	ID_PNTS	MAKE_ID('P','N','T','S')
#define	ID_SRFS	MAKE_ID('S','R','F','S')
#define	ID_SURF	MAKE_ID('S','U','R','F')
#define	ID_POLS	MAKE_ID('P','O','L','S')


static IFFTypeID lwprops[] =
 {
ID_LWOB, ID_PNTS,
ID_LWOB, ID_SRFS,
ID_LWOB, ID_POLS,
0
};

static IFFTypeID lwcoll[] =
{
  ID_LWOB, ID_SURF,
  0
};


static M2Err Name_Process(char *inName, char **procName, bool lower)
{
  char *myName;
  char *lastSlash;
  char *lastColon;
  char *lastBack;
  char *temp;
  int i, slashLen, backLen, colonLen, tempLen;
  
  /* Find the file name minus the path name */
  
  lastBack = (char *)strrchr(inName, '\\');
  lastColon = (char *)strrchr(inName, ':');
  lastSlash = (char *)strrchr(inName, '/');
  tempLen = strlen(inName);

  if (lastColon != NULL)
    {
      lastColon++;
      colonLen = strlen(lastColon);
    }
  else
    {
      lastColon = inName;
      colonLen = tempLen;
    }
  if (lastSlash != NULL)
  {
    lastSlash++;
    slashLen = strlen(lastSlash);
  }  
  else
    {
      lastSlash = inName;
      slashLen = tempLen;
    }
  if (lastBack != NULL)
    { 
      lastBack++;
      backLen = strlen(lastBack);
    }
  else
    {
      lastBack = inName;
      backLen = tempLen;
    }

  if (colonLen < slashLen)
    {
      tempLen = colonLen;
      temp = lastColon;
    }
  else
    {
      tempLen = slashLen;
      temp = lastSlash;
    }
  
  if (backLen < tempLen)
    {
      tempLen = backLen;
      temp = lastBack;
    }

  /* Copy just the file Name */
  myName = (char *)qMemClearPtr(tempLen+8, 1);
  strcpy(myName, temp);
  
  if (lower)
    {
      for (i=0; i<strlen(myName); i++)
		 myName[i] = tolower(myName[i]);
    }
  
  *procName = myName;
	return(M2E_NoErr);
}

static M2Err TexName_Process(char *inName, char **texName)
{

  char *lastExtension;
  char *myName;
  int i;

  Name_Process(inName, &myName, ToLower);
  lastExtension = (char *)strrchr(myName, '.');
  
  /* Replace any extension */
  if ((lastExtension != NULL) && (RemoveExtensions))
    {
      for( i=1; lastExtension[i] != '\0'; i++)
	{
	  if (!((lastExtension[i] <= '9') && (lastExtension[i] >= '0')))
	    {  /* If it's not an image number like .120, replace it */
	      *lastExtension = '\0';
	      break;
	    }
	}
    }
  strcat(myName, ".utf");
  *texName = myName;
  return(M2E_NoErr);
}

static uint16 ProcessTextures(FILE *fPtr, LWSURF *surfs, int16 nSurfs, 
			    char *fileIn, char **namePtrs, SDFTex **inTextures)
{

  SDFTex tempTex, *temp, *textures;
  bool          diffUsed, validTexture;
  int16         texIndex;
  uint32        color;
  uint16        refl;
  uint8         temp8;
  SDFMat tempMat;
  char          *texName;
  int i,j, start, matIndex;
  M2Err err;

  textures = *inTextures;
  if (UseDummyTex)
    {
      if (CurTexture == 0)
	CurTexture = 1;
      start = 1;
    }
  else
    start = 0;
  for (i=0; i<nSurfs; i++)
    {
      diffUsed = FALSE;
      validTexture = FALSE;
      texIndex = -1;
      SDFTex_Init(&tempTex);
      if (surfs[i].CTEX != NULL)
	{
	  if (surfs[i].CTEX->TIMG != NULL)
	    {
	      tempTex.OrigName = surfs[i].CTEX->TIMG;
	      TexName_Process(tempTex.OrigName, &texName);
	      tempTex.FileName = texName;

	      if (TexModulate)
		{
		  /* Necessary to make the underlying material white if has color texture */
		  SDFMat_Init(&tempMat);
		  err = LWSURF_MakeWhiteMat(surfs[i], WhiteIntensity, &tempMat);
		  if (err != M2E_NoErr)
		    {
		      fprintf(stderr,"WARNING:Invalid Material White in ProcessTextures\n");
		    }
		  else 
		    {
		      err = SDFMat_Add(&tempMat, &GMaterials, &NMaterials, &CurMaterial, TRUE,
				   &matIndex);
		      if (err!=M2E_NoErr)
			return(err);
		    }
		  
		  surfs[i].MatIndex = matIndex;      


		  SDFTex_SetTAB(&tempTex, TXA_FirstColor, TX_ColorSelectPrimColor);
		  SDFTex_SetTAB(&tempTex, TXA_SecondColor, TX_ColorSelectTexColor);
		  SDFTex_SetTAB(&tempTex, TXA_BlendOp, TX_BlendOpMult);
		  SDFTex_SetTAB(&tempTex, TXA_ColorOut, TX_BlendOutSelectBlend);
		}
	      else
		SDFTex_SetTAB(&tempTex, TXA_ColorOut, TX_BlendOutSelectTex);
	      if (surfs[i].TTEX == NULL)
		SDFTex_SetTAB(&tempTex, TXA_AlphaOut, TX_BlendOutSelectPrim);
	      else if (surfs[i].TTEX->TIMG == NULL)
		SDFTex_SetTAB(&tempTex, TXA_AlphaOut, TX_BlendOutSelectPrim);
	      else
		SDFTex_SetTAB(&tempTex, TXA_AlphaOut, TX_BlendOutSelectTex);
	      validTexture = TRUE;
		  
	      surfs[i].UsedTEX = surfs[i].CTEX;
	    }
	      
	  if (CommentLevel > 3)
	    {
	      fprintf(fPtr,"\t\t#Define Texture %s_ColTex\n", namePtrs[i]);
	      LWTex_Print(fPtr,surfs[i].CTEX);
	    }
	}
      if (surfs[i].DTEX !=NULL)
	{
	  if ((surfs[i].DTEX->TIMG != NULL) && (!validTexture) && (DiffForColor))
	    {
	      tempTex.OrigName = surfs[i].DTEX->TIMG;
	      TexName_Process(tempTex.OrigName, &texName);
	      tempTex.FileName = texName;
	      if (TexModulate)
		{
		  SDFTex_SetTAB(&tempTex, TXA_FirstColor, TX_ColorSelectPrimColor);
		  SDFTex_SetTAB(&tempTex, TXA_SecondColor, TX_ColorSelectTexColor);
		  SDFTex_SetTAB(&tempTex, TXA_BlendOp, TX_BlendOpMult);
		  SDFTex_SetTAB(&tempTex, TXA_ColorOut, TX_BlendOutSelectBlend);
		}
	      else
		SDFTex_SetTAB(&tempTex, TXA_ColorOut, TX_BlendOutSelectTex);

	      if (surfs[i].TTEX == NULL)
		SDFTex_SetTAB(&tempTex, TXA_AlphaOut, TX_BlendOutSelectPrim);
	      else if (surfs[i].TTEX->TIMG == NULL)
		SDFTex_SetTAB(&tempTex, TXA_AlphaOut, TX_BlendOutSelectPrim);
	      else
		SDFTex_SetTAB(&tempTex, TXA_AlphaOut, TX_BlendOutSelectTex);

	      validTexture = TRUE;

	      surfs[i].UsedTEX = surfs[i].DTEX;
	      diffUsed = TRUE;
	    }	
	  if (CommentLevel > 3)
	    {
	      fprintf(fPtr,"\t\t#Define Texture %s_DiffTex\n", namePtrs[i]);
	      LWTex_Print(fPtr,surfs[i].DTEX);
	    }
	}
      if ((surfs[i].RIMG != NULL) && (DoEnvironmentMap))
	{
	  if ((!validTexture) && (LWSURF_GetREFL(surfs[i],&refl)))
	    {
	      tempTex.OrigName = surfs[i].RIMG;
	      TexName_Process(tempTex.OrigName, &texName);
	      tempTex.FileName = texName;
	      if (TexModulate==TRUE)
		{
		  SDFTex_SetTAB(&tempTex, TXA_FirstColor, TX_ColorSelectPrimColor);
		  SDFTex_SetTAB(&tempTex, TXA_SecondColor, TX_ColorSelectTexColor);
		  if (EnvMult==TRUE)
		    SDFTex_SetTAB(&tempTex, TXA_BlendOp, TX_BlendOpMult);
		  else
		    {
		      if (!LWSURF_GetREFL(surfs[i],&refl))
			refl = 192;
		      else
			refl = (refl*255)/256.0;
			
		      temp8=(uint8)refl;

		      color = 0xFF;
		      color = (color<<8) + refl;
		      color = (color<<8) + refl;
		      color = (color<<8) + refl;
		      SDFTex_SetTAB(&tempTex, TXA_BlendOp, TX_BlendOpLerp);
		      SDFTex_SetTAB(&tempTex, TXA_ThirdColor, TX_ColorSelectConstColor);
		      SDFTex_SetTAB(&tempTex, TXA_BlendColorSSB0, color);
		      SDFTex_SetTAB(&tempTex, TXA_BlendColorSSB1, color);
		    }
		  SDFTex_SetTAB(&tempTex, TXA_ColorOut, TX_BlendOutSelectBlend);
		}
	      else
		SDFTex_SetTAB(&tempTex, TXA_ColorOut, TX_BlendOutSelectTex);

	      if (surfs[i].TTEX == NULL)
		SDFTex_SetTAB(&tempTex, TXA_AlphaOut, TX_BlendOutSelectPrim);
	      else if (surfs[i].TTEX->TIMG == NULL)
		SDFTex_SetTAB(&tempTex, TXA_AlphaOut, TX_BlendOutSelectPrim);
	      else
		SDFTex_SetTAB(&tempTex, TXA_AlphaOut, TX_BlendOutSelectTex);

	      tempTex.XTile=TRUE;
	      tempTex.YTile=FALSE;
	      tempTex.EnvMap=TRUE;
	      validTexture = TRUE;
	      surfs[i].UsedTEX = surfs[i].RTEX;
	      surfs[i].EnvMap = TRUE;
	    }	

	  if (CommentLevel > 3)
	    {
	      fprintf(fPtr,"\t\t#Define Spherical Environment Map %s\n", namePtrs[i]);
	      fprintf(fPtr,"\t\t#FileName\"%s\"\n\n", surfs[i].RIMG);
	    }
	}
 
      if (surfs[i].TTEX != NULL)
	{
	  if ((!validTexture) && (surfs[i].TTEX->TIMG != NULL))
	    {
	      tempTex.OrigName = surfs[i].TTEX->TIMG;
	      TexName_Process(tempTex.OrigName, &texName);
	      tempTex.FileName = texName;
	      SDFTex_SetTAB(&tempTex, TXA_ColorOut, TX_BlendOutSelectPrim);
	      SDFTex_SetTAB(&tempTex, TXA_AlphaOut, TX_BlendOutSelectTex);
	      validTexture = TRUE;
	      surfs[i].UsedTEX = surfs[i].TTEX;
	    }	
	  if (CommentLevel > 3)
	    {
	      fprintf(fPtr,"\t\t#Define Texture %s_TransTex\n", namePtrs[i]);
	      LWTex_Print(fPtr,surfs[i].TTEX);
	    }
	}

      if (surfs[i].RTEX != NULL)
	{
	  if (CommentLevel > 3)
	    {
	      fprintf(fPtr,"\t\t#Define Texture %s_ReflTex\n", namePtrs[i]);
	      LWTex_Print(fPtr,surfs[i].RTEX);
	    }
	}
      if (surfs[i].BTEX != NULL)
	{
	  if (CommentLevel > 3)
	    {
	      fprintf(fPtr,"\t\t#Define Texture %s_BumpTex\n", namePtrs[i]);
	      LWTex_Print(fPtr,surfs[i].BTEX);
	    }
	}
      if (surfs[i].STEX != NULL)
	{
	  if (CommentLevel > 3)
	    {
	      fprintf(fPtr,"\t\t\t#Define Texture %s_SpecTex\n", namePtrs[i]);
	      LWTex_Print(fPtr,surfs[i].STEX);
	    }
	}      

      if (validTexture)
	{
	  for (j=start; j<CurTexture; j++)
	    {
	      if (SDFTex_Compare(&tempTex, &(textures[j])))
		{
		  texIndex = j;
		  break;
		}
	    }
	  if (texIndex == -1)
	    {
	      texIndex = CurTexture;
	      /* Check if the texture size is exceeded*/

	      CurTexture++;
	      if (CurTexture >= NTextures)
		{
		  NTextures += TEX_BUF_SIZE;
		  temp = qMemResizePtr(textures, NTextures*sizeof(SDFTex));
		  if (temp == NULL)
		    {
		      fprintf(stderr,"ERROR:Out of memory in Texture_ReadFile!\n");
		      return(M2E_NoMem);
		    }
		  textures = temp;
		}
	      SDFTex_Copy(&(textures[texIndex]), &tempTex);
	    }
	}

      surfs[i].TexIndex = texIndex;      
      if (UseDummyTex)
	{
	  if (texIndex == -1)
	    {
	      if (surfs[i].TRAN > 0)
		surfs[i].TexIndex = 0;
	    }
	}
    }

  if (CurTexture>0)
    {
      if (texAppend)
	{
	  fprintf(fPtr,"SDFVersion 1.0\n\n");
	  fprintf(fPtr,"Define TexArray \"%s\" {\n",fileIn);
	}
      else
	{
	  if (SepMatTex)
	    fprintf(fPtr,"SDFVersion 1.0\n\n");
	  fprintf(fPtr,"Define TexArray \"%s_textures\" {\n",fileIn);
	}
      if (UseDummyTex)
	{
	  fprintf(fPtr,"\t{\n");
	  fprintf(fPtr,"\t\ttxColorOut Prim\n");
	  fprintf(fPtr,"\t\ttxAlphaOut Prim\n");
	  fprintf(fPtr,"\t}\n");
	}
  
      for (i=start; i<CurTexture; i++)
	{
	  SDFTex_Print(&(textures[i]), fPtr, 1);
	}
      
      /*  if (CurTexture > 0) */
      fprintf(fPtr,"}\n");

      if (ferror(fPtr))
	{
	  fprintf(stderr,"WARNING:Error detected on SDF TexArray output.  Disk may be full.\n");
	}
    }

  *inTextures = textures;
  return(CurTexture);
}

static
Err ProcessLWOB(IFFParser *iff, char *fileIn, FILE *fPtr)
{
  PropChunk	*pc;
  CollectionChunk *cc;
  char          *texName;
  uint16        uniqueTextures, uniqueMaterials;
  int texIndex;
  uint16 *nSP, *nSDP;
  uint32 nShorts;
  MyPolyData    pData;
  MyPointData   myPointData;
  bool inPoly, inDetail;
  PointTag *points;
  uint32 polsChunkSize, i;
  M2Err err;
  char *srfs = NULL;
  char *inName;
  int16 *polsChunk = NULL;
  int16 tempShort, nVerts, nDetails;
  uint16 ***surfPolys, ***surfDPolys, ***sP, ***sDP;
  int16 *polyStart, *chunkPtr;
  pData.pData = &myPointData;

  fprintf(fPtr,"SDFVersion 1.0\n\n");

  Name_Process(fileIn, &inName, FALSE);
/*   tex = (M2TX *) FindFrame (iff, ID_TXTR);
*/
  pc = FindPropChunk (iff, ID_LWOB, ID_SRFS);
  if (pc!=0)
    {
      err = LWOB_ReadSRFS(pc, &srfs, &(pData.namePtrs), &(pData.nSurfs));
    }
  else
    {
      return(IFF_ERR_MANGLED);
    }
  if (CommentLevel > 1)
    printf("Surfaces=%d\n",pData.nSurfs);
  pData.surfs = (LWSURF *)qMemClearPtr(pData.nSurfs, sizeof(LWSURF));
  for (i=0; i<pData.nSurfs; i++)
    LWSURF_Init(&(pData.surfs[i]));

  pc =  FindPropChunk (iff, ID_LWOB, ID_PNTS);
  if (pc != 0)
    {
      err = LWOB_ReadPNTS(pc, &points, &NPoints);
      TotalVertices = NPoints;
      myPointData.points = points;
      myPointData.nPoints = myPointData.allocPoints = NPoints;
      myPointData.pointFacetAlloc = (int32 *)qMemClearPtr(NPoints, sizeof(int32));
      if (myPointData.pointFacetAlloc == NULL)
	return (M2E_NoMem);
      myPointData.pointFacetOffset = (int32 *)qMemClearPtr(NPoints, sizeof(int32));
      if (myPointData.pointFacetOffset == NULL)
	return (M2E_NoMem);
      if (CommentLevel > 1)
      printf("Vertices=%d\n",NPoints);
    }
  else
    {
      return(IFF_ERR_MANGLED);
    }
  
  pc = FindPropChunk (iff, ID_LWOB, ID_POLS);
  if (pc != 0)
    {
      err = LWOB_ReadPOLS(pc, pData.nSurfs, &polsChunk, &polsChunkSize, 
			  &(pData.nSurfPolys), &(pData.nSurfDPolys));
    }
  else
    {
      return(IFF_ERR_MANGLED);
    }
  
  if (err != M2E_NoErr)
    {
      fprintf(stderr,"ERROR:LWOBReadSRFSSError = %d\n", err);
      return(err);
    }
  cc =  FindCollection (iff, ID_LWOB, ID_SURF);
  if (cc != 0)
    {
      if (!cc)
	{
	  fprintf(stderr,"WARNING:No surfaces found\n");
	}
      else
        for ( ;  cc;  cc = cc->cc_Next) 
	  {
	    err = LWOB_ReadSURF(cc, pData.namePtrs, pData.surfs, pData.nSurfs);
	  }
    }

  uniqueMaterials = 0;
  if ((!GlobalMaterials))
    {
      GMaterials = (SDFMat *)qMemClearPtr(MAT_BUF_SIZE, sizeof(SDFMat));
      NMaterials = MAT_BUF_SIZE;
      CurMaterial = 0;
    }
  if (!matAppend)
    {
      if (SepMatTex)
	{
	  fprintf(fPtr, "include \"%s.mat\"\n",BaseName);
	  MatScript = MatOutFile;
	}
      else
	MatScript = fPtr;
    }
    err = Materials_Process(pData.surfs, pData.nSurfs, pData.namePtrs);
      
  uniqueMaterials = CurMaterial;
  /*
    if (SepMatTex)
    {
    fprintf(fPtr, "include \"%s.mat\"\n",BaseName);
    LWSURF_Print(MatOutFile, BaseName, namePtrs, surfs, nSurfs);
    }
    else
    {
    LWSURF_Print(fPtr, BaseName, namePtrs, surfs, nSurfs);
    }
    */
  uniqueTextures = 0;
  if (!NoTextures)
    {
      if ((!GlobalTextures))
	{
	  GTextures = (SDFTex *)qMemClearPtr(TEX_BUF_SIZE, sizeof(SDFTex));
	  NTextures = TEX_BUF_SIZE;
	  CurTexture = 0;
	}
      if (!texAppend)
	{
	  if (SepMatTex)
	    {
	      fprintf(fPtr, "include \"%s.tex\"\n",BaseName);
	      TexScript = TexOutFile;
	    }
	    else
	      TexScript = fPtr;
	  uniqueTextures = ProcessTextures(TexScript, pData.surfs, pData.nSurfs,
					   BaseName, pData.namePtrs, &GTextures);
	}
      else
	uniqueTextures = ProcessTextures(TexScript, pData.surfs, pData.nSurfs,
					 TexFileName, pData.namePtrs, &GTextures);
      
    }
  else
    uniqueTextures = 0;

  if (!matAppend)
    {
      if (SepMatTex)
	{
	  fprintf(fPtr, "include \"%s.mat\"\n",BaseName);
	  MatScript = MatOutFile;
	}
      else
	MatScript = fPtr;
      err = Materials_Print(MatScript, BaseName);
    }
  else
    err = Materials_Print(MatScript, MatFileName);


  /*  Moved from WriteModelGroup */

  nSP = pData.nSurfPolys;
  nSDP = pData.nSurfDPolys;
  sP = surfPolys = (uint16 ***)qMemNewPtr(pData.nSurfs*sizeof(uint16 **));
  sDP = surfDPolys = (uint16 ***)qMemNewPtr(pData.nSurfs*sizeof(uint16 **));
  for (i=0; i<pData.nSurfs; i++)
    {
      if (nSP[i] <= 0)
	sP[i] = NULL;
      else 
	{
	  sP[i] = (uint16 **)qMemNewPtr(nSP[i]*sizeof(uint16 *));
	  if (sP[i] == NULL)
	    return (M2E_NoMem);
	}
      if (nSDP[i] <= 0)
	{
	  sDP[i] = NULL;
	}
      else 
	{
	  sDP[i] = (uint16 **)qMemNewPtr(nSDP[i]*sizeof(uint16 *));
	  if (sDP[i] == NULL)
	    return (M2E_NoMem);
	}
    }
  for (i=0; i<pData.nSurfs; i++)
    {
      nSP[i]=0;
      nSDP[i]=0;
    }
  /*
   * This time get pointers to each poly and fill out the array
   */
  inPoly = inDetail = FALSE;
  nDetails = 0;
  nShorts = polsChunkSize/2;
  chunkPtr = polsChunk;
   for (i=0; i<nShorts; i++)
    {
      tempShort = *chunkPtr;
      if (!inPoly)
	{
	  /*
	   * Either starting a new Poly or a Detail Poly
	   */
	  if ((inDetail) &&(nDetails==0))
	    nDetails = tempShort;
	  else
	    {
	      nVerts = tempShort;
	      polyStart = chunkPtr;
	      inPoly = TRUE;
	    }
	}
      else
	{
	  if (nVerts <= 0)  /* The next short is a surface */
	    {
	      inPoly = FALSE;
	      if (tempShort>0)
		{
		  if (inDetail)
		    {
		      nDetails--;
		      sDP[tempShort-1][nSDP[tempShort-1]] = (uint16 *)polyStart;
		      nSDP[tempShort-1]++;
		      if (nDetails <=0)
			inDetail = FALSE;
		    }
		  else
		    {
		      sP[tempShort-1][nSP[tempShort-1]] = (uint16 *)polyStart;
		      nSP[tempShort-1]++;
		    }
		}
	      else
		{
		  sP[(-tempShort)-1][nSP[(-tempShort)-1]] = (uint16 *)polyStart;
		  nSP[(-tempShort)-1]++;
		  inDetail = TRUE;  /* We are now going to start the Detail Polys */
		  nDetails = 0;
		}
	    }
	  else
	    nVerts--;
	}
      chunkPtr++;
    }





  if (setScale == 0)
    {
      fprintf(fPtr,
	      "Define Model \"%s\"{\n",inName);
      if (uniqueTextures > 0)
	{
	  if (texAppend)
	    fprintf(fPtr,"\tUse TexArray \"%s\"\n", TexFileName);
	  else
	    fprintf(fPtr,"\tUse TexArray \"%s_textures\"\n",
		    BaseName);
	}
      if (uniqueMaterials > 0)
	{
	  if (matAppend)
	    fprintf(fPtr,"\tUse MatArray \"%s\"\n", MatFileName);
	  else
	    fprintf(fPtr,"\tUse MatArray \"%s_materials\"\n",
		    BaseName);	
	}
      fprintf(fPtr,"\tScale {1 1 -1}\n\tSurface{\n",
	      inName);
/*      fprintf(fPtr,"\tSurface{\n"); */
    }
  else
    {
      fprintf(fPtr,
	      "Define Model \"%s\" {\n", inName);
      if (uniqueTextures > 0)
	{
	  if (texAppend)
	    fprintf(fPtr,"\tUse TexArray \"%s\"\n", TexFileName);
	  else
	    fprintf(fPtr,"\tUse TexArray \"%s_textures\"\n",
		    BaseName);	
	}
      if (uniqueMaterials > 0)
	{
	  if (matAppend)
	    fprintf(fPtr,"\tUse MatArray \"%s\"\n", MatFileName);
	  else
	    fprintf(fPtr,"\tUse MatArray \"%s_materials\"\n",
		    BaseName);	
	}
      fprintf(fPtr,"\tSurface{\n");
    }
/*  fprintf(fPtr,"\t\tVertexOrder 0\n");
*/
  
/*  fprintf(fPtr,
	  "  Define Model %s{\n\tSurface{\n",
	  inName);
*/

  pData.detail = FALSE;
  for (i=0; i<pData.nSurfs; i++)
    {
      pData.surfPolys = surfPolys[i];
      pData.surfIndex = i;
      err = LWOB_WriteModelGroup(fPtr, &pData);      
    }
  pData.detail = TRUE;
  for (i=0; i<pData.nSurfs; i++)
    {
      pData.surfPolys = surfDPolys[i];
      pData.surfIndex = i;
      err = LWOB_WriteModelGroup(fPtr, &pData);      
    }

  fprintf(fPtr,"\t}\n");
  fprintf(fPtr,"}\n");

  if (ferror(fPtr))
    {
      fprintf(stderr,"WARNING:Error detected on WriteModelGroup output.  Disk may be full.\n");
    }

  if (CommentLevel>2)
    {
      fprintf(stderr,"Total Smooth Polygons:\t%d\n",TotalSPolys);
      fprintf(stderr,"Total Flat Polygons:\t%d\n",TotalFPolys);
      fprintf(stderr,"Total Regular Polygons:\t%d\n",TotalPolys);
      fprintf(stderr,"Total Detail Polygons:\t%d\n",TotalDPolys);
      fprintf(stderr,"Total Polygons:\t%d\n",TotalPolys+TotalDPolys);
    }
  


  for (i=0; i<pData.nSurfs; i++)
    {
/*
      fprintf(stderr, "Surf %d:Min(U,V)=(%g,%g) Max(U,V)=(%g,%g)\n", i, 
	      surfs[i].MinU, surfs[i].MinV, surfs[i].MaxU, surfs[i].MaxV);
*/
      if (pData.surfs[i].MinU < pData.surfs[i].MaxU)
	{
	  if ((pData.surfs[i].MaxU - pData.surfs[i].MinU) > 1.001)
	    {
	      pData.surfs[i].XTile = TRUE;
	      if (pData.surfs[i].UsedTEX != NULL)
		{
		  texIndex = pData.surfs[i].TexIndex;
		  if (texIndex != -1)
		    if (GTextures[texIndex].XTile!=TRUE)
		      {
			GTextures[texIndex].XTile = TRUE;
			fseek(TexScript, GTextures[texIndex].XClampPos, SEEK_SET);
			fprintf(TexScript,"Tile ");
		      }
		}
	    }
	}
      if (pData.surfs[i].MinV < pData.surfs[i].MaxV)
	{
	  if ((pData.surfs[i].MaxV - pData.surfs[i].MinV) > 1.001)
	    {
	      pData.surfs[i].YTile = TRUE;
	      if (pData.surfs[i].UsedTEX != NULL)
		{
		  texIndex = pData.surfs[i].TexIndex;
		  if (texIndex != -1)
		    if (GTextures[texIndex].YTile!=TRUE)
		      {
			GTextures[texIndex].YTile = TRUE;
			fseek(TexScript, GTextures[texIndex].YClampPos, SEEK_SET);
			fprintf(TexScript,"Tile ");
		      }
		}
	    }
	}
    }
  for (i=0; i<uniqueTextures; i++)
    {
      if (GTextures[i].OrigName != NULL)
	{
	  fprintf(OutScript, "convert");
	  if (GTextures[i].XTile==TRUE)
	fprintf(OutScript, "x");
	  if (GTextures[i].YTile==TRUE)
	    fprintf(OutScript, "y");
	  Name_Process(GTextures[i].OrigName, &texName, ToLower);
	  fprintf(OutScript, " \"%s\" \"%s\"\n", texName, 
		  GTextures[i].FileName);
	  
	  if (texName != NULL)
	   qMemReleasePtr(texName);
	}
    }
}

/* Read the Lightwave file format stuff */

static M2Err ReadIFF (char *fileName, char *fileOut)
{
  FILE *fPtr;
  uint32 fileSize;
  IFFParser	*iff;
  ContextNode	*top;
  M2Err         returnErr = M2E_NoErr;
  Err			result;
  bool			foundLW = 0;
  TagArg          tags[2];
  
  /*
   * Create IFF file and open a DOS stream on it.
   */

  tags[0].ta_Tag = IFF_TAG_FILE;
  tags[0].ta_Arg = fileName;
  tags[1].ta_Tag = TAG_END;

  result = CreateIFFParser (&iff, FALSE, tags);
  
  if (result<0)
    goto err;

  result = InstallExitHandler (iff, ID_LWOB, ID_FORM,
			       IFF_CIL_TOP, stopOnExit, iff);
    if (result<0)
    goto err;
  
  /*
   * Declare property, collection and stop chunks.
   * (You still have to do this because YOU handle the data.)
   */
  
  result = RegisterPropChunks (iff, lwprops);
  if (result<0)
    goto err;

/*
   result = RegisterStopChunks (iff, lwstops);
   if (result<0)
   goto err;
   */

  result = RegisterCollectionChunks (iff, lwcoll);
  if (result<0)
    goto err;

  fPtr = fopen(fileOut, "w");
 setvbuf(fPtr, NULL, _IOFBF, LW_IOBUF_SIZE);
  if (fPtr == NULL)
    {
      fprintf(stderr,"ERROR:Can't open file \"%s\" \n",fileOut);
      return(M2E_BadFile);
    }
  
  result = 0;
  while (result>=0)
    {
      /*
       * ParseIFF() when the frame is filled.  In any case, we 
       * should nab a frame for this context (even if
       * it's really NULL).
       */
      
      result = ParseIFF (iff, IFF_PARSE_SCAN);
      
      if (result == IFF_PARSE_EOC)
	{
	  top = GetCurrentContext (iff);
	  /* if (GetParentContext(top)) */
	  if (top)
	    {
	      /*      if (GetParentContext(top) -> cn_ID == ID_FORM)  */
	      if (top -> cn_ID == ID_FORM)  
		{
		  result = ProcessLWOB(iff, fileName, fPtr);
		  if (result <0)
		    {
		      foundLW = FALSE;
		      fprintf(stderr, "WARNING:Possible bad IFF in file form, going to the next one\n");
		    }
		  else
		    {
		      fileSize = top->cn_Size;
		      foundLW = TRUE;
		      goto die;
		    }
		}
	    }
	  result = 0;
	  continue;
	}
      /*
       * Other errors are real and possibily nasty errors.
       */
      if ((result<0)&&(result != IFF_PARSE_EOF))
	{
	  goto err;
	}
      else if (result == IFF_PARSE_EOF)
	{
	  break;
	}
    }
 err:
  if (result == IFF_PARSE_EOF) 
    {
      if (foundLW)
	returnErr = M2E_NoErr;
      else
	returnErr = M2E_BadFile;
    } 
  else
    switch(result)
      {
      case IFF_ERR_NOMEM: 
	returnErr = M2E_NoMem;
	break;
      case IFF_ERR_BADTAG:
	returnErr = M2E_Range;
	break;
      case IFF_ERR_MANGLED:
      case IFF_ERR_NOTIFF:
	returnErr = M2E_BadFile;
	break;
      case IFF_ERR_BADPTR:
      case IFF_ERR_BADLOCATION:
      default:
	returnErr = M2E_BadPtr;
	break;
      }
  die:
  if (iff)
    {
      fclose(fPtr);
      result = DeleteIFFParser(iff);
      if ((result<0) && (returnErr != M2E_NoErr))
	returnErr = M2E_Limit;
    }    
  return(returnErr);
}

static void main2()
{
  char fileIn[256];
  char fileOut[256];
  M2Err err;

  printf("Enter Lightwave Object to SDF: <FileIn> <FileOut>\n");
  printf("Example: dumb.lw dumb.sdf\n");
  fscanf(stdin,"%s %s",fileIn,fileOut);
  
  err = ReadIFF(fileIn, fileOut);
}

static void print_description()
{
  printf("Description:\n");
  printf("   Version %s\n", LW_VERSION_STRING);
  printf("   Convert Lightwave Object files to ASCII SDF models\n");
  printf("   -sa <Name> \tAppend the texture processing script to the end of this file.\n");
  printf("   -sn <Name> \tWrite the texture processing script to this file. (default=stderr)\n");
  printf("   -ta <Name> \tRead the SDF TexArray from this file and append new TexBlends to it.\n");
  printf("   -tn <Name> \tWrite the SDF TexAray to this file. (default=output file)\n");
  printf("   -ma <Name> \tRead the SDF MatArray from this file and append new Materials to it.\n");
  printf("   -mn <Name> \tWrite the SDF MatArray to this file. (default=output file)\n");
  printf("   -m  \tWrite out the MatArray and the TexArray into a separate file.\n");
  printf("   -smooth \tForce all flat polygons to be smooth.\n");
  printf("   -norm \tTurn off normal generation.\n");
  printf("   -e  \tDon't strip off extensions.\n");
  printf("   -case \tDon't convert names to lowercase.\n");
  printf("   -envmap \tDon't convert environment map data.\n");
  printf("   -envmult \tMultiply enviroment maps by material instead of blending.\n");
  printf("   -t    \tOnly output geometry, no textures.\n");
  printf("   -amb <0.0-1.0> \tSet the ambient lighting value. (default=0.2)\n");
  printf("   -white <0.0-1.0> \tSet the \"white\" lighting value for textured surfaces. (default=0.8)\n");
}

void triangulate_init();

int main(int argc, char *argv[])
{
  M2Err err;
  int argn, i;
  char fileIn[256];
  char fileOut[256];
  char scriptFile[256];
  char TexMatName[256];
  char *lastExtension;
  bool validScript = FALSE;
  bool scriptAppend = FALSE;
  bool scriptNew = FALSE;
  bool validTexScript = FALSE;
  bool validMatScript = FALSE;
  bool texNew = FALSE;
  bool matNew = FALSE;

triangulate_init();

#ifdef M2STANDALONE	
  printf("Enter: <FileIn> <FileOut>\n");
  printf("Example: dumb.lwo dumb.sdf\n");
  fscanf(stdin,"%s %s",fileIn, fileOut);
  
#else

  if (argc < 3) 
    {
      fprintf(stderr,"Usage: %s <FileIn> [options] <FileOut>\n",argv[0]);
      print_description();
      return(-1);
    }

  strcpy(fileIn, argv[1]);
  argn = 2;
  while ( (argn < argc) && (argv[argn][0] == '-') && (argv[argn][1] != '\0') )
    {
      if ( strcmp( argv[argn], "-smooth")==0 )
        {
	  ++argn;
	  NoFlat = TRUE;
        }
      else if ( strcmp( argv[argn], "-norm")==0 )
        {
	  ++argn;
	  ComputeNormals = FALSE;
        }
      else if ( strcmp( argv[argn], "-case")==0 )
        {
	  ++argn;
	  ToLower = FALSE;
        }
      else if ( strcmp( argv[argn], "-dummy")==0 )
        {
	  ++argn;
	   UseDummyTex = !UseDummyTex;
        }
      else if ( strcmp( argv[argn], "-m")==0 )
        {
	  ++argn;
	  SepMatTex = TRUE;
        }
      else if ( strcmp( argv[argn], "-t")==0 )
        {
	  ++argn;
	  NoTextures = TRUE;
        }
      else if ( strcmp( argv[argn], "-e")==0 )
        {
	  ++argn;
	  RemoveExtensions = FALSE;
        }
      else if ( strcmp( argv[argn], "-amb")==0 )
        {
	  ++argn;
	  AmbientScale = atof(argv[argn]);
	  ++argn;
	  if ((AmbientScale<0.0) || (AmbientScale== 0.0))
	    NoAmbient = TRUE;
	  else
	    NoAmbient = FALSE;
        }
      else if ( strcmp( argv[argn], "-white")==0 )
        {
	  ++argn;
	  WhiteIntensity = atof(argv[argn]);
	  ++argn;
        }
      else if ( strcmp( argv[argn], "-envmult")==0 )
        {
	  ++argn;
	  EnvMult = !EnvMult;
        }
      else if ( strcmp( argv[argn], "-envmap")==0 )
        {
	  ++argn;
	  DoEnvironmentMap = !DoEnvironmentMap;
        }
      else if ( strcmp( argv[argn], "-cl")==0 )
        {
	  ++argn;
	  CommentLevel = atol(argv[argn]);
	  ++argn;
        }
      else if ( strcmp( argv[argn], "-sa")==0 )
        {
	  ++argn;
	  strcpy(scriptFile,argv[argn]);
	  ++argn;
	  if (scriptNew)
	    {
	      fprintf(stderr,"ERROR:-sa and -sn are mutually exclusive.\n");
	      return(-1);
	    }
	  OutScript = fopen( scriptFile, "a");
 		setvbuf(OutScript, NULL, _IOFBF, LW_IOBUF_SIZE);
	  if (OutScript == NULL)
	    {
	      fprintf(stderr,"ERROR: Can't of script file %s\n",scriptFile);
	      return(-1);
	    }
	  validScript = TRUE;
	  scriptAppend = TRUE;
        }
      else if ( strcmp( argv[argn], "-sn")==0 )
        {
	  ++argn;
	  strcpy(scriptFile,argv[argn]);
	  ++argn;
	  if (scriptAppend)
	    {
	      fprintf(stderr,"ERROR:-sa and -sn are mutually exclusive.\n");
	      return(-1);
	    }
	  OutScript = fopen( scriptFile, "w");
		setvbuf(OutScript, NULL, _IOFBF, LW_IOBUF_SIZE);
	  if (OutScript == NULL)
	    {
	      fprintf(stderr,"ERROR: Can't of script file %s\n",scriptFile);
	      return(-1);
	    }
	  validScript = TRUE;
	  scriptAppend = TRUE;
        }
      else if ( strcmp( argv[argn], "-ta")==0 )
        {
	  ++argn;
	  strcpy(TexFileName,argv[argn]);
	  ++argn;
	  if (texNew)
	    {
	      fprintf(stderr,"ERROR:-ta and -tn are mutually exclusive.\n");
	      return(-1);
	    }
	  err = Texture_ReadFile(TexFileName, &GTextures, &NTextures, &CurTexture); 
	  if (err != M2E_NoErr)
	    {
	      fprintf(stderr,"ERROR:Error during read of %s\n",TexFileName);
	      return(-1);
	    }
	  TexScript = fopen( TexFileName, "w");
	  setvbuf(TexScript, NULL, _IOFBF, LW_IOBUF_SIZE);
	  if (TexScript == NULL)
	    {
	      fprintf(stderr,"ERROR: Can't open script file %s\n",TexFileName);
	      return(-1);
	    }
	  validTexScript = TRUE;
	  texAppend = TRUE;
	  GlobalTextures = TRUE;
        }
      else if ( strcmp( argv[argn], "-tn")==0 )
        {
	  ++argn;
	  strcpy(TexFileName,argv[argn]);
	  ++argn;
	  if (texAppend)
	    {
	      fprintf(stderr,"ERROR:-ta and -tn are mutually exclusive.\n",TexFileName);
	      return(-1);
	    }
	  TexScript = fopen( TexFileName, "w");
		setvbuf(TexScript, NULL, _IOFBF, LW_IOBUF_SIZE);
	  if (TexScript == NULL)
	    {
	      fprintf(stderr,"ERROR: Can't open script file %s\n", 
		      TexFileName);
	      return(-1);
	    }
	  validTexScript = TRUE;
	  texAppend = texNew = TRUE;
        }
      else if ( strcmp( argv[argn], "-ma")==0 )
        {
	  ++argn;
	  strcpy(MatFileName,argv[argn]);
	  ++argn;
	  if (matNew)
	    {
	      fprintf(stderr,"ERROR:-ma and -mn are mutually exclusive.\n");
	      return(-1);
	    }
	  err = Material_ReadFile(MatFileName, &GMaterials, &NMaterials, &CurMaterial); 
	  if (err != M2E_NoErr)
	    {
	      fprintf(stderr,"ERROR:Error during read of %s\n",MatFileName);
	      return(-1);
	    }
	  MatScript = fopen( MatFileName, "w");
	  setvbuf(MatScript, NULL, _IOFBF, LW_IOBUF_SIZE);
	  if (MatScript == NULL)
	    {
	      fprintf(stderr,"ERROR: Can't open script file %s\n",MatFileName);
	      return(-1);
	    }
	  validMatScript = TRUE;
	  matAppend = TRUE;
	  GlobalMaterials = TRUE;
        }
      else if ( strcmp( argv[argn], "-mn")==0 )
        {
	  ++argn;
	  strcpy(MatFileName,argv[argn]);
	  ++argn;
	  if (matAppend)
	    {
	      fprintf(stderr,"ERROR:-ma and -mn are mutually exclusive.\n",MatFileName);
	      return(-1);
	    }
	  MatScript = fopen( MatFileName, "w");
		setvbuf(MatScript, NULL, _IOFBF, LW_IOBUF_SIZE);
	  if (MatScript == NULL)
	    {
	      fprintf(stderr,"ERROR: Can't open script file %s\n", 
		      MatFileName);
	      return(-1);
	    }
	  validMatScript = TRUE;
	  matAppend = matNew = TRUE;
        }
      else
	{
	  fprintf(stderr,"Usage: %s <FileIn> <FileOut>\n",argv[0]);
	  return(-1);
	}
    }

  /* Open the input file. */
  if (argn != argc)
    {
      strcpy( fileOut, argv[argn] );
    }
  else
    {
      /* No input file specified. */
      fprintf(stderr,"Usage: %s <FileIn> <FileOut>\n",argn, argc, argv[0]);
      return(-1);   
    }

  if (!validScript)
    {  
      OutScript = stderr;
    }

#endif  

  strcpy(TexMatName, fileOut);
  lastExtension = (char *)strrchr(TexMatName, '.');
  
  /* Replace any extension */
  if ((lastExtension != NULL) && (RemoveExtensions))
    {
      for( i=1; lastExtension[i] != '\0'; i++)
	{
	  if (!((lastExtension[i] <= '9') && (lastExtension[i] >= '0')))
	    {  /* If it's not an image number like .120, replace it */
	      *lastExtension = '\0';
	      break;
	    }
	}
    }

  if (SepMatTex)
    {
      if (!validTexScript)
	{
	  sprintf(TexFileName,"%s.mat",TexMatName);
	  MatOutFile = fopen( TexFileName, "w");
	  setvbuf(MatOutFile, NULL, _IOFBF, LW_IOBUF_SIZE);
	  if (MatOutFile == NULL)
	    {
	      fprintf(stderr,"ERROR: Can't of texture file %s\n", 
		      TexFileName);
	      return(-1);
	    }
	  SepMatScript = TRUE;
	}
	  
      if (!validTexScript)
	{
	  sprintf(TexFileName,"%s.tex",TexMatName);
	  TexOutFile = fopen( TexFileName, "w");
	  setvbuf(TexOutFile, NULL, _IOFBF, LW_IOBUF_SIZE);
	  if (TexOutFile == NULL)
	    {
	      fprintf(stderr,"ERROR: Can't of script file %s\n", 
		      TexFileName);
	      return(-1);
	    }
	  SepTexScript = TRUE;
	}
    }

  Name_Process(TexMatName, &BaseName, FALSE);

  err = ReadIFF(fileIn, fileOut);

  if (err != M2E_NoErr)
    {
      fprintf(stderr,"ERROR:Bad file \"%s\"\n",fileIn);
      return(-1);
    }

  if (OutScript != stderr)
    fclose(OutScript);

  if (err == M2E_NoErr)
    return(0);
  else if (err == M2E_BadFile)
    fprintf(stderr,"ERROR:Bad File \"%s\"\n", fileIn);
    return(-1);
}




