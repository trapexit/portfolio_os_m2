/*
	File:		M2TXQuant.c

	Contains:	M2 Texture Library, Color quantization, remapping,
	                and dithering.

	Written by:	Todd Allendorf 

	Copyright:	© 1996 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		<10>	12/17/95	TMA		Added support to clear color re-mapping tables         in
									find_best_color.
		 <8>	 7/15/95	TMA		Update autodocs. Properly set IndexOffset and NumColors field of
									M2TXPIP.
		 <6>	 7/11/95	TMA		Removed unused variables.
		 <4>	 7/11/95	TMA		Fixed initialization error which caused random failure of
									quantizing.
		 <3>	  6/9/95	TMA		Make structures fit in 32K.
		 <2>	 5/30/95	TMA		Removed a printf.
		 <1>	 5/30/95	TMA		first checked in
	To Do:
*/


#include <stdlib.h>
#include <time.h>
#include "M2TXTypes.h"
#include "M2TXLibrary.h"
#include "M2TXQuant.h"
#include <stdio.h>
#include "qmem.h"

#define compInd(r,g,b) (((r)<<10)+((r)<<6)+(r)+((g)<<5)+(g)+(b))  

/**********************************************************************
	    C Implementation of Wu's Color Quantizer (v. 2)
	    (see Graphics Gems vol. II, pp. 126-133)

Author:	Xiaolin Wu
	Dept. of Computer Science
	Univ. of Western Ontario
	London, Ontario N6A 5B7
	wu@csd.uwo.ca

Algorithm: Greedy orthogonal bipartition of RGB space for variance
	   minimization aided by inclusion-exclusion tricks.
	   For speed no nearest neighbor search is done. Slightly
	   better performance can be expected by more sophisticated
	   but more expensive versions.

The author thanks Tom Lane at Tom_Lane@G.GP.CS.CMU.EDU for much of
additional documentation and a cure to a previous bug.

Free to distribute, comments and suggestions are appreciated.
**********************************************************************/	

#define MAXCOLOR	256
#define	RED	2
#define	GREEN	1
#define BLUE	0

struct box {
    int r0;			 /* min value, exclusive */
    int r1;			 /* max value, inclusive */
    int g0;  
    int g1;  
    int b0;  
    int b1;
    int vol;
};

/* Types need for colormapping */

typedef uint16 pixval;
typedef unsigned long pixel;
#define PPM_GETR(p) (((p) & 0x3ff00000) >> 20) 
#define PPM_GETG(p) (((p) & 0x000ffc00) >> 10 ) 
#define PPM_GETB(p) ((p) & 0x0000003ff) 
#define PPM_ASSIGN(p,red,grn,blu) (p) = ((pixel) (red) << 20) |((pixel) (grn) << 10) | (pixel) (blu)
#define PPM_EQUAL(p,q) ((p) == (q))

struct colorhist_item
{
  pixel color;
  int value;
};

typedef struct colorhist_item* colorhist_vector;

/* Magic Square for ordered dithering from p. 509 GG II */

static int magic4x4[4][4] = 
{
  0, 14, 3, 13,
  11, 5, 8, 6,
  12, 2, 15, 1,
  7, 9, 4, 10
};

/* Create a 16x16 Magic Square from the outer product of the 4x4 Magic Square */
/* p. 511 GG II */
/* For 256, N should 256/6 = 42.5.  For 64, N = 256/4 = 64, For 16 N=256/2 = 128 */
static void make_square( double N, int divN[256], int modN[256], int magic[16][16])
{
  register int i, j, k, l;
  double magicfact;
  
  for (i=0; i<256; i++)
    {
      divN[i] = (int)(i/N);
      modN[i] = (int)(N*divN[i]);
    }

  modN[255] = 0;     /* always */

  /* magicfact is (N-1)/16 so we get numbers in the matrix from 0 to N-1 */
  magicfact = (N-1) /16;
  for (i=0; i<4; i++)
    for (j=0; j<4; j++)
      for (k=0; k<4; k++)
	for (l=0; l<4; l++)
	  magic[4*k+i][4*l+j] =
	    (int)(0.5 + magic4x4[i][j] * magicfact +
		  (magic4x4[k][l]/16.0)*magicfact);
}

#define DMAP (v,x,y) (modN[v]>magic[x][y] ? divN[b] + 1: divN[v])

static int red_compare( colorhist_vector  ch1, colorhist_vector ch2 )
{
  return (int) PPM_GETR( ch1->color ) - (int) PPM_GETR( ch2->color );
}

static int green_compare( colorhist_vector  ch1, colorhist_vector ch2 )
{
  return (int) PPM_GETG( ch1->color ) - (int) PPM_GETG( ch2->color );
}

static int blue_compare( colorhist_vector  ch1, colorhist_vector ch2 )
{
  return (int) PPM_GETB( ch1->color ) - (int) PPM_GETB( ch2->color );
}


