/*
#include "fw.i"
*/
#include "gp.i"
#include "tmutils.h"

#define VERT_SEPARATE_UV 1

typedef struct arc
{
	long *pusage;
	struct arc *pnextarc;
	struct arc *potherarc;
} arc;

/* counterclockwise arcs for each point, followed by clockwise */
typedef struct arcusage
{
	arc *parc[6];
	struct arcusage *pnextarcusage;
	struct arcusage *pprevarcusage;
	long mytriangle;
} arcusage;

extern int facetSurfaceFlag;
long triangleCount,*pTriangles;
long pointCount;
long twoSidedFlag;

/* Allocated arrays */
long *pConn;	/* each triangle's three neighbors, or -1 if nothing there */
long *pResult;	/* returned by MakeStrips */
long *pUsed;	/* non-zero if triangle already used */

long arcCount;	/* six arcs for each vertex, plus one to terminate lists */
arc *pArcs;		/* speedier version of pConn */

long *pOneSnake;	/* triangles used in a single trial snake */

uint16 		*indexArray;
#ifdef VERT_SEPARATE_UV
uint16 		*indexUVArray;
#endif

/* STRICTLY USED TO SPEED UP EDGE SEARCH! */
long *pPointLinks;
long *pPointBases;

arcusage *pArcUsage;
arcusage anchorArcUsage;

long AllocateMemory(void);
void FreeMemory(void);
long FindEdges(void);
void MakeArcs(void);
void SearchForSnakes(void);

/*
 *	Make a list of strips for input triangles
 *	The input is "countoftriangles" triangles, each described by three longs,
 *	which are the vertex indicies.  The vertex coordinates are irrelevant and
 *	not described.
 *	If the twosided input is zero, then the triangles are one sided; each is
 *	visible when the points are in counter-clockwise order.  If the twosided
 *	input is non-zero, the triangles are two-sided; triangles are flipped
 *	internally to create ordered surfaces, which should be completely
 *	irrelevant to display, as two-sided surfaces are not culled.
 *
 *	A pointer to a set of snakes is returned.  Several error conditions will
 *	cause the return of a null pointer:
 *		If a single vertex is used multiple times in a triangle
 *		If a negative vertex index is used in a triangle, or one over 65535
 *		Failure to allocate an array
 *
 *
 *	All errors have unique error messages.
 *
 *	MakeSnake allocates several arrays, which it frees -- except, of course,
 *	for the returned array. Allocations are all done by need.
 *
 *	The returned array contains snakes (combination strip/fans) in the format
 *
 *	Three points
 *	Arbitrary number of additional points
 *	Snake terminator
 *	Three points
 *	Arbitrary number of additional points
 *	Snake terminator
 *	...
 *	Global terminator
 *
 *	The array consists of long words.  If the first three points in the
 *	snake are given in reverse order from the input array triangle, then
 *	0x10000 is added to the first point value (unless twosided is set,
 *	in which case nothing is added).  Subsequent points have 0x10000 added
 *	to them if the middle point is to be replaced.  The snake terminator
 * 	is -1; the global terminator is -2.
 */
long *MakeSnake(long countoftriangles, long *ptrtotriangles, long twosided)
{
	pTriangles = ptrtotriangles;
	triangleCount = countoftriangles;
	twoSidedFlag = twosided;

	{
		long i,max;
		max = 0;
		for(i = 0; i < (triangleCount*3); i++)
		{
			if(*(pTriangles + i) < 0)
			{
				printf("MakeSnake: Negative vertex index used, aborting!\n");
				return(0);
			}
			if(*(pTriangles + i) > max)
				max = *(pTriangles + i);
		}
		pointCount = max+1;
	}
	if(!AllocateMemory())
	{
		printf("MakeSnake: Unable to allocate sufficient memory\n");
		FreeMemory();
		if(pResult)
			Mem_Free(pResult);
		return(0);
	}
	
	if(!FindEdges())
	{
		FreeMemory();
		if(pResult)
			Mem_Free(pResult);
		return(0);
	}

	MakeArcs();

	SearchForSnakes();
	
	FreeMemory();
	return(pResult);
}

