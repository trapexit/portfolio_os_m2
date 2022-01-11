/* @(#) terrain.c 96/04/15 1.3 */

/*************************************************
** Fractal Terrain Generator
**
** Author: Phil Burk
** Copyright 1995, The 3DO Company
**************************************************/

#include <kernel/kernel.h>
#include <kernel/mem.h>
#include <kernel/item.h>
#include <kernel/random.h>
#include <stdio.h>

/* Application specific includes. */
#include "plane.h"

/* Handy printing and debugging macros. */
#define	PRT(x)	{ printf x; }
#define	ERR(x)	PRT(x)
#define	DBUG(x)	/* PRT(x) */

#define RAND01  ( (((uint32)rand()) & 0xFF) / 256.0 )
#define RANDTRI01  ((RAND01 + RAND01)/2.0)
#define UNSET_ELEVATION   (-9000000.0)

/********************************************************************************/
void DeleteTerrainData( Terrain **tranPtr )
{
	Terrain *tran;

	if( tranPtr == NULL ) return;
	tran = *tranPtr;
	if( tran == NULL ) return;

	if( tran->tran_VertexColors ) free( tran->tran_VertexColors );
	if( tran->tran_Vertices ) free( tran->tran_Vertices );
	if( tran->tran_HeightArray ) free( tran->tran_HeightArray );
	free( tran );
	*tranPtr = NULL;
}

/********************************************************************************/
Err CreateTerrainData( Terrain **tranPtr, int32 numRows, int32 numCols )
{
	Terrain *tran;
	int32    numPoints;
	
DBUG(("CreateTerrainData( ,%d,%d)\n", numRows, numCols));
	tran = malloc( sizeof(Terrain) );
	if( tran == NULL ) return -1;
	memset( tran, 0, sizeof(Terrain));
	
	tran->tran_NumRows = numRows;
	tran->tran_NumCols = numCols;
	tran->tran_HeightArray = NULL;
	tran->tran_Vertices = NULL;
	tran->tran_VertexColors = NULL;
	
/* Allocate arrays. */
	numPoints = numRows * numCols;
	tran->tran_HeightArray = malloc( numPoints * sizeof(float32) );
	if( tran->tran_HeightArray == NULL ) goto nomem_error;
	
	tran->tran_Vertices = malloc( numPoints * sizeof(Point3) );
	if( tran->tran_Vertices == NULL ) goto nomem_error;
	
	tran->tran_VertexColors = malloc( numPoints * sizeof(Color4) );
	if( tran->tran_VertexColors == NULL ) goto nomem_error;
	
	*tranPtr = tran;
	return 0;
	
nomem_error:
	DeleteTerrainData( &tran );
	return -1;
}


/********************************************************************************/
#define SUBDIVIDE_SEGMENT( dst_r,dst_c, src_r1, src_c1, src_r2, src_c2 ) \
	if( zarray[(ir+(dst_r))*numCols + (ic+(dst_c))] == UNSET_ELEVATION ) \
	{	zarray[(ir+(dst_r))*numCols + (ic+(dst_c))] = \
		((( zarray[(ir+(src_r1))*numCols + (ic+(src_c1))] + \
		    zarray[(ir+(src_r2))*numCols + (ic+(src_c2))])/2.0) + \
		   (RANDTRI01 - 0.5)*scale); \
	}

/********************************************************************************/
static void SubdivideElevationQuad( int32 numRows, int32 numCols, gfloat *zarray,
	      gfloat scale, uint32 ir, uint32 ic, uint32 dr, uint32 dc )
{
	uint32  hr,hc;

DBUG(("SubdivideElevationQuad: scale = %g, ir = %d, ic = %d, dr = %d, dc = %d\n", scale, ir,ic,dr,dc ));
/* Generate subdivided indices. */
	hr = dr / 2;
	hc = dc / 2;

/* Fill in new values between rows. */
	if( hr > 0 )
	{
		SUBDIVIDE_SEGMENT( hr,0, 0,0, dr,0);
		SUBDIVIDE_SEGMENT( hr,dc, 0,dc, dr,dc);
	}
/* Fill in new values between columns. */
	if( hc > 0 )
	{
		SUBDIVIDE_SEGMENT( 0,hc, 0,0, 0,dc);
		SUBDIVIDE_SEGMENT( dr,hc, dr,0, dr,dc);
	}
/* Center point. */
	if( (hr > 0) && (hc > 0) )
	{
		SUBDIVIDE_SEGMENT( hr,hc, 0,0, dr,dc);
	}

/* Recurse */
	if( (hr > 1) || (hc > 1) )
	{
		SubdivideElevationQuad( numRows, numCols, zarray, scale/2.0, ir,ic, hr,hc );
		SubdivideElevationQuad( numRows, numCols, zarray, scale/2.0, ir,ic+hc, hr,dc-hc );
		SubdivideElevationQuad( numRows, numCols, zarray, scale/2.0, ir+hr,ic, dr-hr,hc );
		SubdivideElevationQuad( numRows, numCols, zarray, scale/2.0, ir+hr,ic+hc, dr-hr,dc-hc );
	}
}

