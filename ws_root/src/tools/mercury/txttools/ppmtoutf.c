/*
	File:		ppmtoutf.c

	Contains:	Takes a ppm file and converts it to a given utf channel	

	Written by:	Todd Allendorf 

	Copyright:	© 1996 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <4>	  8/7/95	TMA		Changed parameters to be more intuitive.
		 <3>	 7/15/95	TMA		Change 0,0 to be UPPER left corner to match change in the UTF
									format.  Properly set the PIP of indexed textures.
		 <2>	 5/16/95	TMA		Fixed vertical image flip. Added autodocs.
	To Do:
*/

/**
|||	AUTODOC -public -class tools -group m2tx -name ppmtoutf
|||	Convert a PPM file into a UTF file.
|||	
|||	  Synopsis
|||	
|||	    ppmtoutf [options] <input file>
|||	
|||	  Description
|||	
|||	    This tool converts all the components of a PPM file into a UTF file. 
|||	    Only a single channel of a UTF file (color, alpha, ssb) may be generated
|||	    at once.  utfmerge must be used to merge different channels into a single
|||	    UTF file, if needed.
|||	
|||	  Arguments
|||	
|||	    <input file>
|||	        The input PPM texture.
|||	
|||	  Options
|||	
|||	    -name
|||	        The UTF file name.  If none is given, the input name is used.
|||	    -cmap
|||	        Indicates that image should be output as an indexed image. 
|||	    -24
|||	        Indicates that image should be output as an literal image. 
|||	    -16
|||	        Indicates that image should be output as an literal but with a 16 bit image. 
|||	    -color
|||	        The PPM file represents color information.
|||	    -alpha
|||	        The PPM file represents alpha information.
|||	    -ssb
|||	        The PPM file represents ssb information.
|||	
|||	  Examples
|||	
|||	    ppmtoutf -name out.utf -cmap -color in.ppm
|||	
|||	    This will convert the ppm color image into an indexed color UTF image.
|||	
|||	    ppmtoutf -name out.utf -24 -alpha in.ppm
|||	
|||	    This will convert the ppm color image into literal alpha image.
|||	
|||	  See Also
|||	
|||	    utftoppm, utfmerge
**/

#include <stdlib.h>
#include <stdio.h>
#include "ppm.h"
#include "ppmcmap.h"
#include "M2TXlib.h"

/* Max number of colors allowed for colormapped output. */
#define MAXCOLORS 256

/* Forward routines. */
static void put_map_entry ARGS(( pixel* valueP, int size, pixval maxval ));
static void compute_runlengths ARGS(( int cols, pixel* pixelrow, int* runlength ));
static void put_pixel ARGS(( pixel* pP, int imgtype, pixval maxval, colorhash_table cht ));
static void put_mono ARGS(( pixel* pP, pixval maxval ));
static void put_map ARGS(( pixel* pP, colorhash_table cht ));
static void put_rgb ARGS(( pixel* pP, pixval maxval ));

/* Routines. */

void print_description()
{
  printf("Description:\n");
  printf("   Version 1.1\n");
  printf("   Convert PPM to UTF\n");
  printf("   -name\tThe output UTF filename.\n");
  printf("   -cmap\tMake an indexed image.\n");
  printf("   -24\tMake a literal image.\n");
  printf("   -16\tMake a literal 16 bit (5-5-5) color image.\n");
  printf("   -alpha\tTreat the incoming data as alpha data.\n");
  printf("   -colort\tTreat the incoming data as color data.(default)\n");
  printf("   -ssbt\tTreat the incoming data as ssb data.(default)\n");
}