long AllocateMemory(void)
{
	pConn = 0;
	pResult = 0;
	pUsed = 0;
	pArcs = 0;
	pArcUsage = 0;
	pOneSnake = 0;

	pPointLinks = 0;
	pPointBases = 0;
	
	pConn = (long *)Mem_Alloc(triangleCount * 3 * sizeof(long));
	if(!pConn)
		return(0);

	pResult = (long *)Mem_Alloc(((triangleCount * 4) + 1) * sizeof(long));
	if(!pResult)
		return(0);

	pUsed = (long *)Mem_Alloc((triangleCount  + 1) * sizeof(long));
	if(!pUsed)
		return(0);

	pArcs = (arc *)Mem_Alloc((triangleCount * 6 + 1) * sizeof(struct arc));
	if(!pArcs)
		return(0);

	pArcUsage = (arcusage *)Mem_Alloc(triangleCount * sizeof(struct arcusage));
	if(!pArcUsage)
		return(0);

	pOneSnake = (long *)Mem_Alloc(triangleCount * sizeof(long));
	if(!pOneSnake)
		return(0);

	pPointLinks = (long *)Mem_Alloc(triangleCount * 3 * sizeof(long));
	if(!pPointLinks)
		return(0);

	pPointBases = (long *)Mem_Alloc(pointCount * sizeof(long));
	if(!pPointBases)
		return(0);

	return(1);
}

/* 
 * pResult is freed separately, as it is needed for the return array 
 */
void FreeMemory(void)
{
	if(pConn)
		Mem_Free(pConn);
	if(pUsed)
		Mem_Free(pUsed);
	if(pArcs)
		Mem_Free(pArcs);
	if(pArcUsage)
		Mem_Free(pArcUsage);
	if(pOneSnake)
		Mem_Free(pOneSnake);

	if(pPointLinks)
		Mem_Free(pPointLinks);
	if(pPointBases)
		Mem_Free(pPointBases);
}

