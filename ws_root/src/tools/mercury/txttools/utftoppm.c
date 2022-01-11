/*
	File:		utftoppm.c

	Contains:	Take a utf file and converts its channels into ppm files	

	Written by:	Todd Allendorf 

	Copyright:	© 1994 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <3>	 7/15/95	TMA		Changed pixel 0,0 to be the UPPER left corner to match change in
									the UTF format.
		 <2>	 5/16/95	TMA		Vertical image flip problem fixed. Autodocs added.
	To Do:
*/


/**
|||	AUTODOC -public -class tools -group m2tx -name utftoppm
|||	Convert a UTF file into a PPM file.
|||	
|||	  Synopsis
|||	
|||	    utftoppm <input file> <output file>
|||	
|||	  Description
|||	
|||	    This tool converts all the components of a UTF file into PPM files.  If
|||	    the UTF has separate alpha and SSB components, they will be written out
|||	    to separate files.  All output files will have a .ppm extension added 
|||	    automatically.
|||	
|||	  Arguments
|||	
|||	    <input file>
|||	        The input UTF texture.
|||	    <output file>
|||	        The resulting PPM file(s) base name.
|||	
|||	  Examples
|||	
|||	    utftoppm check.rgba.utf checkout
|||	
|||	    Assume check.rgba.utf has two levels of detail and has an imbedded alpha
|||	    component.  The tool will output four files named checkout.0.ppm,  
|||	    checkout.0.alpha.ppm, checkout.1.ppm, and checkout.1.alpha.ppm.
|||	
|||	  See Also
|||	
|||	    ppmtoutf
**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "M2TXlib.h"
#include "ppm.h"

void print_description()
{
  printf("Description:\n");
  printf("   Version %s\n",TEXTOOLS_VERISION);
  printf("   Convert UTF to PPM.\n");
}

#define MAXCOLORS 256

int main( int argc, char *argv[] )    
{
  M2TX tex;
  M2Err err;
  M2TXColor color;
  M2TXRaw raw;
  M2TXHeader *header;
  M2TXPIP *oldPIP;
  bool hasPIP, isCompressed, isLiteral, hasColor, hasAlpha, hasSSB;
  uint8 numLODs;
  char fileIn[256];
  char fileOut[256];
  char fileName[256];
  int i;
  uint32 curPixel;
  FILE* fPtr;
  int rows, cols, row, col, realrow;
  pixel** pixels;
  pixel myPixel;
  uint8 red, green, blue, ssb, alpha;
  
#ifdef M2STANDALONE
  printf("Enter: <FileIn> <FileOut>\n");
  printf("Example: dumb.utf dumb\n");
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
  
  fPtr = fopen(fileIn, "r");
  if (fPtr == NULL)
    {
      fprintf(stderr,"ERROR:Can't open file \"%s\" \n",fileIn);
      return(-1);
    }
  else 
    fclose(fPtr);		
  
  M2TX_Init(&tex);		/* Initialize a texture */
  
  err = M2TX_ReadFile(fileIn,&tex);

  if (err != M2E_NoErr)
    {
      fprintf(stderr,"ERROR:Bad file \"%s\" \n",fileIn);
      return(-1);
    }
  
  M2TX_GetHeader(&tex,&header);
  M2TXHeader_GetNumLOD(header,&numLODs);
  M2TXHeader_GetFIsCompressed(header, &isCompressed);
  M2TXHeader_GetFIsLiteral(header, &isLiteral);
  M2TXHeader_GetFHasColor(header, &hasColor);
  M2TXHeader_GetFHasAlpha(header, &hasAlpha);
  M2TXHeader_GetFHasSSB(header, &hasSSB);
  M2TXHeader_GetFHasPIP(header, &hasPIP);
  if (hasPIP)
      M2TX_GetPIP(&tex,&oldPIP);

  for (i=0; i<numLODs; i++)
    {
      if (isCompressed)
	err = M2TX_ComprToM2TXRaw(&tex, oldPIP, i, &raw);
      else
	err = M2TX_UncomprToM2TXRaw(&tex, oldPIP, i, &raw);		
      
      rows = raw.YSize;
      cols = raw.XSize;
      pixels = ppm_allocarray( cols, rows );
      if (hasColor)
	{
	  for (curPixel=0,row = 0; row < rows; ++row )
	    {
	      /*   realrow = rows - row - 1; */
	      realrow = row;
	      for ( col = 0; col < cols; ++col,curPixel++ )
		{
		  M2TXRaw_GetColor(&raw,curPixel,&color);
		  M2TXColor_Decode(color,&ssb,&alpha,&red,&green,&blue);
		  PPM_ASSIGN(myPixel, red, green, blue);
		  pixels[realrow][col]= myPixel;
		}
	    }
	  if (numLODs == 1)
	    sprintf(fileName, "%s.ppm",fileOut);
	  else
	    sprintf(fileName, "%s.%d.ppm",fileOut,i);
	  fPtr = fopen(fileName, "w");
	  if (fPtr == NULL)
	    {
	      fprintf(stderr,"ERROR:Can't open file \"%s\" \n",fileIn);
	      return(-1);
	    }
	  ppm_writeppm(fPtr, pixels, cols, rows, (pixval) 255, 0 );
	  fclose(fPtr);		
	}	
      if (hasAlpha)
	{
	  for (curPixel=0,row = 0; row < rows; row++ )
	    {
	      /*   realrow = rows - row - 1; */
	      realrow = row;
	      for ( col = 0; col < cols; ++col,curPixel++ )
		{
		  M2TXRaw_GetColor(&raw,curPixel,&color);
		  M2TXColor_Decode(color,&ssb,&alpha,&red,&green,&blue);
		  PPM_ASSIGN(myPixel,alpha,alpha,alpha);
		  pixels[realrow][col]=myPixel;
		}
	    }
	  if (numLODs == 1)
	    sprintf(fileName, "%s.alpha.ppm",fileOut);
	  else
	    sprintf(fileName, "%s.alpha.%d.ppm",fileOut,i);
	  fPtr = fopen(fileName, "w");
	  if (fPtr == NULL)
	    {
	      fprintf(stderr,"ERROR:Can't open file \"%s\" \n",fileIn);
	      return(-1);
		}
	  ppm_writeppm(fPtr, pixels, cols, rows, (pixval) 255, 0 );
	  fclose(fPtr);		
	}	  
      if (hasSSB)
	{
	  for (curPixel=0,row = 0; row < rows; ++row )
	    {
	      /*   realrow = rows - row - 1; */
	      realrow = row;
	      for ( col = 0; col < cols; ++col,curPixel++ )
		{
		  M2TXRaw_GetColor(&raw,curPixel,&color);
		  M2TXColor_Decode(color,&ssb,&alpha,&red,&green,&blue);
		  PPM_ASSIGN(myPixel,ssb,ssb,ssb);
		  pixels[realrow][col]=myPixel;
		}
	    }
	  if (numLODs == 1)
	    sprintf(fileName, "%s.ssb.ppm",fileOut);
	  else
	    sprintf(fileName, "%s.ssb.%d.ppm",fileOut,i);
	  fPtr = fopen(fileName, "w");
	  if (fPtr == NULL)
	    {
	      fprintf(stderr,"ERROR:Can't open file \"%s\" \n",fileIn);
	      return(-1);
	    }
	  ppm_writeppm(fPtr, pixels, cols, rows, (pixval) 255, 0 );
	  fclose(fPtr);		
	}	  
      M2TXRaw_Free(&raw);
      ppm_freearray(pixels,rows);
    }
  return( 0 );
} 


