/*
	File:		quantizer.c

	Contains:	An example of using M2TXlib's quantizing functions.

	Written by:	Todd Allendorf 

	Copyright:	© 1996 by The 3DO Company. All rights reserved.
	          	This material constitutes confidential and proprietary
	          	information of the 3DO Company and shall not be used by
	          	any Person or for any purpose except as expressly
	          	authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <4>	  8/7/95	TMA		Added autodocs and more error checking.
		 <2>	 7/16/95	TMA		Removed unused variables
	To Do:
*/

/**
|||	AUTODOC -public -class tools -group m2tx -name quantizer
|||	Finds a representative number of colors or alphas in an image.
|||	
|||	  Synopsis
|||	
|||	    quantizer <input file> <# of colors> [options] <output file>
|||	
|||	  Description
|||	
|||	    This tool takes a UTF file, finds a representative number of colors
|||	    or alpha values for the image.  Optional Floyd-Steinberg may be
|||	    performed at that time of color remapping.
|||	
|||	  Arguments
|||	
|||	    <input file>
|||	        The input UTF texture.
|||	    <# of colors>
|||	        The number of colors or alpha the output image will contain.
|||	    <output file>
|||	        The resulting UTF texture.
|||	
|||	  Options
|||	
|||	    -floyd
|||	        Perform Floyd-Steinberg dithering during color reduction
|||	    -alpha
|||	        Reduce the values of the alpha channel instead of the color channel
|||	
|||	  Caveats
|||	
|||	    This tool only does color reduction, it does not change the format
|||	    of the input image.  Therefore, if the input image is a literal image,
|||	    the output image will be a literal image.  Use utfmakepip to change
|||	    a quantized literal image to an indexed image.
|||	    
|||	  See Also
|||	
|||	    utflitdown, utfmakepip
**/


#include<stdio.h>
#include "M2TXlib.h"
#include <stdlib.h>
#include <string.h>


void print_description()
{
  printf("Description:\n");
  printf("   Version %s\n",TEXTOOLS_VERISION);
  printf("   Quantize the UTF image\n");
  printf("   -alpha \tQuantize the alpha instead of the alpha channel channel\n");
  printf("   -floyd \tUse Floyd-Steinberg dithering on the result\n");
}