long FindEdges()
{
	long i,j;
	long side,point;
	
	register long tri,othertri;
	register long *ptri,*pothertri;

	/*
	 * -1 means no neighbor on that side
	 * side 0 is between points 0 and 1
	 * side 1 is between points 1 and 2
	 * side 2 is between points 2 and 0
	 */
	for (i = 0; i < (triangleCount * 3); i++)
		*(pConn + i) = -1;

	{
		long *plong;
		plong = pTriangles;

		for(tri = 0; tri < triangleCount; tri++)
		{
			if((*plong == *(plong+1)) || (*(plong+1) == *(plong+2)) || (*plong == *(plong+2)))
			{
				printf("MakeSnake: Degenerate triangle, might cause problem!!!\n");
			}
			plong += 3;
		}
	}
/*
 *	Create pPointLinks and pPointBases to speed the edge search from O(n**2) to
 *	roughly O(n).  pPointBases contains the first usage of each point, or -1 if it
 *	is not used; divide by 3 to get which triangle, % 3 indicates position in triangle.
 *	pPointLinks contains the next usage of each point, or -1 if there is no next usage,
 *	in the same format as pPointBases.
 */
	for(i = 0; i < (triangleCount*3); i++)
		*(pPointLinks + i) = -1;
		
	for(i = 0; i < pointCount; i++)
		*(pPointBases + i) = -1;

	for(i = 0; i < (triangleCount*3); i++)
	{
		point = *(pTriangles + i);
		*(pPointLinks + i) = *(pPointBases + point);
		*(pPointBases + point) = i;
	}

	ptri = pTriangles;
	for(tri = 0; tri < triangleCount; tri++)
	{
/*
 		Slow code:
 		pothertri = pTriangles + (tri + 1)*3;	
 		for(othertri = tri+1; othertri < triangleCount; othertri++, pothertri += 3)
 		{
 			Two points must match for there to be a common edge, therefore
 			one of the first two points must match -- no need to check
 			the third.
 			if((*ptri != *pothertri) && (*(ptri+1) != *pothertri) &&
 			   (*ptri != *(pothertri+1)) && (*(ptri+1) != *(pothertri+1)) &&
 			   (*ptri != *(pothertri+2)) && (*(ptri+1) != *(pothertri+2)))
 				continue;
*/

/*		Corresponding fast code:		*/

		for(side = 0; side < 3; side++)
		{
		  point = *(pPointBases + *(ptri + side));
		  while(point != -1)
		  {
		  	othertri = point/3;
		  	point = *(pPointLinks + point);
		  	if(othertri <= tri)
		  		continue;
		  	pothertri = pTriangles + othertri*3;

/*		Corresponding fast code ends */

			for(i = 0; i < 3; i++)
			{
				for(j = 0; j < 3; j++)
				{
					if(	(*(ptri+i) == *(pothertri+j)) && (*(ptri+((i+1)%3)) == *(pothertri+((j+1)%3))) )
					{
						/*
						 * The orientation of the edge should be different from
						 * each triangle's perspective; bad edges in two-sided surfaces are removed below. 
						 */
						if(!twoSidedFlag)
							continue;
					}
					else if( (*(ptri+i) != *(pothertri+((j+1)%3))) || (*(ptri+((i+1)%3)) != *(pothertri+j)) )
						continue;

					/* Is this side already taken in either triangle? */

					if( (*(pConn + tri*3 + i) >= 0) || (*(pConn + othertri*3 + j) >= 0) )
						continue;
					*(pConn + tri*3 + i) = othertri;
					*(pConn + othertri*3 + j) = tri;

					/* break out of both loops -- only one adjacent side allowed per triangle pair */
					break;
				}
				if(j != 3)
					break;
			}	
		  }	
		}

		ptri += 3;
	}

	if(!twoSidedFlag)
		return(1);

	/*
	 * Edges where edge directions match are removed from two-sided surfaces
	 */
	{
		long firstunfrozen,frozesomething;
		long frozenneighbors,badneighbors;
		long temp;

		/*
	 	 * pUsed is used as a temporary array, with a different meaning than
		 * normal.  During the below processing, a "1" indicates that the
		 * orientation of a triangle has been frozen
		 */
		for(tri = 0; tri < triangleCount; tri++)
			*(pUsed + tri) = 0;

		while(1)
		{
			firstunfrozen = -1;
			frozesomething = 0;
			ptri = pConn;
			for(tri = 0; tri < triangleCount; tri++, ptri += 3)
			{
				if(*(pUsed+tri))
					continue;
				if(firstunfrozen == -1)
					firstunfrozen = tri;

				/*
				 * Process each unfrozen triangle adjoining a frozen one
	 			 */
				frozenneighbors = 0;
				badneighbors = 0;

				for(i = 0; i < 3; i++)
				{
					othertri = *(ptri+i);
					if(othertri < 0)
						continue;
					if(*(pUsed + othertri))
					{
						frozenneighbors++;
						for(j = 0; j < 3; j++)
							if(*(pConn + othertri*3 + j) == tri)
								break;
						if(*(pTriangles + tri*3 + i) == *(pTriangles + othertri*3 + j))
							badneighbors++;
					}
				}
				
				if(!frozenneighbors)
					continue;

				/*
				 * If the bad count is greater than the good count, flip this triangle
				 */
				if(badneighbors > (frozenneighbors - badneighbors))
				{
/*
					printf("Flipping triangle %d\n",tri);
*/
					temp = *ptri;
					*ptri = *(ptri+2);
					*(ptri+2) = temp;
					temp = *(pTriangles+tri*3+1);
					*(pTriangles+tri*3+1) = *(pTriangles+tri*3+2);
					*(pTriangles+tri*3+2) = temp;
				}

				/*
				 *	Delete the common bad edges between this triangle and the adjoining frozen triangles
				 */
				for(i = 0; i < 3; i++)
				{
					othertri = *(ptri+i);
					if(othertri < 0)
						continue;
					if(*(pUsed + othertri))
					{
						for(j = 0; j < 3; j++)
							if(*(pConn + othertri*3 + j) == tri)
								break;
						if(*(pTriangles + tri*3 + i) == *(pTriangles + othertri*3 + j))
						{
/*
							printf("Removing edge between %d and %d\n",tri,othertri);	
*/
							*(ptri+i) = -1;
							*(pConn + othertri*3 + j) = -1;
						}
					}
				}

				/*
				 * Freeze this triangle
				 */

				*(pUsed + tri) = 1;
				frozesomething++;
			}
			if(firstunfrozen == -1)
				break;
			if(!frozesomething)
				*(pUsed + firstunfrozen) = 1;
		}
	}
		
	return(1);
}

