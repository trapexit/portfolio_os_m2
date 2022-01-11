/*
	File:		M2TXDither.c

	Contains:	 

	Written by:	 

	Copyright:	© 1994 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		<1+>	  8/4/95	TMA		Fix compiler warnings.

	To Do:
*/



/* 
 * dither.c - Functions for RGB color dithering.
 * 
 * Author:	Spencer W. Thomas
 * 		Computer Science Dept.
 * 		University of Utah
 * Date:	Mon Feb  2 1987
 * Copyright (c) 1987, University of Utah
 */

#include <math.h>
#include <time.h>
#include <stdlib.h>
#include "M2TXTypes.h"
#include <stdio.h>
#include "qmem.h"
#include "M2TXDither.h"

static void	make_square( double, int [256], int [256], int [16][16] );

static int magic4x4[4][4] =  {
 	 0, 14,  3, 13,
	11,  5,  8,  6,
	12,  2, 15,  1,
	 7,  9,  4, 10
};

/* basic dithering macro */
#define DMAP(v,x,y)	(modN[v]>magic[x][y] ? divN[v] + 1 : divN[v])

/*****************************************************************
 * TAG( make_square )
 * 
 * Build the magic square for a given number of levels.
 * Inputs:
 * 	N:		Pixel values per level (255.0 / levels).
 * Outputs:
 * 	divN:		Integer value of pixval / N
 *	modN:		Integer remainder between pixval and divN[pixval]*N
 *	magic:		Magic square for dithering to N sublevels.
 * Assumptions:
 * 	
 * Algorithm:
 *	divN[pixval] = (int)(pixval / N) maps pixval to its appropriate level.
 *	modN[pixval] = pixval - (int)(N * divN[pixval]) maps pixval to
 *	its sublevel, and is used in the dithering computation.
 *	The magic square is computed as the (modified) outer product of
 *	a 4x4 magic square with itself.
 *	magic[4*k + i][4*l + j] = (magic4x4[i][j] + magic4x4[k][l]/16.0)
 *	multiplied by an appropriate factor to get the correct dithering
 *	range.
 */
static void
make_square( double N, int divN[256], int modN[256], int magic[16][16])
{
    register int i, j, k, l;
    double magicfact;

    for ( i = 0; i < 256; i++ )
    {
	divN[i] = (int)(i / N);
	modN[i] = i - (int)(N * divN[i]);
    }
    modN[255] = 0;		/* always */

    /*
     * Expand 4x4 dither pattern to 16x16.  4x4 leaves obvious patterning,
     * and doesn't give us full intensity range (only 17 sublevels).
     * 
     * magicfact is (N - 1)/16 so that we get numbers in the matrix from 0 to
     * N - 1: mod N gives numbers in 0 to N - 1, don't ever want all
     * pixels incremented to the next level (this is reserved for the
     * pixel value with mod N == 0 at the next level).
     */
    magicfact = (N - 1) / 16.;
    for ( i = 0; i < 4; i++ )
	for ( j = 0; j < 4; j++ )
	    for ( k = 0; k < 4; k++ )
		for ( l = 0; l < 4; l++ )
		    magic[4*k+i][4*l+j] =
			(int)(0.5 + magic4x4[i][j] * magicfact +
			      (magic4x4[k][l] / 16.) * magicfact);
}


#define FS_SCALE 1024

/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXRaw_DitherDown
|||	Reduce bitdepth of a literal color or alpha channel and dither it.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXRaw_DitherDown(M2TXRaw *raw, bool doColor, 
|||	                           uint8 endDepth, bool doFloyd)
|||	
|||	  Description
|||	
|||	    This function takes an M2TXRaw image and reduces the bit depth of the
|||	    literal values of the color alpha channel and dithers the final result.
|||	    With either ordered(16x16 Magic Square) or Floyd-Steinberg dithering.
|||	
|||	  Arguments
|||	    
|||	    raw
|||	        The input M2TXRaw image.
|||	    doColor
|||	        Should I reduce the color values (red, green, and blue).
|||	    endDepth
|||	        The final bits per component.
|||	    doFloyd
|||	        Whether to Floyd-Steinberg or order dithered.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXDither.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXRaw_MakePIP(), M2TXRaw_MapToPIP()
|||	
**/