/* Convert a colormap into a 3-D sparse array of pixel*, indexed
** by [r][g][b].
*/
static M2Err add_pixel_to_array( pixel**** color_array, int r, int g, int b,
				pixel* newpP, pixval maxval )
{
  register int i;
  register pixel*** redvec;
  register pixel** greenvec;
  register pixel* pP;

  /* Fill in the red entry, if missing. */
  redvec = color_array[r];
  if ( redvec == NULL )
    {
      /* Check red neighbors to see if we can clone them. */
      if ( r-1 >= 0 && color_array[r-1] != NULL &&
	  ( color_array[r-1][g] == NULL ||
	   color_array[r-1][g][b] == NULL ||
	   PPM_EQUAL( *color_array[r-1][g][b], *newpP ) ) )
	redvec = color_array[r] = color_array[r-1];
      else if ( r+1 <= (int) maxval && color_array[r+1] != NULL &&
	       ( color_array[r+1][g] == NULL ||
		color_array[r+1][g][b] == NULL ||
		PPM_EQUAL( *color_array[r+1][g][b], *newpP ) ) )
	redvec = color_array[r] = color_array[r+1];
      else if ( r-2 >= 0 && color_array[r-2] != NULL &&
	       ( color_array[r-2][g] == NULL ||
		color_array[r-2][g][b] == NULL ||
		PPM_EQUAL( *color_array[r-2][g][b], *newpP ) ) )
	redvec = color_array[r] = color_array[r-2];
      else if ( r+2 <= (int) maxval && color_array[r+2] != NULL &&
	       ( color_array[r+2][g] == NULL ||
		color_array[r+2][g][b] == NULL ||
		PPM_EQUAL( *color_array[r+2][g][b], *newpP ) ) )
	redvec = color_array[r] = color_array[r+2];
      else if ( r-3 >= 0 && color_array[r-3] != NULL &&
	       ( color_array[r-3][g] == NULL ||
		color_array[r-3][g][b] == NULL ||
		PPM_EQUAL( *color_array[r-3][g][b], *newpP ) ) )
	redvec = color_array[r] = color_array[r-3];
      else if ( r+3 <= (int) maxval && color_array[r+3] != NULL &&
	       ( color_array[r+3][g] == NULL ||
		color_array[r+3][g][b] == NULL ||
		PPM_EQUAL( *color_array[r+3][g][b], *newpP ) ) )
	redvec = color_array[r] = color_array[r+3];
      else
	{
	  /* Nope, make a new red entry. */
	  redvec = color_array[r] =
	    (pixel***) qMemNewPtr(((int) maxval + 1)*sizeof(pixel**) );
	  if ( redvec == NULL )
	    {
	      return(M2E_NoMem);
	    }
	  for ( i = 0; i <= maxval; ++i )
	    redvec[i] = NULL;
	}
    }
  
  /* Fill in the green entry, if missing. */
  greenvec = redvec[g];
  if ( greenvec == NULL )
    {
      /* Check green neighbors to see if we can clone them. */
      if ( g-1 >= 0 && redvec[g-1] != NULL &&
	  ( redvec[g-1][b] == NULL ||
	   PPM_EQUAL( *redvec[g-1][b], *newpP ) ) )
	greenvec = redvec[g] = redvec[g-1];
      else if ( g+1 <= (int) maxval && redvec[g+1] != NULL &&
	       ( redvec[g+1][b] == NULL ||
		PPM_EQUAL( *redvec[g+1][b], *newpP ) ) )
	greenvec = redvec[g] = redvec[g+1];
      else if ( g-2 >= 0 && redvec[g-2] != NULL &&
	       ( redvec[g-2][b] == NULL ||
		PPM_EQUAL( *redvec[g-2][b], *newpP ) ) )
	greenvec = redvec[g] = redvec[g-2];
      else if ( g+2 <= (int) maxval && redvec[g+2] != NULL &&
	       ( redvec[g+2][b] == NULL ||
		PPM_EQUAL( *redvec[g+2][b], *newpP ) ) )
	greenvec = redvec[g] = redvec[g+2];
      else if ( g-3 >= 0 && redvec[g-3] != (pixel**) 0 &&
	       ( redvec[g-3][b] == NULL ||
		PPM_EQUAL( *redvec[g-3][b], *newpP ) ) )
	greenvec = redvec[g] = redvec[g-3];
      else if ( g+3 <= (int) maxval && redvec[g+3] != NULL &&
	       ( redvec[g+3][b] == NULL ||
		PPM_EQUAL( *redvec[g+3][b], *newpP ) ) )
	greenvec = redvec[g] = redvec[g+3];
      else
	{
	  /* Nope, make a new green entry. */
	  greenvec = redvec[g] = 
	    (pixel**) qMemNewPtr(((int) maxval + 1)* sizeof(pixel*) );
	  if ( greenvec == NULL )
	    {
	      return(M2E_NoMem);
	    }
	  for ( i = 0; i <= maxval; ++i )
	    greenvec[i] = NULL;
	}
    }
  
  /* Fill in the blue entry. */
  pP = greenvec[b];
  if ( pP == NULL )
    {
      /* Check blue neighbors to see if we can clone them. */
      if ( b-1 >= 0 && greenvec[b-1] != NULL &&
	  PPM_EQUAL( *greenvec[b-1], *newpP ) )
	greenvec[b] = greenvec[b-1];
      else if ( b+1 <= (int) maxval && greenvec[b+1] != NULL &&
	       PPM_EQUAL( *greenvec[b+1], *newpP ) )
	greenvec[b] = greenvec[b+1];
      else if ( b-2 >= 0 && greenvec[b-2] != NULL &&
	       PPM_EQUAL( *greenvec[b-2], *newpP ) )
	greenvec[b] = greenvec[b-2];
      else if ( b+2 <= (int) maxval && greenvec[b+2] != NULL &&
	       PPM_EQUAL( *greenvec[b+2], *newpP ) )
	greenvec[b] = greenvec[b+2];
      else if ( b-3 >= 0 && greenvec[b-3] != NULL &&
	       PPM_EQUAL( *greenvec[b-3], *newpP ) )
	greenvec[b] = greenvec[b-3];
      else if ( b+3 <= (int) maxval && greenvec[b+3] != NULL &&
	       PPM_EQUAL( *greenvec[b+3], *newpP ) )
	greenvec[b] = greenvec[b+3];
      else
	{
	  /* Nope, make a new blue entry. */
	  pP = greenvec[b] = (pixel*) qMemNewPtr( sizeof(pixel) );
	  if ( pP == NULL )
	    {
	      return(M2E_NoMem);
	    }
	  *pP = *newpP;
	}
    }
  else
    {
      if ( ! (PPM_EQUAL( *pP, *newpP ) ) )
	{
	  /*
		fprintf(stderr,"colliding colormap entries?!?\n R=%d G=%d B=%d R=%d G=%d B=%d\n",
		PPM_GETR(*pP), PPM_GETG(*pP), PPM_GETB(*pP), 
		PPM_GETR(*newpP), PPM_GETG(*newpP), PPM_GETB(*newpP));
		fprintf(stderr,"Original R=%d G=%d B=%d\n", r, g, b);
		*/
	  ;
	}
    }
  
  return (M2E_NoErr);
}

static long* fbc_squares;
static long* fbc_squares_o;
static int* fbc_rs;
static int* fbc_gs;
static int* fbc_bs;
  