void MakeArcs(void)
{
	long i,j;
	long tri,nexttri;
	arc *pcurarc;
	
	/*
	 * If an arc reaches its end, rather than use a pointer of zero,
	 * it points to this "terminal" arc structure, which has a usage
	 * pointer to the final entry in pUsed.
	 */
	(pArcs)->pusage = pUsed + triangleCount;
	(pArcs)->pnextarc = 0;
	(pArcs)->potherarc = 0;

	/*
	 * All counterclockwise arcs are followed by all clockwise arcs,
	 * allowing for greater memory coherence during the snake creation process
	 */
	{
		arcusage *pau;
		for(tri = 0; tri < triangleCount; tri++)
		{
			pau = pArcUsage + tri;
			for(i = 0; i < 3; i++)
				pau->parc[i] = pArcs + 1 + (tri*3 + i);
			for(i = 0; i < 3; i++)
				pau->parc[i+3] = pArcs + (triangleCount*3 + 1) + (tri*3 + i);
			if(!tri)
				pau->pprevarcusage = &anchorArcUsage;
			else
				pau->pprevarcusage = pau - 1;
			if(tri == (triangleCount - 1))
				pau->pnextarcusage = 0;
			else
				pau->pnextarcusage = pau + 1;
			pau->mytriangle = tri;
		}
		anchorArcUsage.pnextarcusage = pArcUsage;
	}

	for(tri = 0; tri < triangleCount; tri++)
	{
		for(i = 0; i < 3; i++)
		{
			pcurarc = (pArcUsage+tri)->parc[i];
			pcurarc->pusage = pUsed + tri;
			nexttri = *(pConn + tri*3 + ((i+2)%3) );
			if(nexttri == -1)
				pcurarc->pnextarc = pArcs;
			else
			{
				for(j = 0; j < 3; j++)
					if(*(pTriangles + tri*3 + i) == *(pTriangles + nexttri*3 + j))
						break;
				pcurarc->pnextarc = (pArcUsage + nexttri)->parc[j];
			}
			
			nexttri = *(pConn + tri*3 + ((i+1)%3) );
			if(nexttri == -1)
				pcurarc->potherarc = pArcs;
			else
			{
				for(j = 0; j < 3; j++)
					if(*(pTriangles + tri*3 + ((i+2)%3) ) == *(pTriangles + nexttri*3 + j))
						break;
				pcurarc->potherarc = (pArcUsage + nexttri)->parc[j];
			}
		}
	
		for(i = 0; i < 3; i++)
		{
			pcurarc = (pArcUsage+tri)->parc[i+3];
			pcurarc->pusage = pUsed + tri;
			nexttri = *(pConn + tri*3 + i);
			if(nexttri == -1)
				pcurarc->pnextarc = pArcs;
			else
			{
				for(j = 0; j < 3; j++)
					if(*(pTriangles + tri*3 + i) == *(pTriangles + nexttri*3 + j))
						break;
				pcurarc->pnextarc = (pArcUsage + nexttri)->parc[j+3];
			}
			
			nexttri = *(pConn + tri*3 + ((i+1)%3) );
			if(nexttri == -1)
				pcurarc->potherarc = pArcs;
			else
			{
				for(j = 0; j < 3; j++)
					if(*(pTriangles + tri*3 + ((i+1)%3) ) == *(pTriangles + nexttri*3 + j))
						break;
				pcurarc->potherarc = (pArcUsage + nexttri)->parc[j+3];
			}
		}
	}

}

