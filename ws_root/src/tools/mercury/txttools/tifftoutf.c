/*
	File:		tifftoutf.c

	Contains:	Converts a TIFF file to a UTF file

	Written by:	Todd Allendorf 

	Copyright:	© 1996 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

*/


/**
|||	AUTODOC -public -class tools -group m2tx -name tifftoutf
|||	Converts a TIFF file into a UTF file.
|||	
|||	  Synopsis
|||	
|||	    tifftoutf <input file> <output file>
|||	
|||	  Description
|||	
|||	    This tool converts all the components of a TIFF 3.0 file into a UTF file. 
|||	    The first color channel(s) get mapped to a color channels of a UTF file.
|||	    If additional channels are present, the next one is mapped into a
|||	    alpha channel in the UTF file.  If yet another channel is present,
|||	    it is convertd into an SSB channel in the UTF file.
|||	
|||	  Arguments
|||	
|||	    <input file>
|||	        The input TIFF texture.
|||	    <output file>
|||	        The resulting UTF texture
|||	
|||	  See Also
|||	
|||	    ppmtoutf, psdtoutf
**/



#include <stdlib.h>
#include <string.h>
#include "M2TXlib.h"
#include "tiffio.h"

#define MAXCOLORS 256
#define UTF_MAXMAXVAL 256



void print_description()
{
  printf("Description:\n");
  printf("   Version %s\n",TEXTOOLS_VERISION);
  printf("   TIFF to UTF\n");
}