/* Nice color-searching routine by James H. Bruner. */
static int find_best_color( pixel* pP, pixval maxval, colorhist_vector chv, 
			   int newColors, bool *resetStatic)
{
  register int i;
  register int gdist;
  register int r, g, b;
  register int gidx, low, mid, high;
  long dist, newdist;
  int idx;

  if ((*resetStatic)== TRUE)
    {
      fbc_squares = (long *)0;
      *resetStatic = FALSE;
    }
  if ( fbc_squares == (long*) 0 )
    {
      /* Initialize.  Cache squares of [-maxval..maxval]. */
      fbc_squares = (long*) qMemNewPtr( (2 * (int) maxval + 1)* sizeof(long) );
      fbc_squares_o = fbc_squares + maxval;
      for ( i = -((int) maxval); i <= (int) maxval; ++i )
	fbc_squares_o[i] = (long) i * (long) i;
      
      /* Sort by the green component.  This is just a guess but since
       ** green contributes most to luminosity, I think that there might be
       ** better distribution across green.  It really doesn't matter which
       ** color though.
       */
      qsort(
	    (char*) chv, newColors, sizeof(struct colorhist_item),
	    (int (*)(const void*, const void*))green_compare );
      
      /* Cache colormap values. */
      fbc_rs = (int*) qMemNewPtr( newColors* sizeof(int) );
      fbc_gs = (int*) qMemNewPtr( newColors* sizeof(int) );
      fbc_bs = (int*) qMemNewPtr( newColors* sizeof(int) );
      for ( i = 0; i < newColors; ++i )
	{
	  fbc_rs[i] = PPM_GETR( chv[i].color );
	  fbc_gs[i] = PPM_GETG( chv[i].color );
	  fbc_bs[i] = PPM_GETB( chv[i].color );
	}
    }
  
  r = PPM_GETR( *pP );
  g = PPM_GETG( *pP );
  b = PPM_GETB( *pP );
  
  /* The colormap is sorted by the green component.  Do binary search. */
  low = 0;
  high = newColors - 1;
  while ( low < high )
    {
      mid = ( high + low ) / 2;
      gdist = fbc_gs[mid];
      if ( gdist == g )
	high = low = mid;
      else
	if ( gdist < g )
	  low = mid + 1;
	else
	  high = mid - 1;
    }
    gidx = high;
  if ( gidx < 0 )
    gidx = 0;

  /* Could be <0 if value less than 1st.  Can't be
   ** greater than newColors because division for mid
   ** rounds down.
   */
  
  /* The color map is sorted by the green component.  We've searched and now
   ** have the index at the closest green match.  This means, that we can now
   ** search up and down from this location and only have to search until
   ** the green component alone is worse than the best total color match.
   ** Because it is sorted by green, we know it will only get worse!
   */
  
  /* First search down. */
  dist = 2000000000;
  for ( i = gidx; i >= 0; --i )
    {
      gdist = fbc_squares_o[g - fbc_gs[i]];
      if ( gdist >= dist )
	break;
      newdist = gdist + fbc_squares_o[r - fbc_rs[i]] + fbc_squares_o[b - fbc_bs[i]];
      if ( newdist < dist )
	{
	  dist = newdist;
	  idx = i;
	}
    }
  
  /* Now search up. */
  for ( i = gidx + 1; i < newColors; ++i )
    {
	gdist = fbc_squares_o[g - fbc_gs[i]];
	if ( gdist >= dist )
	  break;
	newdist = gdist + fbc_squares_o[r - fbc_rs[i]] + 
	  fbc_squares_o[b - fbc_bs[i]];
	if ( newdist < dist )
	  {
	    dist = newdist;
	    idx = i;
	  }
      }
  
  return idx;
}

/* Write out the file using only the new colormap. */
#define FS_SCALE 1024