/*
 *	pUsed indicates whether or not the triangle has been used in the current
 *	snake trial.  pUsed is only zeroed at the top of the routine. Later,
 *	pUsed is set to a different value (called "time") for each snake trial;
 *	if this value is encountered, that means that this particular snake
 *	already used that triangle.  After the best candidate is chosen,
 *	pArcs is modified to remove all references to that triangle; triangles
 *	that formerly were linked to it are now linked to the "terminal" arc.
 *
 *	A doubly-linked list specifying all remaining triangles is also maintained
 *	through the arcusage structures.  This list is traversed twice; once for
 *	the counter-clockwise snakes, once for the clockwise.
 */
void SearchForSnakes(void)
{
	long snakecount;

	arcusage *pau;
	long bestlength,besttri,bestpoint;

	long orient,point;
	long *padd;

	register long length,time,*ptrtousage;
	register arc *parc,*pnext;
	
	{
		long i;
		for(i = 0; i < triangleCount; i++)
			*(pUsed + i) = 0;
	}

	snakecount = 0;
	time = 0;
	padd = pResult;
	while(!facetSurfaceFlag)
	{
		bestlength = 0;

		for(orient = 0; orient < 6; orient += 3)
		{
			pau = anchorArcUsage.pnextarcusage;
			while(pau)
			{
				for(point = 0; point < 3; point++)
				{
					length = 0;
					pnext = pau->parc[point+orient];

					ptrtousage = pnext->pusage;

					time++;
					*(pUsed + triangleCount) = time;

					/*
					 * If it will immediately go to "other", let some other arc do this sequence
					 * This will leave all single triangles for later
					 */
					if(*((pnext->pnextarc)->pusage) == time)
						continue;

					do
					{
						*ptrtousage = time;
						length++;
						parc = pnext;

						pnext = parc->pnextarc;
						ptrtousage = pnext->pusage;
						if(*ptrtousage != time)
							continue;
						
						pnext = parc->potherarc;
						ptrtousage = pnext->pusage;
					}
					while(*ptrtousage != time);
					
					if(length > bestlength)
					{
						bestlength = length;
						besttri = pau->mytriangle;
						bestpoint = point + orient;
					}
				}
				pau = pau->pnextarcusage;
			}
		}

		if(!bestlength)
			break;

		/* Above code included in this loop.  Different statements are marked */
		length = 0;
		pnext = (pArcUsage+besttri)->parc[bestpoint];		/* Changed */
		ptrtousage = pnext->pusage;
		time++;
		*(pUsed + triangleCount) = time;
		do
		{
			/* Derive the triangle being used from the pnext pointer */

			*(pOneSnake + length) = ((pnext - pArcs - 1)/3) % triangleCount;	/* New */
			*ptrtousage = time;
			length++;
			parc = pnext;

			pnext = parc->pnextarc;
			ptrtousage = pnext->pusage;
			if(*ptrtousage != time)
				continue;
			
			pnext = parc->potherarc;
			ptrtousage = pnext->pusage;
		}
		while(*ptrtousage != time);

		{
			long i,j,k;
			long tri,othertri;
			long *paddsave;
			long old,mid,new;
			
			/*
			 * write this snake out to presult.  The specific order of the
			 * first three points depends on the first four triangles in the
			 * snake.  Rather than case it out, the code just tries all six
			 * orientations.
			 */
			paddsave = padd;
			for(orient = 0; orient < 6; orient++)
			{
				padd = paddsave;
				tri = *pOneSnake;

				old = orient % 3;
				mid = (old + 1 + (orient/3)) % 3;
				new = 3 - old - mid;

				old = *(pTriangles + tri*3 + old);
				mid = *(pTriangles + tri*3 + mid);
				new = *(pTriangles + tri*3 + new);

				if(orient >= 3)
					*padd++ = 0x10000 + old;
				else
					*padd++ = old;
				*padd++ = mid;
				*padd++ = new;
				
				for(i = 1; i < length; i++)
				{
					tri = *(pOneSnake + i);

					/*
					 * If the new point and one other point are not contained in
					 * this new triangle, this orientation is not acceptable.
					 */
					for(j = 0; j < 3; j++)
						if(new == *(pTriangles + tri*3 + j))
							break;
					if(j == 3)
						break;
						
					for(k = 0; k < 3; k++)
						if(mid == *(pTriangles + tri*3 + k))
							break;
					if(k != 3)
					{
						/* continue as strip */

						old = mid;
						mid = new;
						new = *(pTriangles + tri*3 + (3-j-k));
						*padd++ = new;
						continue;
					}
					
					for(k = 0; k < 3; k++)
						if(old == *(pTriangles + tri*3 + k))
							break;
					if(k == 3)
						break;

					/* continue as fan */
					mid = new;
					new = *(pTriangles + tri*3 + (3-j-k));
					*padd++ = new + 0x10000;
				}
				if(i == length)
					break;
			}
			*padd++ = -1;
			snakecount++;

			/*
			 * Search the arc structures of all three neighbors of this triangle --
			 * if any of them point to this triangle, change the pointer to
			 * point to the "terminator" arc			
			 */
			for(i = 0; i < length; i++)
			{
				tri = *(pOneSnake + i);
				for(j = 0; j < 3; j++)
				{
					othertri = *(pConn + tri*3 + j);
					if(othertri == -1)
						continue;
					for(k = 0; k < 6; k++)
					{
						parc = (pArcUsage + othertri)->parc[k];
						if((parc->pnextarc)->pusage == (pUsed + tri))
							parc->pnextarc = pArcs;
						if((parc->potherarc)->pusage == (pUsed + tri))
							parc->potherarc = pArcs;
					}
				}

				/* Also remove this triangle from the linked list of triangles to check */
				pau = pArcUsage + tri;
				(pau->pprevarcusage)->pnextarcusage = pau->pnextarcusage;
				if(pau->pnextarcusage)
					(pau->pnextarcusage)->pprevarcusage = pau->pprevarcusage;
			}
		}
	}
	/* Add in all single triangles */
	{
		long singles;
		long tri;
		
		singles = 0;
		pau = anchorArcUsage.pnextarcusage;
		while(pau)
		{
			tri = pau->mytriangle;
			*padd++ = *(pTriangles + 3*tri);
			*padd++ = *(pTriangles + 3*tri + 1);
			*padd++ = *(pTriangles + 3*tri + 2);
			*padd++ = -1;
			singles++;
			pau = pau->pnextarcusage;
		}
		snakecount += singles;
	}
	*padd = -2;
}

