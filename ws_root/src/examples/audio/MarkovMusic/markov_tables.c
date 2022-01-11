/******************************************************************************
**
**  @(#) markov_tables.c 96/02/28 1.3
**
**	Markov Tables
**
**	Data tables for Markov Music
**	Author: rnm
**
******************************************************************************/

#include <audio/audio.h>
#include <audio/soundplayer.h>
#include <stdlib.h>
#include <stdio.h>
#include "markov_music.h"
#include "markov_tables.h"

const int32 SoundNumBlocks[NUMBER_OF_SOUNDS] =
{
	2, 3, 2, 1
};

/* ---- STOPPED_SOUND tables */

float32 mt11[2][2] =
{
	{ 0.3, 0.7 },
	{ 0.7, 0.3 }
};

float32 mt12[2][3] = 
{
	{ 0.3, 0.2, 0.5 },
	{ 0.3, 0.2, 0.5 }
};

float32 mt13[2][2] = 
{
	{ 0.5, 0.5 },
	{ 0.5, 0.5 } 
};

/* ---- MOVING_SOUND tables */

float32 mt21[3][2] = 
{
	{ 0.8, 0.2 },
	{ 0.8, 0.2 },
	{ 0.8, 0.2 }
};

float32 mt22[3][3] = 
{
	{ 0.3, 0.6, 0.1 },
	{ 0.6, 0.2, 0.2 },
	{ 0.6, 0.3, 0.1 }
};

float32 mt23[3][2] = 
{
	{ 0.7, 0.3 },
	{ 0.3, 0.7 },
	{ 0.5, 0.5 }
};

/* ---- FAST_SOUND tables */

float32 mt31[2][2] = 
{
	{ 0.3, 0.7 },
	{ 0.3, 0.7 }
};

float32 mt32[2][3] = 
{
	{ 0.7, 0.15, 0.15 },
	{ 0.15, 0.7, 0.15 }
};

float32 mt33[2][2] = 
{
	{ 0.7, 0.3 },
	{ 0.7, 0.3 }
};

/* ---- TRANSITION_SOUND tables */

float32 mt41[1][2] = 
{
	{ 0.6, 0.4 }
	
};

float32 mt42[1][3] = 
{
	{ 0.2, 0.2, 0.6 }
};

float32 mt43[1][2] = 
{
	{ 0.7, 0.3 }
};

float32* glTables[4][3] = 
{
	&(mt11[0][0]), &(mt12[0][0]), &(mt13[0][0]),
	&(mt21[0][0]), &(mt22[0][0]), &(mt23[0][0]),
	&(mt31[0][0]), &(mt32[0][0]), &(mt33[0][0]),
	&(mt41[0][0]), &(mt42[0][0]), &(mt43[0][0])
};

/******************************************************************************/
void mtPrintTable ( int32 from, int32 to )
{
	float32* table;
	int32 i,j;
	
	table = glTables[from][to];
	
	printf("Markov table from sound %i to sound %i\n\n", from, to);
	
	for (i=0;i<SoundNumBlocks[from];i++)
	{
		printf("%i: ",i);
		
		for (j=0;j<SoundNumBlocks[to];j++)
			printf("\t%f", *(table + i*SoundNumBlocks[to] + j));
			
		printf("\n");
	}
	
	return;
}

/*******************************************************************/
static float32 Choose ( float32 range )
{
	float32 val, r;

	r = (float32)(rand() & 0x0000FFFF);
	val = (r / 65536.0) * range;
	return val;
}

#define wChoose(min, max) (Choose( max - min ) + min)

/******************************************************************************/
char* mtChoose( int32 fromSound, int32 toSound, char* fromMarker,
  char* toMarker )
{
	float32* table;
	int32 row, i, bar;
	float32 cutoff, accum;
	
DBUG(("mtChoose: we're here\n"));

	table = glTables[fromSound][toSound];
	markerNametoNumber( fromMarker, &row, &bar );
	
	row -= 1;    /* map block1..n to row 0..n-1 */
	if (bar == 0) row -= 1;
		         /* marker blockn, bar 0 => just come out of blockn-1 */
	bar = MOD(bar, 4);    /* map bar to 0..3 */

DBUG(("mtChoose: looking at table %i:%i, row %i, sub %i\n",
  fromSound, toSound, row, bar));
  	
	cutoff = Choose(1.0);
	accum = *(table + row*SoundNumBlocks[toSound]);
	i = 0;
	while ((i < SoundNumBlocks[toSound]-1) && (accum < cutoff))
	{

DBUG(("mtChoose: cutoff is %f, accum is %f, i is %i\n", cutoff, accum, i));

		i += 1;
		accum += *(table + row*SoundNumBlocks[toSound] + i);
	}

DBUG(("mtChoose: picked column %i (accum = %f)\n", i, accum));

	markerNumbertoName( i+1, bar, toMarker );
	  /* map row 0..n-1 to block1..n */
	  
	return ( toMarker );
}

