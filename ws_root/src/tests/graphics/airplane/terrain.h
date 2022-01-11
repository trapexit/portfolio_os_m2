#ifndef _TERRAIN_H
#define _TERRAIN_H
/******************************************************************************
**
**  @(#) terrain.h 96/04/15 1.3
**
******************************************************************************/

/*
** Generate fractal terrain.
**
** Author: phil Burk
** Copyright 1995 3DO
** All Rights Reserved
*/

#include <kernel/types.h>
#include "stdio.h"
#include "math.h"

#include <graphics/gp.h>
#include <graphics/fw.h>
#include <graphics/graphics.h>

typedef struct Terrain
{
	int32     tran_NumRows;
	int32     tran_NumCols;
/* The heights array is ordered first by all the elements of the first row. */
	float32  *tran_HeightArray;
	Point3   *tran_Vertices;
	Color4   *tran_VertexColors;
} Terrain;

#define kWaterLevel      (0.0)
#define kGridWidth       (25000.0)
#define kVerticalScale   (7500.0)

Err CreateTerrainData( Terrain **tranPtr, int32 numRows, int32 numCols );
void DeleteTerrainData( Terrain **tranPtr );
void GenerateTerrain( Terrain *tran );


#endif