#ifdef VERT_SEPARATE_UV
void
TmSnakeUV(TmTriMesh *tmesh, TmFormat format, long *pSnake, uint16 *idx_ptr,
	uint16 *idx_uv_ptr, int32 texIdx)
{
  int32 	*padd;
  int32		index;
  int32		indexUV;
  int32		count;
  bool 		cont;
  bool 		order, cur_order;
  bool 		type, cur_type;
  uint16	*pIndex, *pIndex1;
  uint16	*pUVIndex, *pUVIndex1;
  
  padd = pSnake;
  count = 0;
  while((index = *padd++) != -2)
    {
      if(index != -1) {
	count++;
      }
    }
  indexArray = (uint16 *)Mem_Alloc(count * sizeof(uint16));
  indexUVArray = (uint16 *)Mem_Alloc(count * sizeof(uint16));

  padd = pSnake;
  type = TM_STRIP;
  cont = TM_NEW;
  order = TM_COUNTERCLOCKWISE;
  cur_order = order;
  count = 0;
  pIndex = pIndex1 = indexArray;
  pUVIndex = pUVIndex1 = indexUVArray;
  while(*padd != -2)
    {
      if (*padd < 0x10000)
	{
	  if (*padd < 0)
	    {
	      index = *padd;
	      indexUV = *padd++;
	    }
	  else
	    {
	      index = idx_ptr[*padd];
	      indexUV = idx_uv_ptr[*padd++];
	    }
	}
      else
	{
	  indexUV = *padd;
	  index = idx_ptr[indexUV-0x10000] + 0x10000;
	  indexUV = idx_uv_ptr[indexUV-0x10000];
	  padd++;
	}
      if(index == -1) {
	format.type = type;
	format.cont = cont;
	format.order = order;
	if (GFX_OK != TM_StripFan(tmesh, format, count, 
				  pIndex, 
				  ((format.texCoord) ? pUVIndex : NULL), 
				  ((format.color) ? pIndex : NULL), 
				  texIdx, 0)) {
	  exit(-1);
			}
			type = TM_STRIP;
			cont = TM_NEW;
			order = TM_COUNTERCLOCKWISE;
			cur_order = order;
			count = 0;
			pIndex = pIndex1;
			pUVIndex = pUVIndex1;
		} else {
			if(index >= 0x10000) {
				index -= 0x10000;
				cur_type = TM_FAN;
			} else {
				cur_type = TM_STRIP;
			}
			if (type ^ cur_type) {
				format.type = type;
				format.cont = cont;
				format.order = order;
				if (GFX_OK != TM_StripFan(tmesh, format, count, 
									pIndex, 
									((format.texCoord) ? pUVIndex : NULL), 
									((format.color) ? pIndex : NULL), 
									texIdx, 0)) {
					exit(-1);
				}
				if (cur_type == TM_FAN) {	/* strip to fan */
					order = cur_order;
				} else {					/* fan to strip */
					order = cur_order ^ 1;
					cur_order = order;
				}
				type = cur_type;
				if (cont == TM_NEW) {
					cont = TM_CONTINUE;
				}
				count = 1;
				pIndex = pIndex1;
				pUVIndex = pUVIndex1;
			} else {
				if (cur_type == TM_STRIP) {
					if (((cont == TM_NEW) && (count >= 3)) ||
						(cont == TM_CONTINUE)) {
						cur_order ^= 1;
					}
				}
				count++;
			}
			*pIndex1++ = index;
			*pUVIndex1++ = indexUV;
		}
	}
	Mem_Free(pSnake);
}
#endif