static M2Err map_colors(M2TXRaw *raw, pixval maxval, pixel**** color_array, 
		       colorhist_vector chv, int newColors, bool floyd )
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
  pixel p;
  register pixel *pP;
  long r, g, b, err;
  pixel*** redvec;
  pixel** greenvec;
  pixel* newpP;
  int i;
  uint32 curPixel;
  M2Err myErr;

  bool resetStatic = TRUE;


  pP = &p;
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
	  r = raw->Red[curPixel];
	  g = raw->Green[curPixel];
	  b = raw->Blue[curPixel];
	  PPM_ASSIGN(p, r, g, b);
	  if ( floyd )
	    {
	      /* Use Floyd-Steinberg errors to adjust actual color. */
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
	      raw->Red[curPixel] = r;
	      raw->Green[curPixel] = g;
	      raw->Blue[curPixel] = b;
	      PPM_ASSIGN(p, r, g, b);
	    }
	  /* Check closest-color sparse array. */
	  if ( (( redvec = color_array[r] ) != NULL) &&
	      ( ( greenvec = redvec[g] ) != NULL) &&
	      ( ( newpP = greenvec[b] ) != NULL) )
	    {
	      /* Change *pP to the closest color in the new colormap. */
	      p = *newpP;
	      raw->Red[curPixel] = PPM_GETR(p);
	      raw->Green[curPixel] = PPM_GETG(p);
	      raw->Blue[curPixel] = PPM_GETB(p);
	    }
	  else
	    {
	      /* Got to add a new entry. */
	      i = find_best_color( &p, maxval, chv, newColors, &resetStatic );
	      myErr = add_pixel_to_array( color_array, r, g, b, &(chv[i].color),
					 maxval );
	      if (myErr!=M2E_NoErr)
		{
		  fprintf(stderr,"In MapColors, add_pixel.. err=%d\n",myErr);
		  if (myErr == M2E_Range)
		    fprintf(stderr,"redvec=%x greenvec=%x newpP=%x\n", redvec,
			    greenvec, newpP);
		  else
		    return(myErr);
		}
	      
	      /* And change *pP to new closest color. */
	      p = chv[i].color;
	      raw->Red[curPixel] = PPM_GETR(p);
	      raw->Green[curPixel] = PPM_GETG(p);
	      raw->Blue[curPixel] = PPM_GETB(p);
	    }
	  
	  if ( floyd )
	    {
	      /* Propagate Floyd-Steinberg error terms.  We optimize by
	       ** multiplying each error by FS_SCALE/16 instead of dividing
	       ** each sub-error by 16.
	       */
	      if ( fs_direction )
		{
		  err = ( r - (long) PPM_GETR( p ) ) * (FS_SCALE/16);
		  thisrerr[col + 2] += err * 7;
		  nextrerr[col    ] += err * 3;
		  nextrerr[col + 1] += err * 5;
		  nextrerr[col + 2] += err;
		  err = ( g - (long) PPM_GETG( p ) ) * (FS_SCALE/16);
		  thisgerr[col + 2] += err * 7;
		  nextgerr[col    ] += err * 3;
		  nextgerr[col + 1] += err * 5;
		  nextgerr[col + 2] += err;
		  err = ( b - (long) PPM_GETB( p ) ) * (FS_SCALE/16);
		  thisberr[col + 2] += err * 7;
		  nextberr[col    ] += err * 3;
		  nextberr[col + 1] += err * 5;
		  nextberr[col + 2] += err;
		}
	      else
		{
		  err = ( r - (long) PPM_GETR( p ) ) * (FS_SCALE/16);
		  thisrerr[col    ] += err * 7;
		  nextrerr[col + 2] += err * 3;
		  nextrerr[col + 1] += err * 5;
		  nextrerr[col    ] += err;
		  err = ( g - (long) PPM_GETG( p ) ) * (FS_SCALE/16);
		  thisgerr[col    ] += err * 7;
		  nextgerr[col + 2] += err * 3;
		  nextgerr[col + 1] += err * 5;
		  nextgerr[col    ] += err;
		  err = ( b - (long) PPM_GETB( p ) ) * (FS_SCALE/16);
		  thisberr[col    ] += err * 7;
		  nextberr[col + 2] += err * 3;
		  nextberr[col + 1] += err * 5;
		  nextberr[col    ] += err;
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
  if (floyd)
    {
      qMemReleasePtr(thisrerr);
      qMemReleasePtr(nextrerr);
      qMemReleasePtr(thisgerr);
      qMemReleasePtr(nextgerr);
      qMemReleasePtr(thisberr);
      qMemReleasePtr(nextberr);
#if 0 
      qMemReleasePtr(fbc_squares);
      qMemReleasePtr(fbc_squares_o);
      qMemReleasePtr(fbc_rs);
      qMemReleasePtr(fbc_gs);
      qMemReleasePtr(fbc_bs);
#endif
    }
  return (M2E_NoErr);
}

/*  Making Color Hist */
/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXRaw_MapToPIP
|||	Map an existing image to a given PIP and optionally dither it.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXRaw_MapToPIP(M2TXRaw *raw, M2TXPIP *pip, int newColors,
|||	                           bool floyd)
|||	
|||	  Description
|||	
|||	    This function takes an M2TXRaw image and remaps the image using only
|||	    the specified number of colors in the M2TXPIP supplied.  The image may
|||	    also be Floyd-Steinberg dithered during this remapping.
|||	
|||	  Arguments
|||	    
|||	    raw
|||	        The input M2TXRaw image.
|||	    pip
|||	        The output PIP containing the best fit palette.
|||	    newColors
|||	        The number of colors in the PIP (that should be used).
|||	    floyd
|||	        Whether the raw image should be Floyd-Steinberg dithered.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXQuant.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXRaw_MakePIP()
|||	
**/

static bool mtp_preserve = FALSE;
void M2TXRaw_MapPreserve()
{
  mtp_preserve = TRUE;
}

static  pixel ****color_array = NULL;
static  colorhist_vector chv = NULL;

M2Err M2TXRaw_MapToPIP(M2TXRaw *raw, M2TXPIP *pip, int newColors, bool floyd)
{
  M2TXColor color;
  uint8 red, grn, blu, ssb, alpha;
  int i, j, k, l;
  register pixel *pP;
  pixel ***redvec;
  pixel **greenvec;
  pixel p;
  M2Err err;
  pixval maxval = 255;
  
  if (chv == NULL)
    {
      chv = (colorhist_vector)qMemNewPtr(newColors*sizeof(struct colorhist_item));
      if (chv == NULL)
	return (M2E_NoMem);
      for (i=0; i<newColors; i++)
	{
	  M2TXPIP_GetColor(pip, i, &color);
	  M2TXColor_Decode(color, &ssb, &alpha, &red, &grn, &blu);
	  if (raw->HasColor)
	    PPM_ASSIGN(p, red, grn, blu);
	  else 
	    PPM_ASSIGN(p, alpha, alpha, alpha);
	  chv[i].color = p;
	}
      /* Initialize sparse array for color mapping. */
      if (color_array == NULL)
	{
	  color_array = (pixel****) qMemNewPtr(((int) maxval+1)*sizeof(pixel***));
	  
	  if (color_array == NULL)
	    {
	      qMemReleasePtr(chv);
	      return (M2E_NoMem);
	    }
	  for ( i = 0; i <= maxval; ++i )
	    color_array[i] = NULL;
	}
      /* Convert the colormap into a 3-D sparse array of pixel*, indexed
      ** by [r][g][b].
      */
      
      for ( i = 0; i < newColors; ++i )
	{
	  pP = &(chv[i].color);
	  err = add_pixel_to_array( color_array, PPM_GETR( *pP ), PPM_GETG( *pP ), 
				    PPM_GETB( *pP ), pP, maxval );
	  if (err == M2E_NoMem)
	    { 
	      qMemReleasePtr(chv);
	      qMemReleasePtr(color_array);
	      /*  fprintf (stderr,"Remap error = %d\n",err); */
	      return (err);
	    }
	}
    }

  /* Map to the new colors and write the file. */
  err = map_colors( raw, maxval, color_array, chv, newColors, floyd );

  if (!mtp_preserve)
    {
      /*      fprintf(stderr,"Oops! I'm freeing up these guys\n"); */
      qMemReleasePtr(chv);
      qMemReleasePtr(color_array);
      color_array = NULL;
      chv = NULL;
    }
  return(err);  
}  

/* build 3-D color histogram of counts, r/g/b, c^2 */
M2Err M2TXRaw_Hist3d(M2TXRaw *raw, colorspace *cs, uint16 **qadd, 
			    float *m2) 
{
  register int ind, r, g, b;
  int	     inr, ing, inb, table[256];
  register long int i;
  uint32 size;
  uint32  *vwt, *vmr, *vmb, *vmg; 
  uint16 *Qadd;

  for(i=0; i<256; ++i) 
    table[i]=i*i;

  vwt = (uint32 *)cs->wt;
  vmr = (uint32 *)cs->mr;
  vmg = (uint32 *)cs->mg;
  vmb = (uint32 *)cs->mb;

  size = raw->XSize * raw->YSize;
  *qadd = Qadd = (uint16 *)qMemNewPtr(sizeof(short int)*size);
  if (Qadd==NULL)
    {
      return (M2E_NoMem);
    }
  for(i=0; i<size; ++i)
    {
      r = raw->Red[i]; g = raw->Green[i]; b = raw->Blue[i];
      inr=(r>>3)+1; 
      ing=(g>>3)+1; 
      inb=(b>>3)+1; 
      Qadd[i]=ind=(inr<<10)+(inr<<6)+inr+(ing<<5)+ing+inb;
      /*[inr][ing][inb]*/
      ++vwt[ind];
      vmr[ind] += r;
      vmg[ind] += g;
      vmb[ind] += b;
      m2[ind] += (float)(table[r]+table[g]+table[b]);
    }
  return (M2E_NoErr);
}

/* At conclusion of the histogram step, we can interpret
 *   wt[r][g][b] = sum over voxel of P(c)
 *   mr[r][g][b] = sum over voxel of r*P(c)  ,  similarly for mg, mb
 *   m2[r][g][b] = sum over voxel of c^2*P(c)
 * Actually each of these should be divided by 'size' to give the usual
 * interpretation of P() as ranging from 0 to 1, but we needn't do that here.
 */

/* We now convert histogram into moments so that we can rapidly calculate
 * the sums of the above quantities over any desired box.
 */

 /* compute cumulative moments. */
static void M3d(colorspace *cs, float *m2) 
{
  register unsigned short int ind1, ind2;
  register unsigned char i, r, g, b;
  long int line, line_r, line_g, line_b,
  area[33], area_r[33], area_g[33], area_b[33];
  float    line2, area2[33];
  uint32  *vwt, *vmr, *vmb, *vmg; 
  
  vwt = (uint32 *)cs->wt;
  vmr = (uint32 *)cs->mr;
  vmg = (uint32 *)cs->mg;
  vmb = (uint32 *)cs->mb;
  
  for(r=1; r<=32; ++r){
    for(i=0; i<=32; ++i) 
      area2[i]=area[i]=area_r[i]=area_g[i]=area_b[i]=0;
    for(g=1; g<=32; ++g){
      line2 = line = line_r = line_g = line_b = 0;
      for(b=1; b<=32; ++b){
	ind1 = (r<<10) + (r<<6) + r + (g<<5) + g + b; /* [r][g][b] */
	line += vwt[ind1];
	line_r += vmr[ind1]; 
	line_g += vmg[ind1]; 
	line_b += vmb[ind1];
	line2 += m2[ind1];
	area[b] += line;
	area_r[b] += line_r;
	area_g[b] += line_g;
	area_b[b] += line_b;
	area2[b] += line2;
	ind2 = ind1 - 1089; /* [r-1][g][b] */
	vwt[ind1] = vwt[ind2] + area[b];
	vmr[ind1] = vmr[ind2] + area_r[b];
	vmg[ind1] = vmg[ind2] + area_g[b];
	vmb[ind1] = vmb[ind2] + area_b[b];
	m2[ind1] = m2[ind2] + area2[b];
      }
    }
  }
}


/* Compute sum over a box of any given statistic */
static int32 Vol(struct box *cube, uint32 *mmt) 
{

    return( mmt[compInd(cube->r1,cube->g1,cube->b1)] 
	   -mmt[compInd(cube->r1,cube->g1,cube->b0)]
	   -mmt[compInd(cube->r1,cube->g0,cube->b1)]
	   +mmt[compInd(cube->r1,cube->g0,cube->b0)]
	   -mmt[compInd(cube->r0,cube->g1,cube->b1)]
	   +mmt[compInd(cube->r0,cube->g1,cube->b0)]
	   +mmt[compInd(cube->r0,cube->g0,cube->b1)]
	   -mmt[compInd(cube->r0,cube->g0,cube->b0)] );
}

/* The next two routines allow a slightly more efficient calculation
 * of Vol() for a proposed subbox of a given box.  The sum of Top()
 * and Bottom() is the Vol() of a subbox split in the given direction
 * and with the specified new upper bound.
 */

/* Compute part of Vol(cube, mmt) that doesn't depend on r1, g1, or b1 */
/* (depending on dir) */
static int32 Bottom(struct box *cube, uint8 dir, uint32 *mmt)
{
    switch(dir){
	case RED:
	    return( -mmt[compInd(cube->r0,cube->g1,cube->b1)]
		    +mmt[compInd(cube->r0,cube->g1,cube->b0)]
		    +mmt[compInd(cube->r0,cube->g0,cube->b1)]
		    -mmt[compInd(cube->r0,cube->g0,cube->b0)] );
	    break;
	case GREEN:
	    return( -mmt[compInd(cube->r1,cube->g0,cube->b1)]
		    +mmt[compInd(cube->r1,cube->g0,cube->b0)]
		    +mmt[compInd(cube->r0,cube->g0,cube->b1)]
		    -mmt[compInd(cube->r0,cube->g0,cube->b0)] );
	    break;
	case BLUE:
 default:
	    return( -mmt[compInd(cube->r1,cube->g1,cube->b0)]
		    +mmt[compInd(cube->r1,cube->g0,cube->b0)]
		    +mmt[compInd(cube->r0,cube->g1,cube->b0)]
		    -mmt[compInd(cube->r0,cube->g0,cube->b0)] );
	    break;
    }
}

/* Compute remainder of Vol(cube, mmt), substituting pos for */
/* r1, g1, or b1 (depending on dir) */
static int32 Top(struct box *cube, uint8 dir, int pos, uint32 *mmt)
{
    switch(dir){
	case RED:
	    return( mmt[compInd(pos,cube->g1,cube->b1)] 
		   -mmt[compInd(pos,cube->g1,cube->b0)]
		   -mmt[compInd(pos,cube->g0,cube->b1)]
		   +mmt[compInd(pos,cube->g0,cube->b0)] );
	    break;
	case GREEN:
	    return( mmt[compInd(cube->r1,pos,cube->b1)] 
		   -mmt[compInd(cube->r1,pos,cube->b0)]
		   -mmt[compInd(cube->r0,pos,cube->b1)]
		   +mmt[compInd(cube->r0,pos,cube->b0)] );
	    break;
	case BLUE:
		default:
	    return( mmt[compInd(cube->r1,cube->g1,pos)]
		   -mmt[compInd(cube->r1,cube->g0,pos)]
		   -mmt[compInd(cube->r0,cube->g1,pos)]
		   +mmt[compInd(cube->r0,cube->g0,pos)] );
	    break;
    }
}


/* Compute the weighted variance of a box */
/* NB: as with the raw statistics, this is really the variance * size */
static float Var(struct box *cube, colorspace *cs, float *m2)
{
  float dr, dg, db, xx;

  dr = Vol(cube, (uint32 *)cs->mr); 
  dg = Vol(cube, (uint32 *)cs->mg); 
  db = Vol(cube, (uint32 *)cs->mb);
  xx = (m2[compInd(cube->r1,cube->g1,cube->b1)]) 
    -(m2[compInd(cube->r1,cube->g1,cube->b0)])
      -(m2[compInd(cube->r1,cube->g0,cube->b1)])
	+(m2[compInd(cube->r1,cube->g0,cube->b0)])
	  -(m2[compInd(cube->r0,cube->g1,cube->b1)])
	    +(m2[compInd(cube->r0,cube->g1,cube->b0)])
	      +(m2[compInd(cube->r0,cube->g0,cube->b1)])
		-(m2[compInd(cube->r0,cube->g0,cube->b0)]);
  
  return( xx - (dr*dr+dg*dg+db*db)/(float)Vol(cube,  (uint32 *)(cs->wt)) );    
}

/* We want to minimize the sum of the variances of two subboxes.
 * The sum(c^2) terms can be ignored since their sum over both subboxes
 * is the same (the sum for the whole box) no matter where we split.
 * The remaining terms have a minus sign in the variance formula,
 * so we drop the minus sign and MAXIMIZE the sum of the two terms.
 */


static float Maximize(struct box *cube, uint8 dir, int16 first, int16 last, 
		      int16 *cut, uint32 whole_r, uint32 whole_g, uint32 whole_b, 
		      uint32 whole_w, colorspace *cs)
{
register int32 half_r, half_g, half_b, half_w;
int32 base_r, base_g, base_b, base_w;
register int16 i;
register float temp, max;

    base_r = Bottom(cube, dir, (uint32 *)cs->mr);
    base_g = Bottom(cube, dir, (uint32 *)cs->mg);
    base_b = Bottom(cube, dir, (uint32 *)cs->mb);
    base_w = Bottom(cube, dir, (uint32 *)cs->wt);
    max = 0.0;
    *cut = -1;
    for(i=first; i<last; ++i){
	half_r = base_r + Top(cube, dir, i, (uint32 *)cs->mr);
	half_g = base_g + Top(cube, dir, i, (uint32 *)cs->mg);
	half_b = base_b + Top(cube, dir, i, (uint32 *)cs->mb);
	half_w = base_w + Top(cube, dir, i, (uint32 *)cs->wt);
        /* now half_x is sum over lower half of box, if split at i */
        if (half_w == 0) {      /* subbox could be empty of pixels! */
          continue;             /* never split into an empty box */
	} else
        temp = ((float)half_r*half_r + (float)half_g*half_g +
                (float)half_b*half_b)/half_w;

	half_r = whole_r - half_r;
	half_g = whole_g - half_g;
	half_b = whole_b - half_b;
	half_w = whole_w - half_w;
        if (half_w == 0) {      /* subbox could be empty of pixels! */
          continue;             /* never split into an empty box */
	} else
        temp += ((float)half_r*half_r + (float)half_g*half_g +
                 (float)half_b*half_b)/half_w;

    	if (temp > max) {max=temp; *cut=i;}
    }
    return(max);
}

static int16 Cut(struct box *set1, struct box *set2, colorspace *cs)
{
  unsigned char dir;
  int16 cutr, cutg, cutb;
  float maxr, maxg, maxb;
  int32 whole_r, whole_g, whole_b, whole_w;
  
  whole_r = Vol(set1,   (uint32 *)cs->mr);
  whole_g = Vol(set1,   (uint32 *)cs->mg);
  whole_b = Vol(set1,   (uint32 *)cs->mb);
  whole_w = Vol(set1,   (uint32 *)cs->wt);
  
  maxr = Maximize(set1, RED, set1->r0+1, set1->r1, &cutr,
		  whole_r, whole_g, whole_b, whole_w,cs);
  maxg = Maximize(set1, GREEN, set1->g0+1, set1->g1, &cutg,
		  whole_r, whole_g, whole_b, whole_w,cs);
  maxb = Maximize(set1, BLUE, set1->b0+1, set1->b1, &cutb,
		  whole_r, whole_g, whole_b, whole_w,cs);
  
  if( (maxr>=maxg)&&(maxr>=maxb) )
    {
      dir = RED;
      if (cutr < 0) return 0; /* can't split the box */
    }
  else
    if( (maxg>=maxr)&&(maxg>=maxb) ) 
      dir = GREEN;
    else
      dir = BLUE; 
  
  set2->r1 = set1->r1;
  set2->g1 = set1->g1;
  set2->b1 = set1->b1;
  
  switch (dir)
    {
    case RED:
      set2->r0 = set1->r1 = cutr;
      set2->g0 = set1->g0;
      set2->b0 = set1->b0;
      break;
    case GREEN:
      set2->g0 = set1->g1 = cutg;
      set2->r0 = set1->r0;
      set2->b0 = set1->b0;
      break;
    case BLUE:
      set2->b0 = set1->b1 = cutb;
      set2->r0 = set1->r0;
      set2->g0 = set1->g0;
      break;
    }
  set1->vol=(set1->r1-set1->r0)*(set1->g1-set1->g0)*(set1->b1-set1->b0);
  set2->vol=(set2->r1-set2->r0)*(set2->g1-set2->g0)*(set2->b1-set2->b0);
  return 1;
}


static void Mark(struct box *cube, int16 label, uint8 *tag)
{
register int r, g, b;

    for(r=cube->r0+1; r<=cube->r1; ++r)
       for(g=cube->g0+1; g<=cube->g1; ++g)
	  for(b=cube->b0+1; b<=cube->b1; ++b)
	    tag[compInd(r,g,b)] = label;
}


/**
|||	AUTODOC -public -class tools -group m2txlib -name M2TXRaw_MakePIP
|||	Construct a PIP for the raw image by quantization.
|||	
|||	  Synopsis
|||	
|||	    M2Err M2TXRaw_MakePIP(M2TXRaw *raw, M2TXPIP *pip, int *numColors, 
|||	                          bool remap)
|||	
|||	  Description
|||	
|||	    This function takes an M2TXRaw image and, using Wu's algorithm, finds
|||	    a representative color palette for the raw image.  Only colors are taken
|||	    into account, alpha and ssb values and not placed into the PIP.
|||	    If the remap flag is set to TRUE, the raw image will be remapped to the
|||	    new PIP, otherwise it will be left untouched.  The remapping does no
|||	    dithering, use M2TXRaw_MapToPIP if you wish to dither the image.
|||	    The NumColors field of the PIP will be set to the ACTUAL number of
|||	    colors in the PIP, which may be less than the requested number of colors.
|||	
|||	  Arguments
|||	    
|||	    raw
|||	        The input M2TXRaw image.
|||	    pip
|||	        The output PIP containing the best fit palette.
|||	    numColors
|||	        The maximum number of colors to find for the PIP and the number of
|||	        colors actually used for the PIP (if less than the requested #).
|||	    remap
|||	        Whether the raw image should be remapped to the PIP or not.
|||	
|||	  Return Value
|||	
|||	    It returns an error code if it fails.
|||	
|||	  Associated Files
|||	
|||	    <M2TXQuant.h>, M2TXlib.a
|||	
|||	  Implementation
|||	
|||	    Library call implemented in M2TXlib V10.
|||	
|||	  See Also
|||	
|||	    M2TXRaw_MapToPIP()
|||	
**/
M2Err M2TXRaw_MakePIP(M2TXRaw *raw, M2TXPIP *pip, int *numColors, bool remap)
{
  colorspace *cs;
  uint16 *Qadd;
  struct box	cube[MAXCOLOR];
  uint8 	*tag, red, green, blue;
  uint8 	lut_r[MAXCOLOR], lut_g[MAXCOLOR], lut_b[MAXCOLOR];
  int		next;
  register int32	i, weight;
  register int	k;
  float		vv[MAXCOLOR], temp;
  M2TXColor     color;
  uint32       size;
  static float        *m2;
  M2Err        err;
  int          findColors;

  findColors = *numColors;

  cs = (colorspace *)qMemNewPtr(sizeof(colorspace));
  cs->wt = (uint32 *)qMemNewPtr(33*33*33*sizeof(uint32));
  cs->mr = (uint32 *)qMemNewPtr(33*33*33*sizeof(uint32));
  cs->mg = (uint32 *)qMemNewPtr(33*33*33*sizeof(uint32));
  cs->mb = (uint32 *)qMemNewPtr(33*33*33*sizeof(uint32));
  m2 = (float *)qMemNewPtr(33*33*33*sizeof(float));

  if ((m2 == NULL) || ((cs->mb) == NULL))
    return (M2E_NoMem);

 for (i=0; i<(33*33*33); i++)
    {
      cs->wt[i] = 0;
      cs->mr[i] = 0;
      cs->mg[i] = 0;
      cs->mb[i] = 0;
      m2[i] = 0.0;
    }

  size = raw->XSize * raw->YSize;
  err = M2TXRaw_Hist3d(raw, cs, &Qadd, (float *)m2); 

  if (err != M2E_NoErr)
    {
      qMemReleasePtr(m2);
      qMemReleasePtr(cs->wt);
      qMemReleasePtr(cs->mr);
      qMemReleasePtr(cs->mg);
      qMemReleasePtr(cs->mb);
      qMemReleasePtr(cs);
      return (err);
    }

  M3d(cs, (float *)m2);
  
  cube[0].r0 = cube[0].g0 = cube[0].b0 = 0;
  cube[0].r1 = cube[0].g1 = cube[0].b1 = 32;
  next = 0;

  for(i=1; i<findColors; ++i)
    {
      if (Cut(&cube[next], &cube[i], cs)) 
	{
	  /* volume test ensures we won't try to cut one-cell box */
	  vv[next] = (cube[next].vol>1) ? Var(&cube[next], cs, (float *)m2) : 0.0;
	  vv[i] = (cube[i].vol>1) ? Var(&cube[i], cs, (float *)m2) : 0.0;
      } 
      else 
	{
	  vv[next] = 0.0;   /* don't try to split this box again */
	  i--;              /* didn't create box i */
	}
      next = 0; 
      temp = vv[0];
      for(k=1; k<=i; ++k)
	if (vv[k] > temp) 
	  {
	    temp = vv[k]; 
	    next = k;
	  }
      if (temp <= 0.0) 
	{
	  *numColors = i+1;
	  break;
	}
    }  
  /* the space for array m2 can be freed now */
  
  qMemReleasePtr(m2);

  M2TXPIP_SetNumColors(pip,*numColors);
  M2TXPIP_SetIndexOffset(pip,0);
  tag = (unsigned char *)qMemNewPtr(33*33*33);
  if (tag==NULL) 
  {
    qMemReleasePtr(Qadd);
    qMemReleasePtr(cs->wt);
    qMemReleasePtr(cs->mr);
    qMemReleasePtr(cs->mg);
    qMemReleasePtr(cs->mb);
    qMemReleasePtr(cs);
    return (M2E_NoMem);
  }
  for(k=0; k<*numColors; ++k)
    {
      Mark(&cube[k], k, tag);
      weight = Vol(&cube[k], (uint32 *)(cs->wt));
      if (weight) {
	lut_r[k] = red = Vol(&cube[k],  (uint32 *)(cs->mr)) / weight;
	lut_g[k] = green = Vol(&cube[k], (uint32 *)(cs->mg)) / weight;
	lut_b[k] = blue = Vol(&cube[k],  (uint32 *)(cs->mb)) / weight;
      }
      else
	{
	  /*  fprintf(stderr, "bogus box %d\n", k); */
	  lut_r[k] = lut_g[k] = lut_b[k] = red = green = blue = 0;		
	}
      color = M2TXColor_Create(0x00, 0xFF, red, green, blue);
      M2TXPIP_SetColor(pip, k, color);
    }

  qMemReleasePtr(cs->wt);
  qMemReleasePtr(cs->mr);
  qMemReleasePtr(cs->mg);
  qMemReleasePtr(cs->mb);
  qMemReleasePtr(cs);

  if (remap)
    for(i=0; i<size; ++i)
      {
	Qadd[i] = tag[Qadd[i]];
	raw->Red[i] = lut_r[Qadd[i]];
	raw->Green[i] = lut_g[Qadd[i]];
	raw->Blue[i] = lut_b[Qadd[i]];
      } 
  qMemReleasePtr(tag);
  qMemReleasePtr(Qadd);
  return (M2E_NoErr);
}


static colorspace *cs;

M2Err M2TXRaw_MultiMakePIP(M2TXRaw *raw, M2TXPIP *pip, 
			   int *numColors, bool init, bool makePIP)
{
  uint16 *Qadd;
  struct box	cube[MAXCOLOR];
  uint8 	*tag, red, green, blue;
  int		next;
  register int32	i, weight;
  register int	k;
  float		vv[MAXCOLOR], temp;
  M2TXColor     color;
  uint32       size;
  static float        *m2;
  M2Err        err;
  int          findColors;

  findColors = *numColors;

  if (init)
    {
      cs = (colorspace *)qMemNewPtr(sizeof(colorspace));
      cs->wt = (uint32 *)qMemNewPtr(33*33*33*sizeof(uint32));
      cs->mr = (uint32 *)qMemNewPtr(33*33*33*sizeof(uint32));
      cs->mg = (uint32 *)qMemNewPtr(33*33*33*sizeof(uint32));
      cs->mb = (uint32 *)qMemNewPtr(33*33*33*sizeof(uint32));
      m2 = (float *)qMemNewPtr(33*33*33*sizeof(float));

      if ((m2 == NULL) || ((cs->mb) == NULL))
	return (M2E_NoMem);
      
      for (i=0; i<(33*33*33); i++)
	{
	  cs->wt[i] = 0;
	  cs->mr[i] = 0;
	  cs->mg[i] = 0;
	  cs->mb[i] = 0;
	  m2[i] = 0.0;
	}
    }

  size = raw->XSize * raw->YSize;
  err = M2TXRaw_Hist3d(raw, cs, &Qadd, (float *)m2); 
  qMemReleasePtr(Qadd);

  if (err != M2E_NoErr)
    {
      qMemReleasePtr(m2);
      qMemReleasePtr(cs->wt);
      qMemReleasePtr(cs->mr);
      qMemReleasePtr(cs->mg);
      qMemReleasePtr(cs->mb);
      qMemReleasePtr(cs);
      return (err);
    }

  if (makePIP)
    {
      M3d(cs, (float *)m2);
      
      cube[0].r0 = cube[0].g0 = cube[0].b0 = 0;
      cube[0].r1 = cube[0].g1 = cube[0].b1 = 32;
      next = 0;
      
      for(i=1; i<findColors; ++i)
	{
	  if (Cut(&cube[next], &cube[i], cs)) 
	    {
	      /* volume test ensures we won't try to cut one-cell box */
	      vv[next] = (cube[next].vol>1) ? Var(&cube[next], cs, (float *)m2) : 0.0;
	      vv[i] = (cube[i].vol>1) ? Var(&cube[i], cs, (float *)m2) : 0.0;
	    } 
	  else 
	    {
	      vv[next] = 0.0;   /* don't try to split this box again */
	      i--;              /* didn't create box i */
	    }
	  next = 0; 
	  temp = vv[0];
	  for(k=1; k<=i; ++k)
	    if (vv[k] > temp) 
	      {
		temp = vv[k]; 
		next = k;
	      }
	  if (temp <= 0.0) 
	    {
	      *numColors = i+1;
	      break;
	    }
	}  
      /* the space for array m2 can be freed now */
  
      qMemReleasePtr(m2);

      M2TXPIP_SetNumColors(pip,*numColors);
      M2TXPIP_SetIndexOffset(pip,0);
      tag = (unsigned char *)qMemNewPtr(33*33*33);
      if (tag==NULL) 
	{
	  qMemReleasePtr(Qadd);
	  qMemReleasePtr(cs->wt);
	  qMemReleasePtr(cs->mr);
	  qMemReleasePtr(cs->mg);
	  qMemReleasePtr(cs->mb);
	  qMemReleasePtr(cs);
	  return (M2E_NoMem);
	}
      for(k=0; k<*numColors; ++k)
	{
	  Mark(&cube[k], k, tag);
	  weight = Vol(&cube[k], (uint32 *)(cs->wt));
	  if (weight) {
	    red = Vol(&cube[k],  (uint32 *)(cs->mr)) / weight;
	    green = Vol(&cube[k], (uint32 *)(cs->mg)) / weight;
	    blue = Vol(&cube[k],  (uint32 *)(cs->mb)) / weight;
	  }
	  else
	    {
	      /*  fprintf(stderr, "bogus box %d\n", k); */
	      red = green = blue = 0;		
	    }
	  color = M2TXColor_Create(0x00, 0xFF, red, green, blue);
	  M2TXPIP_SetColor(pip, k, color);
	}

      qMemReleasePtr(cs->wt);
      qMemReleasePtr(cs->mr);
      qMemReleasePtr(cs->mg);
      qMemReleasePtr(cs->mb);
      qMemReleasePtr(cs);

      qMemReleasePtr(tag);

    }
  return (M2E_NoErr);
}