/********************************************************************************/
static void GenerateFractalElevations( int32 numRows, int32 numCols, gfloat *zarray )
{
	uint32 ir,dr, ic,dc;

/* Preset heights to avoid cliffs along major subdivisions. */
	for( ir=0; ir<(numRows*numCols); ir++ ) zarray[ir] = UNSET_ELEVATION;

	ir = 0;
	ic = 0;
	dr = numRows - 1;
	dc = numCols - 1;
/* First we set the corner elevations. */
	zarray[0] = kWaterLevel;
	zarray[dc] = kWaterLevel;
	zarray[dr*numCols] = kWaterLevel;
	zarray[dr*numCols + dc] = kWaterLevel;

/* Now we recursively subdivide that quad. */
	SubdivideElevationQuad( numRows, numCols, zarray, kVerticalScale, ir, ic, dr, dc );
}

/********************************************************************************/
void GenerateTerrain( Terrain *tran )
{
	int32           ix,iz, i;
	gfloat          vx,vy,vz;
	Point3         *pp;
	Color4	       *cc;
	gfloat          hMin, hMax, hNorm, hCur;

	GenerateFractalElevations( tran->tran_NumRows, tran->tran_NumCols, tran->tran_HeightArray);

/* Fill in vertices with elevation data and grid. */
DBUG(("Fill in vertices with elevation data and grid.\n"));
	pp = tran->tran_Vertices;
	hMin = 99999.0;
	hMax = -hMin;
	for( ix=0; ix<tran->tran_NumRows; ix++ )
	{
		for( iz=0; iz<tran->tran_NumCols; iz++ )
		{
			vx = ((((gfloat)ix)/tran->tran_NumRows) * kGridWidth) - (kGridWidth/2.0);
			hCur = tran->tran_HeightArray[ix*tran->tran_NumCols + iz];
			
			if( hCur < kWaterLevel ) hCur = kWaterLevel; /* Set ocean level. */
			if( hCur > hMax ) hMax = hCur;
			if( hCur < hMin ) hMin = hCur;
			vy = hCur;
			vz = ((((gfloat)iz)/tran->tran_NumCols)  * kGridWidth) - (kGridWidth/2.0);
			DBUG(("v[%d,%d] = %g, %g, %g\n", iz,ix,vx,vy,vz));
			Pt3_Set( pp, vx, vy, vz );
			pp++;  /* Must increment outside Pt3_Set cuz its a macro! */
		}
	}
	PRT(("hMin = %g, hMax = %g\n", hMin, hMax ));

/* Normalize colors to height range. */
DBUG(("Normalize colors to height range.\n"));
	hNorm = 1.0/(hMax - hMin);
	cc = &tran->tran_VertexColors[0];
	for( i=0; i<(tran->tran_NumRows*tran->tran_NumCols); i++ )
	{
		gfloat r,g,b,cnorm;
		hCur = tran->tran_HeightArray[i];
		cnorm = (hCur - hMin)*hNorm;
		
/* Paint water blue. */
		if( hCur < kWaterLevel )
		{
			r = 0.1;
			g = (RAND01 * 0.1) + 0.3;
			b = (RAND01 * 0.1) + 0.7;
			DBUG(("Water r,g,b = %8g,%8g,%8g\n", r,g,b ));
		}
/* Mountain tops with snow. */
		else if ( (cnorm*(RAND01+1.0)) > 0.75 )
		{
			r = g = b = (RAND01 * 0.1) + 0.9;
		}
/* Green and brown. */
		else
		{
			r = (RAND01 * 0.3) + (cnorm * 0.4) + 0.3;
			g = (RAND01 * 0.3) + ((1.0-cnorm) * 0.5) + 0.1;
			b = (RAND01 * 0.3) + (cnorm * 0.4) + 0.2;
		}
		Col_Set(cc, r, g, b, 1.0);
		cc++;
	}
DBUG(("Finished generating terrain.\n"));
}