void
TmSnake(TmTriMesh *tmesh, TmFormat format, long *pSnake, int32 texIdx)
{
	int32 		*padd;
	int32		index;
	int32		count;
	bool 		cont;
	bool 		order, cur_order;
	bool 		type, cur_type;
	uint16		*pIndex, *pIndex1;

	padd = pSnake;
	count = 0;
	while((index = *padd++) != -2)
	{
		if(index != -1) {
			count++;
		}
	}
	indexArray = (uint16 *)Mem_Alloc(count * sizeof(uint16));

	padd = pSnake;
	type = TM_STRIP;
	cont = TM_NEW;
	order = TM_COUNTERCLOCKWISE;
	cur_order = order;
	count = 0;
	pIndex = pIndex1 = indexArray;
	while(*padd != -2)
	{
		index = *padd++;
		if(index == -1) {
			format.type = type;
			format.cont = cont;
			format.order = order;
			if (GFX_OK != TM_StripFan(tmesh, format, count, 
							pIndex, 
							((format.texCoord) ? pIndex : NULL), 
							((format.color) ? pIndex : NULL), 
							texIdx, 0)) {
			    exit(-1);
			}
			type = TM_STRIP;
			cont = TM_NEW;
			order = TM_COUNTERCLOCKWISE;
			cur_order = order;
			count = 0;
			pIndex = pIndex1;
		} else {
			if(index >= 0x10000) {
				index -= 0x10000;
				cur_type = TM_FAN;
			} else {
				cur_type = TM_STRIP;
			}
			if (type ^ cur_type) {
				format.type = type;
				format.cont = cont;
				format.order = order;
				if (GFX_OK != TM_StripFan(tmesh, format, count, 
									pIndex, 
									((format.texCoord) ? pIndex : NULL), 
									((format.color) ? pIndex : NULL), 
									texIdx, 0)) {
					exit(-1);
				}
				if (cur_type == TM_FAN) {	/* strip to fan */
					order = cur_order;
				} else {					/* fan to strip */
					order = cur_order ^ 1;
					cur_order = order;
				}
				type = cur_type;
				if (cont == TM_NEW) {
					cont = TM_CONTINUE;
				}
				count = 1;
				pIndex = pIndex1;
			} else {
				if (cur_type == TM_STRIP) {
					if (((cont == TM_NEW) && (count >= 3)) ||
						(cont == TM_CONTINUE)) {
						cur_order ^= 1;
					}
				}
				count++;
			}
			*pIndex1++ = index;
		}
	}
	Mem_Free(pSnake);
}