M2Err M2TXRaw_DitherDown(M2TXRaw *raw, bool doColor, uint8 endDepth,
			 bool floyd)
{

  long *thisrerr;
  long *nextrerr;
  long *thisgerr;
  long *nextgerr;
  long *thisberr;
  long *nextberr;
  int rows, cols;
  int row, col;
  int fs_direction;
  int limitcol;
  long r, g, b, a, err;
  uint32 curPixel;
  long maxval = 255;
  /* Stuff for ordered dithering */
  int magic[16][16];
  int divN[256];
  int modN[256];
  int shiftUp;
  uint8 mask;

  shiftUp = 8 - endDepth;
  mask =  (0xFF >> shiftUp)<<shiftUp;

  if (raw == NULL)
    return(M2E_BadPtr);
  if (doColor)
   {
     if (((raw->Red)== NULL) || ((raw->Green)== NULL) || ((raw->Blue)== NULL))
       return(M2E_BadPtr);
   }
  else if (raw->Alpha == NULL)
    return(M2E_BadPtr);

  if ((endDepth < 1) || (endDepth > 7))
    return (M2E_Range);

  cols = raw->XSize;
  rows = raw->YSize;
  /* Allocate color Only */
  if ( floyd )
    {
      /* Initialize Floyd-Steinberg error vectors. */
      thisrerr = (long *) qMemNewPtr((cols+2)* sizeof(long) );
      nextrerr = (long *) qMemNewPtr((cols+2)* sizeof(long) );
      thisgerr = (long *) qMemNewPtr((cols+2)* sizeof(long) );
      nextgerr = (long *) qMemNewPtr((cols+2)* sizeof(long) );
      thisberr = (long *) qMemNewPtr((cols+2)* sizeof(long) );
      nextberr = (long *) qMemNewPtr((cols+2)* sizeof(long) );
      srand( (int) ( time( 0 ) ) );
      for ( col = 0; col < cols + 2; ++col )
	{
	  thisrerr[col] = rand( ) % ( FS_SCALE * 2 ) - FS_SCALE;
	  thisgerr[col] = rand( ) % ( FS_SCALE * 2 ) - FS_SCALE;
	  thisberr[col] = rand( ) % ( FS_SCALE * 2 ) - FS_SCALE;
	  /* (random errors in [-1 .. 1], scaled) */
	}
      fs_direction = 1;
    }
  else
    {
      make_square( 255.0/31.0, divN, modN, magic);
    }


  for ( row = 0; row < rows; ++row )
    {
      if ( floyd )
	for ( col = 0; col < cols + 2; ++col )
	  nextrerr[col] = nextgerr[col] = nextberr[col] = 0;
       if ( ( ! floyd ) || fs_direction )
	{
	  col = 0;
	  limitcol = cols;
	}
      else
	{
	  col = cols - 1;
	  limitcol = -1;
	}
      do
	{
	  curPixel = row*cols+col;
	  if (doColor)
	    {
	      r = raw->Red[curPixel];
	      g = raw->Green[curPixel];
	      b = raw->Blue[curPixel];
	    }
	  else
	    a = raw->Alpha[curPixel];

	  if ( floyd )
	    {
	      /* Use Floyd-Steinberg errors to adjust actual color. */
	      if (doColor)
		{
		  r += thisrerr[col + 1] / FS_SCALE;
		  g += thisgerr[col + 1] / FS_SCALE;
		  b += thisberr[col + 1] / FS_SCALE;
		  if ( r < 0 ) 
		    r = 0;
		  else if ( r > maxval )
		    r = maxval;
		  if ( g < 0 )
		    g = 0;
		  else if ( g > maxval )
		    g = maxval;
		  if ( b < 0 )
		    b = 0;
		  else if ( b > maxval )
		    b = maxval;
		  raw->Red[curPixel] = r & mask;
		  raw->Green[curPixel] = g & mask;
		  raw->Blue[curPixel] = b & mask;
		}
	      else
		{
		  a += thisrerr[col + 1] / FS_SCALE;
		  if ( a < 0 ) 
		    a = 0;
		  else if ( a > maxval )
		    a = maxval;
		  raw->Alpha[curPixel] = a & mask;
		}
	    }
	  else
	    {
	      if (doColor)
		{
		  raw->Red[curPixel] = (uint8)(DMAP(r, col%16, row%16)<<shiftUp);
		  raw->Green[curPixel] = (uint8)(DMAP(g, col%16, row%16)<<shiftUp);
		  raw->Blue[curPixel] = (uint8)(DMAP(b, col%16, row%16)<<shiftUp);
		}
	      else
		raw->Alpha[curPixel] = (uint8)(DMAP(a, col%16, row%16)<<shiftUp);
	    }
	  if ( floyd )
	    {
	      /* Propagate Floyd-Steinberg error terms.  We optimize by
	       ** multiplying each error by FS_SCALE/16 instead of dividing
	       ** each sub-error by 16.
	       */
	      if ( fs_direction )
		{
		  if (doColor)
		    {
		      err = ( r - raw->Red[curPixel] ) * (FS_SCALE/16);
		      thisrerr[col + 2] += err * 7;
		      nextrerr[col    ] += err * 3;
		      nextrerr[col + 1] += err * 5;
		      nextrerr[col + 2] += err;
		      err = ( g - raw->Green[curPixel] ) * (FS_SCALE/16);
		      thisgerr[col + 2] += err * 7;
		      nextgerr[col    ] += err * 3;
		      nextgerr[col + 1] += err * 5;
		      nextgerr[col + 2] += err;
		      err = ( b - raw->Blue[curPixel] ) * (FS_SCALE/16);
		      thisberr[col + 2] += err * 7;
		      nextberr[col    ] += err * 3;
		      nextberr[col + 1] += err * 5;
		      nextberr[col + 2] += err;
		    }
		  else
		    {
		      err = ( a - raw->Alpha[curPixel] ) * (FS_SCALE/16);
		      thisrerr[col + 2] += err * 7;
		      nextrerr[col    ] += err * 3;
		      nextrerr[col + 1] += err * 5;
		      nextrerr[col + 2] += err;
		    }
		}
	      else
		{
		  if (doColor)
		    {
		      err = ( r - raw->Red[curPixel] ) * (FS_SCALE/16);
		      thisrerr[col    ] += err * 7;
		      nextrerr[col + 2] += err * 3;
		      nextrerr[col + 1] += err * 5;
		      nextrerr[col    ] += err;
		      err = ( g - raw->Green[curPixel] ) * (FS_SCALE/16);
		      thisgerr[col    ] += err * 7;
		      nextgerr[col + 2] += err * 3;
		      nextgerr[col + 1] += err * 5;
		      nextgerr[col    ] += err;
		      err = ( b - raw->Blue[curPixel] ) * (FS_SCALE/16);
		      thisberr[col    ] += err * 7;
		      nextberr[col + 2] += err * 3;
		      nextberr[col + 1] += err * 5;
		      nextberr[col    ] += err;
		    }
		  else
		    {
		      err = ( a - raw->Alpha[curPixel] ) * (FS_SCALE/16);
		      thisrerr[col    ] += err * 7;
		      nextrerr[col + 2] += err * 3;
		      nextrerr[col + 1] += err * 5;
		      nextrerr[col    ] += err;
		    }
		}
	    }
	  
	  if ( ( ! floyd ) || fs_direction )
	    {
	      ++col;
	    }
	  else
	    {
	      --col;
	    }
	}
      while ( col != limitcol );
      
      
      if ( floyd )
	{
	  register long *temperr;
	  
	  temperr = thisrerr;
	  thisrerr = nextrerr;
	  nextrerr = temperr;
	  
	  temperr = thisgerr;
	  thisgerr = nextgerr;
	  nextgerr = temperr;
	  
	  temperr = thisberr;
	  thisberr = nextberr;
	  nextberr = temperr;
	  
	  fs_direction = ! fs_direction;
	}      
    }
  return (M2E_NoErr);
}
