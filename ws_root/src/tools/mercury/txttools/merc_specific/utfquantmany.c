/*
	File:		utfquantmany.c

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
|||	AUTODOC -public -class tools -group m2tx -name utfquantmany
|||	Finds a representative number of colors or alphas in a series of images
|||	and quantize them to that palette
|||	
|||	  Synopsis
|||	
|||	    utfquantmany <pip output file> <# of colors> [options] <input files>
|||	
|||	  Description
|||	
|||	    This tool takes a series of UTF files, finds a representative number 
|||	    of colors or alpha values for the image.  Optional Floyd-Steinberg may
|||	    be performed at that time of color remapping.
|||	
|||	  Arguments
|||	
|||	    <pip output file>
|||	        The quantized pip for all the input images
|||	    <# of colors>
|||	        The number of colors or alpha the output images will contain.
|||	    <input files>
|||	        The input UTF textures.
|||	
|||	  Options
|||	
|||	    -floyd
|||	        Perform Floyd-Steinberg dithering during color reduction
|||	    -alpha
|||	        Reduce the values of the alpha channel instead of the color channel
|||	    -pip
|||	        Change the input images to use the output pip
|||	
|||	  Caveats
|||	
|||	    The -pip option is destructive to the input data (i.e. you will lose
|||	    your input files).  Copies of original data should be used when using 
|||	    the -pip to prevent data loss.
|||	    
|||	  See Also
|||	
|||	    quanttopip, utfmakepip
**/


#include<stdio.h>
#include "M2TXlib.h"
#include <stdlib.h>
#include <string.h>

void print_description()
{
  printf("Description:\n");
  printf("   Version %s\n",TEXTOOLS_VERISION);
  printf("   Quantize a series of images to the same pip\n");
  printf("   -alpha \tQuantize the alpha instead of the alpha channel channel\n");
  printf("   -floyd \tUse Floyd-Steinberg dithering on the result\n");
  printf("   -pip   \tWrite out the input images as indexed with the new PIP\n");
}

#define Usage  printf("Usage: %s <output PIP File> <Num Colors> [-alpha] [-floyd] <Input Files>\n",argv[0])

M2Err make_legal(M2TXHeader *header)
{
  uint8 aDepth, cDepth, ssbDepth;
  bool isLiteral, hasSSB, hasColor, hasAlpha;
  bool needSSB = FALSE;

  M2TXHeader_GetFIsLiteral(header, &isLiteral);
  M2TXHeader_GetFHasSSB(header, &hasSSB);
  if (hasSSB)
    ssbDepth = 1;
  else
    ssbDepth = 0;
  M2TXHeader_GetFHasAlpha(header, &hasAlpha);
  if (hasAlpha)
    M2TXHeader_GetADepth(header, &aDepth);	
  else
    aDepth = 0;
  M2TXHeader_GetFHasColor(header, &hasColor);
  if (hasColor)
    M2TXHeader_GetCDepth(header, &cDepth);	
  else
    cDepth = 0;

  while ((!(M2TX_IsLegal(cDepth, aDepth, ssbDepth, isLiteral))) && (cDepth<9))
    cDepth++;
  if (cDepth>8)
    {
      cDepth = 8;
      if (!hasSSB)
	{
	  if (M2TX_IsLegal(cDepth,aDepth, 1, isLiteral))
	    needSSB = TRUE;
	  else
	    {
	      fprintf(stderr,"ERROR:Can't find the right color, alpha, ssb combo\n");
	      return(M2E_Range);
	    }
	}	
      else
	{
	  fprintf(stderr,"ERROR:Can't find the right color, alpha, ssb combo\n");
	  return(M2E_Range);
	}
    }
  M2TXHeader_SetCDepth(header,cDepth);
  if (needSSB)
    M2TXHeader_SetFHasSSB(header, TRUE);
}

void M2TXRaw_MapPreserve();