int main( int argc, char *argv[] )
{
  uint32        pixels, curPixel;
  M2TXRaw       raw, rawAlpha;
  M2TX tex, newTex;
  bool          isCompressed, hasColor, hasAlpha, hasSSB;
  bool          firstLOD, done;
  bool          alpha = FALSE;
  bool          floyd = FALSE;
  M2TXHeader    *header, *newHeader;
  char fileIn[256];
  char fileOut[256];
  uint8         numLOD;
  int           numColors, lod;
  int argn;
  M2Err         err;

  M2TXPIP       *oldPIP, *newPIP, alphaPIP;
  FILE          *fPtr;
 
#ifdef M2STANDALONE
  printf("Enter: <FileIn> <numColors> <FileOut>\n");
  printf("Example: dumb.utf 64 dumb.indexed.utf\n");
  fscanf(stdin,"%s %d %s",fileIn, &numColors, fileOut);
#else
  /* Check for command line options. */
  if (argc < 4)
    {
      fprintf(stderr,"Usage: %s <Input File> <Num Colors> [-alpha] [-floyd] <Output File>\n",argv[0]);
      print_description();
      return(-1);    
    }
  strcpy(fileIn, argv[1]);
  argn = 3;
  while ( argn < argc && argv[argn][0] == '-' && argv[argn][1] != '\0' )
    {
      if ( strcmp( argv[argn], "-alpha")==0 )
        {
	  ++argn;
	  alpha = TRUE;
        }      
      else if ( strcmp( argv[argn], "-floyd")==0 )
        {
	  ++argn;
	  floyd = TRUE;
        }
      else
	{
	  fprintf(stderr,"Usage: %s <Input File> <Num Colors> [-alpha] [-floyd] <Output File>\n",argv[0]);
	  return(-1);
	}
    }
  if ( argn != argc )
    {
      /* Open the output file. */
      strcpy( fileOut, argv[argn] );
    }
  else
    {
      /* No output file specified. */
      fprintf(stderr,"Usage: %s <Input File> <Num Colors> [-alpha] [-floyd] <Output File>\n",argv[0]);
      return(-1);   
    }
  
  numColors = strtol(argv[2], NULL, 10);
  if ((numColors >256) || (numColors<1))
  {
    fprintf(stderr,"ERROR:%d colors not allowed. 1-256 per channel\n",numColors);
    return (-1);
  }
  strcpy( fileOut, argv[argn] );
  
#endif
  
  fPtr = fopen(fileIn, "r");
  if (fPtr == NULL)
    {
      fprintf(stderr,"ERROR:Can't open file \"%s\" \n",fileIn);
      return(-1);
    }
  else 
    fclose(fPtr);		
  
  M2TX_Init(&tex);			/* Initialize a texture */
  M2TX_Init(&newTex);			/* Initialize a texture */
  
  err = M2TX_ReadFile(fileIn,&tex);
  if (err != M2E_NoErr)
    {
      fprintf(stderr,"ERROR: Bad file \"%s\" \n", fileIn);
      return(-1);
    }
  M2TX_GetHeader(&tex,&header);
  M2TX_GetHeader(&newTex,&newHeader);
  /*  M2TX_SetHeader(&newTex, header); */
  M2TX_Copy(&newTex, &tex);
/* Old texture header to the new texture */
  M2TXHeader_GetFIsCompressed(header, &isCompressed);
  M2TXHeader_GetFHasColor(header, &hasColor);
  M2TXHeader_GetFHasAlpha(header, &hasAlpha);
  M2TXHeader_GetFHasSSB(header, &hasSSB);
  M2TX_GetPIP(&tex,&oldPIP);
  M2TX_GetPIP(&newTex, &newPIP);

  M2TXHeader_GetNumLOD(header, &numLOD);
  

  firstLOD = TRUE;
  for (lod=0; lod<numLOD; lod++)
    {
      if (isCompressed)
	err = M2TX_ComprToM2TXRaw(&tex, oldPIP, lod, &raw);
      else
	err = M2TX_UncomprToM2TXRaw(&tex, oldPIP, lod, &raw);
      
      if (err != M2E_NoErr)
	{
	  fprintf(stderr,"ERROR:Error during decompression file \"%s\" lod=%d, must abort.\n", fileIn, lod);
	  return(-1);
	} 

      if (lod == (numLOD-1))
	done = TRUE;
      else
	done = FALSE;
      
      /* Find the best PIP with numColors colors, leave the original data in raw alone */
      if (alpha)
	{
	  if (hasAlpha)
	    {	
	      M2TXRaw_Init(&rawAlpha, raw.XSize, raw.YSize, TRUE, FALSE, FALSE);
	      pixels = raw.XSize * raw.YSize;
	      for (curPixel=0; curPixel<pixels; curPixel++)
		{
		  rawAlpha.Red[curPixel] = rawAlpha.Green[curPixel] 
		    = rawAlpha.Blue[curPixel] = raw.Alpha[curPixel];
		}
	      err = M2TXRaw_MultiMakePIP(&rawAlpha, newPIP,
					 &numColors, firstLOD, done);
	      M2TXRaw_Free(&rawAlpha);
	    }
	}
      else
	{
	  if (hasColor)
	    err = M2TXRaw_MultiMakePIP(&raw, newPIP, &numColors,
				       firstLOD,done);
	}

      if (err != M2E_NoErr)
	{
	  fprintf(stderr,"ERROR:An error occured during MakePIP of file \"%s\" lod=%d:%d\n", fileIn, lod, err);
	  return(-1);
	}	      
      
      if (firstLOD)
	firstLOD = FALSE;
      M2TXRaw_Free(&raw);
    }

  fprintf(stderr,"Number of colors used = %d\n", numColors);  
  M2TXPIP_SetNumColors(&alphaPIP, 0);
  
  for (lod=0; lod<numLOD; lod++)
    {
      if (isCompressed)
	err = M2TX_ComprToM2TXRaw(&tex, oldPIP, lod, &raw);
      else
	err = M2TX_UncomprToM2TXRaw(&tex, oldPIP, lod, &raw);
	  
      if (err != M2E_NoErr)
	{
	  fprintf(stderr,"ERROR:Error during decompression file \"%s\" lod=%d, must abort.\n", fileIn, lod);
	  return(-1);
	} 

      if (alpha)
	{
	  if (raw.Alpha == NULL)
	    {
	      fprintf(stderr,"ERROR:No alpha channel to process.\n");
	      fprintf(stderr,"If alpha values are in the PIP, use utfmakesame to create an image with a literal alpha channel.\n");
	      return(-1);
	    }
	  M2TXRaw_Init(&rawAlpha, raw.XSize, raw.YSize, TRUE, FALSE, FALSE);
	  pixels = raw.XSize * raw.YSize;
	  
	  for (curPixel=0; curPixel<pixels; curPixel++)
	    {
	      rawAlpha.Red[curPixel] = rawAlpha.Green[curPixel] 
		= rawAlpha.Blue[curPixel] = raw.Alpha[curPixel];
	    }
	  err = M2TXRaw_MapToPIP(&rawAlpha, newPIP, numColors,
				 floyd);
	  if (err != M2E_NoErr)
	    {
	      fprintf(stderr,"ERROR:An error occured during ditherof \"%s\" lod=%d:%d\n", fileIn, lod, err);
	      return(-1);
	    }
	  for (curPixel=0; curPixel<pixels; curPixel++)
	    {
	      raw.Alpha[curPixel] = rawAlpha.Red[curPixel]; 
	    }
	  M2TXRaw_Free(&rawAlpha);
#if 0
	  err = M2TXRaw_FindPIP(&raw, &alphaPIP, &aCDepth, !isLiteral,
				!hasAlpha, !hasSSB);
#endif
	  err = M2TXRaw_ToUncompr(&newTex, oldPIP, lod, &raw);
	  if (err != M2E_NoErr)
	    {
	      fprintf(stderr,"ERROR:An error occured during encoding of image\"%s\" lod=%d : %d\n", fileIn, lod, err);
	      return(-1);
	    }
	}
      else
	{
	  err = M2TXRaw_MapToPIP (&raw, newPIP, numColors, floyd);
	  if (err != M2E_NoErr)
	    {
	      fprintf(stderr,"ERROR:An error occured during ditherof \"%s\" lod=%d:%d\n", fileIn, lod, err);
	      return(-1);
	    }
	  err = M2TXRaw_ToUncompr(&newTex, newPIP, lod, &raw);
	  if (err != M2E_NoErr)
	    {
	      fprintf(stderr,"ERROR:An error occured during encoding of image\"%s\" lod=%d : %d\n", fileIn, lod, err);
	      return(-1);
	    }
	}
      /* Write out the new quantized texture, changing nothing else */
      M2TXRaw_Free(&raw);
    }
  M2TX_WriteFile(fileOut, &newTex);
  /*  Crashes the Mac, screw it
      M2TX_Free(&tex, TRUE, TRUE, TRUE, TRUE);
      M2TX_Free(&newTex, TRUE, TRUE, TRUE, TRUE);
      */
  return(0);
}