int main( int argc, char *argv[] )
{
  int argn, cols, rows, grayscale, format;
  int numcolors;
  TIFF* tif;
  int row, i;
  int col;
  uint8 *buf;
  uint8 *inP;
  int maxval;
  uint8 *xelrow;
  uint8 *xP;
  M2TXPIP *pip;
  int headerdump;
  uint8 sample;
  int bitsleft;
  unsigned short bps, spp, photomet;
  unsigned short* redcolormap;
  unsigned short* greencolormap;
  unsigned short* bluecolormap;
  
  static M2TX tex;
  M2Err err;
  M2TXRaw raw;
  M2TXIndex index;
  M2TXColor color;
  M2TXHeader *header;
  uint8 numLODs;
  FILE *fPtr;
  char fileIn[256];
  char fileOut[256];
  bool isLiteral, hasAlpha;
  uint32 pixel;
  
#ifdef M2STANDALONE
  printf("Enter: <FileIn> <FileOut>\n");
  printf("Example: dumb.utf dumb.cmp.utf\n");
  fscanf(stdin,"%s %s",fileIn, fileOut);
#else
  /* Check for command line options. */
  if (argc != 3)
    {
      fprintf(stderr,"Usage: %s <Input File> <Output File>\n",argv[0]);
      print_description();
      return(-1);	
    }	
  else
    {
      strcpy(fileIn, argv[1]);
      strcpy(fileOut, argv[2]);
    }
#endif
  
  tif = TIFFOpen( fileIn, "r" );
  if ( tif == NULL )
    fprintf(stderr, "error opening TIFF file %s\n", argv[argn] );
  
  headerdump = 0;

  if ( headerdump )
    TIFFPrintDirectory( tif, stderr, TIFFPRINT_NONE );
  
  if ( ! TIFFGetField( tif, TIFFTAG_BITSPERSAMPLE, &bps ) )
    bps = 1;
  if ( ! TIFFGetField( tif, TIFFTAG_SAMPLESPERPIXEL, &spp ) )
    spp = 1;
  if ( ! TIFFGetField( tif, TIFFTAG_PHOTOMETRIC, &photomet ) )
    fprintf(stderr, "error getting photometric\n" );

  switch ( spp )
    {
    case 1:
    case 3:
    case 4:
      break;
      
    default:
      fprintf(stderr,
	      "can only handle 1-channel gray scale or 1- or 3-channel color\n" );
    }

  M2TX_Init(&tex);
  M2TX_GetPIP(&tex, &pip);
  M2TX_GetHeader(&tex, &header);
  M2TXHeader_SetFHasColor(header, TRUE);
  M2TXHeader_SetCDepth(header, 8);
  M2TXHeader_SetNumLOD(header, 1);
  
  (void) TIFFGetField( tif, TIFFTAG_IMAGEWIDTH, &cols );
  (void) TIFFGetField( tif, TIFFTAG_IMAGELENGTH, &rows );
  
  if ( headerdump )
    {
      fprintf(stderr, "%dx%dx%d image\n", cols, rows, bps * spp );
      fprintf(stderr, "%d bits/sample, %d samples/pixel\n", bps, spp );
    }
  
  maxval = ( 1 << bps ) - 1;
  if ( maxval == 1 && spp == 1 )
    {
      if ( headerdump )
	fprintf(stderr,"monochrome\n" );
      grayscale = 1;
    }
  else
    {
      switch ( photomet )
	{
	case PHOTOMETRIC_MINISBLACK:
	  if ( headerdump )
	    fprintf(stderr, "%d graylevels (min=black)\n", maxval + 1 );
	  grayscale = 1;
	  isLiteral = FALSE;
	  break;
	  
	case PHOTOMETRIC_MINISWHITE:
	  if ( headerdump )
	    fprintf(stderr, "%d graylevels (min=white)\n", maxval + 1 );
	  grayscale = 1;
	  isLiteral = FALSE;
	  break;
	  
	case PHOTOMETRIC_PALETTE:
	  if ( headerdump )
	    fprintf(stderr, "colormapped\n");
	  if (!TIFFGetField( tif, TIFFTAG_COLORMAP, &redcolormap, &greencolormap, &bluecolormap))
	    fprintf(stderr,"error getting colormaps\n" );
	  numcolors = maxval + 1;
	  if ( numcolors > MAXCOLORS )
	    fprintf(stderr,"too many colors\n" );

	  grayscale = 0;
	  for ( i = 0; i < numcolors; ++i )
	    {
	      register uint8 r, g, b, a;
	      r =  redcolormap[i]*UTF_MAXMAXVAL/65535L;
	      g =  greencolormap[i]*UTF_MAXMAXVAL/65535L;
	      b =  bluecolormap[i]*UTF_MAXMAXVAL/65535L;
	      color = M2TXColor_Create(1,0xFF, r, g, b);
	      M2TXPIP_SetColor(pip, i, color); 
	    }
	  isLiteral = FALSE;
	  break;

	case PHOTOMETRIC_RGB:
	  if ( headerdump )
	    fprintf(stderr, "truecolor\n");
	  grayscale = 0;
	  isLiteral = TRUE;
	  break;

	case PHOTOMETRIC_MASK:
	  fprintf(stderr, "don't know how to handle PHOTOMETRIC_MASK\n");

	case PHOTOMETRIC_SEPARATED:
	  fprintf(stderr,"don't know how to handle PHOTOMETRIC_SEPARATED\n" );

	case PHOTOMETRIC_YCBCR:
	  fprintf(stderr, "don't know how to handle PHOTOMETRIC_YCBCR\n" );

	case PHOTOMETRIC_CIELAB:
	  fprintf(stderr, "don't know how to handle PHOTOMETRIC_CIELAB\n" );

	default:
	  fprintf(stderr, "unknown photometric: %d\n", photomet );
	}
    }
  if ( maxval > UTF_MAXMAXVAL )
    fprintf(stderr,
	     "bits/sample is too large - try reconfiguring with PGM_BIGGRAYS\n    or without PPM_PACKCOLORS\n" );


  buf = (uint8 *) malloc(TIFFScanlineSize(tif));
  if ( buf == NULL )
    fprintf(stderr,"can't allocate memory for scanline buffer\n");

  hasAlpha = FALSE;
  if (!isLiteral)
    {
      err = M2TXIndex_Init(&index, cols, rows, TRUE, FALSE, FALSE);
      M2TXHeader_SetFHasPIP(header, TRUE);
    }
  else
    {
      if (spp>3)
	{
	  M2TXHeader_SetADepth(header, 7);
	  M2TXHeader_SetFHasSSB(header, TRUE);
	  hasAlpha = TRUE;
	}
      else
	{
	  M2TXHeader_SetADepth(header, 0);
	  hasAlpha = FALSE;
	}
      err = M2TXRaw_Init(&raw, cols, rows, TRUE, hasAlpha, FALSE);
    }
  M2TXHeader_SetFIsLiteral(header, isLiteral);
  M2TXHeader_SetFHasAlpha(header, hasAlpha);
  M2TXHeader_SetMinXSize(header, cols);
  M2TXHeader_SetMinYSize(header, rows);

#define NEXTSAMPLE \
  { \
      if ( bitsleft == 0 ) \
      { \
	  ++inP; \
	  bitsleft = 8; \
      } \
      bitsleft -= bps; \
      sample = ( *inP >> bitsleft ) & maxval; \
  }

  for (row = 0, pixel = 0; row < rows; ++row )
    {
      if ( TIFFReadScanline( tif, buf, row, 0 ) < 0 )
	fprintf(stderr, "bad data read on line %d\n", row );
      inP = buf;
      bitsleft = 8;
      xP = xelrow;

      switch ( photomet )
	{
	case PHOTOMETRIC_MINISBLACK:
	  for (col=0; col < cols; ++col, pixel++)
	    {
	      NEXTSAMPLE
		index.Index[pixel] = sample;
	    }
	  break;
	  
	case PHOTOMETRIC_MINISWHITE:
	  for (col=0; col < cols; ++col, pixel++)
	    {
	      NEXTSAMPLE
		sample = maxval - sample;
	      index.Index[pixel] = sample;
	    }
	  break;

	case PHOTOMETRIC_PALETTE:
	  for (col=0; col < cols; ++col, pixel++)
	    {
	      NEXTSAMPLE
		index.Index[pixel] = sample;
	    }
	  break;
	  
	case PHOTOMETRIC_RGB:
	  for (col=0; col < cols; ++col, pixel++)
	    {
	      register uint8 r, g, b, a;

	      NEXTSAMPLE
		r = sample;
	      raw.Red[pixel] = r;
	      NEXTSAMPLE
		g = sample;
	      raw.Green[pixel] = g;
	      NEXTSAMPLE
		b = sample;
	      raw.Blue[pixel] = b;
	      if ( spp == 4 )
		{
		  NEXTSAMPLE		/* skip alpha channel */
		    a = sample;
		  raw.Alpha[pixel] = a;
		}
	    }
	  break;

	default:
	  fprintf(stderr,"unknown photometric: %d\n", photomet);
	}
    }

  if (!isLiteral )
    {
      M2TXIndex_ToUncompr(&tex, pip, 0, &index);
      M2TXIndex_Free(&index);
    }
  else
    {
      M2TXRaw_ToUncompr(&tex, NULL, 0, &raw);
      M2TXRaw_Free(&raw);
    }

  M2TX_WriteFile(fileOut, &tex);
  M2TXHeader_FreeLODPtrs(header);
}