int main( int argc, char *argv[] )
{
  uint32        pixels, curPixel;
  M2TXRaw       raw, rawAlpha;
  M2TX tex, newTex;
  bool          isCompressed, hasColor, hasAlpha;
  bool          alpha = FALSE;
  bool          floyd = FALSE;
  bool          pippify = FALSE;
  bool          firstLOD, done;
  M2TXHeader    *header, *newHeader;
  char fileIn[256];
  char fileOut[256];
  uint8         numLOD, lod, pipCDepth, red, grn, blu, ssb, opac;
  int           numColors;
  int           fileArg, i, argn;
  M2Err         err;

  M2TXColor     color;
  M2TXPIP       manyPIP;
  M2TXPIP       *oldPIP;
 
#ifdef M2STANDALONE
  printf("Enter: <output pip file> <numColors> <input file>\n");
  printf("Example: dumb.utf 64 dumb.indexed.utf\n");
  fscanf(stdin,"%s %d %s",fileIn, &numColors, fileOut);
#else
  /* Check for command line options. */
  if (argc < 4)
    {
      Usage;
      print_description();
      return(-1);    
    }
  strcpy(fileOut, argv[1]);
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
      else if ( strcmp( argv[argn], "-pip")==0 )
        {
	  ++argn;
	  pippify = TRUE;
        }
      else
	{
	  Usage;
	  return(-1);
	}
    }
  if ( argn == argc )
    {
      /* No output file specified. */
      Usage;
      return(-1);
    }
  
  numColors = strtol(argv[2], NULL, 10);
  if ((numColors >256) || (numColors<1))
  {
    fprintf(stderr,"ERROR:%d colors not allowed. 1-256 per channel\n",numColors);
    return (-1);
  }

  fileArg = argn;