int main( int argc, char *argv[] )
{
  M2TX tex;
  M2Err err;
  M2TXRaw raw;
  M2TXHeader *header;
  M2TXPIP pip, *usePIP;
  uint8 cDepth;
  FILE *ifp, *fPtr;
  uint32 curPixel;
  pixel **pixels;
  register pixel *pP;
  int argn, rows, cols, ncolors, row, col, format, realrow;
  uint16 width, height;
  bool isLiteral=TRUE;
  bool is24=TRUE;
  bool hasColor=TRUE;
  bool hasAlpha=FALSE;
  bool hasSSB=FALSE;
  pixval maxval;
  colorhist_vector chv;
  char fileIn[256];
  char fileOut[256];
  char out_name[100];
  char* cp;
  char* usage = "[-name <utfname>] [-cmap|-16|-24] [-color|-alpha|-ssb] [ppmfile]";
  
  out_name[0] = '\0';
  isLiteral = TRUE;
  
#ifndef M2STANDALONE
  ppm_init( &argc, argv );
  
  /* Check for command line options. */
  argn = 1;
  while ( argn < argc && argv[argn][0] == '-' && argv[argn][1] != '\0' )
    {
      if ( pm_keymatch( argv[argn], "-name", 2 ) )
	{
	  ++argn;
	  if ( argn == argc )
	   {
		   pm_usage( usage );
		    print_description();
		}
	  (void) strcpy( out_name, argv[argn] );
	}
      else if ( pm_keymatch( argv[argn], "-cmap", 2 ) )
	isLiteral = FALSE;
      else if ( pm_keymatch( argv[argn], "-color", 2 ) )
        {
	  hasColor = TRUE;
	  hasAlpha = FALSE;
	  hasSSB = FALSE;
        }
      else if ( pm_keymatch( argv[argn], "-alpha", 2 ) )
        {
	  hasAlpha = TRUE;
	  hasSSB   = FALSE;
	  hasColor = FALSE;
        }
      else if ( pm_keymatch( argv[argn], "-ssb", 2 ) )
        {
	  hasAlpha = FALSE;
	  hasSSB   = TRUE;
	  hasColor = FALSE;
        }
      else if ( pm_keymatch( argv[argn], "-24", 2 ) )
	{
	  isLiteral = TRUE;
	  is24 = TRUE;
	}
      else if ( pm_keymatch( argv[argn], "-16", 2 ) )
	{
	  isLiteral = TRUE;
	  is24 = FALSE;
	  hasSSB = TRUE;
	}
      else
	pm_usage( usage );
      ++argn;
    }
  
  if ( argn != argc )
    {
      /* Open the input file. */
      ifp = pm_openr( argv[argn] );
      
      /* If output filename not specified, use input filename as default. */
      if ( out_name[0] == '\0' )
	{
	  (void) strcpy( out_name, argv[argn] );
	  cp = index( out_name, '.' );
	  if ( cp != 0 )
	    *cp = '\0';	/* remove extension */
	  if ( strcmp( out_name, "-" ) == 0 )
	    (void) strcpy( out_name, "noname" );
	}
      
      ++argn;
    }
  else
    {
      /* No input file specified. */
      ifp = stdin;
      if ( out_name[0] == '\0' )
	(void) strcpy( out_name, "noname" );
    }
  
  if ( argn != argc )
    pm_usage( usage );
  
#else
  printf("Enter: <FileIn> <FileOut>\n");
  printf("Example: dumb.ppm dumb.utf\n");
  fscanf(stdin,"%s %s",fileIn,  fileOut);
  fPtr = fopen(fileIn, "r");
  if (fPtr == NULL)
    {
      printf("ERROR:Can't open file \"%s\" \n",fileIn);
      return(-1);
    }
  else 
    fclose(fPtr);
  
  /* Open the input file. */
  ifp = pm_openr( fileIn );
  (void) strcpy( out_name, fileOut );
  /* If output filename not specified, use input filename as default. */
  if ( out_name[0] == '\0' )
    {
      (void) strcpy( out_name, fileIn );
      cp = index( out_name, '.' );
      if ( cp != 0 )
	*cp = '\0';	/* remove extension */
      if ( strcmp( out_name, "-" ) == 0 )
	(void) strcpy( out_name, "noname" );
    }
#endif
  
  /* Read in the ppm file. */
  ppm_readppminit( ifp, &cols, &rows, &maxval, &format);
  pixels = ppm_allocarray( cols, rows );
  for ( row = 0; row < rows; ++row )
    ppm_readppmrow( ifp, pixels[row], cols, maxval, format );
  pm_close( ifp );
  
  /* Figure out the colormap. */
  switch ( PPM_FORMAT_TYPE( format ) )
    {
    case PPM_TYPE:
      if ( !isLiteral )
	{
	  pm_message( "computing colormap..." );
	  chv = ppm_computecolorhist(pixels, cols, rows, MAXCOLORS, &ncolors );
	  if ( chv == (colorhist_vector) 0 )
	    {
	      pm_error(
		       "too many colors - try doing a 'ppmquant %d'",
		       MAXCOLORS );
	    }
	  else
	    {
	      pm_message( "%d colors found", ncolors );
			}
	}
      break;
      
    case PGM_TYPE:
    case PBM_TYPE:
      pm_message( "computing colormap..." );
      chv = ppm_computecolorhist(pixels, cols, rows, MAXCOLORS, &ncolors );
      if ( chv == (colorhist_vector) 0 )
	pm_error( "can't happen" );
      pm_message( "%d colors found", ncolors );
      break;
      
    default:
      pm_error( "can't happen" );
    }
  
  M2TX_Init(&tex);
  M2TX_GetHeader(&tex,&header);
  width = cols;
  height = rows;
  /* Color, but no Alpha and SSB */
  M2TXRaw_Init(&raw, width, height, hasColor, hasAlpha, hasSSB); 
  
  /* Turn PPM into literal image */
  curPixel = 0;
  for ( row = 0; row < rows; ++row )
    {
      realrow = row;
      /* realrow = rows - row - 1; */
      for ( col = 0, pP = pixels[realrow]; col < cols; ++col, ++pP )
	{
	  if (hasColor)
	    {
	      raw.Red[curPixel] = PPM_GETR(*pP); 	
	      raw.Green[curPixel] = PPM_GETG(*pP); 	
	      raw.Blue[curPixel] = PPM_GETB(*pP); 	
	    }
	  if (hasAlpha)
	    {
	      raw.Alpha[curPixel] = PPM_GETR(*pP);
	    }
	  if (hasSSB)
	    {
	      raw.SSB[curPixel] = PPM_GETR(*pP);
	    }
	  curPixel++;
	}
    }
  
  usePIP = NULL;
  M2TXHeader_SetFHasColor(header, hasColor);
  M2TXHeader_SetFHasAlpha(header, hasAlpha);
  M2TXHeader_SetFHasSSB(header, hasSSB);
  M2TXHeader_SetFIsCompressed(header, FALSE);
  if ( !isLiteral )
    {
      M2TXPIP_SetNumColors(&pip, 0);
      err = M2TXRaw_FindPIP(&raw, &pip, &cDepth,hasColor,hasAlpha,hasSSB);
      while (!(M2TX_IsLegal(cDepth, 0, 0, FALSE)))
	cDepth++;
      
      /* Make a hash table for fast color lookup. */
      usePIP = &pip;
      M2TXHeader_SetFIsLiteral(header, FALSE);
      M2TXHeader_SetFHasPIP(header, TRUE);
      M2TXHeader_SetFHasColor(header, TRUE);
      M2TXHeader_SetFHasAlpha(header, FALSE);
      M2TXHeader_SetFHasSSB(header, FALSE);        
      M2TXHeader_SetCDepth(header, cDepth);
      M2TX_SetPIP(&tex, usePIP);
    }
  else
    {
      M2TXHeader_SetFIsLiteral(header, TRUE);
      M2TXHeader_SetFHasPIP(header, FALSE);
      if (hasColor)
	{
	  if (is24)
	    M2TXHeader_SetCDepth(header, 8);
	  else
	    M2TXHeader_SetCDepth(header, 5);
	}	    
      else if (hasAlpha)
	M2TXHeader_SetADepth(header,7);
    }
  M2TXHeader_SetMinXSize(header, width);
  M2TXHeader_SetMinYSize(header, height);
  M2TXHeader_SetNumLOD(header, 1);
  
  M2TXRaw_ToUncompr(&tex, &pip, 0, &raw);
  M2TX_WriteFile(out_name, &tex);
  M2TXHeader_FreeLODPtrs(header);
  
  return(0);
}