#endif
  
  

  firstLOD = TRUE;
  for (;argn < argc; argn++ )
    {
      M2TX_Init(&tex);			/* Initialize a texture */
      strcpy( fileIn, argv[argn] );

      err = M2TX_ReadFile(fileIn,&tex);
      if (err != M2E_NoErr)
	return(-1);
      M2TX_GetHeader(&tex,&header);
      
      /*M2TX_SetHeader(&newTex, header);   */
      /* Old texture header to the new texture */
      M2TXHeader_GetFIsCompressed(header, &isCompressed);
      M2TXHeader_GetFHasColor(header, &hasColor);
      M2TXHeader_GetFHasAlpha(header, &hasAlpha);
      M2TX_GetPIP(&tex,&oldPIP);
      
      M2TXHeader_GetNumLOD(header, &numLOD);
      
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

	  if ((argn == argc-1) &&(lod == (numLOD-1)))
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
		  err = M2TXRaw_MultiMakePIP(&rawAlpha, &manyPIP,
					     &numColors, firstLOD, done);
		  M2TXRaw_Free(&rawAlpha);
		}
	    }
	  else
	    {
	      if (hasColor)
		err = M2TXRaw_MultiMakePIP(&raw, &manyPIP, &numColors,
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
      M2TX_Free(&tex, TRUE, FALSE, FALSE, FALSE);
    }
  
  fprintf(stderr,"Number of colors used = %d\n", numColors);  
  M2TXRaw_MapPreserve();

  pipCDepth = 1;
  while ( numColors > (1<<pipCDepth))
    pipCDepth++;
   
  for (argn = fileArg; argn < argc; argn++ )
    {
      M2TX_Init(&tex);			/* Initialize a texture */
      M2TX_Init(&newTex);			/* Initialize a texture */
      strcpy( fileIn, argv[argn] );
      
      err = M2TX_ReadFile(fileIn,&tex);
      if (err != M2E_NoErr)
	return(-1);
      M2TX_GetHeader(&tex,&header);
      M2TX_GetHeader(&newTex,&newHeader);

      M2TX_Copy(&newTex, &tex);
      /* M2TX_SetHeader(&newTex, header);   */
      /* Old texture header to the new texture */
      M2TXHeader_GetFIsCompressed(header, &isCompressed);
      M2TXHeader_GetFHasColor(header, &hasColor);
      M2TXHeader_GetFHasAlpha(header, &hasAlpha);
      M2TX_GetPIP(&tex,&oldPIP);
      M2TXHeader_SetFIsCompressed(newHeader, FALSE);
      
      M2TXHeader_GetNumLOD(header, &numLOD);
      
      if (alpha) 
	{
	  M2TXHeader_SetFHasAlpha(newHeader, TRUE);
	  M2TXHeader_SetADepth(newHeader, 7);
	  M2TXHeader_SetFHasSSB(newHeader, TRUE);
	}

      if (pippify) 
	{
	  M2TXHeader_SetFHasColor(newHeader, TRUE);
	  M2TXHeader_SetCDepth(newHeader, pipCDepth);
	  M2TXHeader_SetFIsLiteral(newHeader, FALSE);
	  M2TXHeader_SetFHasPIP(newHeader, TRUE);
	  M2TX_SetPIP(&newTex, &manyPIP);
	}

      make_legal(newHeader);

      for (lod=0; lod<numLOD; lod++)
	{
	  if (isCompressed)
	    err = M2TX_ComprToM2TXRaw(&tex, oldPIP, lod, &raw);
	  else
	    err = M2TX_UncomprToM2TXRaw(&tex, oldPIP, lod, &raw);
	  
	  if (alpha)
	    {
	      if (raw.Alpha == NULL)
		{
		  fprintf(stderr,"ERROR:No alpha channel to process.\n");
		  fprintf(stderr,"If alpha values are in the PIP, use utfmakesame to create an image with a literal alpha channel.\n");
		  M2TXRaw_Free(&raw);
		  continue;
		}
	      M2TXRaw_Init(&rawAlpha, raw.XSize, raw.YSize, TRUE, FALSE, FALSE);
	      pixels = raw.XSize * raw.YSize;	  
	      for (curPixel=0; curPixel<pixels; curPixel++)
		{
		  rawAlpha.Red[curPixel] = rawAlpha.Green[curPixel] 
		    = rawAlpha.Blue[curPixel] = raw.Alpha[curPixel];
		}
	      err = M2TXRaw_MapToPIP(&rawAlpha, &manyPIP, numColors,
				     floyd);
	      	      
	      /* err = M2TXRawAlpha_MapToPIP(&raw, newPIP, numColors,
		 floyd);
		 */
	      if (err != M2E_NoErr)
		{
		  fprintf(stderr,"ERROR:An error occured during ditherof \"%s\" lod=%d:%d\n", fileIn, lod, err);	
		  M2TXRaw_Free(&rawAlpha);
		  M2TXRaw_Free(&raw);
		  continue;
		}
	      for (curPixel=0; curPixel<pixels; curPixel++)
		{
		  raw.Alpha[curPixel] = rawAlpha.Red[curPixel]; 
		}
	      M2TXRaw_Free(&rawAlpha);
	      err = M2TXRaw_ToUncompr(&newTex, oldPIP, lod, &raw);
	    }
	  else
	    {
	      err = M2TXRaw_MapToPIP (&raw, &manyPIP, numColors, floyd);
	      if (err != M2E_NoErr)
		{
		  fprintf(stderr,"ERROR:An error occured during ditherof \"%s\" lod=%d:%d\n", fileIn, lod, err);
		  M2TXRaw_Free(&raw);
		  continue;
		}
	      err = M2TXRaw_ToUncompr(&newTex, &manyPIP, lod, &raw);
	    }
	  /* Write out the new quantized texture, changing nothing else */
	  if (err != M2E_NoErr)
	    {
	      fprintf(stderr,"ERROR:An error occured during encoding of image\"%s\" lod=%d : %d\n", fileIn, lod, err);
	      return(-1);
	    }

	  M2TXRaw_Free(&raw);
	}
      M2TX_WriteFile(fileIn, &newTex);
      M2TX_Free(&tex, TRUE, FALSE, FALSE, FALSE);
      M2TX_Free(&newTex, TRUE, FALSE, FALSE, FALSE);
    }

  M2TXHeader_SetFIsLiteral(newHeader, FALSE);
  M2TXHeader_SetFHasPIP(newHeader, TRUE);
  /* We need to do something special for Alpha here? */

  if(alpha)
    {
      for (i=0; i<numColors; i++)
	{
	  M2TXPIP_GetColor(&manyPIP, i, &color);
	  M2TXColor_Decode(color, &ssb, &opac, &red, &grn, &blu);
	  color = M2TXColor_Create(0, red, 0, 0, 0);
	  M2TXPIP_SetColor(&manyPIP, i, color);
	}
      M2TXPIP_SetNumColors(&manyPIP, numColors);
    }

  M2TX_Init(&newTex);			/* Initialize a texture */
  M2TX_SetPIP(&newTex, &manyPIP);
  M2TXHeader_SetNumLOD(newHeader, 0);
  M2TXHeader_SetFHasColor(newHeader, TRUE);
  M2TXHeader_SetFHasAlpha(newHeader, FALSE);
  M2TXHeader_SetFHasSSB(newHeader, FALSE);
  M2TXHeader_SetCDepth(newHeader, pipCDepth);
  M2TXHeader_SetADepth(newHeader, 0);
  M2TXHeader_SetFIsLiteral(newHeader, FALSE);
  M2TXHeader_SetFHasPIP(newHeader, TRUE);
  M2TX_SetPIP(&newTex, &manyPIP);
  make_legal(newHeader);
  M2TX_WriteFile(fileOut, &newTex);
  return(0);
}

